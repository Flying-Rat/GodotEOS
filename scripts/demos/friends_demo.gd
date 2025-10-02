extends Control

# Friends Demo Script
# Demonstrates how to use the EpicOS friends features

@onready var status_label: Label = $VBoxContainer/HeaderContainer/StatusPanel/StatusContainer/StatusLabel
@onready var query_friends_button: Button = $VBoxContainer/ActionsSection/QueryFriendsButton
@onready var query_all_info_button: Button = $VBoxContainer/ActionsSection/QueryAllInfoButton
@onready var refresh_button: Button = $VBoxContainer/HeaderContainer/RefreshButton
@onready var friends_list: ItemList = $VBoxContainer/FriendsSection/FriendsListPanel/FriendsListContainer/FriendsList
@onready var selected_friend_label: Label = $VBoxContainer/FriendsSection/FriendDetailsPanel/FriendDetailsContainer/FriendDetailsLabel
@onready var friend_info_text: RichTextLabel = $VBoxContainer/FriendsSection/FriendDetailsPanel/FriendDetailsContainer/FriendInfoScrollContainer/FriendInfoText
@onready var query_friend_info_button: Button = $VBoxContainer/ActionsSection/QueryFriendInfoButton
@onready var output_text: RichTextLabel = $VBoxContainer/OutputSection/OutputText
@onready var back_button: Button = $VBoxContainer/HeaderContainer/BackButton

var cached_friends: Array = []
var selected_friend_id: String = ""

func _ready():
	# Connect button signals
	query_friends_button.pressed.connect(_on_query_friends_button_pressed)
	query_all_info_button.pressed.connect(_on_query_all_info_button_pressed)
	refresh_button.pressed.connect(_on_refresh_button_pressed)
	query_friend_info_button.pressed.connect(_on_query_friend_info_button_pressed)
	back_button.pressed.connect(_on_back_button_pressed)

	# Connect friends list selection
	friends_list.item_selected.connect(_on_friends_list_item_selected)

	# Connect EpicOS signals
	if EpicOS:
		EpicOS.friends_query_completed.connect(_on_friends_query_completed)
		EpicOS.friend_info_query_completed.connect(_on_friend_info_query_completed)
		EpicOS.login_completed.connect(_on_login_status_changed)
		EpicOS.logout_completed.connect(_on_logout_status_changed)

	# Enable debug mode for detailed logging
	if EpicOS:
		EpicOS.set_debug_mode(true)

	_update_ui_state()
	_log_message("[color=cyan]Friends Demo initialized. Ready to demonstrate EpicOS friends functionality.[/color]")

func _on_query_friends_button_pressed():
	_log_message("[color=yellow]Querying friends list...[/color]")

	if EpicOS:
		EpicOS.query_friends()
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_query_all_info_button_pressed():
	_log_message("[color=yellow]Querying detailed info for all friends...[/color]")

	if EpicOS:
		EpicOS.query_all_friends_info()
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_refresh_button_pressed():
	_log_message("[color=yellow]Refreshing friends display from cache...[/color]")
	_refresh_friends_display()

func _on_query_friend_info_button_pressed():
	if selected_friend_id.is_empty():
		_log_message("[color=red]No friend selected![/color]")
		return

	_log_message("[color=yellow]Querying info for friend: " + selected_friend_id + "[/color]")

	if EpicOS:
		EpicOS.query_friend_info(selected_friend_id)
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_back_button_pressed():
	# Navigate back to demo menu
	get_tree().change_scene_to_file("res://scenes/demos/demo_menu.tscn")

func _on_friends_list_item_selected(index: int):
	if index >= 0 and index < cached_friends.size():
		var friend_data = cached_friends[index]
		if friend_data.has("id"):
			selected_friend_id = friend_data["id"]
			selected_friend_label.text = "Selected: " + str(friend_data.get("display_name", "Unknown"))
			_update_friend_details(friend_data)
			_update_ui_state()
		else:
			_log_message("[color=red]Invalid friend data![/color]")

func _on_friends_query_completed(success: bool, friends_list_data: Array):
	if success:
		_log_message("[color=green]✓ Friends query completed successfully![/color]")
		_log_message("[color=green]Found " + str(friends_list_data.size()) + " friends[/color]")

		cached_friends = friends_list_data
		_refresh_friends_display()
	else:
		_log_message("[color=red]✗ Friends query failed![/color]")

func _on_friend_info_query_completed(success: bool, friend_info: Dictionary):
	if success:
		_log_message("[color=green]✓ Friend info query completed![/color]")
		_log_message("[color=green]Friend info: " + str(friend_info) + "[/color]")

		# Update the cached friend data
		for i in range(cached_friends.size()):
			var friend = cached_friends[i]
			if friend.has("friend_id") and friend["friend_id"] == friend_info.get("friend_id", ""):
				# Merge the new info with existing data
				for key in friend_info:
					friend[key] = friend_info[key]
				break

		# Update display if this is the selected friend
		if not selected_friend_id.is_empty() and friend_info.get("friend_id", "") == selected_friend_id:
			_update_friend_details(friend_info)
	else:
		_log_message("[color=red]✗ Friend info query failed![/color]")

func _on_login_status_changed(success: bool, user_info: Dictionary):
	if success:
		_log_message("[color=green]User logged in - friends features now available[/color]")
	_update_ui_state()

func _on_logout_status_changed(success: bool):
	if success:
		_log_message("[color=yellow]User logged out - clearing friends data[/color]")
		cached_friends.clear()
		selected_friend_id = ""
		_refresh_friends_display()
	_update_ui_state()

func _refresh_friends_display():
	friends_list.clear()

	if cached_friends.is_empty():
		_log_message("[color=yellow]No friends data to display. Try querying friends first.[/color]")
		return

	for friend in cached_friends:
		var display_text = str(friend.get("display_name", "Unknown Friend"))
		var status = friend.get("status", "Unknown")

		# Add status indicator
		if status == "online":
			display_text = "🟢 " + display_text
		elif status == "offline":
			display_text = "🔴 " + display_text
		else:
			display_text = "⚪ " + display_text

		friends_list.add_item(display_text)

	_log_message("[color=cyan]Refreshed friends display with " + str(cached_friends.size()) + " friends[/color]")

func _update_friend_details(friend_data: Dictionary):
	var details_text = "[color=cyan]Friend Details:[/color]\n\n"

	# Display all available friend information
	for key in friend_data:
		var value = str(friend_data[key])
		details_text += "[color=yellow]" + key + ":[/color] " + value + "\n"

	if friend_data.is_empty():
		details_text = "[color=gray]No detailed information available.\nTry querying friend info.[/color]"

	friend_info_text.text = details_text

func _update_ui_state():
	var is_logged_in = false
	var platform_initialized = false

	if EpicOS:
		is_logged_in = EpicOS.is_user_logged_in()
		platform_initialized = EpicOS.is_platform_initialized()

	# Update status label
	if not platform_initialized:
		status_label.text = "Status: Platform not initialized"
	elif not is_logged_in:
		status_label.text = "Status: Not logged in (Friends require authentication)"
	else:
		status_label.text = "Status: Logged in - Friends features available"

	# Enable/disable buttons based on authentication status
	var friends_available = platform_initialized and is_logged_in
	query_friends_button.disabled = not friends_available
	query_all_info_button.disabled = not friends_available
	query_friend_info_button.disabled = not friends_available or selected_friend_id.is_empty()

func _log_message(message: String):
	if output_text:
		output_text.append_text(message + "\n")

# Update UI state periodically
func _process(_delta):
	# Update UI state periodically (every second)
	if Engine.get_process_frames() % 60 == 0:  # Assuming 60 FPS
		_update_ui_state()

# Demonstrate using cached friends data
func _demonstrate_cached_data():
	if not EpicOS:
		return

	var cached_friends_list = EpicOS.get_friends_list()
	_log_message("[color=cyan]Cached friends count: " + str(cached_friends_list.size()) + "[/color]")

	for friend in cached_friends_list:
		var friend_id = friend.get("friend_id", "")
		if not friend_id.is_empty():
			var cached_info = EpicOS.get_friend_info(friend_id)
			_log_message("[color=cyan]Cached info for " + friend_id + ": " + str(cached_info) + "[/color]")
