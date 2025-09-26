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
#include "EpicInitOptions.h"
#include "SubsystemManager.h"

namespace godot {

class GodotEpic : public Object {
	GDCLASS(GodotEpic, Object)

private:
	static GodotEpic* instance;

	// Authentication state
	EOS_EpicAccountId epic_account_id;
	EOS_ProductUserId product_user_id;
	bool is_logged_in;
	String current_username;

	// Achievements state
	EOS_NotificationId achievements_notification_id;
	bool achievements_definitions_cached;
	bool player_achievements_cached;

	double time_passed;

	// EOS logging callback
	static void EOS_CALL logging_callback(const EOS_LogMessage* message);

	// Authentication callbacks
	static void EOS_CALL auth_login_callback(const EOS_Auth_LoginCallbackInfo* data);
	static void EOS_CALL auth_logout_callback(const EOS_Auth_LogoutCallbackInfo* data);
	static void EOS_CALL connect_login_callback(const EOS_Connect_LoginCallbackInfo* data);

	// Friends callbacks
	static void EOS_CALL friends_query_callback(const EOS_Friends_QueryFriendsCallbackInfo* data);

	// Achievements callbacks
	static void EOS_CALL achievements_query_definitions_callback(const EOS_Achievements_OnQueryDefinitionsCompleteCallbackInfo* data);
	static void EOS_CALL achievements_query_player_callback(const EOS_Achievements_OnQueryPlayerAchievementsCompleteCallbackInfo* data);
	static void EOS_CALL achievements_unlock_callback(const EOS_Achievements_OnUnlockAchievementsCompleteCallbackInfo* data);
	static void EOS_CALL achievements_unlocked_notification(const EOS_Achievements_OnAchievementsUnlockedCallbackV2Info* data);

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

	// Achievements methods
	void query_achievement_definitions();
	void query_player_achievements();
	void unlock_achievement(const String& achievement_id);
	void unlock_achievements(const Array& achievement_ids);
	Array get_achievement_definitions();
	Array get_player_achievements();
	Dictionary get_achievement_definition(const String& achievement_id);
	Dictionary get_player_achievement(const String& achievement_id);

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

		// Test method for SubsystemManager functionality
	Dictionary test_subsystem_manager();

private:
	// Helper methods
	EpicInitOptions _dict_to_init_options(const Dictionary& options_dict);
	bool _validate_init_options(const EpicInitOptions& options);
	void setup_authentication_callback();
	void on_authentication_completed(bool success, const Dictionary& user_info);
};

}

#endif // GODOTEPIC_H