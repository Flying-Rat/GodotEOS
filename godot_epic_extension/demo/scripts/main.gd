extends Node2D

var godot_epic: GodotEpic = null

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

	# Example initialization options for Epic Online Services
	var init_options = {
		"product_name": "GodotEpic Demo",
		"product_version": "1.0.0",
		"product_id": "your_product_id_here",
		"sandbox_id": "your_sandbox_id_here",
		"deployment_id": "your_deployment_id_here",
		"client_id": "your_client_id_here",
		"client_secret": "your_client_secret_here",
		"encryption_key": "your_encryption_key_here"  # Optional but recommended
	}

	# Initialize the EOS platform
	var success = godot_epic.initialize_platform(init_options)
	if success:
		print("EOS Platform initialized successfully!")
		print("Ready for authentication, friends, and achievements features!")
		print("")
		print("Commands you can try:")
		print("Authentication:")
		print("- Press '1' to login with Epic Account (opens browser)")
		print("- Press '2' to login with Device ID (development)")
		print("- Press '3' to logout")
		print("")
		print("Friends:")
		print("- Press '4' to query friends list")
		print("- Press '5' to get current friends list")
		print("")
		print("Achievements:")
		print("- Press '6' to query achievement definitions")
		print("- Press '7' to query player achievements")
		print("- Press '8' to get current achievement definitions")
		print("- Press '9' to get current player achievements")
		print("- Press '0' to unlock test achievement")
		print("- Press 'Q' to get specific achievement definition")
		print("- Press 'W' to get specific player achievement")
	else:
		print("Failed to initialize EOS Platform. Check your credentials.")


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	# Tick the EOS platform to handle callbacks and updates
	if godot_epic and godot_epic.is_platform_initialized():
		godot_epic.tick()


func _input(event):
	if event is InputEventKey and event.pressed:
		match event.keycode:
			KEY_1:
				print("Starting Epic Account login...")
				godot_epic.login_with_epic_account("", "")  # Will use browser/launcher
			KEY_2:
				print("Starting Device ID login...")
				godot_epic.login_with_device_id("TestUser")
			KEY_3:
				if godot_epic.is_user_logged_in():
					print("Logging out...")
					godot_epic.logout()
				else:
					print("Not logged in!")
			KEY_4:
				if godot_epic.is_user_logged_in():
					print("Querying friends list...")
					godot_epic.query_friends()
				else:
					print("Please login first!")
			KEY_5:
				if godot_epic.is_user_logged_in():
					print("Getting current friends list...")
					var friends = godot_epic.get_friends_list()
					_display_friends_list(friends)
				else:
					print("Please login first!")
			KEY_6:
				print("Querying achievement definitions...")
				godot_epic.query_achievement_definitions()
			KEY_7:
				if godot_epic.is_user_logged_in():
					print("Querying player achievements...")
					godot_epic.query_player_achievements()
				else:
					print("Please login first!")
			KEY_8:
				print("Getting current achievement definitions...")
				var definitions = godot_epic.get_achievement_definitions()
				_display_achievement_definitions(definitions)
			KEY_9:
				if godot_epic.is_user_logged_in():
					print("Getting current player achievements...")
					var achievements = godot_epic.get_player_achievements()
					_display_player_achievements(achievements)
				else:
					print("Please login first!")
			KEY_0:
				if godot_epic.is_user_logged_in():
					# Try to unlock a test achievement - you'll need to replace "test_achievement" with a real achievement ID
					print("Attempting to unlock test achievement...")
					godot_epic.unlock_achievement("test_achievement")
				else:
					print("Please login first!")
			KEY_Q:
				print("Getting specific achievement definition...")
				var definition = godot_epic.get_achievement_definition("test_achievement")
				_display_single_achievement_definition(definition)
			KEY_W:
				if godot_epic.is_user_logged_in():
					print("Getting specific player achievement...")
					var achievement = godot_epic.get_player_achievement("test_achievement")
					_display_single_player_achievement(achievement)
				else:
					print("Please login first!")


# Signal handlers
func _on_login_completed(success: bool, username: String):
	if success:
		print("✅ Login successful!")
		print("Username: " + username)
		print("Epic Account ID: " + godot_epic.get_epic_account_id())
		print("Product User ID: " + godot_epic.get_product_user_id())
		print("")
		print("You can now use all features! Try achievements (6-0, Q, W) or friends (4-5)")
	else:
		print("❌ Login failed!")


func _on_logout_completed(success: bool):
	if success:
		print("✅ Logout successful!")
	else:
		print("❌ Logout failed!")


func _on_friends_updated(friends_list: Array):
	print("🔄 Friends list updated!")
	_display_friends_list(friends_list)


func _display_friends_list(friends: Array):
	if friends.size() == 0:
		print("📝 Friends list is empty")
		return

	print("👥 Friends list (" + str(friends.size()) + " friends):")
	for i in range(friends.size()):
		var friend = friends[i]
		print("  " + str(i + 1) + ". ID: " + str(friend.get("id", "Unknown")))
		print("     Status: " + str(friend.get("status", "Unknown")))
		print("")


# Achievement signal handlers
func _on_achievement_definitions_updated(definitions: Array):
	print("🏆 Achievement definitions updated!")
	_display_achievement_definitions(definitions)


func _on_player_achievements_updated(achievements: Array):
	print("🎯 Player achievements updated!")
	_display_player_achievements(achievements)


func _on_achievements_unlocked(unlocked_achievement_ids: Array):
	print("🎉 Achievements unlocked!")
	if unlocked_achievement_ids.size() > 0:
		print("Unlocked achievement IDs: " + str(unlocked_achievement_ids))
	else:
		print("Achievement unlock completed (specific IDs not available)")


func _on_achievement_unlocked(achievement_id: String, unlock_time: int):
	print("🏅 Achievement unlocked: " + achievement_id)
	print("Unlock time: " + str(unlock_time))


# Achievement display functions
func _display_achievement_definitions(definitions: Array):
	if definitions.size() == 0:
		print("📝 No achievement definitions available")
		return

	print("🏆 Achievement Definitions (" + str(definitions.size()) + " achievements):")
	for i in range(definitions.size()):
		var def = definitions[i]
		print("  " + str(i + 1) + ". " + str(def.get("unlocked_display_name", "Unknown Achievement")))
		print("     ID: " + str(def.get("achievement_id", "Unknown")))
		print("     Description: " + str(def.get("unlocked_description", "No description")))
		print("     Hidden: " + str(def.get("is_hidden", false)))

		var stat_thresholds = def.get("stat_thresholds", [])
		if stat_thresholds.size() > 0:
			print("     Requirements:")
			for threshold in stat_thresholds:
				print("       - " + str(threshold.get("name", "Unknown")) + ": " + str(threshold.get("threshold", 0)))
		print("")


func _display_player_achievements(achievements: Array):
	if achievements.size() == 0:
		print("📝 No player achievements available")
		return

	print("🎯 Player Achievements (" + str(achievements.size()) + " achievements):")
	var unlocked_count = 0

	for i in range(achievements.size()):
		var ach = achievements[i]
		var is_unlocked = ach.get("is_unlocked", false)
		if is_unlocked:
			unlocked_count += 1

		var status_icon = "🏅" if is_unlocked else "🔒"
		var progress = ach.get("progress", 0.0)

		print("  " + status_icon + " " + str(ach.get("display_name", "Unknown Achievement")))
		print("     ID: " + str(ach.get("achievement_id", "Unknown")))
		print("     Progress: " + str(progress) + "%")

		if is_unlocked:
			var unlock_time = ach.get("unlock_time", 0)
			if unlock_time > 0:
				print("     Unlocked: " + str(unlock_time))

		var stat_info = ach.get("stat_info", [])
		if stat_info.size() > 0:
			print("     Stats:")
			for stat in stat_info:
				var current = stat.get("current_value", 0)
				var threshold = stat.get("threshold_value", 0)
				print("       - " + str(stat.get("name", "Unknown")) + ": " + str(current) + "/" + str(threshold))
		print("")

	print("📊 Summary: " + str(unlocked_count) + "/" + str(achievements.size()) + " achievements unlocked")
	print("")


func _display_single_achievement_definition(definition: Dictionary):
	if definition.is_empty():
		print("❌ Achievement definition not found")
		return

	print("🏆 Achievement Definition:")
	print("  Name: " + str(definition.get("unlocked_display_name", "Unknown")))
	print("  ID: " + str(definition.get("achievement_id", "Unknown")))
	print("  Description: " + str(definition.get("unlocked_description", "No description")))
	print("  Locked Description: " + str(definition.get("locked_description", "No description")))
	print("  Hidden: " + str(definition.get("is_hidden", false)))

	var stat_thresholds = definition.get("stat_thresholds", [])
	if stat_thresholds.size() > 0:
		print("  Requirements:")
		for threshold in stat_thresholds:
			print("    - " + str(threshold.get("name", "Unknown")) + ": " + str(threshold.get("threshold", 0)))
	print("")


func _display_single_player_achievement(achievement: Dictionary):
	if achievement.is_empty():
		print("❌ Player achievement not found")
		return

	var is_unlocked = achievement.get("is_unlocked", false)
	var status_icon = "🏅" if is_unlocked else "🔒"

	print("🎯 Player Achievement:")
	print("  " + status_icon + " " + str(achievement.get("display_name", "Unknown")))
	print("  ID: " + str(achievement.get("achievement_id", "Unknown")))
	print("  Progress: " + str(achievement.get("progress", 0.0)) + "%")

	if is_unlocked:
		var unlock_time = achievement.get("unlock_time", 0)
		if unlock_time > 0:
			print("  Unlocked: " + str(unlock_time))

	var stat_info = achievement.get("stat_info", [])
	if stat_info.size() > 0:
		print("  Stats:")
		for stat in stat_info:
			var current = stat.get("current_value", 0)
			var threshold = stat.get("threshold_value", 0)
			print("    - " + str(stat.get("name", "Unknown")) + ": " + str(current) + "/" + str(threshold))
	print("")


# Called when the node is about to be removed from the scene
func _exit_tree():
	# Clean shutdown of EOS platform
	if godot_epic:
		godot_epic.shutdown_platform()
		print("EOS Platform shut down.")
