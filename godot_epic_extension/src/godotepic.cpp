#include "godotepic.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "SubsystemManager.h"
#include "IAuthenticationSubsystem.h"
#include "IAchievementsSubsystem.h"
#include "ILeaderboardsSubsystem.h"
#include "IFriendsSubsystem.h"
#include "IUserInfoSubsystem.h"
#include "PlatformSubsystem.h"
#include "AuthenticationSubsystem.h"
#include "AchievementsSubsystem.h"
#include "LeaderboardsSubsystem.h"
#include "FriendsSubsystem.h"
#include "UserInfoSubsystem.h"
#include "AccountHelpers.h"

using namespace godot;

// Static member definitions
GodotEpic* GodotEpic::instance = nullptr;

void GodotEpic::_bind_methods() {
	ClassDB::bind_static_method("GodotEpic", D_METHOD("get_singleton"), &GodotEpic::get_singleton);
	ClassDB::bind_method(D_METHOD("initialize_platform", "options"), &GodotEpic::initialize_platform);
	ClassDB::bind_method(D_METHOD("shutdown_platform"), &GodotEpic::shutdown_platform);
	ClassDB::bind_method(D_METHOD("tick", "delta"), &GodotEpic::tick);
	ClassDB::bind_method(D_METHOD("is_platform_initialized"), &GodotEpic::is_platform_initialized);

	// Authentication methods
	ClassDB::bind_method(D_METHOD("login_with_epic_account", "email", "password"), &GodotEpic::login_with_epic_account);
	ClassDB::bind_method(D_METHOD("login_with_account_portal"), &GodotEpic::login_with_account_portal);
	ClassDB::bind_method(D_METHOD("login_with_device_id", "display_name"), &GodotEpic::login_with_device_id);
	ClassDB::bind_method(D_METHOD("login_with_dev", "display_name"), &GodotEpic::login_with_dev);
	ClassDB::bind_method(D_METHOD("logout"), &GodotEpic::logout);
	ClassDB::bind_method(D_METHOD("is_user_logged_in"), &GodotEpic::is_user_logged_in);
	ClassDB::bind_method(D_METHOD("get_current_username"), &GodotEpic::get_current_username);
	ClassDB::bind_method(D_METHOD("get_epic_account_id"), &GodotEpic::get_epic_account_id);
	ClassDB::bind_method(D_METHOD("get_product_user_id"), &GodotEpic::get_product_user_id);

	// Friends methods
	ClassDB::bind_method(D_METHOD("query_friends"), &GodotEpic::query_friends);
	ClassDB::bind_method(D_METHOD("get_friends_list"), &GodotEpic::get_friends_list);
	ClassDB::bind_method(D_METHOD("get_friend_info", "friend_id"), &GodotEpic::get_friend_info);
	ClassDB::bind_method(D_METHOD("query_friend_info", "friend_id"), &GodotEpic::query_friend_info);
	ClassDB::bind_method(D_METHOD("query_all_friends_info"), &GodotEpic::query_all_friends_info);

	// Achievements methods
	ClassDB::bind_method(D_METHOD("query_achievement_definitions"), &GodotEpic::query_achievement_definitions);
	ClassDB::bind_method(D_METHOD("query_player_achievements"), &GodotEpic::query_player_achievements);
	ClassDB::bind_method(D_METHOD("unlock_achievement", "achievement_id"), &GodotEpic::unlock_achievement);
	ClassDB::bind_method(D_METHOD("unlock_achievements", "achievement_ids"), &GodotEpic::unlock_achievements);
	ClassDB::bind_method(D_METHOD("get_achievement_definitions"), &GodotEpic::get_achievement_definitions);
	ClassDB::bind_method(D_METHOD("get_player_achievements"), &GodotEpic::get_player_achievements);
	ClassDB::bind_method(D_METHOD("get_achievement_definition", "achievement_id"), &GodotEpic::get_achievement_definition);
	ClassDB::bind_method(D_METHOD("get_player_achievement", "achievement_id"), &GodotEpic::get_player_achievement);

	// Stats methods (AchievementsSubsystem)
	ClassDB::bind_method(D_METHOD("ingest_achievement_stat", "stat_name", "amount"), &GodotEpic::ingest_achievement_stat);
	ClassDB::bind_method(D_METHOD("query_achievement_stats"), &GodotEpic::query_achievement_stats);
	ClassDB::bind_method(D_METHOD("get_achievement_stats"), &GodotEpic::get_achievement_stats);
	ClassDB::bind_method(D_METHOD("get_achievement_stat", "stat_name"), &GodotEpic::get_achievement_stat);

	// Leaderboards methods
	ClassDB::bind_method(D_METHOD("query_leaderboard_definitions"), &GodotEpic::query_leaderboard_definitions);
	ClassDB::bind_method(D_METHOD("query_leaderboard_ranks", "leaderboard_id", "limit"), &GodotEpic::query_leaderboard_ranks);
	ClassDB::bind_method(D_METHOD("query_leaderboard_user_scores", "leaderboard_id", "user_ids"), &GodotEpic::query_leaderboard_user_scores);
	ClassDB::bind_method(D_METHOD("ingest_stat", "stat_name", "value"), &GodotEpic::ingest_stat);
	ClassDB::bind_method(D_METHOD("ingest_stats", "stats"), &GodotEpic::ingest_stats);
	ClassDB::bind_method(D_METHOD("get_leaderboard_definitions"), &GodotEpic::get_leaderboard_definitions);
	ClassDB::bind_method(D_METHOD("get_leaderboard_ranks"), &GodotEpic::get_leaderboard_ranks);
	ClassDB::bind_method(D_METHOD("get_leaderboard_user_scores"), &GodotEpic::get_leaderboard_user_scores);

	// Authentication callback
	ClassDB::bind_method(D_METHOD("on_authentication_completed", "success", "user_info"), &GodotEpic::on_authentication_completed);
	ClassDB::bind_method(D_METHOD("on_logout_completed", "success"), &GodotEpic::on_logout_completed);

	// Achievements callback
	ClassDB::bind_method(D_METHOD("on_achievement_definitions_completed", "success", "definitions"), &GodotEpic::on_achievement_definitions_completed);
	ClassDB::bind_method(D_METHOD("on_player_achievements_completed", "success", "achievements"), &GodotEpic::on_player_achievements_completed);
	ClassDB::bind_method(D_METHOD("on_achievements_unlocked_completed", "success", "unlocked_achievement_ids"), &GodotEpic::on_achievements_unlocked_completed);

	// Stats callback
	ClassDB::bind_method(D_METHOD("on_achievement_stats_completed", "success", "stats"), &GodotEpic::on_achievement_stats_completed);

	// Leaderboards callback
	ClassDB::bind_method(D_METHOD("on_leaderboard_definitions_completed", "success", "definitions"), &GodotEpic::on_leaderboard_definitions_completed);
	ClassDB::bind_method(D_METHOD("on_leaderboard_ranks_completed", "success", "ranks"), &GodotEpic::on_leaderboard_ranks_completed);
	ClassDB::bind_method(D_METHOD("on_leaderboard_user_scores_completed", "success", "user_scores"), &GodotEpic::on_leaderboard_user_scores_completed);

	// Friends callbacks
	ClassDB::bind_method(D_METHOD("on_friends_query_completed", "success", "friends_list"), &GodotEpic::on_friends_query_completed);
	ClassDB::bind_method(D_METHOD("on_friend_info_query_completed", "success", "friend_info"), &GodotEpic::on_friend_info_query_completed);

	// Signals
	ADD_SIGNAL(MethodInfo("login_completed", PropertyInfo(Variant::BOOL, "success"), PropertyInfo(Variant::DICTIONARY, "user_info")));
	ADD_SIGNAL(MethodInfo("logout_completed", PropertyInfo(Variant::BOOL, "success")));
	ADD_SIGNAL(MethodInfo("friends_updated", PropertyInfo(Variant::BOOL, "success"), PropertyInfo(Variant::ARRAY, "friends_list")));
	ADD_SIGNAL(MethodInfo("friend_info_updated", PropertyInfo(Variant::DICTIONARY, "friend_info")));
	ADD_SIGNAL(MethodInfo("achievement_definitions_updated", PropertyInfo(Variant::ARRAY, "definitions")));
	ADD_SIGNAL(MethodInfo("player_achievements_updated", PropertyInfo(Variant::ARRAY, "achievements")));
	ADD_SIGNAL(MethodInfo("achievements_unlocked", PropertyInfo(Variant::ARRAY, "unlocked_achievement_ids")));
	ADD_SIGNAL(MethodInfo("achievement_unlocked", PropertyInfo(Variant::STRING, "achievement_id"), PropertyInfo(Variant::INT, "unlock_time")));
	ADD_SIGNAL(MethodInfo("achievement_stats_updated", PropertyInfo(Variant::BOOL, "success"), PropertyInfo(Variant::ARRAY, "stats")));
	ADD_SIGNAL(MethodInfo("leaderboard_definitions_updated", PropertyInfo(Variant::ARRAY, "definitions")));
	ADD_SIGNAL(MethodInfo("leaderboard_ranks_updated", PropertyInfo(Variant::ARRAY, "ranks")));
	ADD_SIGNAL(MethodInfo("leaderboard_user_scores_updated", PropertyInfo(Variant::DICTIONARY, "user_scores")));
	ADD_SIGNAL(MethodInfo("stats_ingested", PropertyInfo(Variant::ARRAY, "stat_names")));
}

GodotEpic::GodotEpic() {
	// Initialize any variables here.
	instance = this;

	// Initialize authentication state
}

GodotEpic::~GodotEpic() {
	UtilityFunctions::print("GodotEpic: Destructor called");

	// Ensure platform is shutdown on destruction
	try {
		shutdown_platform();
	} catch (...) {
		UtilityFunctions::printerr("GodotEpic: Exception during shutdown in destructor");
	}

	if (instance == this) {
		instance = nullptr;
	}

	UtilityFunctions::print("GodotEpic: Destructor completed");
}

void GodotEpic::on_logout_completed(bool success) {
	if (success) {
		UtilityFunctions::print("GodotEpic: Logout completed successfully");
	} else {
		UtilityFunctions::printerr("GodotEpic: Logout failed");
	}

	emit_signal("logout_completed", success);
}

GodotEpic* GodotEpic::get_singleton() {
	if (!instance) {
		instance = memnew(GodotEpic);
	}
	return instance;
}

void GodotEpic::cleanup_singleton() {
	if (instance) {
		UtilityFunctions::print("GodotEpic: Cleaning up singleton instance");
		// Shutdown platform first to ensure clean resource cleanup
		instance->shutdown_platform();
		// Delete the instance
		memdelete(instance);
		// The destructor will set instance = nullptr
	}
}

bool GodotEpic::initialize_platform(const Dictionary& options) {
	UtilityFunctions::print("Starting EOS Platform initialization");

	// Convert dictionary to init options
	EpicInitOptions init_options = _dict_to_init_options(options);

	// Validate options
	if (!_validate_init_options(init_options)) {
		UtilityFunctions::printerr("EOS Platform initialization failed: Invalid options");
		return false;
	}
	// Setup logging
	EOS_EResult LogResult = EOS_Logging_SetCallback(logging_callback);
	if (LogResult == EOS_EResult::EOS_Success) {
		EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, EOS_ELogLevel::EOS_LOG_Verbose);
	}

	// Initialize subsystems
	if (!initialize_subsystems(init_options)) {
		return false;
	}

	return true;
}

void GodotEpic::shutdown_platform() {
	UtilityFunctions::print("GodotEpic: Starting platform shutdown...");

	// Shutdown all subsystems - they will handle their own cleanup including logout
	SubsystemManager* manager = SubsystemManager::GetInstance();
	manager->ShutdownAll();

	UtilityFunctions::print("GodotEpic: Platform shutdown complete");
}void GodotEpic::tick(double delta) {
	SubsystemManager* manager = SubsystemManager::GetInstance();
	const double clamped_delta = delta < 0.0 ? 0.0 : delta;
	manager->TickAll(static_cast<float>(clamped_delta));
}

bool GodotEpic::is_platform_initialized() const {
	auto platform_subsystem = Get<IPlatformSubsystem>();
	return platform_subsystem && platform_subsystem->GetPlatformHandle() != nullptr;
}

EOS_HPlatform GodotEpic::get_platform_handle() const {
	auto platform_subsystem = Get<IPlatformSubsystem>();
	return platform_subsystem ? platform_subsystem->GetPlatformHandle() : nullptr;
}

// Authentication methods
void GodotEpic::login_with_epic_account(const String& email, const String& password) {
	UtilityFunctions::print("Starting Epic account login");

	auto auth = Get<IAuthenticationSubsystem>();
	if (!auth) {
		UtilityFunctions::push_warning("AuthenticationSubsystem not available");
		Dictionary empty_user_info;
		emit_signal("login_completed", false, empty_user_info);
		return;
	}

	Dictionary credentials;
	credentials["email"] = email;
	credentials["password"] = password;

	if (!auth->Login("epic_account", credentials)) {
		UtilityFunctions::printerr("AuthenticationSubsystem login failed");
		Dictionary empty_user_info;
		emit_signal("login_completed", false, empty_user_info);
	}
}

void GodotEpic::login_with_account_portal() {
	UtilityFunctions::print("Starting Account Portal login");

	auto auth = Get<IAuthenticationSubsystem>();
	if (!auth) {
		UtilityFunctions::printerr("GodotEpic::login_with_account_portal - AuthenticationSubsystem not available");
		on_authentication_completed(false, Dictionary());
		return;
	}

	Dictionary credentials; // Empty for account portal login

	if (!auth->Login("account_portal", credentials)) {
		UtilityFunctions::printerr("GodotEpic::login_with_account_portal - Login call failed");
		on_authentication_completed(false, Dictionary());
	}
}

void GodotEpic::login_with_dev(const String& display_name) {
	UtilityFunctions::print("Starting dev login");

	auto auth = Get<IAuthenticationSubsystem>();
	if (!auth) {
		UtilityFunctions::push_warning("AuthenticationSubsystem not available");
		Dictionary empty_user_info;
		emit_signal("login_completed", false, empty_user_info);
		return;
	}

	Dictionary credentials;
	credentials["id"] = "localhost:7777";
	credentials["token"] = display_name.is_empty() ? "TestUser" : display_name;

	if (!auth->Login("dev", credentials)) {
		UtilityFunctions::printerr("AuthenticationSubsystem dev login failed");
		Dictionary empty_user_info;
		emit_signal("login_completed", false, empty_user_info);
	}
}

void GodotEpic::login_with_device_id(const String& display_name) {
	UtilityFunctions::print("Starting device ID login");

	auto auth = Get<IAuthenticationSubsystem>();
	if (!auth) {
		UtilityFunctions::push_warning("AuthenticationSubsystem not available");
		Dictionary empty_user_info;
		emit_signal("login_completed", false, empty_user_info);
		return;
	}

	if (!auth->Login("device_id", Dictionary())) {
		UtilityFunctions::printerr("AuthenticationSubsystem device ID login failed");
		Dictionary empty_user_info;
		emit_signal("login_completed", false, empty_user_info);
	}
}

void GodotEpic::logout() {
	UtilityFunctions::print("Starting logout");

	auto auth = Get<IAuthenticationSubsystem>();
	if (!auth) {
		UtilityFunctions::push_warning("AuthenticationSubsystem not available");
		emit_signal("logout_completed", false);
		return;
	}

	if (!auth->Logout()) {
		UtilityFunctions::printerr("AuthenticationSubsystem logout failed");
		emit_signal("logout_completed", false);
	}
}

bool GodotEpic::is_user_logged_in() const {
	auto auth = Get<IAuthenticationSubsystem>();
	return auth ? auth->IsLoggedIn() : false;
}

String GodotEpic::get_current_username() const {
	auto auth = Get<IAuthenticationSubsystem>();
	return auth ? auth->GetDisplayName() : "";
}

String GodotEpic::get_epic_account_id() const {
	EOS_EpicAccountId epic_id = Get<IAuthenticationSubsystem>()->GetEpicAccountId();
	if (!epic_id) return "";

	return FAccountHelpers::EpicAccountIDToString(epic_id);
}

String GodotEpic::get_product_user_id() const {
	EOS_ProductUserId product_user_id = Get<IAuthenticationSubsystem>()->GetProductUserId();
	if (!EOS_ProductUserId_IsValid(product_user_id)) return "";

	return FAccountHelpers::ProductUserIDToString(product_user_id);
}

// Friends methods
void GodotEpic::query_friends() {
	auto friends = Get<IFriendsSubsystem>();
	if (!friends) {
		UtilityFunctions::push_warning("FriendsSubsystem not available");
		Array empty_friends;
		emit_signal("friends_updated", false, empty_friends);
		return;
	}

	if (!friends->QueryFriends()) {
		UtilityFunctions::printerr("FriendsSubsystem query friends failed");
		Array empty_friends;
		emit_signal("friends_updated", false, empty_friends);
	}
}

Array GodotEpic::get_friends_list() {
	auto friends = Get<IFriendsSubsystem>();
	return friends ? friends->GetFriendsList() : Array();
}

Dictionary GodotEpic::get_friend_info(const String& friend_id) {
	auto friends = Get<IFriendsSubsystem>();
	return friends ? friends->GetFriendInfo(friend_id) : Dictionary();
}

void GodotEpic::query_friend_info(const String& friend_id) {
	auto friends = Get<IFriendsSubsystem>();
	if (!friends) {
		UtilityFunctions::push_warning("FriendsSubsystem not available");
		Dictionary empty_info;
		empty_info["id"] = friend_id;
		empty_info["error"] = "Subsystem not available";
		emit_signal("friend_info_updated", empty_info);
		return;
	}

	if (!friends->QueryFriendInfo(friend_id)) {
		UtilityFunctions::printerr("FriendsSubsystem query friend info failed");
		Dictionary empty_info;
		empty_info["id"] = friend_id;
		empty_info["error"] = "Query failed";
		emit_signal("friend_info_updated", empty_info);
	}
}

void GodotEpic::query_all_friends_info() {
	auto friends = Get<IFriendsSubsystem>();
	if (!friends) {
		UtilityFunctions::push_warning("FriendsSubsystem not available");
		return;
	}

	if (!friends->QueryAllFriendsInfo()) {
		UtilityFunctions::printerr("FriendsSubsystem query all friends info failed");
	}
}

// Achievements methods
void GodotEpic::query_achievement_definitions() {
	UtilityFunctions::print("Starting achievement definitions query");

	auto achievements = Get<IAchievementsSubsystem>();
	if (!achievements) {
		UtilityFunctions::push_warning("AchievementsSubsystem not available");
		Array empty_definitions;
		emit_signal("achievement_definitions_updated", empty_definitions);
		return;
	}

	if (!achievements->QueryAchievementDefinitions()) {
		UtilityFunctions::printerr("AchievementsSubsystem query definitions failed");
		Array empty_definitions;
		emit_signal("achievement_definitions_updated", empty_definitions);
	}
}

void GodotEpic::query_player_achievements() {
	UtilityFunctions::print("GodotEpic: Starting player achievements query");

	auto achievements = Get<IAchievementsSubsystem>();
	if (!achievements) {
		UtilityFunctions::printerr("AchievementsSubsystem not available");
		Array empty_achievements;
		emit_signal("player_achievements_updated", empty_achievements);
		return;
	}

	if (!achievements->QueryPlayerAchievements()) {
		UtilityFunctions::printerr("AchievementsSubsystem query player achievements failed");
		Array empty_achievements;
		emit_signal("player_achievements_updated", empty_achievements);
	}
}

void GodotEpic::unlock_achievement(const String& achievement_id) {
	Array achievement_ids;
	achievement_ids.push_back(achievement_id);
	unlock_achievements(achievement_ids);
}

void GodotEpic::unlock_achievements(const Array& achievement_ids) {
	UtilityFunctions::print("GodotEpic: Starting achievement unlock for " + String::num_int64(achievement_ids.size()) + " achievements");

	auto achievements = Get<IAchievementsSubsystem>();
	if (!achievements) {
		UtilityFunctions::printerr("AchievementsSubsystem not available");
		Array empty_unlocked;
		emit_signal("achievements_unlocked", empty_unlocked);
		return;
	}

	if (!achievements->UnlockAchievements(achievement_ids)) {
		UtilityFunctions::printerr("AchievementsSubsystem unlock achievements failed");
		Array empty_unlocked;
		emit_signal("achievements_unlocked", empty_unlocked);
	}
}

Array GodotEpic::get_achievement_definitions() {
	auto achievements = Get<IAchievementsSubsystem>();
	return achievements ? achievements->GetAchievementDefinitions() : Array();
}

Array GodotEpic::get_player_achievements() {
	auto achievements = Get<IAchievementsSubsystem>();
	return achievements ? achievements->GetPlayerAchievements() : Array();
}

Dictionary GodotEpic::get_achievement_definition(const String& achievement_id) {
	auto achievements = Get<IAchievementsSubsystem>();
	return achievements ? achievements->GetAchievementDefinition(achievement_id) : Dictionary();
}

Dictionary GodotEpic::get_player_achievement(const String& achievement_id) {
	auto achievements = Get<IAchievementsSubsystem>();
	return achievements ? achievements->GetPlayerAchievement(achievement_id) : Dictionary();
}

// Achievement Stats methods
void GodotEpic::ingest_achievement_stat(const String& stat_name, int amount) {
	UtilityFunctions::print("GodotEpic: Starting stat ingestion: " + stat_name + " = " + String::num_int64(amount));

	auto achievements = Get<IAchievementsSubsystem>();
	if (!achievements) {
		UtilityFunctions::printerr("AchievementsSubsystem not available");
		Array empty_stats;
		emit_signal("achievement_stats_updated", false, empty_stats);
		return;
	}

	if (!achievements->IngestStat(stat_name, amount)) {
		UtilityFunctions::printerr("AchievementsSubsystem ingest stat failed");
		Array empty_stats;
		emit_signal("achievement_stats_updated", false, empty_stats);
	}
}

void GodotEpic::query_achievement_stats() {
	UtilityFunctions::print("GodotEpic: Starting achievement stats query");

	auto achievements = Get<IAchievementsSubsystem>();
	if (!achievements) {
		UtilityFunctions::printerr("AchievementsSubsystem not available");
		Array empty_stats;
		emit_signal("achievement_stats_updated", false, empty_stats);
		return;
	}

	if (!achievements->QueryStats()) {
		UtilityFunctions::printerr("AchievementsSubsystem query stats failed");
		Array empty_stats;
		emit_signal("achievement_stats_updated", false, empty_stats);
	}
}

Array GodotEpic::get_achievement_stats() {
	auto achievements = Get<IAchievementsSubsystem>();
	return achievements ? achievements->GetStats() : Array();
}

Dictionary GodotEpic::get_achievement_stat(const String& stat_name) {
	auto achievements = Get<IAchievementsSubsystem>();
	return achievements ? achievements->GetStat(stat_name) : Dictionary();
}

// Static logging callback
void EOS_CALL GodotEpic::logging_callback(const EOS_LogMessage* message) {
	if (!message || !message->Message) {
		return;
	}

	String log_text = String::utf8(message->Message);
	String category = message->Category ? String::utf8(message->Category) : "EOS";

	switch (message->Level) {
		case EOS_ELogLevel::EOS_LOG_Fatal:
		case EOS_ELogLevel::EOS_LOG_Error:
			{
				String log_msg = String("[") + category + "] " + log_text;
				UtilityFunctions::printerr(log_msg);
			}
			break;
		case EOS_ELogLevel::EOS_LOG_Warning:
			{
				String log_msg = String("[") + category + "] " + log_text;
				WARN_PRINT(log_msg);
			}
			break;
		case EOS_ELogLevel::EOS_LOG_Info:
		case EOS_ELogLevel::EOS_LOG_Verbose:
		case EOS_ELogLevel::EOS_LOG_VeryVerbose:
		default:
			{
				String log_msg = String("[") + category + "] " + log_text;
				UtilityFunctions::printerr(log_msg);
			}
			break;
	}
}

// Leaderboards methods
void GodotEpic::query_leaderboard_definitions() {
	UtilityFunctions::print("GodotEpic: Starting leaderboard definitions query");

	auto leaderboards = Get<ILeaderboardsSubsystem>();
	if (!leaderboards) {
		UtilityFunctions::printerr("LeaderboardsSubsystem not available");
		Array empty_definitions;
		emit_signal("leaderboard_definitions_updated", empty_definitions);
		return;
	}

	if (!leaderboards->QueryLeaderboardDefinitions()) {
		UtilityFunctions::printerr("LeaderboardsSubsystem query definitions failed");
		Array empty_definitions;
		emit_signal("leaderboard_definitions_updated", empty_definitions);
	}
}

void GodotEpic::query_leaderboard_ranks(const String& leaderboard_id, int limit) {
	UtilityFunctions::print("GodotEpic: Starting leaderboard ranks query for: " + leaderboard_id + " (limit: " + String::num_int64(limit) + ")");

	auto leaderboards = Get<ILeaderboardsSubsystem>();
	if (!leaderboards) {
		UtilityFunctions::printerr("LeaderboardsSubsystem not available");
		Array empty_ranks;
		emit_signal("leaderboard_ranks_updated", empty_ranks);
		return;
	}

	if (!leaderboards->QueryLeaderboardRanks(leaderboard_id, limit)) {
		UtilityFunctions::printerr("LeaderboardsSubsystem query ranks failed");
		Array empty_ranks;
		emit_signal("leaderboard_ranks_updated", empty_ranks);
	}
}

void GodotEpic::query_leaderboard_user_scores(const String& leaderboard_id, const Array& user_ids) {
	UtilityFunctions::print("GodotEpic: Starting leaderboard user scores query for: " + leaderboard_id + " (" + String::num_int64(user_ids.size()) + " users)");

	auto leaderboards = Get<ILeaderboardsSubsystem>();
	if (!leaderboards) {
		UtilityFunctions::printerr("LeaderboardsSubsystem not available");
		Dictionary empty_scores;
		emit_signal("leaderboard_user_scores_updated", empty_scores);
		return;
	}

	if (!leaderboards->QueryLeaderboardUserScores(leaderboard_id, user_ids)) {
		UtilityFunctions::printerr("LeaderboardsSubsystem query user scores failed");
		Dictionary empty_scores;
		emit_signal("leaderboard_user_scores_updated", empty_scores);
	}
}

void GodotEpic::ingest_stat(const String& stat_name, int value) {
	UtilityFunctions::print("GodotEpic: Starting stat ingestion: " + stat_name + " = " + String::num_int64(value));

	auto achievements = Get<IAchievementsSubsystem>();
	if (!achievements) {
		UtilityFunctions::printerr("AchievementsSubsystem not available");
		Array empty_stats;
		emit_signal("stats_ingested", empty_stats);
		return;
	}

	if (!achievements->IngestStat(stat_name, value)) {
		UtilityFunctions::printerr("AchievementsSubsystem ingest stat failed");
		Array empty_stats;
		emit_signal("stats_ingested", empty_stats);
	}
}

void GodotEpic::ingest_stats(const Dictionary& stats) {
	UtilityFunctions::print("GodotEpic: Starting bulk stat ingestion for " + String::num_int64(stats.size()) + " stats");

	auto achievements = Get<IAchievementsSubsystem>();
	if (!achievements) {
		UtilityFunctions::printerr("AchievementsSubsystem not available");
		Array empty_stats;
		emit_signal("stats_ingested", empty_stats);
		return;
	}

	// For multiple stats, call IngestStat for each
	Array keys = stats.keys();
	for (int i = 0; i < keys.size(); i++) {
		String stat_name = keys[i];
		Variant stat_value = stats[stat_name];
		if (stat_value.get_type() == Variant::INT) {
			if (!achievements->IngestStat(stat_name, (int)stat_value)) {
				UtilityFunctions::printerr("AchievementsSubsystem ingest stat failed for: " + stat_name);
			}
		}
	}
	// Emit signal with the stat names that were ingested
	emit_signal("stats_ingested", keys);
}

Array GodotEpic::get_leaderboard_definitions() {
	auto leaderboards = Get<ILeaderboardsSubsystem>();
	return leaderboards ? leaderboards->GetLeaderboardDefinitions() : Array();
}

Array GodotEpic::get_leaderboard_ranks() {
	auto leaderboards = Get<ILeaderboardsSubsystem>();
	return leaderboards ? leaderboards->GetLeaderboardRanks() : Array();
}

Dictionary GodotEpic::get_leaderboard_user_scores() {
	auto leaderboards = Get<ILeaderboardsSubsystem>();
	return leaderboards ? leaderboards->GetLeaderboardUserScores() : Dictionary();
}

// Helper methods
EpicInitOptions GodotEpic::_dict_to_init_options(const Dictionary& options_dict) {
	EpicInitOptions options;

	// Use values from dictionary if provided, otherwise use SampleConstants as fallback
	if (options_dict.has("product_name")) {
		options.product_name = options_dict["product_name"];
	} else {
		options.product_name = String();
	}

	if (options_dict.has("product_version")) {
		options.product_version = options_dict["product_version"];
	}
	// product_version keeps default "1.0.0"

	if (options_dict.has("product_id")) {
		options.product_id = options_dict["product_id"];
	} else {
		options.product_id = String();
	}

	if (options_dict.has("sandbox_id")) {
		options.sandbox_id = options_dict["sandbox_id"];
	} else {
		options.sandbox_id = String();
	}

	if (options_dict.has("deployment_id")) {
		options.deployment_id = options_dict["deployment_id"];
	} else {
		options.deployment_id = String();
	}

	if (options_dict.has("client_id")) {
		options.client_id = options_dict["client_id"];
	} else {
		options.client_id = String();
	}

	if (options_dict.has("client_secret")) {
		options.client_secret = options_dict["client_secret"];
	} else {
		options.client_secret = String();
	}

	if (options_dict.has("encryption_key")) {
		options.encryption_key = options_dict["encryption_key"];
	} else {
		options.encryption_key = String();
	}

	return options;
}

bool GodotEpic::_validate_init_options(const EpicInitOptions& options) {
	Array required_fields;
	required_fields.append("product_id");
	required_fields.append("sandbox_id");
	required_fields.append("deployment_id");
	required_fields.append("client_id");
	required_fields.append("client_secret");

	bool valid = true;

	if (options.product_id.is_empty()) {
		UtilityFunctions::printerr("Missing required initialization option: product_id");
		valid = false;
	}
	if (options.sandbox_id.is_empty()) {
		UtilityFunctions::printerr("Missing required initialization option: sandbox_id");
		valid = false;
	}
	if (options.deployment_id.is_empty()) {
		UtilityFunctions::printerr("Missing required initialization option: deployment_id");
		valid = false;
	}
	if (options.client_id.is_empty()) {
		UtilityFunctions::printerr("Missing required initialization option: client_id");
		valid = false;
	}
	if (options.client_secret.is_empty()) {
		UtilityFunctions::printerr("Missing required initialization option: client_secret");
		valid = false;
	}

	if (options.encryption_key.is_empty()) {
		WARN_PRINT("Encryption key not set - data will not be encrypted");
	}

	return valid;
}

bool GodotEpic::initialize_subsystems(const EpicInitOptions& init_options) {
	// Check for reinitialization
	SubsystemManager* manager = SubsystemManager::GetInstance();
	if (manager->IsHealthy()) {
		UtilityFunctions::print("EOS Platform already initialized and healthy - skipping reinitialization");
		return true;
	}

	// Register subsystems with SubsystemManager
	// Register PlatformSubsystem first (needed by other subsystems)
	manager->RegisterSubsystem<IPlatformSubsystem, PlatformSubsystem>("PlatformSubsystem");

	// Register UserInfoSubsystem early (needed by Authentication and Friends)
	manager->RegisterSubsystem<IUserInfoSubsystem, UserInfoSubsystem>("UserInfoSubsystem");

	// Register other subsystems
	manager->RegisterSubsystem<IAuthenticationSubsystem, AuthenticationSubsystem>("AuthenticationSubsystem");
	manager->RegisterSubsystem<IAchievementsSubsystem, AchievementsSubsystem>("AchievementsSubsystem");
	manager->RegisterSubsystem<ILeaderboardsSubsystem, LeaderboardsSubsystem>("LeaderboardsSubsystem");
	manager->RegisterSubsystem<IFriendsSubsystem, FriendsSubsystem>("FriendsSubsystem");

	// Initialize PlatformSubsystem with EpicInitOptions
	auto platform_subsystem = manager->GetSubsystem<IPlatformSubsystem>();
	if (!platform_subsystem) {
		UtilityFunctions::printerr("Failed to get PlatformSubsystem");
		return false;
	}

	if (!platform_subsystem->InitializePlatform(init_options)) {
		UtilityFunctions::printerr("PlatformSubsystem initialization failed");
		return false;
	}

	// Initialize all subsystems
	if (!manager->InitializeAll()) {
		UtilityFunctions::printerr("Failed to initialize subsystems");
		return false;
	}

	// Set up authentication callback
	setup_authentication_callback();

	// Set up achievements callbacks
	setup_achievements_callbacks();

	// Set up leaderboards callbacks
	setup_leaderboards_callbacks();

	// Set up friends callbacks
	setup_friends_callbacks();

	return true;
}

void GodotEpic::setup_authentication_callback() {
	auto auth = Get<IAuthenticationSubsystem>();
	if (auth) {
		Callable login_callback = Callable(this, "on_authentication_completed");
		auth->SetLoginCallback(login_callback);

		Callable logout_callback = Callable(this, "on_logout_completed");
		auth->SetLogoutCallback(logout_callback);
	} else {
		UtilityFunctions::printerr("Failed to set up authentication callback - AuthenticationSubsystem not available");
	}
}

void GodotEpic::setup_achievements_callbacks() {
	auto achievements = Get<IAchievementsSubsystem>();
	if (achievements) {
		// Create callables that bind to our instance methods
		Callable definitions_callback(this, "on_achievement_definitions_completed");
		achievements->SetAchievementDefinitionsCallback(definitions_callback);

		Callable player_callback(this, "on_player_achievements_completed");
		achievements->SetPlayerAchievementsCallback(player_callback);

		Callable unlocked_callback(this, "on_achievements_unlocked_completed");
		achievements->SetAchievementsUnlockedCallback(unlocked_callback);

		Callable stats_callback(this, "on_achievement_stats_completed");
		achievements->SetStatsCallback(stats_callback);
	} else {
		UtilityFunctions::printerr("Failed to set up achievements callbacks - AchievementsSubsystem not available");
	}
}

void GodotEpic::setup_leaderboards_callbacks() {
	auto leaderboards = Get<ILeaderboardsSubsystem>();
	if (leaderboards) {
		// Create callables that bind to our instance methods
		Callable definitions_callback(this, "on_leaderboard_definitions_completed");
		leaderboards->SetLeaderboardDefinitionsCallback(definitions_callback);

		Callable ranks_callback(this, "on_leaderboard_ranks_completed");
		leaderboards->SetLeaderboardRanksCallback(ranks_callback);

		Callable user_scores_callback(this, "on_leaderboard_user_scores_completed");
		leaderboards->SetLeaderboardUserScoresCallback(user_scores_callback);
	} else {
		UtilityFunctions::printerr("Failed to set up leaderboards callbacks - LeaderboardsSubsystem not available");
	}
}

void GodotEpic::setup_friends_callbacks() {
	auto friends = Get<IFriendsSubsystem>();
	if (friends) {
		// Create callables that bind to our instance methods
		Callable friends_query_callback(this, "on_friends_query_completed");
		friends->SetFriendsQueryCallback(friends_query_callback);

		Callable friend_info_callback(this, "on_friend_info_query_completed");
		friends->SetFriendInfoQueryCallback(friend_info_callback);
	} else {
		UtilityFunctions::printerr("Failed to set up friends callbacks - FriendsSubsystem not available");
	}
}

void GodotEpic::on_authentication_completed(bool success, const Dictionary& user_info) {
	UtilityFunctions::printerr("GodotEpic: Authentication completed - success: " + String(success ? "true" : "false"));

	if (success) {
		String display_name = user_info.get("display_name", "Unknown User");
		String epic_account_id = user_info.get("epic_account_id", "");
		String product_user_id = user_info.get("product_user_id", "");

		UtilityFunctions::printerr("GodotEpic: Login successful for user: " + display_name);
		UtilityFunctions::printerr("GodotEpic: Epic Account ID: " + epic_account_id);
		UtilityFunctions::printerr("GodotEpic: Product User ID: " + product_user_id);
	} else {
		UtilityFunctions::printerr("GodotEpic: Login failed");
	}

	// Emit the login_completed signal
	emit_signal("login_completed", success, user_info);
}

void GodotEpic::on_achievement_definitions_completed(bool success, const Array& definitions) {
	UtilityFunctions::printerr("GodotEpic: Achievement definitions query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		UtilityFunctions::printerr("GodotEpic: Achievement definitions updated (" + String::num_int64(definitions.size()) + " definitions)");
	} else {
		UtilityFunctions::printerr("GodotEpic: Achievement definitions query failed");
	}

	// Emit the achievement_definitions_updated signal
	emit_signal("achievement_definitions_updated", definitions);
}

void GodotEpic::on_player_achievements_completed(bool success, const Array& achievements) {
	UtilityFunctions::printerr("GodotEpic: Player achievements query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		UtilityFunctions::printerr("GodotEpic: Player achievements updated (" + String::num_int64(achievements.size()) + " achievements)");
	} else {
		UtilityFunctions::printerr("GodotEpic: Player achievements query failed");
	}

	// Emit the player_achievements_updated signal
	emit_signal("player_achievements_updated", achievements);
}

void GodotEpic::on_achievements_unlocked_completed(bool success, const Array& unlocked_achievement_ids) {
	UtilityFunctions::printerr("GodotEpic: Achievements unlock completed - success: " + String(success ? "true" : "false"));

	if (success) {
		UtilityFunctions::printerr("GodotEpic: Achievements unlocked successfully");
	} else {
		UtilityFunctions::printerr("GodotEpic: Achievements unlock failed");
	}

	// Emit the achievements_unlocked signal
	emit_signal("achievements_unlocked", unlocked_achievement_ids);
}

void GodotEpic::on_achievement_stats_completed(bool success, const Array& stats) {
	UtilityFunctions::printerr("GodotEpic: Achievement stats query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		UtilityFunctions::printerr("GodotEpic: Achievement stats updated (" + String::num_int64(stats.size()) + " stats)");
	} else {
		UtilityFunctions::printerr("GodotEpic: Achievement stats query failed");
	}

	// Emit the achievement_stats_updated signal
	emit_signal("achievement_stats_updated", success, stats);
}

void GodotEpic::on_leaderboard_definitions_completed(bool success, const Array& definitions) {
	UtilityFunctions::printerr("GodotEpic: Leaderboard definitions query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		UtilityFunctions::printerr("GodotEpic: Leaderboard definitions updated (" + String::num_int64(definitions.size()) + " definitions)");
	} else {
		UtilityFunctions::printerr("GodotEpic: Leaderboard definitions query failed");
	}

	// Emit the leaderboard_definitions_updated signal
	emit_signal("leaderboard_definitions_updated", definitions);
}

void GodotEpic::on_leaderboard_ranks_completed(bool success, const Array& ranks) {
	UtilityFunctions::printerr("GodotEpic: Leaderboard ranks query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		UtilityFunctions::printerr("GodotEpic: Leaderboard ranks updated (" + String::num_int64(ranks.size()) + " ranks)");
	} else {
		UtilityFunctions::printerr("GodotEpic: Leaderboard ranks query failed");
	}

	// Emit the leaderboard_ranks_updated signal
	emit_signal("leaderboard_ranks_updated", ranks);
}

void GodotEpic::on_leaderboard_user_scores_completed(bool success, const Dictionary& user_scores) {
	UtilityFunctions::printerr("GodotEpic: Leaderboard user scores query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		UtilityFunctions::printerr("GodotEpic: Leaderboard user scores updated (" + String::num_int64(user_scores.size()) + " user scores)");
	} else {
		UtilityFunctions::printerr("GodotEpic: Leaderboard user scores query failed");
	}

	// Emit the leaderboard_user_scores_updated signal
	emit_signal("leaderboard_user_scores_updated", user_scores);
}

void GodotEpic::on_friends_query_completed(bool success, const Array& friends_list) {
	UtilityFunctions::printerr("GodotEpic: Friends query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		UtilityFunctions::printerr("GodotEpic: Friends list updated (" + String::num_int64(friends_list.size()) + " friends)");
	} else {
		UtilityFunctions::printerr("GodotEpic: Friends query failed");
	}

	// Emit the friends_updated signal
	emit_signal("friends_updated", success, friends_list);
}

void GodotEpic::on_friend_info_query_completed(bool success, const Dictionary& friend_info) {
	UtilityFunctions::printerr("GodotEpic: Friend info query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		String friend_id = friend_info.get("id", "unknown");
		UtilityFunctions::printerr("GodotEpic: Friend info updated for: " + friend_id);
	} else {
		UtilityFunctions::printerr("GodotEpic: Friend info query failed");
	}

	// Emit the friend_info_updated signal
	emit_signal("friend_info_updated", friend_info);
}
