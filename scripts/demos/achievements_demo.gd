extends Control

# Achievements Demo Script
# Demonstrates EpicOS achievements and statistics features
# Assumes user is already logged in via Authentication Demo

# ============================================================================
# UI REFERENCES
# ============================================================================

@onready var status_label: Label = %StatusLabel

# Quick Actions
@onready var query_definitions_button: Button = %QueryDefinitionsButton
@onready var query_player_achievements_button: Button = %QueryPlayerAchievementsButton
@onready var get_definitions_button: Button = %GetDefinitionsButton
@onready var get_player_achievements_button: Button = %GetPlayerAchievementsButton
@onready var query_stats_button: Button = %QueryStatsButton
@onready var get_stats_button: Button = %GetStatsButton

# Achievement Display
@onready var achievement_cards_container: VBoxContainer = %AchievementCardsContainer

# Manual Controls
@onready var achievement_input: LineEdit = %AchievementIDLineEdit
@onready var unlock_button: Button = %UnlockAchievementButton
@onready var stat_name_input: LineEdit = %StatNameLineEdit
@onready var stat_amount_input: SpinBox = %StatAmountSpinBox
@onready var ingest_stat_button: Button = %IngestStatButton

# Output & Navigation
@onready var output_text: RichTextLabel = %OutputText
@onready var clear_log_button: Button = %ClearLogButton
@onready var auto_scroll_toggle: CheckButton = %AutoScrollCheckbox
@onready var back_button: Button = %BackButton

# ============================================================================
# STATE VARIABLES
# ============================================================================

var cached_definitions: Array = []
var cached_player_achievements: Array = []
var cached_stats: Array = []

var auto_scroll_enabled: bool = true

# ============================================================================
# INITIALIZATION
# ============================================================================

func _ready():
	auto_scroll_enabled = auto_scroll_toggle.button_pressed
	output_text.scroll_following = auto_scroll_enabled

	_setup_signal_connections()
	_update_ui_state()
	_set_featured_stat("", 0)

	_log_message("[color=cyan]═══════════════════════════════════════[/color]")
	_log_message("[color=cyan]Achievements Demo Initialized[/color]")
	_log_message("[color=cyan]═══════════════════════════════════════[/color]")

	if EpicOS and EpicOS.is_user_logged_in():
		_log_message("[color=yellow]Please use the buttons above to test achievement functions[/color]")
	else:
		_log_message("[color=yellow]Please login first (use Authentication Demo)[/color]")

func _setup_signal_connections():
	# New Function Buttons
	if query_definitions_button:
		query_definitions_button.pressed.connect(_on_query_definitions_button_pressed)
	if query_player_achievements_button:
		query_player_achievements_button.pressed.connect(_on_query_player_achievements_button_pressed)
	if get_definitions_button:
		get_definitions_button.pressed.connect(_on_get_definitions_button_pressed)
	if get_player_achievements_button:
		get_player_achievements_button.pressed.connect(_on_get_player_achievements_button_pressed)
	if query_stats_button:
		query_stats_button.pressed.connect(_on_query_stats_button_pressed)
	if get_stats_button:
		get_stats_button.pressed.connect(_on_get_stats_button_pressed)

	# Manual controls
	if unlock_button:
		unlock_button.pressed.connect(_trigger_unlock_from_input)
	if ingest_stat_button:
		ingest_stat_button.pressed.connect(_on_ingest_stat_button_pressed)

	# Output controls
	if clear_log_button:
		clear_log_button.pressed.connect(_on_clear_log_pressed)
	if auto_scroll_toggle:
		auto_scroll_toggle.toggled.connect(_on_auto_scroll_toggled)
	if back_button:
		back_button.pressed.connect(_on_back_button_pressed)

	if stat_name_input:
		stat_name_input.text_submitted.connect(func(_text): _update_featured_stat_display_from_cache())

	if EpicOS:
		EpicOS.achievement_definitions_completed.connect(_on_achievement_definitions_completed)
		EpicOS.player_achievements_completed.connect(_on_player_achievements_completed)
		EpicOS.achievements_unlocked_completed.connect(_on_achievements_unlocked_completed)
		# Connect achievement_unlocked signal if it exists
		if EpicOS.has_signal("achievement_unlocked"):
			EpicOS.achievement_unlocked.connect(_on_achievement_unlocked)
		EpicOS.achievement_stats_completed.connect(_on_achievement_stats_completed)
		EpicOS.login_completed.connect(_on_login_status_changed)
		EpicOS.logout_completed.connect(_on_logout_status_changed)# ============================================================================
# QUICK ACTION HELPERS
# ============================================================================

func _trigger_unlock_from_input():
	var achievement_id := achievement_input.text.strip_edges()
	_unlock_achievement(achievement_id)

func _unlock_achievement(achievement_id: String):
	if achievement_id.is_empty():
		_log_message("[color=red]✗ Please enter an achievement ID[/color]")
		return

	_log_message("[color=yellow]Attempting to unlock: " + achievement_id + "[/color]")
	if EpicOS:
		EpicOS.unlock_achievement(achievement_id)
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

func _on_ingest_stat_button_pressed():
	var stat_name := stat_name_input.text.strip_edges()
	var amount := int(stat_amount_input.value)
	_ingest_stat(stat_name, amount)

func _ingest_stat(stat_name: String, amount: int):
	if stat_name.is_empty():
		_log_message("[color=red]✗ Please enter a stat name[/color]")
		return

	_log_message("[color=yellow]Updating stat: " + stat_name + " +" + str(amount) + "[/color]")
	if EpicOS:
		EpicOS.ingest_achievement_stat(stat_name, amount)
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

# ============================================================================
# NEW FUNCTION BUTTON HANDLERS
# ============================================================================

func _on_query_definitions_button_pressed():
	_log_message("[color=yellow]🔍 QueryAchievementDefinitions() - Fetching achievement definitions from EOS...[/color]")
	if EpicOS:
		EpicOS.query_achievement_definitions()
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

func _on_query_player_achievements_button_pressed():
	_log_message("[color=yellow]🔍 QueryPlayerAchievements() - Fetching player achievement progress from EOS...[/color]")
	if EpicOS:
		EpicOS.query_player_achievements()
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

func _on_get_definitions_button_pressed():
	_log_message("[color=yellow]📚 GetAchievementDefinitions() - Getting cached achievement definitions...[/color]")
	if EpicOS:
		var definitions = EpicOS.get_achievement_definitions()
		_log_message("[color=green]✓ Retrieved " + str(definitions.size()) + " cached achievement definitions[/color]")
		
		# Print detailed information for each achievement definition
		if definitions.size() > 0:
			_log_message("[color=cyan]📋 Achievement Definitions Details (from cache):[/color]")
			for i in range(definitions.size()):
				var def = definitions[i]
				_log_message("[color=white]─── Achievement " + str(i + 1) + " ──────────────────────────────────[/color]")
				_log_message("[color=light_blue]achievement_id:[/color] " + str(def.get("achievement_id", "N/A")))
				_log_message("[color=light_blue]unlocked_display_name:[/color] " + str(def.get("unlocked_display_name", "N/A")))
				_log_message("[color=light_blue]unlocked_description:[/color] " + str(def.get("unlocked_description", "N/A")))
				_log_message("[color=light_blue]locked_display_name:[/color] " + str(def.get("locked_display_name", "N/A")))
				_log_message("[color=light_blue]locked_description:[/color] " + str(def.get("locked_description", "N/A")))
				_log_message("[color=light_blue]flavor_text:[/color] " + str(def.get("flavor_text", "N/A")))
				_log_message("[color=light_blue]unlocked_icon_url:[/color] " + str(def.get("unlocked_icon_url", "N/A")))
				_log_message("[color=light_blue]locked_icon_url:[/color] " + str(def.get("locked_icon_url", "N/A")))
				_log_message("[color=light_blue]is_hidden:[/color] " + str(def.get("is_hidden", "N/A")))
				_log_message("")  # Empty line for readability
		else:
			_log_message("[color=yellow]  No cached achievement definitions available[/color]")
			_log_message("[color=gray]  Try 'Query Definitions' first to fetch from EOS[/color]")
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

func _on_get_player_achievements_button_pressed():
	_log_message("[color=yellow]🏆 GetPlayerAchievements() - Getting cached player achievement progress...[/color]")
	if EpicOS:
		var achievements = EpicOS.get_player_achievements()
		_log_message("[color=green]✓ Retrieved " + str(achievements.size()) + " cached player achievements[/color]")
		
		# Print detailed information for each player achievement
		if achievements.size() > 0:
			_log_message("[color=cyan]🏆 Player Achievements Details (from cache):[/color]")
			for i in range(achievements.size()):
				var achievement = achievements[i]
				_log_message("[color=white]─── Player Achievement " + str(i + 1) + " ──────────────────────[/color]")
				_log_message("[color=light_green]achievement_id:[/color] " + str(achievement.get("achievement_id", "N/A")))
				_log_message("[color=light_green]progress:[/color] " + str(achievement.get("progress", "N/A")))
				_log_message("[color=light_green]unlock_time:[/color] " + str(achievement.get("unlock_time", "N/A")))
				_log_message("[color=light_green]is_unlocked:[/color] " + str(achievement.get("is_unlocked", "N/A")))
				_log_message("[color=light_green]display_name:[/color] " + str(achievement.get("display_name", "N/A")))
				_log_message("[color=light_green]description:[/color] " + str(achievement.get("description", "N/A")))
				_log_message("[color=light_green]icon_url:[/color] " + str(achievement.get("icon_url", "N/A")))
				_log_message("[color=light_green]flavor_text:[/color] " + str(achievement.get("flavor_text", "N/A")))
				_log_message("")  # Empty line for readability
		else:
			_log_message("[color=yellow]  No cached player achievements available[/color]")
			_log_message("[color=gray]  Try 'Query Player Achievements' first to fetch from EOS[/color]")
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

func _on_query_stats_button_pressed():
	_log_message("[color=yellow]📊 QueryStats() - Fetching player statistics from EOS...[/color]")
	if EpicOS:
		EpicOS.query_achievement_stats()
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

func _on_get_stats_button_pressed():
	_log_message("[color=yellow]📊 GetStats() - Getting cached player statistics...[/color]")
	if EpicOS:
		var stats = EpicOS.get_stats()
		_log_message("[color=green]✓ Retrieved " + str(stats.size()) + " cached statistics[/color]")
		
		# Print detailed information for each stat
		if stats.size() > 0:
			_log_message("[color=cyan]📊 Statistics Details (from cache):[/color]")
			_log_message("[color=gray]Note: EpicOS only returns stats that have been initialized/touched[/color]")
			for i in range(stats.size()):
				var stat = stats[i]
				_log_message("[color=white]─── Statistic " + str(i + 1) + " ───────────────────────────────────[/color]")
				_log_message("[color=yellow]name:[/color] " + str(stat.get("name", "N/A")))
				_log_message("[color=yellow]value:[/color] " + str(stat.get("value", "N/A")))
				_log_message("[color=yellow]start_time:[/color] " + str(stat.get("start_time", "N/A")))
				_log_message("[color=yellow]end_time:[/color] " + str(stat.get("end_time", "N/A")))
				_log_message("")  # Empty line for readability
		else:
			_log_message("[color=yellow]  No cached statistics available[/color]")
			_log_message("[color=gray]  Try 'Query Stats' first to fetch from EOS[/color]")
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

# ============================================================================
# EPICOS SIGNAL HANDLERS
# ============================================================================

func _on_achievement_definitions_completed(success: bool, definitions: Array):
	if success:
		cached_definitions = definitions
		_refresh_definitions_display()
		_refresh_achievements_display()
		_log_message("[color=green]✓ Loaded " + str(definitions.size()) + " achievement definitions[/color]")
		
		# Print detailed information for each achievement definition
		if definitions.size() > 0:
			_log_message("[color=cyan]📋 Achievement Definitions Details:[/color]")
			for i in range(definitions.size()):
				var def = definitions[i]
				_log_message("[color=white]─── Achievement " + str(i + 1) + " ──────────────────────────────────[/color]")
				_log_message("[color=light_blue]achievement_id:[/color] " + str(def.get("achievement_id", "N/A")))
				_log_message("[color=light_blue]unlocked_display_name:[/color] " + str(def.get("unlocked_display_name", "N/A")))
				_log_message("[color=light_blue]unlocked_description:[/color] " + str(def.get("unlocked_description", "N/A")))
				_log_message("[color=light_blue]locked_display_name:[/color] " + str(def.get("locked_display_name", "N/A")))
				_log_message("[color=light_blue]locked_description:[/color] " + str(def.get("locked_description", "N/A")))
				_log_message("[color=light_blue]flavor_text:[/color] " + str(def.get("flavor_text", "N/A")))
				_log_message("[color=light_blue]unlocked_icon_url:[/color] " + str(def.get("unlocked_icon_url", "N/A")))
				_log_message("[color=light_blue]locked_icon_url:[/color] " + str(def.get("locked_icon_url", "N/A")))
				_log_message("[color=light_blue]is_hidden:[/color] " + str(def.get("is_hidden", "N/A")))
				_log_message("")  # Empty line for readability
	else:
		_log_message("[color=red]✗ Failed to load achievement definitions[/color]")

func _on_player_achievements_completed(success: bool, achievements: Array):
	if success:
		cached_player_achievements = achievements
		_refresh_player_display()
		_refresh_achievements_display()
		_log_message("[color=green]✓ Loaded " + str(achievements.size()) + " player achievements[/color]")
		
		# Print detailed information for each player achievement
		if achievements.size() > 0:
			_log_message("[color=cyan]🏆 Player Achievements Details:[/color]")
			for i in range(achievements.size()):
				var achievement = achievements[i]
				_log_message("[color=white]─── Player Achievement " + str(i + 1) + " ──────────────────────[/color]")
				_log_message("[color=light_green]achievement_id:[/color] " + str(achievement.get("achievement_id", "N/A")))
				_log_message("[color=light_green]progress:[/color] " + str(achievement.get("progress", "N/A")))
				_log_message("[color=light_green]unlock_time:[/color] " + str(achievement.get("unlock_time", "N/A")))
				_log_message("[color=light_green]is_unlocked:[/color] " + str(achievement.get("is_unlocked", "N/A")))
				_log_message("[color=light_green]display_name:[/color] " + str(achievement.get("display_name", "N/A")))
				_log_message("[color=light_green]description:[/color] " + str(achievement.get("description", "N/A")))
				_log_message("[color=light_green]icon_url:[/color] " + str(achievement.get("icon_url", "N/A")))
				_log_message("[color=light_green]flavor_text:[/color] " + str(achievement.get("flavor_text", "N/A")))
				_log_message("")  # Empty line for readability
	else:
		_log_message("[color=red]✗ Failed to load player achievements[/color]")

func _on_achievements_unlocked_completed(success: bool, unlocked_achievement_ids: Array):
	if success:
		_log_message("[color=green]✓ Successfully processed achievement unlock request[/color]")
		if EpicOS:
			EpicOS.query_player_achievements()
	else:
		_log_message("[color=red]✗ Failed to unlock achievements[/color]")

func _on_achievement_unlocked(achievement_id: String, unlock_time: int):
	_log_message("[color=gold]═══════════════════════════════════════[/color]")
	_log_message("[color=gold]🎉 ACHIEVEMENT UNLOCKED! 🎉[/color]")
	_log_message("[color=gold]═══════════════════════════════════════[/color]")

	var title := achievement_id
	var description := ""

	if EpicOS:
		var definition: Dictionary = EpicOS.get_achievement_definition(achievement_id)
		if not definition.is_empty():
			title = definition.get("unlocked_display_name", achievement_id)
			description = definition.get("unlocked_description", "")
			
			# Show all definition fields for the unlocked achievement
			_log_message("[color=cyan]📋 Achievement Definition Details:[/color]")
			_log_message("[color=light_blue]achievement_id:[/color] " + str(definition.get("achievement_id", "N/A")))
			_log_message("[color=light_blue]unlocked_display_name:[/color] " + str(definition.get("unlocked_display_name", "N/A")))
			_log_message("[color=light_blue]unlocked_description:[/color] " + str(definition.get("unlocked_description", "N/A")))
			_log_message("[color=light_blue]locked_display_name:[/color] " + str(definition.get("locked_display_name", "N/A")))
			_log_message("[color=light_blue]locked_description:[/color] " + str(definition.get("locked_description", "N/A")))
			_log_message("[color=light_blue]flavor_text:[/color] " + str(definition.get("flavor_text", "N/A")))
			_log_message("[color=light_blue]unlocked_icon_url:[/color] " + str(definition.get("unlocked_icon_url", "N/A")))
			_log_message("[color=light_blue]locked_icon_url:[/color] " + str(definition.get("locked_icon_url", "N/A")))
			_log_message("[color=light_blue]is_hidden:[/color] " + str(definition.get("is_hidden", "N/A")))
			_log_message("")

	_log_message("[color=yellow]🎉 " + title + "[/color]")
	if not description.is_empty():
		_log_message("[color=white]" + description + "[/color]")
	_log_message("[color=cyan]Achievement ID: " + achievement_id + "[/color]")
	_log_message("[color=cyan]Unlocked at: " + Time.get_datetime_string_from_unix_time(unlock_time) + "[/color]")
	_log_message("[color=gold]═══════════════════════════════════════[/color]")
	
	# Refresh achievement display to show the newly unlocked achievement
	if EpicOS:
		EpicOS.query_player_achievements()

func _on_achievement_stats_completed(success: bool, stats: Array):
	if success:
		cached_stats = stats
		_refresh_stats_display()
		_log_message("[color=green]✓ Loaded " + str(stats.size()) + " achievement stats[/color]")
		
		# Print detailed information for each stat
		if stats.size() > 0:
			_log_message("[color=cyan]📊 Achievement Statistics Details:[/color]")
			_log_message("[color=gray]Note: EpicOS only returns stats that have been initialized/touched[/color]")
			for i in range(stats.size()):
				var stat = stats[i]
				_log_message("[color=white]─── Statistic " + str(i + 1) + " ───────────────────────────────────[/color]")
				_log_message("[color=yellow]name:[/color] " + str(stat.get("name", "N/A")))
				_log_message("[color=yellow]value:[/color] " + str(stat.get("value", "N/A")))
				_log_message("[color=yellow]start_time:[/color] " + str(stat.get("start_time", "N/A")))
				_log_message("[color=yellow]end_time:[/color] " + str(stat.get("end_time", "N/A")))
				_log_message("")  # Empty line for readability
		else:
			_log_message("[color=yellow]  No achievement stats found (no stats have been initialized yet)[/color]")
			_log_message("[color=gray]  Try using 'Increment Stat' in Manual Testing to initialize some stats[/color]")
	else:
		_log_message("[color=red]✗ Failed to load achievement stats[/color]")

func _on_login_status_changed(success: bool, user_info: Dictionary):
	if success:
		var username: String = user_info.get("display_name", "Unknown")
		_log_message("[color=green]✓ Logged in as " + username + "[/color]")
		_update_ui_state()

func _on_logout_status_changed(success: bool):
	if success:
		_log_message("[color=yellow]Logged out - clearing cached data[/color]")
		cached_definitions.clear()
		cached_player_achievements.clear()
		cached_stats.clear()
		_refresh_all_displays()
	_update_ui_state()

# ============================================================================
# DISPLAY UPDATE FUNCTIONS
# ============================================================================

func _refresh_all_displays():
	_refresh_achievements_display()

func _refresh_definitions_display():
	# UI element not present in current scene - function kept for potential future use
	pass

func _refresh_player_display():
	# UI element not present in current scene - function kept for potential future use
	pass

func _refresh_stats_display():
	# UI element not present in current scene - function kept for potential future use
	pass

func _refresh_achievements_display():
	# Clear existing cards
	for child in achievement_cards_container.get_children():
		child.queue_free()
	
	# Wait a frame for the queued deletions to complete
	await get_tree().process_frame
	
	# If we don't have both definitions and player achievements, show a placeholder
	if cached_definitions.is_empty() or cached_player_achievements.is_empty():
		_create_placeholder_card()
		return
	
	# Create achievement cards by merging definition and player data
	for definition in cached_definitions:
		var achievement_id := str(definition.get("achievement_id", ""))
		var player_data = _find_player_achievement(achievement_id)
		_create_achievement_card(definition, player_data)

func _find_player_achievement(achievement_id: String) -> Dictionary:
	"""Find player achievement data for a given achievement ID"""
	for achievement in cached_player_achievements:
		if str(achievement.get("achievement_id", "")) == achievement_id:
			return achievement
	return {}

func _find_achievement_definition(achievement_id: String) -> Dictionary:
	"""Find achievement definition data for a given achievement ID"""
	for definition in cached_definitions:
		if str(definition.get("achievement_id", "")) == achievement_id:
			return definition
	return {}

func _create_placeholder_card():
	"""Create a placeholder item when no achievement data is available"""
	var item = PanelContainer.new()
	item.custom_minimum_size = Vector2(0, 40)  # Match simplified height
	item.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	
	var margin = MarginContainer.new()
	margin.add_theme_constant_override("margin_left", 8)
	margin.add_theme_constant_override("margin_right", 8)
	margin.add_theme_constant_override("margin_top", 4)
	margin.add_theme_constant_override("margin_bottom", 4)
	item.add_child(margin)
	
	var label = Label.new()
	label.text = "Loading achievements... Please use the buttons above to fetch data"
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	label.add_theme_font_size_override("font_size", 14)
	label.modulate = Color.GRAY
	margin.add_child(label)
	
	achievement_cards_container.add_child(item)

func _create_achievement_card(definition: Dictionary, player_data: Dictionary):
	"""Create a simple achievement item"""
	var achievement_id := str(definition.get("achievement_id", "Unknown"))
	var display_name := str(definition.get("display_name", achievement_id))
	
	# Player data - simplified state values (no percentages)
	var is_unlocked: bool = player_data.get("is_unlocked", false)
	
	var state_text: String = "Unlocked" if is_unlocked else "Locked"
	
	# Create simple item container
	var item = PanelContainer.new()
	item.custom_minimum_size = Vector2(0, 40)  # Much smaller height
	item.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	
	# Add minimal margin
	var margin = MarginContainer.new()
	margin.add_theme_constant_override("margin_left", 8)
	margin.add_theme_constant_override("margin_right", 8)
	margin.add_theme_constant_override("margin_top", 4)
	margin.add_theme_constant_override("margin_bottom", 4)
	item.add_child(margin)
	
	# Simple horizontal layout
	var hbox = HBoxContainer.new()
	hbox.add_theme_constant_override("separation", 10)
	margin.add_child(hbox)
	
	# Achievement name
	var name_label = Label.new()
	name_label.text = display_name
	name_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	name_label.add_theme_font_size_override("font_size", 14)
	if is_unlocked:
		name_label.modulate = Color.WHITE
	else:
		name_label.modulate = Color.LIGHT_GRAY
	hbox.add_child(name_label)
	
	# State display (no percentages)
	var state_label = Label.new()
	state_label.text = state_text
	state_label.add_theme_font_size_override("font_size", 12)
	if is_unlocked:
		state_label.modulate = Color.GREEN
	else:
		state_label.modulate = Color.YELLOW
	hbox.add_child(state_label)
	
	# Add the item to the container
	achievement_cards_container.add_child(item)

func _update_featured_stat_display_from_cache():
	# UI elements not present in current scene - function kept for potential future use
	pass

func _set_featured_stat(stat_name: String, value: int):
	# UI elements not present in current scene - function kept for potential future use
	pass

# ============================================================================
# UI STATE MANAGEMENT
# ============================================================================

func _update_ui_state():
	var is_logged_in := false
	var platform_initialized := false
	var username := "Unknown"

	if EpicOS:
		is_logged_in = EpicOS.is_user_logged_in()
		platform_initialized = EpicOS.is_platform_initialized()
		if is_logged_in:
			username = EpicOS.get_current_username()

	if status_label:
		if not platform_initialized:
			status_label.text = "Status: Platform not initialized"
		elif not is_logged_in:
			status_label.text = "Status: Please login first (use Authentication Demo)"
		else:
			status_label.text = "Status: Logged in as " + username + " - Ready!"

	var controls_enabled := platform_initialized and is_logged_in
	var buttons_to_disable = [
		query_definitions_button,
		query_player_achievements_button,
		get_definitions_button,
		get_player_achievements_button,
		query_stats_button,
		get_stats_button,
		unlock_button,
		ingest_stat_button
	]
	
	for button in buttons_to_disable:
		if button:
			button.disabled = not controls_enabled

	if achievement_input:
		achievement_input.editable = controls_enabled
	if stat_name_input:
		stat_name_input.editable = controls_enabled
	if stat_amount_input:
		stat_amount_input.editable = controls_enabled

# ============================================================================
# UTILITY FUNCTIONS
# ============================================================================

func _log_message(message: String):
	if output_text:
		output_text.scroll_following = auto_scroll_enabled
		output_text.append_text(message + "\n")
		if auto_scroll_enabled:
			var line_count: int = max(output_text.get_line_count() - 1, 0)
			output_text.scroll_to_line(line_count)

func _on_clear_log_pressed():
	if output_text:
		output_text.clear()
	_log_message("[color=cyan]Log cleared.[/color]")

func _on_auto_scroll_toggled(button_pressed: bool):
	auto_scroll_enabled = button_pressed
	if output_text:
		output_text.scroll_following = auto_scroll_enabled

func _on_back_button_pressed():
	get_tree().change_scene_to_file("res://scenes/demos/demo_menu.tscn")
