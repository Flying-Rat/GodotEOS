#include "UserInfoSubsystem.h"
#include "../Utils/SubsystemManager.h"
#include "../Platform/IPlatformSubsystem.h"
#include "../Authentication/IAuthenticationSubsystem.h"
#include "../Utils/AccountHelpers.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/error_macros.hpp>

namespace godot {

UserInfoSubsystem::UserInfoSubsystem()
    : userinfo_handle(nullptr)
{
    
}

UserInfoSubsystem::~UserInfoSubsystem() {
    Shutdown();
}

bool UserInfoSubsystem::Init() {
    UtilityFunctions::print("UserInfoSubsystem: Initializing...");

    // Get and validate platform
    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->IsOnline()) {
        UtilityFunctions::printerr("UserInfoSubsystem: Platform not available or offline");
        return false;
    }

    EOS_HPlatform platform_handle = platform->GetPlatformHandle();
    if (!platform_handle) {
        UtilityFunctions::printerr("UserInfoSubsystem: Invalid platform handle");
        return false;
    }

    userinfo_handle = EOS_Platform_GetUserInfoInterface(platform_handle);
    if (!userinfo_handle) {
        UtilityFunctions::printerr("UserInfoSubsystem: Failed to get UserInfo interface");
        return false;
    }

    UtilityFunctions::print("UserInfoSubsystem: Initialized successfully");
    return true;
}

void UserInfoSubsystem::Tick(float delta_time) {
    // No periodic tasks needed
}

void UserInfoSubsystem::Shutdown() {
    if (!userinfo_handle) {
        return;
    }

    UtilityFunctions::print("UserInfoSubsystem: Shutting down...");
    
    ClearCache();
    userinfo_handle = nullptr;

    UtilityFunctions::print("UserInfoSubsystem: Shutdown complete");
}

bool UserInfoSubsystem::QueryUserInfo(EOS_EpicAccountId target_user_id) {
    if (!userinfo_handle) {
        UtilityFunctions::printerr("UserInfoSubsystem: Not initialized");
        return false;
    }

    EOS_EpicAccountId local_user_id = Get<IAuthenticationSubsystem>()->GetEpicAccountId();
    if (!EOS_EpicAccountId_IsValid(local_user_id)) {
        UtilityFunctions::printerr("UserInfoSubsystem: Invalid local user ID");
        return false;
    }

    if (!EOS_EpicAccountId_IsValid(target_user_id)) {
        UtilityFunctions::printerr("UserInfoSubsystem: Invalid target user ID");
        return false;
    }

    // Create context for the query
    auto context = std::make_unique<QueryUserInfoContext>();
    context->subsystem = this;
    context->local_user_id = local_user_id;
    context->target_user_id = target_user_id;

    // Setup query options
    EOS_UserInfo_QueryUserInfoOptions query_options = {};
    query_options.ApiVersion = EOS_USERINFO_QUERYUSERINFO_API_LATEST;
    query_options.LocalUserId = local_user_id;
    query_options.TargetUserId = target_user_id;

    // Initiate query
    EOS_UserInfo_QueryUserInfo(userinfo_handle, &query_options, context.release(), on_query_user_info_complete);

    return true;
}

Dictionary UserInfoSubsystem::GetCachedUserInfo(EOS_EpicAccountId target_user_id) {
    if (!userinfo_handle) {
        return Dictionary();
    }

    EOS_EpicAccountId local_user_id = Get<IAuthenticationSubsystem>()->GetEpicAccountId();
    if (!EOS_EpicAccountId_IsValid(local_user_id)) {
        return Dictionary();
    }

    if (!EOS_EpicAccountId_IsValid(target_user_id)) {
        return Dictionary();
    }

    return copy_user_info_to_dictionary(local_user_id, target_user_id);
}

String UserInfoSubsystem::GetUserDisplayName(EOS_EpicAccountId target_user_id) {
    Dictionary user_info = GetCachedUserInfo(target_user_id);
    
    if (user_info.is_empty()) {
        return "Unknown";
    }

    // Try display name first
    String display_name = user_info.get("display_name", "");
    if (!display_name.is_empty()) {
        return display_name;
    }

    // Fall back to nickname
    String nickname = user_info.get("nickname", "");
    if (!nickname.is_empty()) {
        return nickname;
    }

    return "Unknown";
}

bool UserInfoSubsystem::IsUserInfoCached(EOS_EpicAccountId target_user_id) {
    if (!userinfo_handle) {
        return false;
    }

    EOS_EpicAccountId local_user_id = Get<IAuthenticationSubsystem>()->GetEpicAccountId();
    if (!EOS_EpicAccountId_IsValid(local_user_id)) {
        return false;
    }

    if (!EOS_EpicAccountId_IsValid(target_user_id)) {
        return false;
    }

    // Try to copy user info - if it succeeds, it's cached
    EOS_UserInfo_CopyUserInfoOptions copy_options = {};
    copy_options.ApiVersion = EOS_USERINFO_COPYUSERINFO_API_LATEST;
    copy_options.LocalUserId = local_user_id;
    copy_options.TargetUserId = target_user_id;

    EOS_UserInfo* user_info = nullptr;
    EOS_EResult result = EOS_UserInfo_CopyUserInfo(userinfo_handle, &copy_options, &user_info);

    if (result == EOS_EResult::EOS_Success && user_info) {
        EOS_UserInfo_Release(user_info);
        return true;
    }

    return false;
}

void UserInfoSubsystem::ClearCache() {
    // Note: EOS SDK manages its own cache internally
    // We don't maintain a separate cache here
    UtilityFunctions::print("UserInfoSubsystem: Cache cleared (EOS manages cache internally)");
    
    // Phase 1: Clear our user cache
    user_cache.clear();
    pending_product_id_queries.clear();
}

void UserInfoSubsystem::SetUserInfoQueryCallback(const Callable& callback) {
    user_info_query_callback = callback;
}

void UserInfoSubsystem::SetUserCacheUpdateCallback(const Callable& callback) {
    user_cache_update_callback = callback;
}

// Phase 1: User cache implementation

void UserInfoSubsystem::UpdateUserCache(EOS_EpicAccountId epic_id, EOS_ProductUserId product_id, bool is_local_user) {
    if (!EOS_EpicAccountId_IsValid(epic_id)) {
        return;
    }

    String epic_id_str = FAccountHelpers::EpicAccountIDToString(epic_id);
    String product_id_str = FAccountHelpers::ProductUserIDToString(product_id);

    UtilityFunctions::print(vformat("UserInfoSubsystem: UpdateUserCache called - Epic ID: %s, Product ID: %s, Is Local User: %s", epic_id_str, product_id_str, is_local_user ? "true" : "false"));

    // Find existing entry or create new one
    UserCacheEntry* entry = find_cache_entry(epic_id);

    if (!entry) {
        // Create new entry
        UserCacheEntry new_entry;
        new_entry.epic_account_id = epic_id;
        new_entry.product_user_id = product_id;
        new_entry.is_local_user = is_local_user;
        user_cache.push_back(new_entry);
        entry = &user_cache.back();
    }

    // Update entry
    entry->is_local_user = is_local_user;
    entry->product_user_id = product_id;

    // Query user info if not already cached
    if (entry->display_name.is_empty()) {
        if (Get<IAuthenticationSubsystem>()->IsLoggedIn()) {
            QueryUserInfo(epic_id);
        }
    }

    // Query Product ID if needed and not already attempted
    // Skip querying for local user since Product ID comes from Connect login
    // Also skip if local user doesn't have a valid Product User ID (Connect login not completed)
    auto auth_check = Get<IAuthenticationSubsystem>();
    EOS_ProductUserId local_product_id = auth_check ? auth_check->GetProductUserId() : nullptr;
    if (!entry->product_user_id && !entry->product_id_queried && !is_local_user && EOS_ProductUserId_IsValid(local_product_id)) {
        UtilityFunctions::print(vformat("UserInfoSubsystem: UpdateUserCache - Triggering Product ID query for Epic ID %s", epic_id_str));
        query_product_id_for_user(epic_id);
    } else {
        String reason = "already has Product ID";
        if (!entry->product_user_id && entry->product_id_queried) reason = "already queried";
        if (is_local_user) reason = "is local user";
        if (!EOS_ProductUserId_IsValid(local_product_id)) reason = "Connect login not completed";
        UtilityFunctions::print(vformat("UserInfoSubsystem: UpdateUserCache - Skipping Product ID query for Epic ID %s (%s)", epic_id_str, reason));
    }
}

String UserInfoSubsystem::GetUserProductId(EOS_EpicAccountId epic_id) {
    if (!EOS_EpicAccountId_IsValid(epic_id)) {
        return "";
    }

    const UserCacheEntry* entry = find_cache_entry(epic_id);
    if (entry && entry->product_user_id && EOS_ProductUserId_IsValid(entry->product_user_id)) {
        return FAccountHelpers::ProductUserIDToString(entry->product_user_id);
    }

    return "";
}

bool UserInfoSubsystem::IsProductIdCached(EOS_EpicAccountId epic_id) {
    if (!EOS_EpicAccountId_IsValid(epic_id)) {
        return false;
    }

    const UserCacheEntry* entry = find_cache_entry(epic_id);
    return entry && entry->product_user_id != nullptr && EOS_ProductUserId_IsValid(entry->product_user_id);
}

Dictionary UserInfoSubsystem::GetCachedUserData(EOS_EpicAccountId epic_id) {
    Dictionary result;

    if (!EOS_EpicAccountId_IsValid(epic_id)) {
        return result;
    }

    const UserCacheEntry* entry = find_cache_entry(epic_id);
    if (entry) {
        result["epic_account_id"] = FAccountHelpers::EpicAccountIDToString(entry->epic_account_id);
        result["product_user_id"] = FAccountHelpers::ProductUserIDToString(entry->product_user_id);
        result["display_name"] = entry->display_name;
        result["nickname"] = entry->nickname;
        result["country"] = entry->country;
        result["preferred_language"] = entry->preferred_language;
        result["is_local_user"] = entry->is_local_user;
        result["product_id_queried"] = entry->product_id_queried;  // Debug info
    }

    return result;
}

bool UserInfoSubsystem::ForceQueryProductId(EOS_EpicAccountId epic_id) {
    if (!EOS_EpicAccountId_IsValid(epic_id)) {
        UtilityFunctions::printerr("UserInfoSubsystem: ForceQueryProductId - Invalid Epic Account ID");
        return false;
    }

    String epic_id_str = FAccountHelpers::EpicAccountIDToString(epic_id);
    UtilityFunctions::print(vformat("UserInfoSubsystem: ForceQueryProductId called for Epic ID: %s", epic_id_str));

    // Find the cache entry
    UserCacheEntry* entry = find_cache_entry(epic_id);
    if (entry) {
        // Reset the queried flag to allow re-querying
        entry->product_id_queried = false;
        UtilityFunctions::print(vformat("UserInfoSubsystem: ForceQueryProductId - Reset product_id_queried flag for Epic ID %s", epic_id_str));
    }

    // Trigger the query
    query_product_id_for_user(epic_id);
    return true;
}

void UserInfoSubsystem::RetryFriendProductIdQueries() {
    UtilityFunctions::print("UserInfoSubsystem: RetryFriendProductIdQueries called - retrying Product ID queries for cached friends");

    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        UtilityFunctions::print("UserInfoSubsystem: RetryFriendProductIdQueries - Authentication not available, skipping");
        return;
    }

    EOS_ProductUserId local_product_id = auth->GetProductUserId();
    if (!EOS_ProductUserId_IsValid(local_product_id)) {
        UtilityFunctions::print("UserInfoSubsystem: RetryFriendProductIdQueries - Connect login not completed, skipping");
        return;
    }

    int retry_count = 0;
    for (auto& entry : user_cache) {
        // Only retry for friends (not local user) that haven't been successfully queried yet
        if (!entry.is_local_user && !entry.product_user_id && entry.product_id_queried) {
            // Reset the flag and retry
            entry.product_id_queried = false;
            String epic_id_str = FAccountHelpers::EpicAccountIDToString(entry.epic_account_id);
            UtilityFunctions::print(vformat("UserInfoSubsystem: RetryFriendProductIdQueries - Retrying Product ID query for friend: %s", epic_id_str));
            query_product_id_for_user(entry.epic_account_id);
            retry_count++;
        }
    }

    UtilityFunctions::print(vformat("UserInfoSubsystem: RetryFriendProductIdQueries - Initiated %d retry queries", retry_count));
}

// Phase 1: Cache helper methods

UserInfoSubsystem::UserCacheEntry* UserInfoSubsystem::find_cache_entry(EOS_EpicAccountId epic_id) {
    if (!EOS_EpicAccountId_IsValid(epic_id)) {
        return nullptr;
    }

    String epic_id_str = FAccountHelpers::EpicAccountIDToString(epic_id);

    for (auto& entry : user_cache) 
    {
        if (FAccountHelpers::EpicAccountIDToString(entry.epic_account_id) == epic_id_str) 
        {
            return &entry;
        }
    }

    return nullptr;
}

const UserInfoSubsystem::UserCacheEntry* UserInfoSubsystem::find_cache_entry(EOS_EpicAccountId epic_id) const {
    if (!EOS_EpicAccountId_IsValid(epic_id)) {
        return nullptr;
    }

    String epic_id_str = FAccountHelpers::EpicAccountIDToString(epic_id);

    for (const auto& entry : user_cache) 
    {
        if (FAccountHelpers::EpicAccountIDToString(entry.epic_account_id) == epic_id_str) 
        {
            return &entry;
        }
    }

    return nullptr;
}

void UserInfoSubsystem::query_product_id_for_user(EOS_EpicAccountId epic_id) {
    String epic_id_str = EOS_EpicAccountId_IsValid(epic_id) ? FAccountHelpers::EpicAccountIDToString(epic_id) : "invalid";
    UtilityFunctions::print(vformat("UserInfoSubsystem: query_product_id_for_user called for Epic ID: %s", epic_id_str));

    if (!EOS_EpicAccountId_IsValid(epic_id)) {
        UtilityFunctions::print("UserInfoSubsystem: query_product_id_for_user - Invalid Epic Account ID, returning early");
        return;
    }

    // Check if already querying
    if (pending_product_id_queries.find(epic_id_str) != pending_product_id_queries.end()) {
        UtilityFunctions::print(vformat("UserInfoSubsystem: query_product_id_for_user - Already querying Product ID for Epic ID %s, returning early", epic_id_str));
        return;
    }

    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        UtilityFunctions::print(vformat("UserInfoSubsystem: query_product_id_for_user - Authentication not available or not logged in for Epic ID %s, returning early", epic_id_str));
        return;
    }

    // Check if Connect login has completed (we need a valid Product User ID)
    EOS_ProductUserId local_product_id = auth->GetProductUserId();
    if (!EOS_ProductUserId_IsValid(local_product_id)) {
        UtilityFunctions::print(vformat("UserInfoSubsystem: query_product_id_for_user - Connect login not completed (no valid Product User ID) for Epic ID %s, returning early", epic_id_str));
        return;
    }

    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->GetPlatformHandle()) {
        UtilityFunctions::print(vformat("UserInfoSubsystem: query_product_id_for_user - Platform not available or invalid handle for Epic ID %s, returning early", epic_id_str));
        return;
    }

    EOS_HConnect connect_handle = EOS_Platform_GetConnectInterface(platform->GetPlatformHandle());
    if (!connect_handle) {
        UtilityFunctions::print(vformat("UserInfoSubsystem: query_product_id_for_user - Failed to get Connect interface for Epic ID %s, returning early", epic_id_str));
        return;
    }

    // Mark as querying
    pending_product_id_queries.insert(epic_id_str);
    UtilityFunctions::print(vformat("UserInfoSubsystem: query_product_id_for_user - Starting Product ID query for Epic ID %s", epic_id_str));

    // Setup query
    auto context = new QueryProductIdContext{this, epic_id};

    std::vector<String> account_strings = {epic_id_str};
    std::vector<const char*> account_chars = {epic_id_str.utf8().get_data()};

    EOS_Connect_QueryExternalAccountMappingsOptions options = {};
    options.ApiVersion = EOS_CONNECT_QUERYEXTERNALACCOUNTMAPPINGS_API_LATEST;
    options.LocalUserId = local_product_id;
    options.AccountIdType = EOS_EExternalAccountType::EOS_EAT_EPIC;
    options.ExternalAccountIds = account_chars.data();
    options.ExternalAccountIdCount = 1;

    EOS_Connect_QueryExternalAccountMappings(
        connect_handle,
        &options,
        context,
        on_product_id_query_complete
    );
}

Dictionary UserInfoSubsystem::copy_user_info_to_dictionary(EOS_EpicAccountId local_user_id, EOS_EpicAccountId target_user_id) {
    
    Dictionary result;

    EOS_UserInfo_CopyUserInfoOptions copy_options = {};
    copy_options.ApiVersion = EOS_USERINFO_COPYUSERINFO_API_LATEST;
    copy_options.LocalUserId = local_user_id;
    copy_options.TargetUserId = target_user_id;
    
    EOS_UserInfo* user_info = nullptr;
    EOS_EResult copy_result = EOS_UserInfo_CopyUserInfo(userinfo_handle, &copy_options, &user_info);
    
    if (copy_result == EOS_EResult::EOS_Success && user_info) {
        
        // Add target user ID to the result
        result["epic_account_id"] = FAccountHelpers::EpicAccountIDToString(target_user_id);
        
        // Extract display name
        if (user_info->DisplayName && strlen(user_info->DisplayName) > 0) {
            result["display_name"] = String::utf8(user_info->DisplayName);
        }

        // Extract nickname
        if (user_info->Nickname && strlen(user_info->Nickname) > 0) {
            result["nickname"] = String::utf8(user_info->Nickname);
        }

        // Extract country
        if (user_info->Country && strlen(user_info->Country) > 0) {
            result["country"] = String::utf8(user_info->Country);
        }

        // Extract preferred language
        if (user_info->PreferredLanguage && strlen(user_info->PreferredLanguage) > 0) {
            result["preferred_language"] = String::utf8(user_info->PreferredLanguage);
        }

        EOS_UserInfo_Release(user_info);
    }

    return result;
}

void EOS_CALL UserInfoSubsystem::on_query_user_info_complete(const EOS_UserInfo_QueryUserInfoCallbackInfo* data) {
    if (!data) {
        UtilityFunctions::printerr("UserInfoSubsystem: Query callback data is null");
        return;
    }

    std::unique_ptr<QueryUserInfoContext> context(static_cast<QueryUserInfoContext*>(data->ClientData));
    if (!context || !context->subsystem) {
        UtilityFunctions::printerr("UserInfoSubsystem: Invalid context in callback");
        return;
    }

    UserInfoSubsystem* subsystem = context->subsystem;
    String target_epic_id = FAccountHelpers::EpicAccountIDToString(context->target_user_id);

    UtilityFunctions::print(vformat("UserInfoSubsystem: on_query_user_info_complete called for Epic ID %s", target_epic_id));

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        UtilityFunctions::print("UserInfoSubsystem: User info query successful");

        // Create dictionary with user info data
        Dictionary user_info = subsystem->copy_user_info_to_dictionary(context->local_user_id, context->target_user_id);

        // Phase 1: Update cache with user info
        UserCacheEntry* entry = subsystem->find_cache_entry(context->target_user_id);
        if (entry) {
            entry->display_name = user_info.get("display_name", "");
            entry->nickname = user_info.get("nickname", "");
            entry->country = user_info.get("country", "");
            entry->preferred_language = user_info.get("preferred_language", "");

            // Phase 3: Query Product ID if needed and not already cached
            // Skip querying for local user since Product ID comes from Connect login
            // Also skip if local user doesn't have a valid Product User ID (Connect login not completed)
            auto auth_check = Get<IAuthenticationSubsystem>();
            EOS_ProductUserId local_product_id = auth_check ? auth_check->GetProductUserId() : nullptr;
            if (!entry->product_user_id && !entry->product_id_queried && !entry->is_local_user && EOS_ProductUserId_IsValid(local_product_id)) {
                UtilityFunctions::print(vformat("UserInfoSubsystem: on_query_user_info_complete - Triggering Product ID query for Epic ID %s (is_local_user: %s)",
                    target_epic_id, entry->is_local_user ? "true" : "false"));
                subsystem->query_product_id_for_user(context->target_user_id);
            } else {
                String reason = "already has Product ID";
                if (!entry->product_user_id && entry->product_id_queried) reason = "already queried";
                if (entry->is_local_user) reason = "is local user";
                if (!EOS_ProductUserId_IsValid(local_product_id)) reason = "Connect login not completed";
                UtilityFunctions::print(vformat("UserInfoSubsystem: on_query_user_info_complete - Skipping Product ID query for Epic ID %s (%s)",
                    target_epic_id, reason));
            }
        }

        // Emit callback if set
        if (subsystem->user_info_query_callback.is_valid()) {
            subsystem->user_info_query_callback.call(true, user_info);
        }
    } else {
        String error_msg = "UserInfoSubsystem: User info query failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
        UtilityFunctions::printerr(error_msg);

        // Emit callback with failure
        if (subsystem->user_info_query_callback.is_valid()) {
            subsystem->user_info_query_callback.call(false, Dictionary());
        }
    }
}

void EOS_CALL UserInfoSubsystem::on_product_id_query_complete(const EOS_Connect_QueryExternalAccountMappingsCallbackInfo* data) {
    UtilityFunctions::print("UserInfoSubsystem: on_product_id_query_complete callback triggered");

    if (!data) {
        UtilityFunctions::printerr("UserInfoSubsystem: on_product_id_query_complete - Callback data is null");
        return;
    }

    QueryProductIdContext* context = static_cast<QueryProductIdContext*>(data->ClientData);
    if (!context || !context->subsystem) {
        UtilityFunctions::printerr("UserInfoSubsystem: on_product_id_query_complete - Invalid context or subsystem");
        delete context;
        return;
    }

    UserInfoSubsystem* subsystem = context->subsystem;
    EOS_EpicAccountId epic_id = context->epic_account_id;
    String epic_id_str = FAccountHelpers::EpicAccountIDToString(epic_id);

    UtilityFunctions::print(vformat("UserInfoSubsystem: on_product_id_query_complete - Processing result for Epic ID %s", epic_id_str));

    // Remove from pending queries
    subsystem->pending_product_id_queries.erase(epic_id_str);
    UtilityFunctions::print(vformat("UserInfoSubsystem: on_product_id_query_complete - Removed Epic ID %s from pending queries", epic_id_str));

    // Mark that we've attempted to query the Product ID for this user
    UserCacheEntry* entry = subsystem->find_cache_entry(epic_id);
    if (entry) {
        entry->product_id_queried = true;
        UtilityFunctions::print(vformat("UserInfoSubsystem: on_product_id_query_complete - Marked Product ID as queried for Epic ID %s", epic_id_str));
    }

    if (data->ResultCode != EOS_EResult::EOS_Success) {
        UtilityFunctions::print(vformat("UserInfoSubsystem: Failed to query Product ID for Epic ID %s: %s",
            epic_id_str, EOS_EResult_ToString(data->ResultCode)));
        delete context;
        return;
    }

    UtilityFunctions::print(vformat("UserInfoSubsystem: on_product_id_query_complete - Query successful for Epic ID %s, retrieving mapping", epic_id_str));

    // Get the Product User ID from the mapping
    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        UtilityFunctions::print(vformat("UserInfoSubsystem: on_product_id_query_complete - Authentication not available for Epic ID %s", epic_id_str));
        delete context;
        return;
    }

    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->GetPlatformHandle()) {
        UtilityFunctions::print(vformat("UserInfoSubsystem: on_product_id_query_complete - Platform not available for Epic ID %s", epic_id_str));
        delete context;
        return;
    }

    EOS_HConnect connect_handle = EOS_Platform_GetConnectInterface(platform->GetPlatformHandle());
    if (!connect_handle) {
        UtilityFunctions::print(vformat("UserInfoSubsystem: on_product_id_query_complete - Connect interface not available for Epic ID %s", epic_id_str));
        delete context;
        return;
    }

    EOS_Connect_GetExternalAccountMappingsOptions get_options = {};
    get_options.ApiVersion = EOS_CONNECT_GETEXTERNALACCOUNTMAPPINGS_API_LATEST;
    get_options.LocalUserId = auth->GetProductUserId();
    get_options.AccountIdType = EOS_EExternalAccountType::EOS_EAT_EPIC;
    get_options.TargetExternalUserId = epic_id_str.utf8().get_data();

    EOS_ProductUserId product_id = EOS_Connect_GetExternalAccountMapping(
        connect_handle,
        &get_options
    );

    String product_id_str = EOS_ProductUserId_IsValid(product_id) ? FAccountHelpers::ProductUserIDToString(product_id) : "invalid/null";

    if (EOS_ProductUserId_IsValid(product_id)) {
        UtilityFunctions::print(vformat("UserInfoSubsystem: on_product_id_query_complete - Found valid Product ID %s for Epic ID %s", product_id_str, epic_id_str));

        // Update cache with Product ID
        // Determine if this is the local user
        auto auth_check = Get<IAuthenticationSubsystem>();
        bool is_local_user = (auth_check && auth_check->IsLoggedIn() && 
                             FAccountHelpers::EpicAccountIDToString(auth_check->GetEpicAccountId()) == epic_id_str);
        
        subsystem->UpdateUserCache(epic_id, product_id, is_local_user);
        UtilityFunctions::print(vformat("UserInfoSubsystem: Successfully cached Product ID for Epic ID %s", epic_id_str));

        // Emit success callback
        if (subsystem->user_cache_update_callback.is_valid()) {
            subsystem->user_cache_update_callback.call(true, epic_id_str, subsystem->GetCachedUserData(epic_id));
        }
    } else {
        // This is not necessarily an error - the Epic Account may not have a Product User ID mapping
        UtilityFunctions::print(vformat("UserInfoSubsystem: No Product ID mapping found for Epic ID %s (this is normal if the account hasn't used Connect login)", epic_id_str));
        
        // Still update cache to mark that we've checked for this user
        // Determine if this is the local user
        auto auth_check = Get<IAuthenticationSubsystem>();
        bool is_local_user = (auth_check && auth_check->IsLoggedIn() && 
                             FAccountHelpers::EpicAccountIDToString(auth_check->GetEpicAccountId()) == epic_id_str);
        
        subsystem->UpdateUserCache(epic_id, nullptr, is_local_user);

        // Emit failure callback
        if (subsystem->user_cache_update_callback.is_valid()) {
            subsystem->user_cache_update_callback.call(false, epic_id_str, subsystem->GetCachedUserData(epic_id));
        }
    }

    delete context;
}

} // namespace godot
