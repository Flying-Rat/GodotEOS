#include "PlatformSubsystem.h"
#include "IPlatform.h"
#include "../eos_sdk/Include/eos_sdk.h"
#include "../eos_sdk/Include/eos_logging.h"
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

PlatformSubsystem::PlatformSubsystem() : platform_handle(nullptr), initialized(false), online(false) {}

PlatformSubsystem::~PlatformSubsystem() {
    Shutdown();
}

bool PlatformSubsystem::Init() {
    UtilityFunctions::print("PlatformSubsystem: Initializing...");

    // Get platform interface from the existing Platform instance
    IPlatform* platform_interface = IPlatform::get();
    if (!platform_interface) {
        UtilityFunctions::printerr("PlatformSubsystem: No Platform instance available");
        return false;
    }

    if (!platform_interface->is_initialized()) {
        UtilityFunctions::printerr("PlatformSubsystem: Platform not initialized");
        return false;
    }

    platform_handle = platform_interface->get_platform_handle();
    if (!platform_handle) {
        UtilityFunctions::printerr("PlatformSubsystem: Invalid platform handle");
        return false;
    }

    initialized = true;
    online = true; // Platform is initialized, so consider it online
    UtilityFunctions::print("PlatformSubsystem: Initialized successfully");
    return true;
}

void PlatformSubsystem::Tick(float delta_time) {
    if (platform_handle && initialized) {
        EOS_Platform_Tick(platform_handle);
    }
}

void PlatformSubsystem::Shutdown() {
    if (!initialized) {
        return;
    }

    if (platform_handle) {
        EOS_Platform_Release(platform_handle);
        platform_handle = nullptr;
    }

    EOS_Shutdown();
    initialized = false;
    online = false;
    UtilityFunctions::print("PlatformSubsystem: Shutdown complete");
}

bool PlatformSubsystem::InitializePlatform(
    const String& product_name,
    const String& product_version,
    const String& product_id,
    const String& sandbox_id,
    const String& deployment_id,
    const String& client_id,
    const String& client_secret,
    const String& encryption_key
) {
    if (initialized) {
        UtilityFunctions::print("PlatformSubsystem: Platform already initialized");
        return true;
    }

    UtilityFunctions::printerr("PlatformSubsystem: InitializePlatform called but platform should be initialized by Platform class");
    return false;
}

void* PlatformSubsystem::GetPlatformHandle() const {
    return platform_handle;
}

bool PlatformSubsystem::IsOnline() const {
    return initialized && online;
}

void PlatformSubsystem::SetLogLevel(int level) {
    if (!initialized) return;

    EOS_ELogLevel eos_level = EOS_ELogLevel::EOS_LOG_Off;
    switch (level) {
        case 0: eos_level = EOS_ELogLevel::EOS_LOG_Off; break;
        case 1: eos_level = EOS_ELogLevel::EOS_LOG_Error; break;
        case 2: eos_level = EOS_ELogLevel::EOS_LOG_Warning; break;
        case 3: eos_level = EOS_ELogLevel::EOS_LOG_Info; break;
        case 4: eos_level = EOS_ELogLevel::EOS_LOG_Verbose; break;
        default: eos_level = EOS_ELogLevel::EOS_LOG_Warning; break;
    }

    EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, eos_level);
    UtilityFunctions::print("PlatformSubsystem: Log level set to " + String::num_int64(level));
}

} // namespace godot