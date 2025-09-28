extends Node2D

var godot_epic: GodotEpic = null

# UI References
@onready var status_label: Label = $CanvasLayer/UI/MainContainer/ButtonsPanel/StatusLabel
@onready var output_text: RichTextLabel = $CanvasLayer/UI/MainContainer/OutputPanel/OutputScroll/OutputText

# Button references
@onready var login_epic_button: Button = $CanvasLayer/UI/MainContainer/ButtonsPanel/AuthGroup/LoginEpicButton
@onready var login_device1_button: Button = $CanvasLayer/UI/MainContainer/ButtonsPanel/AuthGroup/LoginDevice1Button
@onready var login_device2_button: Button = $CanvasLayer/UI/MainContainer/ButtonsPanel/AuthGroup/LoginDevice2Button
@onready var logout_button: Button = $CanvasLayer/UI/MainContainer/ButtonsPanel/AuthGroup/LogoutButton
@onready var query_friends_button: Button = $CanvasLayer/UI/MainContainer/ButtonsPanel/FriendsGroup/QueryFriendsButton
@onready var get_friends_button: Button = $CanvasLayer/UI/MainContainer/ButtonsPanel/FriendsGroup/GetFriendsButton
@onready var query_ach_defs_button: Button = $CanvasLayer/UI/MainContainer/ButtonsPanel/AchievementsGroup/QueryAchDefsButton
@onready var query_player_ach_button: Button = $CanvasLayer/UI/MainContainer/ButtonsPanel/AchievementsGroup/QueryPlayerAchButton
@onready var get_ach_defs_button: Button = $CanvasLayer/UI/MainContainer/ButtonsPanel/AchievementsGroup/GetAchDefsButton
@onready var get_player_ach_button: Button = $CanvasLayer/UI/MainContainer/ButtonsPanel/AchievementsGroup/GetPlayerAchButton
@onready var unlock_test_button: Button = $CanvasLayer/UI/MainContainer/ButtonsPanel/AchievementsGroup/UnlockTestButton
@onready var get_specific_def_button: Button = $CanvasLayer/UI/MainContainer/ButtonsPanel/AchievementsGroup/GetSpecificDefButton
@onready var get_specific_player_button: Button = $CanvasLayer/UI/MainContainer/ButtonsPanel/AchievementsGroup/GetSpecificPlayerButton
@onready var clear_output_button: Button = $CanvasLayer/UI/MainContainer/ButtonsPanel/ClearOutputButton
@onready var test_subsystem_button: Button = $CanvasLayer/UI/MainContainer/ButtonsPanel/TestSubsystemButton

# Auto-test variables
var auto_test_timer: Timer = null
var auto_test_step: int = 0
var auto_test_enabled: bool = true  # Set to false to disable auto-test
var auto_test_time_accumulator: float = 0.0
var auto_test_current_delay: float = 0.0

# Called when the node enters the scene tree for the first time.
func _ready():
	godot_epic = GodotEpic.get_singleton()

	# Connect to signals for async operations
	godot_epic.connect("login_completed", _on_login_completed)
	godot_epic.connect("logout_completed", _on_logout_completed)
	godot_epic.connect("friends_updated", _on_friends_updated)
	godot_epic.connect("achievement_definitions_updated", _on_achievement_definitions_updated)
	godot_epic.connect("player_achievements_updated", _on_player_achievements_updated)
	godot_epic.connect("achievements_unlocked", _on_achievements_unlocked)
	godot_epic.connect("achievement_unlocked", _on_achievement_unlocked)

	# Connect button signals
	login_epic_button.pressed.connect(_on_login_epic_pressed)
	login_device1_button.pressed.connect(_on_login_device1_pressed)
	login_device2_button.pressed.connect(_on_login_device2_pressed)
	logout_button.pressed.connect(_on_logout_pressed)
	query_friends_button.pressed.connect(_on_query_friends_pressed)
	get_friends_button.pressed.connect(_on_get_friends_pressed)
	query_ach_defs_button.pressed.connect(_on_query_ach_defs_pressed)
	query_player_ach_button.pressed.connect(_on_query_player_ach_pressed)
	get_ach_defs_button.pressed.connect(_on_get_ach_defs_pressed)
	get_player_ach_button.pressed.connect(_on_get_player_ach_pressed)
	unlock_test_button.pressed.connect(_on_unlock_test_pressed)
	get_specific_def_button.pressed.connect(_on_get_specific_def_pressed)
	get_specific_player_button.pressed.connect(_on_get_specific_player_pressed)
	clear_output_button.pressed.connect(_on_clear_output_pressed)
	test_subsystem_button.pressed.connect(_on_test_subsystem_pressed)

	# Example initialization options for Epic Online Services
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
		add_output_line("")
		add_output_line("[i]Use the buttons on the left or keyboard shortcuts (1-9, 0, Q, W, Alt+2)[/i]")
		
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
		godot_epic.tick()
	
	# Handle auto-test timing
	if auto_test_enabled and auto_test_step >= 0 and auto_test_current_delay > 0:
		auto_test_time_accumulator += _delta
		if auto_test_time_accumulator >= auto_test_current_delay:
			auto_test_time_accumulator = 0.0
			auto_test_current_delay = 0.0
			_on_auto_test_timer_timeout()


# Helper function to add formatted output
func add_output_line(text: String):
	if output_text:
		output_text.append_text(text + "\n")
		print(text)  # Also print to console

func update_button_states():
	var is_logged_in = godot_epic and godot_epic.is_user_logged_in()
	var has_product_user_id = godot_epic and not godot_epic.get_product_user_id().is_empty()

	# Update buttons that require login
	query_friends_button.disabled = not is_logged_in
	get_friends_button.disabled = not is_logged_in
	
	# Achievements require Product User ID (cross-platform features)
	query_player_ach_button.disabled = not (is_logged_in and has_product_user_id)
	get_player_ach_button.disabled = not (is_logged_in and has_product_user_id)
	unlock_test_button.disabled = not (is_logged_in and has_product_user_id)
	get_specific_player_button.disabled = not (is_logged_in and has_product_user_id)

	logout_button.disabled = not is_logged_in

# Button handlers
func _on_login_epic_pressed():
	add_output_line("[color=cyan]🔐 Starting Epic Account login...[/color]")
	godot_epic.login_with_epic_account("", "")

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
			godot_epic.unlock_achievement("test_achievement")
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

func _on_clear_output_pressed():
	if output_text:
		output_text.clear()
		add_output_line("[i]Output cleared[/i]")

func _on_test_subsystem_pressed():
	add_output_line("[color=magenta]🔧 Testing Subsystem Manager functionality...[/color]")
	var test_results = godot_epic.test_subsystem_manager()

	# Display test results in UI
	if test_results.has("all_tests_passed"):
		var passed = test_results["all_tests_passed"]
		var status_color = "green" if passed else "red"
		var status_icon = "✅" if passed else "❌"
		add_output_line("[" + status_color + "]" + status_icon + " Overall Test Result: " + ("PASSED" if passed else "FAILED") + "[/" + status_color + "]")

	if test_results.has("subsystem_count"):
		add_output_line("Subsystem Count: " + str(test_results["subsystem_count"]))

	if test_results.has("test_messages"):
		var messages = test_results["test_messages"]
		if messages.size() > 0:
			add_output_line("Test Details:")
			for msg in messages:
				add_output_line("  " + str(msg))
		else:
			add_output_line("No test messages available")

	add_output_line("")

func _input(event):
	if event is InputEventKey and event.pressed:
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
		update_button_states()
	else:
		status_label.text = "❌ Login Failed"
		add_output_line("[color=red]❌ Login failed! Check the error message above for details.[/color]")
		add_output_line("[color=yellow]Common issues:[/color]")
		add_output_line("• [color=yellow]Error 10: For Device ID, run EOS Dev Auth Tool on localhost:7777[/color]")
		add_output_line("• [color=yellow]Error 10: Check email/password format for Epic Account login[/color]")
		add_output_line("• [color=yellow]Error 32: Check deployment_id in EOS Developer Portal[/color]")
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
		add_output_line("  " + str(i + 1) + ". ID: " + str(friend.get("id", "Unknown")))
		add_output_line("     Status: " + str(friend.get("status", "Unknown")))
		add_output_line("")


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


# Called when the node is about to be removed from the scene
func _exit_tree():
	# Clean shutdown of EOS platform
	if godot_epic:
		godot_epic.shutdown_platform()
		print("EOS Platform shut down.")
