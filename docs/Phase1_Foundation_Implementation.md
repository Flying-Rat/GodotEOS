# GodotEOS Phase 1: Foundation Implementation
## Platform Management, Authentication & Base Architecture

This document provides detailed implementation guidance for Phase 1 of GodotEOS development, establishing the foundational architecture for Epic Online Services integration in Godot Engine.

---

## 📋 **Phase 1 Overview**

### **Objectives**
- ✅ **Platform Management**: EOS SDK initialization and lifecycle
- ✅ **Authentication System**: Epic Auth + Connect integration
- ✅ **Base Architecture**: Singleton pattern with Godot signals
- ✅ **Error Handling**: Robust failure recovery and logging
- ✅ **Cross-Platform Support**: Windows and Linux foundations

### **Duration**: Days 1-3 (3 days)
### **Deliverables**:
- Working EOS platform integration
- Complete authentication flow
- Signal-based callback system
- Foundation for Phase 2 services

---

## 🏗️ **1. Platform Management Implementation**

### **1.1 Core Platform Wrapper**

#### **File**: `addons/godot_epic/platform/epic_platform.cpp`
```cpp
#include "epic_platform.h"
#include <eos_sdk.h>
#include <eos_init.h>
#include <eos_logging.h>

EpicPlatform* EpicPlatform::instance = nullptr;
EOS_HPlatform EpicPlatform::platform_handle = nullptr;
bool EpicPlatform::is_initialized = false;

EpicPlatform* EpicPlatform::get_singleton() {
    if (!instance) {
        instance = new EpicPlatform();
    }
    return instance;
}

bool EpicPlatform::initialize(const EpicInitOptions& options) {
    if (is_initialized) {
        return true;
    }

    // Initialize EOS SDK
    EOS_InitializeOptions InitOptions = {};
    InitOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
    InitOptions.AllocateMemoryFunction = nullptr;  // Use default
    InitOptions.ReallocateMemoryFunction = nullptr;
    InitOptions.ReleaseMemoryFunction = nullptr;
    InitOptions.ProductName = options.product_name.utf8().get_data();
    InitOptions.ProductVersion = options.product_version.utf8().get_data();
    InitOptions.Reserved = nullptr;
    InitOptions.SystemInitializeOptions = nullptr;

    EOS_EResult InitResult = EOS_Initialize(&InitOptions);
    if (InitResult != EOS_EResult::EOS_Success) {
        godot::UtilityFunctions::push_error(
            vformat("Failed to initialize EOS SDK: %d", InitResult));
        return false;
    }

    // Create platform instance
    EOS_Platform_Options PlatformOptions = {};
    PlatformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
    PlatformOptions.bIsServer = false;
    PlatformOptions.ProductId = options.product_id.utf8().get_data();
    PlatformOptions.SandboxId = options.sandbox_id.utf8().get_data();
    PlatformOptions.DeploymentId = options.deployment_id.utf8().get_data();
    PlatformOptions.ClientCredentials.ClientId = options.client_id.utf8().get_data();
    PlatformOptions.ClientCredentials.ClientSecret = options.client_secret.utf8().get_data();
    PlatformOptions.EncryptionKey = options.encryption_key.utf8().get_data();
    PlatformOptions.OverrideCountryCode = nullptr;
    PlatformOptions.OverrideLocaleCode = nullptr;

    platform_handle = EOS_Platform_Create(&PlatformOptions);
    if (!platform_handle) {
        godot::UtilityFunctions::push_error("Failed to create EOS Platform");
        EOS_Shutdown();
        return false;
    }

    // Setup logging
    EOS_EResult LogResult = EOS_Logging_SetCallback(LoggingCallback);
    if (LogResult == EOS_EResult::EOS_Success) {
        EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES,
                               EOS_ELogLevel::EOS_LOG_Verbose);
    }

    is_initialized = true;
    return true;
}

void EpicPlatform::shutdown() {
    if (!is_initialized) {
        return;
    }

    if (platform_handle) {
        EOS_Platform_Release(platform_handle);
        platform_handle = nullptr;
    }

    EOS_Shutdown();
    is_initialized = false;
}

void EpicPlatform::tick() {
    if (platform_handle) {
        EOS_Platform_Tick(platform_handle);
    }
}

// Static logging callback
void EpicPlatform::LoggingCallback(const EOS_LogMessage* Message) {
    String log_text = String::utf8(Message->Message);

    switch (Message->Level) {
        case EOS_ELogLevel::EOS_LOG_Fatal:
        case EOS_ELogLevel::EOS_LOG_Error:
            godot::UtilityFunctions::push_error(vformat("[EOS] %s", log_text));
            break;
        case EOS_ELogLevel::EOS_LOG_Warning:
            godot::UtilityFunctions::push_warning(vformat("[EOS] %s", log_text));
            break;
        default:
            godot::UtilityFunctions::print(vformat("[EOS] %s", log_text));
            break;
    }
}
```

#### **File**: `addons/godot_epic/platform/epic_platform.h`
```cpp
#ifndef EPIC_PLATFORM_H
#define EPIC_PLATFORM_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/string.hpp>
#include <eos_types.h>

using namespace godot;

struct EpicInitOptions {
    String product_name = "GodotEOS";
    String product_version = "1.0.0";
    String product_id = "";
    String sandbox_id = "";
    String deployment_id = "";
    String client_id = "";
    String client_secret = "";
    String encryption_key = "";
};

class EpicPlatform {
private:
    static EpicPlatform* instance;
    static EOS_HPlatform platform_handle;
    static bool is_initialized;

    static void LoggingCallback(const EOS_LogMessage* Message);

public:
    static EpicPlatform* get_singleton();

    bool initialize(const EpicInitOptions& options);
    void shutdown();
    void tick();

    EOS_HPlatform get_platform_handle() const { return platform_handle; }
    bool is_platform_initialized() const { return is_initialized; }
};

#endif // EPIC_PLATFORM_H
```

### **1.2 Godot Integration Layer**

#### **File**: `addons/godot_epic/epic_os.gd`
```gdscript
class_name EpicOS
extends Node

## Epic Online Services main interface singleton
##
## This singleton provides access to all Epic Online Services functionality
## and manages the platform lifecycle, authentication, and service interfaces.

# Initialization signals
signal platform_initialized(success: bool)
signal platform_shutdown()

# Authentication signals
signal login_started()
signal login_completed(success: bool, user_info: Dictionary)
signal logout_completed()

# Error signals
signal error_occurred(error_code: int, error_message: String)

# Private variables
var _platform_initialized: bool = false
var _debug_mode: bool = false
var _init_options: Dictionary = {}

# Core initialization options
const DEFAULT_INIT_OPTIONS = {
    "product_name": "GodotEOS",
    "product_version": "1.0.0",
    "product_id": "",
    "sandbox_id": "",
    "deployment_id": "",
    "client_id": "",
    "client_secret": "",
    "encryption_key": ""
}

func _ready() -> void:
    set_process(true)
    print("EpicOS: Singleton ready")

func _process(_delta: float) -> void:
    # Tick the EOS platform every frame
    if _platform_initialized:
        EpicPlatform.tick()

## Initialize the Epic Online Services platform
## @param options: Dictionary with initialization parameters
## @return: bool - true if initialization succeeded
func initialize_platform(options: Dictionary = {}) -> bool:
    if _platform_initialized:
        push_warning("EpicOS: Platform already initialized")
        return true

    # Merge with default options
    _init_options = DEFAULT_INIT_OPTIONS.duplicate()
    for key in options:
        if _init_options.has(key):
            _init_options[key] = options[key]

    # Validate required options
    if not _validate_init_options():
        return false

    # Initialize platform
    var success: bool = EpicPlatform.initialize(_init_options)

    if success:
        _platform_initialized = true
        _log_info("Platform initialized successfully")
        platform_initialized.emit(true)
    else:
        _log_error("Platform initialization failed")
        platform_initialized.emit(false)

    return success

## Shutdown the Epic Online Services platform
func shutdown_platform() -> void:
    if not _platform_initialized:
        return

    EpicPlatform.shutdown()
    _platform_initialized = false
    _log_info("Platform shutdown complete")
    platform_shutdown.emit()

## Check if platform is initialized
func is_initialized() -> bool:
    return _platform_initialized

## Enable or disable debug logging
func set_debug_mode(enabled: bool) -> void:
    _debug_mode = enabled
    _log_info("Debug mode " + ("enabled" if enabled else "disabled"))

## Get current initialization options
func get_init_options() -> Dictionary:
    return _init_options.duplicate()

# Validation helpers
func _validate_init_options() -> bool:
    var required_fields = ["product_id", "sandbox_id", "deployment_id", "client_id", "client_secret"]

    for field in required_fields:
        if _init_options[field] == "":
            _log_error("Missing required initialization option: " + field)
            return false

    return true

# Logging helpers
func _log_info(message: String) -> void:
    print("[EpicOS] " + message)

func _log_warning(message: String) -> void:
    push_warning("[EpicOS] " + message)

func _log_error(message: String) -> void:
    push_error("[EpicOS] " + message)
    error_occurred.emit(-1, message)

func _log_debug(message: String) -> void:
    if _debug_mode:
        print("[EpicOS Debug] " + message)
```

---

## 🔐 **2. Authentication System Implementation**

### **2.1 Authentication Wrapper**

#### **File**: `addons/godot_epic/auth/epic_auth.cpp`
```cpp
#include "epic_auth.h"
#include "../platform/epic_platform.h"
#include <eos_auth.h>
#include <eos_connect.h>

EpicAuth* EpicAuth::instance = nullptr;

EpicAuth* EpicAuth::get_singleton() {
    if (!instance) {
        instance = new EpicAuth();
    }
    return instance;
}

bool EpicAuth::initialize() {
    EOS_HPlatform platform = EpicPlatform::get_singleton()->get_platform_handle();
    if (!platform) {
        return false;
    }

    auth_handle = EOS_Platform_GetAuthInterface(platform);
    connect_handle = EOS_Platform_GetConnectInterface(platform);

    return (auth_handle != nullptr && connect_handle != nullptr);
}

void EpicAuth::login_with_epic_account(const EpicAuthCredentials& credentials) {
    if (!auth_handle) {
        _emit_login_result(false, "Auth interface not initialized");
        return;
    }

    EOS_Auth_Credentials EOSCredentials = {};

    // Set credential type
    switch (credentials.type) {
        case EpicAuthCredentials::ACCOUNT_PORTAL:
            EOSCredentials.Type = EOS_ELoginCredentialType::EOS_LCT_AccountPortal;
            break;
        case EpicAuthCredentials::DEVELOPER:
            EOSCredentials.Type = EOS_ELoginCredentialType::EOS_LCT_Developer;
            EOSCredentials.Id = credentials.id.utf8().get_data();
            EOSCredentials.Token = credentials.token.utf8().get_data();
            break;
        case EpicAuthCredentials::EXCHANGE_CODE:
            EOSCredentials.Type = EOS_ELoginCredentialType::EOS_LCT_ExchangeCode;
            EOSCredentials.Token = credentials.token.utf8().get_data();
            break;
        default:
            EOSCredentials.Type = EOS_ELoginCredentialType::EOS_LCT_AccountPortal;
            break;
    }

    EOS_Auth_LoginOptions LoginOptions = {};
    LoginOptions.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
    LoginOptions.Credentials = &EOSCredentials;
    LoginOptions.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile |
                             EOS_EAuthScopeFlags::EOS_AS_FriendsList;

    EOS_Auth_Login(auth_handle, &LoginOptions, this, OnAuthLoginComplete);
}

void EpicAuth::OnAuthLoginComplete(const EOS_Auth_LoginCallbackInfo* Data) {
    EpicAuth* auth = static_cast<EpicAuth*>(Data->ClientData);

    if (Data->ResultCode == EOS_EResult::EOS_Success) {
        auth->epic_account_id = Data->LocalUserId;
        auth->_login_with_connect();
    } else {
        auth->_emit_login_result(false,
            vformat("Epic Auth login failed: %d", Data->ResultCode));
    }
}

void EpicAuth::_login_with_connect() {
    if (!connect_handle || !epic_account_id) {
        _emit_login_result(false, "Connect interface or Epic Account ID not available");
        return;
    }

    // Get auth token for Connect login
    EOS_Auth_CopyUserAuthTokenOptions TokenOptions = {};
    TokenOptions.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

    EOS_Auth_Token* AuthToken = nullptr;
    EOS_EResult TokenResult = EOS_Auth_CopyUserAuthToken(auth_handle, &TokenOptions,
                                                        epic_account_id, &AuthToken);

    if (TokenResult != EOS_EResult::EOS_Success || !AuthToken) {
        _emit_login_result(false, "Failed to get auth token for Connect login");
        return;
    }

    // Login with Connect using Epic token
    EOS_Connect_Credentials ConnectCredentials = {};
    ConnectCredentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
    ConnectCredentials.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC;
    ConnectCredentials.Token = AuthToken->AccessToken;

    EOS_Connect_LoginOptions ConnectLoginOptions = {};
    ConnectLoginOptions.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
    ConnectLoginOptions.Credentials = &ConnectCredentials;

    EOS_Connect_Login(connect_handle, &ConnectLoginOptions, this, OnConnectLoginComplete);

    // Clean up auth token
    EOS_Auth_Token_Release(AuthToken);
}

void EpicAuth::OnConnectLoginComplete(const EOS_Connect_LoginCallbackInfo* Data) {
    EpicAuth* auth = static_cast<EpicAuth*>(Data->ClientData);

    if (Data->ResultCode == EOS_EResult::EOS_Success) {
        auth->product_user_id = Data->LocalUserId;
        auth->is_logged_in = true;

        // Build user info
        Dictionary user_info;
        user_info["epic_account_id"] = auth->_account_id_to_string(auth->epic_account_id);
        user_info["product_user_id"] = auth->_product_user_id_to_string(auth->product_user_id);
        user_info["login_status"] = "connected";

        auth->_emit_login_result(true, "Login successful", user_info);
    } else {
        auth->_emit_login_result(false,
            vformat("Connect login failed: %d", Data->ResultCode));
    }
}

void EpicAuth::logout() {
    if (!is_logged_in) {
        return;
    }

    // Logout from Connect
    if (connect_handle && product_user_id) {
        EOS_Connect_LogoutOptions LogoutOptions = {};
        LogoutOptions.ApiVersion = EOS_CONNECT_LOGOUT_API_LATEST;
        LogoutOptions.LocalUserId = product_user_id;

        EOS_Connect_Logout(connect_handle, &LogoutOptions, this, OnConnectLogoutComplete);
    }

    // Logout from Auth
    if (auth_handle && epic_account_id) {
        EOS_Auth_LogoutOptions AuthLogoutOptions = {};
        AuthLogoutOptions.ApiVersion = EOS_AUTH_LOGOUT_API_LATEST;
        AuthLogoutOptions.LocalUserId = epic_account_id;

        EOS_Auth_Logout(auth_handle, &AuthLogoutOptions, nullptr, nullptr);
    }

    // Reset state
    epic_account_id = nullptr;
    product_user_id = nullptr;
    is_logged_in = false;
}

void EpicAuth::OnConnectLogoutComplete(const EOS_Connect_LogoutCallbackInfo* Data) {
    // Logout completed - could emit signal here if needed
}

// Helper functions
String EpicAuth::_account_id_to_string(EOS_EpicAccountId account_id) {
    char AccountIdBuffer[EOS_EPICACCOUNTID_MAX_LENGTH + 1];
    int32_t AccountIdBufferSize = sizeof(AccountIdBuffer);

    EOS_EpicAccountId_ToString(account_id, AccountIdBuffer, &AccountIdBufferSize);
    return String::utf8(AccountIdBuffer);
}

String EpicAuth::_product_user_id_to_string(EOS_ProductUserId product_user_id) {
    char ProductUserIdBuffer[EOS_PRODUCTUSERID_MAX_LENGTH + 1];
    int32_t ProductUserIdBufferSize = sizeof(ProductUserIdBuffer);

    EOS_ProductUserId_ToString(product_user_id, ProductUserIdBuffer, &ProductUserIdBufferSize);
    return String::utf8(ProductUserIdBuffer);
}

void EpicAuth::_emit_login_result(bool success, const String& message, const Dictionary& user_info) {
    // This would call back into GDScript through the singleton
    // Implementation depends on the callback mechanism chosen
    godot::UtilityFunctions::print(vformat("Auth Result: %s - %s", success, message));
}
```

### **2.2 GDScript Authentication Interface**

#### **File Extension to**: `addons/godot_epic/epic_os.gd`
```gdscript
# Authentication-related additions to EpicOS singleton

# Authentication state
var _is_logged_in: bool = false
var _current_user_info: Dictionary = {}

# Authentication methods
enum AuthType {
    ACCOUNT_PORTAL,    # Epic Games Store login
    DEVELOPER,         # Developer credentials (testing)
    EXCHANGE_CODE      # OAuth exchange code
}

## Login with Epic Games account
## @param auth_type: AuthType - Type of authentication to use
## @param credentials: Dictionary - Authentication credentials (optional for ACCOUNT_PORTAL)
func login(auth_type: AuthType = AuthType.ACCOUNT_PORTAL, credentials: Dictionary = {}) -> void:
    if not _platform_initialized:
        _log_error("Cannot login: Platform not initialized")
        error_occurred.emit(-1, "Platform not initialized")
        return

    if _is_logged_in:
        _log_warning("User already logged in")
        return

    _log_info("Starting login process...")
    login_started.emit()

    # Call native authentication
    var auth_credentials = _build_auth_credentials(auth_type, credentials)
    var success = EpicAuth.login_with_epic_account(auth_credentials)

    if not success:
        _log_error("Failed to start login process")
        login_completed.emit(false, {})

## Logout from Epic Games account
func logout() -> void:
    if not _is_logged_in:
        _log_warning("User not logged in")
        return

    _log_info("Logging out...")
    EpicAuth.logout()

    _is_logged_in = false
    _current_user_info.clear()
    logout_completed.emit()

## Check if user is currently logged in
func is_logged_in() -> bool:
    return _is_logged_in

## Get current user information
func get_current_user() -> Dictionary:
    return _current_user_info.duplicate()

# Authentication callback handlers
func _on_login_completed(success: bool, user_info: Dictionary) -> void:
    if success:
        _is_logged_in = true
        _current_user_info = user_info
        _log_info("Login successful for user: " + user_info.get("display_name", "Unknown"))
    else:
        _is_logged_in = false
        _current_user_info.clear()
        _log_error("Login failed")

    login_completed.emit(success, user_info)

# Helper methods
func _build_auth_credentials(auth_type: AuthType, credentials: Dictionary) -> Dictionary:
    var auth_creds = {
        "type": auth_type,
        "id": credentials.get("id", ""),
        "token": credentials.get("token", "")
    }

    return auth_creds

## Convenience method for developer login (testing)
func login_developer(user_id: String, password: String) -> void:
    login(AuthType.DEVELOPER, {"id": user_id, "token": password})

## Convenience method for exchange code login
func login_exchange_code(exchange_code: String) -> void:
    login(AuthType.EXCHANGE_CODE, {"token": exchange_code})
```

---

## 🎯 **3. Base Architecture Implementation**

### **3.1 Signal System Architecture**

#### **File**: `addons/godot_epic/core/epic_signals.gd`
```gdscript
class_name EpicSignals
extends RefCounted

## Central signal hub for Epic Online Services
##
## This class defines all signals used throughout the EpicOS system
## and provides a centralized way to manage cross-component communication.

# Platform signals
signal platform_initialized(success: bool)
signal platform_shutdown()
signal platform_tick_error(error_code: int)

# Authentication signals
signal auth_login_started()
signal auth_login_completed(success: bool, user_info: Dictionary)
signal auth_logout_completed()
signal auth_status_changed(status: String)

# User management signals
signal user_info_updated(user_info: Dictionary)
signal user_presence_changed(user_id: String, presence: Dictionary)

# Achievement signals (Phase 2 preparation)
signal achievement_definitions_received(definitions: Array)
signal achievement_unlocked(achievement_id: String)
signal achievement_progress_updated(achievement_id: String, progress: Dictionary)

# Stats signals (Phase 2 preparation)
signal stats_received(stats: Dictionary)
signal stats_updated(stat_name: String, value: Variant)
signal stats_ingest_completed(success: bool)

# Cloud save signals (Phase 2 preparation)
signal file_list_received(files: Array)
signal file_download_started(filename: String)
signal file_download_completed(success: bool, filename: String, data: PackedByteArray)
signal file_upload_started(filename: String)
signal file_upload_completed(success: bool, filename: String)

# Error and logging signals
signal error_occurred(source: String, error_code: int, error_message: String)
signal warning_occurred(source: String, warning_message: String)

# Debug signals
signal debug_message(source: String, message: String)
signal verbose_log(category: String, message: String)
```

### **3.2 Error Handling System**

#### **File**: `addons/godot_epic/core/epic_error.gd`
```gdscript
class_name EpicError
extends RefCounted

## Error handling and result codes for Epic Online Services
##
## Provides standardized error handling and Epic result code mapping

# Epic Result Code mappings (from EOS_EResult)
enum Result {
    SUCCESS = 0,
    NO_CONNECTION = 1,
    INVALID_USER = 2,
    INVALID_AUTH = 3,
    ACCESS_DENIED = 4,
    MISSING_PERMISSION = 5,
    TOKEN_NOT_ACCOUNT = 6,
    TOS_NOT_ACCEPTED = 7,
    INVALID_PARAMETERS = 8,
    INVALID_REQUEST = 9,
    UNKNOWN_ERROR = 10,
    # Add more as needed
}

# Error severity levels
enum Severity {
    INFO,
    WARNING,
    ERROR,
    FATAL
}

# Error categories
enum Category {
    PLATFORM,
    AUTHENTICATION,
    ACHIEVEMENTS,
    STATS,
    CLOUD_SAVE,
    NETWORKING,
    UNKNOWN
}

## Convert EOS result code to human-readable message
static func result_to_message(result_code: int) -> String:
    match result_code:
        Result.SUCCESS:
            return "Operation completed successfully"
        Result.NO_CONNECTION:
            return "No connection to Epic Online Services"
        Result.INVALID_USER:
            return "Invalid user or user not logged in"
        Result.INVALID_AUTH:
            return "Authentication failed"
        Result.ACCESS_DENIED:
            return "Access denied"
        Result.MISSING_PERMISSION:
            return "Missing required permissions"
        Result.TOKEN_NOT_ACCOUNT:
            return "Token does not represent an account"
        Result.TOS_NOT_ACCEPTED:
            return "Terms of Service not accepted"
        Result.INVALID_PARAMETERS:
            return "Invalid parameters provided"
        Result.INVALID_REQUEST:
            return "Invalid request"
        _:
            return "Unknown error (code: %d)" % result_code

## Check if result code indicates success
static func is_success(result_code: int) -> bool:
    return result_code == Result.SUCCESS

## Check if result code indicates a recoverable error
static func is_recoverable(result_code: int) -> bool:
    match result_code:
        Result.NO_CONNECTION, Result.INVALID_REQUEST:
            return true
        _:
            return false

## Create standardized error dictionary
static func create_error(category: Category, result_code: int, details: String = "") -> Dictionary:
    return {
        "category": Category.keys()[category],
        "result_code": result_code,
        "message": result_to_message(result_code),
        "details": details,
        "severity": _get_severity_for_result(result_code),
        "timestamp": Time.get_unix_time_from_system(),
        "recoverable": is_recoverable(result_code)
    }

static func _get_severity_for_result(result_code: int) -> Severity:
    match result_code:
        Result.SUCCESS:
            return Severity.INFO
        Result.NO_CONNECTION, Result.INVALID_REQUEST:
            return Severity.WARNING
        Result.INVALID_AUTH, Result.ACCESS_DENIED:
            return Severity.ERROR
        _:
            return Severity.ERROR
```

### **3.3 Configuration Management**

#### **File**: `addons/godot_epic/core/epic_config.gd`
```gdscript
class_name EpicConfig
extends Resource

## Configuration resource for Epic Online Services
##
## Handles loading and validation of EOS configuration from various sources

@export var product_name: String = "GodotEOS Game"
@export var product_version: String = "1.0.0"
@export_group("Epic Developer Portal Settings")
@export var product_id: String = ""
@export var sandbox_id: String = ""
@export var deployment_id: String = ""
@export_group("Client Credentials")
@export var client_id: String = ""
@export var client_secret: String = ""
@export var encryption_key: String = ""
@export_group("Development Options")
@export var use_dev_auth: bool = false
@export var dev_auth_host: String = "localhost:6547"
@export var dev_auth_credential_name: String = "Player1"
@export_group("Logging")
@export var enable_debug_logging: bool = false
@export var log_level: String = "Warning"  # Off, Fatal, Error, Warning, Info, Verbose, VeryVerbose

## Load configuration from project settings
static func load_from_project_settings() -> EpicConfig:
    var config = EpicConfig.new()

    config.product_name = ProjectSettings.get_setting("epic_os/product_name", config.product_name)
    config.product_version = ProjectSettings.get_setting("epic_os/product_version", config.product_version)
    config.product_id = ProjectSettings.get_setting("epic_os/product_id", config.product_id)
    config.sandbox_id = ProjectSettings.get_setting("epic_os/sandbox_id", config.sandbox_id)
    config.deployment_id = ProjectSettings.get_setting("epic_os/deployment_id", config.deployment_id)
    config.client_id = ProjectSettings.get_setting("epic_os/client_id", config.client_id)
    config.client_secret = ProjectSettings.get_setting("epic_os/client_secret", config.client_secret)
    config.encryption_key = ProjectSettings.get_setting("epic_os/encryption_key", config.encryption_key)
    config.use_dev_auth = ProjectSettings.get_setting("epic_os/use_dev_auth", config.use_dev_auth)
    config.enable_debug_logging = ProjectSettings.get_setting("epic_os/enable_debug_logging", config.enable_debug_logging)

    return config

## Load configuration from environment variables
static func load_from_environment() -> EpicConfig:
    var config = EpicConfig.new()

    if OS.has_environment("EOS_PRODUCT_ID"):
        config.product_id = OS.get_environment("EOS_PRODUCT_ID")
    if OS.has_environment("EOS_SANDBOX_ID"):
        config.sandbox_id = OS.get_environment("EOS_SANDBOX_ID")
    if OS.has_environment("EOS_DEPLOYMENT_ID"):
        config.deployment_id = OS.get_environment("EOS_DEPLOYMENT_ID")
    if OS.has_environment("EOS_CLIENT_ID"):
        config.client_id = OS.get_environment("EOS_CLIENT_ID")
    if OS.has_environment("EOS_CLIENT_SECRET"):
        config.client_secret = OS.get_environment("EOS_CLIENT_SECRET")
    if OS.has_environment("EOS_ENCRYPTION_KEY"):
        config.encryption_key = OS.get_environment("EOS_ENCRYPTION_KEY")

    return config

## Validate configuration completeness
func validate() -> Dictionary:
    var errors: Array[String] = []
    var warnings: Array[String] = []

    # Check required fields
    if product_id.is_empty():
        errors.append("Product ID is required")
    if sandbox_id.is_empty():
        errors.append("Sandbox ID is required")
    if deployment_id.is_empty():
        errors.append("Deployment ID is required")
    if client_id.is_empty():
        errors.append("Client ID is required")
    if client_secret.is_empty():
        errors.append("Client Secret is required")

    # Check optional but recommended fields
    if encryption_key.is_empty():
        warnings.append("Encryption key not set - data will not be encrypted")

    # Development warnings
    if use_dev_auth:
        warnings.append("Development authentication enabled - not for production")

    return {
        "valid": errors.is_empty(),
        "errors": errors,
        "warnings": warnings
    }

## Convert to options dictionary for platform initialization
func to_init_options() -> Dictionary:
    return {
        "product_name": product_name,
        "product_version": product_version,
        "product_id": product_id,
        "sandbox_id": sandbox_id,
        "deployment_id": deployment_id,
        "client_id": client_id,
        "client_secret": client_secret,
        "encryption_key": encryption_key
    }
```

---

## 🔌 **4. Plugin Architecture**

### **4.1 Plugin Configuration**

#### **File**: `addons/godot_epic/plugin.cfg`
```ini
[plugin]

name="GodotEOS"
description="Epic Online Services integration for Godot Engine"
author="Flying-Rat"
version="0.1.0"
script="plugin.gd"
```

#### **File**: `addons/godot_epic/plugin.gd`
```gdscript
@tool
extends EditorPlugin

const EpicOS = preload("res://addons/godot_epic/epic_os.gd")

func _enter_tree():
    print("GodotEOS: Plugin activated")

    # Add autoload singleton
    add_autoload_singleton("EpicOS", "res://addons/godot_epic/epic_os.gd")

    # Add project settings
    _add_project_settings()

    print("GodotEOS: Initialization complete")

func _exit_tree():
    print("GodotEOS: Plugin deactivated")

    # Remove autoload singleton
    remove_autoload_singleton("EpicOS")

    # Cleanup (settings remain for user convenience)
    print("GodotEOS: Cleanup complete")

func _add_project_settings():
    # Define Epic OS project settings
    var settings = {
        "epic_os/product_name": {
            "value": "My Godot Game",
            "type": TYPE_STRING,
            "hint": PROPERTY_HINT_NONE
        },
        "epic_os/product_version": {
            "value": "1.0.0",
            "type": TYPE_STRING,
            "hint": PROPERTY_HINT_NONE
        },
        "epic_os/product_id": {
            "value": "",
            "type": TYPE_STRING,
            "hint": PROPERTY_HINT_NONE
        },
        "epic_os/sandbox_id": {
            "value": "",
            "type": TYPE_STRING,
            "hint": PROPERTY_HINT_NONE
        },
        "epic_os/deployment_id": {
            "value": "",
            "type": TYPE_STRING,
            "hint": PROPERTY_HINT_NONE
        },
        "epic_os/client_id": {
            "value": "",
            "type": TYPE_STRING,
            "hint": PROPERTY_HINT_NONE
        },
        "epic_os/client_secret": {
            "value": "",
            "type": TYPE_STRING,
            "hint": PROPERTY_HINT_PASSWORD
        },
        "epic_os/encryption_key": {
            "value": "",
            "type": TYPE_STRING,
            "hint": PROPERTY_HINT_PASSWORD
        },
        "epic_os/use_dev_auth": {
            "value": false,
            "type": TYPE_BOOL,
            "hint": PROPERTY_HINT_NONE
        },
        "epic_os/enable_debug_logging": {
            "value": false,
            "type": TYPE_BOOL,
            "hint": PROPERTY_HINT_NONE
        }
    }

    # Add settings to project
    for setting_name in settings:
        var setting_info = settings[setting_name]
        if not ProjectSettings.has_setting(setting_name):
            ProjectSettings.set_setting(setting_name, setting_info.value)
            var property_info = {
                "name": setting_name,
                "type": setting_info.type,
                "hint": setting_info.hint
            }
            ProjectSettings.add_property_info(property_info)

    # Save project settings
    ProjectSettings.save()
```

### **4.2 GDExtension Configuration**

#### **File**: `addons/godot_epic/godot_epic.gdextension`
```ini
[configuration]

entry_symbol = "godot_epic_library_init"
compatibility_minimum = "4.3"

[libraries]

windows.debug.x86_64 = "res://addons/godot_epic/bin/windows/libgodot_epic.windows.template_debug.x86_64.dll"
windows.release.x86_64 = "res://addons/godot_epic/bin/windows/libgodot_epic.windows.template_release.x86_64.dll"
linux.debug.x86_64 = "res://addons/godot_epic/bin/linux/libgodot_epic.linux.template_debug.x86_64.so"
linux.release.x86_64 = "res://addons/godot_epic/bin/linux/libgodot_epic.linux.template_release.x86_64.so"

[dependencies]

windows.debug.x86_64 = {
    "res://addons/godot_epic/bin/windows/EOSSDK-Win64-Shipping.dll" : ""
}
windows.release.x86_64 = {
    "res://addons/godot_epic/bin/windows/EOSSDK-Win64-Shipping.dll" : ""
}
linux.debug.x86_64 = {
    "res://addons/godot_epic/bin/linux/libEOSSDK-Linux-Shipping.so" : ""
}
linux.release.x86_64 = {
    "res://addons/godot_epic/bin/linux/libEOSSDK-Linux-Shipping.so" : ""
}
```

---

## 🧪 **5. Testing and Validation**

### **5.1 Unit Tests**

#### **File**: `tests/test_epic_platform.gd`
```gdscript
extends TestSuite

func test_platform_initialization():
    var config = EpicConfig.new()
    config.product_id = "test_product"
    config.sandbox_id = "test_sandbox"
    config.deployment_id = "test_deployment"
    config.client_id = "test_client"
    config.client_secret = "test_secret"

    var validation = config.validate()
    assert_true(validation.valid, "Config should be valid")

    var options = config.to_init_options()
    assert_eq(options.product_id, "test_product", "Product ID should match")

func test_authentication_flow():
    # Test authentication state management
    assert_false(EpicOS.is_logged_in(), "Should not be logged in initially")

    # Mock login
    EpicOS._on_login_completed(true, {"user_id": "test_user"})
    assert_true(EpicOS.is_logged_in(), "Should be logged in after successful auth")

    # Mock logout
    EpicOS.logout()
    assert_false(EpicOS.is_logged_in(), "Should not be logged in after logout")
```

### **5.2 Integration Test Scene**

#### **File**: `tests/integration_test.gd`
```gdscript
extends Control

@onready var status_label: Label = $VBoxContainer/StatusLabel
@onready var init_button: Button = $VBoxContainer/InitButton
@onready var login_button: Button = $VBoxContainer/LoginButton
@onready var logout_button: Button = $VBoxContainer/LogoutButton
@onready var log_text: TextEdit = $VBoxContainer/LogText

func _ready():
    # Connect UI signals
    init_button.pressed.connect(_on_init_pressed)
    login_button.pressed.connect(_on_login_pressed)
    logout_button.pressed.connect(_on_logout_pressed)

    # Connect EpicOS signals
    EpicOS.platform_initialized.connect(_on_platform_initialized)
    EpicOS.login_completed.connect(_on_login_completed)
    EpicOS.logout_completed.connect(_on_logout_completed)
    EpicOS.error_occurred.connect(_on_error_occurred)

    _update_ui()
    _log("Integration test ready")

func _on_init_pressed():
    _log("Initializing platform...")
    var config = EpicConfig.load_from_project_settings()
    var success = EpicOS.initialize_platform(config.to_init_options())
    if not success:
        _log("Failed to start platform initialization")

func _on_login_pressed():
    _log("Starting login...")
    EpicOS.login(EpicOS.AuthType.DEVELOPER, {
        "id": "TestUser",
        "token": "TestPassword"
    })

func _on_logout_pressed():
    _log("Logging out...")
    EpicOS.logout()

func _on_platform_initialized(success: bool):
    _log("Platform initialized: " + str(success))
    _update_ui()

func _on_login_completed(success: bool, user_info: Dictionary):
    _log("Login completed: " + str(success))
    if success:
        _log("User info: " + str(user_info))
    _update_ui()

func _on_logout_completed():
    _log("Logout completed")
    _update_ui()

func _on_error_occurred(error_code: int, error_message: String):
    _log("ERROR: " + error_message)

func _update_ui():
    var platform_ready = EpicOS.is_initialized()
    var logged_in = EpicOS.is_logged_in()

    status_label.text = "Platform: %s | Auth: %s" % [
        "Ready" if platform_ready else "Not Ready",
        "Logged In" if logged_in else "Logged Out"
    ]

    init_button.disabled = platform_ready
    login_button.disabled = not platform_ready or logged_in
    logout_button.disabled = not logged_in

func _log(message: String):
    var timestamp = "[%s] " % Time.get_datetime_string_from_system()
    log_text.text += timestamp + message + "\n"
    log_text.scroll_vertical = log_text.get_v_scroll_bar().max_value
```

---

## 📋 **Phase 1 Implementation Checklist**

### **Day 1: Platform Foundation**
- [ ] Create EpicPlatform C++ wrapper class
- [ ] Implement EOS SDK initialization/shutdown
- [ ] Set up logging callback system
- [ ] Create EpicOS GDScript singleton
- [ ] Implement platform tick mechanism
- [ ] Add configuration management system
- [ ] Create plugin structure and autoload

### **Day 2: Authentication System**
- [ ] Create EpicAuth C++ wrapper class
- [ ] Implement Epic Games authentication flow
- [ ] Implement Connect (Product User) authentication
- [ ] Add authentication state management
- [ ] Create GDScript authentication interface
- [ ] Implement login/logout methods
- [ ] Add user information retrieval

### **Day 3: Integration & Testing**
- [ ] Complete signal system architecture
- [ ] Implement error handling and logging
- [ ] Create configuration validation
- [ ] Add project settings integration
- [ ] Build integration test scene
- [ ] Test authentication flow end-to-end
- [ ] Cross-platform build verification
- [ ] Documentation and cleanup

---

## 🎯 **Success Criteria**

### **Functional Requirements**
- ✅ **Platform initializes successfully** with valid Epic credentials
- ✅ **Authentication works** with Epic Games accounts
- ✅ **Signals fire correctly** for all async operations
- ✅ **Error handling** provides clear feedback
- ✅ **Configuration loading** from multiple sources

### **Technical Requirements**
- ✅ **Cross-platform builds** (Windows + Linux)
- ✅ **Memory management** with proper cleanup
- ✅ **Thread safety** for callback processing
- ✅ **Performance** - minimal impact on game loop
- ✅ **Extensibility** - ready for Phase 2 services

### **Quality Requirements**
- ✅ **Code documentation** with clear examples
- ✅ **Unit tests** for core functionality
- ✅ **Integration tests** with real EOS backend
- ✅ **Error scenarios** handled gracefully
- ✅ **Developer experience** - easy to configure and use

---

**Phase 1 Foundation Complete**
*Ready for Phase 2: Game Services Implementation*