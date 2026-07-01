#include "hdtNIFBoneRefValidator.h"

#include "../Utils/hdtTemplateDefaults.h"  // isDefaultNodeName
#include "../Utils/hdtValidatorFamily.h"   // familyForNode
#include "NetImmerseUtils.h"               // readAllFile2, castNiNode

#include <pugixml.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <unordered_set>

namespace hdt
{
	namespace
	{
		// ASCII lower-case, mirroring the engine's case-insensitive BSFixedString name matching
		// so a name written in a different case than the skeleton/mesh uses still compares equal.
		std::string toLowerAscii(std::string s)
		{
			for (char& c : s)
				c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			return s;
		}

		// Mirror of SkyrimSystemCreator::getRenamedBone: a mapped name resolves to its
		// merged-skeleton form, an unmapped name is looked up verbatim. Case-insensitive like
		// getRenamedBone's BSFixedString lookup — `renameMap` must have lower-cased keys.
		std::string applyRename(const std::string& name,
			const std::unordered_map<std::string, std::string>& renameMap)
		{
			auto it = renameMap.find(toLowerAscii(name));
			return it == renameMap.end() ? name : it->second;
		}

		// Mirror of SkyrimSystemCreator::findObjectByName (the runtime bone binder): look the
		// resolved name up the exact way the engine does — GetObjectByName over the whole NPC
		// subtree, then require the hit to be a NiNode. We resolve live against the skeleton
		// instead of pre-collecting a NiNode-only name set (the old approach) so the report
		// agrees with runtime in the cases that set missed: a named NiNode hanging *below* a
		// non-NiNode parent is still found (GetObjectByName recurses the full tree, the old
		// NiNode-only recursion stopped at the non-node parent), the match is case-folded like
		// BSFixedString, and VR NiStream stubs are rejected by castNiNode exactly as the engine
		// fails to bind them. So a reference is called "absent" iff findObjectByName would also
		// return null and the engine would drop it — no false "absent" for a bone SMP resolves.
		// (Limitation, see issue #402: a name that exists only as a mesh-skin bone — created by
		// generateMeshBody from skinInstance->bones[] with no name lookup — is not in the NPC
		// tree; the caller suppresses those via the mesh skin-bound set (this stays skeleton-only).)
		bool resolvesToSkeletonNode(RE::NiNode* skeletonRoot, const std::string& resolvedName)
		{
			if (!skeletonRoot || resolvedName.empty())
				return false;
			return castNiNode(skeletonRoot->GetObjectByName(RE::BSFixedString(resolvedName.c_str()))) != nullptr;
		}

		// Collect the names of every bone the equipped mesh is skinned to. The engine's
		// generateMeshBody creates a body for each of these straight from skinInstance->bones[]
		// (live NiNode pointers) with NO name lookup against the skeleton — so a name reachable
		// only this way is a real body at runtime even though findObjectByName(m_skeleton) can't
		// resolve it. Walk mirrors validateSkinningSubtree (BSTriShape covers BSDynamicTriShape).
		void collectSkinBoundBoneNames(RE::NiAVObject* obj, std::unordered_set<std::string>& out)
		{
			if (!obj)
				return;
			if (RE::BSTriShape* triShape = castBSTriShape(obj)) {
				auto& runtimeData = triShape->GetGeometryRuntimeData();
				if (runtimeData.skinInstance) {
					RE::NiSkinInstance* skinInst = runtimeData.skinInstance.get();
					RE::NiSkinData* skinData = skinInst->skinData ? skinInst->skinData.get() : nullptr;
					if (skinData && skinInst->bones) {
						const std::uint32_t boneCount = skinData->GetBoneCount();
						for (std::uint32_t i = 0; i < boneCount; ++i) {
							auto bone = skinInst->bones[i];
							if (bone && bone->name.size())
								out.insert(toLowerAscii(bone->name.c_str()));
						}
					}
				}
			}
			if (RE::NiNode* node = castNiNode(obj)) {
				for (auto& child : node->GetChildren())
					if (child)
						collectSkinBoundBoneNames(child.get(), out);
			}
		}

		// Per-resolved-name accumulator: how many times, and in what roles, the XML reaches
		// a node that the skeleton does not provide.
		struct MissingAcc
		{
			std::string referenced;  // first-seen written form
			bool usedAsBone = false;
			int constraintRefs = 0;
		};

		// Depth-first walk over every element, classifying bone/constraint references.
		// Recursion (rather than first-level children) is needed because constraints may be
		// nested inside <constraint-group> and bones/shapes are siblings under <system>.
		void collectReferences(pugi::xml_node node,
			RE::NiNode* skeletonRoot,
			const std::unordered_set<std::string>& skinBoundBones,
			const std::unordered_map<std::string, std::string>& renameMap,
			std::unordered_map<std::string, MissingAcc>& missingByResolved)
		{
			auto record = [&](const char* written, bool asBone) {
				if (!written || written[0] == '\0')
					return;
				std::string resolved = applyRename(written, renameMap);
				if (resolvesToSkeletonNode(skeletonRoot, resolved))
					return;  // the engine's findObjectByName would bind it — not absent
				// A node absent from the skeleton but skinned by the mesh is still turned into a
				// body by generateMeshBody (from skinInstance->bones[], no name lookup), so it is
				// not "absent". We suppress it for BOTH <bone> declarations and constraint
				// endpoints. For a constraint this can miss a real drop — the endpoint's skin body
				// may be built after the constraint in document order, so the engine drops the
				// constraint — but that is an accepted trade under issue #402's policy: zero false
				// positives (never a spurious "absent" that sends an author chasing a non-problem),
				// missed positives tolerated. Names compared lower-cased (see toLowerAscii).
				if (skinBoundBones.count(toLowerAscii(resolved)))
					return;
				auto& acc = missingByResolved[resolved];
				if (acc.referenced.empty())
					acc.referenced = written;
				if (asBone)
					acc.usedAsBone = true;
				else
					++acc.constraintRefs;
			};

			for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling()) {
				if (child.type() != pugi::node_element)
					continue;
				// Classify by element family, but skip "-default" template nodes: their
				// name/bodyA/bodyB carry a template class name, not a skeleton node.
				const std::string tag = child.name();
				if (!isDefaultNodeName(tag)) {
					switch (familyForNode(tag)) {
					case Family::Bone:
						record(child.attribute("name").value(), true);
						break;
					case Family::Generic:
					case Family::StiffSpring:
					case Family::ConeTwist:
						record(child.attribute("bodyA").value(), false);
						record(child.attribute("bodyB").value(), false);
						break;
					default:
						break;
					}
				}
				collectReferences(child, skeletonRoot, skinBoundBones, renameMap, missingByResolved);
			}
		}
	}  // namespace

	std::vector<MissingBoneRef> FindMissingPhysicsXmlBoneRefs(
		RE::NiNode* skeletonRoot,
		RE::NiAVObject* meshRoot,
		const std::string& xmlPath,
		const std::unordered_map<std::string, std::string>& renameMap)
	{
		// A missing/malformed XML yields no findings on purpose: reporting bad XML is the
		// schema validator's job, and it runs over these same equipped XMLs in the report.
		std::string bytes = readAllFile2(xmlPath.c_str());
		pugi::xml_document doc;
		if (!doc.load_buffer(bytes.data(), bytes.size()))
			return {};

		// Bones the equipped mesh is skinned to count as present even when absent from the
		// skeleton — the engine fabricates a body for them from the mesh skin with no name
		// lookup. meshRoot may be null (then the set is simply empty). Stored lower-cased.
		std::unordered_set<std::string> skinBoundBones;
		collectSkinBoundBoneNames(meshRoot, skinBoundBones);

		// Lower-case the rename keys so applyRename mirrors getRenamedBone's case-insensitive
		// BSFixedString lookup (values keep their case; the skeleton lookup case-folds anyway).
		std::unordered_map<std::string, std::string> renameLc;
		renameLc.reserve(renameMap.size());
		for (const auto& [k, v] : renameMap)
			renameLc.emplace(toLowerAscii(k), v);

		std::unordered_map<std::string, MissingAcc> missingByResolved;
		collectReferences(doc, skeletonRoot, skinBoundBones, renameLc, missingByResolved);

		std::vector<MissingBoneRef> missing;
		missing.reserve(missingByResolved.size());
		for (auto& [resolved, acc] : missingByResolved) {
			MissingBoneRef m;
			m.referencedName = acc.referenced;
			m.resolvedName = resolved;
			m.usedAsBone = acc.usedAsBone;
			m.constraintRefs = acc.constraintRefs;
			missing.push_back(std::move(m));
		}
		// Deterministic report ordering.
		std::sort(missing.begin(), missing.end(),
			[](const MissingBoneRef& a, const MissingBoneRef& b) { return a.resolvedName < b.resolvedName; });

		return missing;
	}
}  // namespace hdt
