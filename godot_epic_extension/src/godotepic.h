#ifndef GODOTEPIC_H
#define GODOTEPIC_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>
#include <eos_sdk.h>
#include <eos_types.h>
#include <eos_init.h>
#include <eos_logging.h>
#include <eos_auth.h>
#include <eos_connect.h>
#include <eos_friends.h>
#include <eos_achievements.h>
#include <eos_userinfo.h>
#include "EpicInitOptions.h"
#include "SubsystemManager.h"
#include "IFriendsSubsystem.h"

namespace godot {

class GodotEpic : public Object {
	GDCLASS(GodotEpic, Object)

private:
	static GodotEpic* instance;

	// Authentication state
	EOS_ProductUserId product_user_id;
	bool is_logged_in;
	String current_username;

	double time_passed;

	// EOS logging callback
	static void EOS_CALL logging_callback(const EOS_LogMessage* message);

protected:
	static void _bind_methods();

public:
	GodotEpic();
	~GodotEpic();

	// Singleton access
	static GodotEpic* get_singleton();

	// Platform management
	bool initialize_platform(const Dictionary& options);
	void shutdown_platform();
	void tick();

	// Authentication methods
	void login_with_epic_account(const String& email, const String& password);
	void login_with_device_id(const String& display_name);
	void login_with_dev(const String& display_name);
	void logout();
	bool is_user_logged_in() const;
	String get_current_username() const;
	String get_epic_account_id() const;
	String get_product_user_id() const;

	// Friends methods
	void query_friends();
	Array get_friends_list();
	Dictionary get_friend_info(const String& friend_id);
	void query_friend_info(const String& friend_id);
	void query_all_friends_info();

	// Achievements methods
	void query_achievement_definitions();
	void query_player_achievements();
	void unlock_achievement(const String& achievement_id);
	void unlock_achievements(const Array& achievement_ids);
	Array get_achievement_definitions();
	Array get_player_achievements();
	Dictionary get_achievement_definition(const String& achievement_id);
	Dictionary get_player_achievement(const String& achievement_id);

	// Achievement Stats methods
	void ingest_achievement_stat(const String& stat_name, int amount);
	void query_achievement_stats();
	Array get_achievement_stats();
	Dictionary get_achievement_stat(const String& stat_name);

	// Leaderboards methods
	void query_leaderboard_definitions();
	void query_leaderboard_ranks(const String& leaderboard_id, int limit = 100);
	void query_leaderboard_user_scores(const String& leaderboard_id, const Array& user_ids);
	void ingest_stat(const String& stat_name, int value);
	void ingest_stats(const Dictionary& stats);
	Array get_leaderboard_definitions();
	Array get_leaderboard_ranks();
	Dictionary get_leaderboard_user_scores();

	// Status methods
	bool is_platform_initialized() const;
	EOS_HPlatform get_platform_handle() const;

private:
	// Helper methods
	EpicInitOptions _dict_to_init_options(const Dictionary& options_dict);
	bool _validate_init_options(const EpicInitOptions& options);
	
	void setup_authentication_callback();
	void setup_achievements_callbacks();
	void setup_leaderboards_callbacks();
	void setup_friends_callbacks();

	void on_authentication_completed(bool success, const Dictionary& user_info);
	void on_achievement_definitions_completed(bool success, const Array& definitions);
	void on_player_achievements_completed(bool success, const Array& achievements);
	void on_achievements_unlocked_completed(bool success, const Array& unlocked_achievement_ids);
	void on_achievement_stats_completed(bool success, const Array& stats);
	void on_leaderboard_definitions_completed(bool success, const Array& definitions);
	void on_leaderboard_ranks_completed(bool success, const Array& ranks);
	void on_leaderboard_user_scores_completed(bool success, const Dictionary& user_scores);
	void on_friends_query_completed(bool success, const Array& friends_list);
	void on_friend_info_query_completed(bool success, const Dictionary& friend_info);
};

}

#endif // GODOTEPIC_H