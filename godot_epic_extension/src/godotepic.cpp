#include "godotepic.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

using namespace godot;

// Static member definitions
GodotEpic* GodotEpic::instance = nullptr;
EOS_HPlatform GodotEpic::platform_handle = nullptr;
bool GodotEpic::is_initialized = false;

void GodotEpic::_bind_methods() {
	ClassDB::bind_static_method("GodotEpic", D_METHOD("get_singleton"), &GodotEpic::get_singleton);
	ClassDB::bind_method(D_METHOD("initialize_platform", "options"), &GodotEpic::initialize_platform);
	ClassDB::bind_method(D_METHOD("shutdown_platform"), &GodotEpic::shutdown_platform);
	ClassDB::bind_method(D_METHOD("tick"), &GodotEpic::tick);
	ClassDB::bind_method(D_METHOD("is_platform_initialized"), &GodotEpic::is_platform_initialized);

	// Authentication methods
	ClassDB::bind_method(D_METHOD("login_with_epic_account", "email", "password"), &GodotEpic::login_with_epic_account);
	ClassDB::bind_method(D_METHOD("login_with_device_id", "display_name"), &GodotEpic::login_with_device_id);
	ClassDB::bind_method(D_METHOD("logout"), &GodotEpic::logout);
	ClassDB::bind_method(D_METHOD("is_user_logged_in"), &GodotEpic::is_user_logged_in);
	ClassDB::bind_method(D_METHOD("get_current_username"), &GodotEpic::get_current_username);
	ClassDB::bind_method(D_METHOD("get_epic_account_id"), &GodotEpic::get_epic_account_id);
	ClassDB::bind_method(D_METHOD("get_product_user_id"), &GodotEpic::get_product_user_id);

	// Friends methods
	ClassDB::bind_method(D_METHOD("query_friends"), &GodotEpic::query_friends);
	ClassDB::bind_method(D_METHOD("get_friends_list"), &GodotEpic::get_friends_list);
	ClassDB::bind_method(D_METHOD("get_friend_info", "friend_id"), &GodotEpic::get_friend_info);

	// Signals
	ADD_SIGNAL(MethodInfo("login_completed", PropertyInfo(Variant::BOOL, "success"), PropertyInfo(Variant::STRING, "username")));
	ADD_SIGNAL(MethodInfo("logout_completed", PropertyInfo(Variant::BOOL, "success")));
	ADD_SIGNAL(MethodInfo("friends_updated", PropertyInfo(Variant::ARRAY, "friends_list")));
}

GodotEpic::GodotEpic() {
	// Initialize any variables here.
	time_passed = 0.0;
	instance = this;

	// Initialize authentication state
	epic_account_id = nullptr;
	product_user_id = nullptr;
	is_logged_in = false;
	current_username = "";
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

// Authentication methods
void GodotEpic::login_with_epic_account(const String& email, const String& password) {
	if (!platform_handle) {
		ERR_PRINT("EOS Platform not initialized");
		return;
	}

	EOS_HAuth auth_handle = EOS_Platform_GetAuthInterface(platform_handle);
	if (!auth_handle) {
		ERR_PRINT("Failed to get Auth interface");
		return;
	}

	EOS_Auth_LoginOptions login_options = {};
	login_options.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;

	// Set up credentials for Epic Account
	EOS_Auth_Credentials credentials = {};
	credentials.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
	credentials.Type = EOS_ELoginCredentialType::EOS_LCT_AccountPortal;
	credentials.Id = nullptr;  // Will open Epic launcher/browser
	credentials.Token = nullptr;

	login_options.Credentials = &credentials;
	login_options.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList;

	EOS_Auth_Login(auth_handle, &login_options, nullptr, auth_login_callback);
	ERR_PRINT("Epic Account login initiated - check browser/Epic launcher");
}

void GodotEpic::login_with_device_id(const String& display_name) {
	if (!platform_handle) {
		ERR_PRINT("EOS Platform not initialized");
		return;
	}

	EOS_HAuth auth_handle = EOS_Platform_GetAuthInterface(platform_handle);
	if (!auth_handle) {
		ERR_PRINT("Failed to get Auth interface");
		return;
	}

	EOS_Auth_LoginOptions login_options = {};
	login_options.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;

	// Set up credentials for Device ID (development only)
	EOS_Auth_Credentials credentials = {};
	credentials.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
	credentials.Type = EOS_ELoginCredentialType::EOS_LCT_Developer;
	credentials.Id = "localhost:7777";  // Development host
	credentials.Token = display_name.utf8().get_data();

	login_options.Credentials = &credentials;
	login_options.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList;

	EOS_Auth_Login(auth_handle, &login_options, nullptr, auth_login_callback);
	ERR_PRINT("Device ID login initiated");
}

void GodotEpic::logout() {
	if (!platform_handle || !is_logged_in) {
		ERR_PRINT("Not logged in or platform not initialized");
		return;
	}

	EOS_HAuth auth_handle = EOS_Platform_GetAuthInterface(platform_handle);
	if (!auth_handle) {
		ERR_PRINT("Failed to get Auth interface");
		return;
	}

	EOS_Auth_LogoutOptions logout_options = {};
	logout_options.ApiVersion = EOS_AUTH_LOGOUT_API_LATEST;
	logout_options.LocalUserId = epic_account_id;

	EOS_Auth_Logout(auth_handle, &logout_options, nullptr, auth_logout_callback);
	ERR_PRINT("Logout initiated");
}

bool GodotEpic::is_user_logged_in() const {
	return is_logged_in;
}

String GodotEpic::get_current_username() const {
	return current_username;
}

String GodotEpic::get_epic_account_id() const {
	if (!epic_account_id) {
		return "";
	}

	static char account_id_str[EOS_EPICACCOUNTID_MAX_LENGTH + 1];
	int32_t buffer_size = sizeof(account_id_str);

	EOS_EResult result = EOS_EpicAccountId_ToString(epic_account_id, account_id_str, &buffer_size);
	if (result == EOS_EResult::EOS_Success) {
		return String::utf8(account_id_str);
	}

	return "";
}

String GodotEpic::get_product_user_id() const {
	if (!product_user_id) {
		return "";
	}

	static char user_id_str[EOS_PRODUCTUSERID_MAX_LENGTH + 1];
	int32_t buffer_size = sizeof(user_id_str);

	EOS_EResult result = EOS_ProductUserId_ToString(product_user_id, user_id_str, &buffer_size);
	if (result == EOS_EResult::EOS_Success) {
		return String::utf8(user_id_str);
	}

	return "";
}

// Friends methods
void GodotEpic::query_friends() {
	if (!platform_handle || !epic_account_id) {
		ERR_PRINT("Platform not initialized or user not logged in");
		return;
	}

	EOS_HFriends friends_handle = EOS_Platform_GetFriendsInterface(platform_handle);
	if (!friends_handle) {
		ERR_PRINT("Failed to get Friends interface");
		return;
	}

	EOS_Friends_QueryFriendsOptions query_options = {};
	query_options.ApiVersion = EOS_FRIENDS_QUERYFRIENDS_API_LATEST;
	query_options.LocalUserId = epic_account_id;

	EOS_Friends_QueryFriends(friends_handle, &query_options, nullptr, friends_query_callback);
	ERR_PRINT("Friends query initiated");
}

Array GodotEpic::get_friends_list() {
	Array friends_array;

	if (!platform_handle || !epic_account_id) {
		ERR_PRINT("Platform not initialized or user not logged in");
		return friends_array;
	}

	EOS_HFriends friends_handle = EOS_Platform_GetFriendsInterface(platform_handle);
	if (!friends_handle) {
		ERR_PRINT("Failed to get Friends interface");
		return friends_array;
	}

	EOS_Friends_GetFriendsCountOptions count_options = {};
	count_options.ApiVersion = EOS_FRIENDS_GETFRIENDSCOUNT_API_LATEST;
	count_options.LocalUserId = epic_account_id;

	int32_t friends_count = EOS_Friends_GetFriendsCount(friends_handle, &count_options);

	for (int32_t i = 0; i < friends_count; i++) {
		EOS_Friends_GetFriendAtIndexOptions friend_options = {};
		friend_options.ApiVersion = EOS_FRIENDS_GETFRIENDATINDEX_API_LATEST;
		friend_options.LocalUserId = epic_account_id;
		friend_options.Index = i;

		EOS_EpicAccountId friend_id = EOS_Friends_GetFriendAtIndex(friends_handle, &friend_options);
		if (friend_id) {
			Dictionary friend_info;

			// Convert friend ID to string
			static char friend_id_str[EOS_EPICACCOUNTID_MAX_LENGTH + 1];
			int32_t buffer_size = sizeof(friend_id_str);

			EOS_EResult result = EOS_EpicAccountId_ToString(friend_id, friend_id_str, &buffer_size);
			if (result == EOS_EResult::EOS_Success) {
				friend_info["id"] = String::utf8(friend_id_str);

				// Get friend status
				EOS_Friends_GetStatusOptions status_options = {};
				status_options.ApiVersion = EOS_FRIENDS_GETSTATUS_API_LATEST;
				status_options.LocalUserId = epic_account_id;
				status_options.TargetUserId = friend_id;

				EOS_EFriendsStatus status = EOS_Friends_GetStatus(friends_handle, &status_options);

				String status_str = "Unknown";
				switch (status) {
					case EOS_EFriendsStatus::EOS_FS_Friends:
						status_str = "Friends";
						break;
					case EOS_EFriendsStatus::EOS_FS_InviteSent:
						status_str = "Invite Sent";
						break;
					case EOS_EFriendsStatus::EOS_FS_InviteReceived:
						status_str = "Invite Received";
						break;
					default:
						status_str = "Not Friends";
						break;
				}

				friend_info["status"] = status_str;
				friends_array.append(friend_info);
			}
		}
	}

	return friends_array;
}

Dictionary GodotEpic::get_friend_info(const String& friend_id) {
	Dictionary friend_info;

	if (!platform_handle || !epic_account_id) {
		ERR_PRINT("Platform not initialized or user not logged in");
		return friend_info;
	}

	// For now, return basic structure
	// In a full implementation, you'd query user info, presence, etc.
	friend_info["id"] = friend_id;
	friend_info["display_name"] = "Friend";  // Would get from UserInfo interface
	friend_info["status"] = "Unknown";
	friend_info["online"] = false;

	return friend_info;
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

// Authentication callbacks
void EOS_CALL GodotEpic::auth_login_callback(const EOS_Auth_LoginCallbackInfo* data) {
	if (!data || !instance) {
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		instance->epic_account_id = data->LocalUserId;
		instance->is_logged_in = true;

		// Get user info
		EOS_HAuth auth_handle = EOS_Platform_GetAuthInterface(platform_handle);
		if (auth_handle) {
			EOS_Auth_CopyUserAuthTokenOptions token_options = {};
			token_options.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

			EOS_Auth_Token* auth_token = nullptr;
			EOS_EResult result = EOS_Auth_CopyUserAuthToken(auth_handle, &token_options, data->LocalUserId, &auth_token);

			if (result == EOS_EResult::EOS_Success && auth_token) {
				// Use the App field as display name if available
				if (auth_token->App) {
					instance->current_username = String::utf8(auth_token->App);
				} else {
					// Fallback to a simple default
					instance->current_username = "Epic User";
				}
				EOS_Auth_Token_Release(auth_token);
			}
		}

		// Now login to Connect service for cross-platform features
		EOS_HConnect connect_handle = EOS_Platform_GetConnectInterface(platform_handle);
		if (connect_handle) {
			EOS_Connect_LoginOptions connect_options = {};
			connect_options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;

			EOS_Connect_Credentials connect_credentials = {};
			connect_credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
			connect_credentials.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC;
			connect_credentials.Token = nullptr;  // Will use current Epic session

			connect_options.Credentials = &connect_credentials;

			EOS_Connect_Login(connect_handle, &connect_options, nullptr, connect_login_callback);
		}

		ERR_PRINT("Epic Account login successful!");

		// Emit login completed signal
		instance->emit_signal("login_completed", true, instance->current_username);
	} else {
		String error_msg = "Epic Account login failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
		ERR_PRINT(error_msg);

		// Emit login failed signal
		instance->emit_signal("login_completed", false, "");
	}
}

void EOS_CALL GodotEpic::auth_logout_callback(const EOS_Auth_LogoutCallbackInfo* data) {
	if (!data || !instance) {
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		instance->epic_account_id = nullptr;
		instance->product_user_id = nullptr;
		instance->is_logged_in = false;
		instance->current_username = "";

		ERR_PRINT("Logout successful");

		// Emit logout completed signal
		instance->emit_signal("logout_completed", true);
	} else {
		String error_msg = "Logout failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
		ERR_PRINT(error_msg);

		// Emit logout failed signal
		instance->emit_signal("logout_completed", false);
	}
}

void EOS_CALL GodotEpic::connect_login_callback(const EOS_Connect_LoginCallbackInfo* data) {
	if (!data || !instance) {
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		instance->product_user_id = data->LocalUserId;
		ERR_PRINT("Connect login successful - cross-platform features enabled");
	} else {
		String error_msg = "Connect login failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
		WARN_PRINT(error_msg);
	}
}

void EOS_CALL GodotEpic::friends_query_callback(const EOS_Friends_QueryFriendsCallbackInfo* data) {
	if (!data || !instance) {
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		ERR_PRINT("Friends query successful - friends list updated");

		// Get updated friends list and emit signal
		Array friends_list = instance->get_friends_list();
		instance->emit_signal("friends_updated", friends_list);
	} else {
		String error_msg = "Friends query failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
		ERR_PRINT(error_msg);

		// Emit empty friends list on failure
		Array empty_friends;
		instance->emit_signal("friends_updated", empty_friends);
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