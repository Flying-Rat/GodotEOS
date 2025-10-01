extends Control

# Authentication Demo Script
# Demonstrates how to use the EpicOS authentication features

@onready var status_label: Label = $VBoxContainer/StatusPanel/StatusContainer/StatusLabel
@onready var user_label: Label = $VBoxContainer/StatusPanel/StatusContainer/UserLabel
@onready var user_info_label: Label = $VBoxContainer/StatusPanel/StatusContainer/UserInfoLabel
@onready var account_info_label: Label = $VBoxContainer/StatusPanel/StatusContainer/AccountInfoLabel
@onready var product_user_label: Label = $VBoxContainer/StatusPanel/StatusContainer/ProductUserLabel

@onready var init_button: Button = $VBoxContainer/InitSection/InitButton
@onready var email_input: LineEdit = $VBoxContainer/LoginSection/LoginGrid/EpicAccountPanel/EpicAccountContainer/EmailInput
@onready var password_input: LineEdit = $VBoxContainer/LoginSection/LoginGrid/EpicAccountPanel/EpicAccountContainer/PasswordInput
@onready var epic_login_button: Button = $VBoxContainer/LoginSection/LoginGrid/EpicAccountPanel/EpicAccountContainer/EpicLoginButton
@onready var portal_login_button: Button = $VBoxContainer/LoginSection/LoginGrid/OtherMethodsPanel/OtherMethodsContainer/PortalLoginButton
@onready var device_input: LineEdit = $VBoxContainer/LoginSection/LoginGrid/OtherMethodsPanel/OtherMethodsContainer/DeviceContainer/DeviceInput
@onready var device_login_button: Button = $VBoxContainer/LoginSection/LoginGrid/OtherMethodsPanel/OtherMethodsContainer/DeviceContainer/DeviceLoginButton
@onready var dev_input: LineEdit = $VBoxContainer/LoginSection/LoginGrid/OtherMethodsPanel/OtherMethodsContainer/DevContainer/DevInput
@onready var dev_login_button: Button = $VBoxContainer/LoginSection/LoginGrid/OtherMethodsPanel/OtherMethodsContainer/DevContainer/DevLoginButton
@onready var logout_button: Button = $VBoxContainer/LoginSection/LogoutButton

@onready var output_text: RichTextLabel = $VBoxContainer/OutputSection/ScrollContainer/OutputText
@onready var back_button: Button = $VBoxContainer/BackButton

var is_platform_initialized: bool = false
var is_user_logged_in: bool = false

func _ready():
	# Connect button signals
	init_button.pressed.connect(_on_init_button_pressed)
	epic_login_button.pressed.connect(_on_epic_login_button_pressed)
	portal_login_button.pressed.connect(_on_portal_login_button_pressed)
	device_login_button.pressed.connect(_on_device_login_button_pressed)
	dev_login_button.pressed.connect(_on_dev_login_button_pressed)
	logout_button.pressed.connect(_on_logout_button_pressed)
	back_button.pressed.connect(_on_back_button_pressed)

	# Connect EpicOS signals
	if EpicOS:
		EpicOS.login_completed.connect(_on_login_completed)
		EpicOS.logout_completed.connect(_on_logout_completed)

	# Enable debug mode for detailed logging
	if EpicOS:
		EpicOS.set_debug_mode(true)

	# Update initial UI state
	_update_ui_state()
	_log_message("[color=cyan]Authentication Demo initialized. Ready to demonstrate EpicOS authentication.[/color]")

func _on_init_button_pressed():
	_log_message("[color=yellow]Initializing EOS platform...[/color]")

	# Example configuration - replace with your actual Epic Games credentials
	var config = {
		"product_name": "GodotEpic Demo",
		"product_version": "1.0.0",
		"product_id": "your_product_id_here",  # Get from Epic Developer Portal
		"sandbox_id": "your_sandbox_id_here",  # Get from Epic Developer Portal
		"deployment_id": "your_deployment_id_here",  # Get from Epic Developer Portal
		"client_id": "your_client_id_here",  # Get from Epic Developer Portal
		"client_secret": "your_client_secret_here"  # Get from Epic Developer Portal
	}

	if EpicOS:
		is_platform_initialized = EpicOS.initialize(config)

		if is_platform_initialized:
			_log_message("[color=green]✓ EOS platform initialized successfully![/color]")
		else:
			_log_message("[color=red]✗ Failed to initialize EOS platform. Check your credentials.[/color]")
	else:
		_log_message("[color=red]✗ EpicOS singleton not available![/color]")

	_update_ui_state()

func _on_epic_login_button_pressed():
	var email = email_input.text.strip_edges()
	var password = password_input.text.strip_edges()

	if email.is_empty() or password.is_empty():
		_log_message("[color=red]Please enter both email and password.[/color]")
		return

	_log_message("[color=yellow]Attempting Epic Account login for: " + email + "[/color]")

	if EpicOS:
		EpicOS.login_with_epic_account(email, password)
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_portal_login_button_pressed():
	_log_message("[color=yellow]Opening Epic Games Account Portal for login...[/color]")

	if EpicOS:
		EpicOS.login_with_account_portal()
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_device_login_button_pressed():
	var display_name = device_input.text.strip_edges()

	if display_name.is_empty():
		display_name = "GodotEpic_User"

	_log_message("[color=yellow]Attempting Device ID login with display name: " + display_name + "[/color]")

	if EpicOS:
		EpicOS.login_with_device_id(display_name)
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_dev_login_button_pressed():
	var display_name = dev_input.text.strip_edges()

	if display_name.is_empty():
		display_name = "Developer_User"

	_log_message("[color=yellow]Attempting Developer login with display name: " + display_name + "[/color]")

	if EpicOS:
		EpicOS.login_with_dev(display_name)
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_logout_button_pressed():
	_log_message("[color=yellow]Logging out...[/color]")

	if EpicOS:
		EpicOS.logout()
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_back_button_pressed():
	# Navigate back to demo menu
	get_tree().change_scene_to_file("res://scenes/demos/demo_menu.tscn")

func _on_login_completed(success: bool, user_info: Dictionary):
	if success:
		_log_message("[color=green]✓ Login successful![/color]")
		_log_message("[color=green]User info received: " + str(user_info) + "[/color]")
		is_user_logged_in = true
	else:
		_log_message("[color=red]✗ Login failed![/color]")
		is_user_logged_in = false

	_update_user_info()
	_update_ui_state()

func _on_logout_completed(success: bool):
	if success:
		_log_message("[color=green]✓ Logout successful![/color]")
	else:
		_log_message("[color=red]✗ Logout failed![/color]")

	is_user_logged_in = false
	_update_user_info()
	_update_ui_state()

func _update_ui_state():
	# Update platform status
	if EpicOS and EpicOS.is_platform_initialized():
		status_label.text = "Platform Status: Initialized"
		is_platform_initialized = true
	else:
		status_label.text = "Platform Status: Not Initialized"
		is_platform_initialized = false

	# Update user status
	if EpicOS and EpicOS.is_user_logged_in():
		user_label.text = "User Status: Logged In"
		is_user_logged_in = true
	else:
		user_label.text = "User Status: Not Logged In"
		is_user_logged_in = false

	# Enable/disable buttons based on state
	init_button.disabled = is_platform_initialized
	epic_login_button.disabled = not is_platform_initialized or is_user_logged_in
	portal_login_button.disabled = not is_platform_initialized or is_user_logged_in
	device_login_button.disabled = not is_platform_initialized or is_user_logged_in
	dev_login_button.disabled = not is_platform_initialized or is_user_logged_in
	logout_button.disabled = not is_user_logged_in

func _update_user_info():
	if EpicOS and is_user_logged_in:
		var username = EpicOS.get_current_username()
		var epic_account_id = EpicOS.get_epic_account_id()
		var product_user_id = EpicOS.get_product_user_id()

		user_info_label.text = "Username: " + (username if not username.is_empty() else "N/A")
		account_info_label.text = "Epic Account ID: " + (epic_account_id if not epic_account_id.is_empty() else "N/A")
		product_user_label.text = "Product User ID: " + (product_user_id if not product_user_id.is_empty() else "N/A")
	else:
		user_info_label.text = "Username: N/A"
		account_info_label.text = "Epic Account ID: N/A"
		product_user_label.text = "Product User ID: N/A"

func _log_message(message: String):
	if output_text:
		output_text.append_text(message + "\n")

# Update status periodically
func _on_timer_timeout():
	_update_ui_state()
	_update_user_info()

func _process(_delta):
	# Update UI state periodically (every second)
	if Engine.get_process_frames() % 60 == 0:  # Assuming 60 FPS
		_update_ui_state()
		_update_user_info()