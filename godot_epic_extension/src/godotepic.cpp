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

	// Leaderboards methods
	ClassDB::bind_method(D_METHOD("query_leaderboard_definitions"), &GodotEpic::query_leaderboard_definitions);
	ClassDB::bind_method(D_METHOD("query_leaderboard_ranks", "leaderboard_id", "limit"), &GodotEpic::query_leaderboard_ranks);
	ClassDB::bind_method(D_METHOD("query_leaderboard_user_scores", "leaderboard_id", "user_ids"), &GodotEpic::query_leaderboard_user_scores);
	ClassDB::bind_method(D_METHOD("ingest_stat", "stat_name", "value"), &GodotEpic::ingest_stat);
	ClassDB::bind_method(D_METHOD("ingest_stats", "stats"), &GodotEpic::ingest_stats);
	ClassDB::bind_method(D_METHOD("get_leaderboard_definitions"), &GodotEpic::get_leaderboard_definitions);
	ClassDB::bind_method(D_METHOD("get_leaderboard_ranks"), &GodotEpic::get_leaderboard_ranks);
	ClassDB::bind_method(D_METHOD("get_leaderboard_user_scores"), &GodotEpic::get_leaderboard_user_scores);

	ClassDB::bind_method(D_METHOD("test_subsystem_manager"), &GodotEpic::test_subsystem_manager);
	ClassDB::bind_method(D_METHOD("on_authentication_completed", "success", "user_info"), &GodotEpic::on_authentication_completed);

	// Signals
	ADD_SIGNAL(MethodInfo("login_completed", PropertyInfo(Variant::BOOL, "success"), PropertyInfo(Variant::DICTIONARY, "user_info")));
	ADD_SIGNAL(MethodInfo("logout_completed", PropertyInfo(Variant::BOOL, "success")));
	ADD_SIGNAL(MethodInfo("friends_updated", PropertyInfo(Variant::ARRAY, "friends_list")));
	ADD_SIGNAL(MethodInfo("achievement_definitions_updated", PropertyInfo(Variant::ARRAY, "definitions")));
	ADD_SIGNAL(MethodInfo("player_achievements_updated", PropertyInfo(Variant::ARRAY, "achievements")));
	ADD_SIGNAL(MethodInfo("achievements_unlocked", PropertyInfo(Variant::ARRAY, "unlocked_achievement_ids")));
	ADD_SIGNAL(MethodInfo("achievement_unlocked", PropertyInfo(Variant::STRING, "achievement_id"), PropertyInfo(Variant::INT, "unlock_time")));
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
	epic_account_id = nullptr;
	product_user_id = nullptr;
	is_logged_in = false;
	current_username = "";

	// Initialize achievements state
	achievements_notification_id = EOS_INVALID_NOTIFICATIONID;
	achievements_definitions_cached = false;
	player_achievements_cached = false;
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
	EOS_HPlatform platform_handle = get_platform_handle();
	if (!platform_handle || !epic_account_id) {
		ERR_PRINT("Platform not initialized or user not logged in");
		return;
	}

	EOS_HFriends friends_handle = EOS_Platform_GetFriendsInterface(platform_handle);
	if (!friends_handle) {
		ERR_PRINT("Failed to get Friends interface");
		return;
	}

	EOS_Friends_QueryFriendsOptions query_options = {};
	query_options.ApiVersion = EOS_FRIENDS_QUERYFRIENDS_API_LATEST;
	query_options.LocalUserId = epic_account_id;

	EOS_Friends_QueryFriends(friends_handle, &query_options, nullptr, friends_query_callback);
	ERR_PRINT("Friends query initiated");
}

Array GodotEpic::get_friends_list() {
	Array friends_array;

	EOS_HPlatform platform_handle = get_platform_handle();
	if (!platform_handle || !epic_account_id) {
		ERR_PRINT("Platform not initialized or user not logged in");
		return friends_array;
	}

	EOS_HFriends friends_handle = EOS_Platform_GetFriendsInterface(platform_handle);
	if (!friends_handle) {
		ERR_PRINT("Failed to get Friends interface");
		return friends_array;
	}

	EOS_Friends_GetFriendsCountOptions count_options = {};
	count_options.ApiVersion = EOS_FRIENDS_GETFRIENDSCOUNT_API_LATEST;
	count_options.LocalUserId = epic_account_id;

	int32_t friends_count = EOS_Friends_GetFriendsCount(friends_handle, &count_options);

	for (int32_t i = 0; i < friends_count; i++) {
		EOS_Friends_GetFriendAtIndexOptions friend_options = {};
		friend_options.ApiVersion = EOS_FRIENDS_GETFRIENDATINDEX_API_LATEST;
		friend_options.LocalUserId = epic_account_id;
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
				status_options.LocalUserId = epic_account_id;
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

	EOS_HPlatform platform_handle = get_platform_handle();
	if (!platform_handle || !epic_account_id) {
		ERR_PRINT("Platform not initialized or user not logged in");
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

// Authentication callbacks
void EOS_CALL GodotEpic::auth_login_callback(const EOS_Auth_LoginCallbackInfo* data) {
	if (!data || !instance) {
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		instance->epic_account_id = data->LocalUserId;
		instance->is_logged_in = true;

		// Get user info
		EOS_HPlatform platform_handle = instance->get_platform_handle();
		if (!platform_handle) {
			ERR_PRINT("Failed to get platform handle");
			Dictionary empty_user_info;
			instance->emit_signal("login_completed", false, empty_user_info);
			return;
		}

		EOS_HAuth auth_handle = EOS_Platform_GetAuthInterface(platform_handle);
		if (auth_handle) {
			EOS_Auth_CopyUserAuthTokenOptions token_options = {};
			token_options.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

			EOS_Auth_Token* auth_token = nullptr;
			EOS_EResult result = EOS_Auth_CopyUserAuthToken(auth_handle, &token_options, data->LocalUserId, &auth_token);

			if (result == EOS_EResult::EOS_Success && auth_token) {
				// Use the App field as display name if available
				if (auth_token->App) {
					instance->current_username = String::utf8(auth_token->App);
				} else {
					// Fallback to a simple default
					instance->current_username = "Epic User";
				}
				EOS_Auth_Token_Release(auth_token);
			}
		}

		// Now login to Connect service for cross-platform features
		EOS_HConnect connect_handle = EOS_Platform_GetConnectInterface(platform_handle);
		if (connect_handle) {
			// Get Auth Token for Connect login (not Epic Account ID)
			EOS_Auth_Token* auth_token = nullptr;
			EOS_Auth_CopyUserAuthTokenOptions copy_options = {};
			copy_options.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;

			EOS_EResult copy_result = EOS_Auth_CopyUserAuthToken(auth_handle, &copy_options, data->LocalUserId, &auth_token);

			if (copy_result == EOS_EResult::EOS_Success && auth_token) {
				EOS_Connect_LoginOptions connect_options = {};
				connect_options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;

				EOS_Connect_Credentials connect_credentials = {};
				connect_credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
				connect_credentials.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC;
				connect_credentials.Token = auth_token->AccessToken;  // Use Auth Token instead of Account ID

				connect_options.Credentials = &connect_credentials;

				EOS_Connect_Login(connect_handle, &connect_options, nullptr, connect_login_callback);

				// Clean up the auth token
				EOS_Auth_Token_Release(auth_token);
			} else {
				ERR_PRINT("Failed to copy Auth Token for Connect login - skipping Connect service");
				// Emit success anyway since Auth login succeeded, but without Connect features
				Dictionary user_info;
				user_info["display_name"] = instance->current_username;
				user_info["epic_account_id"] = instance->get_epic_account_id();
				user_info["product_user_id"] = "";  // Empty since Connect failed

				instance->emit_signal("login_completed", true, user_info);
			}
		}

		ERR_PRINT("Epic Account login successful - initiating Connect login...");

		// Don't emit login signal yet - wait for Connect login to complete
	} else {
		String error_msg = "Epic Account login failed: ";

		// Provide more descriptive error messages
		switch (data->ResultCode) {
			case EOS_EResult::EOS_InvalidParameters:
				error_msg += "Invalid parameters (10) - For Device ID login, make sure EOS Dev Auth Tool is running on localhost:7777. For Epic Account login, check email/password format";
				break;
			case EOS_EResult::EOS_Invalid_Deployment:
				error_msg += "Invalid deployment (32) - Check your deployment_id in EOS Developer Portal";
				break;
			case EOS_EResult::EOS_InvalidCredentials:
				error_msg += "Invalid credentials (2) - Check your email/password";
				break;
			case EOS_EResult::EOS_InvalidUser:
				error_msg += "Invalid user (3) - User may need to be linked";
				break;
			case EOS_EResult::EOS_MissingPermissions:
				error_msg += "Missing permissions (6) - Check app permissions in EOS Developer Portal";
				break;
			case EOS_EResult::EOS_ApplicationSuspended:
				error_msg += "Application suspended (40) - App may be suspended in EOS Developer Portal";
				break;
			case EOS_EResult::EOS_NetworkDisconnected:
				error_msg += "Network disconnected (41) - Check internet connection";
				break;
			case EOS_EResult::EOS_NotConfigured:
				error_msg += "Not configured (14) - Check your EOS app configuration";
				break;
			case EOS_EResult::EOS_Invalid_Sandbox:
				error_msg += "Invalid sandbox (31) - Check your sandbox_id in EOS Developer Portal";
				break;
			case EOS_EResult::EOS_Invalid_Product:
				error_msg += "Invalid product (33) - Check your product_id in EOS Developer Portal";
				break;
			default:
				error_msg += String::num_int64(static_cast<int64_t>(data->ResultCode));
				break;
		}

		ERR_PRINT(error_msg);

		// Emit login failed signal
		Dictionary empty_user_info;
		instance->emit_signal("login_completed", false, empty_user_info);
	}
}

void EOS_CALL GodotEpic::auth_logout_callback(const EOS_Auth_LogoutCallbackInfo* data) {
	if (!data || !instance) {
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		instance->epic_account_id = data->LocalUserId;
		instance->product_user_id = nullptr;
		instance->is_logged_in = false;
		instance->current_username = "";

		ERR_PRINT("Logout successful");

		// Emit logout completed signal
		instance->emit_signal("logout_completed", true);
	} else {
		String error_msg = "Logout failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
		ERR_PRINT(error_msg);

		// Emit logout failed signal
		instance->emit_signal("logout_completed", false);
	}
}

void EOS_CALL GodotEpic::connect_login_callback(const EOS_Connect_LoginCallbackInfo* data) {
	if (!data || !instance) {
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		instance->product_user_id = data->LocalUserId;
		WARN_PRINT("Connect login successful - cross-platform features enabled");

		// Now both Auth and Connect logins are complete, emit the signal
		Dictionary user_info;
		user_info["display_name"] = instance->current_username;
		user_info["epic_account_id"] = instance->get_epic_account_id();
		user_info["product_user_id"] = instance->get_product_user_id();

		instance->emit_signal("login_completed", true, user_info);
	} else {
		String error_msg = "Connect login failed: ";

		// Provide more descriptive error messages for Connect login
		switch (data->ResultCode) {
			case EOS_EResult::EOS_InvalidParameters:
				error_msg += "Invalid parameters (10) - Connect login requires valid Epic Account ID from Auth login";
				break;
			case EOS_EResult::EOS_InvalidUser:
				error_msg += "Invalid user (3) - User may need to be linked or created in Connect service";
				break;
			case EOS_EResult::EOS_NotFound:
				error_msg += "User not found (13) - User account may need to be created in Connect service";
				break;
			case EOS_EResult::EOS_DuplicateNotAllowed:
				error_msg += "Duplicate not allowed (15) - User may already be logged in";
				break;
			case EOS_EResult::EOS_Connect_ExternalTokenValidationFailed:
				error_msg += "External token validation failed (7000) - Epic Account ID token was rejected by Connect service. Try using Auth Token instead of Account ID";
				break;
			case EOS_EResult::EOS_Connect_InvalidToken:
				error_msg += "Invalid token (7003) - The provided token is not valid for Connect service";
				break;
			case EOS_EResult::EOS_Connect_UnsupportedTokenType:
				error_msg += "Unsupported token type (7004) - Connect service doesn't support this token type";
				break;
			case EOS_EResult::EOS_Connect_AuthExpired:
				error_msg += "Auth expired (7002) - The authentication token has expired";
				break;
			default:
				error_msg += String::num_int64(static_cast<int64_t>(data->ResultCode));
				break;
		}		ERR_PRINT(error_msg);

		// Connect failed, but Auth succeeded - still emit login signal but without product_user_id
		ERR_PRINT("Login completed with Epic Account only (no cross-platform features)");
		Dictionary user_info;
		user_info["display_name"] = instance->current_username;
		user_info["epic_account_id"] = instance->get_epic_account_id();
		user_info["product_user_id"] = "";  // Empty since Connect failed

		instance->emit_signal("login_completed", true, user_info);
	}
}

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

// Achievements callbacks
void EOS_CALL GodotEpic::achievements_query_definitions_callback(const EOS_Achievements_OnQueryDefinitionsCompleteCallbackInfo* data) {
	if (!data || !instance) {
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		WARN_PRINT("Achievement definitions query successful");
		instance->achievements_definitions_cached = true;

		// Get updated achievement definitions and emit signal
		Array definitions = instance->get_achievement_definitions();
		instance->emit_signal("achievement_definitions_updated", definitions);
	} else {
		String error_msg = "Achievement definitions query failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
		ERR_PRINT(error_msg);
		instance->achievements_definitions_cached = false;

		// Emit empty array on failure
		Array empty_definitions;
		instance->emit_signal("achievement_definitions_updated", empty_definitions);
	}
}

void EOS_CALL GodotEpic::achievements_query_player_callback(const EOS_Achievements_OnQueryPlayerAchievementsCompleteCallbackInfo* data) {
	if (!data || !instance) {
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		WARN_PRINT("Player achievements query successful");
		instance->player_achievements_cached = true;

		// Get updated player achievements and emit signal
		Array achievements = instance->get_player_achievements();
		instance->emit_signal("player_achievements_updated", achievements);
	} else {
		String error_msg = "Player achievements query failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
		ERR_PRINT(error_msg);
		instance->player_achievements_cached = false;

		// Emit empty array on failure
		Array empty_achievements;
		instance->emit_signal("player_achievements_updated", empty_achievements);
	}
}

void EOS_CALL GodotEpic::achievements_unlock_callback(const EOS_Achievements_OnUnlockAchievementsCompleteCallbackInfo* data) {
	if (!data || !instance) {
		return;
	}

	if (data->ResultCode == EOS_EResult::EOS_Success) {
		String success_msg = "Achievements unlocked successfully: " + String::num_int64(data->AchievementsCount) + " achievements";
		WARN_PRINT(success_msg);

		// We don't know which specific achievements were unlocked from this callback,
		// but we can trigger a refresh of player achievements
		instance->query_player_achievements();

		// Emit generic unlock signal
		Array unlocked_ids; // We'd need to track which IDs we sent to populate this properly
		instance->emit_signal("achievements_unlocked", unlocked_ids);
	} else {
		String error_msg = "Achievement unlock failed: " + String::num_int64(static_cast<int64_t>(data->ResultCode));
		ERR_PRINT(error_msg);

		// Emit empty array on failure
		Array empty_unlocked;
		instance->emit_signal("achievements_unlocked", empty_unlocked);
	}
}

void EOS_CALL GodotEpic::achievements_unlocked_notification(const EOS_Achievements_OnAchievementsUnlockedCallbackV2Info* data) {
	if (!data || !instance) {
		return;
	}

	String achievement_id = String(data->AchievementId ? data->AchievementId : "");
	int64_t unlock_time = data->UnlockTime;

	WARN_PRINT("Achievement unlocked notification: " + achievement_id);

	// Emit specific achievement unlock signal
	instance->emit_signal("achievement_unlocked", achievement_id, unlock_time);

	// Also refresh player achievements to get updated progress
	instance->query_player_achievements();
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

Dictionary GodotEpic::test_subsystem_manager() {
	WARN_PRINT("Testing SubsystemManager functionality...");

	Dictionary test_results;
	Array test_messages;
	bool all_tests_passed = true;

	// Test 1: Get singleton instance
	SubsystemManager* manager = SubsystemManager::GetInstance();
	if (manager) {
		test_messages.append("✅ SubsystemManager singleton created successfully");
		test_results["singleton_created"] = true;
	} else {
		test_messages.append("❌ Failed to get SubsystemManager singleton");
		test_results["singleton_created"] = false;
		all_tests_passed = false;
		test_results["all_tests_passed"] = all_tests_passed;
		test_results["test_messages"] = test_messages;
		return test_results;
	}

	// Test 2: Check current state (should be initialized with subsystems)
	bool is_initialized = manager->IsInitialized();
	if (is_initialized) {
		test_messages.append("✅ SubsystemManager is initialized (as expected after platform init)");
		test_results["is_initialized"] = true;
	} else {
		test_messages.append("ℹ️  SubsystemManager not initialized (platform may not be initialized yet)");
		test_results["is_initialized"] = false;
	}

	int subsystem_count = manager->GetSubsystemCount();
	test_results["subsystem_count"] = subsystem_count;
	if (subsystem_count > 0) {
		test_messages.append("✅ SubsystemManager has " + String::num_int64(subsystem_count) + " subsystems registered");
		test_results["has_subsystems"] = true;
	} else {
		test_messages.append("ℹ️  SubsystemManager has no subsystems (platform may not be initialized yet)");
		test_results["has_subsystems"] = false;
	}

	// Test 3: Test subsystem access
	auto platform_subsystem = manager->GetSubsystem<IPlatformSubsystem>();
	if (platform_subsystem) {
		test_messages.append("✅ PlatformSubsystem accessible via GetSubsystem<IPlatformSubsystem>()");
		test_results["platform_subsystem_accessible"] = true;
		if (platform_subsystem->IsOnline()) {
			test_messages.append("✅ PlatformSubsystem reports online status");
			test_results["platform_online"] = true;
		} else {
			test_messages.append("ℹ️  PlatformSubsystem reports offline (may be expected)");
			test_results["platform_online"] = false;
		}
	} else {
		test_messages.append("❌ PlatformSubsystem not accessible");
		test_results["platform_subsystem_accessible"] = false;
		all_tests_passed = false;
	}

	auto auth_subsystem = manager->GetSubsystem<IAuthenticationSubsystem>();
	if (auth_subsystem) {
		test_messages.append("✅ AuthenticationSubsystem accessible via GetSubsystem<IAuthenticationSubsystem>()");
		test_results["auth_subsystem_accessible"] = true;
	} else {
		test_messages.append("ℹ️  AuthenticationSubsystem not accessible (may not be initialized yet)");
		test_results["auth_subsystem_accessible"] = false;
	}

	auto achievements_subsystem = manager->GetSubsystem<IAchievementsSubsystem>();
	if (achievements_subsystem) {
		test_messages.append("✅ AchievementsSubsystem accessible via GetSubsystem<IAchievementsSubsystem>()");
		test_results["achievements_subsystem_accessible"] = true;
	} else {
		test_messages.append("ℹ️  AchievementsSubsystem not accessible (may not be initialized yet)");
		test_results["achievements_subsystem_accessible"] = false;
	}

	auto leaderboards_subsystem = manager->GetSubsystem<ILeaderboardsSubsystem>();
	if (leaderboards_subsystem) {
		test_messages.append("✅ LeaderboardsSubsystem accessible via GetSubsystem<ILeaderboardsSubsystem>()");
		test_results["leaderboards_subsystem_accessible"] = true;
	} else {
		test_messages.append("ℹ️  LeaderboardsSubsystem not accessible (may not be initialized yet)");
		test_results["leaderboards_subsystem_accessible"] = false;
	}

	// Test 4: Tick subsystems (should not crash)
	manager->TickAll(0.016f); // 60 FPS delta
	test_messages.append("✅ TickAll() completed without crash");
	test_results["tick_completed"] = true;

	// Test 5: Test idempotent operations
	bool init_result = manager->InitializeAll();
	if (init_result) {
		test_messages.append("✅ InitializeAll() succeeds when called multiple times (idempotent)");
		test_results["idempotent_init"] = true;
	} else {
		test_messages.append("❌ InitializeAll() failed on subsequent call");
		test_results["idempotent_init"] = false;
		all_tests_passed = false;
	}

	// Note: We don't test ShutdownAll() here as it would shut down the actual subsystems
	// that are needed for the application to function

	test_messages.append("SubsystemManager integration test completed!");
	test_results["all_tests_passed"] = all_tests_passed;
	test_results["test_messages"] = test_messages;

	WARN_PRINT("SubsystemManager integration test completed!");

	return test_results;
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