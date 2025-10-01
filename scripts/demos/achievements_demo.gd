extends Control

# Achievements Demo Script
# Demonstrates how to use the EpicOS achievements features

@onready var status_label: Label = $VBoxContainer/StatusPanel/StatusContainer/StatusLabel
@onready var query_definitions_button: Button = $VBoxContainer/ActionsSection/ActionsGrid/QueryDefinitionsButton
@onready var query_player_button: Button = $VBoxContainer/ActionsSection/ActionsGrid/QueryPlayerButton
@onready var query_stats_button: Button = $VBoxContainer/ActionsSection/ActionsGrid/QueryStatsButton
@onready var refresh_button: Button = $VBoxContainer/ActionsSection/ActionsGrid/RefreshButton

@onready var achievement_input: LineEdit = $VBoxContainer/UnlockSection/UnlockContainer/AchievementInput
@onready var unlock_button: Button = $VBoxContainer/UnlockSection/UnlockContainer/UnlockButton
@onready var unlock_all_button: Button = $VBoxContainer/UnlockSection/UnlockContainer/UnlockAllButton

@onready var stat_name_input: LineEdit = $VBoxContainer/StatsSection/StatsContainer/StatNameInput
@onready var stat_value_input: SpinBox = $VBoxContainer/StatsSection/StatsContainer/StatValueInput
@onready var ingest_stat_button: Button = $VBoxContainer/StatsSection/StatsContainer/IngestStatButton

@onready var definitions_list: ItemList = $VBoxContainer/DataSection/DefinitionsPanel/DefinitionsContainer/DefinitionsScrollContainer/DefinitionsList
@onready var player_list: ItemList = $VBoxContainer/DataSection/PlayerPanel/PlayerContainer/PlayerScrollContainer/PlayerList
@onready var stats_list: ItemList = $VBoxContainer/DataSection/StatsPanel/StatsDataContainer/StatsScrollContainer/StatsList

@onready var output_text: RichTextLabel = $VBoxContainer/OutputSection/OutputScrollContainer/OutputText
@onready var back_button: Button = $VBoxContainer/BackButton

var cached_definitions: Array = []
var cached_player_achievements: Array = []
var cached_stats: Array = []

# Test achievement IDs for demonstration
var test_achievement_ids: Array = [
	"first_game",
	"ten_games",
	"first_win",
	"streak_five",
	"level_up"
]

func _ready():
	# Connect button signals
	query_definitions_button.pressed.connect(_on_query_definitions_button_pressed)
	query_player_button.pressed.connect(_on_query_player_button_pressed)
	query_stats_button.pressed.connect(_on_query_stats_button_pressed)
	refresh_button.pressed.connect(_on_refresh_button_pressed)

	unlock_button.pressed.connect(_on_unlock_button_pressed)
	unlock_all_button.pressed.connect(_on_unlock_all_button_pressed)

	ingest_stat_button.pressed.connect(_on_ingest_stat_button_pressed)

	back_button.pressed.connect(_on_back_button_pressed)

	# Connect list selection signals
	definitions_list.item_selected.connect(_on_definitions_list_item_selected)
	player_list.item_selected.connect(_on_player_list_item_selected)
	stats_list.item_selected.connect(_on_stats_list_item_selected)

	# Connect EpicOS signals
	if EpicOS:
		EpicOS.achievement_definitions_completed.connect(_on_achievement_definitions_completed)
		EpicOS.player_achievements_completed.connect(_on_player_achievements_completed)
		EpicOS.achievements_unlocked_completed.connect(_on_achievements_unlocked_completed)
		EpicOS.achievement_stats_completed.connect(_on_achievement_stats_completed)
		EpicOS.login_completed.connect(_on_login_status_changed)
		EpicOS.logout_completed.connect(_on_logout_status_changed)

	# Enable debug mode for detailed logging
	if EpicOS:
		EpicOS.set_debug_mode(true)

	_update_ui_state()
	_log_message("[color=cyan]Achievements Demo initialized. Ready to demonstrate EpicOS achievements functionality.[/color]")

func _on_query_definitions_button_pressed():
	_log_message("[color=yellow]Querying achievement definitions...[/color]")

	if EpicOS:
		EpicOS.query_achievement_definitions()
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_query_player_button_pressed():
	_log_message("[color=yellow]Querying player achievements...[/color]")

	if EpicOS:
		EpicOS.query_player_achievements()
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_query_stats_button_pressed():
	_log_message("[color=yellow]Querying achievement stats...[/color]")

	if EpicOS:
		EpicOS.query_achievement_stats()
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_refresh_button_pressed():
	_log_message("[color=yellow]Refreshing display from cache...[/color]")
	_refresh_displays()

func _on_unlock_button_pressed():
	var achievement_id = achievement_input.text.strip_edges()

	if achievement_id.is_empty():
		_log_message("[color=red]Please enter an achievement ID![/color]")
		return

	_log_message("[color=yellow]Unlocking achievement: " + achievement_id + "[/color]")

	if EpicOS:
		EpicOS.unlock_achievement(achievement_id)
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_unlock_all_button_pressed():
	_log_message("[color=yellow]Unlocking all test achievements: " + str(test_achievement_ids) + "[/color]")

	if EpicOS:
		EpicOS.unlock_achievements(test_achievement_ids)
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_ingest_stat_button_pressed():
	var stat_name = stat_name_input.text.strip_edges()
	var stat_value = int(stat_value_input.value)

	if stat_name.is_empty():
		_log_message("[color=red]Please enter a stat name![/color]")
		return

	_log_message("[color=yellow]Ingesting stat: " + stat_name + " = " + str(stat_value) + "[/color]")

	if EpicOS:
		EpicOS.ingest_achievement_stat(stat_name, stat_value)
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_back_button_pressed():
	# Navigate back to demo menu
	get_tree().change_scene_to_file("res://scenes/demos/demo_menu.tscn")

func _on_definitions_list_item_selected(index: int):
	if index >= 0 and index < cached_definitions.size():
		var definition = cached_definitions[index]
		achievement_input.text = str(definition.get("achievement_id", ""))
		_log_message("[color=cyan]Selected achievement: " + str(definition) + "[/color]")

func _on_player_list_item_selected(index: int):
	if index >= 0 and index < cached_player_achievements.size():
		var achievement = cached_player_achievements[index]
		_log_message("[color=cyan]Selected player achievement: " + str(achievement) + "[/color]")

func _on_stats_list_item_selected(index: int):
	if index >= 0 and index < cached_stats.size():
		var stat = cached_stats[index]
		stat_name_input.text = str(stat.get("name", ""))
		stat_value_input.value = float(stat.get("value", 0))
		_log_message("[color=cyan]Selected stat: " + str(stat) + "[/color]")

func _on_achievement_definitions_completed(success: bool, definitions: Array):
	if success:
		_log_message("[color=green]✓ Achievement definitions query completed![/color]")
		_log_message("[color=green]Found " + str(definitions.size()) + " achievement definitions[/color]")

		cached_definitions = definitions
		_refresh_definitions_display()
	else:
		_log_message("[color=red]✗ Achievement definitions query failed![/color]")

func _on_player_achievements_completed(success: bool, achievements: Array):
	if success:
		_log_message("[color=green]✓ Player achievements query completed![/color]")
		_log_message("[color=green]Found " + str(achievements.size()) + " player achievements[/color]")

		cached_player_achievements = achievements
		_refresh_player_display()
	else:
		_log_message("[color=red]✗ Player achievements query failed![/color]")

func _on_achievements_unlocked_completed(success: bool, unlocked_achievement_ids: Array):
	if success:
		_log_message("[color=green]✓ Achievements unlocked successfully![/color]")
		_log_message("[color=green]Unlocked achievements: " + str(unlocked_achievement_ids) + "[/color]")

		# Refresh player achievements to see the updates
		if EpicOS:
			EpicOS.query_player_achievements()
	else:
		_log_message("[color=red]✗ Failed to unlock achievements![/color]")

func _on_achievement_stats_completed(success: bool, stats: Array):
	if success:
		_log_message("[color=green]✓ Achievement stats query completed![/color]")
		_log_message("[color=green]Found " + str(stats.size()) + " stats[/color]")

		cached_stats = stats
		_refresh_stats_display()
	else:
		_log_message("[color=red]✗ Achievement stats query failed![/color]")

func _on_login_status_changed(success: bool, user_info: Dictionary):
	if success:
		_log_message("[color=green]User logged in - achievement features now available[/color]")
	_update_ui_state()

func _on_logout_status_changed(success: bool):
	if success:
		_log_message("[color=yellow]User logged out - clearing achievement data[/color]")
		cached_definitions.clear()
		cached_player_achievements.clear()
		cached_stats.clear()
		_refresh_displays()
	_update_ui_state()

func _refresh_displays():
	_refresh_definitions_display()
	_refresh_player_display()
	_refresh_stats_display()

func _refresh_definitions_display():
	definitions_list.clear()

	if cached_definitions.is_empty():
		_log_message("[color=yellow]No achievement definitions to display. Try querying first.[/color]")
		return

	for definition in cached_definitions:
		var display_text = str(definition.get("display_name", "Unknown Achievement"))
		var achievement_id = str(definition.get("achievement_id", ""))
		var description = str(definition.get("description", ""))

		if not achievement_id.is_empty():
			display_text += " [" + achievement_id + "]"

		definitions_list.add_item(display_text)

		# Set tooltip with description
		var item_index = definitions_list.get_item_count() - 1
		definitions_list.set_item_tooltip(item_index, description)

	_log_message("[color=cyan]Refreshed definitions display with " + str(cached_definitions.size()) + " definitions[/color]")

func _refresh_player_display():
	player_list.clear()

	if cached_player_achievements.is_empty():
		_log_message("[color=yellow]No player achievements to display. Try querying first.[/color]")
		return

	for achievement in cached_player_achievements:
		var achievement_id = str(achievement.get("achievement_id", "Unknown"))
		var unlocked = achievement.get("unlocked", false)
		var progress = achievement.get("progress", 0.0)

		var display_text = achievement_id

		if unlocked:
			display_text = "✓ " + display_text + " (Unlocked)"
		else:
			display_text = "⏳ " + display_text + " (" + str(progress) + "%)"

		player_list.add_item(display_text)

	_log_message("[color=cyan]Refreshed player achievements display with " + str(cached_player_achievements.size()) + " achievements[/color]")

func _refresh_stats_display():
	stats_list.clear()

	if cached_stats.is_empty():
		_log_message("[color=yellow]No achievement stats to display. Try querying first.[/color]")
		return

	for stat in cached_stats:
		var name = str(stat.get("name", "Unknown Stat"))
		var value = stat.get("value", 0)

		var display_text = name + ": " + str(value)
		stats_list.add_item(display_text)

	_log_message("[color=cyan]Refreshed stats display with " + str(cached_stats.size()) + " stats[/color]")

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
		status_label.text = "Status: Not logged in (Achievements require authentication)"
	else:
		status_label.text = "Status: Logged in - Achievement features available"

	# Enable/disable buttons based on authentication status
	var achievements_available = platform_initialized and is_logged_in
	query_definitions_button.disabled = not achievements_available
	query_player_button.disabled = not achievements_available
	query_stats_button.disabled = not achievements_available
	unlock_button.disabled = not achievements_available
	unlock_all_button.disabled = not achievements_available
	ingest_stat_button.disabled = not achievements_available

func _log_message(message: String):
	if output_text:
		output_text.append_text(message + "\n")

# Demonstrate using cached achievement data
func _demonstrate_cached_data():
	if not EpicOS:
		return

	# Show how to access cached data directly
	var cached_definitions_list = EpicOS.get_achievement_definitions()
	var cached_player_list = EpicOS.get_player_achievements()
	var cached_stats_list = EpicOS.get_achievement_stats()

	_log_message("[color=cyan]Cached definitions count: " + str(cached_definitions_list.size()) + "[/color]")
	_log_message("[color=cyan]Cached player achievements count: " + str(cached_player_list.size()) + "[/color]")
	_log_message("[color=cyan]Cached stats count: " + str(cached_stats_list.size()) + "[/color]")

	# Example of getting specific achievement data
	if not test_achievement_ids.is_empty():
		var test_id = test_achievement_ids[0]
		var definition = EpicOS.get_achievement_definition(test_id)
		var player_achievement = EpicOS.get_player_achievement(test_id)

		_log_message("[color=cyan]Example - Definition for " + test_id + ": " + str(definition) + "[/color]")
		_log_message("[color=cyan]Example - Player achievement for " + test_id + ": " + str(player_achievement) + "[/color]")

# Update UI state periodically
func _process(_delta):
	# Update UI state periodically (every second)
	if Engine.get_process_frames() % 60 == 0:  # Assuming 60 FPS
		_update_ui_state()