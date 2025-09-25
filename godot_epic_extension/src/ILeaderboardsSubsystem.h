#pragma once

#include "ISubsystem.h"
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot {

/**
 * @brief Leaderboards subsystem interface.
 *
 * Handles EOS leaderboards functionality including querying, submitting scores,
 * and managing statistics.
 */
class ILeaderboardsSubsystem : public ISubsystem {
public:
    /**
     * @brief Query leaderboard definitions from EOS.
     * @return true if query initiated successfully.
     */
    virtual bool QueryLeaderboardDefinitions() = 0;

    /**
     * @brief Query leaderboard ranks for a specific leaderboard.
     * @param leaderboard_id The leaderboard ID to query.
     * @param limit Maximum number of entries to retrieve.
     * @return true if query initiated successfully.
     */
    virtual bool QueryLeaderboardRanks(const String& leaderboard_id, int limit) = 0;

    /**
     * @brief Query leaderboard user scores for specific users.
     * @param leaderboard_id The leaderboard ID to query.
     * @param user_ids Array of user IDs to query scores for.
     * @return true if query initiated successfully.
     */
    virtual bool QueryLeaderboardUserScores(const String& leaderboard_id, const Array& user_ids) = 0;

    /**
     * @brief Ingest a single statistic value.
     * @param stat_name The statistic name.
     * @param value The statistic value.
     * @return true if statistic ingestion initiated successfully.
     */
    virtual bool IngestStat(const String& stat_name, int value) = 0;

    /**
     * @brief Ingest multiple statistics.
     * @param stats Dictionary of stat names to values.
     * @return true if statistics ingestion initiated successfully.
     */
    virtual bool IngestStats(const Dictionary& stats) = 0;

    /**
     * @brief Get cached leaderboard definitions.
     * @return Array of leaderboard definition dictionaries.
     */
    virtual Array GetLeaderboardDefinitions() const = 0;

    /**
     * @brief Get cached leaderboard ranks.
     * @return Array of leaderboard rank dictionaries.
     */
    virtual Array GetLeaderboardRanks() const = 0;

    /**
     * @brief Get cached leaderboard user scores.
     * @return Dictionary of user scores.
     */
    virtual Dictionary GetLeaderboardUserScores() const = 0;
};

} // namespace godot