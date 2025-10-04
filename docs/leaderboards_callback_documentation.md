# EpicOS Leaderboards Callbacks Documentation

This document describes the callback functions for the EpicOS Leaderboards subsystem and their parameters.

## Callbacks

### Leaderboard Definitions

#### `on_leaderboard_definitions_completed(success: bool, definitions: Array)`
Called when leaderboard definitions are queried.

**Parameters:**
- `success` (bool): Query success
- `definitions` (Array): Array of definition dictionaries

**Dictionary Structure:**
```gdscript
{
    "leaderboard_id": String,
    "stat_name": String,
    "aggregation": int,  # Aggregation type enum value
    "start_time": int,   # Unix timestamp
    "end_time": int      # Unix timestamp
}
```

### Leaderboard Ranks

#### `on_leaderboard_ranks_completed(success: bool, ranks: Array)`
Called when leaderboard ranks are queried.

**Parameters:**
- `success` (bool): Query success
- `ranks` (Array): Array of rank dictionaries

**Dictionary Structure:**
```gdscript
{
    "rank": int,
    "score": int,
    "user_id": String,
    "display_name": String
}
```

### Leaderboard User Scores

#### `on_leaderboard_user_scores_completed(success: bool, user_scores: Dictionary)`
Called when user scores are queried.

**Parameters:**
- `success` (bool): Query success
- `user_scores` (Dictionary): Dictionary with user IDs as keys

**Dictionary Structure (per user):**
```gdscript
{
    "score": int,
    "rank": int  # Always 0 (not available in EOS user score struct)
}
```

## Signal Connections

```gdscript
EpicOS.leaderboard_definitions_updated.connect(on_leaderboard_definitions_completed)
EpicOS.leaderboard_ranks_updated.connect(on_leaderboard_ranks_completed)
EpicOS.leaderboard_user_scores_updated.connect(on_leaderboard_user_scores_completed)
```