# EpicOS Friends Callbacks Documentation

This document describes the callback functions for the EpicOS Friends subsystem and their parameters.

## Callbacks

### Friends Query

#### `on_friends_query_completed(success: bool, friends_list: Array)`
Called when friends list is queried.

**Parameters:**
- `success` (bool): Query success
- `friends_list` (Array): Array of friend dictionaries

**Dictionary Structure:**
```gdscript
{
    "id": String,
    "display_name": String,
    "status": String  # "Friends", "Invite Sent", "Invite Received", "Not Friends"
}
```

### Friend Info Query

#### `on_friend_info_query_completed(success: bool, friend_info: Dictionary)`
Called when friend info is queried.

**Parameters:**
- `success` (bool): Query success
- `friend_info` (Dictionary): Friend information

**Dictionary Structure:**
```gdscript
{
    "id": String,
    "display_name": String,
    "status": String  # "Friends", "Invite Sent", "Invite Received", "Not Friends"
}
```

## Signal Connections

```gdscript
EpicOS.friends_updated.connect(on_friends_query_completed)
EpicOS.friend_info_updated.connect(on_friend_info_query_completed)
```