#include "AuthenticationSubsystem.h"
#include "SubsystemManager.h"
#include "IPlatformSubsystem.h"
#include "../eos_sdk/Include/eos_sdk.h"
#include "../eos_sdk/Include/eos_auth.h"
#include "../eos_sdk/Include/eos_connect.h"
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/classes/os.hpp>

namespace godot {

AuthenticationSubsystem::AuthenticationSubsystem()
    : auth_handle(nullptr)
    , connect_handle(nullptr)
    , is_logged_in(false)
    , login_status(EOS_ELoginStatus::EOS_LS_NotLoggedIn)
    , auth_login_status_changed_id(EOS_INVALID_NOTIFICATIONID)
    , connect_login_status_changed_id(EOS_INVALID_NOTIFICATIONID)
{
}

AuthenticationSubsystem::~AuthenticationSubsystem() {
    Shutdown();
}

bool AuthenticationSubsystem::Init() {
    UtilityFunctions::print("AuthenticationSubsystem: Initializing...");

    // Get platform handle from PlatformSubsystem
    auto platform = Get<IPlatformSubsystem>();
    if (!platform) {
        UtilityFunctions::printerr("AuthenticationSubsystem: PlatformSubsystem not available");
        return false;
    }

    if (!platform->IsOnline()) {
        UtilityFunctions::printerr("AuthenticationSubsystem: Platform not online");
        return false;
    }

    EOS_HPlatform platform_handle = static_cast<EOS_HPlatform>(platform->GetPlatformHandle());
    if (!platform_handle) {
        UtilityFunctions::printerr("AuthenticationSubsystem: Invalid platform handle");
        return false;
    }

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
    // Check login status periodically
    if (auth_handle && !epic_account_id.is_empty()) {
        EOS_EpicAccountId eaid = EOS_EpicAccountId_FromString(epic_account_id.utf8().get_data());
        if (EOS_EpicAccountId_IsValid(eaid)) {
            EOS_ELoginStatus current_status = EOS_Auth_GetLoginStatus(auth_handle, eaid);
            if (current_status != login_status) {
                login_status = current_status;
                UtilityFunctions::print("AuthenticationSubsystem: Login status changed to: " + String::num_int64((int)login_status));
            }
        }
    }
}

void AuthenticationSubsystem::Shutdown() {
    Logout();
    cleanup_notifications();

    is_logged_in = false;
    product_user_id = "";
    epic_account_id = "";
    display_name = "";
    login_status = EOS_ELoginStatus::EOS_LS_NotLoggedIn;

    UtilityFunctions::print("AuthenticationSubsystem: Shutdown complete");
}

bool AuthenticationSubsystem::Login(const String& login_type, const Dictionary& credentials) {
    if (!auth_handle || !connect_handle) {
        UtilityFunctions::printerr("AuthenticationSubsystem: Not initialized");
        return false;
    }

    if (is_logged_in) {
        UtilityFunctions::print("AuthenticationSubsystem: Already logged in");
        return true;
    }

    UtilityFunctions::print("AuthenticationSubsystem: Attempting login with type: " + login_type);

    if (login_type == "epic_account") {
        return perform_epic_account_login(credentials);
    } else if (login_type == "dev") {
        // Try developer login first, but fallback to device_id if it fails
        UtilityFunctions::print("AuthenticationSubsystem: Attempting developer login (requires EOS Dev Auth Tool)");
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
    if (!is_logged_in) {
        return true;
    }

    UtilityFunctions::print("AuthenticationSubsystem: Logging out...");

    // Logout from Connect first
    if (connect_handle && !product_user_id.is_empty()) {
        EOS_ProductUserId puid = EOS_ProductUserId_FromString(product_user_id.utf8().get_data());
        if (EOS_ProductUserId_IsValid(puid)) {
            EOS_Connect_LogoutOptions logout_options = {};
            logout_options.ApiVersion = EOS_CONNECT_LOGOUT_API_LATEST;
            logout_options.LocalUserId = puid;

            EOS_Connect_Logout(connect_handle, &logout_options, nullptr, on_connect_logout_complete);
        }
    }

    // Logout from Auth
    if (auth_handle && !epic_account_id.is_empty()) {
        EOS_EpicAccountId eaid = EOS_EpicAccountId_FromString(epic_account_id.utf8().get_data());
        if (EOS_EpicAccountId_IsValid(eaid)) {
            EOS_Auth_LogoutOptions logout_options = {};
            logout_options.ApiVersion = EOS_AUTH_LOGOUT_API_LATEST;
            logout_options.LocalUserId = eaid;

            EOS_Auth_Logout(auth_handle, &logout_options, nullptr, on_auth_logout_complete);
        }
    }

    is_logged_in = false;
    product_user_id = "";
    epic_account_id = "";
    display_name = "";
    login_status = EOS_ELoginStatus::EOS_LS_NotLoggedIn;

    return true;
}

bool AuthenticationSubsystem::IsLoggedIn() const {
    return is_logged_in;
}

String AuthenticationSubsystem::GetProductUserId() const {
    return product_user_id;
}

String AuthenticationSubsystem::GetEpicAccountId() const {
    return epic_account_id;
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
    if (auth_handle && auth_login_status_changed_id != EOS_INVALID_NOTIFICATIONID) {
        EOS_Auth_RemoveNotifyLoginStatusChanged(auth_handle, auth_login_status_changed_id);
        auth_login_status_changed_id = EOS_INVALID_NOTIFICATIONID;
    }

    if (connect_handle && connect_login_status_changed_id != EOS_INVALID_NOTIFICATIONID) {
        EOS_Connect_RemoveNotifyLoginStatusChanged(connect_handle, connect_login_status_changed_id);
        connect_login_status_changed_id = EOS_INVALID_NOTIFICATIONID;
    }

    UtilityFunctions::print("AuthenticationSubsystem: Status change notifications unregistered");
}

bool AuthenticationSubsystem::perform_epic_account_login(const Dictionary& credentials) {
    String email = credentials.get("email", "");
    String password = credentials.get("password", "");

    if (email.is_empty() || password.is_empty()) {
        UtilityFunctions::printerr("AuthenticationSubsystem: Email and password required for Epic account login");
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

    EOS_Auth_Login(auth_handle, &options, this, on_auth_login_complete);
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

    EOS_Connect_Login(connect_handle, &options, this, on_connect_login_complete);
    return true;
}

bool AuthenticationSubsystem::perform_exchange_code_login(const String& exchange_code) {
    if (exchange_code.is_empty()) {
        UtilityFunctions::printerr("AuthenticationSubsystem: Exchange code required");
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

    EOS_Connect_Login(connect_handle, &options, this, on_connect_login_complete);
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

    EOS_Auth_Login(auth_handle, &options, this, on_auth_login_complete);
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

    EOS_Auth_Login(auth_handle, &options, this, on_auth_login_complete);
    return true;
}

bool AuthenticationSubsystem::perform_developer_login(const Dictionary& credentials) {
    String id = credentials.get("id", "");
    String token = credentials.get("token", "");

    if (id.is_empty()) {
        UtilityFunctions::printerr("AuthenticationSubsystem: ID required for developer login");
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

void EOS_CALL AuthenticationSubsystem::auth_login_callback(const EOS_Auth_LoginCallbackInfo* data) {
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
			// Get Auth Token for Connect login (not Epic Account ID)
			EOS_Auth_Token* auth_token = nullptr;
			EOS_Auth_CopyUserAuthTokenOptions copy_options = {};
			copy_options.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

			EOS_EResult copy_result = EOS_Auth_CopyUserAuthToken(auth_handle, &copy_options, data->LocalUserId, &auth_token);

			if (copy_result == EOS_EResult::EOS_Success && auth_token) {
				EOS_Connect_LoginOptions connect_options = {};
				connect_options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;

				EOS_Connect_Credentials connect_credentials = {};
				connect_credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
				connect_credentials.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC;
				connect_credentials.Token = auth_token->AccessToken;  // Use Auth Token instead of Account ID

				connect_options.Credentials = &connect_credentials;

				EOS_Connect_Login(connect_handle, &connect_options, nullptr, connect_login_callback);

				// Clean up the auth token
				EOS_Auth_Token_Release(auth_token);
			} else {
				ERR_PRINT("Failed to copy Auth Token for Connect login - skipping Connect service");
				// Emit success anyway since Auth login succeeded, but without Connect features
				Dictionary user_info;
				user_info["display_name"] = instance->current_username;
				user_info["epic_account_id"] = instance->get_epic_account_id();
				user_info["product_user_id"] = "";  // Empty since Connect failed

				instance->emit_signal("login_completed", true, user_info);
			}
		}

		ERR_PRINT("Epic Account login successful - initiating Connect login...");

		// Don't emit login signal yet - wait for Connect login to complete
	} else {
		String error_msg = "Epic Account login failed: ";

		// Provide more descriptive error messages
		switch (data->ResultCode) {
			case EOS_EResult::EOS_InvalidParameters:
				error_msg += "Invalid parameters (10) - For Device ID login, make sure EOS Dev Auth Tool is running on localhost:7777. For Epic Account login, check email/password format";
				break;
			case EOS_EResult::EOS_Invalid_Deployment:
				error_msg += "Invalid deployment (32) - Check your deployment_id in EOS Developer Portal";
				break;
			case EOS_EResult::EOS_InvalidCredentials:
				error_msg += "Invalid credentials (2) - Check your email/password";
				break;
			case EOS_EResult::EOS_InvalidUser:
				error_msg += "Invalid user (3) - User may need to be linked";
				break;
			case EOS_EResult::EOS_MissingPermissions:
				error_msg += "Missing permissions (6) - Check app permissions in EOS Developer Portal";
				break;
			case EOS_EResult::EOS_ApplicationSuspended:
				error_msg += "Application suspended (40) - App may be suspended in EOS Developer Portal";
				break;
			case EOS_EResult::EOS_NetworkDisconnected:
				error_msg += "Network disconnected (41) - Check internet connection";
				break;
			case EOS_EResult::EOS_NotConfigured:
				error_msg += "Not configured (14) - Check your EOS app configuration";
				break;
			case EOS_EResult::EOS_Invalid_Sandbox:
				error_msg += "Invalid sandbox (31) - Check your sandbox_id in EOS Developer Portal";
				break;
			case EOS_EResult::EOS_Invalid_Product:
				error_msg += "Invalid product (33) - Check your product_id in EOS Developer Portal";
				break;
			default:
				error_msg += String::num_int64(static_cast<int64_t>(data->ResultCode));
				break;
		}

		ERR_PRINT(error_msg);

		// Emit login failed signal
		Dictionary empty_user_info;
		instance->emit_signal("login_completed", false, empty_user_info);
	}
}

void EOS_CALL AuthenticationSubsystem::auth_logout_callback(const EOS_Auth_LogoutCallbackInfo* data) {
	if (!data || !instance) {
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		instance->epic_account_id = data->LocalUserId;
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

void EOS_CALL AuthenticationSubsystem::connect_login_callback(const EOS_Connect_LoginCallbackInfo* data) {
	if (!data || !instance) {
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		instance->product_user_id = data->LocalUserId;
		WARN_PRINT("Connect login successful - cross-platform features enabled");

		// Now both Auth and Connect logins are complete, emit the signal
		Dictionary user_info;
		user_info["display_name"] = instance->current_username;
		user_info["epic_account_id"] = instance->get_epic_account_id();
		user_info["product_user_id"] = instance->get_product_user_id();

		instance->emit_signal("login_completed", true, user_info);
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
		}		ERR_PRINT(error_msg);

		// Connect failed, but Auth succeeded - still emit login signal but without product_user_id
		ERR_PRINT("Login completed with Epic Account only (no cross-platform features)");
		Dictionary user_info;
		user_info["display_name"] = instance->current_username;
		user_info["epic_account_id"] = instance->get_epic_account_id();
		user_info["product_user_id"] = "";  // Empty since Connect failed

		instance->emit_signal("login_completed", true, user_info);
	}
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