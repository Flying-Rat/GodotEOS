#pragma once

#include "IUserInfoSubsystem.h"
#include <eos_sdk.h>
#include <eos_userinfo.h>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <map>
#include <memory>

namespace godot {

/**
 * @brief User Info subsystem implementation.
 *
 * THREAD SAFETY:
 * - All public methods must be called from the Godot main thread
 * - EOS callbacks are delivered on the thread that calls Tick()
 */
class UserInfoSubsystem : public IUserInfoSubsystem {
public:
    UserInfoSubsystem();
    virtual ~UserInfoSubsystem();

    // ISubsystem interface
    virtual bool Init() override;
    virtual void Tick(float delta_time) override;
    virtual void Shutdown() override;
    virtual const char* GetSubsystemName() const override { return "UserInfoSubsystem"; }

    // IUserInfoSubsystem interface
    virtual bool QueryUserInfo(EOS_EpicAccountId local_user_id, EOS_EpicAccountId target_user_id) override;
    virtual Dictionary GetCachedUserInfo(EOS_EpicAccountId local_user_id, EOS_EpicAccountId target_user_id) override;
    virtual String GetUserDisplayName(EOS_EpicAccountId local_user_id, EOS_EpicAccountId target_user_id) override;
    virtual bool IsUserInfoCached(EOS_EpicAccountId local_user_id, EOS_EpicAccountId target_user_id) override;
    virtual void ClearCache() override;

private:
    // EOS handles
    EOS_HUserInfo userinfo_handle;

    // Context for async queries
    struct QueryUserInfoContext {
        UserInfoSubsystem* subsystem;
        EOS_EpicAccountId local_user_id;
        EOS_EpicAccountId target_user_id;
    };

    // EOS callbacks
    static void EOS_CALL on_query_user_info_complete(const EOS_UserInfo_QueryUserInfoCallbackInfo* data);

    // Helper methods
    Dictionary copy_user_info_to_dictionary(EOS_EpicAccountId local_user_id, EOS_EpicAccountId target_user_id);
};

} // namespace godot
