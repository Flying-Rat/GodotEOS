#include "godotepic.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include "SubsystemManager.h"
#include "IAuthenticationSubsystem.h"
#include "IAchievementsSubsystem.h"
#include "ILeaderboardsSubsystem.h"
#include "PlatformSubsystem.h"
#include "AuthenticationSubsystem.h"
#include "AchievementsSubsystem.h"
#include "LeaderboardsSubsystem.h"

using namespace godot;

// Static member definitions
GodotEpic* GodotEpic::instance = nullptr;

void GodotEpic::_bind_methods() {
	ClassDB::bind_static_method("GodotEpic", D_METHOD("get_singleton"), &GodotEpic::get_singleton);
	ClassDB::bind_method(D_METHOD("initialize_platform", "options"), &GodotEpic::initialize_platform);
	ClassDB::bind_method(D_METHOD("shutdown_platform"), &GodotEpic::shutdown_platform);
	ClassDB::bind_method(D_METHOD("tick"), &GodotEpic::tick);
	ClassDB::bind_method(D_METHOD("is_platform_initialized"), &GodotEpic::is_platform_initialized);

	// Authentication methods
	ClassDB::bind_method(D_METHOD("login_with_epic_account", "email", "password"), &GodotEpic::login_with_epic_account);
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

	// Achievements callback
	ClassDB::bind_method(D_METHOD("on_achievement_definitions_completed", "success", "definitions"), &GodotEpic::on_achievement_definitions_completed);
	ClassDB::bind_method(D_METHOD("on_player_achievements_completed", "success", "achievements"), &GodotEpic::on_player_achievements_completed);
	ClassDB::bind_method(D_METHOD("on_achievements_unlocked_completed", "success", "unlocked_achievement_ids"), &GodotEpic::on_achievements_unlocked_completed);

	// Stats callback
	ClassDB::bind_method(D_METHOD("on_achievement_stats_completed", "success", "stats"), &GodotEpic::on_achievement_stats_completed);


	// Signals
	ADD_SIGNAL(MethodInfo("login_completed", PropertyInfo(Variant::BOOL, "success"), PropertyInfo(Variant::DICTIONARY, "user_info")));
	ADD_SIGNAL(MethodInfo("logout_completed", PropertyInfo(Variant::BOOL, "success")));
	ADD_SIGNAL(MethodInfo("friends_updated", PropertyInfo(Variant::ARRAY, "friends_list")));
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
	time_passed = 0.0;
	instance = this;

	// Initialize authentication state
	product_user_id = nullptr;
	is_logged_in = false;
	current_username = "";
}

GodotEpic::~GodotEpic() {
	// Ensure platform is shutdown on destruction
	shutdown_platform();
	if (instance == this) {
		instance = nullptr;
	}
}

GodotEpic* GodotEpic::get_singleton() {
	if (!instance) {
		instance = memnew(GodotEpic);
	}
	return instance;
}

bool GodotEpic::initialize_platform(const Dictionary& options) {
	// Convert dictionary to init options
	EpicInitOptions init_options = _dict_to_init_options(options);

	// Debug: print product name/version we will pass to platform
	ERR_PRINT("[initialize_platform] product_name: " + init_options.product_name);
	ERR_PRINT("[initialize_platform] product_version: " + init_options.product_version);

	// Validate options
	if (!_validate_init_options(init_options)) {
		ERR_PRINT("EOS Platform initialization failed: Invalid options");
		return false;
	}

	// Setup logging
	EOS_EResult LogResult = EOS_Logging_SetCallback(logging_callback);
	if (LogResult == EOS_EResult::EOS_Success) {
		EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES,
							   EOS_ELogLevel::EOS_LOG_Verbose);
	}

	// Register subsystems with SubsystemManager
	SubsystemManager* manager = SubsystemManager::GetInstance();

	// Register PlatformSubsystem first (needed by other subsystems)
	manager->RegisterSubsystem<IPlatformSubsystem, PlatformSubsystem>("PlatformSubsystem");

	// Register other subsystems
	manager->RegisterSubsystem<IAuthenticationSubsystem, AuthenticationSubsystem>("AuthenticationSubsystem");
	manager->RegisterSubsystem<IAchievementsSubsystem, AchievementsSubsystem>("AchievementsSubsystem");
	manager->RegisterSubsystem<ILeaderboardsSubsystem, LeaderboardsSubsystem>("LeaderboardsSubsystem");

	// Initialize PlatformSubsystem with EpicInitOptions
	auto platform_subsystem = manager->GetSubsystem<IPlatformSubsystem>();
	if (!platform_subsystem) {
		ERR_PRINT("Failed to get PlatformSubsystem");
		return false;
	}

	if (!platform_subsystem->InitializePlatform(init_options)) {
		ERR_PRINT("PlatformSubsystem initialization failed");
		return false;
	}

	// Initialize all subsystems
	if (!manager->InitializeAll()) {
		ERR_PRINT("Failed to initialize subsystems");
		return false;
	}

	// Set up authentication callback
	setup_authentication_callback();

	// Set up achievements callbacks
	setup_achievements_callbacks();

	return true;
}

void GodotEpic::shutdown_platform() {
	// Shutdown all subsystems
	SubsystemManager* manager = SubsystemManager::GetInstance();
	manager->ShutdownAll();
}

void GodotEpic::tick() {
	// Tick all subsystems
	SubsystemManager* manager = SubsystemManager::GetInstance();
	manager->TickAll(time_passed);
}

bool GodotEpic::is_platform_initialized() const {
	auto platform_subsystem = Get<IPlatformSubsystem>();
	return platform_subsystem && platform_subsystem->GetPlatformHandle() != nullptr;
}

EOS_HPlatform GodotEpic::get_platform_handle() const {
	auto platform_subsystem = Get<IPlatformSubsystem>();
	return platform_subsystem ? static_cast<EOS_HPlatform>(platform_subsystem->GetPlatformHandle()) : nullptr;
}

// Authentication methods
void GodotEpic::login_with_epic_account(const String& email, const String& password) {
	auto auth = Get<IAuthenticationSubsystem>();
	if (!auth) {
		ERR_PRINT("AuthenticationSubsystem not available");
		Dictionary empty_user_info;
		emit_signal("login_completed", false, empty_user_info);
		return;
	}

	Dictionary credentials;
	credentials["email"] = email;
	credentials["password"] = password;

	if (!auth->Login("epic_account", credentials)) {
		ERR_PRINT("AuthenticationSubsystem login failed");
		Dictionary empty_user_info;
		emit_signal("login_completed", false, empty_user_info);
	}
}

void GodotEpic::login_with_dev(const String& display_name) {
	auto auth = Get<IAuthenticationSubsystem>();
	if (!auth) {
		ERR_PRINT("AuthenticationSubsystem not available");
		Dictionary empty_user_info;
		emit_signal("login_completed", false, empty_user_info);
		return;
	}

	Dictionary credentials;
	credentials["id"] = "localhost:7777";
	credentials["token"] = display_name.is_empty() ? "TestUser" : display_name;

	if (!auth->Login("dev", credentials)) {
		ERR_PRINT("AuthenticationSubsystem dev login failed");
		Dictionary empty_user_info;
		emit_signal("login_completed", false, empty_user_info);
	}
	WARN_PRINT("Dev login initiated with display name: " + (display_name.is_empty() ? String("TestUser") : display_name));
}

void GodotEpic::login_with_device_id(const String& display_name) {
	auto auth = Get<IAuthenticationSubsystem>();
	if (!auth) {
		ERR_PRINT("AuthenticationSubsystem not available");
		Dictionary empty_user_info;
		emit_signal("login_completed", false, empty_user_info);
		return;
	}

	if (!auth->Login("device_id", Dictionary())) {
		ERR_PRINT("AuthenticationSubsystem device ID login failed");
		Dictionary empty_user_info;
		emit_signal("login_completed", false, empty_user_info);
	}
	WARN_PRINT("Device ID login initiated with display name: " + display_name);
}

void GodotEpic::logout() {
	auto auth = Get<IAuthenticationSubsystem>();
	if (!auth) {
		ERR_PRINT("AuthenticationSubsystem not available");
		emit_signal("logout_completed", false);
		return;
	}

	if (!auth->Logout()) {
		ERR_PRINT("AuthenticationSubsystem logout failed");
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
	auto auth = Get<IAuthenticationSubsystem>();
	return auth ? auth->GetEpicAccountId() : "";
}

String GodotEpic::get_product_user_id() const {
	auto auth = Get<IAuthenticationSubsystem>();
	return auth ? auth->GetProductUserId() : "";
}

// Friends methods
void GodotEpic::query_friends() {
	auto auth = Get<IAuthenticationSubsystem>();
	if (!auth || !auth->IsLoggedIn()) {
		ERR_PRINT("AuthenticationSubsystem not available or user not logged in");
		return;
	}

	EOS_HPlatform platform_handle = get_platform_handle();
	if (!platform_handle) {
		ERR_PRINT("Platform not initialized");
		return;
	}

	EOS_HFriends friends_handle = EOS_Platform_GetFriendsInterface(platform_handle);
	if (!friends_handle) {
		ERR_PRINT("Failed to get Friends interface");
		return;
	}

	EOS_Friends_QueryFriendsOptions query_options = {};
	query_options.ApiVersion = EOS_FRIENDS_QUERYFRIENDS_API_LATEST;
	query_options.LocalUserId = static_cast<AuthenticationSubsystem*>(auth)->GetRawEpicAccountId();

	EOS_Friends_QueryFriends(friends_handle, &query_options, nullptr, friends_query_callback);
	ERR_PRINT("Friends query initiated");
}

Array GodotEpic::get_friends_list() {
	Array friends_array;

	auto auth = Get<IAuthenticationSubsystem>();
	if (!auth || !auth->IsLoggedIn()) {
		ERR_PRINT("AuthenticationSubsystem not available or user not logged in");
		return friends_array;
	}

	EOS_HPlatform platform_handle = get_platform_handle();
	if (!platform_handle) {
		ERR_PRINT("Platform not initialized");
		return friends_array;
	}

	EOS_HFriends friends_handle = EOS_Platform_GetFriendsInterface(platform_handle);
	if (!friends_handle) {
		ERR_PRINT("Failed to get Friends interface");
		return friends_array;
	}

	EOS_Friends_GetFriendsCountOptions count_options = {};
	count_options.ApiVersion = EOS_FRIENDS_GETFRIENDSCOUNT_API_LATEST;
	count_options.LocalUserId = static_cast<AuthenticationSubsystem*>(auth)->GetRawEpicAccountId();

	int32_t friends_count = EOS_Friends_GetFriendsCount(friends_handle, &count_options);

	for (int32_t i = 0; i < friends_count; i++) {
		EOS_Friends_GetFriendAtIndexOptions friend_options = {};
		friend_options.ApiVersion = EOS_FRIENDS_GETFRIENDATINDEX_API_LATEST;
		friend_options.LocalUserId = static_cast<AuthenticationSubsystem*>(auth)->GetRawEpicAccountId();
		friend_options.Index = i;

		EOS_EpicAccountId friend_id = EOS_Friends_GetFriendAtIndex(friends_handle, &friend_options);
		if (friend_id) {
			Dictionary friend_info;

			// Convert friend ID to string
			static char friend_id_str[EOS_EPICACCOUNTID_MAX_LENGTH + 1];
			int32_t buffer_size = sizeof(friend_id_str);

			EOS_EResult result = EOS_EpicAccountId_ToString(friend_id, friend_id_str, &buffer_size);
			if (result == EOS_EResult::EOS_Success) {
				friend_info["id"] = String::utf8(friend_id_str);

				// Get friend status
				EOS_Friends_GetStatusOptions status_options = {};
				status_options.ApiVersion = EOS_FRIENDS_GETSTATUS_API_LATEST;
				status_options.LocalUserId = static_cast<AuthenticationSubsystem*>(auth)->GetRawEpicAccountId();
				status_options.TargetUserId = friend_id;

				EOS_EFriendsStatus status = EOS_Friends_GetStatus(friends_handle, &status_options);

				String status_str = "Unknown";
				switch (status) {
					case EOS_EFriendsStatus::EOS_FS_Friends:
						status_str = "Friends";
						break;
					case EOS_EFriendsStatus::EOS_FS_InviteSent:
						status_str = "Invite Sent";
						break;
					case EOS_EFriendsStatus::EOS_FS_InviteReceived:
						status_str = "Invite Received";
						break;
					default:
						status_str = "Not Friends";
						break;
				}

				friend_info["status"] = status_str;
				friends_array.append(friend_info);
			}
		}
	}

	return friends_array;
}

Dictionary GodotEpic::get_friend_info(const String& friend_id) {
	Dictionary friend_info;

	auto auth = Get<IAuthenticationSubsystem>();
	if (!auth || !auth->IsLoggedIn()) {
		ERR_PRINT("AuthenticationSubsystem not available or user not logged in");
		return friend_info;
	}

	EOS_HPlatform platform_handle = get_platform_handle();
	if (!platform_handle) {
		ERR_PRINT("Platform not initialized");
		return friend_info;
	}

	// For now, return basic structure
	// In a full implementation, you'd query user info, presence, etc.
	friend_info["id"] = friend_id;
	friend_info["display_name"] = "Friend";  // Would get from UserInfo interface
	friend_info["status"] = "Unknown";
	friend_info["online"] = false;

	return friend_info;
}

// Achievements methods
void GodotEpic::query_achievement_definitions() {
	auto achievements = Get<IAchievementsSubsystem>();
	if (!achievements) {
		ERR_PRINT("AchievementsSubsystem not available");
		Array empty_definitions;
		emit_signal("achievement_definitions_updated", empty_definitions);
		return;
	}

	if (!achievements->QueryAchievementDefinitions()) {
		ERR_PRINT("AchievementsSubsystem query definitions failed");
		Array empty_definitions;
		emit_signal("achievement_definitions_updated", empty_definitions);
	}
}

void GodotEpic::query_player_achievements() {
	auto achievements = Get<IAchievementsSubsystem>();
	if (!achievements) {
		ERR_PRINT("AchievementsSubsystem not available");
		Array empty_achievements;
		emit_signal("player_achievements_updated", empty_achievements);
		return;
	}

	if (!achievements->QueryPlayerAchievements()) {
		ERR_PRINT("AchievementsSubsystem query player achievements failed");
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
	auto achievements = Get<IAchievementsSubsystem>();
	if (!achievements) {
		ERR_PRINT("AchievementsSubsystem not available");
		Array empty_unlocked;
		emit_signal("achievements_unlocked", empty_unlocked);
		return;
	}

	if (!achievements->UnlockAchievements(achievement_ids)) {
		ERR_PRINT("AchievementsSubsystem unlock achievements failed");
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
	auto achievements = Get<IAchievementsSubsystem>();
	if (!achievements) {
		ERR_PRINT("AchievementsSubsystem not available");
		Array empty_stats;
		emit_signal("achievement_stats_updated", false, empty_stats);
		return;
	}

	if (!achievements->IngestStat(stat_name, amount)) {
		ERR_PRINT("AchievementsSubsystem ingest stat failed");
		Array empty_stats;
		emit_signal("achievement_stats_updated", false, empty_stats);
	}
}

void GodotEpic::query_achievement_stats() {
	auto achievements = Get<IAchievementsSubsystem>();
	if (!achievements) {
		ERR_PRINT("AchievementsSubsystem not available");
		Array empty_stats;
		emit_signal("achievement_stats_updated", false, empty_stats);
		return;
	}

	if (!achievements->QueryStats()) {
		ERR_PRINT("AchievementsSubsystem query stats failed");
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
				ERR_PRINT(log_msg);
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
				ERR_PRINT(log_msg);
			}
			break;
	}
}

// Friends callbacks
void EOS_CALL GodotEpic::friends_query_callback(const EOS_Friends_QueryFriendsCallbackInfo* data) {
	if (!data || !instance) {
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		ERR_PRINT("Friends query successful - friends list updated");

		// Get updated friends list and emit signal
		Array friends_list = instance->get_friends_list();
		instance->emit_signal("friends_updated", friends_list);
	} else {
		String error_msg = "Friends query failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
		ERR_PRINT(error_msg);

		// Emit empty friends list on failure
		Array empty_friends;
		instance->emit_signal("friends_updated", empty_friends);
	}
}

// Leaderboards methods
void GodotEpic::query_leaderboard_definitions() {
	auto leaderboards = Get<ILeaderboardsSubsystem>();
	if (!leaderboards) {
		ERR_PRINT("LeaderboardsSubsystem not available");
		Array empty_definitions;
		emit_signal("leaderboard_definitions_updated", empty_definitions);
		return;
	}

	if (!leaderboards->QueryLeaderboardDefinitions()) {
		ERR_PRINT("LeaderboardsSubsystem query definitions failed");
		Array empty_definitions;
		emit_signal("leaderboard_definitions_updated", empty_definitions);
	}
}

void GodotEpic::query_leaderboard_ranks(const String& leaderboard_id, int limit) {
	auto leaderboards = Get<ILeaderboardsSubsystem>();
	if (!leaderboards) {
		ERR_PRINT("LeaderboardsSubsystem not available");
		Array empty_ranks;
		emit_signal("leaderboard_ranks_updated", empty_ranks);
		return;
	}

	if (!leaderboards->QueryLeaderboardRanks(leaderboard_id, limit)) {
		ERR_PRINT("LeaderboardsSubsystem query ranks failed");
		Array empty_ranks;
		emit_signal("leaderboard_ranks_updated", empty_ranks);
	}
}

void GodotEpic::query_leaderboard_user_scores(const String& leaderboard_id, const Array& user_ids) {
	auto leaderboards = Get<ILeaderboardsSubsystem>();
	if (!leaderboards) {
		ERR_PRINT("LeaderboardsSubsystem not available");
		Dictionary empty_scores;
		emit_signal("leaderboard_user_scores_updated", empty_scores);
		return;
	}

	if (!leaderboards->QueryLeaderboardUserScores(leaderboard_id, user_ids)) {
		ERR_PRINT("LeaderboardsSubsystem query user scores failed");
		Dictionary empty_scores;
		emit_signal("leaderboard_user_scores_updated", empty_scores);
	}
}

void GodotEpic::ingest_stat(const String& stat_name, int value) {
	auto leaderboards = Get<ILeaderboardsSubsystem>();
	if (!leaderboards) {
		ERR_PRINT("LeaderboardsSubsystem not available");
		Array empty_stats;
		emit_signal("stats_ingested", empty_stats);
		return;
	}

	if (!leaderboards->IngestStat(stat_name, value)) {
		ERR_PRINT("LeaderboardsSubsystem ingest stat failed");
		Array empty_stats;
		emit_signal("stats_ingested", empty_stats);
	}
}

void GodotEpic::ingest_stats(const Dictionary& stats) {
	auto leaderboards = Get<ILeaderboardsSubsystem>();
	if (!leaderboards) {
		ERR_PRINT("LeaderboardsSubsystem not available");
		Array empty_stats;
		emit_signal("stats_ingested", empty_stats);
		return;
	}

	if (!leaderboards->IngestStats(stats)) {
		ERR_PRINT("LeaderboardsSubsystem ingest stats failed");
		Array empty_stats;
		emit_signal("stats_ingested", empty_stats);
	}
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

	// Debug print to verify values are being set correctly
	ERR_PRINT("EOS Init Options:");
	ERR_PRINT("  Product Name: " + options.product_name);
	ERR_PRINT("  Product Version: " + options.product_version);
	ERR_PRINT("  Product ID: " + options.product_id);
	ERR_PRINT("  Sandbox ID: " + options.sandbox_id);
	ERR_PRINT("  Deployment ID: " + options.deployment_id);
	ERR_PRINT("  Client ID: " + options.client_id);
	ERR_PRINT("  Client Secret: " + String(options.client_secret.is_empty() ? "EMPTY" : "SET"));
	ERR_PRINT("  Encryption Key: " + String(options.encryption_key.is_empty() ? "EMPTY" : "SET"));

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
		ERR_PRINT("Missing required initialization option: product_id");
		valid = false;
	}
	if (options.sandbox_id.is_empty()) {
		ERR_PRINT("Missing required initialization option: sandbox_id");
		valid = false;
	}
	if (options.deployment_id.is_empty()) {
		ERR_PRINT("Missing required initialization option: deployment_id");
		valid = false;
	}
	if (options.client_id.is_empty()) {
		ERR_PRINT("Missing required initialization option: client_id");
		valid = false;
	}
	if (options.client_secret.is_empty()) {
		ERR_PRINT("Missing required initialization option: client_secret");
		valid = false;
	}

	if (options.encryption_key.is_empty()) {
		WARN_PRINT("Encryption key not set - data will not be encrypted");
	}

	return valid;
}

void GodotEpic::setup_authentication_callback() {
	auto auth = Get<IAuthenticationSubsystem>();
	if (auth) {
		// Create a callable that binds to our instance method
		Callable callback = Callable(this, "on_authentication_completed");
		auth->SetLoginCallback(callback);
		ERR_PRINT("Authentication callback set up successfully");
	} else {
		ERR_PRINT("Failed to set up authentication callback - AuthenticationSubsystem not available");
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

		ERR_PRINT("Achievements callbacks set up successfully");
	} else {
		ERR_PRINT("Failed to set up achievements callbacks - AchievementsSubsystem not available");
	}
}

void GodotEpic::on_authentication_completed(bool success, const Dictionary& user_info) {
	ERR_PRINT("GodotEpic: Authentication completed - success: " + String(success ? "true" : "false"));

	if (success) {
		String display_name = user_info.get("display_name", "Unknown User");
		String epic_account_id = user_info.get("epic_account_id", "");
		String product_user_id = user_info.get("product_user_id", "");

		ERR_PRINT("GodotEpic: Login successful for user: " + display_name);
		ERR_PRINT("GodotEpic: Epic Account ID: " + epic_account_id);
		ERR_PRINT("GodotEpic: Product User ID: " + product_user_id);

		// Update legacy state for backward compatibility
		is_logged_in = true;
		current_username = display_name;
	} else {
		ERR_PRINT("GodotEpic: Login failed");
		is_logged_in = false;
		current_username = "";
	}

	// Emit the login_completed signal
	emit_signal("login_completed", success, user_info);
}

void GodotEpic::on_achievement_definitions_completed(bool success, const Array& definitions) {
	ERR_PRINT("GodotEpic: Achievement definitions query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		ERR_PRINT("GodotEpic: Achievement definitions updated (" + String::num_int64(definitions.size()) + " definitions)");
	} else {
		ERR_PRINT("GodotEpic: Achievement definitions query failed");
	}

	// Emit the achievement_definitions_updated signal
	emit_signal("achievement_definitions_updated", definitions);
}

void GodotEpic::on_player_achievements_completed(bool success, const Array& achievements) {
	ERR_PRINT("GodotEpic: Player achievements query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		ERR_PRINT("GodotEpic: Player achievements updated (" + String::num_int64(achievements.size()) + " achievements)");
	} else {
		ERR_PRINT("GodotEpic: Player achievements query failed");
	}

	// Emit the player_achievements_updated signal
	emit_signal("player_achievements_updated", achievements);
}

void GodotEpic::on_achievements_unlocked_completed(bool success, const Array& unlocked_achievement_ids) {
	ERR_PRINT("GodotEpic: Achievements unlock completed - success: " + String(success ? "true" : "false"));

	if (success) {
		ERR_PRINT("GodotEpic: Achievements unlocked successfully");
	} else {
		ERR_PRINT("GodotEpic: Achievements unlock failed");
	}

	// Emit the achievements_unlocked signal
	emit_signal("achievements_unlocked", unlocked_achievement_ids);
}

void GodotEpic::on_achievement_stats_completed(bool success, const Array& stats) {
	ERR_PRINT("GodotEpic: Achievement stats query completed - success: " + String(success ? "true" : "false"));

	if (success) {
		ERR_PRINT("GodotEpic: Achievement stats updated (" + String::num_int64(stats.size()) + " stats)");
	} else {
		ERR_PRINT("GodotEpic: Achievement stats query failed");
	}

	// Emit the achievement_stats_updated signal
	emit_signal("achievement_stats_updated", success, stats);
}