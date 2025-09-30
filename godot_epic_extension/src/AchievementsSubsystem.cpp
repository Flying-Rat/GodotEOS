#include "AchievementsSubsystem.h"
#include "SubsystemManager.h"
#include "IPlatformSubsystem.h"
#include "IAuthenticationSubsystem.h"
#include "AccountHelpers.h"
#include "../eos_sdk/Include/eos_sdk.h"
#include "../eos_sdk/Include/eos_achievements.h"
#include "../eos_sdk/Include/eos_stats.h"
#include <godot_cpp/core/error_macros.hpp>
#include <memory>
#include <string>
#include <vector>

namespace godot {

struct UnlockAchievementsContext {
    AchievementsSubsystem* subsystem = nullptr;
    std::vector<std::string> achievement_ids;
    std::vector<const char*> achievement_id_ptrs;
};

AchievementsSubsystem::AchievementsSubsystem()
    : achievements_handle(nullptr)
    , stats_handle(nullptr)
    , unlock_notification_id(EOS_INVALID_NOTIFICATIONID)
    , definitions_cached(false)
    , player_achievements_cached(false)
    , stats_cached(false)
{
}

AchievementsSubsystem::~AchievementsSubsystem() {
    Shutdown();
}

bool AchievementsSubsystem::Init() {
    UtilityFunctions::print("AchievementsSubsystem: Initializing...");

    // Get and validate platform
    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->IsOnline()) {
        UtilityFunctions::printerr("AchievementsSubsystem: Platform not available or offline");
        return false;
    }

    EOS_HPlatform platform_handle = platform->GetPlatformHandle();
    if (!platform_handle) {
        UtilityFunctions::printerr("AchievementsSubsystem: Invalid platform handle");
        return false;
    }

    achievements_handle = EOS_Platform_GetAchievementsInterface(platform_handle);
    if (!achievements_handle) {
        UtilityFunctions::printerr("AchievementsSubsystem: Failed to get achievements interface");
        return false;
    }

    stats_handle = EOS_Platform_GetStatsInterface(platform_handle);
    if (!stats_handle) {
        UtilityFunctions::printerr("AchievementsSubsystem: Failed to get stats interface");
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
    stats.clear();

    definitions_cached = false;
    player_achievements_cached = false;
    stats_cached = false;

    UtilityFunctions::print("AchievementsSubsystem: Shutdown complete");
}

bool AchievementsSubsystem::QueryAchievementDefinitions() {
    if (!achievements_handle) {
        UtilityFunctions::push_warning("AchievementsSubsystem: Not initialized");
        return false;
    }

    // Need Product User ID from AuthenticationSubsystem
    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        UtilityFunctions::push_warning("AchievementsSubsystem: User not authenticated");
        return false;
    }

    EOS_ProductUserId product_user_id = auth->GetProductUserId();
    if (!EOS_ProductUserId_IsValid(product_user_id)) {
        UtilityFunctions::printerr("AchievementsSubsystem: Invalid Product User ID");
        return false;
    }

    EOS_Achievements_QueryDefinitionsOptions options = {};
    options.ApiVersion = EOS_ACHIEVEMENTS_QUERYDEFINITIONS_API_LATEST;
    options.LocalUserId = product_user_id;

    EOS_Achievements_QueryDefinitions(achievements_handle, &options, this, on_query_definitions_complete);
    return true;
}

bool AchievementsSubsystem::QueryPlayerAchievements() {
    if (!achievements_handle) {
        UtilityFunctions::push_warning("AchievementsSubsystem: Not initialized");
        return false;
    }

    // Need Product User ID from AuthenticationSubsystem
    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        UtilityFunctions::push_warning("AchievementsSubsystem: User not authenticated");
        return false;
    }

    EOS_ProductUserId product_user_id = auth->GetProductUserId();
    if (!EOS_ProductUserId_IsValid(product_user_id)) {
        UtilityFunctions::printerr("AchievementsSubsystem: Invalid Product User ID");
        return false;
    }

    EOS_Achievements_QueryPlayerAchievementsOptions options = {};
    options.ApiVersion = EOS_ACHIEVEMENTS_QUERYPLAYERACHIEVEMENTS_API_LATEST;
    options.TargetUserId = product_user_id;
    options.LocalUserId = product_user_id;

    EOS_Achievements_QueryPlayerAchievements(achievements_handle, &options, this, on_query_player_achievements_complete);
    return true;
}

bool AchievementsSubsystem::UnlockAchievement(const String& achievement_id) {
    Array ids;
    ids.append(achievement_id);
    return UnlockAchievements(ids);
}

bool AchievementsSubsystem::UnlockAchievements(const Array& achievement_ids) {
    if (!achievements_handle) {
        UtilityFunctions::push_warning("AchievementsSubsystem: Not initialized");
        return false;
    }

    if (achievement_ids.size() == 0) {
        UtilityFunctions::push_warning("AchievementsSubsystem: No achievement IDs provided");
        return false;
    }

    // Need Product User ID from AuthenticationSubsystem
    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        UtilityFunctions::push_warning("AchievementsSubsystem: User not authenticated");
        return false;
    }

    EOS_ProductUserId product_user_id = auth->GetProductUserId();
    if (!EOS_ProductUserId_IsValid(product_user_id)) {
        UtilityFunctions::push_warning("AchievementsSubsystem: Invalid Product User ID");
        return false;
    }

    std::unique_ptr<UnlockAchievementsContext> context = std::make_unique<UnlockAchievementsContext>();
    context->subsystem = this;
    context->achievement_ids.reserve(achievement_ids.size());
    context->achievement_id_ptrs.reserve(achievement_ids.size());

    for (int i = 0; i < achievement_ids.size(); i++) {
        String achievement_id = achievement_ids[i];
        CharString id_utf8 = achievement_id.utf8();
        context->achievement_ids.emplace_back(id_utf8.get_data());
        context->achievement_id_ptrs.push_back(context->achievement_ids.back().c_str());
    }

    EOS_Achievements_UnlockAchievementsOptions options = {};
    options.ApiVersion = EOS_ACHIEVEMENTS_UNLOCKACHIEVEMENTS_API_LATEST;
    options.UserId = product_user_id;
    options.AchievementIds = context->achievement_id_ptrs.data();
    options.AchievementsCount = context->achievement_id_ptrs.size();

    EOS_Achievements_UnlockAchievements(achievements_handle, &options, context.get(), on_unlock_achievements_complete);
    context.release();
    UtilityFunctions::print("AchievementsSubsystem: Starting achievement unlock operation");
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

    EOS_ProductUserId product_user_id = auth->GetProductUserId();
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

void AchievementsSubsystem::SetAchievementDefinitionsCallback(const Callable& callback) {
    achievement_definitions_callback = callback;
}

void AchievementsSubsystem::SetPlayerAchievementsCallback(const Callable& callback) {
    player_achievements_callback = callback;
}

void AchievementsSubsystem::SetAchievementsUnlockedCallback(const Callable& callback) {
    achievements_unlocked_callback = callback;
}

bool AchievementsSubsystem::IngestStat(const String& stat_name, int amount) {
    if (!stats_handle) {
        UtilityFunctions::push_warning("AchievementsSubsystem: Not initialized");
        return false;
    }

    // Need Product User ID from AuthenticationSubsystem
    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        UtilityFunctions::push_warning("AchievementsSubsystem: User not authenticated");
        return false;
    }

    EOS_ProductUserId product_user_id = auth->GetProductUserId();
    if (!EOS_ProductUserId_IsValid(product_user_id)) {
        UtilityFunctions::push_warning("AchievementsSubsystem: Invalid Product User ID");
        return false;
    }

    if (stat_name.is_empty()) {
        UtilityFunctions::push_warning("AchievementsSubsystem: Stat name is empty");
        return false;
    }

    if (amount <= 0) {
        UtilityFunctions::push_warning("AchievementsSubsystem: Amount must be positive");
        return false;
    }

    EOS_Stats_IngestStatOptions options = {};
    options.ApiVersion = EOS_STATS_INGESTSTAT_API_LATEST;
    options.LocalUserId = product_user_id;
    options.TargetUserId = product_user_id;
    options.StatsCount = 1;

    EOS_Stats_IngestData ingest_data = {};
    ingest_data.ApiVersion = EOS_STATS_INGESTDATA_API_LATEST;
    std::string stat_name_str = stat_name.utf8().get_data();
    ingest_data.StatName = stat_name_str.c_str();
    ingest_data.IngestAmount = amount;

    options.Stats = &ingest_data;

    EOS_Stats_IngestStat(stats_handle, &options, this, on_ingest_stat_complete);
    return true;
}

bool AchievementsSubsystem::QueryStats() {
    if (!stats_handle) {
        UtilityFunctions::printerr("AchievementsSubsystem: Not initialized");
        return false;
    }

    // Need Product User ID from AuthenticationSubsystem
    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        UtilityFunctions::printerr("AchievementsSubsystem: User not authenticated");
        return false;
    }

    EOS_ProductUserId product_user_id = auth->GetProductUserId();
    if (!EOS_ProductUserId_IsValid(product_user_id)) {
        UtilityFunctions::printerr("AchievementsSubsystem: Invalid Product User ID");
        return false;
    }

    EOS_Stats_QueryStatsOptions options = {};
    options.ApiVersion = EOS_STATS_QUERYSTATS_API_LATEST;
    options.LocalUserId = product_user_id;
    options.TargetUserId = product_user_id;
    options.StartTime = EOS_STATS_TIME_UNDEFINED;
    options.EndTime = EOS_STATS_TIME_UNDEFINED;
    options.StatNamesCount = 0;
    options.StatNames = nullptr;

    EOS_Stats_QueryStats(stats_handle, &options, this, on_query_stats_complete);
    return true;
}

Array AchievementsSubsystem::GetStats() const {
    return stats;
}

Dictionary AchievementsSubsystem::GetStat(const String& stat_name) const {
    Dictionary result;

    for (int i = 0; i < stats.size(); i++) {
        Dictionary stat = stats[i];
        if (stat.has("name") && String(stat["name"]) == stat_name) {
            result = stat;
            break;
        }
    }

    return result;
}

void AchievementsSubsystem::SetStatsCallback(const Callable& callback) {
    stats_callback = callback;
}

void AchievementsSubsystem::setup_notifications() {
    if (!achievements_handle) return;

    EOS_Achievements_AddNotifyAchievementsUnlockedV2Options options = {};
    options.ApiVersion = EOS_ACHIEVEMENTS_ADDNOTIFYACHIEVEMENTSUNLOCKEDV2_API_LATEST;

    unlock_notification_id = EOS_Achievements_AddNotifyAchievementsUnlockedV2(
        achievements_handle, &options, nullptr, on_achievements_unlocked);

    if (unlock_notification_id == EOS_INVALID_NOTIFICATIONID) {
        UtilityFunctions::printerr("AchievementsSubsystem: Failed to register unlock notifications");
    }
}

void AchievementsSubsystem::cleanup_notifications() {
    if (achievements_handle && unlock_notification_id != EOS_INVALID_NOTIFICATIONID) {
        EOS_Achievements_RemoveNotifyAchievementsUnlocked(achievements_handle, unlock_notification_id);
        unlock_notification_id = EOS_INVALID_NOTIFICATIONID;
    }
}

// Static callback implementations
void EOS_CALL AchievementsSubsystem::on_query_definitions_complete(const EOS_Achievements_OnQueryDefinitionsCompleteCallbackInfo* data) {
    AchievementsSubsystem* self = static_cast<AchievementsSubsystem*>(data->ClientData);
    if (!data || !self) return;

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        self->achievement_definitions.clear();

        EOS_Achievements_GetAchievementDefinitionCountOptions count_options = {};
        count_options.ApiVersion = EOS_ACHIEVEMENTS_GETACHIEVEMENTDEFINITIONCOUNT_API_LATEST;

        uint32_t definitions_count = EOS_Achievements_GetAchievementDefinitionCount(self->achievements_handle, &count_options);

        for (uint32_t i = 0; i < definitions_count; i++) {
            EOS_Achievements_CopyAchievementDefinitionV2ByIndexOptions copy_options = {};
            copy_options.ApiVersion = EOS_ACHIEVEMENTS_COPYACHIEVEMENTDEFINITIONV2BYINDEX_API_LATEST;
            copy_options.AchievementIndex = i;

            EOS_Achievements_DefinitionV2* definition = nullptr;
            EOS_EResult result = EOS_Achievements_CopyAchievementDefinitionV2ByIndex(self->achievements_handle, &copy_options, &definition);

            if (result == EOS_EResult::EOS_Success && definition) {
                Dictionary definition_dict;
                definition_dict["achievement_id"] = String(definition->AchievementId ? definition->AchievementId : "");
                definition_dict["unlocked_display_name"] = String(definition->UnlockedDisplayName ? definition->UnlockedDisplayName : "");
                definition_dict["unlocked_description"] = String(definition->UnlockedDescription ? definition->UnlockedDescription : "");
                definition_dict["locked_display_name"] = String(definition->LockedDisplayName ? definition->LockedDisplayName : "");
                definition_dict["locked_description"] = String(definition->LockedDescription ? definition->LockedDescription : "");
                definition_dict["flavor_text"] = String(definition->FlavorText ? definition->FlavorText : "");
                definition_dict["unlocked_icon_url"] = String(definition->UnlockedIconURL ? definition->UnlockedIconURL : "");
                definition_dict["locked_icon_url"] = String(definition->LockedIconURL ? definition->LockedIconURL : "");
                definition_dict["is_hidden"] = definition->bIsHidden == EOS_TRUE;

                self->achievement_definitions.push_back(definition_dict);
                EOS_Achievements_DefinitionV2_Release(definition);
            }
        }

        self->definitions_cached = true;

        // Call the callback if set
        if (self->achievement_definitions_callback.is_valid()) {
            Array definitions = self->GetAchievementDefinitions();
            self->achievement_definitions_callback.call(true, definitions);
        }
    } else {
        UtilityFunctions::printerr("AchievementsSubsystem: Achievement definitions query failed");

        // Call the callback with failure
        if (self->achievement_definitions_callback.is_valid()) {
            Array empty_definitions;
            UtilityFunctions::print("AchievementsSubsystem: Calling achievement definitions callback with failure (empty definitions)");
            self->achievement_definitions_callback.call(false, empty_definitions);
        }
    }
}

void EOS_CALL AchievementsSubsystem::on_query_player_achievements_complete(const EOS_Achievements_OnQueryPlayerAchievementsCompleteCallbackInfo* data) {
    AchievementsSubsystem* self = static_cast<AchievementsSubsystem*>(data->ClientData);
    if (!data || !self) return;

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        self->player_achievements.clear();

        // Get Product User ID from AuthenticationSubsystem
        auto auth = godot::Get<IAuthenticationSubsystem>();
        if (!auth || !auth->IsLoggedIn()) {
            UtilityFunctions::printerr("AchievementsSubsystem: User not authenticated during player achievements query callback");
            return;
        }

        EOS_ProductUserId product_user_id = auth->GetProductUserId();
        if (!EOS_ProductUserId_IsValid(product_user_id)) {
            UtilityFunctions::printerr("AchievementsSubsystem: Invalid Product User ID during player achievements query callback");
            return;
        }

        EOS_Achievements_GetPlayerAchievementCountOptions count_options = {};
        count_options.ApiVersion = EOS_ACHIEVEMENTS_GETPLAYERACHIEVEMENTCOUNT_API_LATEST;
        count_options.UserId = product_user_id;

        uint32_t achievements_count = EOS_Achievements_GetPlayerAchievementCount(self->achievements_handle, &count_options);

        for (uint32_t i = 0; i < achievements_count; i++) {
            EOS_Achievements_CopyPlayerAchievementByIndexOptions copy_options = {};
            copy_options.ApiVersion = EOS_ACHIEVEMENTS_COPYPLAYERACHIEVEMENTBYINDEX_API_LATEST;
            copy_options.TargetUserId = product_user_id;
            copy_options.AchievementIndex = i;
            copy_options.LocalUserId = product_user_id;

            EOS_Achievements_PlayerAchievement* achievement = nullptr;
            EOS_EResult result = EOS_Achievements_CopyPlayerAchievementByIndex(self->achievements_handle, &copy_options, &achievement);

            if (result == EOS_EResult::EOS_Success && achievement) {
                Dictionary achievement_dict;
                achievement_dict["achievement_id"] = String(achievement->AchievementId ? achievement->AchievementId : "");
                achievement_dict["progress"] = achievement->Progress;
                achievement_dict["unlock_time"] = achievement->UnlockTime;
                achievement_dict["is_unlocked"] = achievement->UnlockTime != EOS_ACHIEVEMENTS_ACHIEVEMENT_UNLOCKTIME_UNDEFINED;
                achievement_dict["display_name"] = String(achievement->DisplayName ? achievement->DisplayName : "");
                achievement_dict["description"] = String(achievement->Description ? achievement->Description : "");
                achievement_dict["icon_url"] = String(achievement->IconURL ? achievement->IconURL : "");
                achievement_dict["flavor_text"] = String(achievement->FlavorText ? achievement->FlavorText : "");

                self->player_achievements.push_back(achievement_dict);
                EOS_Achievements_PlayerAchievement_Release(achievement);
            }
        }

        self->player_achievements_cached = true;

        // Call the callback if set
        if (self->player_achievements_callback.is_valid()) {
            Array achievements = self->GetPlayerAchievements();
            self->player_achievements_callback.call(true, achievements);
        }

    } else {
        UtilityFunctions::printerr("AchievementsSubsystem: Player achievements query failed");

        // Call the callback with failure
        if (self->player_achievements_callback.is_valid()) {
            Array empty_achievements;
            UtilityFunctions::print("AchievementsSubsystem: Calling player achievements callback with failure (empty achievements)");
            self->player_achievements_callback.call(false, empty_achievements);
        }
    }
}

void EOS_CALL AchievementsSubsystem::on_unlock_achievements_complete(const EOS_Achievements_OnUnlockAchievementsCompleteCallbackInfo* data) {
    if (!data) {
        return;
    }

    std::unique_ptr<UnlockAchievementsContext> context(static_cast<UnlockAchievementsContext*>(data->ClientData));
    if (!context) {
        return;
    }

    AchievementsSubsystem* self = context->subsystem;
    if (!self) {
        return;
    }

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        // Call the callback if set
        if (self->achievements_unlocked_callback.is_valid()) {
            Array unlocked_ids; // We don't have the specific IDs here, could be enhanced later
            self->achievements_unlocked_callback.call(true, unlocked_ids);
        }
    } else {
        UtilityFunctions::printerr("AchievementsSubsystem: Achievement unlock failed");

        // Call the callback with failure
        if (self->achievements_unlocked_callback.is_valid()) {
            Array empty_unlocked;
            self->achievements_unlocked_callback.call(false, empty_unlocked);
        }
    }
}

void EOS_CALL AchievementsSubsystem::on_achievements_unlocked(const EOS_Achievements_OnAchievementsUnlockedCallbackV2Info* data) {
    if (!data) return;

    String achievement_id = String(data->AchievementId ? data->AchievementId : "");
    UtilityFunctions::print("AchievementsSubsystem: Achievement unlocked: " + achievement_id);
}

void EOS_CALL AchievementsSubsystem::on_ingest_stat_complete(const EOS_Stats_IngestStatCompleteCallbackInfo* data) {
    AchievementsSubsystem* self = static_cast<AchievementsSubsystem*>(data->ClientData);
    if (!data || !self) return;

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        // Stat ingested successfully - no need to log
    } else {
        UtilityFunctions::printerr("AchievementsSubsystem: Failed to ingest stat, error: " + String::num_int64((int64_t)data->ResultCode));
    }
}

void EOS_CALL AchievementsSubsystem::on_query_stats_complete(const EOS_Stats_OnQueryStatsCompleteCallbackInfo* data) {
    AchievementsSubsystem* self = static_cast<AchievementsSubsystem*>(data->ClientData);
    if (!data || !self) return;

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        // Cache the stats
        self->stats.clear();

        EOS_Stats_GetStatCountOptions count_options = {};
        count_options.ApiVersion = EOS_STATS_GETSTATCOUNT_API_LATEST;
        count_options.TargetUserId = data->TargetUserId;

        uint32_t stat_count = EOS_Stats_GetStatsCount(self->stats_handle, &count_options);

        for (uint32_t i = 0; i < stat_count; ++i) {
            EOS_Stats_Stat* stat = nullptr;
            EOS_Stats_CopyStatByIndexOptions copy_options = {};
            copy_options.ApiVersion = EOS_STATS_COPYSTATBYINDEX_API_LATEST;
            copy_options.TargetUserId = data->TargetUserId;
            copy_options.StatIndex = i;

            EOS_EResult copy_result = EOS_Stats_CopyStatByIndex(self->stats_handle, &copy_options, &stat);
            if (copy_result == EOS_EResult::EOS_Success && stat) {
                Dictionary stat_dict;
                stat_dict["name"] = String(stat->Name ? stat->Name : "");
                stat_dict["value"] = stat->Value;
                stat_dict["start_time"] = stat->StartTime;
                stat_dict["end_time"] = stat->EndTime;

                self->stats.append(stat_dict);
                EOS_Stats_Stat_Release(stat);
            }
        }

        self->stats_cached = true;

        if (self->stats_callback.is_valid()) {
            self->stats_callback.call(true, self->stats);
        }
    } else {
        UtilityFunctions::printerr("AchievementsSubsystem: Failed to query stats, error: " + String::num_int64((int64_t)data->ResultCode));

        if (self->stats_callback.is_valid()) {
            Array empty_stats;
            self->stats_callback.call(false, empty_stats);
        }
    }
}

} // namespace godot
