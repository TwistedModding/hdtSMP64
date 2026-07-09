#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hdt
{
	struct PhysicsXmlSource;  // fwd: a shared read+expand result the caller may pass to reuse across validators

	struct XSDViolation
	{
		std::string xmlPath;
		int line = 0;
		int column = 0;
		std::string elementPath;
		std::string message;
	};

	struct XSDValidationResult
	{
		bool isValid = true;
		bool schemaSkipped = false;  // true when the XSD schema was unavailable; isValid is set to true but no real validation occurred
		std::vector<XSDViolation> violations;
	};

	// Validate an FSMP physics XML file against the hdtSMP64 XSD constraints.
	// Checks required attributes, valid enum values, and cross-references within the XML.
	// Pass `precomputed` to reuse a shared read+expand across validators; null reads and expands here.
	XSDValidationResult ValidatePhysicsXMLWithXSD(
		const std::string& xmlPath, const PhysicsXmlSource* precomputed = nullptr);

	// Access to the parsed XSD schema, for use by hdtXMLImprover.
	// Thread-safe: schema is loaded once on first access via std::call_once.
	const std::unordered_map<std::string, std::unordered_set<std::string>>& GetSchemaAllowedChildren();
	const std::unordered_set<std::string>& GetSchemaKnownElements();

}  // namespace hdt
