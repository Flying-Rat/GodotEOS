# EpicOS Achievements Callbacks Documentation

This document describes the callback functions for the EpicOS Achievements subsystem and their parameters.

## Callbacks

### Achievement Definitions

#### `on_achievement_definitions_completed(success: bool, definitions: Array)`
Called when achievement definitions are queried.

**Parameters:**
- `success` (bool): Query success
- `definitions` (Array): Array of definition dictionaries

**Dictionary Structure:**
```gdscript
{
    "achievement_id": String,
    "unlocked_display_name": String,
    "unlocked_description": String,
    "locked_display_name": String,
    "locked_description": String,
    "flavor_text": String,
    "unlocked_icon_url": String,
    "locked_icon_url": String,
    "is_hidden": bool
}
```

### Player Achievements

#### `on_player_achievements_completed(success: bool, achievements: Array)`
Called when player achievement progress is queried.

**Parameters:**
- `success` (bool): Query success
- `achievements` (Array): Array of achievement dictionaries

**Dictionary Structure:**
```gdscript
{
    "achievement_id": String,
    "progress": float,  # 0.0 to 1.0
    "unlock_time": int,  # Unix timestamp, 0 if not unlocked
    "is_unlocked": bool,
    "display_name": String,
    "description": String,
    "icon_url": String,
    "flavor_text": String
}
```

### Achievement Statistics

#### `on_achievement_stats_completed(success: bool, stats: Array)`
Called when achievement statistics are queried.

**Parameters:**
- `success` (bool): Query success
- `stats` (Array): Array of statistic dictionaries

**Dictionary Structure:**
```gdscript
{
    "name": String,
    "value": float,
    "start_time": int,  # Unix timestamp
    "end_time": int     # Unix timestamp, 0 if ongoing
}
```

### Achievement Unlocks

#### `on_achievements_unlocked_completed(success: bool, unlocked_achievement_ids: Array)`
Called when unlock request is processed.

**Parameters:**
- `success` (bool): Unlock success
- `unlocked_achievement_ids` (Array): Array of unlocked achievement IDs (strings)

#### `on_achievement_unlocked(achievement_id: String, unlock_time: int)`
Called for real-time unlock notifications.

**Parameters:**
- `achievement_id` (String): Unlocked achievement ID
- `unlock_time` (int): Unix timestamp of unlock

## Notes

- Timestamps are Unix timestamps (seconds since epoch).
- Progress is 0.0 to 1.0 for percentage-based achievements.
- Icon URLs may be empty.
- Hidden achievements have `is_hidden: true`.
- Statistics appear only after modification.

## Signal Connections

```gdscript
EpicOS.achievement_definitions_updated.connect(on_achievement_definitions_completed)
EpicOS.player_achievements_updated.connect(on_player_achievements_completed)
EpicOS.achievements_unlocked.connect(on_achievements_unlocked_completed)
EpicOS.achievement_unlocked.connect(on_achievement_unlocked)
EpicOS.achievement_stats_updated.connect(on_achievement_stats_completed)
```