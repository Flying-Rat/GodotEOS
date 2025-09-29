#pragma once

#include "ILeaderboardsSubsystem.h"
#include "../eos_sdk/Include/eos_sdk.h"
#include "../eos_sdk/Include/eos_leaderboards.h"
#include "../eos_sdk/Include/eos_stats.h"
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {

class LeaderboardsSubsystem : public ILeaderboardsSubsystem {
public:
    LeaderboardsSubsystem();
    virtual ~LeaderboardsSubsystem();

    // ISubsystem interface
    virtual bool Init() override;
    virtual void Tick(float delta_time) override;
    virtual void Shutdown() override;
    virtual const char* GetSubsystemName() const override { return "LeaderboardsSubsystem"; }

    // ILeaderboardsSubsystem interface
    virtual bool QueryLeaderboardDefinitions() override;
    virtual bool QueryLeaderboardRanks(const String& leaderboard_id, int limit) override;
    virtual bool QueryLeaderboardUserScores(const String& leaderboard_id, const Array& user_ids) override;
    virtual Array GetLeaderboardDefinitions() const override;
    virtual Array GetLeaderboardRanks() const override;
    virtual Dictionary GetLeaderboardUserScores() const override;

    // Callback setters
    virtual void SetLeaderboardDefinitionsCallback(const Callable& callback) override;
    virtual void SetLeaderboardRanksCallback(const Callable& callback) override;
    virtual void SetLeaderboardUserScoresCallback(const Callable& callback) override;

private:
    // EOS handles
    EOS_HLeaderboards leaderboards_handle;
    EOS_HStats stats_handle;

    // Cached data
    Array leaderboard_definitions;
    Array leaderboard_ranks;
    Dictionary leaderboard_user_scores;

    // Callback callables
    Callable leaderboard_definitions_callback;
    Callable leaderboard_ranks_callback;
    Callable leaderboard_user_scores_callback;

    // Internal methods
    bool validate_user_authentication() const;

    // Static callback implementations
    static void EOS_CALL on_query_leaderboard_definitions_complete(const EOS_Leaderboards_OnQueryLeaderboardDefinitionsCompleteCallbackInfo* data);
    static void EOS_CALL on_query_leaderboard_ranks_complete(const EOS_Leaderboards_OnQueryLeaderboardRanksCompleteCallbackInfo* data);
    static void EOS_CALL on_query_leaderboard_user_scores_complete(const EOS_Leaderboards_OnQueryLeaderboardUserScoresCompleteCallbackInfo* data);
};

} // namespace godot