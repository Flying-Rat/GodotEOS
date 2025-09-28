extends Control

# Main demo scene script showcasing EOS integration

@onready var login_button = $VBoxContainer/LoginSection/LoginButton
@onready var logout_button = $VBoxContainer/LoginSection/LogoutButton
@onready var status_label = $VBoxContainer/StatusLabel

@onready var achievement_button = $VBoxContainer/FeaturesSection/AchievementButton
@onready var stats_button = $VBoxContainer/FeaturesSection/StatsButton
@onready var leaderboard_button = $VBoxContainer/FeaturesSection/LeaderboardButton
@onready var save_button = $VBoxContainer/FeaturesSection/FeaturesGrid/SaveButton
@onready var load_button = $VBoxContainer/FeaturesSection/FeaturesGrid/LoadButton

@onready var output_text = $VBoxContainer/OutputSection/ScrollContainer/OutputText

func _ready():
	# Connect UI signals
	login_button.pressed.connect(_on_login_pressed)
	logout_button.pressed.connect(_on_logout_pressed)
	achievement_button.pressed.connect(_on_achievement_pressed)
	stats_button.pressed.connect(_on_stats_pressed)
	leaderboard_button.pressed.connect(_on_leaderboard_pressed)
	save_button.pressed.connect(_on_save_pressed)
	load_button.pressed.connect(_on_load_pressed)

	# Connect EOS signals
	EpicOS.login_completed.connect(_on_login_completed)
	EpicOS.achievement_unlocked.connect(_on_achievement_unlocked)
	EpicOS.stats_updated.connect(_on_stats_updated)
	EpicOS.leaderboard_retrieved.connect(_on_leaderboard_retrieved)
	EpicOS.file_saved.connect(_on_file_saved)
	EpicOS.file_loaded.connect(_on_file_loaded)

	# Initial setup
	EpicOS.set_debug_mode(true)
	EpicOS.initialize()

	update_ui_state()
	_add_output("GodotEpic Demo Started")
	_add_output("Initialize EOS and try logging in!")

func update_ui_state():
	var logged_in = EpicOS.is_logged_in()
	login_button.disabled = logged_in
	logout_button.disabled = not logged_in

	# Feature buttons only available when logged in
	achievement_button.disabled = not logged_in
	stats_button.disabled = not logged_in
	leaderboard_button.disabled = not logged_in
	save_button.disabled = not logged_in
	load_button.disabled = not logged_in

	status_label.text = "Status: " + ("Logged In" if logged_in else "Not Logged In")

func _add_output(message: String):
	var timestamp = "[%s] " % Time.get_datetime_string_from_system()
	output_text.text += timestamp + message + "\n"
	# Auto scroll to bottom
	await get_tree().process_frame
	var scroll_container = output_text.get_parent().get_parent()
	if scroll_container is ScrollContainer:
		scroll_container.scroll_vertical = scroll_container.get_v_scroll_bar().max_value

# UI Event Handlers
func _on_login_pressed():
	_add_output("Starting login process...")
	_add_output("Note: For Device ID login, make sure EOS Dev Auth Tool is running on localhost:7777")
	_add_output("For Epic Account login, it will open browser or use Epic Launcher")
	EpicOS.login()

func _on_logout_pressed():
	_add_output("Logging out...")
	EpicOS.logout()
	update_ui_state()

func _on_achievement_pressed():
	_add_output("Unlocking test achievement...")
	EpicOS.unlock_achievement("first_login")

func _on_stats_pressed():
	_add_output("Updating stats...")
	var random_score = randi_range(100, 1000)
	EpicOS.update_stat("demo_score", random_score)

	# Also get current stats
	var stats = EpicOS.get_stats()
	_add_output("Current stats: " + str(stats))

func _on_leaderboard_pressed():
	_add_output("Submitting score and getting leaderboard...")
	var score = randi_range(500, 2000)
	EpicOS.submit_score("demo_leaderboard", score)
	EpicOS.get_leaderboard("demo_leaderboard", 5)

func _on_save_pressed():
	_add_output("Saving game data to cloud...")
	var save_data = {
		"player_name": "Demo Player",
		"level": randi_range(1, 10),
		"score": randi_range(1000, 5000),
		"timestamp": Time.get_unix_time_from_system()
	}
	EpicOS.save_file("demo_save.json", JSON.stringify(save_data))

func _on_load_pressed():
	_add_output("Loading game data from cloud...")
	EpicOS.load_file("demo_save.json")

# EOS Event Handlers
func _on_login_completed(success: bool, user_info: Dictionary):
	if success:
		_add_output("Login successful! Welcome " + user_info.display_name)
		_add_output("Epic Account ID: " + str(user_info.get("epic_account_id", "Not available")))
		_add_output("Product User ID: " + str(user_info.get("product_user_id", "Not set - Connect service failed")))

		if user_info.has("product_user_id") and user_info.product_user_id != "":
			_add_output("✓ Cross-platform features enabled")
		else:
			_add_output("⚠ Cross-platform features disabled (Connect login failed)")
	else:
		_add_output("Login failed! Check the error message above for details.")
		_add_output("Common issues:")
		_add_output("- Error 10: For Device ID, run EOS Dev Auth Tool on localhost:7777")
		_add_output("- Error 10: Check email/password format for Epic Account login")
		_add_output("- Error 32: Check deployment_id in EOS Developer Portal")
	update_ui_state()

func _on_achievement_unlocked(achievement_id: String):
	_add_output("Achievement unlocked: " + achievement_id)

func _on_stats_updated(stats: Dictionary):
	_add_output("Stats updated: " + str(stats))

func _on_leaderboard_retrieved(leaderboard_data: Array):
	_add_output("Leaderboard data received:")
	for entry in leaderboard_data:
		_add_output("  #%d - %s: %d points" % [entry.rank, entry.display_name, entry.score])

func _on_file_saved(success: bool, filename: String):
	if success:
		_add_output("File saved successfully: " + filename)
	else:
		_add_output("Failed to save file: " + filename)

func _on_file_loaded(success: bool, filename: String, data: String):
	if success:
		_add_output("File loaded successfully: " + filename)
		_add_output("Data: " + data)
	else:
		_add_output("Failed to load file: " + filename)