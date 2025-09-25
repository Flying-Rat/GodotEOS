#pragma once

#include "IAchievementsSubsystem.h"
#include "../eos_sdk/Include/eos_achievements_types.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

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

private:
    EOS_HAchievements achievements_handle;
    EOS_NotificationId unlock_notification_id;

    Array achievement_definitions;
    Array player_achievements;

    bool definitions_cached;
    bool player_achievements_cached;

    // Callback functions
    static void EOS_CALL on_query_definitions_complete(const EOS_Achievements_OnQueryDefinitionsCompleteCallbackInfo* data);
    static void EOS_CALL on_query_player_achievements_complete(const EOS_Achievements_OnQueryPlayerAchievementsCompleteCallbackInfo* data);
    static void EOS_CALL on_unlock_achievements_complete(const EOS_Achievements_OnUnlockAchievementsCompleteCallbackInfo* data);
    static void EOS_CALL on_achievements_unlocked(const EOS_Achievements_OnAchievementsUnlockedCallbackV2Info* data);

    // Helper methods
    void setup_notifications();
    void cleanup_notifications();
};

} // namespace godot