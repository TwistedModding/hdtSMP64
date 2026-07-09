#include "hdtPatternLibrary.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>

namespace hdt
{
	namespace
	{
		// Folder, relative to the game working directory, scanned for shared pattern-definition files.
		// Sibling of the deployed hdtSMP64.xsd/.sch, mirroring the defaultBBPs.xml data-file convention.
		constexpr const char* kPatternsDir = "data/skse/plugins/hdtSkinnedMeshConfigs/patterns";

		std::vector<PatternLibrary> g_libraries;
		std::once_flag g_once;

		std::string readWholeFile(const std::filesystem::path& path)
		{
			std::ifstream f(path, std::ios::binary);
			if (!f)
				return {};
			std::ostringstream ss;
			ss << f.rdbuf();
			return ss.str();
		}

		void loadOnce()
		{
			g_libraries = scanPatternDir(kPatternsDir);
		}
	}  // namespace

	// Reads every *.xml in `dir` in filename order, so a name clash across mods resolves deterministically
	// (later filename wins -- authors should still namespace with author= to avoid relying on order). A
	// missing folder simply yields no libraries; non-regular entries, non-.xml files, and unreadable/empty
	// files are skipped. Kept separate from loadOnce (which pins the global folder + call_once) so this
	// pure directory-to-libraries mapping can be exercised against a temp directory in tests.
	std::vector<PatternLibrary> scanPatternDir(const std::filesystem::path& dir)
	{
		namespace fs = std::filesystem;
		std::vector<PatternLibrary> libraries;
		std::error_code ec;
		if (!fs::is_directory(dir, ec))
			return libraries;

		std::vector<fs::path> files;
		for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
			if (!it->is_regular_file(ec))
				continue;
			std::string ext = it->path().extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			if (ext == ".xml")
				files.push_back(it->path());
		}
		std::sort(files.begin(), files.end());

		for (const fs::path& p : files) {
			std::string xml = readWholeFile(p);
			if (!xml.empty())
				libraries.push_back({ std::move(xml), p.filename().string() });
		}
		return libraries;
	}

	const std::vector<PatternLibrary>& getGlobalPatternLibraries()
	{
		std::call_once(g_once, loadOnce);
		return g_libraries;
	}
}
