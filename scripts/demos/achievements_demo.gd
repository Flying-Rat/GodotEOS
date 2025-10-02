extends Control

# Achievements Demo Script
# Demonstrates EpicOS achievements and statistics features
# Assumes user is already logged in via Authentication Demo

# ============================================================================
# UI REFERENCES
# ============================================================================

@onready var status_label: Label = %StatusLabel

# Quick Actions
@onready var quick_increment_ten_button: Button = %QueryAllAchievementsButton
@onready var quick_refresh_all_button: Button = %QueryAllStatsButton

# Achievement Display
@onready var achievement_cards_container: VBoxContainer = %AchievementCardsContainer
@onready var definitions_list: ItemList # Not available in current scene
@onready var player_list: ItemList # Not available in current scene
@onready var refresh_achievements_button: Button = %RefreshAchievementsButton

# Statistics Display (removed from UI but keeping references for potential future use)
@onready var stats_list: ItemList # Removed from scene
@onready var featured_stat_name_label: Label # Removed from scene
@onready var featured_stat_value_label: Label # Removed from scene
@onready var featured_increment_one_button: Button # Removed from scene
@onready var featured_increment_five_button: Button # Removed from scene
@onready var featured_increment_ten_button: Button # Removed from scene
@onready var refresh_stats_button: Button # Removed from scene

# Manual Controls
@onready var achievement_input: LineEdit = %AchievementIDLineEdit
@onready var unlock_button: Button = %UnlockAchievementButton
@onready var stat_name_input: LineEdit = %StatNameLineEdit
@onready var stat_amount_input: SpinBox = %StatAmountSpinBox
@onready var ingest_stat_button: Button = %IngestStatButton
@onready var query_definitions_button: Button = %QueryDefinitionsButton
@onready var query_player_button: Button = %QueryPlayerButton
@onready var query_stats_button: Button # Not available in current scene

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
		_log_message("[color=yellow]Auto-loading achievement data...[/color]")
		_on_refresh_all_pressed()
	else:
		_log_message("[color=yellow]Please login first (use Authentication Demo)[/color]")

func _setup_signal_connections():
	# Quick action buttons in the scene are mapped differently
	if quick_increment_ten_button:
		quick_increment_ten_button.pressed.connect(_on_refresh_all_pressed)
	if quick_refresh_all_button:
		quick_refresh_all_button.pressed.connect(_on_query_stats_pressed)

	# Featured stat buttons
	if featured_increment_one_button:
		featured_increment_one_button.pressed.connect(func(): _increment_stat_from_input(1))
	if featured_increment_five_button:
		featured_increment_five_button.pressed.connect(func(): _increment_stat_from_input(5))
	if featured_increment_ten_button:
		featured_increment_ten_button.pressed.connect(func(): _increment_stat_from_input(10))

	# Refresh button
	if refresh_achievements_button:
		refresh_achievements_button.pressed.connect(func():
			_log_message("[color=yellow]Refreshing achievement details...[/color]")
			_on_query_definitions_pressed()
			_on_query_player_pressed()
		)
	
	# Refresh stats button
	if refresh_stats_button:
		refresh_stats_button.pressed.connect(_on_query_stats_pressed)

	# Manual controls
	if unlock_button:
		unlock_button.pressed.connect(_trigger_unlock_from_input)
	if ingest_stat_button:
		ingest_stat_button.pressed.connect(_on_ingest_stat_button_pressed)

	if query_definitions_button:
		query_definitions_button.pressed.connect(_on_query_definitions_pressed)
	if query_player_button:
		query_player_button.pressed.connect(_on_query_player_pressed)
	if query_stats_button:
		query_stats_button.pressed.connect(_on_query_stats_pressed)

	# Output controls
	if clear_log_button:
		clear_log_button.pressed.connect(_on_clear_log_pressed)
	if auto_scroll_toggle:
		auto_scroll_toggle.toggled.connect(_on_auto_scroll_toggled)
	if back_button:
		back_button.pressed.connect(_on_back_button_pressed)

	# Lists (only if they exist)
	if definitions_list:
		definitions_list.item_selected.connect(_on_definitions_list_item_selected)
	if player_list:
		player_list.item_selected.connect(_on_player_list_item_selected)
	if stats_list:
		stats_list.item_selected.connect(_on_stats_list_item_selected)

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
		EpicOS.logout_completed.connect(_on_logout_status_changed)

# ============================================================================
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

func _increment_stat_from_input(amount: int):
	var stat_name := stat_name_input.text.strip_edges()
	stat_amount_input.value = amount
	_ingest_stat(stat_name, amount)

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
# DATA QUERY FUNCTIONS
# ============================================================================

func _on_refresh_all_pressed():
	_log_message("[color=yellow]🔄 Refreshing all achievement data...[/color]")
	if EpicOS:
		EpicOS.query_achievement_definitions()
		EpicOS.query_player_achievements()
		EpicOS.query_achievement_stats()
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

func _on_query_definitions_pressed():
	_log_message("[color=yellow]Querying achievement definitions...[/color]")
	if EpicOS:
		EpicOS.query_achievement_definitions()
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

func _on_query_player_pressed():
	_log_message("[color=yellow]Querying player achievements...[/color]")
	if EpicOS:
		EpicOS.query_player_achievements()
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

func _on_query_stats_pressed():
	_log_message("[color=yellow]Querying achievement stats...[/color]")
	if EpicOS:
		EpicOS.query_achievement_stats()
	else:
		_log_message("[color=red]✗ EpicOS not available[/color]")

# ============================================================================
# LIST SELECTION HANDLERS
# ============================================================================

func _on_definitions_list_item_selected(index: int):
	if not definitions_list or index < 0 or index >= cached_definitions.size():
		return
		
	var definition: Dictionary = cached_definitions[index]
	var achievement_id := str(definition.get("achievement_id", ""))
	if achievement_input:
		achievement_input.text = achievement_id
	_log_message("[color=cyan]Selected definition: " + achievement_id + "[/color]")

func _on_player_list_item_selected(index: int):
	if not player_list or index < 0 or index >= cached_player_achievements.size():
		return
		
	var achievement: Dictionary = cached_player_achievements[index]
	var achievement_id := str(achievement.get("achievement_id", "Unknown"))
	var unlocked: bool = achievement.get("unlocked", false)
	var progress: float = achievement.get("progress", 0.0)
	_log_message("[color=cyan]Selected: " + achievement_id + " - Unlocked: " + str(unlocked) + " - Progress: " + str(progress) + "%[/color]")

func _on_stats_list_item_selected(index: int):
	if not stats_list or index < 0 or index >= cached_stats.size():
		return
		
	var stat: Dictionary = cached_stats[index]
	var stat_name := str(stat.get("name", ""))
	var stat_value: int = stat.get("value", 0)
	if stat_name_input:
		stat_name_input.text = stat_name
	if stat_amount_input:
		stat_amount_input.value = float(stat_value)
	_set_featured_stat(stat_name, stat_value)
	_log_message("[color=cyan]Selected stat: " + stat_name + " = " + str(stat_value) + "[/color]")

# ============================================================================
# EPICOS SIGNAL HANDLERS
# ============================================================================

func _on_achievement_definitions_completed(success: bool, definitions: Array):
	if success:
		cached_definitions = definitions
		_refresh_definitions_display()
		_refresh_achievements_display()
		_log_message("[color=green]✓ Loaded " + str(definitions.size()) + " achievement definitions[/color]")
	else:
		_log_message("[color=red]✗ Failed to load achievement definitions[/color]")

func _on_player_achievements_completed(success: bool, achievements: Array):
	if success:
		cached_player_achievements = achievements
		_refresh_player_display()
		_refresh_achievements_display()
		_log_message("[color=green]✓ Loaded " + str(achievements.size()) + " player achievements[/color]")
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
			title = definition.get("display_name", achievement_id)
			description = definition.get("description", "")

	_log_message("[color=yellow]" + title + "[/color]")
	if not description.is_empty():
		_log_message("[color=white]" + description + "[/color]")
	_log_message("[color=cyan]Achievement ID: " + achievement_id + "[/color]")
	_log_message("[color=cyan]Unlocked at: " + Time.get_datetime_string_from_unix_time(unlock_time) + "[/color]")
	_log_message("[color=gold]═══════════════════════════════════════[/color]")

func _on_achievement_stats_completed(success: bool, stats: Array):
	if success:
		cached_stats = stats
		_refresh_stats_display()
		_log_message("[color=green]✓ Loaded " + str(stats.size()) + " achievement stats[/color]")
	else:
		_log_message("[color=red]✗ Failed to load achievement stats[/color]")

func _on_login_status_changed(success: bool, user_info: Dictionary):
	if success:
		var username: String = user_info.get("display_name", "Unknown")
		_log_message("[color=green]✓ Logged in as " + username + "[/color]")
		_update_ui_state()
		_on_refresh_all_pressed()

func _on_logout_status_changed(success: bool):
	if success:
		_log_message("[color=yellow]Logged out - clearing cached data[/color]")
		cached_definitions.clear()
		cached_player_achievements.clear()
		cached_stats.clear()
		_refresh_all_displays()
		_set_featured_stat("", 0)
	_update_ui_state()

# ============================================================================
# DISPLAY UPDATE FUNCTIONS
# ============================================================================

func _refresh_all_displays():
	_refresh_definitions_display()
	_refresh_player_display()
	_refresh_stats_display()
	_refresh_achievements_display()

func _refresh_definitions_display():
	if not definitions_list:
		_log_message("[color=yellow]Definitions list not available in current UI[/color]")
		return
		
	definitions_list.clear()

	if cached_definitions.is_empty():
		return

	for definition in cached_definitions:
		var display_name := str(definition.get("display_name", "Unknown"))
		var achievement_id := str(definition.get("achievement_id", ""))
		var description := str(definition.get("description", ""))

		var display_text := display_name
		if not achievement_id.is_empty():
			display_text += " [" + achievement_id + "]"

		definitions_list.add_item(display_text)
		var item_index := definitions_list.get_item_count() - 1
		definitions_list.set_item_tooltip(item_index, description)

func _refresh_player_display():
	if not player_list:
		_log_message("[color=yellow]Player achievements list not available in current UI[/color]")
		return
		
	player_list.clear()

	if cached_player_achievements.is_empty():
		return

	for achievement in cached_player_achievements:
		var achievement_id := str(achievement.get("achievement_id", "Unknown"))
		var unlocked: bool = achievement.get("unlocked", false)
		var progress: float = achievement.get("progress", 0.0)
		var unlock_time: int = achievement.get("unlock_time", 0)

		var display_text := ""

		if unlocked:
			var date_str := ""
			if unlock_time > 0:
				var datetime: Dictionary = Time.get_datetime_dict_from_unix_time(unlock_time)
				date_str = " (%d-%02d-%02d)" % [datetime.year, datetime.month, datetime.day]
			display_text = "✓ " + achievement_id + date_str
		else:
			display_text = "⏳ " + achievement_id + " (" + str(progress) + "%)"

		player_list.add_item(display_text)

func _refresh_stats_display():
	if stats_list:
		stats_list.clear()

	if cached_stats.is_empty():
		_update_featured_stat_display_from_cache()
		return

	if stats_list:
		for stat in cached_stats:
			var name := str(stat.get("name", "Unknown"))
			var value: int = stat.get("value", 0)
			var display_text := name + ": " + str(value)
			stats_list.add_item(display_text)

	_update_featured_stat_display_from_cache()

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

func _create_placeholder_card():
	"""Create a placeholder card when no achievement data is available"""
	var card = PanelContainer.new()
	card.custom_minimum_size = Vector2(0, 60)  # Smaller height
	card.size_flags_horizontal = Control.SIZE_EXPAND_FILL  # Make it wide
	
	var margin = MarginContainer.new()
	margin.add_theme_constant_override("margin_left", 10)  # Reduced margins
	margin.add_theme_constant_override("margin_right", 10)
	margin.add_theme_constant_override("margin_top", 8)
	margin.add_theme_constant_override("margin_bottom", 8)
	card.add_child(margin)
	
	var label = Label.new()
	label.text = "Loading achievements... Please wait or click 'Refresh Achievements'"
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	label.add_theme_font_size_override("font_size", 14)  # Smaller font
	margin.add_child(label)
	
	achievement_cards_container.add_child(card)

func _create_achievement_card(definition: Dictionary, player_data: Dictionary):
	"""Create a visual achievement card"""
	var achievement_id := str(definition.get("achievement_id", "Unknown"))
	var display_name := str(definition.get("display_name", achievement_id))
	var description := str(definition.get("description", "No description available"))
	
	# Player data
	var unlocked: bool = player_data.get("unlocked", false)
	var progress: float = player_data.get("progress", 0.0)
	var unlock_time: int = player_data.get("unlock_time", 0)
	
	# Create card container - horizontally smaller but wide
	var card = PanelContainer.new()
	card.custom_minimum_size = Vector2(0, 80)  # Reduced height
	card.size_flags_horizontal = Control.SIZE_EXPAND_FILL  # Make it wide
	
	# Add margin - reduced margins
	var margin = MarginContainer.new()
	margin.add_theme_constant_override("margin_left", 10)  # Reduced from 15
	margin.add_theme_constant_override("margin_right", 10)
	margin.add_theme_constant_override("margin_top", 8)   # Reduced from 10
	margin.add_theme_constant_override("margin_bottom", 8)
	card.add_child(margin)
	
	# Create main horizontal layout
	var hbox = HBoxContainer.new()
	hbox.add_theme_constant_override("separation", 12)  # Reduced from 15
	margin.add_child(hbox)
	
	# Status icon - smaller
	var status_label = Label.new()
	status_label.custom_minimum_size = Vector2(25, 0)  # Reduced from 30
	if unlocked:
		status_label.text = "🏆"
		status_label.modulate = Color.GOLD
	else:
		status_label.text = "⏳"
		status_label.modulate = Color.GRAY
	status_label.add_theme_font_size_override("font_size", 20)  # Reduced from 24
	status_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	status_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	hbox.add_child(status_label)
	
	# Achievement info
	var info_container = VBoxContainer.new()
	info_container.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	hbox.add_child(info_container)
	
	# Title - smaller font
	var title_label = Label.new()
	title_label.text = display_name
	title_label.add_theme_font_size_override("font_size", 14)  # Reduced from 16
	if unlocked:
		title_label.modulate = Color.WHITE
	else:
		title_label.modulate = Color.LIGHT_GRAY
	info_container.add_child(title_label)
	
	# Description - smaller font
	var desc_label = Label.new()
	desc_label.text = description
	desc_label.add_theme_font_size_override("font_size", 10)  # Reduced from 12
	desc_label.modulate = Color.LIGHT_GRAY
	desc_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	info_container.add_child(desc_label)
	
	# Progress/Status info - smaller font
	var status_info = Label.new()
	if unlocked:
		if unlock_time > 0:
			var datetime: Dictionary = Time.get_datetime_dict_from_unix_time(unlock_time)
			var date_str = "%d-%02d-%02d %02d:%02d" % [datetime.year, datetime.month, datetime.day, datetime.hour, datetime.minute]
			status_info.text = "Unlocked on: " + date_str
		else:
			status_info.text = "Unlocked"
		status_info.modulate = Color.GREEN
	else:
		status_info.text = "Progress: " + str(progress) + "%"
		status_info.modulate = Color.YELLOW
	status_info.add_theme_font_size_override("font_size", 9)  # Reduced from 10
	info_container.add_child(status_info)
	
	# Actions - smaller
	var actions_container = VBoxContainer.new()
	actions_container.custom_minimum_size = Vector2(100, 0)  # Reduced from 120
	hbox.add_child(actions_container)
	
	# Achievement ID (for testing) - smaller font
	var id_label = Label.new()
	id_label.text = "ID: " + achievement_id
	id_label.add_theme_font_size_override("font_size", 8)  # Reduced from 9
	id_label.modulate = Color.DIM_GRAY
	id_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	actions_container.add_child(id_label)
	
	# Unlock button (only if not unlocked) - smaller
	if not unlocked:
		var unlock_btn = Button.new()
		unlock_btn.text = "Unlock"
		unlock_btn.custom_minimum_size = Vector2(80, 25)  # Reduced from 100x30
		unlock_btn.pressed.connect(func(): _unlock_achievement(achievement_id))
		actions_container.add_child(unlock_btn)
	
	# Add the card to the container
	achievement_cards_container.add_child(card)
	
	# Log the achievement for debugging
	_log_message("[color=cyan]📋 " + display_name + " (" + achievement_id + ") - " + ("✅ Unlocked" if unlocked else "⏳ " + str(progress) + "%") + "[/color]")

func _update_featured_stat_display_from_cache():
	if not stat_name_input:
		return
		
	var target_name := stat_name_input.text.strip_edges()

	if target_name.is_empty():
		if cached_stats.is_empty():
			_set_featured_stat("", 0)
			return

		var first_stat: Dictionary = cached_stats[0]
		_set_featured_stat(str(first_stat.get("name", "")), int(first_stat.get("value", 0)))
		return

	for stat in cached_stats:
		var name := str(stat.get("name", ""))
		if name == target_name:
			_set_featured_stat(name, int(stat.get("value", 0)))
			return

	_set_featured_stat(target_name, 0)

func _set_featured_stat(stat_name: String, value: int):
	var display_name := stat_name if not stat_name.is_empty() else "Selected Stat"
	if featured_stat_name_label:
		featured_stat_name_label.text = display_name
	if featured_stat_value_label:
		featured_stat_value_label.text = str(value)

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
		quick_increment_ten_button,
		quick_refresh_all_button,
		refresh_achievements_button,
		refresh_stats_button,
		unlock_button,
		ingest_stat_button,
		query_definitions_button,
		query_player_button,
		query_stats_button,
		featured_increment_one_button,
		featured_increment_five_button,
		featured_increment_ten_button
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
