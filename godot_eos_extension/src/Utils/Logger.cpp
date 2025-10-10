#include "Logger.h"
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

// Static member initialization
bool Logger::debug_mode = false;

void Logger::SetDebugMode(bool enabled) {
    debug_mode = enabled;
    if (enabled) {
        UtilityFunctions::print("[Logger] Debug mode enabled - showing all logs");
    } else {
        UtilityFunctions::print("[Logger] Debug mode disabled - showing errors and warnings only");
    }
}

bool Logger::IsDebugMode() {
    return debug_mode;
}

void Logger::Error(const String& message) {
    UtilityFunctions::printerr(message);
}

void Logger::Warning(const String& message) {
    UtilityFunctions::push_warning(message);
}

void Logger::Info(const String& message) {
    if (debug_mode) {
        UtilityFunctions::print(message);
    }
}

void Logger::Verbose(const String& message) {
    if (debug_mode) {
        UtilityFunctions::print(message);
    }
}

void Logger::Log(LogLevel level, const String& message) {
    switch (level) {
        case LogLevel::Error:
            Error(message);
            break;
        case LogLevel::Warning:
            Warning(message);
            break;
        case LogLevel::Info:
            Info(message);
            break;
        case LogLevel::Verbose:
            Verbose(message);
            break;
    }
}

} // namespace godot
