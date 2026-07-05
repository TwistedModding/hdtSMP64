// Unit tests for the shared pattern-library disk scan (hdtPatternLibrary::scanPatternDir). These exercise
// the filesystem behaviour getGlobalPatternLibraries relies on -- case-insensitive *.xml filtering,
// deterministic filename order, and skipping non-xml / unreadable / empty files -- against a temp
// directory, without touching the global patterns/ folder. Pure STL + std::filesystem.

#include "Patterns/hdtPatternLibrary.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>

TEST_CASE("scanPatternDir reads *.xml in filename order, case-insensitively, skipping non-xml and empty")
{
	namespace fs = std::filesystem;
	const fs::path dir = fs::temp_directory_path() / "fsmp_pattern_scan_test";
	fs::remove_all(dir);
	fs::create_directories(dir);
	const auto write = [&](const char* name, const std::string& content) {
		std::ofstream(dir / name, std::ios::binary) << content;
	};
	// Digit-prefixed names so the expected order is unambiguous regardless of case-sensitivity.
	write("01_alpha.xml", "<patterns/>");
	write("02_beta.XML", "<patterns/>");   // uppercase extension still counts as .xml
	write("03_gamma.txt", "<patterns/>");  // not .xml -> skipped
	write("04_delta.xml", "");             // empty -> skipped
	write("05_epsilon.xml", "<patterns/>");

	const auto libs = hdt::scanPatternDir(dir);
	fs::remove_all(dir);

	REQUIRE(libs.size() == 3);
	CHECK(libs[0].origin == "01_alpha.xml");
	CHECK(libs[1].origin == "02_beta.XML");
	CHECK(libs[2].origin == "05_epsilon.xml");
	CHECK(libs[0].xml == "<patterns/>");
}

TEST_CASE("scanPatternDir on a missing directory yields no libraries")
{
	namespace fs = std::filesystem;
	const auto libs = hdt::scanPatternDir(fs::temp_directory_path() / "fsmp_scan_dir_that_does_not_exist_zzz");
	CHECK(libs.empty());
}
