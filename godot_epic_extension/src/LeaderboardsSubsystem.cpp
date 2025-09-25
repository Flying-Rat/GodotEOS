#include "LeaderboardsSubsystem.h"
#include "SubsystemManager.h"
#include "IPlatformSubsystem.h"
#include "IAuthenticationSubsystem.h"
#include "../eos_sdk/Include/eos_sdk.h"
#include "../eos_sdk/Include/eos_leaderboards.h"
#include "../eos_sdk/Include/eos_stats.h"
#include <godot_cpp/core/error_macros.hpp>

namespace godot {

LeaderboardsSubsystem::LeaderboardsSubsystem()
    : leaderboards_handle(nullptr)
    , stats_handle(nullptr)
{
}

LeaderboardsSubsystem::~LeaderboardsSubsystem() {
    Shutdown();
}

bool LeaderboardsSubsystem::Init() {
    UtilityFunctions::print("LeaderboardsSubsystem: Initializing...");

    // Get platform handle from PlatformSubsystem
    auto platform = Get<IPlatformSubsystem>();
    if (!platform) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: PlatformSubsystem not available");
        return false;
    }

    if (!platform->IsOnline()) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Platform not online");
        return false;
    }

    EOS_HPlatform platform_handle = static_cast<EOS_HPlatform>(platform->GetPlatformHandle());
    if (!platform_handle) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Invalid platform handle");
        return false;
    }

    leaderboards_handle = EOS_Platform_GetLeaderboardsInterface(platform_handle);
    stats_handle = EOS_Platform_GetStatsInterface(platform_handle);

    if (!leaderboards_handle || !stats_handle) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Failed to get leaderboards/stats interfaces");
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

    EOS_Leaderboards_QueryLeaderboardDefinitionsOptions options = {};
    options.ApiVersion = EOS_LEADERBOARDS_QUERYLEADERBOARDDEFINITIONS_API_LATEST;

    EOS_Leaderboards_QueryLeaderboardDefinitions(leaderboards_handle, &options, nullptr, on_query_leaderboard_definitions_complete);
    UtilityFunctions::print("LeaderboardsSubsystem: Querying leaderboard definitions...");
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

    EOS_Leaderboards_QueryLeaderboardRanksOptions options = {};
    options.ApiVersion = EOS_LEADERBOARDS_QUERYLEADERBOARDRANKS_API_LATEST;
    options.LeaderboardId = leaderboard_id.utf8().get_data();

    // Note: Limit field may not be available in this EOS SDK version

    EOS_Leaderboards_QueryLeaderboardRanks(leaderboards_handle, &options, nullptr, on_query_leaderboard_ranks_complete);
    UtilityFunctions::print("LeaderboardsSubsystem: Querying leaderboard ranks for: " + leaderboard_id);
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

    // Convert Godot Array to C array
    std::vector<EOS_ProductUserId> product_user_ids;
    std::vector<String> user_id_strings;

    for (int i = 0; i < user_ids.size(); i++) {
        String user_id_str = user_ids[i];
        user_id_strings.push_back(user_id_str);

        EOS_ProductUserId puid = EOS_ProductUserId_FromString(user_id_str.utf8().get_data());
        if (!EOS_ProductUserId_IsValid(puid)) {
            UtilityFunctions::printerr("LeaderboardsSubsystem: Invalid Product User ID: " + user_id_str);
            return false;
        }
        product_user_ids.push_back(puid);
    }

    EOS_Leaderboards_QueryLeaderboardUserScoresOptions options = {};
    options.ApiVersion = EOS_LEADERBOARDS_QUERYLEADERBOARDUSERSCORES_API_LATEST;
    // options.LeaderboardId = leaderboard_id.utf8().get_data(); // Field may not exist in this version
    options.UserIds = product_user_ids.data();
    options.UserIdsCount = product_user_ids.size();

    EOS_Leaderboards_QueryLeaderboardUserScores(leaderboards_handle, &options, nullptr, on_query_leaderboard_user_scores_complete);
    UtilityFunctions::print("LeaderboardsSubsystem: Querying user scores for leaderboard: " + leaderboard_id);
    return true;
}

bool LeaderboardsSubsystem::IngestStat(const String& stat_name, int value) {
    Dictionary stats;
    stats[stat_name] = value;
    return IngestStats(stats);
}

bool LeaderboardsSubsystem::IngestStats(const Dictionary& stats) {
    if (!stats_handle) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Not initialized");
        return false;
    }

    if (stats.size() == 0) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: No stats provided");
        return false;
    }

    if (!validate_user_authentication()) {
        return false;
    }

    // Need Product User ID from AuthenticationSubsystem
    auto auth = Get<IAuthenticationSubsystem>();
    String product_user_id_str = auth->GetProductUserId();

    EOS_ProductUserId product_user_id = EOS_ProductUserId_FromString(product_user_id_str.utf8().get_data());
    if (!EOS_ProductUserId_IsValid(product_user_id)) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Failed to parse Product User ID");
        return false;
    }

    // Convert Godot Dictionary to EOS stats
    std::vector<EOS_Stats_IngestData> ingest_data;
    std::vector<String> stat_names;
    std::vector<String> stat_values;

    Array keys = stats.keys();
    for (int i = 0; i < keys.size(); i++) {
        String stat_name = keys[i];
        Variant stat_value = stats[stat_name];

        if (stat_value.get_type() != Variant::INT) {
            UtilityFunctions::printerr("LeaderboardsSubsystem: Stat value must be integer: " + stat_name);
            continue;
        }

        stat_names.push_back(stat_name);
        stat_values.push_back(String::num_int64((int64_t)stat_value));

        EOS_Stats_IngestData data = {};
        data.ApiVersion = EOS_STATS_INGESTDATA_API_LATEST;
        data.StatName = stat_names[i].utf8().get_data();
        data.IngestAmount = (int64_t)stat_value;
        ingest_data.push_back(data);
    }

    if (ingest_data.empty()) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: No valid stats to ingest");
        return false;
    }

    EOS_Stats_IngestStatOptions options = {};
    options.ApiVersion = EOS_STATS_INGESTSTAT_API_LATEST;
    options.LocalUserId = product_user_id;
    options.Stats = ingest_data.data();
    options.StatsCount = ingest_data.size();

    EOS_Stats_IngestStat(stats_handle, &options, nullptr, on_ingest_stat_complete);
    UtilityFunctions::print("LeaderboardsSubsystem: Ingesting " + String::num_int64(ingest_data.size()) + " stats...");
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

bool LeaderboardsSubsystem::validate_user_authentication() const {
    auto auth = Get<IAuthenticationSubsystem>();
    if (!auth || !auth->IsLoggedIn()) {
        UtilityFunctions::printerr("LeaderboardsSubsystem: User not authenticated");
        return false;
    }
    return true;
}

// Static callback implementations
void EOS_CALL LeaderboardsSubsystem::on_query_leaderboard_definitions_complete(const EOS_Leaderboards_OnQueryLeaderboardDefinitionsCompleteCallbackInfo* data) {
    if (!data) return;

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        UtilityFunctions::print("LeaderboardsSubsystem: Leaderboard definitions query successful");
        // Note: In a full implementation, we'd cache the definitions here
    } else {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Leaderboard definitions query failed");
    }
}

void EOS_CALL LeaderboardsSubsystem::on_query_leaderboard_ranks_complete(const EOS_Leaderboards_OnQueryLeaderboardRanksCompleteCallbackInfo* data) {
    if (!data) return;

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        UtilityFunctions::print("LeaderboardsSubsystem: Leaderboard ranks query successful");
        // Note: In a full implementation, we'd cache the ranks here
    } else {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Leaderboard ranks query failed");
    }
}

void EOS_CALL LeaderboardsSubsystem::on_query_leaderboard_user_scores_complete(const EOS_Leaderboards_OnQueryLeaderboardUserScoresCompleteCallbackInfo* data) {
    if (!data) return;

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        UtilityFunctions::print("LeaderboardsSubsystem: Leaderboard user scores query successful");
        // Note: In a full implementation, we'd cache the user scores here
    } else {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Leaderboard user scores query failed");
    }
}

void EOS_CALL LeaderboardsSubsystem::on_ingest_stat_complete(const EOS_Stats_IngestStatCompleteCallbackInfo* data) {
    if (!data) return;

    if (data->ResultCode == EOS_EResult::EOS_Success) {
        UtilityFunctions::print("LeaderboardsSubsystem: Stats ingested successfully");
        // Note: StatsCount field may not be available in this EOS SDK version
    } else {
        UtilityFunctions::printerr("LeaderboardsSubsystem: Stats ingestion failed");
    }
}

} // namespace godot