extends Node

# EpicOS Singleton - Main interface for Epic Online Services
# Integrates with the GodotEOS GDExtension for actual EOS SDK functionality

# Authentication signals
signal login_completed(success: bool, user_info: Dictionary)
signal logout_completed(success: bool)

# Achievement signals
signal achievement_definitions_completed(success: bool, definitions: Array)
signal player_achievements_completed(success: bool, achievements: Array)
signal achievements_unlocked_completed(success: bool, unlocked_achievement_ids: Array)
signal achievement_stats_completed(success: bool, stats: Array)
signal stats_ingested(success: bool, stat_names: Array)

# Leaderboard signals
signal leaderboard_definitions_completed(success: bool, definitions: Array)
signal leaderboard_ranks_completed(success: bool, ranks: Array)
signal leaderboard_user_scores_completed(success: bool, user_scores: Dictionary)

# Friends signals
signal friends_query_completed(success: bool, friends_list: Array)

# User Info signals
signal user_info_query_completed(success: bool, user_info: Dictionary)

var _godot_epic: GodotEOS = null
var _initialized: bool = false
var _debug_mode: bool = false

func _log(message: String) -> void:
	print("[GodotEOS] EpicOS: ", message)

func _log_error(message: String) -> void:
	printerr("[GodotEOS] EpicOS: ", message)


func _ready():
	_log("Initializing Epic Online Services...")
	# Get the GodotEOS singleton from the GDExtension
	_godot_epic = GodotEOS.get_singleton()
	if not _godot_epic:
		_log_error("GodotEOS GDExtension not found!")
		_log("Make sure the GodotEOS extension is properly installed and enabled.")
		return

	_log("GodotEOS GDExtension found successfully")
	_setup_signal_connections()

func _setup_signal_connections():
	"""Connect to GDExtension signals"""
	if _godot_epic:
		# Connect authentication signals
		_godot_epic.connect("login_completed", _on_authentication_completed)
		_godot_epic.connect("logout_completed", _on_logout_completed)

		# Connect achievement signals
		_godot_epic.connect("achievement_definitions_updated", _on_achievement_definitions_completed)
		_godot_epic.connect("player_achievements_updated", _on_player_achievements_completed)
		_godot_epic.connect("achievements_unlocked", _on_achievements_unlocked_completed)
		_godot_epic.connect("achievement_stats_updated", _on_achievement_stats_completed)
		_godot_epic.connect("stats_ingested", _on_stats_ingested)

		# Connect leaderboard signals
		_godot_epic.connect("leaderboard_definitions_updated", _on_leaderboard_definitions_completed)
		_godot_epic.connect("leaderboard_ranks_updated", _on_leaderboard_ranks_completed)
		_godot_epic.connect("leaderboard_user_scores_updated", _on_leaderboard_user_scores_completed)

		# Connect friends signals
		_godot_epic.connect("friends_query_completed", _on_friends_query_completed)

		# Connect user info signals
		_godot_epic.connect("user_info_updated", _on_user_info_query_completed)

func initialize(config: Dictionary = {}) -> bool:
	"""Initialize the EOS SDK with configuration options"""
	if _debug_mode:
		_log("initialize() called")

	if not _godot_epic:
		_log_error("GodotEOS GDExtension not available")
		return false

	if _initialized:
		_log("Already initialized")
		return true

	# Default configuration - override with your actual Epic credentials
	var default_config = {
		"product_name": "GodotEOS",
		"product_version": "1.0.0",
		"product_id": "",  # Required: Get from Epic Developer Portal
		"sandbox_id": "",  # Required: Get from Epic Developer Portal
		"deployment_id": "",  # Required: Get from Epic Developer Portal
		"client_id": "",  # Required: Get from Epic Developer Portal
		"client_secret": "",  # Required: Get from Epic Developer Portal
		"encryption_key": ""  # Optional: omit, or exactly 64 hexadecimal characters
	}

	# Merge user config with defaults
	for key in config:
		default_config[key] = config[key]

	# Validate required fields
	var required_fields = ["product_id", "sandbox_id", "deployment_id", "client_id", "client_secret"]
	for field in required_fields:
		if default_config[field] == "":
			_log_error("Missing required field: " + str(field))
			_log("Please provide all Epic credentials in the configuration")
			return false

	var encryption_key := str(default_config["encryption_key"])
	if not encryption_key.is_empty() and (encryption_key.length() != 64 or not encryption_key.is_valid_hex_number()):
		_log_error("encryption_key is optional; if set, it must be exactly 64 hexadecimal characters (0-9, a-f)")
		_log("Got encryption_key length: " + str(encryption_key.length()))
		return false

	_log("Initializing EOS SDK...")
	_initialized = _godot_epic.initialize_platform(default_config)

	if _initialized:
		_log("EOS SDK initialized successfully")
	else:
		_log_error("Failed to initialize EOS SDK")
		_log("Check the Godot output above for the specific field (product_id, sandbox_id, deployment_id, client_id, client_secret, or encryption_key).")

	return _initialized

func set_debug_mode(enabled: bool):
	"""Enable or disable debug logging"""
	_debug_mode = enabled
	_log("Debug mode " + ("enabled" if enabled else "disabled"))

func tick(delta: float = 0.0):
	"""Tick the EOS platform - should be called every frame"""
	if _godot_epic and _initialized:
		_godot_epic.tick(delta)

func is_platform_initialized() -> bool:
	"""Check if the EOS platform is initialized"""
	if _godot_epic:
		return _godot_epic.is_platform_initialized()
	return false

func get_platform_handle():
	"""Get the EOS platform handle (for advanced usage)"""
	if _godot_epic:
		return _godot_epic.get_platform_handle()
	return null

func shutdown():
	"""Shutdown the EOS platform"""
	if _debug_mode:
		_log("shutdown() called")

	if _godot_epic and _initialized:
		_godot_epic.shutdown_platform()
		_initialized = false
		_log("EOS platform shutdown complete")

# =============================================================================
# AUTHENTICATION METHODS
# =============================================================================

func login_with_epic_account(email: String, password: String):
	"""Authenticate with Epic Games using email and password"""
	if _debug_mode:
		_log("login_with_epic_account() called")

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.login_with_epic_account(email, password)

func login_with_account_portal():
	"""Authenticate with Epic Games using the account portal"""
	if _debug_mode:
		_log("login_with_account_portal() called")

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.login_with_account_portal()

func login_with_device_id(display_name: String):
	"""Authenticate with Epic Games using device ID"""
	if _debug_mode:
		_log("login_with_device_id() called")

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.login_with_device_id(display_name)

func login_with_dev(display_name: String):
	"""Authenticate with Epic Games using developer credentials (for testing)"""
	if _debug_mode:
		_log("login_with_dev() called")

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.login_with_dev(display_name)

func logout():
	"""Sign out the current user"""
	if _debug_mode:
		_log("logout() called")

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.logout()

func is_user_logged_in() -> bool:
	"""Check if user is currently authenticated"""
	if _godot_epic:
		return _godot_epic.is_user_logged_in()
	return false

func get_current_username() -> String:
	"""Get the current user's display name"""
	if _godot_epic:
		return _godot_epic.get_current_username()
	return ""

func get_epic_account_id() -> String:
	"""Get the current user's Epic Account ID"""
	if _godot_epic:
		return _godot_epic.get_epic_account_id()
	return ""

func get_product_user_id() -> String:
	"""Get the current user's Product User ID"""
	if _godot_epic:
		return _godot_epic.get_product_user_id()
	return ""

# =============================================================================
# FRIENDS METHODS
# =============================================================================

func query_friends():
	"""Query the user's friends list"""
	if _debug_mode:
		_log("query_friends() called")

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.query_friends()

func get_friends_list() -> Array:
	"""Get the cached friends list"""
	if _godot_epic:
		return _godot_epic.get_friends_list()
	return []

func get_user_info(target_user_id: String) -> Dictionary:
	"""Get cached information about a specific user"""
	if _godot_epic:
		return _godot_epic.get_user_info(target_user_id)
	return {}

func query_all_friends_info():
	"""Query information about all friends"""
	if _debug_mode:
		_log("query_all_friends_info() called")

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.query_all_friends_info()

# =============================================================================
# USER INFO METHODS
# =============================================================================

func query_user_info(target_user_id: String):
	"""Query user information for a specific user"""
	if _debug_mode:
		_log("query_user_info() called for target: " + str(target_user_id))

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.query_user_info(target_user_id)
# =============================================================================
# ACHIEVEMENTS METHODS
# =============================================================================

func query_achievement_definitions():
	"""Query all achievement definitions"""
	if _debug_mode:
		_log("query_achievement_definitions() called")

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.query_achievement_definitions()

func query_player_achievements():
	"""Query the player's achievement progress"""
	if _debug_mode:
		_log("query_player_achievements() called")

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.query_player_achievements()

func unlock_achievement(achievement_id: String):
	"""Unlock a single achievement"""
	if _debug_mode:
		_log("unlock_achievement() called for: " + str(achievement_id))

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.unlock_achievement(achievement_id)

func unlock_achievements(achievement_ids: Array):
	"""Unlock multiple achievements"""
	if _debug_mode:
		_log("unlock_achievements() called for: " + str(achievement_ids))

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.unlock_achievements(achievement_ids)

func get_achievement_definitions() -> Array:
	"""Get cached achievement definitions"""
	if _godot_epic:
		return _godot_epic.get_achievement_definitions()
	return []

func get_player_achievements() -> Array:
	"""Get cached player achievements"""
	if _godot_epic:
		return _godot_epic.get_player_achievements()
	return []

func get_achievement_definition(achievement_id: String) -> Dictionary:
	"""Get a specific achievement definition"""
	if _godot_epic:
		return _godot_epic.get_achievement_definition(achievement_id)
	return {}

func get_player_achievement(achievement_id: String) -> Dictionary:
	"""Get a specific player achievement"""
	if _godot_epic:
		return _godot_epic.get_player_achievement(achievement_id)
	return {}

# =============================================================================
# ACHIEVEMENT STATS METHODS
# =============================================================================

func ingest_achievement_stat(stat_name: String, amount: int):
	"""Ingest an achievement statistic"""
	if _debug_mode:
		_log("ingest_achievement_stat() called for: " + str(stat_name) + " amount: " + str(amount))

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.ingest_achievement_stat(stat_name, amount)

func query_achievement_stats():
	"""Query achievement statistics"""
	if _debug_mode:
		_log("query_achievement_stats() called")

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.query_achievement_stats()

func get_achievement_stats() -> Array:
	"""Get cached achievement statistics"""
	if _godot_epic:
		return _godot_epic.get_achievement_stats()
	return []

func get_achievement_stat(stat_name: String) -> Dictionary:
	"""Get a specific achievement statistic"""
	if _godot_epic:
		return _godot_epic.get_achievement_stat(stat_name)
	return {}

# =============================================================================
# LEADERBOARDS METHODS
# =============================================================================

func query_leaderboard_definitions():
	"""Query all leaderboard definitions"""
	if _debug_mode:
		_log("query_leaderboard_definitions() called")

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.query_leaderboard_definitions()

func query_leaderboard_ranks(leaderboard_id: String, limit: int = 100):
	"""Query leaderboard ranks"""
	if _debug_mode:
		_log("query_leaderboard_ranks() called for: " + str(leaderboard_id) + " limit: " + str(limit))

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.query_leaderboard_ranks(leaderboard_id, limit)

func query_leaderboard_user_scores(leaderboard_id: String, user_ids: Array):
	"""Query specific user scores for a leaderboard"""
	if _debug_mode:
		_log("query_leaderboard_user_scores() called for: " + str(leaderboard_id) + " users: " + str(user_ids))

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.query_leaderboard_user_scores(leaderboard_id, user_ids)

func ingest_stat(stat_name: String, value: int):
	"""Ingest a single statistic for leaderboards"""
	if _debug_mode:
		_log("ingest_stat() called for: " + str(stat_name) + " value: " + str(value))

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.ingest_stat(stat_name, value)

func ingest_stats(stats: Dictionary):
	"""Ingest multiple statistics for leaderboards"""
	if _debug_mode:
		_log("ingest_stats() called with: " + str(stats))

	if not _initialized:
		_log_error("Not initialized. Call initialize() first.")
		return

	_godot_epic.ingest_stats(stats)

func get_leaderboard_definitions() -> Array:
	"""Get cached leaderboard definitions"""
	if _godot_epic:
		return _godot_epic.get_leaderboard_definitions()
	return []

func get_leaderboard_ranks() -> Array:
	"""Get cached leaderboard ranks"""
	if _godot_epic:
		return _godot_epic.get_leaderboard_ranks()
	return []

func get_leaderboard_user_scores() -> Dictionary:
	"""Get cached leaderboard user scores"""
	if _godot_epic:
		return _godot_epic.get_leaderboard_user_scores()
	return {}

# =============================================================================
# SIGNAL HANDLERS
# =============================================================================

func _on_authentication_completed(success: bool, user_info: Dictionary):
	"""Handle authentication completion"""
	if _debug_mode:
		_log("Authentication completed - Success: " + str(success))
	login_completed.emit(success, user_info)

func _on_logout_completed(success: bool):
	"""Handle logout completion"""
	if _debug_mode:
		_log("Logout completed - Success: " + str(success))
	logout_completed.emit(success)

func _on_achievement_definitions_completed(success: bool, definitions: Array):
	"""Handle achievement definitions query completion"""
	if _debug_mode:
		_log("Achievement definitions completed - Success: " + str(success) + " Count: " + str(definitions.size()))
	achievement_definitions_completed.emit(success, definitions)

func _on_player_achievements_completed(success: bool, achievements: Array):
	"""Handle player achievements query completion"""
	if _debug_mode:
		_log("Player achievements completed - Success: " + str(success) + " Count: " + str(achievements.size()))
	player_achievements_completed.emit(success, achievements)

func _on_achievements_unlocked_completed(success: bool, unlocked_achievement_ids: Array):
	"""Handle achievement unlock completion"""
	if _debug_mode:
		_log("Achievements unlocked completed - Success: " + str(success) + " IDs: " + str(unlocked_achievement_ids))
	achievements_unlocked_completed.emit(success, unlocked_achievement_ids)

func _on_achievement_stats_completed(success: bool, stats: Array):
	"""Handle achievement stats query completion"""
	if _debug_mode:
		_log("Achievement stats completed - Success: " + str(success) + " Count: " + str(stats.size()))
	achievement_stats_completed.emit(success, stats)

func _on_stats_ingested(success: bool, stat_names: Array):
	"""Handle stat ingestion completion"""
	if _debug_mode:
		_log("Stats ingested - Success: " + str(success) + " Stats: " + str(stat_names))
	stats_ingested.emit(success, stat_names)

func _on_leaderboard_definitions_completed(success: bool, definitions: Array):
	"""Handle leaderboard definitions query completion"""
	if _debug_mode:
		_log("Leaderboard definitions completed - Success: " + str(success) + " Count: " + str(definitions.size()))
	leaderboard_definitions_completed.emit(success, definitions)

func _on_leaderboard_ranks_completed(success: bool, ranks: Array):
	"""Handle leaderboard ranks query completion"""
	if _debug_mode:
		_log("Leaderboard ranks completed - Success: " + str(success) + " Count: " + str(ranks.size()))
	leaderboard_ranks_completed.emit(success, ranks)

func _on_leaderboard_user_scores_completed(success: bool, user_scores: Dictionary):
	"""Handle leaderboard user scores query completion"""
	if _debug_mode:
		_log("Leaderboard user scores completed - Success: " + str(success) + " Users: " + str(user_scores.size()))
	leaderboard_user_scores_completed.emit(success, user_scores)

func _on_friends_query_completed(success: bool, friends_list: Array):
	"""Handle friends query completion"""
	if _debug_mode:
		_log("Friends query completed - Success: " + str(success) + " Count: " + str(friends_list.size()))
	friends_query_completed.emit(success, friends_list)

func _on_user_info_query_completed(success: bool, user_info: Dictionary):
	"""Handle user info query completion"""
	if _debug_mode:
		_log("User info query completed - Success: " + str(success) + " Info: " + str(user_info))
	user_info_query_completed.emit(success, user_info)

# =============================================================================
# PROCESS HANDLING - Integrates with GDExtension ticking
# =============================================================================

func _process(_delta: float) -> void:
	"""Handle EOS platform ticking every frame"""
	# Call the GDExtension tick method to process EOS callbacks
	tick(_delta)
