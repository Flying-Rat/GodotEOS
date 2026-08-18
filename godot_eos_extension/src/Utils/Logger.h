#pragma once

#include <godot_cpp/variant/string.hpp>

namespace godot {

class Logger {
public:
	static String Format(const String& system, const String& message);
	static void Error(const String& system, const String& message);
	static void Warning(const String& system, const String& message);
	static void Info(const String& system, const String& message);
	static void Verbose(const String& system, const String& message);
};

} // namespace godot
