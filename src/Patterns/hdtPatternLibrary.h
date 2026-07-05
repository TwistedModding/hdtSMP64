#pragma once

#include "hdtXmlPatternExpander.h"

#include <filesystem>
#include <vector>

namespace hdt
{
	/// Loads and caches the shared pattern definitions from the global patterns/ folder
	/// (data/skse/plugins/hdtSkinnedMeshConfigs/patterns/*.xml), in deterministic filename order. Built
	/// once on first call and shared thereafter. Pass the result to expandPatterns via
	/// PatternOptions::libraries so the runtime loader and every validator resolve the same cross-mod
	/// patterns. Authors drop a file of <pattern-default>s here (namespaced with author=) to publish
	/// patterns other mods can use.
	const std::vector<PatternLibrary>& getGlobalPatternLibraries();

	/// Scans `dir` for *.xml files (case-insensitive extension) in deterministic filename order, reading
	/// each non-empty one into a PatternLibrary (origin = filename). A missing directory, a non-regular
	/// entry, a non-.xml file, and an unreadable/empty file are all skipped silently rather than erroring.
	/// getGlobalPatternLibraries() runs this once over the global folder; it is exposed so the disk-scan
	/// behaviour can be unit-tested against a temporary directory.
	std::vector<PatternLibrary> scanPatternDir(const std::filesystem::path& dir);
}
