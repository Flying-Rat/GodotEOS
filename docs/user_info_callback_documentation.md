# EpicOS User Info Callbacks Documentation

This document describes the callback functions for the EpicOS User Info subsystem and their parameters.

## Callbacks

### User Info Query

#### `on_user_info_query_completed(success: bool, user_info: Dictionary)`
Called when user information is queried.

**Parameters:**
- `success` (bool): Query success
- `user_info` (Dictionary): User information dictionary

**Dictionary Structure:**
```gdscript
{
    "target_user_id": String,      # The Epic Account ID of the queried user
    "display_name": String,        # User's display name
    "nickname": String,            # User's nickname
    "country": String,             # User's country code
    "preferred_language": String   # User's preferred language code
}
```

## Notes

- `target_user_id` is always included in successful queries to identify which user the information belongs to
- All other fields are optional and may be empty strings if not available
- Display name takes precedence over nickname for user identification
- Country and preferred language are ISO standard codes
- Query results are cached by the EOS SDK

## Signal Connections

```gdscript
EpicOS.user_info_updated.connect(on_user_info_query_completed)
```