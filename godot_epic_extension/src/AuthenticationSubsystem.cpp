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

    EOS_Auth_Login(auth_handle, &options, nullptr, on_auth_login_complete);
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

    EOS_Auth_Login(auth_handle, &options, nullptr, on_auth_login_complete);
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

    EOS_Auth_Login(auth_handle, &options, nullptr, on_auth_login_complete);
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

    EOS_Auth_Login(auth_handle, &options, nullptr, on_auth_login_complete);
    return true;
}

// Static callback implementations
void EOS_CALL AuthenticationSubsystem::on_auth_login_complete(const EOS_Auth_LoginCallbackInfo* data) {
    if (!data) return;

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        UtilityFunctions::print("AuthenticationSubsystem: Auth login successful");

        // Note: Epic Account ID will be retrieved when needed
        // The Product User ID from Connect login is what we primarily use
    } else {
        UtilityFunctions::printerr("AuthenticationSubsystem: Auth login failed: " + String::num_int64((int64_t)data->ResultCode));
    }
}

void EOS_CALL AuthenticationSubsystem::on_connect_login_complete(const EOS_Connect_LoginCallbackInfo* data) {
    if (!data) return;

    AuthenticationSubsystem* subsystem = static_cast<AuthenticationSubsystem*>(data->ClientData);
    if (!subsystem) return;

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        UtilityFunctions::print("AuthenticationSubsystem: Connect login successful");

        // Store Product User ID
        char puid_string[EOS_PRODUCTUSERID_MAX_LENGTH];
        int32_t puid_length = EOS_PRODUCTUSERID_MAX_LENGTH;
        EOS_EResult id_result = EOS_ProductUserId_ToString(data->LocalUserId, puid_string, &puid_length);

        if (id_result == EOS_EResult::EOS_Success) {
            subsystem->product_user_id = String(puid_string);
            subsystem->is_logged_in = true;
            subsystem->display_name = "Device User"; // Device ID login doesn't provide a display name
            subsystem->login_status = EOS_ELoginStatus::EOS_LS_LoggedIn;
            UtilityFunctions::print("AuthenticationSubsystem: Product User ID: " + subsystem->product_user_id);
            UtilityFunctions::print("AuthenticationSubsystem: Login completed successfully");
        } else {
            UtilityFunctions::printerr("AuthenticationSubsystem: Failed to convert Product User ID to string");
        }
    } else {
        UtilityFunctions::printerr("AuthenticationSubsystem: Connect login failed: " + String::num_int64((int64_t)data->ResultCode));
        subsystem->is_logged_in = false;
        subsystem->product_user_id = "";
        subsystem->display_name = "";
        subsystem->login_status = EOS_ELoginStatus::EOS_LS_NotLoggedIn;
    }
}

void EOS_CALL AuthenticationSubsystem::on_auth_logout_complete(const EOS_Auth_LogoutCallbackInfo* data) {
    if (!data) return;

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        UtilityFunctions::print("AuthenticationSubsystem: Auth logout successful");
    } else {
        UtilityFunctions::printerr("AuthenticationSubsystem: Auth logout failed: " + String::num_int64((int64_t)data->ResultCode));
    }
}

void EOS_CALL AuthenticationSubsystem::on_connect_logout_complete(const EOS_Connect_LogoutCallbackInfo* data) {
    if (!data) return;

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        UtilityFunctions::print("AuthenticationSubsystem: Connect logout successful");
    } else {
        UtilityFunctions::printerr("AuthenticationSubsystem: Connect logout failed: " + String::num_int64((int64_t)data->ResultCode));
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