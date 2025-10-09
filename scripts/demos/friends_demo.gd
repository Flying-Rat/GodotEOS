extends Control

# Friends Demo Script
# Demonstrates EpicOS friends features
# Assumes user is already logged in via Authentication Demo

# ============================================================================
# UI REFERENCES
# ============================================================================

@onready var status_label: Label = $VBoxContainer/HeaderContainer/StatusPanel/StatusContainer/StatusLabel
@onready var query_friends_button: Button = $VBoxContainer/ActionsSection/QueryFriendsButton
@onready var query_friend_info_button: Button = $VBoxContainer/ActionsSection/QueryFriendInfoButton
@onready var query_product_id_button: Button = $VBoxContainer/ActionsSection/QueryProductIdButton
@onready var friends_list: ItemList = $VBoxContainer/FriendsSection/FriendsListPanel/FriendsListContainer/FriendsList
@onready var selected_friend_label: Label = $VBoxContainer/FriendsSection/FriendDetailsPanel/FriendDetailsContainer/FriendDetailsLabel
@onready var friend_info_text: RichTextLabel = $VBoxContainer/FriendsSection/FriendDetailsPanel/FriendDetailsContainer/FriendInfoScrollContainer/FriendInfoText
@onready var output_text: RichTextLabel = $VBoxContainer/OutputPanel/VBoxContainer/OutputScrollContainer/OutputText
@onready var auto_scroll_checkbox: CheckButton = $VBoxContainer/OutputPanel/VBoxContainer/OutputHeaderHBox/AutoScrollCheckbox
@onready var clear_log_button: Button = $VBoxContainer/OutputPanel/VBoxContainer/OutputHeaderHBox/ClearLogButton
@onready var back_button: Button = $VBoxContainer/HeaderContainer/BackButton

# ============================================================================
# STATE VARIABLES
# ============================================================================

var cached_friends: Array = []
var selected_friend_id: String = ""

# ============================================================================
# INITIALIZATION
# ============================================================================

func _ready():
	# Connect button signals
	query_friends_button.pressed.connect(_on_query_friends_button_pressed)
	query_friend_info_button.pressed.connect(_on_query_friend_info_button_pressed)
	query_product_id_button.pressed.connect(_on_query_product_id_button_pressed)
	back_button.pressed.connect(_on_back_button_pressed)

	# Connect output controls
	auto_scroll_checkbox.toggled.connect(_on_auto_scroll_toggled)
	clear_log_button.pressed.connect(_on_clear_log_pressed)

	# Connect friends list selection
	friends_list.item_selected.connect(_on_friends_list_item_selected)

	# Connect EpicOS signals
	if EpicOS:
		EpicOS.friends_query_completed.connect(_on_friends_query_completed)
		EpicOS.user_info_query_completed.connect(_on_friend_info_query_completed)
		EpicOS.login_completed.connect(_on_login_status_changed)
		EpicOS.logout_completed.connect(_on_logout_status_changed)
		EpicOS.user_cache_updated.connect(_on_user_cache_updated)

	# Enable debug mode for detailed logging
	if EpicOS:
		EpicOS.set_debug_mode(true)

	_update_ui_state()
	_log_message("[color=cyan]═══════════════════════════════════════[/color]")
	_log_message("[color=cyan]Friends Demo Initialized[/color]")
	_log_message("[color=cyan]═══════════════════════════════════════[/color]")

	if EpicOS and EpicOS.is_user_logged_in():
		_log_message("[color=yellow]📋 INSTRUCTIONS:[/color]")
		_log_message("[color=white]1. Click 'Query Friends List' to get your friends[/color]")
		_log_message("[color=white]2. Select a friend and click 'Query Selected Friend Info' to get their details[/color]")
		_log_message("[color=white]3. Or click 'Query Product ID' to specifically fetch the Product User ID[/color]")
		_log_message("")
		_log_message("[color=yellow]Please use the buttons above to test friends functions[/color]")
	else:
		_log_message("[color=yellow]Please login first (use Authentication Demo)[/color]")

# ============================================================================
# BUTTON HANDLERS
# ============================================================================

func _on_query_friends_button_pressed():
	_log_message("[color=yellow]🔍 QueryFriends() - Fetching friends list from EOS...[/color]")
	if EpicOS:
		EpicOS.query_friends()
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

func _on_query_friend_info_button_pressed():
	if selected_friend_id.is_empty():
		_log_message("[color=red]✗ No friend selected! Select a friend from the list first.[/color]")
		return

	_log_message("[color=yellow]📋 QueryUserInfo() - Fetching info for selected friend: " + selected_friend_id + "[/color]")

	if EpicOS:
		EpicOS.query_user_info(selected_friend_id)
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

func _on_query_product_id_button_pressed():
	if selected_friend_id.is_empty():
		_log_message("[color=yellow]🔗 ForceQueryProductId() - Force re-querying Product ID for current user[/color]")

		if EpicOS:
			# Force re-query the Product ID for the current user, allowing multiple attempts
			EpicOS.force_query_product_id()
		else:
			_log_message("[color=red]✗ EpicOS not available[/color]")
	else:
		_log_message("[color=yellow]🔗 ForceQueryProductIdForUser() - Force re-querying Product ID for selected friend: " + selected_friend_id + "[/color]")

		if EpicOS:
			# Force re-query the Product ID for the selected friend
			EpicOS.force_query_product_id(selected_friend_id)
		else:
			_log_message("[color=red]✗ EpicOS not available[/color]")

func _on_back_button_pressed():
	get_tree().change_scene_to_file("res://scenes/demos/demo_menu.tscn")

# ============================================================================
# LIST ITEM HANDLERS
# ============================================================================

func _on_friends_list_item_selected(index: int):
	if index >= 0 and index < cached_friends.size():
		var friend_data = cached_friends[index]
		if friend_data.has("id"):
			selected_friend_id = friend_data["id"]
			selected_friend_label.text = "Selected: " + str(friend_data.get("display_name", "Unknown"))
			_update_friend_details(selected_friend_id)
			_update_ui_state()
			_log_message("[color=blue]Selected friend: " + str(friend_data.get("display_name", "Unknown")) + "[/color]")
		else:
			_log_message("[color=red]✗ Invalid friend data![/color]")

# ============================================================================
# EPICOS SIGNAL HANDLERS
# ============================================================================

func _on_friends_query_completed(success: bool, friends_list_data: Array):
	if success:
		_log_message("[color=green]✓ QueryFriends() completed![/color]")
		_log_message("[color=green]Found " + str(friends_list_data.size()) + " friends[/color]")

		cached_friends = friends_list_data
		_refresh_friends_display()
	else:
		_log_message("[color=red]✗ Friends list query failed![/color]")

func _on_friend_info_query_completed(success: bool, friend_info: Dictionary):
	if success:
		_log_message("[color=green]✓ QueryUserInfo() completed![/color]")

		# Update cached friend data with new info
		var epic_account_id = friend_info.get("epic_account_id", "")
		for i in range(cached_friends.size()):
			if cached_friends[i].get("id", "") == epic_account_id:
				# Update only the user info fields, preserving existing data like status
				cached_friends[i]["display_name"] = friend_info.get("display_name", cached_friends[i].get("display_name", ""))
				cached_friends[i]["nickname"] = friend_info.get("nickname", cached_friends[i].get("nickname", ""))
				cached_friends[i]["country"] = friend_info.get("country", cached_friends[i].get("country", ""))
				cached_friends[i]["preferred_language"] = friend_info.get("preferred_language", cached_friends[i].get("preferred_language", ""))
				break

		# Refresh the friends display to show updated names
		_refresh_friends_display()

		# Update display if this is the selected friend
		if not selected_friend_id.is_empty() and epic_account_id == selected_friend_id:
			# Find the updated friend data in cached_friends
			_update_friend_details(epic_account_id)
	else:
		_log_message("[color=red]✗ Friend info query failed![/color]")

func _on_login_status_changed(success: bool, user_info: Dictionary):
	if success:
		_log_message("[color=green]✓ User logged in - friends features available[/color]")
	_update_ui_state()

func _on_logout_status_changed(success: bool):
	if success:
		_log_message("[color=yellow]Logged out - clearing cached friends data[/color]")
		cached_friends.clear()
		selected_friend_id = ""
		_refresh_friends_display()
	_update_ui_state()

func _on_user_cache_updated(success: bool, epic_id: String, user_data: Dictionary):
	_log_message("[color=blue]📦 User cache updated for: " + epic_id + " (success: " + str(success) + ")[/color]")
	
	# Update cached_friends with the new user data (especially Product ID)
	for i in range(cached_friends.size()):
		if cached_friends[i].get("id", "") == epic_id:
			# Update the Product ID and any other cached user info
			if user_data.has("product_user_id"):
				cached_friends[i]["product_user_id"] = user_data["product_user_id"]
				_log_message("[color=blue]Updated Product User ID for friend: " + epic_id + "[/color]")
			break
	
	# If this is the currently selected friend, refresh their details
	if selected_friend_id == epic_id:
		_refresh_selected_friend_details()
	
	# Refresh the friends list in case display names or other info changed
	_refresh_friends_display()

# ============================================================================
# DISPLAY UPDATE FUNCTIONS
# ============================================================================

func _refresh_friends_display():
	friends_list.clear()

	if cached_friends.is_empty():
		friends_list.add_item("No friends found. Click 'Query Friends List' first.")
		return

	for friend in cached_friends:
		var display_text = str(friend.get("id", "Unknown ID")) + " - " + str(friend.get("display_name", "Unknown Friend"))
		var status = friend.get("status", "Unknown")

		# Add status indicator
		if status == "Friends":
			display_text = "� " + display_text
		else:
			display_text = "❓ " + display_text

		friends_list.add_item(display_text)

	_log_message("[color=cyan]Displayed " + str(cached_friends.size()) + " friends[/color]")

func _refresh_selected_friend_details():
	if not selected_friend_id.is_empty():
		_update_friend_details(selected_friend_id)

func _update_friend_details(epic_account_id: String):
	var friend_data = {}
	for friend in cached_friends:
		if friend.get("id", "") == epic_account_id:
			friend_data = friend
			break

	var details_text = "[color=cyan]Friend Details:[/color]\n\n"

	if friend_data.is_empty():
		details_text = "[color=gray]No detailed information available.\nSelect a friend and click 'Query Selected Friend Info'.[/color]"
	else:
		# Display Epic Account ID prominently
		var epic_id = friend_data.get("id", "Unknown")
		details_text += "[color=yellow]Epic Account ID:[/color] [color=white]" + str(epic_id) + "[/color]\n"

		# Display Product User ID prominently
		var product_user_id = friend_data.get("product_user_id", "NULL")
		if product_user_id != "NULL":
			details_text += "[color=yellow]Product User ID:[/color] [color=white]" + str(product_user_id) + "[/color]\n"
		else:
			details_text += "[color=yellow]Product User ID:[/color] [color=gray]" + str(product_user_id) + "[/color]\n"
			_log_message("[color=orange]⚠ Product User ID not available yet. It may take time to be fetched from EOS.[/color]")

		details_text += "\n[color=cyan]Additional Information:[/color]\n"

		# Display all other friend information, excluding id and product_user_id as they're shown above
		for key in friend_data:
			if key != "id" and key != "product_user_id":
				var value = str(friend_data[key])
				details_text += "[color=yellow]" + key + ":[/color] " + value + "\n"

	friend_info_text.text = details_text

# ============================================================================
# UI STATE MANAGEMENT
# ============================================================================

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
	query_friend_info_button.disabled = not friends_available or selected_friend_id.is_empty()
	query_product_id_button.disabled = not friends_available

# ============================================================================
# OUTPUT CONTROL HANDLERS
# ============================================================================

func _on_auto_scroll_toggled(button_pressed: bool):
	output_text.scroll_following = button_pressed

func _on_clear_log_pressed():
	output_text.clear()

# ============================================================================
# UTILITY FUNCTIONS
# ============================================================================

func _log_message(message: String):
	if output_text:
		output_text.append_text(message + "\n")
