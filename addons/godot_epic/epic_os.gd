extends Node

# EpicOS Singleton - Main interface for Epic Online Services
# Integrates with the GodotEpic GDExtension for actual EOS SDK functionality

signal login_completed(success: bool, user_info: Dictionary)
signal achievement_unlocked(achievement_id: String)
signal stats_updated(stats: Dictionary)
signal leaderboard_retrieved(leaderboard_data: Array)
signal file_saved(success: bool, filename: String)
signal file_loaded(success: bool, filename: String, data: String)

var _godot_epic: GodotEpic = null
var _initialized: bool = false
var _logged_in: bool = false
var _debug_mode: bool = false

func _ready():
	print("EpicOS: Initializing Epic Online Services...")
	# Get the GodotEpic singleton from the GDExtension
	_godot_epic = GodotEpic.get_singleton()
	if not _godot_epic:
		print("EpicOS: ERROR - GodotEpic GDExtension not found!")
		print("EpicOS: Make sure the GodotEpic extension is properly installed and enabled.")
		return

	print("EpicOS: GodotEpic GDExtension found successfully")

func initialize(config: Dictionary = {}) -> bool:
	"""Initialize the EOS SDK with configuration options"""
	if _debug_mode:
		print("EpicOS: initialize() called")

	if not _godot_epic:
		print("EpicOS: ERROR - GodotEpic GDExtension not available")
		return false

	if _initialized:
		print("EpicOS: Already initialized")
		return true

	# Default configuration - override with your actual Epic credentials
	var default_config = {
		"product_name": "GodotEpic",
		"product_version": "1.0.0",
		"product_id": "",  # Required: Get from Epic Developer Portal
		"sandbox_id": "",  # Required: Get from Epic Developer Portal
		"deployment_id": "",  # Required: Get from Epic Developer Portal
		"client_id": "",  # Required: Get from Epic Developer Portal
		"client_secret": ""  # Required: Get from Epic Developer Portal
	}

	# Merge user config with defaults
	for key in config:
		default_config[key] = config[key]

	# Validate required fields
	var required_fields = ["product_id", "sandbox_id", "deployment_id", "client_id", "client_secret"]
	for field in required_fields:
		if default_config[field] == "":
			print("EpicOS: ERROR - Missing required field: ", field)
			print("EpicOS: Please provide all Epic credentials in the configuration")
			return false

	print("EpicOS: Initializing EOS SDK...")
	_initialized = _godot_epic.initialize_platform(default_config)

	if _initialized:
		print("EpicOS: EOS SDK initialized successfully")
	else:
		print("EpicOS: ERROR - Failed to initialize EOS SDK")

	return _initialized

func set_debug_mode(enabled: bool):
	"""Enable or disable debug logging"""
	_debug_mode = enabled
	print("EpicOS: Debug mode ", "enabled" if enabled else "disabled")

func tick():
	"""Tick the EOS platform - should be called every frame"""
	if _godot_epic and _initialized:
		_godot_epic.tick()

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

func login():
	"""Authenticate with Epic Games - Currently a placeholder"""
	if _debug_mode:
		print("EpicOS: login() called")

	if not _initialized:
		print("EpicOS: Error - Not initialized. Call initialize() first.")
		return

	# TODO: Implement EOS authentication when Auth interface is added to GDExtension
	print("EpicOS: Login functionality not yet implemented in GDExtension")
	print("EpicOS: This will be added when EOS Auth interface is integrated")

	# For now, simulate login for testing
	_logged_in = true
	var user_info = {
		"display_name": "TestUser",
		"user_id": "test_user_123",
		"epic_account_id": "epic_test_123",
		"note": "Simulated login - real auth coming soon"
	}

	login_completed.emit(true, user_info)
	print("EpicOS: Simulated login successful - ", user_info.display_name)

func logout():
	"""Sign out the current user"""
	if _debug_mode:
		print("EpicOS: logout() called")

	# TODO: Implement EOS logout when Auth interface is added
	_logged_in = false
	print("EpicOS: User logged out (simulated)")

func is_logged_in() -> bool:
	"""Check if user is currently authenticated"""
	return _logged_in

func shutdown():
	"""Shutdown the EOS platform"""
	if _debug_mode:
		print("EpicOS: shutdown() called")

	if _godot_epic and _initialized:
		_godot_epic.shutdown_platform()
		_initialized = false
		_logged_in = false
		print("EpicOS: EOS platform shutdown complete")

# =============================================================================
# PLACEHOLDER METHODS - These will be implemented when EOS interfaces are added
# Currently these are placeholders that show the intended API structure
# =============================================================================

func unlock_achievement(achievement_id: String):
	"""Unlock an achievement - PLACEHOLDER"""
	if _debug_mode:
		print("EpicOS: unlock_achievement(", achievement_id, ") called")

	if not is_platform_initialized():
		print("EpicOS: Error - Platform not initialized")
		return

	# TODO: Implement when EOS Achievements interface is added to GDExtension
	print("EpicOS: Achievement unlock not yet implemented - add EOS Achievements interface")
	print("EpicOS: Achievement ID: ", achievement_id)

func query_achievements():
	"""Query all achievements - PLACEHOLDER"""
	if _debug_mode:
		print("EpicOS: query_achievements() called")

	if not is_platform_initialized():
		print("EpicOS: Error - Platform not initialized")
		return

	# TODO: Implement when EOS Achievements interface is added to GDExtension
	print("EpicOS: Achievement query not yet implemented - add EOS Achievements interface")

func update_stat(stat_id: String, value: int):
	"""Update a player statistic - PLACEHOLDER"""
	if _debug_mode:
		print("EpicOS: update_stat(", stat_id, ", ", value, ") called")

	if not is_platform_initialized():
		print("EpicOS: Error - Platform not initialized")
		return

	# TODO: Implement when EOS Stats interface is added to GDExtension
	print("EpicOS: Stats update not yet implemented - add EOS Stats interface")
	print("EpicOS: Stat: ", stat_id, " = ", value)

func get_stats() -> Dictionary:
	"""Get current player statistics - PLACEHOLDER"""
	if _debug_mode:
		print("EpicOS: get_stats() called")

	if not is_platform_initialized():
		print("EpicOS: Error - Platform not initialized")
		return {}

	# TODO: Implement when EOS Stats interface is added to GDExtension
	print("EpicOS: Stats retrieval not yet implemented - add EOS Stats interface")
	return {}

func submit_score(board_id: String, score: int):
	"""Submit a score to a leaderboard - PLACEHOLDER"""
	if _debug_mode:
		print("EpicOS: submit_score(", board_id, ", ", score, ") called")

	if not is_platform_initialized():
		print("EpicOS: Error - Platform not initialized")
		return

	# TODO: Implement when EOS Leaderboards interface is added to GDExtension
	print("EpicOS: Leaderboard submission not yet implemented - add EOS Leaderboards interface")
	print("EpicOS: Board: ", board_id, ", Score: ", score)

func get_leaderboard(board_id: String, count: int = 10):
	"""Get leaderboard entries - PLACEHOLDER"""
	if _debug_mode:
		print("EpicOS: get_leaderboard(", board_id, ", ", count, ") called")

	if not is_platform_initialized():
		print("EpicOS: Error - Platform not initialized")
		return

	# TODO: Implement when EOS Leaderboards interface is added to GDExtension
	print("EpicOS: Leaderboard retrieval not yet implemented - add EOS Leaderboards interface")
	print("EpicOS: Board: ", board_id, ", Count: ", count)

func save_file(filename: String, data: String):
	"""Save data to Epic cloud storage - PLACEHOLDER"""
	if _debug_mode:
		print("EpicOS: save_file(", filename, ") called")

	if not is_platform_initialized():
		print("EpicOS: Error - Platform not initialized")
		return

	# TODO: Implement when EOS Player Data Storage interface is added to GDExtension
	print("EpicOS: Cloud save not yet implemented - add EOS Player Data Storage interface")
	print("EpicOS: File: ", filename, ", Data length: ", data.length())

func load_file(filename: String):
	"""Load data from Epic cloud storage - PLACEHOLDER"""
	if _debug_mode:
		print("EpicOS: load_file(", filename, ") called")

	if not is_platform_initialized():
		print("EpicOS: Error - Platform not initialized")
		return

	# TODO: Implement when EOS Player Data Storage interface is added to GDExtension
	print("EpicOS: Cloud load not yet implemented - add EOS Player Data Storage interface")
	print("EpicOS: File: ", filename)

# =============================================================================
# PROCESS HANDLING - Integrates with GDExtension ticking
# =============================================================================

func _process(_delta: float) -> void:
	"""Handle EOS platform ticking every frame"""
	# Call the GDExtension tick method to process EOS callbacks
	tick()