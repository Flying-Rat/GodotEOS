#include "UserInfoSubsystem.h"
#include "../Utils/SubsystemManager.h"
#include "../Platform/IPlatformSubsystem.h"
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
        return "";
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

    return "";
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
    } else {
        String error_msg = "UserInfoSubsystem: User info query failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
        UtilityFunctions::printerr(error_msg);
    }
}

} // namespace godot
