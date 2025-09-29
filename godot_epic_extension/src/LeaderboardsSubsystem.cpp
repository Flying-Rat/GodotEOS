#include "LeaderboardsSubsystem.h"
#include "SubsystemManager.h"
#include "IPlatformSubsystem.h"
#include "IAuthenticationSubsystem.h"
#include "AuthenticationSubsystem.h"
#include "AccountHelpers.h"
#include "../eos_sdk/Include/eos_sdk.h"
#include "../eos_sdk/Include/eos_leaderboards.h"
#include <godot_cpp/core/error_macros.hpp>

namespace godot {

LeaderboardsSubsystem::LeaderboardsSubsystem()
    : leaderboards_handle(nullptr)
{
}

LeaderboardsSubsystem::~LeaderboardsSubsystem() {
    Shutdown();
}

bool LeaderboardsSubsystem::Init() {
    UtilityFunctions::print("LeaderboardsSubsystem: Initializing...");

    // Get and validate platform
    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->IsOnline()) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Platform not available or offline");
        return false;
    }

    EOS_HPlatform platform_handle = platform->GetPlatformHandle();
    if (!platform_handle) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Invalid platform handle");
        return false;
    }

    leaderboards_handle = EOS_Platform_GetLeaderboardsInterface(platform_handle);

    if (!leaderboards_handle) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Failed to get leaderboards interface");
        return false;
    }

    UtilityFunctions::print("LeaderboardsSubsystem: Initialized successfully");
    return true;
}

void LeaderboardsSubsystem::Tick(float delta_time) {
    // Process any leaderboard-related updates
    // Could check for cross-subsystem interactions here
}

void LeaderboardsSubsystem::Shutdown() {
    leaderboard_definitions.clear();
    leaderboard_ranks.clear();
    leaderboard_user_scores.clear();

    UtilityFunctions::print("LeaderboardsSubsystem: Shutdown complete");
}

bool LeaderboardsSubsystem::QueryLeaderboardDefinitions() {
    if (!leaderboards_handle) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Not initialized");
        return false;
    }

    if (!validate_user_authentication()) {
        return false;
    }

    auto auth = Get<IAuthenticationSubsystem>();

    EOS_Leaderboards_QueryLeaderboardDefinitionsOptions options = {};
    options.ApiVersion = EOS_LEADERBOARDS_QUERYLEADERBOARDDEFINITIONS_API_LATEST;
    options.StartTime = EOS_LEADERBOARDS_TIME_UNDEFINED;
    options.EndTime = EOS_LEADERBOARDS_TIME_UNDEFINED;
    options.LocalUserId = auth->GetProductUserIdHandle();;

    EOS_Leaderboards_QueryLeaderboardDefinitions(leaderboards_handle, &options, this, on_query_leaderboard_definitions_complete);
    UtilityFunctions::print("LeaderboardsSubsystem: Starting leaderboard definitions query");
    return true;
}

bool LeaderboardsSubsystem::QueryLeaderboardRanks(const String& leaderboard_id, int limit) {
    if (!leaderboards_handle) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Not initialized");
        return false;
    }

    if (leaderboard_id.is_empty()) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Invalid leaderboard ID");
        return false;
    }

    if (!validate_user_authentication()) {
        return false;
    }

    auto auth = Get<IAuthenticationSubsystem>();

    EOS_Leaderboards_QueryLeaderboardRanksOptions QueryRanksOptions = { 0 };
	QueryRanksOptions.ApiVersion = EOS_LEADERBOARDS_QUERYLEADERBOARDRANKS_API_LATEST;
	QueryRanksOptions.LeaderboardId = leaderboard_id.utf8().get_data();
	QueryRanksOptions.LocalUserId = auth->GetProductUserIdHandle();

    EOS_Leaderboards_QueryLeaderboardRanks(leaderboards_handle, &QueryRanksOptions, this, on_query_leaderboard_ranks_complete);
    return true;
}

bool LeaderboardsSubsystem::QueryLeaderboardUserScores(const String& leaderboard_id, const Array& user_ids) {
    if (!leaderboards_handle) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Not initialized");
        return false;
    }

    if (leaderboard_id.is_empty() || user_ids.size() == 0) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Invalid leaderboard ID or user IDs");
        return false;
    }

    if (!validate_user_authentication()) {
        return false;
    }

    // Find the leaderboard definition to get stat info
    Dictionary leaderboard_def;
    bool found = false;
    for (int i = 0; i < leaderboard_definitions.size(); i++) {
        Dictionary def = leaderboard_definitions[i];
        if (def["leaderboard_id"] == leaderboard_id) {
            leaderboard_def = def;
            found = true;
            break;
        }
    }

    if (!found) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Leaderboard definition not found for ID: " + leaderboard_id);
        return false;
    }

    String stat_name = leaderboard_def["stat_name"];
    int aggregation_int = leaderboard_def["aggregation"];
    EOS_ELeaderboardAggregation aggregation = static_cast<EOS_ELeaderboardAggregation>(aggregation_int);

    // Convert Godot Array to C array
    std::vector<EOS_ProductUserId> product_user_ids;
    std::vector<String> user_id_strings;

    for (int i = 0; i < user_ids.size(); i++) {
        String user_id_str = user_ids[i];
        user_id_strings.push_back(user_id_str);

        EOS_ProductUserId puid = FAccountHelpers::ProductUserIDFromString(user_id_str.utf8().get_data());
        if (!TValidateAccount<EOS_ProductUserId>::IsValid(puid)) {
            UtilityFunctions::printerr("LeaderboardsSubsystem: Invalid Product User ID: " + user_id_str);
            return false;
        }
        product_user_ids.push_back(puid);
    }

    auto auth = Get<IAuthenticationSubsystem>();

    EOS_Leaderboards_UserScoresQueryStatInfo stat_info = {};
    stat_info.ApiVersion = EOS_LEADERBOARDS_USERSCORESQUERYSTATINFO_API_LATEST;
    stat_info.StatName = stat_name.utf8().get_data();
    stat_info.Aggregation = aggregation;

    EOS_Leaderboards_QueryLeaderboardUserScoresOptions options = {};
    options.ApiVersion = EOS_LEADERBOARDS_QUERYLEADERBOARDUSERSCORES_API_LATEST;
    options.UserIds = product_user_ids.data();
    options.UserIdsCount = product_user_ids.size();
    options.StatInfo = &stat_info;
    options.StatInfoCount = 1;
    options.StartTime = EOS_LEADERBOARDS_TIME_UNDEFINED;
    options.EndTime = EOS_LEADERBOARDS_TIME_UNDEFINED;
    options.LocalUserId = auth->GetProductUserIdHandle();

    EOS_Leaderboards_QueryLeaderboardUserScores(leaderboards_handle, &options, this, on_query_leaderboard_user_scores_complete);
    return true;
}

Array LeaderboardsSubsystem::GetLeaderboardDefinitions() const {
    return leaderboard_definitions;
}

Array LeaderboardsSubsystem::GetLeaderboardRanks() const {
    return leaderboard_ranks;
}

Dictionary LeaderboardsSubsystem::GetLeaderboardUserScores() const {
    return leaderboard_user_scores;
}

void LeaderboardsSubsystem::SetLeaderboardDefinitionsCallback(const Callable& callback) {
    leaderboard_definitions_callback = callback;
}

void LeaderboardsSubsystem::SetLeaderboardRanksCallback(const Callable& callback) {
    leaderboard_ranks_callback = callback;
}

void LeaderboardsSubsystem::SetLeaderboardUserScoresCallback(const Callable& callback) {
    leaderboard_user_scores_callback = callback;
}

bool LeaderboardsSubsystem::validate_user_authentication() const {
    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: User not authenticated");
        return false;
    }
    return true;
}

// Static callback implementations
void EOS_CALL LeaderboardsSubsystem::on_query_leaderboard_definitions_complete(const EOS_Leaderboards_OnQueryLeaderboardDefinitionsCompleteCallbackInfo* Data) {
    LeaderboardsSubsystem* self = static_cast<LeaderboardsSubsystem*>(Data->ClientData);
    if (!Data || !self) return;

    if (Data->ResultCode == EOS_EResult::EOS_Success) {
        self->leaderboard_definitions.clear();

        // Get leaderboard definitions count
        const EOS_Leaderboards_GetLeaderboardDefinitionCountOptions count_options = { EOS_LEADERBOARDS_GETLEADERBOARDDEFINITIONCOUNT_API_LATEST };

        uint32_t definitions_count = EOS_Leaderboards_GetLeaderboardDefinitionCount(self->leaderboards_handle, &count_options);

        for (uint32_t i = 0; i < definitions_count; i++) {
            const EOS_Leaderboards_CopyLeaderboardDefinitionByIndexOptions copy_options = { EOS_LEADERBOARDS_COPYLEADERBOARDDEFINITIONBYINDEX_API_LATEST, i };

            EOS_Leaderboards_Definition* definition = nullptr;
            EOS_EResult result = EOS_Leaderboards_CopyLeaderboardDefinitionByIndex(self->leaderboards_handle, &copy_options, &definition);

            if (result == EOS_EResult::EOS_Success && definition) {
                Dictionary definition_dict;
                definition_dict["leaderboard_id"] = String(definition->LeaderboardId ? definition->LeaderboardId : "");
                definition_dict["stat_name"] = String(definition->StatName ? definition->StatName : "");
                definition_dict["aggregation"] = (int)definition->Aggregation;
                definition_dict["start_time"] = definition->StartTime;
                definition_dict["end_time"] = definition->EndTime;

                self->leaderboard_definitions.push_back(definition_dict);
                EOS_Leaderboards_Definition_Release(definition);
            }
        }

        // Call the callback if set
        if (self->leaderboard_definitions_callback.is_valid()) {
            Array definitions = self->GetLeaderboardDefinitions();
            self->leaderboard_definitions_callback.call(true, definitions);
        }
    } else {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Leaderboard definitions query failed");

        // Call the callback with failure
        if (self->leaderboard_definitions_callback.is_valid()) {
            Array empty_definitions;
            self->leaderboard_definitions_callback.call(false, empty_definitions);
        }
    }
}

void EOS_CALL LeaderboardsSubsystem::on_query_leaderboard_ranks_complete(const EOS_Leaderboards_OnQueryLeaderboardRanksCompleteCallbackInfo* data) {
    LeaderboardsSubsystem* self = static_cast<LeaderboardsSubsystem*>(data->ClientData);
    if (!data || !self) return;

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        self->leaderboard_ranks.clear();

        // Get leaderboard records count
        EOS_Leaderboards_GetLeaderboardRecordCountOptions count_options = {};
        count_options.ApiVersion = EOS_LEADERBOARDS_GETLEADERBOARDRECORDCOUNT_API_LATEST;

        uint32_t records_count = EOS_Leaderboards_GetLeaderboardRecordCount(self->leaderboards_handle, &count_options);

        for (uint32_t i = 0; i < records_count; i++) {
            EOS_Leaderboards_CopyLeaderboardRecordByIndexOptions copy_options = {};
            copy_options.ApiVersion = EOS_LEADERBOARDS_COPYLEADERBOARDRECORDBYINDEX_API_LATEST;
            copy_options.LeaderboardRecordIndex = i;

            EOS_Leaderboards_LeaderboardRecord* record = nullptr;
            EOS_EResult result = EOS_Leaderboards_CopyLeaderboardRecordByIndex(self->leaderboards_handle, &copy_options, &record);

            if (result == EOS_EResult::EOS_Success && record) {
                Dictionary record_dict;
                record_dict["rank"] = (int)record->Rank;
                record_dict["score"] = (int)record->Score;
                
                // Convert user ID to string
                record_dict["user_id"] = String(FAccountHelpers::ProductUserIDToString(record->UserId));
                
                // Get display name if available
                record_dict["display_name"] = String(record->UserDisplayName ? record->UserDisplayName : "");

                self->leaderboard_ranks.push_back(record_dict);
                EOS_Leaderboards_LeaderboardRecord_Release(record);
            }
        }

        // Call the callback if set
        if (self->leaderboard_ranks_callback.is_valid()) {
            Array ranks = self->GetLeaderboardRanks();
            self->leaderboard_ranks_callback.call(true, ranks);
        }
    } else {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Leaderboard ranks query failed");

        // Call the callback with failure
        if (self->leaderboard_ranks_callback.is_valid()) {
            Array empty_ranks;
            self->leaderboard_ranks_callback.call(false, empty_ranks);
        }
    }
}

void EOS_CALL LeaderboardsSubsystem::on_query_leaderboard_user_scores_complete(const EOS_Leaderboards_OnQueryLeaderboardUserScoresCompleteCallbackInfo* data) {
    LeaderboardsSubsystem* self = static_cast<LeaderboardsSubsystem*>(data->ClientData);
    if (!data || !self) return;

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        self->leaderboard_user_scores.clear();

        // Get user scores count
        const EOS_Leaderboards_GetLeaderboardUserScoreCountOptions count_options = { EOS_LEADERBOARDS_GETLEADERBOARDUSERSCORECOUNT_API_LATEST };

        uint32_t scores_count = EOS_Leaderboards_GetLeaderboardUserScoreCount(self->leaderboards_handle, &count_options);

        for (uint32_t i = 0; i < scores_count; i++) {
            const EOS_Leaderboards_CopyLeaderboardUserScoreByIndexOptions copy_options = { EOS_LEADERBOARDS_COPYLEADERBOARDUSERSCOREBYINDEX_API_LATEST, i };

            EOS_Leaderboards_LeaderboardUserScore* user_score = nullptr;
            EOS_EResult result = EOS_Leaderboards_CopyLeaderboardUserScoreByIndex(self->leaderboards_handle, &copy_options, &user_score);

            if (result == EOS_EResult::EOS_Success && user_score) {
                // Convert user ID to string
                String user_id = String(FAccountHelpers::ProductUserIDToString(user_score->UserId));

                Dictionary score_dict;
                score_dict["score"] = (int)user_score->Score;
                score_dict["rank"] = 0;  // Rank not available in user score struct

                self->leaderboard_user_scores[user_id] = score_dict;
                EOS_Leaderboards_LeaderboardUserScore_Release(user_score);
            }
        }

        // Call the callback if set
        if (self->leaderboard_user_scores_callback.is_valid()) {
            Dictionary user_scores = self->GetLeaderboardUserScores();
            self->leaderboard_user_scores_callback.call(true, user_scores);
        }
    } else {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Leaderboard user scores query failed");

        // Call the callback with failure
        if (self->leaderboard_user_scores_callback.is_valid()) {
            Dictionary empty_scores;
            self->leaderboard_user_scores_callback.call(false, empty_scores);
        }
    }
}

} // namespace godot