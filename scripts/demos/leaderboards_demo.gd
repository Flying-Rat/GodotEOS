extends Control

# Leaderboards Demo Script
# Demonstrates how to use the EpicOS leaderboards features

# Status Section
@onready var status_label: Label = $VBoxContainer/StatusContainer/StatusPanel/StatusLabel
@onready var refresh_button: Button = $VBoxContainer/StatusContainer/RefreshButton
@onready var back_button: Button = $VBoxContainer/StatusContainer/BackButton

# Stats Section
@onready var test_stats_button: Button = $VBoxContainer/StatsSection/HBoxContainer/TestStatsButton
@onready var random_score_button: Button = $VBoxContainer/StatsSection/HBoxContainer/RandomScoreButton
@onready var stat_name_input: LineEdit = $VBoxContainer/StatsSection/HBoxContainer/StatNameInput
@onready var stat_value_input: SpinBox = $VBoxContainer/StatsSection/HBoxContainer/StatValueInput
@onready var ingest_stat_button: Button = $VBoxContainer/StatsSection/HBoxContainer/IngestStatButton

# Ranks Query Section
@onready var query_definitions_button: Button = $VBoxContainer/RanksQuerySection/QueryDefinitionsButton
@onready var query_ranks_button: Button = $VBoxContainer/RanksQuerySection/QueryRanksButton
@onready var leaderboard_input: LineEdit = $VBoxContainer/RanksQuerySection/QuerySpecificHBoxContainer/LeaderboardInput
@onready var limit_input: SpinBox = $VBoxContainer/RanksQuerySection/QuerySpecificHBoxContainer/LimitInput
@onready var query_specific_ranks_button: Button = $VBoxContainer/RanksQuerySection/QuerySpecificHBoxContainer/QuerySpecificRanksButton

# Leadeboards Lists
@onready var definitions_list: ItemList = $VBoxContainer/DataSection/DefinitionsPanel/DefinitionsContainer/DefinitionsList
@onready var ranks_list: ItemList = $VBoxContainer/DataSection/RanksPanel/RanksContainer/RanksList
@onready var user_scores_list: ItemList = $VBoxContainer/DataSection/UserScoresPanel/UserScoresContainer/UserScoresList

# Output Log Section
@onready var output_text: RichTextLabel = $VBoxContainer/OutputSection/OutputText

var cached_definitions: Array = []
var cached_ranks: Array = []
var cached_user_scores: Dictionary = {}

# Test leaderboard IDs and stats for demonstration
var test_leaderboard_ids: Array = [
	"global_score",
	"daily_score",
	"weekly_score",
	"kills",
	"wins"
]

var test_stat_names: Array = [
	"score",
	"kills",
	"wins",
	"games_played",
	"time_played"
]

func _ready():
	# Connect button signals
	query_definitions_button.pressed.connect(_on_query_definitions_button_pressed)
	query_ranks_button.pressed.connect(_on_query_ranks_button_pressed)
	refresh_button.pressed.connect(_on_refresh_button_pressed)
	query_specific_ranks_button.pressed.connect(_on_query_specific_ranks_button_pressed)

	ingest_stat_button.pressed.connect(_on_ingest_stat_button_pressed)
	test_stats_button.pressed.connect(_on_test_stats_button_pressed)
	random_score_button.pressed.connect(_on_random_score_button_pressed)

	back_button.pressed.connect(_on_back_button_pressed)

	# Connect list selection signals
	definitions_list.item_selected.connect(_on_definitions_list_item_selected)
	ranks_list.item_selected.connect(_on_ranks_list_item_selected)
	user_scores_list.item_selected.connect(_on_user_scores_list_item_selected)

	# Connect EpicOS signals
	if EpicOS:
		EpicOS.leaderboard_definitions_completed.connect(_on_leaderboard_definitions_completed)
		EpicOS.leaderboard_ranks_completed.connect(_on_leaderboard_ranks_completed)
		EpicOS.leaderboard_user_scores_completed.connect(_on_leaderboard_user_scores_completed)
		EpicOS.login_completed.connect(_on_login_status_changed)
		EpicOS.logout_completed.connect(_on_logout_status_changed)

	# Enable debug mode for detailed logging
	if EpicOS:
		EpicOS.set_debug_mode(true)

	_update_ui_state()
	_log_message("[color=cyan]Leaderboards Demo initialized. Ready to demonstrate EpicOS leaderboards functionality.[/color]")

func _on_query_definitions_button_pressed():
	_log_message("[color=yellow]Querying leaderboard definitions...[/color]")

	if EpicOS:
		EpicOS.query_leaderboard_definitions()
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_query_ranks_button_pressed():
	if cached_definitions.is_empty():
		_log_message("[color=red]No leaderboard definitions available. Query definitions first![/color]")
		return

	# Query ranks for the first available leaderboard
	var first_leaderboard = cached_definitions[0]
	var leaderboard_id = str(first_leaderboard.get("leaderboard_id", ""))

	if leaderboard_id.is_empty():
		_log_message("[color=red]Invalid leaderboard definition![/color]")
		return

	_log_message("[color=yellow]Querying ranks for leaderboard: " + leaderboard_id + "[/color]")

	if EpicOS:
		EpicOS.query_leaderboard_ranks(leaderboard_id, 10)
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_refresh_button_pressed():
	_log_message("[color=yellow]Refreshing display from cache...[/color]")
	_refresh_displays()

func _on_query_specific_ranks_button_pressed():
	var leaderboard_id = leaderboard_input.text.strip_edges()
	var limit = int(limit_input.value)

	if leaderboard_id.is_empty():
		_log_message("[color=red]Please enter a leaderboard ID![/color]")
		return

	# Validate that the leaderboard ID exists
	if not cached_definitions.is_empty():
		var valid_ids = []
		var found = false
		for definition in cached_definitions:
			var def_id = definition.get("leaderboard_id", "")
			valid_ids.append(def_id)
			if def_id == leaderboard_id:
				found = true
				break

		if not found:
			_log_message("[color=red]⚠ Warning: Leaderboard ID '" + leaderboard_id + "' not found in definitions![/color]")
			_log_message("[color=yellow]Available leaderboard IDs: " + str(valid_ids) + "[/color]")
			_log_message("[color=yellow]Query may fail. Select a leaderboard from the definitions list or use a valid ID.[/color]")

	_log_message("[color=yellow]Querying ranks for leaderboard: " + leaderboard_id + " (limit: " + str(limit) + ")[/color]")

	if EpicOS:
		EpicOS.query_leaderboard_ranks(leaderboard_id, limit)
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
		EpicOS.ingest_stat(stat_name, stat_value)
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_test_stats_button_pressed():
	var test_stats = {
		"score": randi() % 10000 + 1000,
		"kills": randi() % 50 + 1,
		"wins": randi() % 10 + 1,
		"games_played": randi() % 100 + 10,
		"time_played": randi() % 3600 + 300
	}

	_log_message("[color=yellow]Ingesting multiple test stats: " + str(test_stats) + "[/color]")

	if EpicOS:
		EpicOS.ingest_stats(test_stats)
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_random_score_button_pressed():
	var random_score = randi() % 50000 + 1000

	_log_message("[color=yellow]Submitting random score: " + str(random_score) + "[/color]")

	if EpicOS:
		EpicOS.ingest_stat("score", random_score)
	else:
		_log_message("[color=red]EpicOS not available![/color]")

func _on_back_button_pressed():
	# Navigate back to demo menu
	get_tree().change_scene_to_file("res://scenes/demos/demo_menu.tscn")

func _on_definitions_list_item_selected(index: int):
	if index >= 0 and index < cached_definitions.size():
		var definition = cached_definitions[index]
		var leaderboard_id = str(definition.get("leaderboard_id", ""))
		leaderboard_input.text = leaderboard_id
		_log_message("[color=cyan]Selected leaderboard: " + str(definition) + "[/color]")

func _on_ranks_list_item_selected(index: int):
	if index >= 0 and index < cached_ranks.size():
		var rank_entry = cached_ranks[index]
		_log_message("[color=cyan]Selected rank entry: " + str(rank_entry) + "[/color]")

func _on_user_scores_list_item_selected(index: int):
	# User scores list shows individual score entries
	var keys = cached_user_scores.keys()
	if index >= 0 and index < keys.size():
		var user_id = keys[index]
		var user_score_data = cached_user_scores[user_id]
		_log_message("[color=cyan]Selected user score: " + user_id + " = " + str(user_score_data) + "[/color]")

func _on_leaderboard_definitions_completed(success: bool, definitions: Array):
	if success:
		_log_message("[color=green]✓ Leaderboard definitions query completed![/color]")
		_log_message("[color=green]Found " + str(definitions.size()) + " leaderboard definitions[/color]")

		cached_definitions = definitions
		_refresh_definitions_display()

		if definitions.size() == 0:
			_log_message("[color=yellow]⚠ No leaderboard definitions found. Make sure you have configured leaderboards in Epic Developer Portal.[/color]")
		else:
			# Log available leaderboards for reference
			_log_message("[color=cyan]Available leaderboards:[/color]")
			for definition in definitions:
				var lb_id = definition.get("leaderboard_id", "")
				var stat_name = definition.get("stat_name", "")
				_log_message("[color=cyan]  • " + lb_id + " (stat: " + stat_name + ")[/color]")

			# Auto-populate the first leaderboard ID in the input field
			if leaderboard_input:
				var first_id = definitions[0].get("leaderboard_id", "")
				leaderboard_input.text = first_id
				_log_message("[color=green]Auto-populated leaderboard input with: " + first_id + "[/color]")
	else:
		_log_message("[color=red]✗ Leaderboard definitions query failed![/color]")
		_log_message("[color=red]Check the console output for error details.[/color]")

func _on_leaderboard_ranks_completed(success: bool, ranks: Array):
	if success:
		_log_message("[color=green]✓ Leaderboard ranks query completed![/color]")
		_log_message("[color=green]Found " + str(ranks.size()) + " rank entries[/color]")

		cached_ranks = ranks
		_refresh_ranks_display()

		if ranks.size() == 0:
			_log_message("[color=yellow]⚠ No rank entries found. The leaderboard may be empty.[/color]")
			_log_message("[color=yellow]Try ingesting some stats first to populate the leaderboard.[/color]")
	else:
		_log_message("[color=red]✗ Leaderboard ranks query failed![/color]")
		_log_message("[color=red]Check the console output for the specific error code.[/color]")
		_log_message("[color=yellow]Common issues:[/color]")
		_log_message("[color=yellow]  • Error 18: Invalid leaderboard ID or not configured[/color]")
		_log_message("[color=yellow]  • The leaderboard ID must match what's in Epic Developer Portal[/color]")
		_log_message("[color=yellow]  • Use 'Query Definitions' to see available leaderboard IDs[/color]")
		_log_message("[color=yellow]  • Example: Use 'EnemiesSmashedEver' not 'EnemiesDefeated'[/color]")

func _on_leaderboard_user_scores_completed(success: bool, user_scores: Dictionary):
	if success:
		_log_message("[color=green]✓ Leaderboard user scores query completed![/color]")
		_log_message("[color=green]Found scores for " + str(user_scores.size()) + " users[/color]")

		cached_user_scores = user_scores
		_refresh_user_scores_display()
	else:
		_log_message("[color=red]✗ Leaderboard user scores query failed![/color]")

func _on_login_status_changed(success: bool, user_info: Dictionary):
	if success:
		_log_message("[color=green]User logged in - leaderboard features now available[/color]")
	_update_ui_state()

func _on_logout_status_changed(success: bool):
	if success:
		_log_message("[color=yellow]User logged out - clearing leaderboard data[/color]")
		cached_definitions.clear()
		cached_ranks.clear()
		cached_user_scores.clear()
		_refresh_displays()
	_update_ui_state()

func _refresh_displays():
	_refresh_definitions_display()
	_refresh_ranks_display()
	_refresh_user_scores_display()

func _refresh_definitions_display():
	definitions_list.clear()

	if cached_definitions.is_empty():
		_log_message("[color=yellow]No leaderboard definitions to display. Try querying first.[/color]")
		return

	for definition in cached_definitions:
		var leaderboard_id = str(definition.get("leaderboard_id", ""))
		var agregation = str(definition.get("aggregation", ""))
		var stat_name = str(definition.get("stat_name", ""))

		var display_text = leaderboard_id
		if not agregation.is_empty():
			display_text += " [" + agregation + "]"
		if not stat_name.is_empty():
			display_text += " (" + stat_name + ")"

		definitions_list.add_item(display_text)

	_log_message("[color=cyan]Refreshed definitions display with " + str(cached_definitions.size()) + " definitions[/color]")

func _refresh_ranks_display():
	ranks_list.clear()

	if cached_ranks.is_empty():
		_log_message("[color=yellow]No leaderboard ranks to display. Try querying ranks first.[/color]")
		return

	for i in range(cached_ranks.size()):
		var rank_entry = cached_ranks[i]
		var rank = rank_entry.get("rank", i + 1)
		var user_id = str(rank_entry.get("user_id", "Unknown User"))
		var display_name = str(rank_entry.get("display_name", ""))
		var score = rank_entry.get("score", 0)

		var display_text = "#" + str(rank) + " - " + display_name + ": " + str(score) + " - " + user_id 
		ranks_list.add_item(display_text)

	_log_message("[color=cyan]Refreshed ranks display with " + str(cached_ranks.size()) + " entries[/color]")

func _refresh_user_scores_display():
	user_scores_list.clear()

	if cached_user_scores.is_empty():
		_log_message("[color=yellow]No user scores to display.[/color]")
		return

	for user_id in cached_user_scores:
		var user_score_data = cached_user_scores[user_id]
		var score = user_score_data.get("score", 0)
		var rank = user_score_data.get("rank", "N/A")

		var display_text = str(user_id) + ": " + str(score) + " (Rank: " + str(rank) + ")"
		user_scores_list.add_item(display_text)

	_log_message("[color=cyan]Refreshed user scores display with " + str(cached_user_scores.size()) + " users[/color]")

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
		status_label.text = "Status: Not logged in (Leaderboards require authentication)"
	else:
		status_label.text = "Status: Logged in - Leaderboard features available"

	# Enable/disable buttons based on authentication status
	var leaderboards_available = platform_initialized and is_logged_in
	query_definitions_button.disabled = not leaderboards_available
	query_ranks_button.disabled = not leaderboards_available
	query_specific_ranks_button.disabled = not leaderboards_available
	ingest_stat_button.disabled = not leaderboards_available
	test_stats_button.disabled = not leaderboards_available
	random_score_button.disabled = not leaderboards_available

func _log_message(message: String):
	if output_text:
		output_text.append_text(message + "\n")

# Demonstrate using cached leaderboard data
func _demonstrate_cached_data():
	if not EpicOS:
		return

	# Show how to access cached data directly
	var cached_definitions_list = EpicOS.get_leaderboard_definitions()
	var cached_ranks_list = EpicOS.get_leaderboard_ranks()
	var cached_user_scores_dict = EpicOS.get_leaderboard_user_scores()

	_log_message("[color=cyan]Cached definitions count: " + str(cached_definitions_list.size()) + "[/color]")
	_log_message("[color=cyan]Cached ranks count: " + str(cached_ranks_list.size()) + "[/color]")
	_log_message("[color=cyan]Cached user scores count: " + str(cached_user_scores_dict.size()) + "[/color]")

# Demonstrate querying user scores for specific users
func _query_user_scores_example():
	if cached_definitions.is_empty():
		_log_message("[color=red]No leaderboard definitions available for user scores query![/color]")
		return

	var leaderboard_id = str(cached_definitions[0].get("leaderboard_id", ""))
	if leaderboard_id.is_empty():
		return

	# Example: Query scores for specific user IDs (replace with actual user IDs)
	var example_user_ids = ["user1", "user2", "user3"]

	_log_message("[color=yellow]Querying user scores for leaderboard: " + leaderboard_id + "[/color]")
	_log_message("[color=yellow]Users: " + str(example_user_ids) + "[/color]")

	if EpicOS:
		EpicOS.query_leaderboard_user_scores(leaderboard_id, example_user_ids)
	else:
		_log_message("[color=red]EpicOS not available![/color]")

# Update UI state periodically
func _process(_delta):
	# Update UI state periodically (every second)
	if Engine.get_process_frames() % 60 == 0:  # Assuming 60 FPS
		_update_ui_state()
