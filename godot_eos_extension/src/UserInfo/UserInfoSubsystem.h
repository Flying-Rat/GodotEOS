#pragma once

#include "IUserInfoSubsystem.h"
#include <eos_sdk.h>
#include <eos_userinfo.h>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <vector>
#include <set>
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
    virtual void SetUserInfoQueryCallback(const Callable& callback) override;

    // Phase 1: User cache interface
    virtual void UpdateUserCache(EOS_EpicAccountId epic_id, EOS_ProductUserId product_id = nullptr, bool is_local_user = false) override;
    virtual String GetUserProductId(EOS_EpicAccountId epic_id) override;
    virtual bool IsProductIdCached(EOS_EpicAccountId epic_id) override;
    virtual Dictionary GetCachedUserData(EOS_EpicAccountId epic_id) override;

private:
    // EOS handles
    EOS_HUserInfo userinfo_handle;

    // Phase 1: User cache entry structure
    struct UserCacheEntry {
        EOS_EpicAccountId epic_account_id;
        EOS_ProductUserId product_user_id;  // May be nullptr if not yet queried
        String display_name;
        String nickname;
        String country;
        String preferred_language;
        bool is_local_user;  // True if this is the authenticated user

        UserCacheEntry()
            : epic_account_id(nullptr)
            , product_user_id(nullptr)
            , is_local_user(false) {}
    };

    // Phase 1: User cache - vector of user entries
    std::vector<UserCacheEntry> user_cache;

    // Phase 1: Track ongoing product ID queries to avoid duplicates
    std::set<String> pending_product_id_queries;

    // Context for async queries
    struct QueryUserInfoContext {
        UserInfoSubsystem* subsystem;
        EOS_EpicAccountId local_user_id;
        EOS_EpicAccountId target_user_id;
    };

    // Callback callable
    Callable user_info_query_callback;

    // EOS callbacks
    static void EOS_CALL on_query_user_info_complete(const EOS_UserInfo_QueryUserInfoCallbackInfo* data);

    // Helper methods
    Dictionary copy_user_info_to_dictionary(EOS_EpicAccountId local_user_id, EOS_EpicAccountId target_user_id);
    
    // Phase 1: Cache helper methods
    void query_product_id_for_user(EOS_EpicAccountId epic_id);
    UserCacheEntry* find_cache_entry(EOS_EpicAccountId epic_id);
    const UserCacheEntry* find_cache_entry(EOS_EpicAccountId epic_id) const;
};

} // namespace godot
