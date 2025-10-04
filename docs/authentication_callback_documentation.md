# EpicOS Authentication Callbacks Documentation

This document describes the callback functions for the EpicOS Authentication subsystem and their parameters.

## Callbacks

### Login

#### `on_authentication_completed(success: bool, user_info: Dictionary)`
Called when login completes.

**Parameters:**
- `success` (bool): Login success
- `user_info` (Dictionary): User information

**Dictionary Structure:**
```gdscript
{
    "display_name": String,
    "epic_account_id": String,
    "product_user_id": String
}
```

### Logout

#### `on_logout_completed(success: bool)`
Called when logout completes.

**Parameters:**
- `success` (bool): Logout success

## Signal Connections

```gdscript
EpicOS.login_completed.connect(on_authentication_completed)
EpicOS.logout_completed.connect(on_logout_completed)
```