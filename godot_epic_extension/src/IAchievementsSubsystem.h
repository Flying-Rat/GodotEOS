#pragma once

#include "ISubsystem.h"
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/array.hpp>

namespace godot {

/**
 * @brief Achievements subsystem interface.
 *
 * Handles EOS achievements functionality including querying, unlocking,
 * and tracking player achievements.
 */
class IAchievementsSubsystem : public ISubsystem {
public:
    /**
     * @brief Query achievement definitions from EOS.
     * @return true if query initiated successfully.
     */
    virtual bool QueryAchievementDefinitions() = 0;

    /**
     * @brief Query player achievements from EOS.
     * @return true if query initiated successfully.
     */
    virtual bool QueryPlayerAchievements() = 0;

    /**
     * @brief Unlock an achievement for the current player.
     * @param achievement_id The achievement ID to unlock.
     * @return true if unlock request initiated successfully.
     */
    virtual bool UnlockAchievement(const String& achievement_id) = 0;

    /**
     * @brief Unlock multiple achievements for the current player.
     * @param achievement_ids Array of achievement IDs to unlock.
     * @return true if unlock request initiated successfully.
     */
    virtual bool UnlockAchievements(const Array& achievement_ids) = 0;

    /**
     * @brief Get cached achievement definitions.
     * @return Array of achievement definition dictionaries.
     */
    virtual Array GetAchievementDefinitions() const = 0;

    /**
     * @brief Get cached player achievements.
     * @return Array of player achievement dictionaries.
     */
    virtual Array GetPlayerAchievements() const = 0;

    /**
     * @brief Get a specific achievement definition.
     * @param achievement_id The achievement ID to retrieve.
     * @return Dictionary containing achievement definition, or empty dict if not found.
     */
    virtual Dictionary GetAchievementDefinition(const String& achievement_id) const = 0;

    /**
     * @brief Get a specific player achievement.
     * @param achievement_id The achievement ID to retrieve.
     * @return Dictionary containing player achievement, or empty dict if not found.
     */
    virtual Dictionary GetPlayerAchievement(const String& achievement_id) const = 0;

    /**
     * @brief Set a callback for achievement definitions query completion.
     * @param callback Callable to invoke when achievement definitions query completes.
     */
    virtual void SetAchievementDefinitionsCallback(const Callable& callback) = 0;

    /**
     * @brief Set a callback for player achievements query completion.
     * @param callback Callable to invoke when player achievements query completes.
     */
    virtual void SetPlayerAchievementsCallback(const Callable& callback) = 0;

    /**
     * @brief Set a callback for achievements unlocked completion.
     * @param callback Callable to invoke when achievements unlock completes.
     */
    virtual void SetAchievementsUnlockedCallback(const Callable& callback) = 0;

    /**
     * @brief Ingest a stat for the current player.
     * @param stat_name The name of the stat to ingest.
     * @param amount The amount to add to the stat.
     * @return true if ingest request initiated successfully.
     */
    virtual bool IngestStat(const String& stat_name, int amount) = 0;

    /**
     * @brief Query stats for the current player.
     * @return true if query initiated successfully.
     */
    virtual bool QueryStats() = 0;

    /**
     * @brief Get cached stats.
     * @return Array of stat dictionaries.
     */
    virtual Array GetStats() const = 0;

    /**
     * @brief Get a specific stat.
     * @param stat_name The stat name to retrieve.
     * @return Dictionary containing stat, or empty dict if not found.
     */
    virtual Dictionary GetStat(const String& stat_name) const = 0;

    /**
     * @brief Set a callback for stats query completion.
     * @param callback Callable to invoke when stats query completes.
     */
    virtual void SetStatsCallback(const Callable& callback) = 0;
};

} // namespace godot