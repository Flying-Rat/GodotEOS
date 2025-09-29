#pragma once

#include "IAuthenticationSubsystem.h"
#include "../eos_sdk/Include/eos_sdk.h"
#include "../eos_sdk/Include/eos_base.h"
#include "../eos_sdk/Include/eos_auth.h"
#include "../eos_sdk/Include/eos_connect.h"
#include "../eos_sdk/Include/eos_logging.h"
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot {

class AuthenticationSubsystem : public IAuthenticationSubsystem {
public:
    AuthenticationSubsystem();
    virtual ~AuthenticationSubsystem();

    // ISubsystem interface
    virtual bool Init() override;
    virtual void Tick(float delta_time) override;
    virtual void Shutdown() override;
    virtual const char* GetSubsystemName() const override { return "AuthenticationSubsystem"; }

    // IAuthenticationSubsystem interface
    virtual bool Login(const String& login_type, const Dictionary& credentials) override;
    virtual bool Logout() override;
    virtual bool IsLoggedIn() const override;
    virtual void SetProductUserId(EOS_ProductUserId product_user_id) override;
    virtual String GetDisplayName() const override;
    virtual int GetLoginStatus() const override;
    virtual void SetLoginCallback(const Callable& callback) override;
    virtual Callable GetLoginCallback() const override;
    
    // Additional getter for raw EOS handle
    virtual EOS_ProductUserId GetProductUserId() const override;
    virtual EOS_EpicAccountId GetEpicAccountId() const override;

private:
    // EOS handles
    EOS_HPlatform platform_handle;
    EOS_HAuth auth_handle;
    EOS_HConnect connect_handle;
    EOS_ProductUserId local_user_id;
    EOS_EpicAccountId epic_account_id;

    // State
    bool is_logged_in;
    String display_name;
    EOS_ELoginStatus login_status;

    // Callbacks
    Callable login_callback;

    // Notification IDs
    EOS_NotificationId auth_login_status_changed_id;
    EOS_NotificationId connect_login_status_changed_id;

    // Internal methods
    void setup_notifications();
    void cleanup_notifications();
    bool perform_epic_account_login(const Dictionary& credentials);
    bool perform_device_id_login();
    bool perform_exchange_code_login(const String& exchange_code);
    bool perform_persistent_auth_login();
    bool perform_account_portal_login();
    bool perform_developer_login(const Dictionary& credentials);

    // Static callback implementations
    static void EOS_CALL logging_callback(const EOS_LogMessage* message);
    static void EOS_CALL auth_login_callback(const EOS_Auth_LoginCallbackInfo* data);
    static void EOS_CALL auth_logout_callback(const EOS_Auth_LogoutCallbackInfo* data);
    static void EOS_CALL connect_login_callback(const EOS_Connect_LoginCallbackInfo* data);

    static void EOS_CALL on_auth_logout_complete(const EOS_Auth_LogoutCallbackInfo* data);
    static void EOS_CALL on_connect_logout_complete(const EOS_Connect_LogoutCallbackInfo* data);
    static void EOS_CALL on_auth_login_status_changed(const EOS_Auth_LoginStatusChangedCallbackInfo* data);
    static void EOS_CALL on_connect_login_status_changed(const EOS_Connect_LoginStatusChangedCallbackInfo* data);
};

} // namespace godot