#include "FriendsSubsystem.h"
#include "../Utils/SubsystemManager.h"
#include "../Platform/IPlatformSubsystem.h"
#include "../Authentication/IAuthenticationSubsystem.h"
#include "../UserInfo/IUserInfoSubsystem.h"
#include "../Utils/AccountHelpers.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <eos_friends.h>
#include <eos_connect.h>
#include "../Utils/Logger.h"

using namespace godot;

struct QueryExternalAccountMappingsContext {
    FriendsSubsystem* subsystem;
    // Owning UTF-8 storage. godot::String is UTF-32 internally, so a pointer
    // cannot be taken into it, and String::utf8() returns a temporary that dies
    // at the end of the full expression. The buffers must be owned here.
    std::vector<godot::CharString> OutstandingExternalAccountsToQueryUtf8;
    std::vector<const char*> OutstandingExternalAccountsToQueryChars;
    std::vector<EOS_EpicAccountId> OutstandingExternalAccountsToQueryEpicIDs;

    // Must be called once the context is in its final location - pointers are
    // only valid when built from this object's own storage, after any move.
    void RebuildCharPointers() {
        OutstandingExternalAccountsToQueryChars.clear();
        OutstandingExternalAccountsToQueryChars.reserve(OutstandingExternalAccountsToQueryUtf8.size());
        for (const godot::CharString& utf8 : OutstandingExternalAccountsToQueryUtf8) {
            OutstandingExternalAccountsToQueryChars.push_back(utf8.get_data());
        }
    }
};

FriendsSubsystem::FriendsSubsystem()
    : friends_cached(false) {
}

FriendsSubsystem::~FriendsSubsystem() {
    Shutdown();
}

bool FriendsSubsystem::Init() {
    Logger::Info("Friends", "Initializing");
    friends_list.clear();
    friends_cached = false;
    return true;
}

void FriendsSubsystem::Tick(float delta_time) {
    // Friends subsystem doesn't need regular ticking
}

void FriendsSubsystem::Shutdown() {
    Logger::Info("Friends", "Shutting down");
    friends_list.clear();
    friends_cached = false;
    friends_query_callback = Callable();
    friend_info_query_callback = Callable();
}

bool FriendsSubsystem::QueryFriends() {
    Logger::Info("Friends", "Starting friends query");

    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        Logger::Warning("Friends", "AuthenticationSubsystem not available or user not logged in");
        return false;
    }

    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->GetPlatformHandle()) {
        Logger::Error("Friends", "Platform not initialized");
        return false;
    }

    EOS_HFriends friends_handle = EOS_Platform_GetFriendsInterface(platform->GetPlatformHandle());
    if (!friends_handle) {
        Logger::Error("Friends", "Failed to get Friends interface");
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
        Logger::Warning("Friends", "Friends list not cached, call QueryFriends() first");
        return Array();
    }

    return friends_list;
}

Dictionary FriendsSubsystem::GetFriendInfo(const String& friend_id) const {
    Dictionary friend_info;

    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        Logger::Warning("Friends", "AuthenticationSubsystem not available or user not logged in");
        return friend_info;
    }

    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->GetPlatformHandle()) {
        Logger::Error("Friends", "Platform not initialized");
        return friend_info;
    }

    // Convert string friend_id to EOS_EpicAccountId
    EOS_EpicAccountId target_user_id = FAccountHelpers::EpicAccountIDFromString(friend_id.utf8().get_data());
    if (!target_user_id) {
        Logger::Error("Friends", "Invalid friend ID format");
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
        Logger::Error("Friends", "UserInfoSubsystem not available");
    }

    return friend_info;
}

String FriendsSubsystem::GetFriendProductId(const String& friend_id) const {
    Dictionary friend_info = GetFriendInfo(friend_id);
    if (friend_info.has("product_id")) {
        return friend_info["product_id"];
    }
    return "";
}

bool FriendsSubsystem::QueryFriendInfo(const String& friend_id) {
    Logger::Info("Friends", "Starting friend info query for: " + friend_id);

    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        Logger::Warning("Friends", "AuthenticationSubsystem not available or user not logged in");
        return false;
    }

    auto userinfo = Get<IUserInfoSubsystem>();
    if (!userinfo) {
        Logger::Error("Friends", "UserInfoSubsystem not available");
        return false;
    }

    // Convert string friend_id to EOS_EpicAccountId
    EOS_EpicAccountId target_user_id = FAccountHelpers::EpicAccountIDFromString(friend_id.utf8().get_data());
    if (!target_user_id) {
        Logger::Warning("Friends", "Invalid friend ID format");
        return false;
    }

    // Use UserInfoSubsystem to query user info
    if (!userinfo->QueryUserInfo(auth->GetEpicAccountId(), target_user_id)) {
        Logger::Error("Friends", "Failed to initiate user info query");
        return false;
    }
    return true;
}

bool FriendsSubsystem::QueryAllFriendsInfo() {
    Logger::Info("Friends", "Starting query for all friends info");

    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        Logger::Warning("Friends", "AuthenticationSubsystem not available or user not logged in");
        return false;
    }

    auto userinfo = Get<IUserInfoSubsystem>();
    if (!userinfo) {
        Logger::Error("Friends", "UserInfoSubsystem not available");
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

    Logger::Info("Friends", "Querying user info for " + String::num_int64(query_count) + " friends");
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
        Logger::Warning("Friends", "AuthenticationSubsystem not available or user not logged in");
        return;
    }

    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->GetPlatformHandle()) {
        Logger::Warning("Friends", "Platform not initialized");
        return;
    }

    EOS_HFriends friends_handle = EOS_Platform_GetFriendsInterface(platform->GetPlatformHandle());
    if (!friends_handle) {
        Logger::Error("Friends", "Failed to get Friends interface");
        return;
    }

    EOS_Friends_GetFriendsCountOptions count_options = {};
    count_options.ApiVersion = EOS_FRIENDS_GETFRIENDSCOUNT_API_LATEST;
    count_options.LocalUserId = auth->GetEpicAccountId();

    int32_t friends_count = EOS_Friends_GetFriendsCount(friends_handle, &count_options);

    std::vector<godot::CharString> OutstandingExternalAccountsToQueryUtf8;
    std::vector<EOS_EpicAccountId> OutstandingExternalAccountsToQueryEpicIDs;

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

        String friend_id_str = FAccountHelpers::EpicAccountIDToString(friend_id);
        OutstandingExternalAccountsToQueryUtf8.push_back(friend_id_str.utf8());
        OutstandingExternalAccountsToQueryEpicIDs.push_back(friend_id);
    }

    if (OutstandingExternalAccountsToQueryUtf8.empty()) {
        Logger::Info("Friends", "No friends to map to product user IDs");
        friends_cached = true;
        return;
    }

    // Get ProductUserId for each friend
    EOS_HConnect connect_handle = EOS_Platform_GetConnectInterface(platform->GetPlatformHandle());
    if (connect_handle) {
        auto context = new QueryExternalAccountMappingsContext{this,
            std::move(OutstandingExternalAccountsToQueryUtf8),
            {},
            std::move(OutstandingExternalAccountsToQueryEpicIDs)
        };
        // Build the pointer array from the context's own storage, after the move.
        context->RebuildCharPointers();

        EOS_Connect_QueryExternalAccountMappingsOptions mapping_options = {};
        mapping_options.ApiVersion = EOS_CONNECT_QUERYEXTERNALACCOUNTMAPPINGS_API_LATEST;
        mapping_options.LocalUserId = auth->GetProductUserId();
        mapping_options.AccountIdType = EOS_EExternalAccountType::EOS_EAT_EPIC;

        mapping_options.ExternalAccountIdCount = context->OutstandingExternalAccountsToQueryChars.size();
        mapping_options.ExternalAccountIds = context->OutstandingExternalAccountsToQueryChars.data();

        Logger::Info("Friends", "Querying external account mappings for " + String::num_int64(mapping_options.ExternalAccountIdCount) + " friends");

        EOS_Connect_QueryExternalAccountMappings(connect_handle, &mapping_options, context, on_query_external_account_mappings);
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

void EOS_CALL FriendsSubsystem::on_query_external_account_mappings(const EOS_Connect_QueryExternalAccountMappingsCallbackInfo* data)
{
    if (!data || !data->ClientData) {
        return;
    }

    QueryExternalAccountMappingsContext* context = static_cast<QueryExternalAccountMappingsContext*>(data->ClientData);
    FriendsSubsystem* subsystem = context->subsystem;


    if (data->ResultCode == EOS_EResult::EOS_Success) {
        Logger::Info("Friends", "External account mappings query successful");

        std::vector<FEpicAccountId> MappingsReceived;
        for (const FEpicAccountId& NextId : context->OutstandingExternalAccountsToQueryEpicIDs)
        {
            EOS_Connect_GetExternalAccountMappingsOptions Options = {};
            Options.ApiVersion = EOS_CONNECT_GETEXTERNALACCOUNTMAPPINGS_API_LATEST;
            Options.AccountIdType = EOS_EExternalAccountType::EOS_EAT_EPIC;
            Options.LocalUserId = Get<IAuthenticationSubsystem>()->GetProductUserId();
            String NextIdString = FAccountHelpers::EpicAccountIDToString(NextId);
            // Keep the UTF-8 buffer alive for the duration of the EOS call -
            // String::utf8() returns a temporary that dies at the semicolon.
            CharString NextIdUtf8 = NextIdString.utf8();
            Options.TargetExternalUserId = NextIdUtf8.get_data();

            EOS_HConnect ConnectHandle = EOS_Platform_GetConnectInterface(Get<IPlatformSubsystem>()->GetPlatformHandle());
            EOS_ProductUserId NewMapping = EOS_Connect_GetExternalAccountMapping(ConnectHandle, &Options);
            if (NewMapping)
            {
                // Update the friends_list with the product_id
                String friend_id_str = FAccountHelpers::EpicAccountIDToString(NextId);
                String product_id_str = FAccountHelpers::ProductUserIDToString(NewMapping);
                for (int i = 0; i < subsystem->friends_list.size(); i++) {
                    Dictionary friend_dict = subsystem->friends_list[i];
                    if (friend_dict["id"] == friend_id_str) {
                        friend_dict["product_id"] = product_id_str;
                        subsystem->friends_list[i] = friend_dict; // update the array
                        Logger::Info("Friends", "Updated friend " + friend_id_str + " with product_id " + product_id_str);
                        break;
                    }
                }
            }
        }    
        

    } else {
        String error_msg = "External account mappings query failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
        Logger::Error("Friends", error_msg);
    }

    delete context;
}


// Static callback implementations
void EOS_CALL FriendsSubsystem::on_friends_query_complete(const EOS_Friends_QueryFriendsCallbackInfo* data) {
    if (!data || !data->ClientData) {
        return;
    }

    FriendsSubsystem* subsystem = static_cast<FriendsSubsystem*>(data->ClientData);

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        Logger::Info("Friends", "Friends query successful - updating friends list");
        subsystem->update_friends_list();

        // Emit callback if set
        if (subsystem->friends_query_callback.is_valid()) {
            subsystem->friends_query_callback.call(true, subsystem->friends_list);
        }
    } else {
        String error_msg = "Friends query failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
        Logger::Error("Friends", error_msg);

        // Emit callback with failure
        if (subsystem->friends_query_callback.is_valid()) {
            subsystem->friends_query_callback.call(false, Array());
        }
    }
}
