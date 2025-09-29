#include "FriendsSubsystem.h"
#include "PlatformSubsystem.h"
#include "AuthenticationSubsystem.h"
#include "AccountHelpers.h"
#include "SubsystemManager.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <eos_sdk.h>
#include <eos_friends.h>
#include <eos_userinfo.h>

using namespace godot;

FriendsSubsystem::FriendsSubsystem()
    : friends_cached(false) {
}

FriendsSubsystem::~FriendsSubsystem() {
    Shutdown();
}

bool FriendsSubsystem::Init() {
    ERR_PRINT("FriendsSubsystem: Initializing");
    friends_list.clear();
    friends_cached = false;
    return true;
}

void FriendsSubsystem::Tick(float delta_time) {
    // Friends subsystem doesn't need regular ticking
}

void FriendsSubsystem::Shutdown() {
    ERR_PRINT("FriendsSubsystem: Shutting down");
    friends_list.clear();
    friends_cached = false;
    friends_query_callback = Callable();
    friend_info_query_callback = Callable();
}

bool FriendsSubsystem::QueryFriends() {
    ERR_PRINT("FriendsSubsystem: Starting friends query");

    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        ERR_PRINT("FriendsSubsystem: AuthenticationSubsystem not available or user not logged in");
        return false;
    }

    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->GetPlatformHandle()) {
        ERR_PRINT("FriendsSubsystem: Platform not initialized");
        return false;
    }

    EOS_HFriends friends_handle = EOS_Platform_GetFriendsInterface(platform->GetPlatformHandle());
    if (!friends_handle) {
        ERR_PRINT("FriendsSubsystem: Failed to get Friends interface");
        return false;
    }

    EOS_Friends_QueryFriendsOptions query_options = {};
    query_options.ApiVersion = EOS_FRIENDS_QUERYFRIENDS_API_LATEST;
    query_options.LocalUserId = auth->GetEpicAccountId();

    EOS_Friends_QueryFriends(friends_handle, &query_options, this, on_friends_query_complete);
    return true;
}

Array FriendsSubsystem::GetFriendsList() const {
    if (!friends_cached) {
        ERR_PRINT("FriendsSubsystem: Friends list not cached, call QueryFriends() first");
        return Array();
    }

    return friends_list;
}

Dictionary FriendsSubsystem::GetFriendInfo(const String& friend_id) const {
    Dictionary friend_info;

    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        ERR_PRINT("FriendsSubsystem: AuthenticationSubsystem not available or user not logged in");
        return friend_info;
    }

    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->GetPlatformHandle()) {
        ERR_PRINT("FriendsSubsystem: Platform not initialized");
        return friend_info;
    }

    // Convert string friend_id to EOS_EpicAccountId
    EOS_EpicAccountId target_user_id = FAccountHelpers::EpicAccountIDFromString(friend_id.utf8().get_data());
    if (!target_user_id) {
        ERR_PRINT("FriendsSubsystem: Invalid friend ID format");
        return friend_info;
    }

    // Try to get cached user info
    EOS_HUserInfo user_info_handle = EOS_Platform_GetUserInfoInterface(platform->GetPlatformHandle());
    if (user_info_handle) {
        EOS_UserInfo_CopyUserInfoOptions copy_options = {};
        copy_options.ApiVersion = EOS_USERINFO_COPYUSERINFO_API_LATEST;
        copy_options.LocalUserId = auth->GetEpicAccountId();
        copy_options.TargetUserId = target_user_id;

        EOS_UserInfo* user_info = nullptr;
        EOS_EResult copy_result = EOS_UserInfo_CopyUserInfo(user_info_handle, &copy_options, &user_info);

        if (copy_result == EOS_EResult::EOS_Success && user_info) {
            friend_info["id"] = friend_id;
            if (user_info->DisplayName) {
                friend_info["display_name"] = String::utf8(user_info->DisplayName);
            } else {
                friend_info["display_name"] = "Unknown User";
            }
            if (user_info->Country) {
                friend_info["country"] = String::utf8(user_info->Country);
            }
            if (user_info->PreferredLanguage) {
                friend_info["preferred_language"] = String::utf8(user_info->PreferredLanguage);
            }
            if (user_info->Nickname) {
                friend_info["nickname"] = String::utf8(user_info->Nickname);
            }
            EOS_UserInfo_Release(user_info);
        } else {
            // User info not cached
            friend_info["id"] = friend_id;
            friend_info["display_name"] = "Not loaded";
            friend_info["status"] = "Call QueryFriendInfo() first";
        }
    } else {
        ERR_PRINT("FriendsSubsystem: Failed to get UserInfo interface");
    }

    return friend_info;
}

bool FriendsSubsystem::QueryFriendInfo(const String& friend_id) {
    ERR_PRINT("FriendsSubsystem: Starting friend info query for: " + friend_id);

    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        ERR_PRINT("FriendsSubsystem: AuthenticationSubsystem not available or user not logged in");
        return false;
    }

    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->GetPlatformHandle()) {
        ERR_PRINT("FriendsSubsystem: Platform not initialized");
        return false;
    }

    // Convert string friend_id to EOS_EpicAccountId
    EOS_EpicAccountId target_user_id = FAccountHelpers::EpicAccountIDFromString(friend_id.utf8().get_data());
    if (!target_user_id) {
        ERR_PRINT("FriendsSubsystem: Invalid friend ID format");
        return false;
    }

    EOS_HUserInfo user_info_handle = EOS_Platform_GetUserInfoInterface(platform->GetPlatformHandle());
    if (!user_info_handle) {
        ERR_PRINT("FriendsSubsystem: Failed to get UserInfo interface");
        return false;
    }

    EOS_UserInfo_QueryUserInfoOptions query_options = {};
    query_options.ApiVersion = EOS_USERINFO_QUERYUSERINFO_API_LATEST;
    query_options.LocalUserId = auth->GetEpicAccountId();
    query_options.TargetUserId = target_user_id;

    EOS_UserInfo_QueryUserInfo(user_info_handle, &query_options, this, on_friend_info_query_complete);
    return true;
}

bool FriendsSubsystem::QueryAllFriendsInfo() {
    ERR_PRINT("FriendsSubsystem: Starting query for all friends info");

    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        ERR_PRINT("FriendsSubsystem: AuthenticationSubsystem not available or user not logged in");
        return false;
    }

    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->GetPlatformHandle()) {
        ERR_PRINT("FriendsSubsystem: Platform not initialized");
        return false;
    }

    EOS_HUserInfo user_info_handle = EOS_Platform_GetUserInfoInterface(platform->GetPlatformHandle());
    if (!user_info_handle) {
        ERR_PRINT("FriendsSubsystem: Failed to get UserInfo interface");
        return false;
    }

    // Get current friends list
    Array current_friends_list = GetFriendsList();
    EOS_EpicAccountId local_user_id = auth->GetEpicAccountId();

    // Query user info for each friend
    for (int i = 0; i < current_friends_list.size(); i++) {
        Dictionary friend_info = current_friends_list[i];
        String friend_id = friend_info["id"];

        EOS_EpicAccountId target_user_id = FAccountHelpers::EpicAccountIDFromString(friend_id.utf8().get_data());
        if (target_user_id) {
            EOS_UserInfo_QueryUserInfoOptions query_options = {};
            query_options.ApiVersion = EOS_USERINFO_QUERYUSERINFO_API_LATEST;
            query_options.LocalUserId = local_user_id;
            query_options.TargetUserId = target_user_id;

            EOS_UserInfo_QueryUserInfo(user_info_handle, &query_options, this, on_friend_info_query_complete);
        }
    }

    ERR_PRINT("FriendsSubsystem: Querying user info for " + String::num_int64(current_friends_list.size()) + " friends");
    return true;
}

void FriendsSubsystem::SetFriendsQueryCallback(const Callable& callback) {
    friends_query_callback = callback;
}

void FriendsSubsystem::SetFriendInfoQueryCallback(const Callable& callback) {
    friend_info_query_callback = callback;
}

void FriendsSubsystem::update_friends_list() {
    friends_list.clear();

    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        ERR_PRINT("FriendsSubsystem: AuthenticationSubsystem not available or user not logged in");
        return;
    }

    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->GetPlatformHandle()) {
        ERR_PRINT("FriendsSubsystem: Platform not initialized");
        return;
    }

    EOS_HFriends friends_handle = EOS_Platform_GetFriendsInterface(platform->GetPlatformHandle());
    if (!friends_handle) {
        ERR_PRINT("FriendsSubsystem: Failed to get Friends interface");
        return;
    }

    EOS_Friends_GetFriendsCountOptions count_options = {};
    count_options.ApiVersion = EOS_FRIENDS_GETFRIENDSCOUNT_API_LATEST;
    count_options.LocalUserId = auth->GetEpicAccountId();

    int32_t friends_count = EOS_Friends_GetFriendsCount(friends_handle, &count_options);

    for (int32_t i = 0; i < friends_count; i++) {
        EOS_Friends_GetFriendAtIndexOptions friend_options = {};
        friend_options.ApiVersion = EOS_FRIENDS_GETFRIENDATINDEX_API_LATEST;
        friend_options.LocalUserId = auth->GetEpicAccountId();
        friend_options.Index = i;

        EOS_EpicAccountId friend_id = EOS_Friends_GetFriendAtIndex(friends_handle, &friend_options);
        if (friend_id) {
            Dictionary friend_info = create_friend_info_dict(friend_id);
            friends_list.append(friend_info);
        }
    }

    friends_cached = true;
}

Dictionary FriendsSubsystem::create_friend_info_dict(EOS_EpicAccountId friend_id) const {
    Dictionary friend_info;

    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth) return friend_info;

    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->GetPlatformHandle()) return friend_info;

    // Convert friend ID to string
    const char* friend_id_cstr = FAccountHelpers::EpicAccountIDToString(friend_id);
    friend_info["id"] = String::utf8(friend_id_cstr);

    // Try to get cached user info for display name
    EOS_HUserInfo user_info_handle = EOS_Platform_GetUserInfoInterface(platform->GetPlatformHandle());
    if (user_info_handle) {
        EOS_UserInfo_CopyUserInfoOptions copy_options = {};
        copy_options.ApiVersion = EOS_USERINFO_COPYUSERINFO_API_LATEST;
        copy_options.LocalUserId = auth->GetEpicAccountId();
        copy_options.TargetUserId = friend_id;
        EOS_UserInfo* user_info = nullptr;
        EOS_EResult copy_result = EOS_UserInfo_CopyUserInfo(user_info_handle, &copy_options, &user_info);

        if (copy_result == EOS_EResult::EOS_Success && user_info) {
            if (user_info->DisplayName) {
                friend_info["display_name"] = String::utf8(user_info->DisplayName);
            } else {
                friend_info["display_name"] = "Unknown User";
            }
            EOS_UserInfo_Release(user_info);
        } else {
            // User info not cached, set placeholder
            friend_info["display_name"] = "Loading...";
        }
    } else {
        friend_info["display_name"] = "Unknown User";
    }

    // Get friend status
    EOS_HFriends friends_handle = EOS_Platform_GetFriendsInterface(platform->GetPlatformHandle());
    if (friends_handle) {
        EOS_Friends_GetStatusOptions status_options = {};
        status_options.ApiVersion = EOS_FRIENDS_GETSTATUS_API_LATEST;
        status_options.LocalUserId = auth->GetEpicAccountId();
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
    }

    return friend_info;
}

// Static callback implementations
void EOS_CALL FriendsSubsystem::on_friends_query_complete(const EOS_Friends_QueryFriendsCallbackInfo* data) {
    if (!data || !data->ClientData) {
        return;
    }

    FriendsSubsystem* subsystem = static_cast<FriendsSubsystem*>(data->ClientData);

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        ERR_PRINT("FriendsSubsystem: Friends query successful - updating friends list");
        subsystem->update_friends_list();

        // Emit callback if set
        if (subsystem->friends_query_callback.is_valid()) {
            subsystem->friends_query_callback.call(true, subsystem->friends_list);
        }
    } else {
        String error_msg = "FriendsSubsystem: Friends query failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
        ERR_PRINT(error_msg);

        // Emit callback with failure
        if (subsystem->friends_query_callback.is_valid()) {
            subsystem->friends_query_callback.call(false, Array());
        }
    }
}

void EOS_CALL FriendsSubsystem::on_friend_info_query_complete(const EOS_UserInfo_QueryUserInfoCallbackInfo* data) {
    if (!data || !data->ClientData) {
        return;
    }

    FriendsSubsystem* subsystem = static_cast<FriendsSubsystem*>(data->ClientData);

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        ERR_PRINT("FriendsSubsystem: Friend info query successful");

        // Convert user ID back to string and get friend info
        const char* user_id_str = FAccountHelpers::EpicAccountIDToString(data->TargetUserId);
        String friend_id = String::utf8(user_id_str);

        Dictionary friend_info = subsystem->GetFriendInfo(friend_id);

        // Emit callback if set
        if (subsystem->friend_info_query_callback.is_valid()) {
            subsystem->friend_info_query_callback.call(true, friend_info);
        }
    } else {
        String error_msg = "FriendsSubsystem: Friend info query failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
        ERR_PRINT(error_msg);

        // Emit callback with failure
        if (subsystem->friend_info_query_callback.is_valid()) {
            Dictionary empty_info;
            empty_info["id"] = FAccountHelpers::EpicAccountIDToString(data->TargetUserId);
            empty_info["error"] = "Query failed";
            subsystem->friend_info_query_callback.call(false, empty_info);
        }
    }
}