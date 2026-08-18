#include "LeaderboardsSubsystem.h"
#include "../Utils/SubsystemManager.h"
#include "../Platform/IPlatformSubsystem.h"
#include "../Authentication/IAuthenticationSubsystem.h"
#include "../Authentication/AuthenticationSubsystem.h"
#include "../Utils/AccountHelpers.h"
#include <eos_sdk.h>
#include <eos_leaderboards.h>
#include <godot_cpp/core/error_macros.hpp>
#include <memory>
#include <string>
#include <vector>
#include "../Utils/Logger.h"

namespace godot {

struct LeaderboardRanksQueryContext {
    LeaderboardsSubsystem* subsystem = nullptr;
    std::string leaderboard_id;
};

struct LeaderboardUserScoresQueryContext {
    LeaderboardsSubsystem* subsystem = nullptr;
    std::string stat_name;
    std::vector<EOS_ProductUserId> user_ids;
};

LeaderboardsSubsystem::LeaderboardsSubsystem()
    : leaderboards_handle(nullptr)
{
}

LeaderboardsSubsystem::~LeaderboardsSubsystem() {
    Shutdown();
}

bool LeaderboardsSubsystem::Init() {
    Logger::Info("Leaderboards", "Initializing...");

    // Get and validate platform
    auto platform = Get<IPlatformSubsystem>();
    if (!platform || !platform->IsOnline()) {
        Logger::Error("Leaderboards", "Platform not available or offline");
        return false;
    }

    EOS_HPlatform platform_handle = platform->GetPlatformHandle();
    if (!platform_handle) {
        Logger::Error("Leaderboards", "Invalid platform handle");
        return false;
    }

    leaderboards_handle = EOS_Platform_GetLeaderboardsInterface(platform_handle);

    if (!leaderboards_handle) {
        Logger::Error("Leaderboards", "Failed to get leaderboards interface");
        return false;
    }

    Logger::Info("Leaderboards", "Initialized successfully");
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

    Logger::Info("Leaderboards", "Shutdown complete");
}

bool LeaderboardsSubsystem::QueryLeaderboardDefinitions() {
    if (!leaderboards_handle) {
        Logger::Warning("Leaderboards", "Not initialized");
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
    options.LocalUserId = auth->GetProductUserId();

    EOS_Leaderboards_QueryLeaderboardDefinitions(leaderboards_handle, &options, this, on_query_leaderboard_definitions_complete);
    Logger::Info("Leaderboards", "Starting leaderboard definitions query");
    return true;
}

bool LeaderboardsSubsystem::QueryLeaderboardRanks(const String& leaderboard_id, int limit) {
    if (!leaderboards_handle) {
        Logger::Error("Leaderboards", "Not initialized");
        return false;
    }

    if (leaderboard_id.is_empty()) {
        Logger::Warning("Leaderboards", "Invalid leaderboard ID");
        return false;
    }

    if (!validate_user_authentication()) {
        return false;
    }

    auto auth = Get<IAuthenticationSubsystem>();
    EOS_ProductUserId local_user = auth->GetProductUserId();

    if (!TValidateAccount<EOS_ProductUserId>::IsValid(local_user)) {
        Logger::Error("Leaderboards", "Invalid local user Product User ID");
        return false;
    }

    std::unique_ptr<LeaderboardRanksQueryContext> context = std::make_unique<LeaderboardRanksQueryContext>();
    context->subsystem = this;
    context->leaderboard_id = leaderboard_id.utf8().get_data();

    Logger::Info("Leaderboards", "Querying ranks for leaderboard ID: " + leaderboard_id + " (limit: " + String::num_int64(limit) + ")");
    Logger::Info("Leaderboards", "Local user: " + FAccountHelpers::ProductUserIDToString(local_user));

    EOS_Leaderboards_QueryLeaderboardRanksOptions query_ranks_options = {};
    query_ranks_options.ApiVersion = EOS_LEADERBOARDS_QUERYLEADERBOARDRANKS_API_LATEST;
    query_ranks_options.LeaderboardId = context->leaderboard_id.c_str();
    query_ranks_options.LocalUserId = local_user;

    EOS_Leaderboards_QueryLeaderboardRanks(leaderboards_handle, &query_ranks_options, context.get(), on_query_leaderboard_ranks_complete);

    Logger::Info("Leaderboards", "Query submitted to EOS SDK, waiting for callback...");

    context.release();
    return true;
}

bool LeaderboardsSubsystem::QueryLeaderboardUserScores(const String& leaderboard_id, const Array& user_ids) {
    Logger::Info("Leaderboards", "QueryLeaderboardUserScores called for leaderboard '" + leaderboard_id + "' with " + String::num_int64(user_ids.size()) + " users");

    if (!leaderboards_handle) {
        Logger::Error("Leaderboards", "Not initialized");
        return false;
    }

    if (leaderboard_id.is_empty() || user_ids.size() == 0) {
        Logger::Warning("Leaderboards", "Invalid leaderboard ID or user IDs");
        return false;
    }

    if (!validate_user_authentication()) {
        return false;
    }

    // Check if leaderboard definitions have been queried
    if (leaderboard_definitions.size() == 0) {
        Logger::Warning("Leaderboards", "Leaderboard definitions not available. Call QueryLeaderboardDefinitions() first.");
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
        Logger::Warning("Leaderboards", "Leaderboard definition not found for ID: " + leaderboard_id);
        return false;
    }

    auto auth = Get<IAuthenticationSubsystem>();
    
    String stat_name = leaderboard_def["stat_name"];
    Logger::Info("Leaderboards", "Found leaderboard definition with stat name: " + stat_name);

    std::unique_ptr<LeaderboardUserScoresQueryContext> context = std::make_unique<LeaderboardUserScoresQueryContext>();
    context->subsystem = this;
    context->user_ids.reserve(user_ids.size());


    
    context->stat_name = stat_name.utf8().get_data();
    
    // Query User Scores
	EOS_Leaderboards_QueryLeaderboardUserScoresOptions QueryUserScoresOptions = { 0 };
	QueryUserScoresOptions.ApiVersion = EOS_LEADERBOARDS_QUERYLEADERBOARDUSERSCORES_API_LATEST;
	QueryUserScoresOptions.UserIdsCount = (uint32_t)user_ids.size();

    EOS_ProductUserId* UserData = new EOS_ProductUserId[user_ids.size()];
	for (uint32_t UserIndex = 0; UserIndex < user_ids.size(); ++UserIndex)
	{
        EOS_ProductUserId product_user_id = FAccountHelpers::ProductUserIDFromString(String(user_ids[UserIndex]).utf8().get_data());
        if (!TValidateAccount<EOS_ProductUserId>::IsValid(product_user_id)) 
        {
            Logger::Warning("Leaderboards", "Invalid Product User ID: " + String(user_ids[UserIndex]));
            return false;
        }
        else 
        {
            Logger::Info("Leaderboards", "Querying score for user: " + String(user_ids[UserIndex]));
        }

        UserData[UserIndex] = product_user_id;
        context->user_ids.push_back(product_user_id);
	}


	QueryUserScoresOptions.UserIds = UserData;
	QueryUserScoresOptions.StatInfoCount = 1;
    
	EOS_Leaderboards_UserScoresQueryStatInfo StatInfoData;
    StatInfoData.ApiVersion = EOS_LEADERBOARDS_USERSCORESQUERYSTATINFO_API_LATEST;
    StatInfoData.StatName = stat_name.utf8().get_data();
    StatInfoData.Aggregation = EOS_ELeaderboardAggregation::EOS_LA_Sum;

    QueryUserScoresOptions.StatInfo = &StatInfoData;
    QueryUserScoresOptions.StartTime = EOS_LEADERBOARDS_TIME_UNDEFINED;
	QueryUserScoresOptions.EndTime = EOS_LEADERBOARDS_TIME_UNDEFINED;
    QueryUserScoresOptions.LocalUserId = auth->GetProductUserId();

    Logger::Info("Leaderboards", "Submitting leaderboard user scores query with stat '" + stat_name + "' and aggregation type Sum");

    EOS_Leaderboards_QueryLeaderboardUserScores(leaderboards_handle, &QueryUserScoresOptions, context.get(), on_query_leaderboard_user_scores_complete);

    delete[] UserData;

    context.release();
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
        Logger::Warning("Leaderboards", "User not authenticated");
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

        Logger::Info("Leaderboards", "Retrieved " + String::num_int64(definitions_count) + " leaderboard definitions");

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

                String leaderboard_id_str = definition_dict["leaderboard_id"];
                String stat_name_str = definition_dict["stat_name"];
                Logger::Info("Leaderboards", "Found leaderboard - ID: " + leaderboard_id_str + ", Stat: " + stat_name_str);

                EOS_Leaderboards_Definition_Release(definition);
            }
        }

        Logger::Info("Leaderboards", "Leaderboard definitions query completed successfully");

        // Call the callback if set
        if (self->leaderboard_definitions_callback.is_valid()) {
            Array definitions = self->GetLeaderboardDefinitions();
            self->leaderboard_definitions_callback.call(true, definitions);
        }
    } else {
        String error_msg = "Leaderboard definitions query failed with error code: " + String::num_int64(static_cast<int64_t>(Data->ResultCode));
        Logger::Error("Leaderboards", error_msg);

        // Call the callback with failure
        if (self->leaderboard_definitions_callback.is_valid()) {
            Array empty_definitions;
            self->leaderboard_definitions_callback.call(false, empty_definitions);
        }
    }
}

void EOS_CALL LeaderboardsSubsystem::on_query_leaderboard_ranks_complete(const EOS_Leaderboards_OnQueryLeaderboardRanksCompleteCallbackInfo* data) {
    if (!data) {
        return;
    }

    std::unique_ptr<LeaderboardRanksQueryContext> context(static_cast<LeaderboardRanksQueryContext*>(data->ClientData));
    if (!context) {
        return;
    }

    LeaderboardsSubsystem* self = context->subsystem;
    if (!self) {
        return;
    }

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        self->leaderboard_ranks.clear();

        // Get leaderboard records count
        EOS_Leaderboards_GetLeaderboardRecordCountOptions count_options = {};
        count_options.ApiVersion = EOS_LEADERBOARDS_GETLEADERBOARDRECORDCOUNT_API_LATEST;

        uint32_t records_count = EOS_Leaderboards_GetLeaderboardRecordCount(self->leaderboards_handle, &count_options);

        Logger::Info("Leaderboards", "Retrieved " + String::num_int64(records_count) + " leaderboard records");

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
                record_dict["user_id"] = FAccountHelpers::ProductUserIDToString(record->UserId);
                // Get display name if available
                record_dict["display_name"] = record->UserDisplayName && strlen(record->UserDisplayName) > 0 ? String::utf8(record->UserDisplayName) : "";

                self->leaderboard_ranks.push_back(record_dict);
                EOS_Leaderboards_LeaderboardRecord_Release(record);
            }
        }

        Logger::Info("Leaderboards", "Leaderboard ranks query completed successfully");

        // Call the callback if set
        if (self->leaderboard_ranks_callback.is_valid()) {
            Array ranks = self->GetLeaderboardRanks();
            self->leaderboard_ranks_callback.call(true, ranks);
        }
    } else {
        String error_msg = "Leaderboard ranks query failed with error code: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
        Logger::Error("Leaderboards", error_msg);

        // Call the callback with failure
        if (self->leaderboard_ranks_callback.is_valid()) {
            Array empty_ranks;
            self->leaderboard_ranks_callback.call(false, empty_ranks);
        }
    }
}

void EOS_CALL LeaderboardsSubsystem::on_query_leaderboard_user_scores_complete(const EOS_Leaderboards_OnQueryLeaderboardUserScoresCompleteCallbackInfo* data) {

    Logger::Info("Leaderboards", "on_query_leaderboard_user_scores_complete called");
    if (!data) {
        Logger::Info("Leaderboards", "on_query_leaderboard_user_scores_complete received null data");
        return;
    }

    std::unique_ptr<LeaderboardUserScoresQueryContext> context(static_cast<LeaderboardUserScoresQueryContext*>(data->ClientData));
    if (!context) {
        Logger::Info("Leaderboards", "on_query_leaderboard_user_scores_complete received null context");
        return;
    }

    LeaderboardsSubsystem* self = context->subsystem;
    if (!self) {
        Logger::Info("Leaderboards", "on_query_leaderboard_user_scores_complete received null subsystem");
        return;
    }



    if (data->ResultCode == EOS_EResult::EOS_Success) {
        self->leaderboard_user_scores.clear();

        // Log requested users for debugging
        Logger::Info("Leaderboards", "Requested scores for " + String::num_int64(context->user_ids.size()) + " users");
        for (size_t i = 0; i < context->user_ids.size(); i++) {
            String user_id_str = FAccountHelpers::ProductUserIDToString(context->user_ids[i]);
            Logger::Info("Leaderboards", "Requested user: " + user_id_str);
        }

        std::string StatName = context->stat_name;

        EOS_Leaderboards_GetLeaderboardUserScoreCountOptions LeaderboardUserScoresCountOptions = { 0 };
        LeaderboardUserScoresCountOptions.ApiVersion = EOS_LEADERBOARDS_GETLEADERBOARDUSERSCORECOUNT_API_LATEST;
        LeaderboardUserScoresCountOptions.StatName = StatName.c_str();

        uint32_t scores_count = EOS_Leaderboards_GetLeaderboardUserScoreCount(self->leaderboards_handle, &LeaderboardUserScoresCountOptions);

        Logger::Info("Leaderboards", "Retrieved " + String::num_int64(scores_count) + " user scores (only users with stats are returned)");

        for (uint32_t i = 0; i < scores_count; i++) {
            EOS_Leaderboards_CopyLeaderboardUserScoreByIndexOptions copy_options = {};
            copy_options.ApiVersion = EOS_LEADERBOARDS_COPYLEADERBOARDUSERSCOREBYINDEX_API_LATEST;
            copy_options.LeaderboardUserScoreIndex = i;
            copy_options.StatName = context->stat_name.c_str();

            EOS_Leaderboards_LeaderboardUserScore* user_score = nullptr;
            EOS_EResult result = EOS_Leaderboards_CopyLeaderboardUserScoreByIndex(self->leaderboards_handle, &copy_options, &user_score);

            if (result == EOS_EResult::EOS_Success && user_score) {
                // Convert user ID to string
                String user_id = FAccountHelpers::ProductUserIDToString(user_score->UserId);

                Dictionary score_dict;
                score_dict["score"] = (int)user_score->Score;
                score_dict["display_name"] = "";  // Display name not provided by QueryLeaderboardUserScores API
                score_dict["rank"] = -1;  // Rank not provided by QueryLeaderboardUserScores API
                // Note: Rank is not provided by QueryLeaderboardUserScores API
                // Results are returned for users with scores, in arbitrary order

                self->leaderboard_user_scores[user_id] = score_dict;
                Logger::Info("Leaderboards", "Found score for user " + user_id + ": " + String::num_int64(user_score->Score));
                EOS_Leaderboards_LeaderboardUserScore_Release(user_score);
            }
        }

        Logger::Info("Leaderboards", "Leaderboard user scores query completed successfully");

        // Iterate through all leaderboard definitions and print score counts
        for (int i = 0; i < self->leaderboard_definitions.size(); i++) {
            Dictionary def = self->leaderboard_definitions[i];
            String stat_name = def["stat_name"];
            String leaderboard_id = def["leaderboard_id"];

            EOS_Leaderboards_GetLeaderboardUserScoreCountOptions options = {0};
            options.ApiVersion = EOS_LEADERBOARDS_GETLEADERBOARDUSERSCORECOUNT_API_LATEST;
            options.StatName = stat_name.utf8().get_data();

            uint32_t count = EOS_Leaderboards_GetLeaderboardUserScoreCount(self->leaderboards_handle, &options);

            Logger::Info("Leaderboards", "Leaderboard '" + leaderboard_id + "' (stat: " + stat_name + ") has " + String::num_int64(count) + " user scores");
        }

        // Call the callback if set
        if (self->leaderboard_user_scores_callback.is_valid()) {
            Dictionary user_scores = self->GetLeaderboardUserScores();
            self->leaderboard_user_scores_callback.call(true, user_scores);
        }
    } else {
        String error_msg = "Leaderboard user scores query failed with error code: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
        Logger::Error("Leaderboards", error_msg);

        // Call the callback with failure
        if (self->leaderboard_user_scores_callback.is_valid()) {
            Dictionary empty_scores;
            self->leaderboard_user_scores_callback.call(false, empty_scores);
        }
    }
}

} // namespace godot
