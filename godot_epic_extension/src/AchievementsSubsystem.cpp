#include "AchievementsSubsystem.h"
#include "SubsystemManager.h"
#include "IPlatformSubsystem.h"
#include "IAuthenticationSubsystem.h"
#include "../eos_sdk/Include/eos_sdk.h"
#include "../eos_sdk/Include/eos_achievements.h"
#include <godot_cpp/core/error_macros.hpp>

namespace godot {

AchievementsSubsystem::AchievementsSubsystem()
    : achievements_handle(nullptr)
    , unlock_notification_id(EOS_INVALID_NOTIFICATIONID)
    , definitions_cached(false)
    , player_achievements_cached(false)
{
}

AchievementsSubsystem::~AchievementsSubsystem() {
    Shutdown();
}

bool AchievementsSubsystem::Init() {
    UtilityFunctions::print("AchievementsSubsystem: Initializing...");

    // Get platform handle from PlatformSubsystem
    auto platform = Get<IPlatformSubsystem>();
    if (!platform) {
        UtilityFunctions::printerr("AchievementsSubsystem: PlatformSubsystem not available");
        return false;
    }

    if (!platform->IsOnline()) {
        UtilityFunctions::printerr("AchievementsSubsystem: Platform not online");
        return false;
    }

    EOS_HPlatform platform_handle = static_cast<EOS_HPlatform>(platform->GetPlatformHandle());
    if (!platform_handle) {
        UtilityFunctions::printerr("AchievementsSubsystem: Invalid platform handle");
        return false;
    }

    achievements_handle = EOS_Platform_GetAchievementsInterface(platform_handle);
    if (!achievements_handle) {
        UtilityFunctions::printerr("AchievementsSubsystem: Failed to get achievements interface");
        return false;
    }

    setup_notifications();
    UtilityFunctions::print("AchievementsSubsystem: Initialized successfully");
    return true;
}

void AchievementsSubsystem::Tick(float delta_time) {
    // Process any achievement-related updates
    // Could check for cross-subsystem interactions here
}

void AchievementsSubsystem::Shutdown() {
    cleanup_notifications();

    achievement_definitions.clear();
    player_achievements.clear();

    definitions_cached = false;
    player_achievements_cached = false;

    UtilityFunctions::print("AchievementsSubsystem: Shutdown complete");
}

bool AchievementsSubsystem::QueryAchievementDefinitions() {
    if (!achievements_handle) {
        UtilityFunctions::printerr("AchievementsSubsystem: Not initialized");
        return false;
    }

    EOS_Achievements_QueryDefinitionsOptions options = {};
    options.ApiVersion = EOS_ACHIEVEMENTS_QUERYDEFINITIONS_API_LATEST;

    EOS_Achievements_QueryDefinitions(achievements_handle, &options, nullptr, on_query_definitions_complete);
    UtilityFunctions::print("AchievementsSubsystem: Querying achievement definitions...");
    return true;
}

bool AchievementsSubsystem::QueryPlayerAchievements() {
    if (!achievements_handle) {
        UtilityFunctions::printerr("AchievementsSubsystem: Not initialized");
        return false;
    }

    // Need Product User ID from AuthenticationSubsystem
    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        UtilityFunctions::printerr("AchievementsSubsystem: User not authenticated");
        return false;
    }

    String product_user_id_str = auth->GetProductUserId();
    if (product_user_id_str.is_empty()) {
        UtilityFunctions::printerr("AchievementsSubsystem: Invalid Product User ID");
        return false;
    }

    EOS_ProductUserId product_user_id = EOS_ProductUserId_FromString(product_user_id_str.utf8().get_data());
    if (!EOS_ProductUserId_IsValid(product_user_id)) {
        UtilityFunctions::printerr("AchievementsSubsystem: Failed to parse Product User ID");
        return false;
    }

    EOS_Achievements_QueryPlayerAchievementsOptions options = {};
    options.ApiVersion = EOS_ACHIEVEMENTS_QUERYPLAYERACHIEVEMENTS_API_LATEST;
    options.TargetUserId = product_user_id;
    options.LocalUserId = product_user_id;

    EOS_Achievements_QueryPlayerAchievements(achievements_handle, &options, nullptr, on_query_player_achievements_complete);
    UtilityFunctions::print("AchievementsSubsystem: Querying player achievements...");
    return true;
}

bool AchievementsSubsystem::UnlockAchievement(const String& achievement_id) {
    Array ids;
    ids.append(achievement_id);
    return UnlockAchievements(ids);
}

bool AchievementsSubsystem::UnlockAchievements(const Array& achievement_ids) {
    if (!achievements_handle) {
        UtilityFunctions::printerr("AchievementsSubsystem: Not initialized");
        return false;
    }

    if (achievement_ids.size() == 0) {
        UtilityFunctions::printerr("AchievementsSubsystem: No achievement IDs provided");
        return false;
    }

    // Need Product User ID from AuthenticationSubsystem
    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        UtilityFunctions::printerr("AchievementsSubsystem: User not authenticated");
        return false;
    }

    String product_user_id_str = auth->GetProductUserId();
    if (product_user_id_str.is_empty()) {
        UtilityFunctions::printerr("AchievementsSubsystem: Invalid Product User ID");
        return false;
    }

    EOS_ProductUserId product_user_id = EOS_ProductUserId_FromString(product_user_id_str.utf8().get_data());
    if (!EOS_ProductUserId_IsValid(product_user_id)) {
        UtilityFunctions::printerr("AchievementsSubsystem: Failed to parse Product User ID");
        return false;
    }

    // Convert Godot Array to C array
    std::vector<const char*> achievement_id_ptrs;
    std::vector<String> achievement_id_strings;

    for (int i = 0; i < achievement_ids.size(); i++) {
        String achievement_id = achievement_ids[i];
        achievement_id_strings.push_back(achievement_id);
        achievement_id_ptrs.push_back(achievement_id_strings[i].utf8().get_data());
    }

    EOS_Achievements_UnlockAchievementsOptions options = {};
    options.ApiVersion = EOS_ACHIEVEMENTS_UNLOCKACHIEVEMENTS_API_LATEST;
    options.UserId = product_user_id;
    options.AchievementIds = achievement_id_ptrs.data();
    options.AchievementsCount = achievement_ids.size();

    EOS_Achievements_UnlockAchievements(achievements_handle, &options, nullptr, on_unlock_achievements_complete);
    UtilityFunctions::print("AchievementsSubsystem: Unlocking " + String::num_int64(achievement_ids.size()) + " achievements...");
    return true;
}

Array AchievementsSubsystem::GetAchievementDefinitions() const {
    return achievement_definitions;
}

Array AchievementsSubsystem::GetPlayerAchievements() const {
    return player_achievements;
}

Dictionary AchievementsSubsystem::GetAchievementDefinition(const String& achievement_id) const {
    Dictionary result;

    if (!achievements_handle) {
        return result;
    }

    EOS_Achievements_CopyAchievementDefinitionV2ByAchievementIdOptions options = {};
    options.ApiVersion = EOS_ACHIEVEMENTS_COPYACHIEVEMENTDEFINITIONV2BYACHIEVEMENTID_API_LATEST;
    options.AchievementId = achievement_id.utf8().get_data();

    EOS_Achievements_DefinitionV2* definition = nullptr;
    EOS_EResult eos_result = EOS_Achievements_CopyAchievementDefinitionV2ByAchievementId(achievements_handle, &options, &definition);

    if (eos_result == EOS_EResult::EOS_Success && definition) {
        result["achievement_id"] = String(definition->AchievementId ? definition->AchievementId : "");
        result["unlocked_display_name"] = String(definition->UnlockedDisplayName ? definition->UnlockedDisplayName : "");
        result["unlocked_description"] = String(definition->UnlockedDescription ? definition->UnlockedDescription : "");
        result["locked_display_name"] = String(definition->LockedDisplayName ? definition->LockedDisplayName : "");
        result["locked_description"] = String(definition->LockedDescription ? definition->LockedDescription : "");
        result["flavor_text"] = String(definition->FlavorText ? definition->FlavorText : "");
        result["unlocked_icon_url"] = String(definition->UnlockedIconURL ? definition->UnlockedIconURL : "");
        result["locked_icon_url"] = String(definition->LockedIconURL ? definition->LockedIconURL : "");
        result["is_hidden"] = definition->bIsHidden == EOS_TRUE;

        EOS_Achievements_DefinitionV2_Release(definition);
    }

    return result;
}

Dictionary AchievementsSubsystem::GetPlayerAchievement(const String& achievement_id) const {
    Dictionary result;

    if (!achievements_handle) {
        return result;
    }

    // Need Product User ID from AuthenticationSubsystem
    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        return result;
    }

    String product_user_id_str = auth->GetProductUserId();
    if (product_user_id_str.is_empty()) {
        return result;
    }

    EOS_ProductUserId product_user_id = EOS_ProductUserId_FromString(product_user_id_str.utf8().get_data());
    if (!EOS_ProductUserId_IsValid(product_user_id)) {
        return result;
    }

    EOS_Achievements_CopyPlayerAchievementByAchievementIdOptions options = {};
    options.ApiVersion = EOS_ACHIEVEMENTS_COPYPLAYERACHIEVEMENTBYACHIEVEMENTID_API_LATEST;
    options.TargetUserId = product_user_id;
    options.AchievementId = achievement_id.utf8().get_data();
    options.LocalUserId = product_user_id;

    EOS_Achievements_PlayerAchievement* achievement = nullptr;
    EOS_EResult eos_result = EOS_Achievements_CopyPlayerAchievementByAchievementId(achievements_handle, &options, &achievement);

    if (eos_result == EOS_EResult::EOS_Success && achievement) {
        result["achievement_id"] = String(achievement->AchievementId ? achievement->AchievementId : "");
        result["progress"] = achievement->Progress;
        result["unlock_time"] = achievement->UnlockTime;
        result["is_unlocked"] = achievement->UnlockTime != EOS_ACHIEVEMENTS_ACHIEVEMENT_UNLOCKTIME_UNDEFINED;
        result["display_name"] = String(achievement->DisplayName ? achievement->DisplayName : "");
        result["description"] = String(achievement->Description ? achievement->Description : "");
        result["icon_url"] = String(achievement->IconURL ? achievement->IconURL : "");
        result["flavor_text"] = String(achievement->FlavorText ? achievement->FlavorText : "");

        EOS_Achievements_PlayerAchievement_Release(achievement);
    }

    return result;
}

void AchievementsSubsystem::setup_notifications() {
    if (!achievements_handle) return;

    EOS_Achievements_AddNotifyAchievementsUnlockedV2Options options = {};
    options.ApiVersion = EOS_ACHIEVEMENTS_ADDNOTIFYACHIEVEMENTSUNLOCKEDV2_API_LATEST;

    unlock_notification_id = EOS_Achievements_AddNotifyAchievementsUnlockedV2(
        achievements_handle, &options, nullptr, on_achievements_unlocked);

    if (unlock_notification_id != EOS_INVALID_NOTIFICATIONID) {
        UtilityFunctions::print("AchievementsSubsystem: Unlock notifications registered");
    } else {
        UtilityFunctions::printerr("AchievementsSubsystem: Failed to register unlock notifications");
    }
}

void AchievementsSubsystem::cleanup_notifications() {
    if (achievements_handle && unlock_notification_id != EOS_INVALID_NOTIFICATIONID) {
        EOS_Achievements_RemoveNotifyAchievementsUnlocked(achievements_handle, unlock_notification_id);
        unlock_notification_id = EOS_INVALID_NOTIFICATIONID;
        UtilityFunctions::print("AchievementsSubsystem: Unlock notifications unregistered");
    }
}

// Static callback implementations
void EOS_CALL AchievementsSubsystem::on_query_definitions_complete(const EOS_Achievements_OnQueryDefinitionsCompleteCallbackInfo* data) {
    if (!data) return;

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        UtilityFunctions::print("AchievementsSubsystem: Achievement definitions query successful");
        // Note: In a full implementation, we'd cache the definitions here
    } else {
        UtilityFunctions::printerr("AchievementsSubsystem: Achievement definitions query failed");
    }
}

void EOS_CALL AchievementsSubsystem::on_query_player_achievements_complete(const EOS_Achievements_OnQueryPlayerAchievementsCompleteCallbackInfo* data) {
    if (!data) return;

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        UtilityFunctions::print("AchievementsSubsystem: Player achievements query successful");
        // Note: In a full implementation, we'd cache the achievements here
    } else {
        UtilityFunctions::printerr("AchievementsSubsystem: Player achievements query failed");
    }
}

void EOS_CALL AchievementsSubsystem::on_unlock_achievements_complete(const EOS_Achievements_OnUnlockAchievementsCompleteCallbackInfo* data) {
    if (!data) return;

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        UtilityFunctions::print("AchievementsSubsystem: Achievements unlocked successfully (" +
                               String::num_int64(data->AchievementsCount) + " achievements)");
    } else {
        UtilityFunctions::printerr("AchievementsSubsystem: Achievement unlock failed");
    }
}

void EOS_CALL AchievementsSubsystem::on_achievements_unlocked(const EOS_Achievements_OnAchievementsUnlockedCallbackV2Info* data) {
    if (!data) return;

    String achievement_id = String(data->AchievementId ? data->AchievementId : "");
    UtilityFunctions::print("AchievementsSubsystem: Achievement unlocked: " + achievement_id);
}

} // namespace godot