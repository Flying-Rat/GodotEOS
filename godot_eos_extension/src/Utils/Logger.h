#pragma once

#include <godot_cpp/variant/string.hpp>

namespace godot {

/**
 * @brief Centralized logging utility that respects debug mode settings.
 * 
 * Provides consistent logging across the GDExtension with automatic
 * filtering based on debug mode (verbose vs minimal logging).
 */
class Logger {
public:
    enum class LogLevel {
        Error,      // Always shown (printerr)
        Warning,    // Always shown (push_warning)
        Info,       // Only in debug mode (print)
        Verbose     // Only in debug mode (print)
    };

    /**
     * @brief Set the debug mode for logging.
     * @param enabled If true, shows all logs. If false, only errors and warnings.
     */
    static void SetDebugMode(bool enabled);

    /**
     * @brief Get current debug mode state.
     * @return true if debug mode is enabled.
     */
    static bool IsDebugMode();

    /**
     * @brief Log an error message (always shown).
     * @param message The error message to log.
     */
    static void Error(const String& message);

    /**
     * @brief Log a warning message (always shown).
     * @param message The warning message to log.
     */
    static void Warning(const String& message);

    /**
     * @brief Log an info message (only in debug mode).
     * @param message The info message to log.
     */
    static void Info(const String& message);

    /**
     * @brief Log a verbose message (only in debug mode).
     * @param message The verbose message to log.
     */
    static void Verbose(const String& message);

    /**
     * @brief Log a message with specific level.
     * @param level The log level.
     * @param message The message to log.
     */
    static void Log(LogLevel level, const String& message);

private:
    static bool debug_mode;
};

} // namespace godot
