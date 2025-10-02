extends Control

# Demo Menu Script
# Main navigation hub for all EpicOS demo scenes

@onready var platform_status: Label = $VBoxContainer/StatusPanel/StatusContainer/PlatformStatus
@onready var user_status: Label = $VBoxContainer/StatusPanel/StatusContainer/UserStatus

@onready var auth_button: Button = $VBoxContainer/DemosSection/ScrollContainer/DemosGrid/AuthenticationPanel/AuthenticationContainer/AuthButton
@onready var friends_button: Button = $VBoxContainer/DemosSection/ScrollContainer/DemosGrid/FriendsPanel/FriendsContainer/FriendsButton
@onready var achievements_button: Button = $VBoxContainer/DemosSection/ScrollContainer/DemosGrid/AchievementsPanel/AchievementsContainer/AchievementsButton
@onready var leaderboards_button: Button = $VBoxContainer/DemosSection/ScrollContainer/DemosGrid/LeaderboardsPanel/LeaderboardsContainer/LeaderboardsButton

@onready var exit_button: Button = $VBoxContainer/FooterContainer/ExitButton
@onready var refresh_button: Button = $VBoxContainer/FooterContainer/RefreshButton

func _ready():
	# Connect button signals
	auth_button.pressed.connect(_on_auth_button_pressed)
	friends_button.pressed.connect(_on_friends_button_pressed)
	achievements_button.pressed.connect(_on_achievements_button_pressed)
	leaderboards_button.pressed.connect(_on_leaderboards_button_pressed)

	exit_button.pressed.connect(_on_exit_button_pressed)
	refresh_button.pressed.connect(_on_refresh_button_pressed)

	# Connect EpicOS signals for status updates
	if EpicOS:
		EpicOS.login_completed.connect(_on_login_status_changed)
		EpicOS.logout_completed.connect(_on_logout_status_changed)

	# Enable debug mode for detailed logging
	if EpicOS:
		EpicOS.set_debug_mode(true)

	# Initial status update
	_update_status()

	print("GodotEOS Demo Menu initialized successfully!")

func _on_auth_button_pressed():
	print("Opening Authentication Demo...")
	get_tree().change_scene_to_file("res://scenes/demos/authentication_demo.tscn")

func _on_friends_button_pressed():
	print("Opening Friends Demo...")
	get_tree().change_scene_to_file("res://scenes/demos/friends_demo.tscn")

func _on_achievements_button_pressed():
	print("Opening Achievements Demo...")
	get_tree().change_scene_to_file("res://scenes/demos/achievements_demo.tscn")

func _on_leaderboards_button_pressed():
	print("Opening Leaderboards Demo...")
	get_tree().change_scene_to_file("res://scenes/demos/leaderboards_demo.tscn")

func _on_exit_button_pressed():
	print("Exiting application...")

	# Gracefully shutdown EpicOS before exiting
	if EpicOS:
		EpicOS.shutdown()

	get_tree().quit()

func _on_refresh_button_pressed():
	print("Refreshing status...")
	_update_status()

func _on_login_status_changed(success: bool, user_info: Dictionary):
	if success:
		print("User logged in - updating menu status")
	_update_status()

func _on_logout_status_changed(success: bool):
	if success:
		print("User logged out - updating menu status")
	_update_status()

func _update_status():
	var platform_initialized = false
	var user_logged_in = false
	var username = ""

	if EpicOS:
		platform_initialized = EpicOS.is_platform_initialized()
		user_logged_in = EpicOS.is_user_logged_in()
		username = EpicOS.get_current_username()

	# Update platform status
	if platform_initialized:
		platform_status.text = "Platform: ✅ Initialized"
		platform_status.modulate = Color.GREEN
	else:
		platform_status.text = "Platform: ❌ Not Initialized"
		platform_status.modulate = Color.ORANGE

	# Update user status
	if user_logged_in:
		var display_name = username if not username.is_empty() else "User"
		user_status.text = "User: ✅ Logged In (" + display_name + ")"
		user_status.modulate = Color.GREEN
	else:
		user_status.text = "User: ❌ Not Logged In"
		user_status.modulate = Color.ORANGE

	# Update button availability based on status
	_update_button_states(platform_initialized, user_logged_in)

func _update_button_states(platform_initialized: bool, user_logged_in: bool):
	# Authentication demo is always available
	auth_button.disabled = false

	# Other demos require platform initialization and user login
	var demos_available = platform_initialized and user_logged_in

	# Visual feedback for button states
	if demos_available:
		friends_button.disabled = false
		achievements_button.disabled = false
		leaderboards_button.disabled = false

		friends_button.modulate = Color.WHITE
		achievements_button.modulate = Color.WHITE
		leaderboards_button.modulate = Color.WHITE
	else:
		friends_button.disabled = true
		achievements_button.disabled = true
		leaderboards_button.disabled = true

		friends_button.modulate = Color.GRAY
		achievements_button.modulate = Color.GRAY
		leaderboards_button.modulate = Color.GRAY

# Update status periodically
func _process(_delta):
	# Update status every 2 seconds
	if Engine.get_process_frames() % 120 == 0:  # Assuming 60 FPS
		_update_status()

# Handle keyboard shortcuts
func _input(event):
	if event is InputEventKey and event.pressed:
		match event.keycode:
			KEY_1:
				_on_auth_button_pressed()
			KEY_2:
				if not friends_button.disabled:
					_on_friends_button_pressed()
			KEY_3:
				if not achievements_button.disabled:
					_on_achievements_button_pressed()
			KEY_4:
				if not leaderboards_button.disabled:
					_on_leaderboards_button_pressed()
			KEY_F5:
				_on_refresh_button_pressed()
			KEY_ESCAPE:
				_on_exit_button_pressed()

# Show helpful information on startup
func _show_startup_info():
	print("=== GodotEOS Demo Suite ===")
	print("Welcome to the Epic Online Services demonstration!")
	print("")
	print("Available demos:")
	print("1. Authentication Demo - Login/logout functionality")
	print("2. Friends Demo - Friends list and information")
	print("3. Achievements Demo - Achievement system")
	print("4. Leaderboards Demo - Leaderboards and statistics")
	print("")
	print("Keyboard shortcuts:")
	print("1-4: Open demo scenes")
	print("F5: Refresh status")
	print("ESC: Exit application")
	print("")

	if EpicOS:
		print("EpicOS singleton is available")
		if EpicOS.is_platform_initialized():
			print("Platform is already initialized")
		else:
			print("Platform needs initialization - use Authentication Demo")

		if EpicOS.is_user_logged_in():
			print("User is already logged in")
		else:
			print("User needs to log in - use Authentication Demo")
	else:
		print("WARNING: EpicOS singleton not found!")
		print("Make sure the GodotEOS extension is properly installed")

func _notification(what):
	if what == NOTIFICATION_READY:
		# Call this after everything is ready
		call_deferred("_show_startup_info")
