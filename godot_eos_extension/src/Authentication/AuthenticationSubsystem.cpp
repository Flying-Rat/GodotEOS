#include "AuthenticationSubsystem.h"
#include "../Utils/SubsystemManager.h"
#include "../Platform/IPlatformSubsystem.h"
#include "../UserInfo/IUserInfoSubsystem.h"
#include <eos_sdk.h>
#include <eos_base.h>
#include <eos_auth.h>
#include <eos_connect.h>
#include <eos_logging.h>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/classes/os.hpp>
#include <cassert>
#include "../Utils/AccountHelpers.h"

/**
 * User context passed to EOS_Connect_Login, so we know what AccountId is logging in
 */
struct FConnectLoginContext
{
	EOS_EpicAccountId AccountId;
};

namespace godot {

AuthenticationSubsystem::AuthenticationSubsystem()
    : platform_handle(nullptr)
    , auth_handle(nullptr)
    , connect_handle(nullptr)
    , local_user_id(nullptr)
    , epic_account_id(nullptr)
    , is_logged_in(false)
    , login_status(EOS_ELoginStatus::EOS_LS_NotLoggedIn)
	, logout_in_progress(false)
	, auth_logout_attempted(false)
	, connect_logout_attempted(false)
	, auth_logout_pending(false)
	, connect_logout_pending(false)
	, auth_logout_result(EOS_EResult::EOS_Success)
	, connect_logout_result(EOS_EResult::EOS_Success)
    , auth_login_status_changed_id(EOS_INVALID_NOTIFICATIONID)
    , connect_login_status_changed_id(EOS_INVALID_NOTIFICATIONID)
{
}

AuthenticationSubsystem::~AuthenticationSubsystem() {
    Shutdown();
}

bool AuthenticationSubsystem::Init() {
    UtilityFunctions::print("AuthenticationSubsystem: Initializing...");

    // Get and validate platform
    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->IsOnline()) {
        UtilityFunctions::printerr("AuthenticationSubsystem: Platform not available or offline");
        return false;
    }

    EOS_HPlatform platform_handle = platform->GetPlatformHandle();
    if (!platform_handle) {
        UtilityFunctions::printerr("AuthenticationSubsystem: Invalid platform handle");
        return false;
    }

    this->platform_handle = platform_handle;
    auth_handle = EOS_Platform_GetAuthInterface(platform_handle);
    connect_handle = EOS_Platform_GetConnectInterface(platform_handle);

    if (!auth_handle || !connect_handle) {
        UtilityFunctions::printerr("AuthenticationSubsystem: Failed to get auth/connect interfaces");
        return false;
    }

    setup_notifications();
    UtilityFunctions::print("AuthenticationSubsystem: Initialized successfully");
    return true;
}

void AuthenticationSubsystem::Tick(float delta_time) {

	// No periodic tasks needed for now
}

void AuthenticationSubsystem::Shutdown() {
    if (!auth_handle && !connect_handle) {
        UtilityFunctions::print("AuthenticationSubsystem: Already shut down, skipping");
        return;
    }

    UtilityFunctions::print("AuthenticationSubsystem: Starting shutdown...");

    // First, clean up notifications while handles are still valid
    cleanup_notifications();

    // Only logout if we have an active session and platform is still valid
    auto platform_subsystem = Get<IPlatformSubsystem>();
    if (platform_subsystem && platform_subsystem->GetPlatformHandle() &&
        (is_logged_in || EOS_ProductUserId_IsValid(local_user_id) || EOS_EpicAccountId_IsValid(epic_account_id))) {
        UtilityFunctions::print("AuthenticationSubsystem: Active session detected, logging out...");
        Logout();
    } else {
        UtilityFunctions::print("AuthenticationSubsystem: Skipping logout - platform unavailable or no active session");
    }

    is_logged_in = false;
    local_user_id = nullptr;
    epic_account_id = nullptr;
    display_name = "";
    login_status = EOS_ELoginStatus::EOS_LS_NotLoggedIn;

    reset_logout_state();

    // Clear interface handles to prevent use after shutdown
    auth_handle = nullptr;
    connect_handle = nullptr;

    UtilityFunctions::print("AuthenticationSubsystem: Shutdown complete");
}

bool AuthenticationSubsystem::Login(const String& login_type, const Dictionary& credentials) {
    if (!auth_handle || !connect_handle) {
        UtilityFunctions::push_warning("AuthenticationSubsystem: Not initialized");
        return false;
    }

    if (is_logged_in) {
        return true;
    }

    UtilityFunctions::print("AuthenticationSubsystem: Starting login with type: " + login_type);

    if (login_type == "epic_account") {
        return perform_epic_account_login(credentials);
    } else if (login_type == "dev") {
        // Try developer login first, but fallback to device_id if it fails
        return perform_developer_login(credentials);
    } else if (login_type == "device_id") {
        return perform_device_id_login();
    } else if (login_type == "exchange_code") {
        String exchange_code = credentials.get("exchange_code", "");
        return perform_exchange_code_login(exchange_code);
    } else if (login_type == "persistent_auth") {
        return perform_persistent_auth_login();
    } else if (login_type == "account_portal") {
        return perform_account_portal_login();
    } else if (login_type == "developer") {
        return perform_developer_login(credentials);
    } else {
        UtilityFunctions::printerr("AuthenticationSubsystem: Unknown login type: " + login_type);
        return false;
    }
}

bool AuthenticationSubsystem::Logout() {
	if (!auth_handle && !connect_handle) {
		UtilityFunctions::push_warning("AuthenticationSubsystem: Logout requested but subsystem not initialized");
		return false;
	}

	if (logout_in_progress) {
		UtilityFunctions::push_warning("AuthenticationSubsystem: Logout already in progress");
		return false;
	}

	const bool has_active_session = is_logged_in || EOS_ProductUserId_IsValid(local_user_id) || EOS_EpicAccountId_IsValid(epic_account_id);
	if (!has_active_session) {
		UtilityFunctions::print("AuthenticationSubsystem: Logout requested with no active session");
		logout_in_progress = true;
		finalize_logout_if_ready();
		return true;
	}

	UtilityFunctions::print("AuthenticationSubsystem: Logging out...");

	logout_in_progress = true;
	auth_logout_attempted = false;
	connect_logout_attempted = false;
	auth_logout_pending = false;
	connect_logout_pending = false;
	auth_logout_result = EOS_EResult::EOS_Success;
	connect_logout_result = EOS_EResult::EOS_Success;

	if (connect_handle && EOS_ProductUserId_IsValid(local_user_id)) {
		EOS_Connect_LogoutOptions logout_options = {};
		logout_options.ApiVersion = EOS_CONNECT_LOGOUT_API_LATEST;
		logout_options.LocalUserId = local_user_id;

		EOS_Connect_Logout(connect_handle, &logout_options, this, on_connect_logout_complete);
		connect_logout_attempted = true;
		connect_logout_pending = true;
	}

	if (auth_handle && EOS_EpicAccountId_IsValid(epic_account_id)) {
		EOS_Auth_LogoutOptions logout_options = {};
		logout_options.ApiVersion = EOS_AUTH_LOGOUT_API_LATEST;
		logout_options.LocalUserId = epic_account_id;

		EOS_Auth_Logout(auth_handle, &logout_options, this, on_auth_logout_complete);
		auth_logout_attempted = true;
		auth_logout_pending = true;
	}

	if (!auth_logout_attempted && !connect_logout_attempted) {
		UtilityFunctions::print("AuthenticationSubsystem: No active interfaces required logout. Completing immediately.");
	}

	finalize_logout_if_ready();

	UtilityFunctions::print("AuthenticationSubsystem: Logout initiated");
	return true;
}

bool AuthenticationSubsystem::IsLoggedIn() const {
    return is_logged_in;
}

EOS_ProductUserId AuthenticationSubsystem::GetProductUserId() const {
    return local_user_id;
}

void AuthenticationSubsystem::SetProductUserId(EOS_ProductUserId product_user_id) {
    local_user_id = product_user_id;
}

String AuthenticationSubsystem::GetDisplayName() const {
    return display_name;
}

int AuthenticationSubsystem::GetLoginStatus() const {
    return (int)login_status;
}

void AuthenticationSubsystem::SetLoginCallback(const Callable& callback) {
    login_callback = callback;
}

Callable AuthenticationSubsystem::GetLoginCallback() const {
    return login_callback;
}

void AuthenticationSubsystem::SetLogoutCallback(const Callable& callback) {
	logout_callback = callback;
}

Callable AuthenticationSubsystem::GetLogoutCallback() const {
	return logout_callback;
}

EOS_EpicAccountId AuthenticationSubsystem::GetEpicAccountId() const {
    return epic_account_id;
}

void AuthenticationSubsystem::setup_notifications() {
    if (!auth_handle || !connect_handle) return;

    // Auth login status changed
    EOS_Auth_AddNotifyLoginStatusChangedOptions auth_options = {};
    auth_options.ApiVersion = EOS_AUTH_ADDNOTIFYLOGINSTATUSCHANGED_API_LATEST;

    auth_login_status_changed_id = EOS_Auth_AddNotifyLoginStatusChanged(
        auth_handle, &auth_options, nullptr, on_auth_login_status_changed);

    // Connect login status changed
    EOS_Connect_AddNotifyLoginStatusChangedOptions connect_options = {};
    connect_options.ApiVersion = EOS_CONNECT_ADDNOTIFYLOGINSTATUSCHANGED_API_LATEST;

    connect_login_status_changed_id = EOS_Connect_AddNotifyLoginStatusChanged(
        connect_handle, &connect_options, nullptr, on_connect_login_status_changed);

    if (auth_login_status_changed_id != EOS_INVALID_NOTIFICATIONID &&
        connect_login_status_changed_id != EOS_INVALID_NOTIFICATIONID) {
        UtilityFunctions::print("AuthenticationSubsystem: Status change notifications registered");
    } else {
        UtilityFunctions::printerr("AuthenticationSubsystem: Failed to register status change notifications");
    }
}

void AuthenticationSubsystem::cleanup_notifications() {
    UtilityFunctions::print("AuthenticationSubsystem: Cleaning up notifications...");

    // Check if platform is still valid before trying to remove notifications
    auto platform_subsystem = Get<IPlatformSubsystem>();
    if (!platform_subsystem || !platform_subsystem->GetPlatformHandle()) {
        UtilityFunctions::print("AuthenticationSubsystem: Platform already shut down, skipping notification cleanup");
        auth_login_status_changed_id = EOS_INVALID_NOTIFICATIONID;
        connect_login_status_changed_id = EOS_INVALID_NOTIFICATIONID;
        return;
    }

    if (auth_handle && auth_login_status_changed_id != EOS_INVALID_NOTIFICATIONID) {
        EOS_Auth_RemoveNotifyLoginStatusChanged(auth_handle, auth_login_status_changed_id);
        auth_login_status_changed_id = EOS_INVALID_NOTIFICATIONID;
        UtilityFunctions::print("AuthenticationSubsystem: Auth login status notification removed");
    }

    if (connect_handle && connect_login_status_changed_id != EOS_INVALID_NOTIFICATIONID) {
        EOS_Connect_RemoveNotifyLoginStatusChanged(connect_handle, connect_login_status_changed_id);
        connect_login_status_changed_id = EOS_INVALID_NOTIFICATIONID;
        UtilityFunctions::print("AuthenticationSubsystem: Connect login status notification removed");
    }

    UtilityFunctions::print("AuthenticationSubsystem: Notification cleanup complete");
}

void AuthenticationSubsystem::reset_logout_state() {
	logout_in_progress = false;
	auth_logout_attempted = false;
	connect_logout_attempted = false;
	auth_logout_pending = false;
	connect_logout_pending = false;
	auth_logout_result = EOS_EResult::EOS_Success;
	connect_logout_result = EOS_EResult::EOS_Success;
}

void AuthenticationSubsystem::finalize_logout_if_ready() {
	if (!logout_in_progress) {
		return;
	}

	if (auth_logout_pending || connect_logout_pending) {
		return;
	}

	bool success = true;

	if (auth_logout_attempted && auth_logout_result != EOS_EResult::EOS_Success) {
		success = false;
	}

	if (connect_logout_attempted && connect_logout_result != EOS_EResult::EOS_Success) {
		success = false;
	}

	if (success) {
		UtilityFunctions::print("AuthenticationSubsystem: Logout completed successfully");
		is_logged_in = false;
		login_status = EOS_ELoginStatus::EOS_LS_NotLoggedIn;
		local_user_id = nullptr;
		epic_account_id = nullptr;
		display_name = "";
	} else {
		String error_details;
		if (auth_logout_attempted) {
			error_details += " auth=" + String::num_int64(static_cast<int64_t>(auth_logout_result));
		}
		if (connect_logout_attempted) {
			error_details += " connect=" + String::num_int64(static_cast<int64_t>(connect_logout_result));
		}
		UtilityFunctions::printerr("AuthenticationSubsystem: Logout failed" + error_details);
	}

	reset_logout_state();

	if (logout_callback.is_valid()) {
		logout_callback.call(success);
	} else {
		UtilityFunctions::printerr("AuthenticationSubsystem: Logout callback is not valid - cannot emit completion signal");
	}
}

bool AuthenticationSubsystem::perform_epic_account_login(const Dictionary& credentials) {
    String email = credentials.get("email", "");
    String password = credentials.get("password", "");

    if (email.is_empty() || password.is_empty()) {
        UtilityFunctions::push_warning("AuthenticationSubsystem: Email and password required for Epic account login");
        return false;
    }

    EOS_Auth_LoginOptions options = {};
    options.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
    options.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList | EOS_EAuthScopeFlags::EOS_AS_Presence;

    // Keep CharString temporaries alive for the duration of the call
    godot::CharString email_cs = email.utf8();
    godot::CharString password_cs = password.utf8();

    EOS_Auth_Credentials credentials_struct = {};
    credentials_struct.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
    credentials_struct.Type = EOS_ELoginCredentialType::EOS_LCT_Password;
    credentials_struct.Id = email_cs.get_data();
    credentials_struct.Token = password_cs.get_data();

    options.Credentials = &credentials_struct;

    EOS_Auth_Login(auth_handle, &options, this, auth_login_callback);
    return true;
}

bool AuthenticationSubsystem::perform_device_id_login() {
    EOS_Connect_LoginOptions options = {};
    options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;

    EOS_Connect_Credentials credentials = {};
    credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
    credentials.Type = EOS_EExternalCredentialType::EOS_ECT_DEVICEID_ACCESS_TOKEN;

    options.Credentials = &credentials;
    options.UserLoginInfo = nullptr;

    EOS_Connect_Login(connect_handle, &options, this, connect_login_callback);
    return true;
}

bool AuthenticationSubsystem::perform_exchange_code_login(const String& exchange_code) {
    if (exchange_code.is_empty()) {
        UtilityFunctions::push_warning("AuthenticationSubsystem: Exchange code required");
        return false;
    }

    EOS_Connect_LoginOptions options = {};
    options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;

    // Keep CharString temporary alive for the duration of the call
    godot::CharString exchange_code_cs = exchange_code.utf8();

    EOS_Connect_Credentials credentials = {};
    credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
    credentials.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC_ID_TOKEN;
    credentials.Token = exchange_code_cs.get_data();

    options.Credentials = &credentials;
    options.UserLoginInfo = nullptr;

    EOS_Connect_Login(connect_handle, &options, this, connect_login_callback);
    return true;
}

bool AuthenticationSubsystem::perform_persistent_auth_login() {
    EOS_Auth_LoginOptions options = {};
    options.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
    options.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList | EOS_EAuthScopeFlags::EOS_AS_Presence;

    EOS_Auth_Credentials credentials_struct = {};
    credentials_struct.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
    credentials_struct.Type = EOS_ELoginCredentialType::EOS_LCT_PersistentAuth;

    options.Credentials = &credentials_struct;

    EOS_Auth_Login(auth_handle, &options, this, auth_login_callback);
    return true;
}

bool AuthenticationSubsystem::perform_account_portal_login() {
    EOS_Auth_LoginOptions options = {};
    options.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
    options.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList | EOS_EAuthScopeFlags::EOS_AS_Presence;

    EOS_Auth_Credentials credentials_struct = {};
    credentials_struct.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
    credentials_struct.Type = EOS_ELoginCredentialType::EOS_LCT_AccountPortal;

    options.Credentials = &credentials_struct;

    EOS_Auth_Login(auth_handle, &options, this, auth_login_callback);
    return true;
}

bool AuthenticationSubsystem::perform_developer_login(const Dictionary& credentials) {
    String id = credentials.get("id", "");
    String token = credentials.get("token", "");

    if (id.is_empty()) {
        UtilityFunctions::push_warning("AuthenticationSubsystem: ID required for developer login");
        return false;
    }

    EOS_Auth_LoginOptions options = {};
    options.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
    options.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList | EOS_EAuthScopeFlags::EOS_AS_Presence;

    // Keep CharString temporaries alive for the duration of the call
    godot::CharString id_cs = id.utf8();
    godot::CharString token_cs = token.utf8();

    EOS_Auth_Credentials credentials_struct = {};
    credentials_struct.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
    credentials_struct.Type = EOS_ELoginCredentialType::EOS_LCT_Developer;
    credentials_struct.Id = id_cs.get_data();
    credentials_struct.Token = token_cs.get_data();

    options.Credentials = &credentials_struct;

    EOS_Auth_Login(auth_handle, &options, this, auth_login_callback);
    return true;
}

void EOS_CALL AuthenticationSubsystem::logging_callback(const EOS_LogMessage* message) {
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
				UtilityFunctions::printerr(log_msg);
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
				UtilityFunctions::printerr(log_msg);
			}
			break;
	}
}

void EOS_CALL AuthenticationSubsystem::auth_login_callback(const EOS_Auth_LoginCallbackInfo* data) {

	assert(data != NULL);

	AuthenticationSubsystem* instance = static_cast<AuthenticationSubsystem*>(data->ClientData);
	if (!data) {
		UtilityFunctions::printerr("AuthenticationSubsystem: auth_login_callback - data is null");
		return;
	}

	if (!instance) {
		UtilityFunctions::printerr("AuthenticationSubsystem: auth_login_callback - instance (ClientData) is null");
		return;
	}

	EOS_HAuth AuthHandle = EOS_Platform_GetAuthInterface(Get<IPlatformSubsystem>()->GetPlatformHandle());
	assert(AuthHandle != nullptr);

	if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		UtilityFunctions::print("AuthenticationSubsystem: Auth login successful");

		// User Logged In event
		FEpicAccountId UserId = data->LocalUserId;

		// Set user data
		instance->epic_account_id = UserId.AccountId;
		instance->is_logged_in = true;
		
		// Query user info to get the real display name using UserInfo subsystem
		auto userinfo = Get<IUserInfoSubsystem>();
		if (userinfo) {
			// Try to get cached user info first (might be available immediately after login)
			instance->display_name = userinfo->GetUserDisplayName(UserId.AccountId, UserId.AccountId);

			if (instance->display_name.is_empty()) {
				// Not cached yet, query it explicitly
				UtilityFunctions::print("AuthenticationSubsystem: Querying user info for display name...");
				userinfo->QueryUserInfo(UserId.AccountId, UserId.AccountId);
			}
		}

		EOS_Auth_Token* UserAuthToken = nullptr;
		EOS_Auth_CopyUserAuthTokenOptions CopyTokenOptions = { 0 };
		CopyTokenOptions.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

		if (EOS_Auth_CopyUserAuthToken(AuthHandle, &CopyTokenOptions, UserId, &UserAuthToken) == EOS_EResult::EOS_Success)
		{
			EOS_Auth_Token_Release(UserAuthToken);
		}
		else
		{
			UtilityFunctions::printerr("AuthenticationSubsystem: Failed to copy user auth token");
		}

		// Call connect login here to enable cross-platform features
		EOS_Auth_Token* ConnectUserAuthToken = nullptr;
		EOS_Auth_CopyUserAuthTokenOptions ConnectCopyTokenOptions = { 0 };
		ConnectCopyTokenOptions.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

		if (EOS_Auth_CopyUserAuthToken(AuthHandle, &ConnectCopyTokenOptions, UserId, &ConnectUserAuthToken) == EOS_EResult::EOS_Success)
		{
			EOS_Connect_Credentials Credentials = {};
			Credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
			Credentials.Token = ConnectUserAuthToken->AccessToken;
			Credentials.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC;

			EOS_Connect_LoginOptions Options = { 0 };
			Options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
			Options.Credentials = &Credentials;
			Options.UserLoginInfo = nullptr;

			// Setup a context so the callback knows what AccountId is logging in.
			std::unique_ptr<FConnectLoginContext> ClientData(new FConnectLoginContext);
			ClientData->AccountId = UserId.AccountId;

			assert(instance->connect_handle != nullptr);
			EOS_Connect_Login(instance->connect_handle, &Options, ClientData.release(), connect_login_callback);

			// Clean up the auth token
			EOS_Auth_Token_Release(ConnectUserAuthToken);
		}
		else
		{
			UtilityFunctions::printerr("AuthenticationSubsystem: Failed to copy Auth Token for Connect login - skipping Connect service");

			// Emit success anyway since Auth login succeeded, but without Connect features
			Dictionary user_info;
			user_info["display_name"] = instance->display_name;
			EOS_EpicAccountId epic_id = instance->GetEpicAccountId();
			user_info["epic_account_id"] = epic_id ? FAccountHelpers::EpicAccountIDToString(epic_id) : "";
			user_info["product_user_id"] = "";  // Empty since Connect failed

			if (instance->login_callback.is_valid()) {
				instance->login_callback.call(true, user_info);
			} else {
				UtilityFunctions::printerr("AuthenticationSubsystem: login_callback is not valid");
			}
		}	
	} else {
		// Handle specific error cases with helpful messages
		String error_msg = "AuthenticationSubsystem: auth_login_callback - Login failed with error: " + String::num_int64(static_cast<int64_t>(data->ResultCode));

		switch (data->ResultCode) {
			case EOS_EResult::EOS_InvalidCredentials:
				error_msg += " - Invalid email or password. Please check your Epic Games account credentials.";
				break;
			case EOS_EResult::EOS_InvalidParameters:
				error_msg += " - Invalid parameters. Check email format and ensure password is not empty.";
				break;
			case EOS_EResult::EOS_Auth_MFARequired:
				error_msg += " - Multi-Factor Authentication (MFA) is required for this account. Use Account Portal login instead of email/password.";
				break;
			case EOS_EResult::EOS_NoConnection:
				error_msg += " - No internet connection available.";
				break;
			case EOS_EResult::EOS_TooManyRequests:
				error_msg += " - Too many login attempts. Please wait before trying again.";
				break;
			default:
				error_msg += " - Please check your Epic Games account credentials and internet connection.";
				break;
		}

		UtilityFunctions::printerr(error_msg);

		// Emit failure signal
		if (instance->login_callback.is_valid()) {
			Dictionary user_info;
			user_info["error_code"] = static_cast<int64_t>(data->ResultCode);
			user_info["error_message"] = error_msg;
			instance->login_callback.call(false, user_info);
		}
	}
}

void EOS_CALL AuthenticationSubsystem::auth_logout_callback(const EOS_Auth_LogoutCallbackInfo* data) {
	AuthenticationSubsystem* instance = static_cast<AuthenticationSubsystem*>(data->ClientData);
	if (!data || !instance) {
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		// Clear user data on successful logout
		instance->epic_account_id = nullptr;
		instance->local_user_id = nullptr;
		instance->is_logged_in = false;
		instance->display_name = "";

		UtilityFunctions::print("Logout successful");

		// Note: Logout completion is handled by the caller
	} else {
		String error_msg = "Logout failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
		UtilityFunctions::printerr(error_msg);

		// Note: Logout completion is handled by the caller
	}
}

void EOS_CALL AuthenticationSubsystem::connect_login_callback(const EOS_Connect_LoginCallbackInfo* data) {
	std::unique_ptr<FConnectLoginContext> ClientData(static_cast<FConnectLoginContext*>(data->ClientData));
	if (!ClientData) {
		UtilityFunctions::printerr("AuthenticationSubsystem: connect_login_callback - ClientData is null");
		return;
	}

	IAuthenticationSubsystem* authIterface = Get<IAuthenticationSubsystem>();
	if(authIterface == nullptr) {
		UtilityFunctions::printerr("AuthenticationSubsystem: connect_login_callback - Get<IAuthenticationSubsystem>() returned null");
		return;
	}

	// Get the login callback from the interface
	Callable login_callback = authIterface->GetLoginCallback();

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		// Set Product User ID directly from handle
		authIterface->SetProductUserId(data->LocalUserId);

		// Now both Auth and Connect logins are complete, emit the signal
		Dictionary user_info;
		user_info["display_name"] = authIterface->GetDisplayName();
		EOS_EpicAccountId epic_id = authIterface->GetEpicAccountId();
		user_info["epic_account_id"] = epic_id ? FAccountHelpers::EpicAccountIDToString(epic_id) : "";
		EOS_ProductUserId product_user_id = authIterface->GetProductUserId();
		String product_user_id_str = "";
		if (EOS_ProductUserId_IsValid(product_user_id)) {
			product_user_id_str = FAccountHelpers::ProductUserIDToString(product_user_id);
		}
		user_info["product_user_id"] = product_user_id_str;

		if (login_callback.is_valid()) {
			UtilityFunctions::print("AuthenticationSubsystem: Login completed successfully");
			login_callback.call(true, user_info);
		} else {
			UtilityFunctions::printerr("AuthenticationSubsystem: Login callback is not valid - cannot emit success signal");
		}
	} else {
		String error_msg = "Connect login failed: ";

		// Provide more descriptive error messages for Connect login
		switch (data->ResultCode) {
			case EOS_EResult::EOS_InvalidParameters:
				error_msg += "Invalid parameters (10) - Connect login requires valid Epic Account ID from Auth login";
				break;
			case EOS_EResult::EOS_InvalidUser:
				error_msg += "Invalid user (3) - User may need to be linked or created in Connect service";
				break;
			case EOS_EResult::EOS_NotFound:
				error_msg += "User not found (13) - User account may need to be created in Connect service";
				break;
			case EOS_EResult::EOS_DuplicateNotAllowed:
				error_msg += "Duplicate not allowed (15) - User may already be logged in";
				break;
			case EOS_EResult::EOS_Connect_ExternalTokenValidationFailed:
				error_msg += "External token validation failed (7000) - Epic Account ID token was rejected by Connect service. Try using Auth Token instead of Account ID";
				break;
			case EOS_EResult::EOS_Connect_InvalidToken:
				error_msg += "Invalid token (7003) - The provided token is not valid for Connect service";
				break;
			case EOS_EResult::EOS_Connect_UnsupportedTokenType:
				error_msg += "Unsupported token type (7004) - Connect service doesn't support this token type";
				break;
			case EOS_EResult::EOS_Connect_AuthExpired:
				error_msg += "Auth expired (7002) - The authentication token has expired";
				break;
			default:
				error_msg += String::num_int64(static_cast<int64_t>(data->ResultCode));
				break;
		}
		UtilityFunctions::printerr(error_msg);

		// Connect failed, but Auth succeeded - still emit login signal but without product_user_id
		Dictionary user_info;
		user_info["display_name"] = authIterface->GetDisplayName();
		EOS_EpicAccountId epic_id_failure = authIterface->GetEpicAccountId();
		user_info["epic_account_id"] = epic_id_failure ? FAccountHelpers::EpicAccountIDToString(epic_id_failure) : "";
		user_info["product_user_id"] = "";  // Empty since Connect failed

		if (login_callback.is_valid()) {
			login_callback.call(true, user_info);
		}
	}
}


void EOS_CALL AuthenticationSubsystem::on_auth_logout_complete(const EOS_Auth_LogoutCallbackInfo* data) {
	AuthenticationSubsystem* instance = static_cast<AuthenticationSubsystem*>(data->ClientData);
	if (!data || !instance) {
		return;
	}

	instance->auth_logout_attempted = true;
	instance->auth_logout_result = data->ResultCode;
	instance->auth_logout_pending = false;

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		UtilityFunctions::print("AuthenticationSubsystem: Auth logout callback completed");
	} else {
		String error_msg = "Auth logout failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
		UtilityFunctions::printerr(error_msg);
	}

	instance->finalize_logout_if_ready();
}

void EOS_CALL AuthenticationSubsystem::on_connect_logout_complete(const EOS_Connect_LogoutCallbackInfo* data) {
	AuthenticationSubsystem* instance = static_cast<AuthenticationSubsystem*>(data->ClientData);
	if (!data || !instance) {
		return;
	}

	instance->connect_logout_attempted = true;
	instance->connect_logout_result = data->ResultCode;
	instance->connect_logout_pending = false;

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		UtilityFunctions::print("AuthenticationSubsystem: Connect logout callback completed");
	} else {
		String error_msg = "Connect logout failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
		UtilityFunctions::printerr(error_msg);
	}

	instance->finalize_logout_if_ready();
}

void EOS_CALL AuthenticationSubsystem::on_auth_login_status_changed(const EOS_Auth_LoginStatusChangedCallbackInfo* data) {
    if (!data) return;

    UtilityFunctions::print("AuthenticationSubsystem: Auth login status changed: " + String::num_int64((int64_t)data->CurrentStatus));
}

void EOS_CALL AuthenticationSubsystem::on_connect_login_status_changed(const EOS_Connect_LoginStatusChangedCallbackInfo* data) {
    if (!data) return;

    UtilityFunctions::print("AuthenticationSubsystem: Connect login status changed: " + String::num_int64((int64_t)data->CurrentStatus));
}

} // namespace godot
