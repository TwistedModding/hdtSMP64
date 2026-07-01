#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace RE
{
	class NiNode;
	class NiAVObject;
}

namespace hdt
{
	// A single physics-XML node reference that does not resolve to any node in the
	// skeleton the XML is applied to. `usedAsBone`/`constraintRefs` record how the XML
	// reaches the node so the report can state the concrete effect of its absence.
	struct MissingBoneRef
	{
		std::string referencedName;  // name exactly as written in the XML (before renameMap)
		std::string resolvedName;    // name after renameMap — what SMP looks up; == referencedName when unmapped
		bool usedAsBone = false;     // referenced by at least one <bone name="…"> definition
		int constraintRefs = 0;      // count of constraint endpoints (bodyA/bodyB) referencing it
	};

	// Returns the physics-XML node references that do NOT resolve to any node in
	// `skeletonRoot` — the NPC node SMP resolves bones against at load time, which already
	// contains the equipped item's merged + renamed nodes.
	//
	// Algorithm: (1) read+parse the XML; (2) walk every element, gathering each <bone>'s
	// `name` and each generic-/stiffspring-/conetwist-constraint's `bodyA`/`bodyB`; (3) push
	// each reference through `renameMap` (mirroring SkyrimSystemCreator::getRenamedBone) and
	// resolve it live against `skeletonRoot` the exact way the engine's findObjectByName does
	// — GetObjectByName over the whole subtree, then AsNode — keeping the ones that do NOT
	// bind to a NiNode, merged per resolved name and sorted by it. Resolving live (rather than
	// diffing against a pre-collected NiNode-only name set) is what makes the report agree with
	// runtime: it finds a NiNode below a non-NiNode parent, folds case, and rejects VR stubs
	// exactly as the engine does — see issue #402.
	//
	// `meshRoot` is the equipped item's 3D (armor/headpart). A reference naming a node absent
	// from the skeleton but skinned by that mesh is NOT reported: the engine creates a body for
	// it from skinInstance->bones[] without any name lookup. This escape covers BOTH <bone>
	// declarations and constraint endpoints. For a constraint it can miss a real drop (the
	// endpoint's skin body may be built after the constraint in document order), which is an
	// accepted trade under issue #402's policy — zero false positives, missed positives
	// tolerated — since a spurious "absent" wastes authors' and users' time. All name matching
	// (skeleton, skin set, rename) is case-insensitive, mirroring the engine's BSFixedString
	// comparisons. `meshRoot` may be null (no skin escape then). See issue #402.
	//
	// A returned entry means SMP would also fail the lookup and therefore silently skip
	// the bone / drop the constraint. The "-default" template element variants are ignored
	// because their `name`/`bodyA`/`bodyB` carry template class names, not node references.
	// Returns empty when the XML is missing or unparsable: reporting bad XML belongs to the
	// schema validator, which runs over the same equipped XMLs. `renameMap` may be empty.
	std::vector<MissingBoneRef> FindMissingPhysicsXmlBoneRefs(
		RE::NiNode* skeletonRoot,
		RE::NiAVObject* meshRoot,
		const std::string& xmlPath,
		const std::unordered_map<std::string, std::string>& renameMap);
}
