// XMLReader stream-contract tests. These pin the reader semantics the physics-XML parser relies
// on -- in particular the mechanism behind the duplicate-<bone> fix (#404): reading attributes
// does not consume an element, an empty element <x/> is delivered as a StartTag followed by a
// synthetic EndTag, and skipCurrentElement() consumes an element's whole subtree INCLUDING its
// EndTag. A dispatch loop that breaks on EndTag (like createOrUpdateSystem's <system> loop)
// therefore only survives an unwanted element if the element is explicitly skipped -- which is
// exactly what readOrUpdateBone's duplicate path must do.

#include "test_pch.h"

#include "XmlReader.h"

#include <doctest/doctest.h>

#include <set>
#include <string>

namespace
{
	// The reader parses in place from a byte range; the caller keeps `xml` alive.
	hdt::XMLReader makeReader(const std::string& xml)
	{
		return hdt::XMLReader((BYTE*)xml.data(), xml.size());
	}
}

TEST_CASE("skipCurrentElement consumes children and the element's EndTag")
{
	const std::string xml =
		"<system><bone name=\"a\"><mass>1.0</mass><inertia x=\"1\" y=\"1\" z=\"1\"/></bone>"
		"<probe/></system>";
	auto reader = makeReader(xml);

	reader.nextStartElement();
	CHECK(reader.GetName() == "system");
	reader.nextStartElement();
	CHECK(reader.GetName() == "bone");
	CHECK(reader.getAttribute("name") == "a");

	reader.skipCurrentElement();
	reader.nextStartElement();
	CHECK(reader.GetName() == "probe");
}

TEST_CASE("attribute reads do not consume the element")
{
	// readOrUpdateBone reads name/template attributes before deciding what to do with
	// the element; the children must still be there afterwards.
	const std::string xml = "<system><bone name=\"a\"><mass>2.5</mass></bone></system>";
	auto reader = makeReader(xml);

	reader.nextStartElement();
	reader.nextStartElement();
	CHECK(reader.getAttribute("name") == "a");
	CHECK(reader.hasAttribute("name"));
	CHECK(reader.getAttribute("template", "fallback") == "fallback");

	reader.nextStartElement();
	CHECK(reader.GetName() == "mass");
}

TEST_CASE("an empty element delivers a synthetic EndTag right after its StartTag")
{
	// The immediate next token after <bone/>'s StartTag is its EndTag -- so a dispatch
	// loop that breaks on EndTag (mistaking it for its enclosing element's close) dies
	// right here unless the element is skipped. This is the truncation mechanism of #404.
	const std::string xml = "<system><bone name=\"X\"/><probe/></system>";
	auto reader = makeReader(xml);

	reader.nextStartElement();
	reader.nextStartElement();
	CHECK(reader.GetName() == "bone");
	CHECK(reader.GetInspected() == Xml::Inspected::StartTag);

	reader.Inspect();
	CHECK(reader.GetInspected() == Xml::Inspected::EndTag);
	CHECK(reader.GetName() == "bone");
}

TEST_CASE("skipCurrentElement works on an empty element")
{
	// The fixed duplicate path calls it unconditionally: the synthetic EndTag is
	// consumed and the next sibling follows.
	const std::string xml = "<system><bone name=\"X\"/><probe/></system>";
	auto reader = makeReader(xml);

	reader.nextStartElement();
	reader.nextStartElement();
	CHECK(reader.GetName() == "bone");

	reader.skipCurrentElement();
	reader.nextStartElement();
	CHECK(reader.GetName() == "probe");
}

TEST_CASE("a dispatch loop survives a duplicate <bone> when the duplicate is skipped")
{
	// End-to-end contract for the #404 fix, driven exactly like createOrUpdateSystem's
	// <system> loop: StartTags dispatch, the first EndTag breaks. With every <bone>
	// element skipped once handled -- including the duplicate -- the loop must still
	// reach the constraint that follows it.
	const std::string xml =
		"<system>"
		"<bone name=\"X\"/>"
		"<bone name=\"X\"><shape type=\"sphere\"/></bone>"
		"<generic-constraint bodyA=\"X\" bodyB=\"Y\"/>"
		"</system>";
	auto reader = makeReader(xml);
	reader.nextStartElement();
	CHECK(reader.GetName() == "system");

	std::set<std::string> seenBones;
	bool constraintReached = false;
	bool leakedShape = false;
	bool done = false;
	while (!done && reader.Inspect()) {
		switch (reader.GetInspected()) {
		case Xml::Inspected::StartTag:
			if (reader.GetName() == "bone") {
				seenBones.insert(reader.getAttribute("name"));
				reader.skipCurrentElement();  // both paths consume, as fixed by #404
			} else if (reader.GetName() == "generic-constraint") {
				constraintReached = true;
				reader.skipCurrentElement();
			} else if (reader.GetName() == "shape") {
				leakedShape = true;  // would mean the duplicate's child escaped
				reader.skipCurrentElement();
			}
			break;
		case Xml::Inspected::EndTag:
			done = true;  // </system> -- or a leaked EndTag, which the checks catch
			break;
		default:
			break;
		}
	}

	CHECK(constraintReached);
	CHECK(!leakedShape);
	CHECK(seenBones.size() == 1);
}

TEST_CASE("a dispatch loop truncates on a duplicate <bone> that is not skipped")
{
	// The same loop WITHOUT skipping the duplicate (the pre-#404 behaviour) demonstrates
	// the failure: the duplicate's <shape> child leaks to the dispatcher and the loop
	// breaks on the duplicate's EndTag before ever reaching the constraint. If this test
	// starts failing, the reader's consumption semantics changed and #404 needs a re-look.
	const std::string xml =
		"<system>"
		"<bone name=\"X\"/>"
		"<bone name=\"X\"><shape type=\"sphere\"/></bone>"
		"<generic-constraint bodyA=\"X\" bodyB=\"Y\"/>"
		"</system>";
	auto reader = makeReader(xml);
	reader.nextStartElement();

	std::set<std::string> seenBones;
	bool constraintReached = false;
	bool leakedShape = false;
	bool done = false;
	while (!done && reader.Inspect()) {
		switch (reader.GetInspected()) {
		case Xml::Inspected::StartTag:
			if (reader.GetName() == "bone") {
				const std::string name = reader.getAttribute("name");
				if (seenBones.insert(name).second)
					reader.skipCurrentElement();
				// duplicate: fall through WITHOUT consuming -- the old behaviour
			} else if (reader.GetName() == "generic-constraint") {
				constraintReached = true;
				reader.skipCurrentElement();
			} else if (reader.GetName() == "shape") {
				leakedShape = true;
				reader.skipCurrentElement();
			}
			break;
		case Xml::Inspected::EndTag:
			done = true;
			break;
		default:
			break;
		}
	}

	CHECK(leakedShape);
	CHECK(!constraintReached);
}
