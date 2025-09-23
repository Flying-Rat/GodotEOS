extends Node2D

var godot_epic: GodotEpic = null

# Called when the node enters the scene tree for the first time.
func _ready():
	godot_epic = GodotEpic.get_singleton()

	# Connect to signals for async operations
	godot_epic.connect("login_completed", _on_login_completed)
	godot_epic.connect("logout_completed", _on_logout_completed)
	godot_epic.connect("friends_updated", _on_friends_updated)

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
		print("Ready for authentication and friends features!")
		print("")
		print("Commands you can try:")
		print("- Press '1' to login with Epic Account (opens browser)")
		print("- Press '2' to login with Device ID (development)")
		print("- Press '3' to logout")
		print("- Press '4' to query friends list")
		print("- Press '5' to get current friends list")
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


# Signal handlers
func _on_login_completed(success: bool, username: String):
	if success:
		print("✅ Login successful!")
		print("Username: " + username)
		print("Epic Account ID: " + godot_epic.get_epic_account_id())
		print("Product User ID: " + godot_epic.get_product_user_id())
		print("")
		print("You can now query friends (press '4') or get friends list (press '5')")
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


# Called when the node is about to be removed from the scene
func _exit_tree():
	# Clean shutdown of EOS platform
	if godot_epic:
		godot_epic.shutdown_platform()
		print("EOS Platform shut down.")
