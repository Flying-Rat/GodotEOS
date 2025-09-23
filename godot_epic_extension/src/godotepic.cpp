#include "godotepic.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

// Static member definitions
GodotEpic* GodotEpic::instance = nullptr;
EOS_HPlatform GodotEpic::platform_handle = nullptr;
bool GodotEpic::is_initialized = false;

void GodotEpic::_bind_methods() {
	ClassDB::bind_method(D_METHOD("initialize_platform", "options"), &GodotEpic::initialize_platform);
	ClassDB::bind_method(D_METHOD("shutdown_platform"), &GodotEpic::shutdown_platform);
	ClassDB::bind_method(D_METHOD("tick"), &GodotEpic::tick);
	ClassDB::bind_method(D_METHOD("is_platform_initialized"), &GodotEpic::is_platform_initialized);
}

GodotEpic::GodotEpic() {
	// Initialize any variables here.
	time_passed = 0.0;
	instance = this;
}

GodotEpic::~GodotEpic() {
	// Ensure platform is shutdown on destruction
	shutdown_platform();
	if (instance == this) {
		instance = nullptr;
	}
}

GodotEpic* GodotEpic::get_singleton() {
	if (!instance) {
		instance = memnew(GodotEpic);
	}
	return instance;
}

bool GodotEpic::initialize_platform(const Dictionary& options) {
	if (is_initialized) {
		ERR_PRINT("EOS Platform already initialized");
		return true;
	}
	
	// Convert dictionary to init options
	EpicInitOptions init_options = _dict_to_init_options(options);
	
	// Validate options
	if (!_validate_init_options(init_options)) {
		ERR_PRINT("EOS Platform initialization failed: Invalid options");
		return false;
	}
	
	// Initialize EOS SDK
	EOS_InitializeOptions InitOptions = {};
	InitOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
	InitOptions.AllocateMemoryFunction = nullptr;  // Use default
	InitOptions.ReallocateMemoryFunction = nullptr;
	InitOptions.ReleaseMemoryFunction = nullptr;
	InitOptions.ProductName = init_options.product_name.utf8().get_data();
	InitOptions.ProductVersion = init_options.product_version.utf8().get_data();
	InitOptions.Reserved = nullptr;
	InitOptions.SystemInitializeOptions = nullptr;
	
	EOS_EResult InitResult = EOS_Initialize(&InitOptions);
	if (InitResult != EOS_EResult::EOS_Success) {
		String error_msg = "Failed to initialize EOS SDK: " + String::num_int64(static_cast<int64_t>(InitResult));
		ERR_PRINT(error_msg);
		return false;
	}
	
	// Create platform instance
	EOS_Platform_Options PlatformOptions = {};
	PlatformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
	PlatformOptions.bIsServer = false;
	PlatformOptions.ProductId = init_options.product_id.utf8().get_data();
	PlatformOptions.SandboxId = init_options.sandbox_id.utf8().get_data();
	PlatformOptions.DeploymentId = init_options.deployment_id.utf8().get_data();
	PlatformOptions.ClientCredentials.ClientId = init_options.client_id.utf8().get_data();
	PlatformOptions.ClientCredentials.ClientSecret = init_options.client_secret.utf8().get_data();
	PlatformOptions.EncryptionKey = init_options.encryption_key.utf8().get_data();
	PlatformOptions.OverrideCountryCode = nullptr;
	PlatformOptions.OverrideLocaleCode = nullptr;
	
	platform_handle = EOS_Platform_Create(&PlatformOptions);
	if (!platform_handle) {
		ERR_PRINT("Failed to create EOS Platform");
		EOS_Shutdown();
		return false;
	}
	
	// Setup logging
	EOS_EResult LogResult = EOS_Logging_SetCallback(logging_callback);
	if (LogResult == EOS_EResult::EOS_Success) {
		EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, 
							   EOS_ELogLevel::EOS_LOG_Verbose);
	}
	
	is_initialized = true;
	ERR_PRINT("EOS Platform initialized successfully");
	return true;
}

void GodotEpic::shutdown_platform() {
	if (!is_initialized) {
		return;
	}
	
	if (platform_handle) {
		EOS_Platform_Release(platform_handle);
		platform_handle = nullptr;
	}
	
	EOS_Shutdown();
	is_initialized = false;
	ERR_PRINT("EOS Platform shutdown complete");
}

void GodotEpic::tick() {
	if (platform_handle) {
		EOS_Platform_Tick(platform_handle);
	}
}

bool GodotEpic::is_platform_initialized() const {
	return is_initialized;
}

EOS_HPlatform GodotEpic::get_platform_handle() const {
	return platform_handle;
}

// Static logging callback
void EOS_CALL GodotEpic::logging_callback(const EOS_LogMessage* message) {
	if (!message || !message->Message) {
		return;
	}
	
	String log_text = String::utf8(message->Message);
	String category = message->Category ? String::utf8(message->Category) : "EOS";
	
	switch (message->Level) {
		case EOS_ELogLevel::EOS_LOG_Fatal:
		case EOS_ELogLevel::EOS_LOG_Error:
			{
				String log_msg = String("[") + category + "] " + log_text;
				ERR_PRINT(log_msg);
			}
			break;
		case EOS_ELogLevel::EOS_LOG_Warning:
			{
				String log_msg = String("[") + category + "] " + log_text;
				WARN_PRINT(log_msg);
			}
			break;
		case EOS_ELogLevel::EOS_LOG_Info:
		case EOS_ELogLevel::EOS_LOG_Verbose:
		case EOS_ELogLevel::EOS_LOG_VeryVerbose:
		default:
			{
				String log_msg = String("[") + category + "] " + log_text;
				ERR_PRINT(log_msg);
			}
			break;
	}
}

// Helper methods
EpicInitOptions GodotEpic::_dict_to_init_options(const Dictionary& options_dict) {
	EpicInitOptions options;
	
	if (options_dict.has("product_name")) {
		options.product_name = options_dict["product_name"];
	}
	if (options_dict.has("product_version")) {
		options.product_version = options_dict["product_version"];
	}
	if (options_dict.has("product_id")) {
		options.product_id = options_dict["product_id"];
	}
	if (options_dict.has("sandbox_id")) {
		options.sandbox_id = options_dict["sandbox_id"];
	}
	if (options_dict.has("deployment_id")) {
		options.deployment_id = options_dict["deployment_id"];
	}
	if (options_dict.has("client_id")) {
		options.client_id = options_dict["client_id"];
	}
	if (options_dict.has("client_secret")) {
		options.client_secret = options_dict["client_secret"];
	}
	if (options_dict.has("encryption_key")) {
		options.encryption_key = options_dict["encryption_key"];
	}
	
	return options;
}

bool GodotEpic::_validate_init_options(const EpicInitOptions& options) {
	Array required_fields;
	required_fields.append("product_id");
	required_fields.append("sandbox_id");
	required_fields.append("deployment_id");
	required_fields.append("client_id");
	required_fields.append("client_secret");
	
	bool valid = true;
	
	if (options.product_id.is_empty()) {
		ERR_PRINT("Missing required initialization option: product_id");
		valid = false;
	}
	if (options.sandbox_id.is_empty()) {
		ERR_PRINT("Missing required initialization option: sandbox_id");
		valid = false;
	}
	if (options.deployment_id.is_empty()) {
		ERR_PRINT("Missing required initialization option: deployment_id");
		valid = false;
	}
	if (options.client_id.is_empty()) {
		ERR_PRINT("Missing required initialization option: client_id");
		valid = false;
	}
	if (options.client_secret.is_empty()) {
		ERR_PRINT("Missing required initialization option: client_secret");
		valid = false;
	}
	
	if (options.encryption_key.is_empty()) {
		WARN_PRINT("Encryption key not set - data will not be encrypted");
	}
	
	return valid;
}