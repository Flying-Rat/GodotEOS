#include "FriendsSubsystem.h"
#include "../Utils/SubsystemManager.h"
#include "../Platform/IPlatformSubsystem.h"
#include "../Authentication/IAuthenticationSubsystem.h"
#include "../UserInfo/IUserInfoSubsystem.h"
#include "../Utils/AccountHelpers.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include "../eos_sdk/Include/eos_friends.h"

using namespace godot;

FriendsSubsystem::FriendsSubsystem()
    : friends_cached(false) {
}

FriendsSubsystem::~FriendsSubsystem() {
    Shutdown();
}

bool FriendsSubsystem::Init() {
    UtilityFunctions::printerr("FriendsSubsystem: Initializing");
    friends_list.clear();
    friends_cached = false;
    return true;
}

void FriendsSubsystem::Tick(float delta_time) {
    // Friends subsystem doesn't need regular ticking
}

void FriendsSubsystem::Shutdown() {
    UtilityFunctions::printerr("FriendsSubsystem: Shutting down");
    friends_list.clear();
    friends_cached = false;
    friends_query_callback = Callable();
    friend_info_query_callback = Callable();
}

bool FriendsSubsystem::QueryFriends() {
    UtilityFunctions::print("FriendsSubsystem: Starting friends query");

    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        UtilityFunctions::push_warning("FriendsSubsystem: AuthenticationSubsystem not available or user not logged in");
        return false;
    }

    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->GetPlatformHandle()) {
        UtilityFunctions::printerr("FriendsSubsystem: Platform not initialized");
        return false;
    }

    EOS_HFriends friends_handle = EOS_Platform_GetFriendsInterface(platform->GetPlatformHandle());
    if (!friends_handle) {
        UtilityFunctions::printerr("FriendsSubsystem: Failed to get Friends interface");
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
        UtilityFunctions::push_warning("FriendsSubsystem: Friends list not cached, call QueryFriends() first");
        return Array();
    }

    return friends_list;
}

Dictionary FriendsSubsystem::GetFriendInfo(const String& friend_id) const {
    Dictionary friend_info;

    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        UtilityFunctions::push_warning("FriendsSubsystem: AuthenticationSubsystem not available or user not logged in");
        return friend_info;
    }

    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->GetPlatformHandle()) {
        UtilityFunctions::printerr("FriendsSubsystem: Platform not initialized");
        return friend_info;
    }

    // Convert string friend_id to EOS_EpicAccountId
    EOS_EpicAccountId target_user_id = FAccountHelpers::EpicAccountIDFromString(friend_id.utf8().get_data());
    if (!target_user_id) {
        UtilityFunctions::printerr("FriendsSubsystem: Invalid friend ID format");
        return friend_info;
    }

    // Use UserInfoSubsystem to get cached user info
    auto userinfo = Get<IUserInfoSubsystem>();
    if (userinfo) {
        // Get display name using convenience method
        String display_name = userinfo->GetUserDisplayName(auth->GetEpicAccountId(), target_user_id);
        
        if (!display_name.is_empty()) {
            friend_info["id"] = friend_id;
            friend_info["display_name"] = display_name;
            
            // Get additional user info if needed
            Dictionary user_info = userinfo->GetCachedUserInfo(auth->GetEpicAccountId(), target_user_id);
            if (user_info.has("country")) {
                friend_info["country"] = user_info["country"];
            }
            if (user_info.has("preferred_language")) {
                friend_info["preferred_language"] = user_info["preferred_language"];
            }
            if (user_info.has("nickname")) {
                friend_info["nickname"] = user_info["nickname"];
            }
        } else {
            // User info not cached
            friend_info["id"] = friend_id;
            friend_info["display_name"] = "";
            friend_info["status"] = "Call QueryFriendInfo() first";
        }
    } else {
        UtilityFunctions::printerr("FriendsSubsystem: UserInfoSubsystem not available");
    }

    return friend_info;
}

bool FriendsSubsystem::QueryFriendInfo(const String& friend_id) {
    UtilityFunctions::print("FriendsSubsystem: Starting friend info query for: " + friend_id);

    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        UtilityFunctions::push_warning("FriendsSubsystem: AuthenticationSubsystem not available or user not logged in");
        return false;
    }

    auto userinfo = Get<IUserInfoSubsystem>();
    if (!userinfo) {
        UtilityFunctions::printerr("FriendsSubsystem: UserInfoSubsystem not available");
        return false;
    }

    // Convert string friend_id to EOS_EpicAccountId
    EOS_EpicAccountId target_user_id = FAccountHelpers::EpicAccountIDFromString(friend_id.utf8().get_data());
    if (!target_user_id) {
        UtilityFunctions::push_warning("FriendsSubsystem: Invalid friend ID format");
        return false;
    }

    // Use UserInfoSubsystem to query user info
    if (!userinfo->QueryUserInfo(auth->GetEpicAccountId(), target_user_id)) {
        UtilityFunctions::printerr("FriendsSubsystem: Failed to initiate user info query");
        return false;
    }
    return true;
}

bool FriendsSubsystem::QueryAllFriendsInfo() {
    UtilityFunctions::print("FriendsSubsystem: Starting query for all friends info");

    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        UtilityFunctions::push_warning("FriendsSubsystem: AuthenticationSubsystem not available or user not logged in");
        return false;
    }

    auto userinfo = Get<IUserInfoSubsystem>();
    if (!userinfo) {
        UtilityFunctions::printerr("FriendsSubsystem: UserInfoSubsystem not available");
        return false;
    }

    // Get current friends list
    Array current_friends_list = GetFriendsList();
    EOS_EpicAccountId local_user_id = auth->GetEpicAccountId();

    // Query user info for each friend using UserInfoSubsystem
    int query_count = 0;
    for (int i = 0; i < current_friends_list.size(); i++) {
        Dictionary friend_info = current_friends_list[i];
        String friend_id = friend_info["id"];

        EOS_EpicAccountId target_user_id = FAccountHelpers::EpicAccountIDFromString(friend_id.utf8().get_data());
        if (target_user_id) {
            // Query for caching
            if (userinfo->QueryUserInfo(local_user_id, target_user_id)) {
                query_count++;
            }
        }
    }

    UtilityFunctions::print("FriendsSubsystem: Querying user info for " + String::num_int64(query_count) + " friends");
    return query_count > 0;
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
        UtilityFunctions::push_warning("FriendsSubsystem: AuthenticationSubsystem not available or user not logged in");
        return;
    }

    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->GetPlatformHandle()) {
        UtilityFunctions::push_warning("FriendsSubsystem: Platform not initialized");
        return;
    }

    EOS_HFriends friends_handle = EOS_Platform_GetFriendsInterface(platform->GetPlatformHandle());
    if (!friends_handle) {
        UtilityFunctions::printerr("FriendsSubsystem: Failed to get Friends interface");
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
    String friend_id_str = FAccountHelpers::EpicAccountIDToString(friend_id);
    friend_info["id"] = friend_id_str;

    // Use UserInfoSubsystem to get display name
    auto userinfo = Get<IUserInfoSubsystem>();
    if (userinfo) {
        String display_name = userinfo->GetUserDisplayName(auth->GetEpicAccountId(), friend_id);
        if (!display_name.is_empty()) {
            friend_info["display_name"] = display_name;
        }
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
        UtilityFunctions::print("FriendsSubsystem: Friends query successful - updating friends list");
        subsystem->update_friends_list();

        // Emit callback if set
        if (subsystem->friends_query_callback.is_valid()) {
            subsystem->friends_query_callback.call(true, subsystem->friends_list);
        }
    } else {
        String error_msg = "FriendsSubsystem: Friends query failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
        UtilityFunctions::printerr(error_msg);

        // Emit callback with failure
        if (subsystem->friends_query_callback.is_valid()) {
            subsystem->friends_query_callback.call(false, Array());
        }
    }
}
