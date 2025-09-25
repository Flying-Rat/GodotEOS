#include "godotepic.h"
#include "Platform.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

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

	// Signals
	ADD_SIGNAL(MethodInfo("login_completed", PropertyInfo(Variant::BOOL, "success"), PropertyInfo(Variant::DICTIONARY, "user_info")));
	ADD_SIGNAL(MethodInfo("logout_completed", PropertyInfo(Variant::BOOL, "success")));
	ADD_SIGNAL(MethodInfo("friends_updated", PropertyInfo(Variant::ARRAY, "friends_list")));
	ADD_SIGNAL(MethodInfo("achievement_definitions_updated", PropertyInfo(Variant::ARRAY, "definitions")));
	ADD_SIGNAL(MethodInfo("player_achievements_updated", PropertyInfo(Variant::ARRAY, "achievements")));
	ADD_SIGNAL(MethodInfo("achievements_unlocked", PropertyInfo(Variant::ARRAY, "unlocked_achievement_ids")));
	ADD_SIGNAL(MethodInfo("achievement_unlocked", PropertyInfo(Variant::STRING, "achievement_id"), PropertyInfo(Variant::INT, "unlock_time")));
}

GodotEpic::GodotEpic() {
	// Initialize any variables here.
	time_passed = 0.0;
	instance = this;

	// Initialize platform instance
	platform_instance = nullptr;

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
	if (platform_instance && platform_instance->is_initialized()) {
		ERR_PRINT("EOS Platform already initialized");
		return true;
	}

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

	// Create platform instance
	platform_instance = std::make_unique<godot::Platform>();

	// Setup logging
	EOS_EResult LogResult = EOS_Logging_SetCallback(logging_callback);
	if (LogResult == EOS_EResult::EOS_Success) {
		EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES,
							   EOS_ELogLevel::EOS_LOG_Verbose);
	}

	// Initialize platform
	if (!platform_instance->initialize(init_options)) {
		platform_instance.reset();
		return false;
	}

	// Register for achievement unlock notifications
	EOS_HAchievements achievements_handle = EOS_Platform_GetAchievementsInterface(platform_instance->get_platform_handle());
	if (achievements_handle) {
		EOS_Achievements_AddNotifyAchievementsUnlockedV2Options notify_options = {};
		notify_options.ApiVersion = EOS_ACHIEVEMENTS_ADDNOTIFYACHIEVEMENTSUNLOCKEDV2_API_LATEST;

		achievements_notification_id = EOS_Achievements_AddNotifyAchievementsUnlockedV2(
			achievements_handle,
			&notify_options,
			nullptr,
			achievements_unlocked_notification
		);

		if (achievements_notification_id != EOS_INVALID_NOTIFICATIONID) {
			WARN_PRINT("Achievement unlock notifications registered successfully");
		} else {
			WARN_PRINT("Failed to register achievement unlock notifications");
		}
	}

	return true;
}

void GodotEpic::shutdown_platform() {
	if (!platform_instance) {
		return;
	}

	// Unregister achievement notifications before shutdown
	if (platform_instance->get_platform_handle() && achievements_notification_id != EOS_INVALID_NOTIFICATIONID) {
		EOS_HAchievements achievements_handle = EOS_Platform_GetAchievementsInterface(platform_instance->get_platform_handle());
		if (achievements_handle) {
			EOS_Achievements_RemoveNotifyAchievementsUnlocked(achievements_handle, achievements_notification_id);
			WARN_PRINT("Achievement unlock notifications unregistered");
		}
		achievements_notification_id = EOS_INVALID_NOTIFICATIONID;
	}

	platform_instance->shutdown();
	platform_instance.reset();
}

void GodotEpic::tick() {
	if (platform_instance && platform_instance->get_platform_handle()) {
		EOS_Platform_Tick(platform_instance->get_platform_handle());
	}
}

bool GodotEpic::is_platform_initialized() const {
	return platform_instance && platform_instance->is_initialized();
}

EOS_HPlatform GodotEpic::get_platform_handle() const {
	return platform_instance ? platform_instance->get_platform_handle() : nullptr;
}

// Authentication methods
void GodotEpic::login_with_epic_account(const String& email, const String& password) {
	if (!platform_instance || !platform_instance->get_platform_handle()) {
		ERR_PRINT("EOS Platform not initialized");
		return;
	}

	EOS_HAuth auth_handle = EOS_Platform_GetAuthInterface(platform_instance->get_platform_handle());
	if (!auth_handle) {
		ERR_PRINT("Failed to get Auth interface");
		return;
	}

	EOS_Auth_LoginOptions login_options = {};
	login_options.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;

	// Try different credential types based on parameters
	EOS_Auth_Credentials credentials = {};
	credentials.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;

	if (!email.is_empty() && !password.is_empty()) {
		// Use email/password if provided
		credentials.Type = EOS_ELoginCredentialType::EOS_LCT_Password;
		credentials.Id = email.utf8().get_data();
		credentials.Token = password.utf8().get_data();
		ERR_PRINT("Epic Account login with email/password initiated");
	} else {
		// Use Account Portal (browser/launcher) - requires proper EOS setup
		credentials.Type = EOS_ELoginCredentialType::EOS_LCT_AccountPortal;
		credentials.Id = nullptr;
		credentials.Token = nullptr;
		ERR_PRINT("Epic Account login with Account Portal initiated - check browser/Epic launcher");
	}

	login_options.Credentials = &credentials;
	login_options.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList | EOS_EAuthScopeFlags::EOS_AS_Presence;

	EOS_Auth_Login(auth_handle, &login_options, nullptr, auth_login_callback);
}

void GodotEpic::login_with_device_id(const String& display_name) {
	if (!platform_instance || !platform_instance->get_platform_handle()) {
		ERR_PRINT("EOS Platform not initialized");
		return;
	}

	EOS_HAuth auth_handle = EOS_Platform_GetAuthInterface(platform_instance->get_platform_handle());
	if (!auth_handle) {
		ERR_PRINT("Failed to get Auth interface");
		return;
	}

	EOS_Auth_LoginOptions login_options = {};
	login_options.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;

	// Set up credentials for Device ID (development only)
	EOS_Auth_Credentials credentials = {};
	credentials.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
	credentials.Type = EOS_ELoginCredentialType::EOS_LCT_Developer;
	credentials.Id = "localhost:7777";  // Default EOS Dev Auth Tool port

	// Use display_name as credential name, or fallback to a default
	String credential_name = display_name.is_empty() ? "TestUser" : display_name;
	credentials.Token = credential_name.utf8().get_data();

	login_options.Credentials = &credentials;
	login_options.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile; // | EOS_EAuthScopeFlags::EOS_AS_FriendsList;

	EOS_Auth_Login(auth_handle, &login_options, nullptr, auth_login_callback);
	WARN_PRINT("Device ID login initiated with credential: " + credential_name);
}

void GodotEpic::logout() {
	if (!platform_instance || !platform_instance->get_platform_handle() || !is_logged_in) {
		ERR_PRINT("Not logged in or platform not initialized");
		return;
	}

	EOS_HAuth auth_handle = EOS_Platform_GetAuthInterface(platform_instance->get_platform_handle());
	if (!auth_handle) {
		ERR_PRINT("Failed to get Auth interface");
		return;
	}

	EOS_Auth_LogoutOptions logout_options = {};
	logout_options.ApiVersion = EOS_AUTH_LOGOUT_API_LATEST;
	logout_options.LocalUserId = epic_account_id;

	EOS_Auth_Logout(auth_handle, &logout_options, nullptr, auth_logout_callback);
	ERR_PRINT("Logout initiated");
}

bool GodotEpic::is_user_logged_in() const {
	return is_logged_in;
}

String GodotEpic::get_current_username() const {
	return current_username;
}

String GodotEpic::get_epic_account_id() const {
	if (!epic_account_id) {
		return "";
	}

	static char account_id_str[EOS_EPICACCOUNTID_MAX_LENGTH + 1];
	int32_t buffer_size = sizeof(account_id_str);

	EOS_EResult result = EOS_EpicAccountId_ToString(epic_account_id, account_id_str, &buffer_size);
	if (result == EOS_EResult::EOS_Success) {
		return String::utf8(account_id_str);
	}

	return "";
}

String GodotEpic::get_product_user_id() const {
	if (!product_user_id) {
		return "";
	}

	static char user_id_str[EOS_PRODUCTUSERID_MAX_LENGTH + 1];
	int32_t buffer_size = sizeof(user_id_str);

	EOS_EResult result = EOS_ProductUserId_ToString(product_user_id, user_id_str, &buffer_size);
	if (result == EOS_EResult::EOS_Success) {
		return String::utf8(user_id_str);
	}

	return "";
}

// Friends methods
void GodotEpic::query_friends() {
	if (!platform_instance || !platform_instance->get_platform_handle() || !epic_account_id) {
		ERR_PRINT("Platform not initialized or user not logged in");
		return;
	}

	EOS_HFriends friends_handle = EOS_Platform_GetFriendsInterface(platform_instance->get_platform_handle());
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

	if (!platform_instance || !platform_instance->get_platform_handle() || !epic_account_id) {
		ERR_PRINT("Platform not initialized or user not logged in");
		return friends_array;
	}

	EOS_HFriends friends_handle = EOS_Platform_GetFriendsInterface(platform_instance->get_platform_handle());
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

	if (!platform_instance || !platform_instance->get_platform_handle() || !epic_account_id) {
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
	if (!platform_instance || !platform_instance->get_platform_handle() || !product_user_id) {
		ERR_PRINT("Platform not initialized or user not logged in");
		return;
	}

	EOS_HAchievements achievements_handle = EOS_Platform_GetAchievementsInterface(platform_instance->get_platform_handle());
	if (!achievements_handle) {
		ERR_PRINT("Failed to get Achievements interface");
		return;
	}

	EOS_Achievements_QueryDefinitionsOptions query_options = {};
	query_options.ApiVersion = EOS_ACHIEVEMENTS_QUERYDEFINITIONS_API_LATEST;
	query_options.LocalUserId = product_user_id;
	query_options.EpicUserId_DEPRECATED = nullptr;
	query_options.HiddenAchievementIds_DEPRECATED = nullptr;
	query_options.HiddenAchievementsCount_DEPRECATED = 0;

	EOS_Achievements_QueryDefinitions(achievements_handle, &query_options, nullptr, achievements_query_definitions_callback);
	WARN_PRINT("Achievement definitions query initiated");
}

void GodotEpic::query_player_achievements() {
	if (!platform_instance || !platform_instance->get_platform_handle() || !product_user_id) {
		ERR_PRINT("Platform not initialized or user not logged in");
		return;
	}

	EOS_HAchievements achievements_handle = EOS_Platform_GetAchievementsInterface(platform_instance->get_platform_handle());
	if (!achievements_handle) {
		ERR_PRINT("Failed to get Achievements interface");
		return;
	}

	EOS_Achievements_QueryPlayerAchievementsOptions query_options = {};
	query_options.ApiVersion = EOS_ACHIEVEMENTS_QUERYPLAYERACHIEVEMENTS_API_LATEST;
	query_options.TargetUserId = product_user_id;
	query_options.LocalUserId = product_user_id;

	EOS_Achievements_QueryPlayerAchievements(achievements_handle, &query_options, nullptr, achievements_query_player_callback);
	WARN_PRINT("Player achievements query initiated");
}

void GodotEpic::unlock_achievement(const String& achievement_id) {
	Array achievement_ids;
	achievement_ids.push_back(achievement_id);
	unlock_achievements(achievement_ids);
}

void GodotEpic::unlock_achievements(const Array& achievement_ids) {
	if (!platform_instance || !platform_instance->get_platform_handle() || !product_user_id) {
		ERR_PRINT("Platform not initialized or user not logged in");
		return;
	}

	if (achievement_ids.size() == 0) {
		ERR_PRINT("No achievement IDs provided");
		return;
	}

	EOS_HAchievements achievements_handle = EOS_Platform_GetAchievementsInterface(platform_instance->get_platform_handle());
	if (!achievements_handle) {
		ERR_PRINT("Failed to get Achievements interface");
		return;
	}

	// Convert Godot Array to C array
	std::vector<const char*> achievement_id_ptrs;
	std::vector<String> achievement_id_strings;

	for (int i = 0; i < achievement_ids.size(); i++) {
		String achievement_id = achievement_ids[i];
		achievement_id_strings.push_back(achievement_id);
		achievement_id_ptrs.push_back(achievement_id_strings[i].utf8().get_data());
	}

	EOS_Achievements_UnlockAchievementsOptions unlock_options = {};
	unlock_options.ApiVersion = EOS_ACHIEVEMENTS_UNLOCKACHIEVEMENTS_API_LATEST;
	unlock_options.UserId = product_user_id;
	unlock_options.AchievementIds = achievement_id_ptrs.data();
	unlock_options.AchievementsCount = achievement_ids.size();

	EOS_Achievements_UnlockAchievements(achievements_handle, &unlock_options, nullptr, achievements_unlock_callback);
	WARN_PRINT("Achievement unlock initiated for " + String::num_int64(achievement_ids.size()) + " achievements");
}

Array GodotEpic::get_achievement_definitions() {
	Array definitions_array;

	if (!platform_instance || !platform_instance->get_platform_handle()) {
		ERR_PRINT("Platform not initialized");
		return definitions_array;
	}

	EOS_HAchievements achievements_handle = EOS_Platform_GetAchievementsInterface(platform_instance->get_platform_handle());
	if (!achievements_handle) {
		ERR_PRINT("Failed to get Achievements interface");
		return definitions_array;
	}

	EOS_Achievements_GetAchievementDefinitionCountOptions count_options = {};
	count_options.ApiVersion = EOS_ACHIEVEMENTS_GETACHIEVEMENTDEFINITIONCOUNT_API_LATEST;

	uint32_t definitions_count = EOS_Achievements_GetAchievementDefinitionCount(achievements_handle, &count_options);

	for (uint32_t i = 0; i < definitions_count; i++) {
		EOS_Achievements_CopyAchievementDefinitionV2ByIndexOptions copy_options = {};
		copy_options.ApiVersion = EOS_ACHIEVEMENTS_COPYACHIEVEMENTDEFINITIONV2BYINDEX_API_LATEST;
		copy_options.AchievementIndex = i;

		EOS_Achievements_DefinitionV2* definition = nullptr;
		EOS_EResult result = EOS_Achievements_CopyAchievementDefinitionV2ByIndex(achievements_handle, &copy_options, &definition);

		if (result == EOS_EResult::EOS_Success && definition) {
			Dictionary definition_dict;
			definition_dict["achievement_id"] = String(definition->AchievementId ? definition->AchievementId : "");
			definition_dict["unlocked_display_name"] = String(definition->UnlockedDisplayName ? definition->UnlockedDisplayName : "");
			definition_dict["unlocked_description"] = String(definition->UnlockedDescription ? definition->UnlockedDescription : "");
			definition_dict["locked_display_name"] = String(definition->LockedDisplayName ? definition->LockedDisplayName : "");
			definition_dict["locked_description"] = String(definition->LockedDescription ? definition->LockedDescription : "");
			definition_dict["flavor_text"] = String(definition->FlavorText ? definition->FlavorText : "");
			definition_dict["unlocked_icon_url"] = String(definition->UnlockedIconURL ? definition->UnlockedIconURL : "");
			definition_dict["locked_icon_url"] = String(definition->LockedIconURL ? definition->LockedIconURL : "");
			definition_dict["is_hidden"] = definition->bIsHidden == EOS_TRUE;

			// Add stat thresholds
			Array stat_thresholds;
			for (uint32_t j = 0; j < definition->StatThresholdsCount; j++) {
				Dictionary threshold_dict;
				threshold_dict["name"] = String(definition->StatThresholds[j].Name ? definition->StatThresholds[j].Name : "");
				threshold_dict["threshold"] = definition->StatThresholds[j].Threshold;
				stat_thresholds.push_back(threshold_dict);
			}
			definition_dict["stat_thresholds"] = stat_thresholds;

			definitions_array.push_back(definition_dict);

			// Release the definition
			EOS_Achievements_DefinitionV2_Release(definition);
		}
	}

	return definitions_array;
}

Array GodotEpic::get_player_achievements() {
	Array achievements_array;

	if (!platform_instance || !platform_instance->get_platform_handle() || !product_user_id) {
		ERR_PRINT("Platform not initialized or user not logged in");
		return achievements_array;
	}

	EOS_HAchievements achievements_handle = EOS_Platform_GetAchievementsInterface(platform_instance->get_platform_handle());
	if (!achievements_handle) {
		ERR_PRINT("Failed to get Achievements interface");
		return achievements_array;
	}

	EOS_Achievements_GetPlayerAchievementCountOptions count_options = {};
	count_options.ApiVersion = EOS_ACHIEVEMENTS_GETPLAYERACHIEVEMENTCOUNT_API_LATEST;
	count_options.UserId = product_user_id;

	uint32_t achievements_count = EOS_Achievements_GetPlayerAchievementCount(achievements_handle, &count_options);

	for (uint32_t i = 0; i < achievements_count; i++) {
		EOS_Achievements_CopyPlayerAchievementByIndexOptions copy_options = {};
		copy_options.ApiVersion = EOS_ACHIEVEMENTS_COPYPLAYERACHIEVEMENTBYINDEX_API_LATEST;
		copy_options.TargetUserId = product_user_id;
		copy_options.AchievementIndex = i;
		copy_options.LocalUserId = product_user_id;

		EOS_Achievements_PlayerAchievement* achievement = nullptr;
		EOS_EResult result = EOS_Achievements_CopyPlayerAchievementByIndex(achievements_handle, &copy_options, &achievement);

		if (result == EOS_EResult::EOS_Success && achievement) {
			Dictionary achievement_dict;
			achievement_dict["achievement_id"] = String(achievement->AchievementId ? achievement->AchievementId : "");
			achievement_dict["progress"] = achievement->Progress;
			achievement_dict["unlock_time"] = achievement->UnlockTime;
			achievement_dict["is_unlocked"] = achievement->UnlockTime != EOS_ACHIEVEMENTS_ACHIEVEMENT_UNLOCKTIME_UNDEFINED;
			achievement_dict["display_name"] = String(achievement->DisplayName ? achievement->DisplayName : "");
			achievement_dict["description"] = String(achievement->Description ? achievement->Description : "");
			achievement_dict["icon_url"] = String(achievement->IconURL ? achievement->IconURL : "");
			achievement_dict["flavor_text"] = String(achievement->FlavorText ? achievement->FlavorText : "");

			// Add stat info
			Array stat_info;
			for (int32_t j = 0; j < achievement->StatInfoCount; j++) {
				Dictionary stat_dict;
				stat_dict["name"] = String(achievement->StatInfo[j].Name ? achievement->StatInfo[j].Name : "");
				stat_dict["current_value"] = achievement->StatInfo[j].CurrentValue;
				stat_dict["threshold_value"] = achievement->StatInfo[j].ThresholdValue;
				stat_info.push_back(stat_dict);
			}
			achievement_dict["stat_info"] = stat_info;

			achievements_array.push_back(achievement_dict);

			// Release the achievement
			EOS_Achievements_PlayerAchievement_Release(achievement);
		}
	}

	return achievements_array;
}

Dictionary GodotEpic::get_achievement_definition(const String& achievement_id) {
	Dictionary definition_dict;

	if (!platform_instance || !platform_instance->get_platform_handle()) {
		ERR_PRINT("Platform not initialized");
		return definition_dict;
	}

	EOS_HAchievements achievements_handle = EOS_Platform_GetAchievementsInterface(platform_instance->get_platform_handle());
	if (!achievements_handle) {
		ERR_PRINT("Failed to get Achievements interface");
		return definition_dict;
	}

	EOS_Achievements_CopyAchievementDefinitionV2ByAchievementIdOptions copy_options = {};
	copy_options.ApiVersion = EOS_ACHIEVEMENTS_COPYACHIEVEMENTDEFINITIONV2BYACHIEVEMENTID_API_LATEST;
	copy_options.AchievementId = achievement_id.utf8().get_data();

	EOS_Achievements_DefinitionV2* definition = nullptr;
	EOS_EResult result = EOS_Achievements_CopyAchievementDefinitionV2ByAchievementId(achievements_handle, &copy_options, &definition);

	if (result == EOS_EResult::EOS_Success && definition) {
		definition_dict["achievement_id"] = String(definition->AchievementId ? definition->AchievementId : "");
		definition_dict["unlocked_display_name"] = String(definition->UnlockedDisplayName ? definition->UnlockedDisplayName : "");
		definition_dict["unlocked_description"] = String(definition->UnlockedDescription ? definition->UnlockedDescription : "");
		definition_dict["locked_display_name"] = String(definition->LockedDisplayName ? definition->LockedDisplayName : "");
		definition_dict["locked_description"] = String(definition->LockedDescription ? definition->LockedDescription : "");
		definition_dict["flavor_text"] = String(definition->FlavorText ? definition->FlavorText : "");
		definition_dict["unlocked_icon_url"] = String(definition->UnlockedIconURL ? definition->UnlockedIconURL : "");
		definition_dict["locked_icon_url"] = String(definition->LockedIconURL ? definition->LockedIconURL : "");
		definition_dict["is_hidden"] = definition->bIsHidden == EOS_TRUE;

		// Add stat thresholds
		Array stat_thresholds;
		for (uint32_t j = 0; j < definition->StatThresholdsCount; j++) {
			Dictionary threshold_dict;
			threshold_dict["name"] = String(definition->StatThresholds[j].Name ? definition->StatThresholds[j].Name : "");
			threshold_dict["threshold"] = definition->StatThresholds[j].Threshold;
			stat_thresholds.push_back(threshold_dict);
		}
		definition_dict["stat_thresholds"] = stat_thresholds;

		// Release the definition
		EOS_Achievements_DefinitionV2_Release(definition);
	} else {
		ERR_PRINT("Failed to get achievement definition for ID: " + achievement_id);
	}

	return definition_dict;
}

Dictionary GodotEpic::get_player_achievement(const String& achievement_id) {
	Dictionary achievement_dict;

	if (!platform_instance || !platform_instance->get_platform_handle() || !product_user_id) {
		ERR_PRINT("Platform not initialized or user not logged in");
		return achievement_dict;
	}

	EOS_HAchievements achievements_handle = EOS_Platform_GetAchievementsInterface(platform_instance->get_platform_handle());
	if (!achievements_handle) {
		ERR_PRINT("Failed to get Achievements interface");
		return achievement_dict;
	}

	EOS_Achievements_CopyPlayerAchievementByAchievementIdOptions copy_options = {};
	copy_options.ApiVersion = EOS_ACHIEVEMENTS_COPYPLAYERACHIEVEMENTBYACHIEVEMENTID_API_LATEST;
	copy_options.TargetUserId = product_user_id;
	copy_options.AchievementId = achievement_id.utf8().get_data();
	copy_options.LocalUserId = product_user_id;

	EOS_Achievements_PlayerAchievement* achievement = nullptr;
	EOS_EResult result = EOS_Achievements_CopyPlayerAchievementByAchievementId(achievements_handle, &copy_options, &achievement);

	if (result == EOS_EResult::EOS_Success && achievement) {
		achievement_dict["achievement_id"] = String(achievement->AchievementId ? achievement->AchievementId : "");
		achievement_dict["progress"] = achievement->Progress;
		achievement_dict["unlock_time"] = achievement->UnlockTime;
		achievement_dict["is_unlocked"] = achievement->UnlockTime != EOS_ACHIEVEMENTS_ACHIEVEMENT_UNLOCKTIME_UNDEFINED;
		achievement_dict["display_name"] = String(achievement->DisplayName ? achievement->DisplayName : "");
		achievement_dict["description"] = String(achievement->Description ? achievement->Description : "");
		achievement_dict["icon_url"] = String(achievement->IconURL ? achievement->IconURL : "");
		achievement_dict["flavor_text"] = String(achievement->FlavorText ? achievement->FlavorText : "");

		// Add stat info
		Array stat_info;
		for (int32_t j = 0; j < achievement->StatInfoCount; j++) {
			Dictionary stat_dict;
			stat_dict["name"] = String(achievement->StatInfo[j].Name ? achievement->StatInfo[j].Name : "");
			stat_dict["current_value"] = achievement->StatInfo[j].CurrentValue;
			stat_dict["threshold_value"] = achievement->StatInfo[j].ThresholdValue;
			stat_info.push_back(stat_dict);
		}
		achievement_dict["stat_info"] = stat_info;

		// Release the achievement
		EOS_Achievements_PlayerAchievement_Release(achievement);
	} else {
		ERR_PRINT("Failed to get player achievement for ID: " + achievement_id);
	}

	return achievement_dict;
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
		EOS_HAuth auth_handle = EOS_Platform_GetAuthInterface(instance->platform_instance->get_platform_handle());
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
		EOS_HConnect connect_handle = EOS_Platform_GetConnectInterface(instance->platform_instance->get_platform_handle());
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