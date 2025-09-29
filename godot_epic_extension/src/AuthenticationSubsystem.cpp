#include "AuthenticationSubsystem.h"
#include "SubsystemManager.h"
#include "IPlatformSubsystem.h"
#include "../eos_sdk/Include/eos_sdk.h"
#include "../eos_sdk/Include/eos_base.h"
#include "../eos_sdk/Include/eos_auth.h"
#include "../eos_sdk/Include/eos_connect.h"
#include "../eos_sdk/Include/eos_logging.h"
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/classes/os.hpp>
#include <cassert>
#include "AccountHelpers.h"

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
        EOS_ProductUserId puid = FAccountHelpers::ProductUserIDFromString(product_user_id.utf8().get_data());
        if (EOS_ProductUserId_IsValid(puid)) {
            EOS_Connect_LogoutOptions logout_options = {};
            logout_options.ApiVersion = EOS_CONNECT_LOGOUT_API_LATEST;
            logout_options.LocalUserId = puid;

            EOS_Connect_Logout(connect_handle, &logout_options, this, on_connect_logout_complete);
        }
    }

    // Logout from Auth
    if (auth_handle && !epic_account_id.is_empty()) {
        EOS_EpicAccountId eaid = FAccountHelpers::EpicAccountIDFromString(epic_account_id.utf8().get_data());
        if (EOS_EpicAccountId_IsValid(eaid)) {
            EOS_Auth_LogoutOptions logout_options = {};
            logout_options.ApiVersion = EOS_AUTH_LOGOUT_API_LATEST;
            logout_options.LocalUserId = eaid;

            EOS_Auth_Logout(auth_handle, &logout_options, this, on_auth_logout_complete);
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

EOS_EpicAccountId AuthenticationSubsystem::GetRawEpicAccountId() const {
    if (epic_account_id.is_empty()) {
        return nullptr;
    }
    return FAccountHelpers::EpicAccountIDFromString(epic_account_id.utf8().get_data());
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

	assert(data != NULL);

	UtilityFunctions::print("AuthenticationSubsystem: Login Complete - User ID: " + String(FEpicAccountId(data->LocalUserId).ToString().c_str()));


	UtilityFunctions::print("AuthenticationSubsystem: auth_login_callback called");

	AuthenticationSubsystem* instance = static_cast<AuthenticationSubsystem*>(data->ClientData);
	if (!data) {
		UtilityFunctions::printerr("AuthenticationSubsystem: auth_login_callback - data is null");
		return;
	}

	if (!instance) {
		UtilityFunctions::printerr("AuthenticationSubsystem: auth_login_callback - instance (ClientData) is null");
		return;
	}

	UtilityFunctions::print("AuthenticationSubsystem: auth_login_callback - ResultCode: " + String::num_int64(static_cast<int64_t>(data->ResultCode)));
	UtilityFunctions::print("AuthenticationSubsystem: auth_login_callback - LocalUserId valid: " + String::num(EOS_EpicAccountId_IsValid(data->LocalUserId)));

	EOS_HAuth AuthHandle = EOS_Platform_GetAuthInterface(static_cast<EOS_HPlatform>(Get<IPlatformSubsystem>()->GetPlatformHandle()));
	assert(AuthHandle != nullptr);

	if (data->ResultCode == EOS_EResult::EOS_Success)
	{
		const int32_t AccountsCount = EOS_Auth_GetLoggedInAccountsCount(AuthHandle);
		UtilityFunctions::print("AuthenticationSubsystem: Found " + String::num(AccountsCount) + " logged-in accounts");

		for (int32_t AccountIdx = 0; AccountIdx < AccountsCount; ++AccountIdx)
		{
			UtilityFunctions::print("AuthenticationSubsystem: Processing account " + String::num(AccountIdx) + " of " + String::num(AccountsCount));

			FEpicAccountId AccountId;
			AccountId = EOS_Auth_GetLoggedInAccountByIndex(AuthHandle, AccountIdx);
			UtilityFunctions::print("AuthenticationSubsystem: Retrieved account ID for index " + String::num(AccountIdx));

			EOS_ELoginStatus LoginStatus;
			LoginStatus = EOS_Auth_GetLoginStatus(AuthHandle, data->LocalUserId);

			UtilityFunctions::print("AuthenticationSubsystem: Account ID: " + String(FEpicAccountId(AccountId).ToString().c_str()) + ", Status: " + String::num((int32_t)LoginStatus));


			UtilityFunctions::print("AuthenticationSubsystem: Retrieved login status: " + String::num((int32_t)LoginStatus));
		}

		UtilityFunctions::print("AuthenticationSubsystem: Finished processing all logged-in accounts");

		// User Logged In event
		UtilityFunctions::print("AuthenticationSubsystem: Processing successful login for user");

		FEpicAccountId UserId = data->LocalUserId;
		UtilityFunctions::print("AuthenticationSubsystem: User ID: " + String(FEpicAccountId(UserId).ToString().c_str()));

		// Set user data
		instance->epic_account_id = String(FEpicAccountId(UserId).ToString().c_str());
		instance->is_logged_in = true;
		instance->display_name = "Epic User"; // Default display name
		UtilityFunctions::print("AuthenticationSubsystem: Set epic_account_id: " + instance->epic_account_id);
		UtilityFunctions::print("AuthenticationSubsystem: Set is_logged_in = true");

		EOS_Auth_Token* UserAuthToken = nullptr;
		EOS_Auth_CopyUserAuthTokenOptions CopyTokenOptions = { 0 };
		CopyTokenOptions.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

		if (EOS_Auth_CopyUserAuthToken(AuthHandle, &CopyTokenOptions, UserId, &UserAuthToken) == EOS_EResult::EOS_Success)
		{
			UtilityFunctions::print("AuthenticationSubsystem: Auth token copied successfully");
			UtilityFunctions::print("AuthenticationSubsystem: Auth token details - App: " + String(UserAuthToken->App ? UserAuthToken->App : "null") + ", ExpiresIn: " + String::num(UserAuthToken->ExpiresIn));
			EOS_Auth_Token_Release(UserAuthToken);
		}
		else
		{
			UtilityFunctions::printerr("AuthenticationSubsystem: Failed to copy user auth token");
		}

		UtilityFunctions::print("AuthenticationSubsystem: Initiating Connect login for cross-platform features");

		// Call connect login here to enable cross-platform features
		EOS_Auth_Token* ConnectUserAuthToken = nullptr;
		EOS_Auth_CopyUserAuthTokenOptions ConnectCopyTokenOptions = { 0 };
		ConnectCopyTokenOptions.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

		if (EOS_Auth_CopyUserAuthToken(AuthHandle, &ConnectCopyTokenOptions, UserId, &ConnectUserAuthToken) == EOS_EResult::EOS_Success)
		{
			UtilityFunctions::print("AuthenticationSubsystem: Auth token for Connect login copied successfully");
			UtilityFunctions::print("AuthenticationSubsystem: AccessToken length: " + String::num(strlen(ConnectUserAuthToken->AccessToken)));

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

			UtilityFunctions::print("AuthenticationSubsystem: Calling EOS_Connect_Login...");
			assert(instance->connect_handle != nullptr);
			EOS_Connect_Login(instance->connect_handle, &Options, ClientData.release(), connect_login_callback);
			UtilityFunctions::print("AuthenticationSubsystem: EOS_Connect_Login called");

			// Clean up the auth token
			EOS_Auth_Token_Release(ConnectUserAuthToken);
			UtilityFunctions::print("AuthenticationSubsystem: Released Connect auth token");
		}
		else
		{
			UtilityFunctions::printerr("AuthenticationSubsystem: Failed to copy Auth Token for Connect login - skipping Connect service");

			// Emit success anyway since Auth login succeeded, but without Connect features
			Dictionary user_info;
			user_info["display_name"] = instance->display_name;
			user_info["epic_account_id"] = instance->GetEpicAccountId();
			user_info["product_user_id"] = "";  // Empty since Connect failed

			UtilityFunctions::print("AuthenticationSubsystem: Emitting login success without Connect features");
			if (instance->login_callback.is_valid()) {
				instance->login_callback.call(true, user_info);
				UtilityFunctions::print("AuthenticationSubsystem: login_callback.call completed");
			} else {
				UtilityFunctions::printerr("AuthenticationSubsystem: login_callback is not valid");
			}
		}	}
	else{
		UtilityFunctions::printerr("AuthenticationSubsystem: auth_login_callback - Login failed with error: " + String::num_int64(static_cast<int64_t>(data->ResultCode)));
	}

	

	/*

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		UtilityFunctions::print("AuthenticationSubsystem: Auth login successful, processing user data...");

		// Convert Epic Account ID to string
		char epic_id_string[EOS_EPICACCOUNTID_MAX_LENGTH];
		int32_t epic_id_length = sizeof(epic_id_string);
		EOS_EResult result = EOS_EpicAccountId_ToString(data->LocalUserId, epic_id_string, &epic_id_length);

		UtilityFunctions::print("AuthenticationSubsystem: EpicAccountId_ToString result: " + String::num_int64(static_cast<int64_t>(result)) + ", length: " + String::num(epic_id_length));

		if (result == EOS_EResult::EOS_Success && epic_id_length > 0) {
			instance->epic_account_id = String::utf8(epic_id_string, epic_id_length);
			UtilityFunctions::print("AuthenticationSubsystem: Set epic_account_id: " + instance->epic_account_id);
		} else {
			UtilityFunctions::printerr("AuthenticationSubsystem: Failed to convert Epic Account ID to string");
		}

		instance->is_logged_in = true;
		UtilityFunctions::print("AuthenticationSubsystem: Set is_logged_in = true");

		// Get user info
		EOS_HAuth auth_handle = EOS_Platform_GetAuthInterface(instance->platform_handle);
		if (!auth_handle) {
			UtilityFunctions::printerr("AuthenticationSubsystem: Failed to get auth interface from platform handle");
		} else {
			UtilityFunctions::print("AuthenticationSubsystem: Got auth interface, copying user auth token...");

			EOS_Auth_CopyUserAuthTokenOptions token_options = {};
			token_options.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

			EOS_Auth_Token* auth_token = nullptr;
			EOS_EResult result = EOS_Auth_CopyUserAuthToken(auth_handle, &token_options, data->LocalUserId, &auth_token);

			UtilityFunctions::print("AuthenticationSubsystem: CopyUserAuthToken result: " + String::num_int64(static_cast<int64_t>(result)));

			if (result == EOS_EResult::EOS_Success && auth_token) {
				UtilityFunctions::print("AuthenticationSubsystem: Auth token copied successfully");

				// Use the App field as display name if available
				if (auth_token->App) {
					instance->display_name = String::utf8(auth_token->App);
					UtilityFunctions::print("AuthenticationSubsystem: Set display_name from App field: " + instance->display_name);
				} else {
					instance->display_name = "Epic User";
					UtilityFunctions::print("AuthenticationSubsystem: Set display_name to default: " + instance->display_name);
				}
				EOS_Auth_Token_Release(auth_token);
				UtilityFunctions::print("AuthenticationSubsystem: Released auth token");
			} else {
				UtilityFunctions::printerr("AuthenticationSubsystem: Failed to copy user auth token");
			}
		}

		// Now login to Connect service for cross-platform features
		EOS_HConnect connect_handle = EOS_Platform_GetConnectInterface(instance->platform_handle);
		if (!connect_handle) {
			UtilityFunctions::printerr("AuthenticationSubsystem: Failed to get connect interface from platform handle");
		} else {
			UtilityFunctions::print("AuthenticationSubsystem: Got connect interface, attempting Connect login...");

			// Get Auth Token for Connect login (not Epic Account ID)
			EOS_Auth_Token* auth_token = nullptr;
			EOS_Auth_CopyUserAuthTokenOptions copy_options = {};
			copy_options.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

			EOS_EResult copy_result = EOS_Auth_CopyUserAuthToken(auth_handle, &copy_options, data->LocalUserId, &auth_token);

			UtilityFunctions::print("AuthenticationSubsystem: CopyUserAuthToken for Connect result: " + String::num_int64(static_cast<int64_t>(copy_result)));

			if (copy_result == EOS_EResult::EOS_Success && auth_token) {
				UtilityFunctions::print("AuthenticationSubsystem: Auth token for Connect login copied successfully");
				UtilityFunctions::print("AuthenticationSubsystem: AccessToken length: " + String::num(strlen(auth_token->AccessToken)));

				EOS_Connect_LoginOptions connect_options = {};
				connect_options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;

				EOS_Connect_Credentials connect_credentials = {};
				connect_credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
				connect_credentials.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC;
				connect_credentials.Token = auth_token->AccessToken;  // Use Auth Token instead of Account ID

				connect_options.Credentials = &connect_credentials;

				UtilityFunctions::print("AuthenticationSubsystem: Calling EOS_Connect_Login...");
				EOS_Connect_Login(connect_handle, &connect_options, instance, connect_login_callback);
				UtilityFunctions::print("AuthenticationSubsystem: EOS_Connect_Login called");

				// Clean up the auth token
				EOS_Auth_Token_Release(auth_token);
				UtilityFunctions::print("AuthenticationSubsystem: Released Connect auth token");
			} else {
				UtilityFunctions::printerr("AuthenticationSubsystem: Failed to copy Auth Token for Connect login - skipping Connect service");

				// Emit success anyway since Auth login succeeded, but without Connect features
				Dictionary user_info;
				user_info["display_name"] = instance->display_name;
				user_info["epic_account_id"] = instance->GetEpicAccountId();
				user_info["product_user_id"] = "";  // Empty since Connect failed

				UtilityFunctions::print("AuthenticationSubsystem: Emitting login success without Connect features");
				UtilityFunctions::print("AuthenticationSubsystem: login_callback valid: " + String::num(instance->login_callback.is_valid()));

				if (instance->login_callback.is_valid()) {
					instance->login_callback.call(true, user_info);
					UtilityFunctions::print("AuthenticationSubsystem: login_callback.call completed");
				} else {
					UtilityFunctions::printerr("AuthenticationSubsystem: login_callback is not valid");
				}
			}
		}

		UtilityFunctions::print("AuthenticationSubsystem: Epic Account login successful - initiating Connect login...");

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

		UtilityFunctions::printerr(error_msg);
		UtilityFunctions::printerr("AuthenticationSubsystem: Auth login failed with ResultCode: " + String::num_int64(static_cast<int64_t>(data->ResultCode)));

		// Emit login failed signal
		Dictionary empty_user_info;
		UtilityFunctions::print("AuthenticationSubsystem: Emitting login failure");
		UtilityFunctions::print("AuthenticationSubsystem: login_callback valid: " + String::num(instance->login_callback.is_valid()));

		if (instance->login_callback.is_valid()) {
			instance->login_callback.call(false, empty_user_info);
			UtilityFunctions::print("AuthenticationSubsystem: login_callback.call (failure) completed");
		} else {
			UtilityFunctions::printerr("AuthenticationSubsystem: login_callback is not valid for failure case");
		}
	}

	UtilityFunctions::print("AuthenticationSubsystem: auth_login_callback completed");
	*/
}

void EOS_CALL AuthenticationSubsystem::auth_logout_callback(const EOS_Auth_LogoutCallbackInfo* data) {
	AuthenticationSubsystem* instance = static_cast<AuthenticationSubsystem*>(data->ClientData);
	if (!data || !instance) {
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		// Clear user data on successful logout
		instance->epic_account_id = "";
		instance->product_user_id = "";
		instance->is_logged_in = false;
		instance->display_name = "";

		ERR_PRINT("Logout successful");

		// Note: Logout completion is handled by the caller
	} else {
		String error_msg = "Logout failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
		ERR_PRINT(error_msg);

		// Note: Logout completion is handled by the caller
	}
}

void EOS_CALL AuthenticationSubsystem::connect_login_callback(const EOS_Connect_LoginCallbackInfo* data) {
	UtilityFunctions::print("AuthenticationSubsystem: connect_login_callback called");
	
	std::unique_ptr<FConnectLoginContext> ClientData(static_cast<FConnectLoginContext*>(data->ClientData));
	if (!ClientData) {
		UtilityFunctions::printerr("AuthenticationSubsystem: connect_login_callback - ClientData is null");
		return;
	}
	else {
		UtilityFunctions::print("AuthenticationSubsystem: connect_login_callback - ClientData valid");
	}

	IAuthenticationSubsystem* authIterface = Get<IAuthenticationSubsystem>();
	if(authIterface == nullptr) {
		UtilityFunctions::printerr("AuthenticationSubsystem: connect_login_callback - Get<IAuthenticationSubsystem>() returned null");
		return;
	}
	else {
		UtilityFunctions::print("AuthenticationSubsystem: connect_login_callback - Got IAuthenticationSubsystem interface");
	}

	AuthenticationSubsystem* auth = static_cast<AuthenticationSubsystem*>(authIterface);
	if (auth == nullptr) {
		UtilityFunctions::printerr("AuthenticationSubsystem: connect_login_callback - static_cast to AuthenticationSubsystem returned null");
		return;
	}
	else {
		UtilityFunctions::print("AuthenticationSubsystem: connect_login_callback - static_cast to AuthenticationSubsystem successful");
	}
	
	if (data->ResultCode == EOS_EResult::EOS_Success) {
		EOS_EpicAccountId UserId = ClientData->AccountId;
		EOS_ProductUserId LocalUserId = data->LocalUserId;

		if (auth == nullptr) {
			UtilityFunctions::printerr("AuthenticationSubsystem: connect_login_callback - instance is null");
			return;
		}
		else {
			UtilityFunctions::print("AuthenticationSubsystem: connect_login_callback - instance valid");

			// Convert Product User ID to string
			const char* product_id_string = FAccountHelpers::ProductUserIDToString(data->LocalUserId);
			if (product_id_string) {
				auth->product_user_id = String::utf8(product_id_string);
				UtilityFunctions::print("AuthenticationSubsystem: Product User ID retrieved: " + auth->product_user_id);
			} else {
				UtilityFunctions::printerr("AuthenticationSubsystem: Failed to convert Product User ID to string");
			}
			WARN_PRINT("Connect login successful - cross-platform features enabled");

			// Now both Auth and Connect logins are complete, emit the signal
			Dictionary user_info;
			user_info["display_name"] = auth->display_name;
			user_info["epic_account_id"] = auth->GetEpicAccountId();
			user_info["product_user_id"] = auth->GetProductUserId();

			UtilityFunctions::print("AuthenticationSubsystem: Preparing to emit login success signal");
			UtilityFunctions::print("AuthenticationSubsystem: User info - Display Name: " + auth->display_name + ", Epic Account ID: " + auth->GetEpicAccountId() + ", Product User ID: " + auth->GetProductUserId());

			if (auth->login_callback.is_valid()) {
				UtilityFunctions::print("AuthenticationSubsystem: Calling login success callback");
				auth->login_callback.call(true, user_info);
				UtilityFunctions::print("AuthenticationSubsystem: Login success callback completed");
			} else {
				UtilityFunctions::printerr("AuthenticationSubsystem: Login callback is not valid - cannot emit success signal");
			}
		}

		
	} else {
		if (auth == nullptr) {
			UtilityFunctions::printerr("AuthenticationSubsystem: connect_login_callback - instance is null in failure case");
			return;
		}

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
		user_info["display_name"] = auth->display_name;
		user_info["epic_account_id"] = auth->GetEpicAccountId();
		user_info["product_user_id"] = "";  // Empty since Connect failed

		if (auth->login_callback.is_valid()) {
			auth->login_callback.call(true, user_info);
		}
	}
}


void EOS_CALL AuthenticationSubsystem::on_auth_logout_complete(const EOS_Auth_LogoutCallbackInfo* data) {
	AuthenticationSubsystem* instance = static_cast<AuthenticationSubsystem*>(data->ClientData);
	if (!data || !instance) {
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		// Auth logout successful
		UtilityFunctions::print("AuthenticationSubsystem: Auth logout successful");
	} else {
		String error_msg = "Auth logout failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
		UtilityFunctions::printerr(error_msg);
	}
}

void EOS_CALL AuthenticationSubsystem::on_connect_logout_complete(const EOS_Connect_LogoutCallbackInfo* data) {
	AuthenticationSubsystem* instance = static_cast<AuthenticationSubsystem*>(data->ClientData);
	if (!data || !instance) {
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		// Connect logout successful
		UtilityFunctions::print("AuthenticationSubsystem: Connect logout successful");
	} else {
		String error_msg = "Connect logout failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
		UtilityFunctions::printerr(error_msg);
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