extends Node2D

var godot_epic: GodotEpic = null

# UI References
@onready var status_label: Label = $CanvasLayer/UI/MainContainer/ButtonsPanel/StatusLabel
@onready var output_text: RichTextLabel = $CanvasLayer/UI/MainContainer/OutputPanel/OutputScroll/OutputText

# Button references
@onready var login_epic_button: Button = $CanvasLayer/UI/MainContainer/ButtonsPanel/AuthGroup/LoginEpicButton
@onready var login_device_button: Button = $CanvasLayer/UI/MainContainer/ButtonsPanel/AuthGroup/LoginDeviceButton
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
	login_device_button.pressed.connect(_on_login_device_pressed)
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
		add_output_line("[b]Available Features:[/b]")
		add_output_line("• Authentication (Epic Account & Device ID)")
		add_output_line("• Friends Management")
		add_output_line("• Achievements System")
		add_output_line("")
		add_output_line("[i]Use the buttons on the left or keyboard shortcuts (1-9, 0, Q, W, Alt+2)[/i]")
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


# Helper function to add formatted output
func add_output_line(text: String):
	if output_text:
		output_text.append_text(text + "\n")
		print(text)  # Also print to console

func update_button_states():
	var is_logged_in = godot_epic and godot_epic.is_user_logged_in()

	# Update buttons that require login
	query_friends_button.disabled = not is_logged_in
	get_friends_button.disabled = not is_logged_in
	query_player_ach_button.disabled = not is_logged_in
	get_player_ach_button.disabled = not is_logged_in
	unlock_test_button.disabled = not is_logged_in
	get_specific_player_button.disabled = not is_logged_in

	logout_button.disabled = not is_logged_in

# Button handlers
func _on_login_epic_pressed():
	add_output_line("[color=cyan]🔐 Starting Epic Account login...[/color]")
	godot_epic.login_with_epic_account("", "")

func _on_login_device_pressed():
	add_output_line("[color=cyan]🔐 Starting Device ID login (TestUser123)...[/color]")
	godot_epic.login_with_device_id("TestUser123")

func _on_login_device2_pressed():
	add_output_line("[color=cyan]🔐 Starting Device ID login (Player1)...[/color]")
	godot_epic.login_with_device_id("Player1")

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
		add_output_line("[color=yellow]🎯 Querying player achievements...[/color]")
		godot_epic.query_player_achievements()
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

func _on_get_ach_defs_pressed():
	add_output_line("[color=yellow]🏆 Getting current achievement definitions...[/color]")
	var definitions = godot_epic.get_achievement_definitions()
	_display_achievement_definitions(definitions)

func _on_get_player_ach_pressed():
	if godot_epic.is_user_logged_in():
		add_output_line("[color=yellow]🎯 Getting current player achievements...[/color]")
		var achievements = godot_epic.get_player_achievements()
		_display_player_achievements(achievements)
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

func _on_unlock_test_pressed():
	if godot_epic.is_user_logged_in():
		add_output_line("[color=purple]🎉 Attempting to unlock test achievement...[/color]")
		godot_epic.unlock_achievement("test_achievement")
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

func _on_get_specific_def_pressed():
	add_output_line("[color=yellow]🏆 Getting specific achievement definition...[/color]")
	var definition = godot_epic.get_achievement_definition("test_achievement")
	_display_single_achievement_definition(definition)

func _on_get_specific_player_pressed():
	if godot_epic.is_user_logged_in():
		add_output_line("[color=yellow]🎯 Getting specific player achievement...[/color]")
		var achievement = godot_epic.get_player_achievement("test_achievement")
		_display_single_player_achievement(achievement)
	else:
		add_output_line("[color=red]❌ Please login first![/color]")

func _on_clear_output_pressed():
	if output_text:
		output_text.clear()
		add_output_line("[i]Output cleared[/i]")

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
				_on_login_device_pressed()
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
func _on_login_completed(success: bool, username: String):
	if success:
		status_label.text = "✅ Logged In: " + username
		add_output_line("[color=green]✅ Login successful![/color]")
		add_output_line("[b]Username:[/b] " + username)
		add_output_line("[b]Epic Account ID:[/b] " + godot_epic.get_epic_account_id())
		add_output_line("[b]Product User ID:[/b] " + godot_epic.get_product_user_id())
		add_output_line("")
		add_output_line("[color=green]🎉 You can now use all features![/color]")
		update_button_states()
	else:
		status_label.text = "❌ Login Failed"
		add_output_line("[color=red]❌ Login failed![/color]")
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
		add_output_line("[color=gray]📝 Friends list is empty[/color]")
		return

	add_output_line("[b]👥 Friends list (" + str(friends.size()) + " friends):[/b]")
	for i in range(friends.size()):
		var friend = friends[i]
		add_output_line("  " + str(i + 1) + ". [b]ID:[/b] " + str(friend.get("id", "Unknown")))
		add_output_line("     [b]Status:[/b] " + str(friend.get("status", "Unknown")))
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
		add_output_line("[b]Unlocked achievement IDs:[/b] " + str(unlocked_achievement_ids))
	else:
		add_output_line("[i]Achievement unlock completed (specific IDs not available)[/i]")


func _on_achievement_unlocked(achievement_id: String, unlock_time: int):
	add_output_line("[color=purple]🏅 Achievement unlocked: [b]" + achievement_id + "[/b][/color]")
	add_output_line("[b]Unlock time:[/b] " + str(unlock_time))


# Achievement display functions
func _display_achievement_definitions(definitions: Array):
	if definitions.size() == 0:
		add_output_line("[color=gray]📝 No achievement definitions available[/color]")
		return

	add_output_line("[b]🏆 Achievement Definitions (" + str(definitions.size()) + " achievements):[/b]")
	for i in range(definitions.size()):
		var def = definitions[i]
		add_output_line("  " + str(i + 1) + ". [b]" + str(def.get("unlocked_display_name", "Unknown Achievement")) + "[/b]")
		add_output_line("     [b]ID:[/b] " + str(def.get("achievement_id", "Unknown")))
		add_output_line("     [b]Description:[/b] " + str(def.get("unlocked_description", "No description")))
		add_output_line("     [b]Hidden:[/b] " + str(def.get("is_hidden", false)))

		var stat_thresholds = def.get("stat_thresholds", [])
		if stat_thresholds.size() > 0:
			add_output_line("     [b]Requirements:[/b]")
			for threshold in stat_thresholds:
				add_output_line("       • " + str(threshold.get("name", "Unknown")) + ": " + str(threshold.get("threshold", 0)))
		add_output_line("")


func _display_player_achievements(achievements: Array):
	if achievements.size() == 0:
		add_output_line("[color=gray]📝 No player achievements available[/color]")
		return

	add_output_line("[b]🎯 Player Achievements (" + str(achievements.size()) + " achievements):[/b]")
	var unlocked_count = 0

	for i in range(achievements.size()):
		var ach = achievements[i]
		var is_unlocked = ach.get("is_unlocked", false)
		if is_unlocked:
			unlocked_count += 1

		var status_icon = "🏅" if is_unlocked else "🔒"
		var progress = ach.get("progress", 0.0)
		var progress_color = "green" if is_unlocked else "orange"

		add_output_line("  " + status_icon + " [b]" + str(ach.get("display_name", "Unknown Achievement")) + "[/b]")
		add_output_line("     [b]ID:[/b] " + str(ach.get("achievement_id", "Unknown")))
		add_output_line("     [color=" + progress_color + "][b]Progress:[/b] " + str(progress) + "%[/color]")

		if is_unlocked:
			var unlock_time = ach.get("unlock_time", 0)
			if unlock_time > 0:
				add_output_line("     [color=green][b]Unlocked:[/b] " + str(unlock_time) + "[/color]")

		var stat_info = ach.get("stat_info", [])
		if stat_info.size() > 0:
			add_output_line("     [b]Stats:[/b]")
			for stat in stat_info:
				var current = stat.get("current_value", 0)
				var threshold = stat.get("threshold_value", 0)
				add_output_line("       • " + str(stat.get("name", "Unknown")) + ": " + str(current) + "/" + str(threshold))
		add_output_line("")

	add_output_line("[b]📊 Summary: [color=green]" + str(unlocked_count) + "[/color]/" + str(achievements.size()) + " achievements unlocked[/b]")
	add_output_line("")


func _display_single_achievement_definition(definition: Dictionary):
	if definition.is_empty():
		add_output_line("[color=red]❌ Achievement definition not found[/color]")
		return

	add_output_line("[b]🏆 Achievement Definition:[/b]")
	add_output_line("  [b]Name:[/b] " + str(definition.get("unlocked_display_name", "Unknown")))
	add_output_line("  [b]ID:[/b] " + str(definition.get("achievement_id", "Unknown")))
	add_output_line("  [b]Description:[/b] " + str(definition.get("unlocked_description", "No description")))
	add_output_line("  [b]Locked Description:[/b] " + str(definition.get("locked_description", "No description")))
	add_output_line("  [b]Hidden:[/b] " + str(definition.get("is_hidden", false)))

	var stat_thresholds = definition.get("stat_thresholds", [])
	if stat_thresholds.size() > 0:
		add_output_line("  [b]Requirements:[/b]")
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

	add_output_line("[b]🎯 Player Achievement:[/b]")
	add_output_line("  " + status_icon + " [b]" + str(achievement.get("display_name", "Unknown")) + "[/b]")
	add_output_line("  [b]ID:[/b] " + str(achievement.get("achievement_id", "Unknown")))
	add_output_line("  [color=" + progress_color + "][b]Progress:[/b] " + str(achievement.get("progress", 0.0)) + "%[/color]")

	if is_unlocked:
		var unlock_time = achievement.get("unlock_time", 0)
		if unlock_time > 0:
			add_output_line("  [color=green][b]Unlocked:[/b] " + str(unlock_time) + "[/color]")

	var stat_info = achievement.get("stat_info", [])
	if stat_info.size() > 0:
		add_output_line("  [b]Stats:[/b]")
		for stat in stat_info:
			var current = stat.get("current_value", 0)
			var threshold = stat.get("threshold_value", 0)
			add_output_line("    • " + str(stat.get("name", "Unknown")) + ": " + str(current) + "/" + str(threshold))
	add_output_line("")


# Called when the node is about to be removed from the scene
func _exit_tree():
	# Clean shutdown of EOS platform
	if godot_epic:
		godot_epic.shutdown_platform()
		print("EOS Platform shut down.")
