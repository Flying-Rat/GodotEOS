extends Control

# Authentication Demo Script
# Demonstrates EpicOS authentication features
# Shows how to initialize the platform and login with different methods

# ============================================================================
# UI REFERENCES
# ============================================================================

@onready var status_label: Label = $VBoxContainer/HBoxContainer/StatusPanel/StatusContainer/StatusTitle
@onready var user_label: Label = $VBoxContainer/HBoxContainer/StatusPanel/StatusContainer/UserLabel
@onready var user_info_label: Label = $VBoxContainer/HBoxContainer/StatusPanel/StatusContainer/UserInfoLabel
@onready var account_info_label: Label = $VBoxContainer/HBoxContainer/StatusPanel/StatusContainer/AccountInfoLabel
@onready var product_user_label: Label = $VBoxContainer/HBoxContainer/StatusPanel/StatusContainer/ProductUserLabel

@onready var init_button: Button = $VBoxContainer/HBoxContainer/VBoxContainer/InitButton
@onready var email_input: LineEdit = $"VBoxContainer/LoginVBoxContainer/EpicAccountContainer/HBoxContainer/VBoxContainer/EmailInput"
@onready var password_input: LineEdit = $"VBoxContainer/LoginVBoxContainer/EpicAccountContainer/HBoxContainer/VBoxContainer/PasswordInput"
@onready var epic_login_button: Button = $"VBoxContainer/LoginVBoxContainer/EpicAccountContainer/HBoxContainer/EpicLoginButton"
@onready var portal_login_button: Button = $"VBoxContainer/LoginVBoxContainer/EpicAccountContainer/HBoxContainer/PortalLoginButton"
@onready var dev_input: LineEdit = $"VBoxContainer/LoginVBoxContainer/EpicAccountContainer/VBoxContainer_LoginSection_LoginGrid_OtherMethodsPanel#OtherMethodsContainer/VBoxContainer_LoginSection_LoginGrid_OtherMethodsPanel_OtherMethodsContainer#DevContainer/DevInput"
@onready var dev_login_button: Button = $"VBoxContainer/LoginVBoxContainer/EpicAccountContainer/VBoxContainer_LoginSection_LoginGrid_OtherMethodsPanel#OtherMethodsContainer/VBoxContainer_LoginSection_LoginGrid_OtherMethodsPanel_OtherMethodsContainer#DevContainer/DevLoginButton"
@onready var logout_button: Button = $VBoxContainer/HBoxContainer/VBoxContainer/LogoutButton

@onready var output_text: RichTextLabel = $VBoxContainer/OutputPanel/VBoxContainer/OutputScrollContainer/OutputText
@onready var auto_scroll_checkbox: CheckButton = $VBoxContainer/OutputPanel/VBoxContainer/OutputHeaderHBox/AutoScrollCheckbox
@onready var clear_log_button: Button = $VBoxContainer/OutputPanel/VBoxContainer/OutputHeaderHBox/ClearLogButton
@onready var back_button: Button = $VBoxContainer/HBoxContainer/VBoxContainer/BackButton

# ============================================================================
# STATE VARIABLES
# ============================================================================

var is_platform_initialized: bool = false
var is_user_logged_in: bool = false

# ============================================================================
# INITIALIZATION
# ============================================================================

func _ready():
	# Connect button signals
	init_button.pressed.connect(_on_init_button_pressed)
	epic_login_button.pressed.connect(_on_epic_login_button_pressed)
	portal_login_button.pressed.connect(_on_portal_login_button_pressed)
	dev_login_button.pressed.connect(_on_dev_login_button_pressed)
	logout_button.pressed.connect(_on_logout_button_pressed)
	back_button.pressed.connect(_on_back_button_pressed)

	# Connect output controls
	auto_scroll_checkbox.toggled.connect(_on_auto_scroll_toggled)
	clear_log_button.pressed.connect(_on_clear_log_pressed)

	# Connect EpicOS signals
	if EpicOS:
		EpicOS.login_completed.connect(_on_login_completed)
		EpicOS.logout_completed.connect(_on_logout_completed)

	# Enable debug mode for detailed logging
	if EpicOS:
		EpicOS.set_debug_mode(true)

	# Update initial UI state
	_update_ui_state()
	_log_message("[color=cyan]═══════════════════════════════════════[/color]")
	_log_message("[color=cyan]Authentication Demo Initialized[/color]")
	_log_message("[color=cyan]═══════════════════════════════════════[/color]")
	_log_message("[color=yellow]📋 INSTRUCTIONS:[/color]")
	_log_message("[color=white]1. Click 'Initialize Platform' to set up EOS[/color]")
	_log_message("[color=white]2. Use login methods: Epic Account, Portal, or Developer[/color]")
	_log_message("[color=white]3. Check user info after successful login[/color]")
	_log_message("")
	_log_message("[color=yellow]Please initialize the platform first, then login[/color]")

# ============================================================================
# BUTTON HANDLERS
# ============================================================================

const CREDENTIALS_PATH := "res://eos_credentials.secrets"

# Credentials are read from a gitignored file so real values never enter the
# repository. Copy eos_credentials.secrets.example, fill it in, and drop the
# ".example" suffix. Keys map 1:1 onto EpicOS.initialize().
func _load_credentials() -> Dictionary:
	var file := ConfigFile.new()
	var err := file.load(CREDENTIALS_PATH)
	if err == ERR_FILE_NOT_FOUND:
		_log_message("[color=red]✗ No credentials found at " + CREDENTIALS_PATH + "[/color]")
		_log_message("[color=yellow]Copy eos_credentials.secrets.example, fill in your values from the[/color]")
		_log_message("[color=yellow]Epic Developer Portal, and remove the .example suffix.[/color]")
		return {}
	if err != OK:
		# Most often an unquoted value - ConfigFile needs product_id="abc", not product_id=abc.
		_log_message("[color=red]✗ " + CREDENTIALS_PATH + " exists but could not be parsed (error " + str(err) + ")[/color]")
		_log_message("[color=yellow]Every value must be wrapped in double quotes, e.g. product_id=\"abc123\".[/color]")
		return {}

	var config := {}
	for key in ["product_name", "product_version", "product_id", "sandbox_id",
			"deployment_id", "client_id", "client_secret", "encryption_key"]:
		var value: Variant = file.get_value("eos", key, "")
		if typeof(value) != TYPE_STRING:
			_log_message("[color=red]✗ " + key + " must be a quoted string in " + CREDENTIALS_PATH + "[/color]")
			return {}
		if not (value as String).is_empty():
			config[key] = value

	var missing: PackedStringArray = []
	for required in ["product_id", "sandbox_id", "deployment_id", "client_id", "client_secret"]:
		if not config.has(required):
			missing.append(required)
	if not missing.is_empty():
		_log_message("[color=red]✗ Missing in " + CREDENTIALS_PATH + ": " + ", ".join(missing) + "[/color]")
		return {}

	return config


func _on_init_button_pressed():
	_log_message("[color=yellow]🔧 InitializePlatform() - Setting up EOS platform...[/color]")

	var config = _load_credentials()
	if config.is_empty():
		return

	if EpicOS:
		is_platform_initialized = EpicOS.initialize(config)

		if is_platform_initialized:
			_log_message("[color=green]✓ InitializePlatform() completed successfully![/color]")
		else:
			_log_message("[color=red]✗ Failed to initialize EOS platform. Check your credentials.[/color]")
	else:
		_log_message("[color=red]✗ EpicOS singleton not available![/color]")

	_update_ui_state()

func _on_epic_login_button_pressed():
	var email = email_input.text.strip_edges()
	var password = password_input.text.strip_edges()

	if email.is_empty() or password.is_empty():
		_log_message("[color=red]✗ Please enter both email and password[/color]")
		return

	_log_message("[color=yellow]🔐 LoginWithEpicAccount() - Attempting login for: " + email + "[/color]")

	if EpicOS:
		EpicOS.login_with_epic_account(email, password)
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

func _on_portal_login_button_pressed():
	_log_message("[color=yellow]🔐 LoginWithAccountPortal() - Opening Epic Games Account Portal...[/color]")

	if EpicOS:
		EpicOS.login_with_account_portal()
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

func _on_dev_login_button_pressed():
	var display_name = dev_input.text.strip_edges()

	if display_name.is_empty():
		display_name = "Developer_User"

	_log_message("[color=yellow]🔐 LoginWithDev() - Attempting developer login with display name: " + display_name + "[/color]")

	if EpicOS:
		EpicOS.login_with_dev(display_name)
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

func _on_logout_button_pressed():
	_log_message("[color=yellow]🚪 Logout() - Logging out...[/color]")

	if EpicOS:
		EpicOS.logout()
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

func _on_back_button_pressed():
	get_tree().change_scene_to_file("res://scenes/demos/demo_menu.tscn")

# ============================================================================
# EPICOS SIGNAL HANDLERS
# ============================================================================

func _on_login_completed(success: bool, user_info: Dictionary):
	if success:
		_log_message("[color=green]✓ Login completed successfully![/color]")
		_log_message("[color=green]User info received: " + str(user_info) + "[/color]")
		is_user_logged_in = true
	else:
		_log_message("[color=red]✗ Login failed![/color]")
		is_user_logged_in = false

	_update_user_info()
	_update_ui_state()

func _on_logout_completed(success: bool):
	if success:
		_log_message("[color=green]✓ Logout completed successfully![/color]")
	else:
		_log_message("[color=red]✗ Logout failed![/color]")

	is_user_logged_in = false
	_update_user_info()
	_update_ui_state()

# ============================================================================
# UI STATE MANAGEMENT
# ============================================================================

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

# ============================================================================
# PROCESSING
# ============================================================================

# Update status periodically
func _on_timer_timeout():
	_update_ui_state()
	_update_user_info()

func _process(_delta):
	# Update UI state periodically (every second)
	if Engine.get_process_frames() % 60 == 0:  # Assuming 60 FPS
		_update_ui_state()
		_update_user_info()
