#include "godotepic.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "Utils/SubsystemManager.h"
#include "Authentication/IAuthenticationSubsystem.h"
#include "Achievements/IAchievementsSubsystem.h"
#include "Leaderboards/ILeaderboardsSubsystem.h"
#include "Friends/IFriendsSubsystem.h"
#include "UserInfo/IUserInfoSubsystem.h"
#include "Platform/PlatformSubsystem.h"
#include "Authentication/AuthenticationSubsystem.h"
#include "Achievements/AchievementsSubsystem.h"
#include "Leaderboards/LeaderboardsSubsystem.h"
#include "Friends/FriendsSubsystem.h"
#include "UserInfo/UserInfoSubsystem.h"
#include "Utils/AccountHelpers.h"

using namespace godot;

// Static member definitions
GodotEOS* GodotEOS::instance = nullptr;

void GodotEOS::_bind_methods() {
	ClassDB::bind_static_method("GodotEOS", D_METHOD("get_singleton"), &GodotEOS::get_singleton);
	ClassDB::bind_method(D_METHOD("initialize_platform", "options"), &GodotEOS::initialize_platform);
	ClassDB::bind_method(D_METHOD("shutdown_platform"), &GodotEOS::shutdown_platform);
	ClassDB::bind_method(D_METHOD("tick", "delta"), &GodotEOS::tick);
	ClassDB::bind_method(D_METHOD("is_platform_initialized"), &GodotEOS::is_platform_initialized);

	// Authentication methods
	ClassDB::bind_method(D_METHOD("login_with_epic_account", "email", "password"), &GodotEOS::login_with_epic_account);
	ClassDB::bind_method(D_METHOD("login_with_account_portal"), &GodotEOS::login_with_account_portal);
	ClassDB::bind_method(D_METHOD("login_with_device_id", "display_name"), &GodotEOS::login_with_device_id);
	ClassDB::bind_method(D_METHOD("login_with_dev", "display_name"), &GodotEOS::login_with_dev);
	ClassDB::bind_method(D_METHOD("logout"), &GodotEOS::logout);
	ClassDB::bind_method(D_METHOD("is_user_logged_in"), &GodotEOS::is_user_logged_in);
	ClassDB::bind_method(D_METHOD("get_current_username"), &GodotEOS::get_current_username);
	ClassDB::bind_method(D_METHOD("get_epic_account_id"), &GodotEOS::get_epic_account_id);
	ClassDB::bind_method(D_METHOD("get_product_user_id"), &GodotEOS::get_product_user_id);

	// Friends methods
	ClassDB::bind_method(D_METHOD("query_friends"), &GodotEOS::query_friends);
	ClassDB::bind_method(D_METHOD("get_friends_list"), &GodotEOS::get_friends_list);
	ClassDB::bind_method(D_METHOD("get_friend_info", "friend_id"), &GodotEOS::get_friend_info);
	ClassDB::bind_method(D_METHOD("query_friend_info", "friend_id"), &GodotEOS::query_friend_info);
	ClassDB::bind_method(D_METHOD("query_all_friends_info"), &GodotEOS::query_all_friends_info);

	// Achievements methods
	ClassDB::bind_method(D_METHOD("query_achievement_definitions"), &GodotEOS::query_achievement_definitions);
	ClassDB::bind_method(D_METHOD("query_player_achievements"), &GodotEOS::query_player_achievements);
	ClassDB::bind_method(D_METHOD("unlock_achievement", "achievement_id"), &GodotEOS::unlock_achievement);
	ClassDB::bind_method(D_METHOD("unlock_achievements", "achievement_ids"), &GodotEOS::unlock_achievements);
	ClassDB::bind_method(D_METHOD("get_achievement_definitions"), &GodotEOS::get_achievement_definitions);
	ClassDB::bind_method(D_METHOD("get_player_achievements"), &GodotEOS::get_player_achievements);
	ClassDB::bind_method(D_METHOD("get_achievement_definition", "achievement_id"), &GodotEOS::get_achievement_definition);
	ClassDB::bind_method(D_METHOD("get_player_achievement", "achievement_id"), &GodotEOS::get_player_achievement);

	// Stats methods (AchievementsSubsystem)
	ClassDB::bind_method(D_METHOD("ingest_achievement_stat", "stat_name", "amount"), &GodotEOS::ingest_achievement_stat);
	ClassDB::bind_method(D_METHOD("query_achievement_stats"), &GodotEOS::query_achievement_stats);
	ClassDB::bind_method(D_METHOD("get_achievement_stats"), &GodotEOS::get_achievement_stats);
	ClassDB::bind_method(D_METHOD("get_achievement_stat", "stat_name"), &GodotEOS::get_achievement_stat);

	// Leaderboards methods
	ClassDB::bind_method(D_METHOD("query_leaderboard_definitions"), &GodotEOS::query_leaderboard_definitions);
	ClassDB::bind_method(D_METHOD("query_leaderboard_ranks", "leaderboard_id", "limit"), &GodotEOS::query_leaderboard_ranks);
	ClassDB::bind_method(D_METHOD("query_leaderboard_user_scores", "leaderboard_id", "user_ids"), &GodotEOS::query_leaderboard_user_scores);
	ClassDB::bind_method(D_METHOD("ingest_stat", "stat_name", "value"), &GodotEOS::ingest_stat);
	ClassDB::bind_method(D_METHOD("ingest_stats", "stats"), &GodotEOS::ingest_stats);
	ClassDB::bind_method(D_METHOD("get_leaderboard_definitions"), &GodotEOS::get_leaderboard_definitions);
	ClassDB::bind_method(D_METHOD("get_leaderboard_ranks"), &GodotEOS::get_leaderboard_ranks);
	ClassDB::bind_method(D_METHOD("get_leaderboard_user_scores"), &GodotEOS::get_leaderboard_user_scores);

	// Authentication callback
	ClassDB::bind_method(D_METHOD("on_authentication_completed", "success", "user_info"), &GodotEOS::on_authentication_completed);
	ClassDB::bind_method(D_METHOD("on_logout_completed", "success"), &GodotEOS::on_logout_completed);

	// Achievements callback
	ClassDB::bind_method(D_METHOD("on_achievement_definitions_completed", "success", "definitions"), &GodotEOS::on_achievement_definitions_completed);
	ClassDB::bind_method(D_METHOD("on_player_achievements_completed", "success", "achievements"), &GodotEOS::on_player_achievements_completed);
	ClassDB::bind_method(D_METHOD("on_achievements_unlocked_completed", "success", "unlocked_achievement_ids"), &GodotEOS::on_achievements_unlocked_completed);

	// Stats callback
	ClassDB::bind_method(D_METHOD("on_achievement_stats_completed", "success", "stats"), &GodotEOS::on_achievement_stats_completed);

	// Leaderboards callback
	ClassDB::bind_method(D_METHOD("on_leaderboard_definitions_completed", "success", "definitions"), &GodotEOS::on_leaderboard_definitions_completed);
	ClassDB::bind_method(D_METHOD("on_leaderboard_ranks_completed", "success", "ranks"), &GodotEOS::on_leaderboard_ranks_completed);
	ClassDB::bind_method(D_METHOD("on_leaderboard_user_scores_completed", "success", "user_scores"), &GodotEOS::on_leaderboard_user_scores_completed);

	// Friends callbacks
	ClassDB::bind_method(D_METHOD("on_friends_query_completed", "success", "friends_list"), &GodotEOS::on_friends_query_completed);
	ClassDB::bind_method(D_METHOD("on_friend_info_query_completed", "success", "friend_info"), &GodotEOS::on_friend_info_query_completed);

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
	ADD_SIGNAL(MethodInfo("leaderboard_definitions_updated", PropertyInfo(Variant::BOOL, "success"), PropertyInfo(Variant::ARRAY, "definitions")));
	ADD_SIGNAL(MethodInfo("leaderboard_ranks_updated", PropertyInfo(Variant::BOOL, "success"), PropertyInfo(Variant::ARRAY, "ranks")));
	ADD_SIGNAL(MethodInfo("leaderboard_user_scores_updated", PropertyInfo(Variant::BOOL, "success"), PropertyInfo(Variant::DICTIONARY, "user_scores")));
	ADD_SIGNAL(MethodInfo("stats_ingested", PropertyInfo(Variant::ARRAY, "stat_names")));
}

GodotEOS::GodotEOS() {
	// Initialize any variables here.
	instance = this;

	// Initialize authentication state
}

GodotEOS::~GodotEOS() {
	UtilityFunctions::print("GodotEOS: Destructor called");

	// Ensure platform is shutdown on destruction
	try {
		shutdown_platform();
	} catch (...) {
		UtilityFunctions::printerr("GodotEOS: Exception during shutdown in destructor");
	}

	if (instance == this) {
		instance = nullptr;
	}

	UtilityFunctions::print("GodotEOS: Destructor completed");
}

void GodotEOS::on_logout_completed(bool success) {
	if (success) {
		UtilityFunctions::print("GodotEOS: Logout completed successfully");
	} else {
		UtilityFunctions::printerr("GodotEOS: Logout failed");
	}

	emit_signal("logout_completed", success);
}

GodotEOS* GodotEOS::get_singleton() {
	if (!instance) {
		instance = memnew(GodotEOS);
	}
	return instance;
}

void GodotEOS::cleanup_singleton() {
	if (instance) {
		UtilityFunctions::print("GodotEOS: Cleaning up singleton instance");
		// Shutdown platform first to ensure clean resource cleanup
		instance->shutdown_platform();
		// Delete the instance
		memdelete(instance);
		// The destructor will set instance = nullptr
	}
}

bool GodotEOS::initialize_platform(const Dictionary& options) {
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

void GodotEOS::shutdown_platform() {
	UtilityFunctions::print("GodotEOS: Starting platform shutdown...");

	// Shutdown all subsystems - they will handle their own cleanup including logout
	SubsystemManager* manager = SubsystemManager::GetInstance();
	manager->ShutdownAll();

	UtilityFunctions::print("GodotEOS: Platform shutdown complete");
}void GodotEOS::tick(double delta) {
	SubsystemManager* manager = SubsystemManager::GetInstance();
	const double clamped_delta = delta < 0.0 ? 0.0 : delta;
	manager->TickAll(static_cast<float>(clamped_delta));
}

bool GodotEOS::is_platform_initialized() const {
	auto platform_subsystem = Get<IPlatformSubsystem>();
	return platform_subsystem && platform_subsystem->GetPlatformHandle() != nullptr;
}

EOS_HPlatform GodotEOS::get_platform_handle() const {
	auto platform_subsystem = Get<IPlatformSubsystem>();
	return platform_subsystem ? platform_subsystem->GetPlatformHandle() : nullptr;
}

// Authentication methods
void GodotEOS::login_with_epic_account(const String& email, const String& password) {
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

void GodotEOS::login_with_account_portal() {
	UtilityFunctions::print("Starting Account Portal login");

	auto auth = Get<IAuthenticationSubsystem>();
	if (!auth) {
		UtilityFunctions::printerr("GodotEOS::login_with_account_portal - AuthenticationSubsystem not available");
		on_authentication_completed(false, Dictionary());
		return;
	}

	Dictionary credentials; // Empty for account portal login

	if (!auth->Login("account_portal", credentials)) {
		UtilityFunctions::printerr("GodotEOS::login_with_account_portal - Login call failed");
		on_authentication_completed(false, Dictionary());
	}
}

void GodotEOS::login_with_dev(const String& display_name) {
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

void GodotEOS::login_with_device_id(const String& display_name) {
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

void GodotEOS::logout() {
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

bool GodotEOS::is_user_logged_in() const {
	auto auth = Get<IAuthenticationSubsystem>();
	return auth ? auth->IsLoggedIn() : false;
}

String GodotEOS::get_current_username() const {
	auto auth = Get<IAuthenticationSubsystem>();
	return auth ? auth->GetDisplayName() : "";
}

String GodotEOS::get_epic_account_id() const {
	auto auth = Get<IAuthenticationSubsystem>();
	if (!auth) {
		UtilityFunctions::push_warning("AuthenticationSubsystem not available");
		return "";
	}

	EOS_EpicAccountId epic_id = auth->GetEpicAccountId();
	if (!epic_id) return "";

	return FAccountHelpers::EpicAccountIDToString(epic_id);
}

String GodotEOS::get_product_user_id() const {
	auto auth = Get<IAuthenticationSubsystem>();
	if (!auth) {
		UtilityFunctions::push_warning("AuthenticationSubsystem not available");
		return "";
	}

	EOS_ProductUserId product_user_id = auth->GetProductUserId();
	if (!EOS_ProductUserId_IsValid(product_user_id)) return "";

	return FAccountHelpers::ProductUserIDToString(product_user_id);
}

// Friends methods
void GodotEOS::query_friends() {
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

Array GodotEOS::get_friends_list() {
	auto friends = Get<IFriendsSubsystem>();
	return friends ? friends->GetFriendsList() : Array();
}

Dictionary GodotEOS::get_friend_info(const String& friend_id) {
	auto friends = Get<IFriendsSubsystem>();
	return friends ? friends->GetFriendInfo(friend_id) : Dictionary();
}

void GodotEOS::query_friend_info(const String& friend_id) {
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

void GodotEOS::query_all_friends_info() {
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
void GodotEOS::query_achievement_definitions() {
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

void GodotEOS::query_player_achievements() {
	UtilityFunctions::print("GodotEOS: Starting player achievements query");

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

void GodotEOS::unlock_achievement(const String& achievement_id) {
	Array achievement_ids;
	achievement_ids.push_back(achievement_id);
	unlock_achievements(achievement_ids);
}

void GodotEOS::unlock_achievements(const Array& achievement_ids) {
	UtilityFunctions::print("GodotEOS: Starting achievement unlock for " + String::num_int64(achievement_ids.size()) + " achievements");

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

Array GodotEOS::get_achievement_definitions() {
	auto achievements = Get<IAchievementsSubsystem>();
	return achievements ? achievements->GetAchievementDefinitions() : Array();
}

Array GodotEOS::get_player_achievements() {
	auto achievements = Get<IAchievementsSubsystem>();
	return achievements ? achievements->GetPlayerAchievements() : Array();
}

Dictionary GodotEOS::get_achievement_definition(const String& achievement_id) {
	auto achievements = Get<IAchievementsSubsystem>();
	return achievements ? achievements->GetAchievementDefinition(achievement_id) : Dictionary();
}

Dictionary GodotEOS::get_player_achievement(const String& achievement_id) {
	auto achievements = Get<IAchievementsSubsystem>();
	return achievements ? achievements->GetPlayerAchievement(achievement_id) : Dictionary();
}

// Achievement Stats methods
void GodotEOS::ingest_achievement_stat(const String& stat_name, int amount) {
	UtilityFunctions::print("GodotEOS: Starting stat ingestion: " + stat_name + " = " + String::num_int64(amount));

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

void GodotEOS::query_achievement_stats() {
	UtilityFunctions::print("GodotEOS: Starting achievement stats query");

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

Array GodotEOS::get_achievement_stats() {
	auto achievements = Get<IAchievementsSubsystem>();
	return achievements ? achievements->GetStats() : Array();
}

Dictionary GodotEOS::get_achievement_stat(const String& stat_name) {
	auto achievements = Get<IAchievementsSubsystem>();
	return achievements ? achievements->GetStat(stat_name) : Dictionary();
}

// Static logging callback
void EOS_CALL GodotEOS::logging_callback(const EOS_LogMessage* message) {
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
				UtilityFunctions::print(log_msg);
			}
			break;
	}
}

// Leaderboards methods
void GodotEOS::query_leaderboard_definitions() {
	UtilityFunctions::print("GodotEOS: Starting leaderboard definitions query");

	auto leaderboards = Get<ILeaderboardsSubsystem>();
	if (!leaderboards) {
		UtilityFunctions::printerr("LeaderboardsSubsystem not available");
		Array empty_definitions;
		emit_signal("leaderboard_definitions_updated", false, empty_definitions);
		return;
	}

	if (!leaderboards->QueryLeaderboardDefinitions()) {
		UtilityFunctions::printerr("LeaderboardsSubsystem query definitions failed");
		Array empty_definitions;
		emit_signal("leaderboard_definitions_updated", false, empty_definitions);
	}
}

void GodotEOS::query_leaderboard_ranks(const String& leaderboard_id, int limit) {
	UtilityFunctions::print("GodotEOS: Starting leaderboard ranks query for: " + leaderboard_id + " (limit: " + String::num_int64(limit) + ")");

	auto leaderboards = Get<ILeaderboardsSubsystem>();
	if (!leaderboards) {
		UtilityFunctions::printerr("LeaderboardsSubsystem not available");
		Array empty_ranks;
		emit_signal("leaderboard_ranks_updated", false, empty_ranks);
		return;
	}

	if (!leaderboards->QueryLeaderboardRanks(leaderboard_id, limit)) {
		UtilityFunctions::printerr("LeaderboardsSubsystem query ranks failed");
		Array empty_ranks;
		emit_signal("leaderboard_ranks_updated", false, empty_ranks);
	}
}

void GodotEOS::query_leaderboard_user_scores(const String& leaderboard_id, const Array& user_ids) {
	UtilityFunctions::print("GodotEOS: Starting leaderboard user scores query for: " + leaderboard_id + " (" + String::num_int64(user_ids.size()) + " users)");

	auto leaderboards = Get<ILeaderboardsSubsystem>();
	if (!leaderboards) {
		UtilityFunctions::printerr("LeaderboardsSubsystem not available");
		Dictionary empty_scores;
		emit_signal("leaderboard_user_scores_updated", false, empty_scores);
		return;
	}

	if (!leaderboards->QueryLeaderboardUserScores(leaderboard_id, user_ids)) {
		UtilityFunctions::printerr("LeaderboardsSubsystem query user scores failed");
		Dictionary empty_scores;
		emit_signal("leaderboard_user_scores_updated", false, empty_scores);
	}
}

void GodotEOS::ingest_stat(const String& stat_name, int value) {
	UtilityFunctions::print("GodotEOS: Starting stat ingestion: " + stat_name + " = " + String::num_int64(value));

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

void GodotEOS::ingest_stats(const Dictionary& stats) {
	UtilityFunctions::print("GodotEOS: Starting bulk stat ingestion for " + String::num_int64(stats.size()) + " stats");

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

Array GodotEOS::get_leaderboard_definitions() {
	auto leaderboards = Get<ILeaderboardsSubsystem>();
	return leaderboards ? leaderboards->GetLeaderboardDefinitions() : Array();
}

Array GodotEOS::get_leaderboard_ranks() {
	auto leaderboards = Get<ILeaderboardsSubsystem>();
	return leaderboards ? leaderboards->GetLeaderboardRanks() : Array();
}

Dictionary GodotEOS::get_leaderboard_user_scores() {
	auto leaderboards = Get<ILeaderboardsSubsystem>();
	return leaderboards ? leaderboards->GetLeaderboardUserScores() : Dictionary();
}

// Helper methods
EpicInitOptions GodotEOS::_dict_to_init_options(const Dictionary& options_dict) {
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

bool GodotEOS::_validate_init_options(const EpicInitOptions& options) {
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

bool GodotEOS::initialize_subsystems(const EpicInitOptions& init_options) {
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

void GodotEOS::setup_authentication_callback() {
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

void GodotEOS::setup_achievements_callbacks() {
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

void GodotEOS::setup_leaderboards_callbacks() {
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

void GodotEOS::setup_friends_callbacks() {
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

void GodotEOS::on_authentication_completed(bool success, const Dictionary& user_info) {
	UtilityFunctions::print("GodotEOS: Authentication completed - success: " + String(success ? "true" : "false"));

	if (success) {
		String display_name = user_info.get("display_name", "Unknown User");
		String epic_account_id = user_info.get("epic_account_id", "");
		String product_user_id = user_info.get("product_user_id", "");

		UtilityFunctions::print("GodotEOS: Login successful for user: " + display_name);
		UtilityFunctions::print("GodotEOS: Epic Account ID: " + epic_account_id);
		UtilityFunctions::print("GodotEOS: Product User ID: " + product_user_id);
	} else {
		UtilityFunctions::printerr("GodotEOS: Login failed");
	}

	// Emit the login_completed signal
	emit_signal("login_completed", success, user_info);
}

void GodotEOS::on_achievement_definitions_completed(bool success, const Array& definitions) {
	UtilityFunctions::print("GodotEOS: Achievement definitions query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		UtilityFunctions::print("GodotEOS: Achievement definitions updated (" + String::num_int64(definitions.size()) + " definitions)");
	} else {
		UtilityFunctions::printerr("GodotEOS: Achievement definitions query failed");
	}

	// Emit the achievement_definitions_updated signal
	emit_signal("achievement_definitions_updated", definitions);
}

void GodotEOS::on_player_achievements_completed(bool success, const Array& achievements) {
	UtilityFunctions::print("GodotEOS: Player achievements query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		UtilityFunctions::print("GodotEOS: Player achievements updated (" + String::num_int64(achievements.size()) + " achievements)");
	} else {
		UtilityFunctions::printerr("GodotEOS: Player achievements query failed");
	}

	// Emit the player_achievements_updated signal
	emit_signal("player_achievements_updated", achievements);
}

void GodotEOS::on_achievements_unlocked_completed(bool success, const Array& unlocked_achievement_ids) {
	UtilityFunctions::print("GodotEOS: Achievements unlock completed - success: " + String(success ? "true" : "false"));

	if (success) {
		UtilityFunctions::print("GodotEOS: Achievements unlocked successfully");
	} else {
		UtilityFunctions::printerr("GodotEOS: Achievements unlock failed");
	}

	// Emit the achievements_unlocked signal
	emit_signal("achievements_unlocked", unlocked_achievement_ids);
}

void GodotEOS::on_achievement_stats_completed(bool success, const Array& stats) {
	UtilityFunctions::print("GodotEOS: Achievement stats query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		UtilityFunctions::print("GodotEOS: Achievement stats updated (" + String::num_int64(stats.size()) + " stats)");
	} else {
		UtilityFunctions::printerr("GodotEOS: Achievement stats query failed");
	}

	// Emit the achievement_stats_updated signal
	emit_signal("achievement_stats_updated", success, stats);
}

void GodotEOS::on_leaderboard_definitions_completed(bool success, const Array& definitions) {
	UtilityFunctions::print("GodotEOS: Leaderboard definitions query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		UtilityFunctions::print("GodotEOS: Leaderboard definitions updated (" + String::num_int64(definitions.size()) + " definitions)");
	} else {
		UtilityFunctions::printerr("GodotEOS: Leaderboard definitions query failed");
	}

	// Emit the leaderboard_definitions_updated signal
	emit_signal("leaderboard_definitions_updated", success, definitions);
}

void GodotEOS::on_leaderboard_ranks_completed(bool success, const Array& ranks) {
	UtilityFunctions::print("GodotEOS: Leaderboard ranks query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		UtilityFunctions::print("GodotEOS: Leaderboard ranks updated (" + String::num_int64(ranks.size()) + " ranks)");
	} else {
		UtilityFunctions::printerr("GodotEOS: Leaderboard ranks query failed");
	}

	// Emit the leaderboard_ranks_updated signal
	emit_signal("leaderboard_ranks_updated", success, ranks);
}

void GodotEOS::on_leaderboard_user_scores_completed(bool success, const Dictionary& user_scores) {
	UtilityFunctions::print("GodotEOS: Leaderboard user scores query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		UtilityFunctions::print("GodotEOS: Leaderboard user scores updated (" + String::num_int64(user_scores.size()) + " user scores)");
	} else {
		UtilityFunctions::printerr("GodotEOS: Leaderboard user scores query failed");
	}

	// Emit the leaderboard_user_scores_updated signal
	emit_signal("leaderboard_user_scores_updated", success, user_scores);
}

void GodotEOS::on_friends_query_completed(bool success, const Array& friends_list) {
	UtilityFunctions::print("GodotEOS: Friends query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		UtilityFunctions::print("GodotEOS: Friends list updated (" + String::num_int64(friends_list.size()) + " friends)");
	} else {
		UtilityFunctions::printerr("GodotEOS: Friends query failed");
	}

	// Emit the friends_updated signal
	emit_signal("friends_updated", success, friends_list);
}

void GodotEOS::on_friend_info_query_completed(bool success, const Dictionary& friend_info) {
	UtilityFunctions::print("GodotEOS: Friend info query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		String friend_id = friend_info.get("id", "unknown");
		UtilityFunctions::print("GodotEOS: Friend info updated for: " + friend_id);
	} else {
		UtilityFunctions::printerr("GodotEOS: Friend info query failed");
	}

	// Emit the friend_info_updated signal
	emit_signal("friend_info_updated", friend_info);
}
