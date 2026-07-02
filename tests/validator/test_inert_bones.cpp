// CollectInertBoneDeclarations tests -- the zero-false-positive inert-<bone> warning. These
// exercise hdtTemplateDefaults.cpp and pugixml only -- no Bullet / CommonLibSSE.
//
// The predicate under test: a top-level <bone> is flagged iff an EARLIER element in the same
// file already claims the same case-folded bone name (an earlier <bone>, a constraint
// bodyA/bodyB -- also inside constraint-group -- or a can-/no-collide-with-bone shape
// reference). Every "not flagged" case here is a false-positive guard; every "flagged" case is
// a claim the engine provably skips ("Bone X already exists, skipped").

#include "Validator/Utils/hdtTemplateDefaults.h"

#include <doctest/doctest.h>
#include <pugixml.hpp>

#include <string>
#include <vector>

namespace
{
	std::vector<hdt::InertBoneInfo> collect(const std::string& xml, bool withSource = false)
	{
		pugi::xml_document doc;
		if (!doc.load_buffer(xml.data(), xml.size()))
			return {};
		return hdt::CollectInertBoneDeclarations(doc, withSource ? &xml : nullptr);
	}
}

TEST_CASE("a duplicate <bone> flags the second declaration only")
{
	auto hits = collect(
		"<system>"
		"<bone name=\"NPC L Breast\"/>"
		"<bone name=\"NPC L Breast\"/>"
		"</system>");
	REQUIRE(hits.size() == 1);
	CHECK(hits[0].boneName == "NPC L Breast");
	CHECK(hits[0].location == "/system[1]/bone[2]");
}

TEST_CASE("duplicate detection is case-folded like BSFixedString")
{
	auto hits = collect(
		"<system>"
		"<bone name=\"NPC L Breast\"/>"
		"<bone name=\"npc l breast\"/>"
		"</system>");
	CHECK(hits.size() == 1);
}

TEST_CASE("three declarations of one name flag two")
{
	auto hits = collect(
		"<system>"
		"<bone name=\"X\"/><bone name=\"X\"/><bone name=\"X\"/>"
		"</system>");
	CHECK(hits.size() == 2);
}

TEST_CASE("a constraint bodyA before the <bone> claims the name")
{
	auto hits = collect(
		"<system>"
		"<generic-constraint bodyA=\"X\" bodyB=\"Y\"/>"
		"<bone name=\"X\"/>"
		"</system>");
	CHECK(hits.size() == 1);
}

TEST_CASE("a constraint bodyB before the <bone> claims the name")
{
	auto hits = collect(
		"<system>"
		"<stiffspring-constraint bodyA=\"A\" bodyB=\"X\"/>"
		"<bone name=\"X\"/>"
		"</system>");
	CHECK(hits.size() == 1);
}

TEST_CASE("a constraint nested in constraint-group claims too")
{
	auto hits = collect(
		"<system>"
		"<constraint-group><conetwist-constraint bodyA=\"X\" bodyB=\"Y\"/></constraint-group>"
		"<bone name=\"X\"/>"
		"</system>");
	CHECK(hits.size() == 1);
}

TEST_CASE("can-collide-with-bone in a per-vertex-shape claims")
{
	auto hits = collect(
		"<system>"
		"<per-vertex-shape name=\"MyMesh\"><can-collide-with-bone>X</can-collide-with-bone></per-vertex-shape>"
		"<bone name=\"X\"/>"
		"</system>");
	CHECK(hits.size() == 1);
}

TEST_CASE("no-collide-with-bone in a per-triangle-shape claims")
{
	auto hits = collect(
		"<system>"
		"<per-triangle-shape name=\"MyMesh\"><no-collide-with-bone>X</no-collide-with-bone></per-triangle-shape>"
		"<bone name=\"X\"/>"
		"</system>");
	CHECK(hits.size() == 1);
}

TEST_CASE("a <bone> before the elements referencing it is the live declaration, not flagged")
{
	auto hits = collect(
		"<system>"
		"<bone name=\"X\"/>"
		"<generic-constraint bodyA=\"X\" bodyB=\"Y\"/>"
		"<per-vertex-shape name=\"M\"><can-collide-with-bone>X</can-collide-with-bone></per-vertex-shape>"
		"</system>");
	CHECK(hits.empty());
}

TEST_CASE("-default template elements do not claim")
{
	// Their name/bodyA/bodyB carry template class names, not skeleton nodes, and
	// defining them creates no bone.
	auto hits = collect(
		"<system>"
		"<bone-default name=\"X\"/>"
		"<generic-constraint-default bodyA=\"X\" bodyB=\"X\"/>"
		"<per-vertex-shape-default name=\"X\"><can-collide-with-bone>X</can-collide-with-bone></per-vertex-shape-default>"
		"<bone name=\"X\"/>"
		"</system>");
	CHECK(hits.empty());
}

TEST_CASE("weight-threshold does not claim")
{
	// Its bone attribute only matches already-skinned bones at runtime -- it never
	// creates one.
	auto hits = collect(
		"<system>"
		"<per-vertex-shape name=\"M\"><weight-threshold bone=\"X\">0.1</weight-threshold></per-vertex-shape>"
		"<bone name=\"X\"/>"
		"</system>");
	CHECK(hits.empty());
}

TEST_CASE("empty or missing name attributes are ignored")
{
	auto hits = collect(
		"<system>"
		"<bone name=\"\"/><bone name=\"\"/><bone/><bone/>"
		"</system>");
	CHECK(hits.empty());
}

TEST_CASE("a document without a <system> root yields nothing")
{
	auto hits = collect("<notsystem><bone name=\"X\"/><bone name=\"X\"/></notsystem>");
	CHECK(hits.empty());
}

TEST_CASE("line numbers are computed from the source bytes")
{
	const std::string xml =
		"<system>\n"
		"<bone name=\"X\"/>\n"
		"<bone name=\"X\"/>\n"
		"</system>\n";
	auto hits = collect(xml, true);
	REQUIRE(hits.size() == 1);
	CHECK(hits[0].line == 3);
}
