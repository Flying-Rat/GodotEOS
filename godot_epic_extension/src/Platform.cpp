#include "Platform.h"
#include "IPlatform.h"
#include "../eos_sdk/Include/eos_sdk.h"
#include "../eos_sdk/Include/eos_achievements.h"
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

Platform::Platform() : platform_handle(nullptr), initialized(false) {}

Platform::~Platform() {
    shutdown();
}

bool Platform::initialize(const EpicInitOptions& options) {
    if (initialized) {
        ERR_PRINT("EOS Platform already initialized");
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
        ERR_PRINT("InitOptions.ProductName is empty or null");
        return false;
    }
    if (!InitOptions.ProductVersion || strlen(InitOptions.ProductVersion) == 0) {
        ERR_PRINT("InitOptions.ProductVersion is empty or null");
        return false;
    }

    EOS_EResult InitResult = EOS_Initialize(&InitOptions);
    if (InitResult != EOS_EResult::EOS_Success) {
        const char* result_str = EOS_EResult_ToString(InitResult);
        String error_msg = "Failed to initialize EOS SDK: " + String(result_str) + " (" + String::num_int64(static_cast<int64_t>(InitResult)) + ")";
        ERR_PRINT(error_msg);
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
    godot::CharString encryption_key_cs = options.encryption_key.utf8();

    PlatformOptions.ProductId = product_id_cs.get_data();
    PlatformOptions.SandboxId = sandbox_id_cs.get_data();
    PlatformOptions.DeploymentId = deployment_id_cs.get_data();
    PlatformOptions.ClientCredentials.ClientId = client_id_cs.get_data();
    PlatformOptions.ClientCredentials.ClientSecret = client_secret_cs.get_data();
    PlatformOptions.EncryptionKey = encryption_key_cs.get_data();
    PlatformOptions.OverrideCountryCode = nullptr;
    PlatformOptions.OverrideLocaleCode = nullptr;

    // Log platform options (mask secrets)
    String platform_log = String("Creating EOS Platform with: \n");
    platform_log += String("  ProductId: ");
    platform_log += String(PlatformOptions.ProductId ? PlatformOptions.ProductId : "(null)");
    platform_log += String("\n");

    platform_log += String("  SandboxId: ");
    platform_log += String(PlatformOptions.SandboxId ? PlatformOptions.SandboxId : "(null)");
    platform_log += String("\n");

    platform_log += String("  DeploymentId: ");
    platform_log += String(PlatformOptions.DeploymentId ? PlatformOptions.DeploymentId : "(null)");
    platform_log += String("\n");

    platform_log += String("  ClientId: ");
    platform_log += String(PlatformOptions.ClientCredentials.ClientId ? PlatformOptions.ClientCredentials.ClientId : "(null)");
    platform_log += String("\n");

    platform_log += String("  ClientSecret: ");
    platform_log += String(PlatformOptions.ClientCredentials.ClientSecret ? "(masked)" : "(null)");
    platform_log += String("\n");

    platform_log += String("  EncryptionKey: ");
    platform_log += String(PlatformOptions.EncryptionKey ? "(masked)" : "(null)");
    platform_log += String("\n");
    WARN_PRINT(platform_log);

    platform_handle = EOS_Platform_Create(&PlatformOptions);
    if (!platform_handle) {
        // Try to get a more specific error from the last result if available
        ERR_PRINT("Failed to create EOS Platform (platform_handle == nullptr)");
        // EOS_Platform_Create returns nullptr on failure; there's no direct EOS_EResult, but common causes are invalid platform options.
        // Log a friendly troubleshooting hint.
        ERR_PRINT("Possible causes: invalid ProductId/SandboxId/DeploymentId, or missing/invalid client credentials.");
        EOS_Shutdown();
        return false;
    }

    initialized = true;
    IPlatform::set(this); // Automatically register as current instance
    WARN_PRINT("EOS Platform initialized successfully");
    return true;
}

void Platform::shutdown() {
    if (!initialized) {
        return;
    }

    if (platform_handle) {
        EOS_Platform_Release(platform_handle);
        platform_handle = nullptr;
    }

    EOS_Shutdown();
    initialized = false;
    IPlatform::set(nullptr);  // Automatically unregister
    WARN_PRINT("EOS Platform shutdown complete");
}

EOS_HPlatform Platform::get_platform_handle() const {
    return platform_handle;
}

bool Platform::is_initialized() const {
    return initialized;
}

void Platform::tick() {
    if (platform_handle) {
        EOS_Platform_Tick(platform_handle);
    }
}

// Define the static instance and accessors declared in IPlatform.h
// IPlatform static instance and accessors are defined in IPlatform.cpp