#pragma once

#include "IAchievementsSubsystem.h"
#include "../eos_sdk/Include/eos_achievements_types.h"
#include "../eos_sdk/Include/eos_stats_types.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace godot {

/**
 * @brief Achievements subsystem implementation.
 *
 * Manages EOS achievements functionality including querying definitions,
 * player achievements, and unlocking achievements.
 */
class AchievementsSubsystem : public IAchievementsSubsystem {
public:
    AchievementsSubsystem();
    ~AchievementsSubsystem() override;

    // ISubsystem interface
    bool Init() override;
    void Tick(float delta_time) override;
    void Shutdown() override;
    const char* GetSubsystemName() const override { return "Achievements"; }

    // IAchievementsSubsystem interface
    bool QueryAchievementDefinitions() override;
    bool QueryPlayerAchievements() override;
    bool UnlockAchievement(const String& achievement_id) override;
    bool UnlockAchievements(const Array& achievement_ids) override;
    Array GetAchievementDefinitions() const override;
    Array GetPlayerAchievements() const override;
    Dictionary GetAchievementDefinition(const String& achievement_id) const override;
    Dictionary GetPlayerAchievement(const String& achievement_id) const override;
    void SetAchievementDefinitionsCallback(const Callable& callback) override;
    void SetPlayerAchievementsCallback(const Callable& callback) override;
    void SetAchievementsUnlockedCallback(const Callable& callback) override;

    // Stats functionality
    bool IngestStat(const String& stat_name, int amount);
    bool QueryStats();
    Array GetStats() const;
    Dictionary GetStat(const String& stat_name) const;
    void SetStatsCallback(const Callable& callback);

private:
    EOS_HAchievements achievements_handle;
    EOS_HStats stats_handle;
    EOS_NotificationId unlock_notification_id;

    Array achievement_definitions;
    Array player_achievements;
    Array stats;

    bool definitions_cached;
    bool player_achievements_cached;
    bool stats_cached;

    // Callback callables
    Callable achievement_definitions_callback;
    Callable player_achievements_callback;
    Callable achievements_unlocked_callback;
    Callable stats_callback;

    // Callback functions
    static void EOS_CALL on_query_definitions_complete(const EOS_Achievements_OnQueryDefinitionsCompleteCallbackInfo* data);
    static void EOS_CALL on_query_player_achievements_complete(const EOS_Achievements_OnQueryPlayerAchievementsCompleteCallbackInfo* data);
    static void EOS_CALL on_unlock_achievements_complete(const EOS_Achievements_OnUnlockAchievementsCompleteCallbackInfo* data);
    static void EOS_CALL on_achievements_unlocked(const EOS_Achievements_OnAchievementsUnlockedCallbackV2Info* data);
    static void EOS_CALL on_ingest_stat_complete(const EOS_Stats_IngestStatCompleteCallbackInfo* data);
    static void EOS_CALL on_query_stats_complete(const EOS_Stats_OnQueryStatsCompleteCallbackInfo* data);

    // Helper methods
    void setup_notifications();
    void cleanup_notifications();
};

} // namespace godot