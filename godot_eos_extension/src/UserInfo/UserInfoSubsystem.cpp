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

bool UserInfoSubsystem::QueryUserInfo(EOS_EpicAccountId local_user_id, EOS_EpicAccountId target_user_id) {
    if (!userinfo_handle) {
        UtilityFunctions::printerr("UserInfoSubsystem: Not initialized");
        return false;
    }

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

Dictionary UserInfoSubsystem::GetCachedUserInfo(EOS_EpicAccountId local_user_id, EOS_EpicAccountId target_user_id) {
    if (!userinfo_handle) {
        return Dictionary();
    }

    if (!EOS_EpicAccountId_IsValid(local_user_id) || !EOS_EpicAccountId_IsValid(target_user_id)) {
        return Dictionary();
    }

    return copy_user_info_to_dictionary(local_user_id, target_user_id);
}

String UserInfoSubsystem::GetUserDisplayName(EOS_EpicAccountId local_user_id, EOS_EpicAccountId target_user_id) {
    Dictionary user_info = GetCachedUserInfo(local_user_id, target_user_id);
    
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

bool UserInfoSubsystem::IsUserInfoCached(EOS_EpicAccountId local_user_id, EOS_EpicAccountId target_user_id) {
    if (!userinfo_handle) {
        return false;
    }

    if (!EOS_EpicAccountId_IsValid(local_user_id) || !EOS_EpicAccountId_IsValid(target_user_id)) {
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

// Phase 1: User cache implementation

void UserInfoSubsystem::UpdateUserCache(EOS_EpicAccountId epic_id, EOS_ProductUserId product_id, bool is_local_user) {
    if (!EOS_EpicAccountId_IsValid(epic_id)) {
        return;
    }

    String epic_id_str = FAccountHelpers::EpicAccountIDToString(epic_id);
    String product_id_str = product_id && EOS_ProductUserId_IsValid(product_id) ? FAccountHelpers::ProductUserIDToString(product_id) : "null";

    UtilityFunctions::print(vformat("UserInfoSubsystem: UpdateUserCache called - Epic ID: %s, Product ID: %s, Is Local User: %s", epic_id_str, product_id_str, is_local_user ? "true" : "false"));

    // Find existing entry or create new one
    UserCacheEntry* entry = find_cache_entry(epic_id);

    if (!entry) {
        // Create new entry
        UserCacheEntry new_entry;
        new_entry.epic_account_id = epic_id;
        new_entry.is_local_user = is_local_user;
        user_cache.push_back(new_entry);
        entry = &user_cache.back();
    }

    // Update entry
    entry->is_local_user = is_local_user;

    // Update Product ID if provided
    if (product_id && EOS_ProductUserId_IsValid(product_id)) {
        entry->product_user_id = product_id;
    }

    // Query user info if not already cached
    if (entry->display_name.is_empty()) {
        auto auth = Get<IAuthenticationSubsystem>();
        if (auth && auth->IsLoggedIn()) {
            QueryUserInfo(auth->GetEpicAccountId(), epic_id);
        }
    }

    // Query Product ID if needed and not already provided
    // Skip querying for local user since Product ID comes from Connect login
    if (!entry->product_user_id && !product_id && !is_local_user) {
        query_product_id_for_user(epic_id);
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
        result["display_name"] = entry->display_name;
        result["nickname"] = entry->nickname;
        result["country"] = entry->country;
        result["preferred_language"] = entry->preferred_language;
        result["is_local_user"] = entry->is_local_user;

        if (entry->product_user_id && EOS_ProductUserId_IsValid(entry->product_user_id)) {
            result["product_user_id"] = FAccountHelpers::ProductUserIDToString(entry->product_user_id);
        }
    }

    return result;
}

// Phase 1: Cache helper methods

UserInfoSubsystem::UserCacheEntry* UserInfoSubsystem::find_cache_entry(EOS_EpicAccountId epic_id) {
    if (!EOS_EpicAccountId_IsValid(epic_id)) {
        return nullptr;
    }

    String epic_id_str = FAccountHelpers::EpicAccountIDToString(epic_id);

    for (auto& entry : user_cache) {
        if (FAccountHelpers::EpicAccountIDToString(entry.epic_account_id) == epic_id_str) {
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

    for (const auto& entry : user_cache) {
        if (FAccountHelpers::EpicAccountIDToString(entry.epic_account_id) == epic_id_str) {
            return &entry;
        }
    }

    return nullptr;
}

void UserInfoSubsystem::query_product_id_for_user(EOS_EpicAccountId epic_id) {
    if (!EOS_EpicAccountId_IsValid(epic_id)) {
        return;
    }

    String epic_id_str = FAccountHelpers::EpicAccountIDToString(epic_id);

    // Check if already querying
    if (pending_product_id_queries.find(epic_id_str) != pending_product_id_queries.end()) {
        return;
    }

    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        return;
    }

    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->GetPlatformHandle()) {
        return;
    }

    EOS_HConnect connect_handle = EOS_Platform_GetConnectInterface(platform->GetPlatformHandle());
    if (!connect_handle) {
        return;
    }

    // Mark as querying
    pending_product_id_queries.insert(epic_id_str);

    // Setup query
    auto context = new QueryProductIdContext{this, epic_id};

    std::vector<String> account_strings = {epic_id_str};
    std::vector<const char*> account_chars = {epic_id_str.utf8().get_data()};

    EOS_Connect_QueryExternalAccountMappingsOptions options = {};
    options.ApiVersion = EOS_CONNECT_QUERYEXTERNALACCOUNTMAPPINGS_API_LATEST;
    options.LocalUserId = auth->GetProductUserId();
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
        result["target_user_id"] = FAccountHelpers::EpicAccountIDToString(target_user_id);
        
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
    if (!data) {
        return;
    }

    QueryProductIdContext* context = static_cast<QueryProductIdContext*>(data->ClientData);
    if (!context || !context->subsystem) {
        delete context;
        return;
    }

    UserInfoSubsystem* subsystem = context->subsystem;
    EOS_EpicAccountId epic_id = context->epic_account_id;
    String epic_id_str = FAccountHelpers::EpicAccountIDToString(epic_id);

    // Remove from pending queries
    subsystem->pending_product_id_queries.erase(epic_id_str);

    if (data->ResultCode != EOS_EResult::EOS_Success) {
        UtilityFunctions::print(vformat("UserInfoSubsystem: Failed to query Product ID for Epic ID %s: %s",
            epic_id_str, EOS_EResult_ToString(data->ResultCode)));
        delete context;
        return;
    }

    // Get the Product User ID from the mapping
    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        delete context;
        return;
    }

    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->GetPlatformHandle()) {
        delete context;
        return;
    }

    EOS_HConnect connect_handle = EOS_Platform_GetConnectInterface(platform->GetPlatformHandle());
    if (!connect_handle) {
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

    if (EOS_ProductUserId_IsValid(product_id)) {
        // Update cache with Product ID
        // Determine if this is the local user
        auto auth_check = Get<IAuthenticationSubsystem>();
        bool is_local_user = (auth_check && auth_check->IsLoggedIn() && 
                             FAccountHelpers::EpicAccountIDToString(auth_check->GetEpicAccountId()) == epic_id_str);
        
        subsystem->UpdateUserCache(epic_id, product_id, is_local_user);
        UtilityFunctions::print(vformat("UserInfoSubsystem: Successfully cached Product ID for Epic ID %s", epic_id_str));
    } else {
        UtilityFunctions::print(vformat("UserInfoSubsystem: Failed to get Product ID mapping for Epic ID %s", epic_id_str));
    }

    delete context;
}

} // namespace godot
