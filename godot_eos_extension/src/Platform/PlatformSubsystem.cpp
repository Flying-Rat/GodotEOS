#include "PlatformSubsystem.h"
#include "../Utils/InitOptionsValidation.h"
#include <eos_sdk.h>
#include <eos_logging.h>
#include <eos_achievements.h>
#include <cstring>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

#ifdef EOS_PLATFORM_OPTIONS_ENCRYPTIONKEY_LENGTH
static_assert(kEosEncryptionKeyHexLength == EOS_PLATFORM_OPTIONS_ENCRYPTIONKEY_LENGTH);
#endif

namespace {

void EOS_CALL platform_logging_callback(const EOS_LogMessage* message) {
    if (!message || !message->Message) {
        return;
    }

    String log_text = String::utf8(message->Message);
    String category = message->Category ? String::utf8(message->Category) : "EOS";
    String log_msg = String("[") + category + "] " + log_text;

    switch (message->Level) {
        case EOS_ELogLevel::EOS_LOG_Fatal:
        case EOS_ELogLevel::EOS_LOG_Error:
            UtilityFunctions::printerr(log_msg);
            break;
        case EOS_ELogLevel::EOS_LOG_Warning:
            WARN_PRINT(log_msg);
            break;
        case EOS_ELogLevel::EOS_LOG_Info:
        case EOS_ELogLevel::EOS_LOG_Verbose:
        case EOS_ELogLevel::EOS_LOG_VeryVerbose:
        default:
            UtilityFunctions::print(log_msg);
            break;
    }
}

} // namespace

PlatformSubsystem::PlatformSubsystem() : platform_handle(nullptr), initialized(false), online(false) {}

PlatformSubsystem::~PlatformSubsystem() {
    Shutdown();
}

bool PlatformSubsystem::Init() {
    UtilityFunctions::print("PlatformSubsystem: Initializing...");
    // PlatformSubsystem now handles its own initialization
    // The actual EOS SDK initialization happens in initialize() method
    initialized = true;
    online = true;
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

bool PlatformSubsystem::InitializePlatform(const EpicInitOptions& options) {
    if (initialized && platform_handle) {
        UtilityFunctions::printerr("EOS Platform already initialized");
        return true;
    }

    // Initialize EOS SDK
    EOS_InitializeOptions InitOptions = {};
    InitOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
    InitOptions.AllocateMemoryFunction = nullptr;  // Use default
    InitOptions.ReallocateMemoryFunction = nullptr;
    InitOptions.ReleaseMemoryFunction = nullptr;
    // Keep CharString temporaries alive for the duration of the call so EOS receives valid pointers
    godot::CharString product_name_cs = options.product_name.utf8();
    godot::CharString product_version_cs = options.product_version.utf8();
    InitOptions.ProductName = product_name_cs.get_data();
    InitOptions.ProductVersion = product_version_cs.get_data();
    InitOptions.Reserved = nullptr;
    InitOptions.SystemInitializeOptions = nullptr;

    // Sanity checks before calling EOS_Initialize
    if (!InitOptions.ProductName || strlen(InitOptions.ProductName) == 0) {
        UtilityFunctions::printerr("InitOptions.ProductName is empty or null");
        return false;
    }
    if (!InitOptions.ProductVersion || strlen(InitOptions.ProductVersion) == 0) {
        UtilityFunctions::printerr("InitOptions.ProductVersion is empty or null");
        return false;
    }

    EOS_EResult InitResult = EOS_Initialize(&InitOptions);
    if (InitResult != EOS_EResult::EOS_Success) {
        const char* result_str = EOS_EResult_ToString(InitResult);
        String error_msg = "Failed to initialize EOS SDK: " + String(result_str) + " (" + String::num_int64(static_cast<int64_t>(InitResult)) + ")";
        UtilityFunctions::printerr(error_msg);
        return false;
    }

    // Logging must be registered after EOS_Initialize so SDK errors from Platform_Create are visible.
    EOS_EResult log_result = EOS_Logging_SetCallback(platform_logging_callback);
    if (log_result != EOS_EResult::EOS_Success) {
        UtilityFunctions::printerr("Failed to set EOS logging callback: " + String(EOS_EResult_ToString(log_result)) + " (" + String::num_int64(static_cast<int64_t>(log_result)) + ")");
    } else {
        EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, EOS_ELogLevel::EOS_LOG_Verbose);
    }

    String encryption_key_error = ValidateEncryptionKey(options.encryption_key);
    if (!encryption_key_error.is_empty()) {
        UtilityFunctions::printerr(encryption_key_error);
        EOS_Shutdown();
        return false;
    }

    // Create platform instance using provided init options (keep CharString temporaries alive)
    EOS_Platform_Options PlatformOptions = {};
    PlatformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
    PlatformOptions.bIsServer = false;

    // Keep CharString temporaries alive for platform option strings
    godot::CharString product_id_cs = options.product_id.utf8();
    godot::CharString sandbox_id_cs = options.sandbox_id.utf8();
    godot::CharString deployment_id_cs = options.deployment_id.utf8();
    godot::CharString client_id_cs = options.client_id.utf8();
    godot::CharString client_secret_cs = options.client_secret.utf8();
    godot::CharString encryption_key_cs;

    PlatformOptions.ProductId = product_id_cs.get_data();
    PlatformOptions.SandboxId = sandbox_id_cs.get_data();
    PlatformOptions.DeploymentId = deployment_id_cs.get_data();
    PlatformOptions.ClientCredentials.ClientId = client_id_cs.get_data();
    PlatformOptions.ClientCredentials.ClientSecret = client_secret_cs.get_data();
    // Empty encryption_key must be nullptr. Passing "" makes EOS_Platform_Create fail.
    if (options.encryption_key.is_empty()) {
        PlatformOptions.EncryptionKey = nullptr;
    } else {
        encryption_key_cs = options.encryption_key.utf8();
        PlatformOptions.EncryptionKey = encryption_key_cs.get_data();
    }
    PlatformOptions.OverrideCountryCode = nullptr;
    PlatformOptions.OverrideLocaleCode = nullptr;

    platform_handle = EOS_Platform_Create(&PlatformOptions);
    if (!platform_handle) {
        UtilityFunctions::printerr("Failed to create EOS Platform (EOS_Platform_Create returned nullptr).");
        UtilityFunctions::printerr("Check the EOS log lines above for the SDK's specific reason.");
        UtilityFunctions::printerr("If no EOS log named a field, verify each value from the Epic Developer Portal:");
        UtilityFunctions::printerr("- product_id, sandbox_id, and deployment_id belong to the same product/sandbox");
        UtilityFunctions::printerr("- client_id and client_secret belong to that product");
        UtilityFunctions::printerr("- encryption_key is omitted, or exactly 64 hexadecimal characters");
        EOS_Shutdown();
        return false;
    }

    initialized = true;
    online = true;
    // IPlatform::set(this); // Removed - using subsystem architecture instead
    WARN_PRINT("EOS Platform initialized successfully");
    return true;
}

EOS_HPlatform PlatformSubsystem::GetPlatformHandle() const {
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
}

} // namespace godot