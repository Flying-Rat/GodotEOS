#include "Logger.h"

#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

String Logger::Format(const String& system, const String& message) {
	return String("[GodotEOS] ") + system + ": " + message;
}

void Logger::Error(const String& system, const String& message) {
	UtilityFunctions::printerr(Format(system, message));
}

void Logger::Warning(const String& system, const String& message) {
	UtilityFunctions::print(Format(system, message));
}

void Logger::Info(const String& system, const String& message) {
	UtilityFunctions::print(Format(system, message));
}

void Logger::Verbose(const String& system, const String& message) {
	UtilityFunctions::print(Format(system, message));
}

} // namespace godot
