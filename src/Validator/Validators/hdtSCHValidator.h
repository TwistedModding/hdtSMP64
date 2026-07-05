#pragma once

#include <string>
#include <vector>

namespace hdt
{
	struct PhysicsXmlSource;  // fwd: a shared read+expand result the caller may pass to reuse across validators

	enum class SCHRole
	{
		Warning,
		Error,
	};

	struct SCHViolation
	{
		std::string xmlPath;
		std::string location;  // e.g. "/system[1]/bone[1]/linearDamping[1]"
		std::string message;
		SCHRole role = SCHRole::Warning;
		int line = 0;  // 1-based source line number, 0 if unknown
	};

	struct SCHValidationResult
	{
		std::vector<SCHViolation> violations;
		bool hasErrors = false;
		bool hasWarnings = false;
	};

	// Validate an FSMP physics XML file against the Schematron rules in hdtSMP64.sch.
	// Rules are loaded once from disk at first call and cached for subsequent calls.
	// Pass `precomputed` to reuse a shared read+expand across validators; null reads and expands here.
	SCHValidationResult ValidatePhysicsXMLWithSchematron(
		const std::string& xmlPath, const PhysicsXmlSource* precomputed = nullptr);

}  // namespace hdt
