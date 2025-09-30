extends Node2D

# ============================================================================
# GODOT EPIC DEMO - TAB-BASED UI
# ============================================================================
# This demo showcases Epic Online Services (EOS) integration with Godot.
# The UI is organized into logical tabs for better user experience:
#
# 📱 AUTHENTICATION TAB - Login/logout functionality
#   • Epic Account Login (requires Epic Games credentials)
#   • Account Portal Login (for accounts with MFA enabled)
#   • Device ID Login (Dev/TestUser123 and Dev/Player1)
#   • Logout
#
# 👥 FRIENDS TAB - Friends management
#   • Query Friends (fetch from EOS)
#   • Get Friends (display cached list)
#
# 🏆 ACHIEVEMENTS TAB - Achievement system
#   • Query/Get Achievement Definitions (available without login)
#   • Query/Get Player Achievements (requires Product User ID)
#   • Unlock Test Achievement (requires Product User ID)
#   • Get Specific Achievement Data (both definition and player)
#
# 📊 STATS TAB - Statistics tracking
#   • Ingest Achievement Stats (requires Product User ID)
#   • Query/Get Achievement Stats (requires Product User ID)
#
# 🏁 LEADERBOARDS TAB - Leaderboard system
#   • Query/Get Leaderboard Definitions (available without login)
#   • Query/Get Leaderboard Ranks (requires Product User ID)
#   • Query/Get User Scores (requires Product User ID)
#   • Ingest Leaderboard Stats (requires Product User ID)
#
# ⚙️ SYSTEM TAB - Utility functions
#   • Clear Output
#
# Note: Features requiring "Product User ID" need cross-platform Connect service,
# which may not be available for developer accounts.
# ============================================================================

var godot_epic: GodotEpic = null

# UI References
@onready var status_label: Label = $CanvasLayer/UI/MainContainer/LeftPanel/StatusLabel
@onready var output_text: RichTextLabel = $CanvasLayer/UI/MainContainer/OutputPanel/OutputScroll/OutputText
@onready var tab_container: TabContainer = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer

# Authentication Tab UI References
@onready var auth_tab: Control = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/AuthenticationTab
@onready var username_field: LineEdit = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/AuthenticationTab/VBoxContainer/UsernameField
@onready var password_field: LineEdit = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/AuthenticationTab/VBoxContainer/PasswordField
@onready var login_epic_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/AuthenticationTab/VBoxContainer/LoginEpicButton
@onready var login_account_portal_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/AuthenticationTab/VBoxContainer/LoginAccountPortalButton
@onready var login_device1_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/AuthenticationTab/VBoxContainer/LoginDevice1Button
@onready var login_device2_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/AuthenticationTab/VBoxContainer/LoginDevice2Button
@onready var logout_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/AuthenticationTab/VBoxContainer/LogoutButton

# Friends Tab UI References
@onready var friends_tab: Control = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/FriendsTab
@onready var query_friends_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/FriendsTab/VBoxContainer/QueryFriendsButton
@onready var get_friends_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/FriendsTab/VBoxContainer/GetFriendsButton

# Achievements Tab UI References
@onready var achievements_tab: Control = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/AchievementsTab
@onready var query_ach_defs_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/AchievementsTab/VBoxContainer/QueryAchDefsButton
@onready var get_ach_defs_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/AchievementsTab/VBoxContainer/GetAchDefsButton
@onready var get_specific_def_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/AchievementsTab/VBoxContainer/GetSpecificDefButton
@onready var query_player_ach_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/AchievementsTab/VBoxContainer/QueryPlayerAchButton
@onready var get_player_ach_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/AchievementsTab/VBoxContainer/GetPlayerAchButton
@onready var get_specific_player_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/AchievementsTab/VBoxContainer/GetSpecificPlayerButton
@onready var unlock_test_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/AchievementsTab/VBoxContainer/UnlockTestButton

# Stats Tab UI References
@onready var stats_tab: Control = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/StatsTab
@onready var ingest_stat_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/StatsTab/VBoxContainer/IngestStatButton
@onready var query_stats_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/StatsTab/VBoxContainer/QueryStatsButton
@onready var get_stats_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/StatsTab/VBoxContainer/GetStatsButton

# Leaderboards Tab UI References
@onready var leaderboards_tab: Control = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/LeaderboardsTab
@onready var query_leaderboard_defs_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/LeaderboardsTab/VBoxContainer/QueryLeaderboardDefsButton
@onready var get_leaderboard_defs_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/LeaderboardsTab/VBoxContainer/GetLeaderboardDefsButton
@onready var query_leaderboard_ranks_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/LeaderboardsTab/VBoxContainer/QueryLeaderboardRanksButton
@onready var get_leaderboard_ranks_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/LeaderboardsTab/VBoxContainer/GetLeaderboardRanksButton
@onready var query_user_scores_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/LeaderboardsTab/VBoxContainer/QueryUserScoresButton
@onready var get_user_scores_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/LeaderboardsTab/VBoxContainer/GetUserScoresButton
@onready var ingest_leaderboard_stat_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/TabContainer/LeaderboardsTab/VBoxContainer/IngestLeaderboardStatButton

# System Tab UI References
@onready var clear_output_button: Button = $CanvasLayer/UI/MainContainer/LeftPanel/ClearOutputButton

# Auto-test variables
var auto_test_timer: Timer = null
var auto_test_step: int = 0
var auto_test_enabled: bool = true  # Set to false to disable auto-test
var auto_test_time_accumulator: float = 0.0
var auto_test_current_delay: float = 0.0

# Called when the node enters the scene tree for the first time.
func _ready():
	godot_epic = GodotEpic.get_singleton()

	# Set tab names by index (0, 1, 2, etc.)
	tab_container.set_tab_title(0, "🔐 Login")
	tab_container.set_tab_title(1, "👥 Friends")
	tab_container.set_tab_title(2, "🏆 Achievements")
	tab_container.set_tab_title(3, "📊 Statistics")
	tab_container.set_tab_title(4, "🏁 Leaderboards")

	# Connect to signals for async operations
	godot_epic.connect("login_completed", _on_login_completed)
	godot_epic.connect("logout_completed", _on_logout_completed)
	godot_epic.connect("friends_updated", _on_friends_updated)
	godot_epic.connect("achievement_definitions_updated", _on_achievement_definitions_updated)
	godot_epic.connect("player_achievements_updated", _on_player_achievements_updated)
	godot_epic.connect("achievements_unlocked", _on_achievements_unlocked)
	godot_epic.connect("achievement_unlocked", _on_achievement_unlocked)
	godot_epic.connect("achievement_stats_updated", _on_achievement_stats_updated)
	godot_epic.connect("leaderboard_definitions_updated", _on_leaderboard_definitions_updated)
	godot_epic.connect("leaderboard_ranks_updated", _on_leaderboard_ranks_updated)
	godot_epic.connect("leaderboard_user_scores_updated", _on_leaderboard_user_scores_updated)
	godot_epic.connect("stats_ingested", _on_stats_ingested)

	# Connect button signals by tab groups
	_connect_authentication_buttons()
	_connect_friends_buttons()
	_connect_achievements_buttons()
	_connect_stats_buttons()
	_connect_leaderboards_buttons()
	_connect_system_buttons()

	var init_options = {
		"product_name": "GodotEpic Demo",
		"product_version": "1.0.0",
		"product_id": "your_product_id_here",
		"sandbox_id": "your_sandbox_id_here",
		"deployment_id": "your_deployment_id_here",
		"client_id": "your_client_id_here",
		"client_secret": "your_client_secret_here",
		"encryption_key": "1111111111111111111111111111111111111111111111111111111"  # Optional but recommended
	}

	# Initialize the EOS platform
	var success = godot_epic.initialize_platform(init_options)
	if success:
		status_label.text = "✅ EOS Platform Ready"
		add_output_line("🚀 [color=green]EOS Platform initialized successfully![/color]")
		add_output_line("Ready for authentication, friends, and achievements features!")
		add_output_line("")
		add_output_line("Available Features:")
		add_output_line("• Authentication (Epic Account & Device ID)")
		add_output_line("• Friends Management")
		add_output_line("• Achievements System")
		add_output_line("• Stats Tracking")
		add_output_line("• Leaderboards System")
		add_output_line("")
		add_output_line("[i]Use the tabs above to organize your testing, or keyboard shortcuts (1-9, 0, Q, W, R, T, Y, E, I, O, P, A, S, D)[/i]")

		# Start auto-test if enabled
		if auto_test_enabled:
			start_auto_test()
	else:
		status_label.text = "❌ Initialization Failed"
		add_output_line("[color=red]❌ Failed to initialize EOS Platform![/color]")
		add_output_line("[color=orange]Check your credentials in the Epic Developer Portal.[/color]")

	# Set initial button states
	update_button_states()


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(_delta):
	# Tick the EOS platform to handle callbacks and updates
	if godot_epic and godot_epic.is_platform_initialized():
		godot_epic.tick(_delta)

	# Handle auto-test timing
	if auto_test_enabled and auto_test_step >= 0 and auto_test_current_delay > 0:
		auto_test_time_accumulator += _delta
		if auto_test_time_accumulator >= auto_test_current_delay:
			auto_test_time_accumulator = 0.0
			auto_test_current_delay = 0.0
			_on_auto_test_timer_timeout()


# Helper function to add formatted output
# Tab-based button connection functions
func _connect_authentication_buttons():
	login_epic_button.pressed.connect(_on_login_epic_pressed)
	login_account_portal_button.pressed.connect(_on_login_account_portal_pressed)
	login_device1_button.pressed.connect(_on_login_device1_pressed)
	login_device2_button.pressed.connect(_on_login_device2_pressed)
	logout_button.pressed.connect(_on_logout_pressed)
	username_field.text_changed.connect(_on_login_credentials_changed)
	password_field.text_changed.connect(_on_login_credentials_changed)
	password_field.text_submitted.connect(_on_password_submitted)

func _connect_friends_buttons():
	query_friends_button.pressed.connect(_on_query_friends_pressed)
	get_friends_button.pressed.connect(_on_get_friends_pressed)

func _connect_achievements_buttons():
	query_ach_defs_button.pressed.connect(_on_query_ach_defs_pressed)
	get_ach_defs_button.pressed.connect(_on_get_ach_defs_pressed)
	get_specific_def_button.pressed.connect(_on_get_specific_def_pressed)
	query_player_ach_button.pressed.connect(_on_query_player_ach_pressed)
	get_player_ach_button.pressed.connect(_on_get_player_ach_pressed)
	get_specific_player_button.pressed.connect(_on_get_specific_player_pressed)
	unlock_test_button.pressed.connect(_on_unlock_test_pressed)

func _connect_stats_buttons():
	ingest_stat_button.pressed.connect(_on_ingest_stat_pressed)
	query_stats_button.pressed.connect(_on_query_stats_pressed)
	get_stats_button.pressed.connect(_on_get_stats_pressed)

func _connect_leaderboards_buttons():
	query_leaderboard_defs_button.pressed.connect(_on_query_leaderboard_defs_pressed)
	get_leaderboard_defs_button.pressed.connect(_on_get_leaderboard_defs_pressed)
	query_leaderboard_ranks_button.pressed.connect(_on_query_leaderboard_ranks_pressed)
	get_leaderboard_ranks_button.pressed.connect(_on_get_leaderboard_ranks_pressed)
	query_user_scores_button.pressed.connect(_on_query_user_scores_pressed)
	get_user_scores_button.pressed.connect(_on_get_user_scores_pressed)
	ingest_leaderboard_stat_button.pressed.connect(_on_ingest_leaderboard_stat_pressed)

func _connect_system_buttons():
	clear_output_button.pressed.connect(_on_clear_output_pressed)

# Helper function to add formatted output
func add_output_line(text: String):
	if output_text:
		output_text.append_text(text + "\n")
		print(text)  # Also print to console

func update_button_states():
	var is_logged_in = godot_epic and godot_epic.is_user_logged_in()
	var has_product_user_id = godot_epic and not godot_epic.get_product_user_id().is_empty()

	_update_epic_login_button(is_logged_in)
	# Update Authentication tab buttons
	logout_button.disabled = not is_logged_in

	# Update Friends tab buttons (require login)
	_update_friends_tab_buttons(is_logged_in)

	# Update Achievements tab buttons (require Product User ID for cross-platform features)
	_update_achievements_tab_buttons(is_logged_in, has_product_user_id)

	# Update Stats tab buttons (require Product User ID for cross-platform features)
	_update_stats_tab_buttons(is_logged_in, has_product_user_id)

	# Update Leaderboards tab buttons (require Product User ID for cross-platform features)
	_update_leaderboards_tab_buttons(is_logged_in, has_product_user_id)

func _update_epic_login_button(is_logged_in: bool):
	if not is_instance_valid(login_epic_button):
		return

	if is_logged_in:
		login_epic_button.disabled = true
		login_account_portal_button.disabled = true
	else:
		var username_ready = is_instance_valid(username_field) and not username_field.text.strip_edges().is_empty()
		var password_ready = is_instance_valid(password_field) and not password_field.text.is_empty()
		login_epic_button.disabled = not (username_ready and password_ready)
		login_account_portal_button.disabled = false  # Account Portal doesn't need credentials

func _on_login_credentials_changed(_new_text: String = ""):
	var is_logged_in = godot_epic and godot_epic.is_user_logged_in()
	_update_epic_login_button(is_logged_in)

func _on_password_submitted(_new_text: String):
	_on_login_epic_pressed()

func _update_friends_tab_buttons(is_logged_in: bool):
	query_friends_button.disabled = not is_logged_in
	get_friends_button.disabled = not is_logged_in

func _update_achievements_tab_buttons(is_logged_in: bool, has_product_user_id: bool):
	# Definition queries don't require login
	# query_ach_defs_button.disabled = false
	# get_ach_defs_button.disabled = false
	# get_specific_def_button.disabled = false

	# Player achievements require Product User ID (cross-platform features)
	query_player_ach_button.disabled = not (is_logged_in and has_product_user_id)
	get_player_ach_button.disabled = not (is_logged_in and has_product_user_id)
	unlock_test_button.disabled = not (is_logged_in and has_product_user_id)
	get_specific_player_button.disabled = not (is_logged_in and has_product_user_id)

func _update_stats_tab_buttons(is_logged_in: bool, has_product_user_id: bool):
	# Stats require Product User ID for cross-platform features
	ingest_stat_button.disabled = not (is_logged_in and has_product_user_id)
	query_stats_button.disabled = not (is_logged_in and has_product_user_id)
	get_stats_button.disabled = not (is_logged_in and has_product_user_id)

func _update_leaderboards_tab_buttons(is_logged_in: bool, has_product_user_id: bool):
	# Leaderboard definitions can be queried without login
	# query_leaderboard_defs_button.disabled = false
	# get_leaderboard_defs_button.disabled = false

	# Leaderboard operations require Product User ID for cross-platform features
	query_leaderboard_ranks_button.disabled = not (is_logged_in and has_product_user_id)
	get_leaderboard_ranks_button.disabled = not (is_logged_in and has_product_user_id)
	query_user_scores_button.disabled = not (is_logged_in and has_product_user_id)
	get_user_scores_button.disabled = not (is_logged_in and has_product_user_id)
	ingest_leaderboard_stat_button.disabled = not (is_logged_in and has_product_user_id)

# ============================================================================
# AUTHENTICATION TAB BUTTON HANDLERS
# ============================================================================

func _on_login_epic_pressed():
	if not godot_epic:
		add_output_line("[color=red]❌ EOS Platform not ready yet.[/color]")
		return

	var username := ""
	var password := ""

	if is_instance_valid(username_field):
		username = username_field.text.strip_edges()
	if is_instance_valid(password_field):
		password = password_field.text

	if username.is_empty() or password.is_empty():
		add_output_line("[color=orange]⚠️ Please enter both username and password for Epic Account login.[/color]")
		add_output_line("[i]Tip: If your account has MFA enabled, use Account Portal login instead.[/i]")
		status_label.text = "⚠️ Enter Epic credentials"
		_update_epic_login_button(false)
		return

	add_output_line("[color=cyan]🔐 Starting Epic Account login for [b]" + username + "[/b]...[/color]")
	status_label.text = "⏳ Logging in..."
	login_epic_button.disabled = true
	if is_instance_valid(password_field):
		password_field.release_focus()
	if is_instance_valid(username_field):
		username_field.release_focus()

	godot_epic.login_with_epic_account(username, password)

func _on_login_account_portal_pressed():
	if not godot_epic:
		add_output_line("[color=red]❌ EOS Platform not ready yet.[/color]")
		return

	add_output_line("[color=cyan]🔐 Starting Account Portal login...[/color]")
	add_output_line("[i]This will open your default browser or Epic Games Launcher for authentication.[/i]")
	status_label.text = "⏳ Opening browser..."
	login_account_portal_button.disabled = true
	login_epic_button.disabled = true

	godot_epic.login_with_account_portal()

func _on_login_device1_pressed():
	add_output_line("[color=cyan]🔐 Starting Dev login (TestUser123)...[/color]")
	godot_epic.login_with_dev("TestUser123")

func _on_login_device2_pressed():
	add_output_line("[color=cyan]🔐 Starting Dev login (Player1)...[/color]")
	godot_epic.login_with_dev("Player1")

func _on_logout_pressed():
	if godot_epic.is_user_logged_in():
		add_output_line("[color=cyan]🚪 Logging out...[/color]")
		godot_epic.logout()
	else:
		add_output_line("[color=orange]⚠️ Not logged in![/color]")

# ============================================================================
# FRIENDS TAB BUTTON HANDLERS
# ============================================================================

func _on_query_friends_pressed():
	if godot_epic.is_user_logged_in():
		add_output_line("[color=blue]👥 Querying friends list...[/color]")
		godot_epic.query_friends()
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

func _on_get_friends_pressed():
	if godot_epic.is_user_logged_in():
		add_output_line("[color=blue]👥 Getting current friends list...[/color]")
		var friends = godot_epic.get_friends_list()
		_display_friends_list(friends)
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

# ============================================================================
# ACHIEVEMENTS TAB BUTTON HANDLERS
# ============================================================================

func _on_query_ach_defs_pressed():
	add_output_line("[color=yellow]🏆 Querying achievement definitions...[/color]")
	godot_epic.query_achievement_definitions()

func _on_query_player_ach_pressed():
	if godot_epic.is_user_logged_in():
		var product_user_id = godot_epic.get_product_user_id()
		if not product_user_id.is_empty():
			add_output_line("[color=yellow]🎯 Querying player achievements...[/color]")
			godot_epic.query_player_achievements()
		else:
			add_output_line("[color=orange]⚠️ Achievements require cross-platform features (Product User ID). Connect service is not available for developer accounts.[/color]")
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

func _on_get_ach_defs_pressed():
	add_output_line("[color=yellow]🏆 Getting current achievement definitions...[/color]")
	var definitions = godot_epic.get_achievement_definitions()
	_display_achievement_definitions(definitions)

func _on_get_player_ach_pressed():
	if godot_epic.is_user_logged_in():
		var product_user_id = godot_epic.get_product_user_id()
		if not product_user_id.is_empty():
			add_output_line("[color=yellow]🎯 Getting current player achievements...[/color]")
			var achievements = godot_epic.get_player_achievements()
			_display_player_achievements(achievements)
		else:
			add_output_line("[color=orange]⚠️ Achievements require cross-platform features (Product User ID). Connect service is not available for developer accounts.[/color]")
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

func _on_unlock_test_pressed():
	if godot_epic.is_user_logged_in():
		var product_user_id = godot_epic.get_product_user_id()
		if not product_user_id.is_empty():
			add_output_line("[color=purple]🎉 Attempting to unlock test achievement...[/color]")
			godot_epic.unlock_achievement("achievement_first_level")
		else:
			add_output_line("[color=orange]⚠️ Achievements require cross-platform features (Product User ID). Connect service is not available for developer accounts.[/color]")
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

func _on_get_specific_def_pressed():
	add_output_line("[color=yellow]🏆 Getting specific achievement definition...[/color]")
	var definition = godot_epic.get_achievement_definition("test_achievement")
	_display_single_achievement_definition(definition)

func _on_get_specific_player_pressed():
	if godot_epic.is_user_logged_in():
		var product_user_id = godot_epic.get_product_user_id()
		if not product_user_id.is_empty():
			add_output_line("[color=yellow]🎯 Getting specific player achievement...[/color]")
			var achievement = godot_epic.get_player_achievement("test_achievement")
			_display_single_player_achievement(achievement)
		else:
			add_output_line("[color=orange]⚠️ Achievements require cross-platform features (Product User ID). Connect service is not available for developer accounts.[/color]")
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

# ============================================================================
# STATS TAB BUTTON HANDLERS
# ============================================================================

func _on_ingest_stat_pressed():
	if godot_epic.is_user_logged_in():
		var product_user_id = godot_epic.get_product_user_id()
		if not product_user_id.is_empty():
			add_output_line("[color=purple]📊 Ingesting test stat (EnemiesDefeated: +1)...[/color]")
			godot_epic.ingest_achievement_stat("EnemiesDefeated", 1)
		else:
			add_output_line("[color=orange]⚠️ Stats require cross-platform features (Product User ID). Connect service is not available for developer accounts.[/color]")
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

func _on_query_stats_pressed():
	if godot_epic.is_user_logged_in():
		var product_user_id = godot_epic.get_product_user_id()
		if not product_user_id.is_empty():
			add_output_line("[color=purple]📊 Querying achievement stats...[/color]")
			godot_epic.query_achievement_stats()
		else:
			add_output_line("[color=orange]⚠️ Stats require cross-platform features (Product User ID). Connect service is not available for developer accounts.[/color]")
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

func _on_get_stats_pressed():
	if godot_epic.is_user_logged_in():
		var product_user_id = godot_epic.get_product_user_id()
		if not product_user_id.is_empty():
			add_output_line("[color=purple]📊 Getting current achievement stats...[/color]")
			var stats = godot_epic.get_achievement_stats()
			_display_achievement_stats(stats)
		else:
			add_output_line("[color=orange]⚠️ Stats require cross-platform features (Product User ID). Connect service is not available for developer accounts.[/color]")
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

# ============================================================================
# LEADERBOARDS TAB BUTTON HANDLERS
# ============================================================================

func _on_query_leaderboard_defs_pressed():
	add_output_line("[color=cyan]🏁 Querying leaderboard definitions...[/color]")
	godot_epic.query_leaderboard_definitions()

func _on_get_leaderboard_defs_pressed():
	add_output_line("[color=cyan]🏁 Getting current leaderboard definitions...[/color]")
	var definitions = godot_epic.get_leaderboard_definitions()
	_display_leaderboard_definitions(definitions)

func _on_query_leaderboard_ranks_pressed():
	if godot_epic.is_user_logged_in():
		var product_user_id = godot_epic.get_product_user_id()
		if not product_user_id.is_empty():
			add_output_line("[color=cyan]🏁 Querying leaderboard ranks (top 10)...[/color]")
			godot_epic.query_leaderboard_ranks("EnemiesSmashedEver", 10)
		else:
			add_output_line("[color=orange]⚠️ Leaderboards require cross-platform features (Product User ID). Connect service is not available for developer accounts.[/color]")
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

func _on_get_leaderboard_ranks_pressed():
	if godot_epic.is_user_logged_in():
		var product_user_id = godot_epic.get_product_user_id()
		if not product_user_id.is_empty():
			add_output_line("[color=cyan]🏁 Getting current leaderboard ranks...[/color]")
			var ranks = godot_epic.get_leaderboard_ranks()
			_display_leaderboard_ranks(ranks)
		else:
			add_output_line("[color=orange]⚠️ Leaderboards require cross-platform features (Product User ID). Connect service is not available for developer accounts.[/color]")
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

func _on_query_user_scores_pressed():
	if godot_epic.is_user_logged_in():
		var product_user_id = godot_epic.get_product_user_id()
		if not product_user_id.is_empty():
			add_output_line("[color=cyan]🏁 Querying user scores...[/color]")
			var user_ids = [product_user_id]  # Query current user's score
			godot_epic.query_leaderboard_user_scores("EnemiesSmashedEver", user_ids)
		else:
			add_output_line("[color=orange]⚠️ Leaderboards require cross-platform features (Product User ID). Connect service is not available for developer accounts.[/color]")
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

func _on_get_user_scores_pressed():
	if godot_epic.is_user_logged_in():
		var product_user_id = godot_epic.get_product_user_id()
		if not product_user_id.is_empty():
			add_output_line("[color=cyan]🏁 Getting current user scores...[/color]")
			var user_scores = godot_epic.get_leaderboard_user_scores()
			_display_leaderboard_user_scores(user_scores)
		else:
			add_output_line("[color=orange]⚠️ Leaderboards require cross-platform features (Product User ID). Connect service is not available for developer accounts.[/color]")
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

func _on_ingest_leaderboard_stat_pressed():
	if godot_epic.is_user_logged_in():
		var product_user_id = godot_epic.get_product_user_id()
		if not product_user_id.is_empty():
			add_output_line("[color=cyan]🏁 Ingesting test leaderboard stat (Score: +100)...[/color]")
			godot_epic.ingest_stat("Score", 100)
		else:
			add_output_line("[color=orange]⚠️ Leaderboards require cross-platform features (Product User ID). Connect service is not available for developer accounts.[/color]")
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

# ============================================================================
# SYSTEM TAB BUTTON HANDLERS
# ============================================================================

func _on_clear_output_pressed():
	if output_text:
		output_text.clear()
		add_output_line("[i]Output cleared[/i]")

# ============================================================================
# INPUT HANDLING
# ============================================================================

func _input(event):
	if event is InputEventKey and event.pressed:
		# Check for Alt+1 combination for Account Portal login
		if event.keycode == KEY_1 and event.alt_pressed:
			_on_login_account_portal_pressed()
			return
		# Check for Alt+2 combination for second device login
		if event.keycode == KEY_2 and event.alt_pressed:
			_on_login_device2_pressed()
			return

		match event.keycode:
			KEY_1:
				_on_login_epic_pressed()
			KEY_2:
				_on_login_device1_pressed()
			KEY_3:
				_on_logout_pressed()
			KEY_4:
				_on_query_friends_pressed()
			KEY_5:
				_on_get_friends_pressed()
			KEY_6:
				_on_query_ach_defs_pressed()
			KEY_7:
				_on_query_player_ach_pressed()
			KEY_8:
				_on_get_ach_defs_pressed()
			KEY_9:
				_on_get_player_ach_pressed()
			KEY_0:
				_on_unlock_test_pressed()
			KEY_Q:
				_on_get_specific_def_pressed()
			KEY_W:
				_on_get_specific_player_pressed()
			KEY_R:
				_on_ingest_stat_pressed()
			KEY_T:
				_on_query_stats_pressed()
			KEY_Y:
				_on_get_stats_pressed()
			KEY_E:
				_on_query_leaderboard_defs_pressed()
			KEY_I:
				_on_get_leaderboard_defs_pressed()
			KEY_O:
				_on_query_leaderboard_ranks_pressed()
			KEY_P:
				_on_get_leaderboard_ranks_pressed()
			KEY_A:
				_on_query_user_scores_pressed()
			KEY_S:
				_on_get_user_scores_pressed()
			KEY_D:
				_on_ingest_leaderboard_stat_pressed()

# ============================================================================
# EOS SIGNAL HANDLERS
# ============================================================================

# Signal handlers
func _on_login_completed(success: bool, user_info: Dictionary):
	if success:
		var display_name = user_info.get("display_name", "Unknown User")
		var epic_account_id = user_info.get("epic_account_id", "Not available")
		var product_user_id = user_info.get("product_user_id", "")

		status_label.text = "✅ Logged In: " + display_name
		add_output_line("[color=green]✅ Login successful![/color]")
		add_output_line("Username: " + display_name)
		add_output_line("Epic Account ID: " + epic_account_id)
		add_output_line("Product User ID: " + product_user_id)

		if product_user_id != "":
			add_output_line("[color=green]✓ Cross-platform features enabled[/color]")
		else:
			add_output_line("[color=orange]⚠ Cross-platform features disabled (Connect service failed)[/color]")

		add_output_line("")
		add_output_line("[color=green]🎉 You can now use available features![/color]")

		if is_instance_valid(password_field):
			password_field.text = ""
		_on_login_credentials_changed()
		update_button_states()
	else:
		status_label.text = "❌ Login Failed"

		# Check for specific error codes
		var error_code = user_info.get("error_code", 0)
		var error_message = user_info.get("error_message", "")

		add_output_line("[color=red]❌ Login failed![/color]")

		if error_code == 1060:  # EOS_Auth_MFARequired
			add_output_line("[color=yellow]🔐 Multi-Factor Authentication (MFA) is required for this account.[/color]")
			add_output_line("[color=cyan]💡 Solution: Use the 'Login with Account Portal' button instead.[/color]")
			add_output_line("[color=cyan]   This will open your browser or Epic Games Launcher for secure authentication.[/color]")
		elif error_code == 2:  # EOS_InvalidCredentials
			add_output_line("[color=yellow]❌ Invalid email or password.[/color]")
			add_output_line("[color=cyan]💡 Please check your Epic Games account credentials.[/color]")
		elif error_code == 10:  # EOS_InvalidParameters
			add_output_line("[color=yellow]⚠️ Invalid parameters.[/color]")
			add_output_line("[color=cyan]💡 Check email format and ensure password is not empty.[/color]")
		else:
			add_output_line("[color=yellow]Common issues:[/color]")
			add_output_line("• [color=yellow]For Device ID: Run EOS Dev Auth Tool on localhost:7777[/color]")
			add_output_line("• [color=yellow]For Epic Account: Check email/password or try Account Portal[/color]")
			add_output_line("• [color=yellow]For MFA accounts: Use Account Portal login[/color]")

		if not error_message.is_empty():
			add_output_line("[i]Technical details: " + error_message + "[/i]")

		update_button_states()


func _on_logout_completed(success: bool):
	if success:
		status_label.text = "🔓 Logged Out"
		add_output_line("[color=green]✅ Logout successful![/color]")
		update_button_states()
	else:
		add_output_line("[color=red]❌ Logout failed![/color]")


func _on_friends_updated(friends_list: Array):
	add_output_line("[color=blue]🔄 Friends list updated![/color]")
	_display_friends_list(friends_list)


func _display_friends_list(friends: Array):
	if friends.size() == 0:
		add_output_line("📝 Friends list is empty")
		return

	add_output_line("👥 Friends list (" + str(friends.size()) + " friends):")
	for i in range(friends.size()):
		var friend = friends[i]
		var friend_id = str(friend.get("id", "Unknown"))
		var display_name = str(friend.get("display_name", "Loading..."))
		var status = str(friend.get("status", "Unknown"))

		# Display friend number and name/ID
		if display_name != "Loading..." and display_name != "Unknown User":
			add_output_line("  " + str(i + 1) + ". [color=cyan]" + display_name + "[/color] (" + friend_id + ")")
		else:
			add_output_line("  " + str(i + 1) + ". " + display_name + " (" + friend_id + ")")

		add_output_line("     Status: " + status)

		# Show additional user info if available
		var country = friend.get("country", "")
		var preferred_language = friend.get("preferred_language", "")
		var nickname = friend.get("nickname", "")

		if country != "":
			add_output_line("     Country: " + country)
		if preferred_language != "":
			add_output_line("     Language: " + preferred_language)
		if nickname != "" and nickname != display_name:
			add_output_line("     Nickname: " + nickname)

		add_output_line("")

# ============================================================================
# ACHIEVEMENT SIGNAL HANDLERS & DISPLAY FUNCTIONS
# ============================================================================

# Achievement signal handlers
func _on_achievement_definitions_updated(definitions: Array):
	add_output_line("[color=yellow]🏆 Achievement definitions updated![/color]")
	_display_achievement_definitions(definitions)


func _on_player_achievements_updated(achievements: Array):
	add_output_line("[color=yellow]🎯 Player achievements updated![/color]")
	_display_player_achievements(achievements)


func _on_achievements_unlocked(unlocked_achievement_ids: Array):
	add_output_line("[color=purple]🎉 Achievements unlocked![/color]")
	if unlocked_achievement_ids.size() > 0:
		add_output_line("Unlocked achievement IDs: " + str(unlocked_achievement_ids))
	else:
		add_output_line("[i]Achievement unlock completed (specific IDs not available)[/i]")


func _on_achievement_unlocked(achievement_id: String, unlock_time: int):
	add_output_line("[color=purple]🏅 Achievement unlocked: " + achievement_id + "[/color]")
	add_output_line("Unlock time: " + str(unlock_time))


# Stats signal handlers
func _on_achievement_stats_updated(success: bool, stats: Array):
	add_output_line("[color=purple]📊 Achievement stats updated![/color]")
	if success:
		_display_achievement_stats(stats)
	else:
		add_output_line("[color=red]❌ Failed to update achievement stats[/color]")

# ============================================================================
# LEADERBOARD SIGNAL HANDLERS
# ============================================================================

# Leaderboard signal handlers
func _on_leaderboard_definitions_updated(definitions: Array):
	add_output_line("[color=cyan]🏁 Leaderboard definitions updated![/color]")
	_display_leaderboard_definitions(definitions)

func _on_leaderboard_ranks_updated(ranks: Array):
	add_output_line("[color=cyan]🏁 Leaderboard ranks updated![/color]")
	_display_leaderboard_ranks(ranks)

func _on_leaderboard_user_scores_updated(user_scores: Dictionary):
	add_output_line("[color=cyan]🏁 Leaderboard user scores updated![/color]")
	_display_leaderboard_user_scores(user_scores)

func _on_stats_ingested(stat_names: Array):
	add_output_line("[color=cyan]🏁 Stats ingested successfully![/color]")
	if stat_names.size() > 0:
		add_output_line("Ingested stats: " + str(stat_names))
	else:
		add_output_line("[i]Stat ingestion completed[/i]")


# Achievement display functions
func _display_achievement_definitions(definitions: Array):
	if definitions.size() == 0:
		add_output_line("📝 No achievement definitions available")
		return

	add_output_line("🏆 Achievement Definitions (" + str(definitions.size()) + " achievements):")
	for i in range(definitions.size()):
		var def = definitions[i]
		add_output_line("  " + str(i + 1) + ". " + str(def.get("unlocked_display_name", "Unknown Achievement")))
		add_output_line("     ID: " + str(def.get("achievement_id", "Unknown")))
		add_output_line("     Description: " + str(def.get("unlocked_description", "No description")))
		add_output_line("     Hidden: " + str(def.get("is_hidden", false)))

		var stat_thresholds = def.get("stat_thresholds", [])
		if stat_thresholds.size() > 0:
			add_output_line("     Requirements:")
			for threshold in stat_thresholds:
				add_output_line("       • " + str(threshold.get("name", "Unknown")) + ": " + str(threshold.get("threshold", 0)))
		add_output_line("")


func _display_player_achievements(achievements: Array):
	if achievements.size() == 0:
		add_output_line("📝 No player achievements available")
		return

	add_output_line("🎯 Player Achievements (" + str(achievements.size()) + " achievements):")
	var unlocked_count = 0

	for i in range(achievements.size()):
		var ach = achievements[i]
		var is_unlocked = ach.get("is_unlocked", false)
		if is_unlocked:
			unlocked_count += 1

		var status_icon = "🏅" if is_unlocked else "🔒"
		var progress = ach.get("progress", 0.0)
		var progress_color = "green" if is_unlocked else "orange"

		add_output_line("  " + status_icon + " " + str(ach.get("display_name", "Unknown Achievement")))
		add_output_line("     ID: " + str(ach.get("achievement_id", "Unknown")))
		add_output_line("     [color=" + progress_color + "]Progress: " + str(progress) + "%[/color]")

		if is_unlocked:
			var unlock_time = ach.get("unlock_time", 0)
			if unlock_time > 0:
				add_output_line("     [color=green]Unlocked: " + str(unlock_time) + "[/color]")

		var stat_info = ach.get("stat_info", [])
		if stat_info.size() > 0:
			add_output_line("     Stats:")
			for stat in stat_info:
				var current = stat.get("current_value", 0)
				var threshold = stat.get("threshold_value", 0)
				add_output_line("       • " + str(stat.get("name", "Unknown")) + ": " + str(current) + "/" + str(threshold))
		add_output_line("")

	add_output_line("📊 Summary: [color=green]" + str(unlocked_count) + "[/color]/" + str(achievements.size()) + " achievements unlocked")
	add_output_line("")


func _display_single_achievement_definition(definition: Dictionary):
	if definition.is_empty():
		add_output_line("[color=red]❌ Achievement definition not found[/color]")
		return

	add_output_line("🏆 Achievement Definition:")
	add_output_line("  Name: " + str(definition.get("unlocked_display_name", "Unknown")))
	add_output_line("  ID: " + str(definition.get("achievement_id", "Unknown")))
	add_output_line("  Description: " + str(definition.get("unlocked_description", "No description")))
	add_output_line("  Locked Description: " + str(definition.get("locked_description", "No description")))
	add_output_line("  Hidden: " + str(definition.get("is_hidden", false)))

	var stat_thresholds = definition.get("stat_thresholds", [])
	if stat_thresholds.size() > 0:
		add_output_line("  Requirements:")
		for threshold in stat_thresholds:
			add_output_line("    • " + str(threshold.get("name", "Unknown")) + ": " + str(threshold.get("threshold", 0)))
	add_output_line("")


func _display_single_player_achievement(achievement: Dictionary):
	if achievement.is_empty():
		add_output_line("[color=red]❌ Player achievement not found[/color]")
		return

	var is_unlocked = achievement.get("is_unlocked", false)
	var status_icon = "🏅" if is_unlocked else "🔒"
	var progress_color = "green" if is_unlocked else "orange"

	add_output_line("🎯 Player Achievement:")
	add_output_line("  " + status_icon + " " + str(achievement.get("display_name", "Unknown")))
	add_output_line("  ID: " + str(achievement.get("achievement_id", "Unknown")))
	add_output_line("  [color=" + progress_color + "]Progress: " + str(achievement.get("progress", 0.0)) + "%[/color]")

	if is_unlocked:
		var unlock_time = achievement.get("unlock_time", 0)
		if unlock_time > 0:
			add_output_line("  [color=green]Unlocked: " + str(unlock_time) + "[/color]")

	var stat_info = achievement.get("stat_info", [])
	if stat_info.size() > 0:
		add_output_line("  Stats:")
		for stat in stat_info:
			var current = stat.get("current_value", 0)
			var threshold = stat.get("threshold_value", 0)
			add_output_line("    • " + str(stat.get("name", "Unknown")) + ": " + str(current) + "/" + str(threshold))
	add_output_line("")

# ============================================================================
# STATS DISPLAY FUNCTIONS
# ============================================================================

# Stats display functions
func _display_achievement_stats(stats: Array):
	if stats.size() == 0:
		add_output_line("📊 No achievement stats available")
		return

	add_output_line("📊 Achievement Stats (" + str(stats.size()) + " stats):")
	for i in range(stats.size()):
		var stat = stats[i]
		add_output_line("  " + str(i + 1) + ". " + str(stat.get("name", "Unknown Stat")))
		add_output_line("     Value: " + str(stat.get("value", 0)))
		add_output_line("")

# ============================================================================
# LEADERBOARD DISPLAY FUNCTIONS
# ============================================================================

# Leaderboard display functions
func _display_leaderboard_definitions(definitions: Array):
	if definitions.size() == 0:
		add_output_line("🏁 No leaderboard definitions available")
		return

	add_output_line("🏁 Leaderboard Definitions (" + str(definitions.size()) + " leaderboards):")
	for i in range(definitions.size()):
		var def = definitions[i]
		add_output_line("  " + str(i + 1) + ". " + str(def.get("leaderboard_id", "Unknown Leaderboard")))
		add_output_line("     Display Name: " + str(def.get("display_name", "No name")))
		add_output_line("     Stat Name: " + str(def.get("stat_name", "Unknown")))
		add_output_line("     Aggregation: " + str(def.get("aggregation", "Unknown")))
		add_output_line("")

func _display_leaderboard_ranks(ranks: Array):
	if ranks.size() == 0:
		add_output_line("🏁 No leaderboard ranks available")
		return

	add_output_line("🏁 Leaderboard Ranks (" + str(ranks.size()) + " entries):")
	for i in range(ranks.size()):
		var rank = ranks[i]
		var rank_number = rank.get("rank", i + 1)
		var user_id = rank.get("user_id", "Unknown User")
		var score = rank.get("score", 0)

		add_output_line("  #" + str(rank_number) + " - Score: " + str(score))
		add_output_line("     User ID: " + str(user_id))
		add_output_line("")

func _display_leaderboard_user_scores(user_scores: Dictionary):
	if user_scores.is_empty():
		add_output_line("🏁 No user scores available")
		return

	add_output_line("🏁 User Scores:")
	for user_id in user_scores:
		var score_data = user_scores[user_id]
		add_output_line("  User: " + str(user_id))
		add_output_line("     Score: " + str(score_data.get("score", 0)))
		add_output_line("     Rank: " + str(score_data.get("rank", "Unknown")))
		add_output_line("")

# ============================================================================
# AUTO-TEST FUNCTIONS
# ============================================================================

# Auto-test functions
func start_auto_test():
	add_output_line("")
	add_output_line("[color=magenta]🤖 Starting automated test sequence...[/color]")
	add_output_line("[color=magenta]Test steps: Login → Query Friends → Query Achievements[/color]")
	add_output_line("")

	auto_test_step = 0
	auto_test_time_accumulator = 0.0
	auto_test_current_delay = 2.0  # Initial delay before starting

	add_output_line("🤖 Process-based timer started (step: " + str(auto_test_step) + ", delay: " + str(auto_test_current_delay) + ")")

func _on_auto_test_timer_timeout():
	add_output_line("🤖 Process timeout triggered (step: " + str(auto_test_step) + ")")

	match auto_test_step:
		0:
			# Step 0: Login with dev id "TestUser123"
			add_output_line("[color=magenta]🤖 Auto-test Step 1/3: Logging in with TestUser123...[/color]")
			godot_epic.login_with_dev("TestUser123")
			auto_test_step = 1
			auto_test_current_delay = 5.0

		1:
			# Step 1: Query friends (only if logged in)
			if godot_epic.is_user_logged_in():
				add_output_line("[color=magenta]🤖 Auto-test Step 2/3: Querying friends...[/color]")
				godot_epic.query_friends()
				auto_test_step = 2
				auto_test_current_delay = 5.0
			else:
				add_output_line("[color=red]🤖 Auto-test failed: Not logged in, skipping friends query[/color]")
				auto_test_step = 3
				auto_test_current_delay = 5.0

		2:
			# Step 2: Query achievements (only if we have Product User ID)
			if godot_epic.is_user_logged_in():
				var product_user_id = godot_epic.get_product_user_id()
				if not product_user_id.is_empty():
					add_output_line("[color=magenta]🤖 Auto-test Step 3/3: Querying achievements...[/color]")
					godot_epic.query_player_achievements()
					auto_test_step = 3
					auto_test_current_delay = 5.0
				else:
					add_output_line("[color=orange]🤖 Auto-test: Skipping achievements (no Product User ID - Connect service unavailable)[/color]")
					auto_test_step = 3
					auto_test_current_delay = 5.0
			else:
				add_output_line("[color=red]🤖 Auto-test failed: Not logged in, skipping achievements query[/color]")
				auto_test_step = 3
				auto_test_current_delay = 5.0

		3:
			# Step 3: Finish auto-test
			add_output_line("[color=magenta]🤖 Auto-test completed![/color]")
			var product_user_id = godot_epic.get_product_user_id()
			if not product_user_id.is_empty():
				add_output_line("[color=green]✓ Cross-platform features available (Product User ID: " + product_user_id + ")[/color]")
			else:
				add_output_line("[color=orange]⚠ Cross-platform features disabled (no Product User ID)[/color]")
			add_output_line("[color=magenta]You can now interact with the demo manually.[/color]")
			add_output_line("")

			# Clean up
			auto_test_step = -1  # Disable auto-test
			auto_test_current_delay = 0.0

# ============================================================================
# CLEANUP
# ============================================================================

# Called when the node is about to be removed from the scene
func _exit_tree():
	# Clean shutdown of EOS platform
	if godot_epic:
		godot_epic.shutdown_platform()
		print("EOS Platform shut down.")
