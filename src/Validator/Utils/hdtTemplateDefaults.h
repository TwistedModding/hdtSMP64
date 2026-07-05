#pragma once

#include <pugixml.hpp>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hdt
{
	struct PatternSourceMap;  // fwd: maps expanded-doc offsets back to authoring source lines (pattern files)

	// Returns true if the tag is a default node (bone-default, generic-constraint-default, etc.)
	bool isDefaultNodeName(const std::string& localName);
	struct TemplateRedundantChildInfo
	{
		std::string location;
		std::string tagName;
		int line = 0;
		bool shadowedByLaterFrameTag = false;
		std::string shadowingTagName;
	};

	// Analyze one physics XML document and return detailed redundant child info.
	// When sourceBytes is provided, line numbers are computed from node offsets; when sourceMap is also
	// provided (a pattern-expanded document), each line is translated back to the author's source line.
	std::vector<TemplateRedundantChildInfo> CollectTemplateRedundantChildrenInfo(
		const pugi::xml_document& doc,
		const std::string* sourceBytes = nullptr,
		const PatternSourceMap* sourceMap = nullptr);

	// Analyze one physics XML document using runtime-like template semantics and
	// return locations of child tags that are redundant relative to effective defaults.
	std::unordered_set<std::string> CollectTemplateRedundantChildLocations(const pugi::xml_document& doc);

	// Remove child tags that are redundant relative to effective defaults.
	// Returns true when at least one child element was removed.
	bool RemoveTemplateRedundantChildren(pugi::xml_document& doc);

	// Remove top-level *-default nodes that are never referenced by any later
	// template/template-inheritance use in the same file.
	bool RemoveUnusedDefaultNodes(pugi::xml_document& doc);

	// Returns duplicate named default templates mapped to an earlier equivalent
	// named template using runtime-like effective template semantics.
	std::unordered_map<std::string, std::string> CollectEquivalentDefaultTemplateAliases(
		const pugi::xml_document& doc);

	// One top-level <bone> declaration the engine never uses: by the time the parser
	// reaches it, an earlier element in the same file has already claimed the same
	// (case-folded) bone name, so readOrUpdateBone skips it ("Bone X already exists,
	// skipped") — or, when the name resolves to no node, repeats the identical failed
	// lookup. Removing such a declaration cannot change behaviour.
	struct InertBoneInfo
	{
		std::string location;  // positional path, e.g. /system[1]/bone[5]
		std::string boneName;  // value of the bone's name attribute (for the message)
		int line = 0;
	};

	// Find top-level <bone> declarations that are inert — never acted on by the engine,
	// so removable with zero behaviour change — because an earlier same-file element
	// claims the same bone name first: an earlier <bone>, a constraint
	// bodyA/bodyB endpoint, or a can-/no-collide-with-bone shape reference — the engine
	// creates a bone at the first of these it reads, and every later <bone> of that name
	// is skipped. The walk follows document order (recursing into constraint-group) and
	// folds case like BSFixedString; renaming cannot break the equivalence because both
	// occurrences go through the same rename. Only certainty is reported: creators the
	// engine resolves through data outside this file (mesh skinning) are ignored, so the
	// walk under-reports rather than ever flagging a live declaration.
	// When sourceBytes is provided, line numbers are computed from offsets; when sourceMap is also provided
	// (a pattern-expanded document), each line is translated back to the author's source line.
	std::vector<InertBoneInfo> CollectInertBoneDeclarations(
		const pugi::xml_document& doc,
		const std::string* sourceBytes = nullptr,
		const PatternSourceMap* sourceMap = nullptr);

}  // namespace hdt
