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

## Signal Connections

```gdscript
EpicOS.friends_query_completed.connect(on_friends_query_completed)
```

## Notes

- Friend information is cached after querying
- Use `query_user_info()` to get detailed information about specific friends
- The `user_info_updated` signal will be emitted when detailed friend info is queried