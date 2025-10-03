# EpicOS Achievements Demo - Callback Parameters Documentation

This document describes all callback functions used in the EpicOS Achievements Demo and the structure of their parameters, particularly focusing on dictionary contents.

## Overview

The achievements demo connects to various EpicOS signals and handles their callbacks. Each callback provides specific data structures that contain achievement, statistics, and user information.

## Callback Functions and Parameters

### 1. Achievement Definitions Callbacks

#### `_on_achievement_definitions_completed(success: bool, definitions: Array)`
Called when achievement definitions are queried from Epic Online Services.

**Parameters:**
- `success` (bool): Whether the query was successful
- `definitions` (Array): Array of achievement definition dictionaries

**Dictionary Structure for each definition:**
```gdscript
{
    "achievement_id": String,           # Unique identifier for the achievement
    "unlocked_display_name": String,    # Display name when achievement is unlocked
    "unlocked_description": String,     # Description when achievement is unlocked
    "locked_display_name": String,      # Display name when achievement is locked
    "locked_description": String,       # Description when achievement is locked
    "flavor_text": String,              # Additional flavor text
    "unlocked_icon_url": String,        # URL to unlocked achievement icon
    "locked_icon_url": String,          # URL to locked achievement icon
    "is_hidden": bool                   # Whether the achievement is hidden
}
```

### 2. Player Achievements Callbacks

#### `_on_player_achievements_completed(success: bool, achievements: Array)`
Called when player achievement progress is queried from Epic Online Services.

**Parameters:**
- `success` (bool): Whether the query was successful
- `achievements` (Array): Array of player achievement progress dictionaries

**Dictionary Structure for each achievement:**
```gdscript
{
    "achievement_id": String,       # Unique identifier for the achievement
    "progress": float,              # Current progress (0.0 to 1.0 for percentage-based)
    "unlock_time": int,             # Unix timestamp when unlocked (0 if not unlocked)
    "is_unlocked": bool,            # Whether the achievement is unlocked
    "display_name": String,         # Current display name (unlocked or locked)
    "description": String,          # Current description (unlocked or locked)
    "icon_url": String,             # Current icon URL (unlocked or locked)
    "flavor_text": String           # Additional flavor text
}
```

### 3. Achievement Statistics Callbacks

#### `_on_achievement_stats_completed(success: bool, stats: Array)`
Called when achievement statistics are queried from Epic Online Services.

**Parameters:**
- `success` (bool): Whether the query was successful
- `stats` (Array): Array of achievement statistic dictionaries

**Dictionary Structure for each stat:**
```gdscript
{
    "name": String,         # Name/identifier of the statistic
    "value": float,         # Current value of the statistic
    "start_time": int,      # Unix timestamp when tracking started
    "end_time": int         # Unix timestamp when tracking ended (0 if ongoing)
}
```

### 4. Achievement Unlock Callbacks

#### `_on_achievements_unlocked_completed(success: bool, unlocked_achievement_ids: Array)`
Called when an achievement unlock request is processed.

**Parameters:**
- `success` (bool): Whether the unlock request was successful
- `unlocked_achievement_ids` (Array): Array of strings containing achievement IDs that were unlocked

#### `_on_achievement_unlocked(achievement_id: String, unlock_time: int)`
Called when an individual achievement is unlocked (real-time notification).

**Parameters:**
- `achievement_id` (String): The ID of the achievement that was unlocked
- `unlock_time` (int): Unix timestamp when the achievement was unlocked

### 5. Authentication Callbacks

#### `_on_login_status_changed(success: bool, user_info: Dictionary)`
Called when login status changes.

**Parameters:**
- `success` (bool): Whether login was successful
- `user_info` (Dictionary): User information dictionary

**Dictionary Structure:**
```gdscript
{
    "display_name": String    # User's display name
}
```

#### `_on_logout_status_changed(success: bool)`
Called when logout is completed.

**Parameters:**
- `success` (bool): Whether logout was successful

## Data Flow and Usage

### Achievement Definitions
- Queried once and cached
- Used to display achievement information and requirements
- Combined with player progress to show current state

### Player Achievements
- Queried to get current progress on all achievements
- Updated in real-time when achievements are unlocked
- Contains both progress data and display information

### Statistics
- Only returns stats that have been initialized/touched
- Used for tracking player progress toward achievements
- Can be incremented using `ingest_achievement_stat()`

### User Information
- Contains basic user profile information
- Currently only includes display name
- May be extended in future versions

## Notes

- **Timestamps**: All timestamp values are Unix timestamps (seconds since epoch)
- **Progress Values**: Achievement progress is typically 0.0 to 1.0 for percentage-based achievements
- **Icon URLs**: May be empty strings if no icons are configured
- **Hidden Achievements**: Have `is_hidden: true` and may show different display names when locked
- **Stat Initialization**: Statistics only appear in queries after they've been modified at least once

## Example Usage in Code

```gdscript
# Processing achievement definitions
for definition in definitions:
    var id = definition.get("achievement_id", "")
    var name = definition.get("unlocked_display_name", "")
    print("Achievement: ", name, " (ID: ", id, ")")

# Processing player achievements
for achievement in achievements:
    var id = achievement.get("achievement_id", "")
    var unlocked = achievement.get("is_unlocked", false)
    var progress = achievement.get("progress", 0.0)
    print("Achievement ", id, " - Unlocked: ", unlocked, " Progress: ", progress)

# Processing statistics
for stat in stats:
    var name = stat.get("name", "")
    var value = stat.get("value", 0)
    print("Stat ", name, ": ", value)
```

## Signal Connections

The demo connects to the following EpicOS signals:

```gdscript
EpicOS.achievement_definitions_completed.connect(_on_achievement_definitions_completed)
EpicOS.player_achievements_completed.connect(_on_player_achievements_completed)
EpicOS.achievements_unlocked_completed.connect(_on_achievements_unlocked_completed)
EpicOS.achievement_unlocked.connect(_on_achievement_unlocked)  # Optional signal
EpicOS.achievement_stats_completed.connect(_on_achievement_stats_completed)
EpicOS.login_completed.connect(_on_login_status_changed)
EpicOS.logout_completed.connect(_on_logout_status_changed)
```

This documentation provides a complete reference for working with EpicOS achievement system callbacks and their data structures.