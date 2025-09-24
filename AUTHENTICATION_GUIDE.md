# GodotEpic Authentication Guide

## Understanding Error Code 10 (EOS_InvalidParameters)

Error code 10 indicates that the Epic Online Services (EOS) SDK received invalid parameters in a login request. This is one of the most common authentication errors.

## Common Causes and Solutions

### 1. Device ID Login Issues
**Error**: `Invalid parameters (10) - For Device ID login, make sure EOS Dev Auth Tool is running on localhost:7777`

**Cause**: Device ID login requires the EOS Dev Auth Tool to be running and accessible.

**Solution**:
- Download and install the EOS Dev Auth Tool from Epic Developer Portal
- Run the Dev Auth Tool and ensure it's listening on `localhost:7777`
- Create a test user in the Dev Auth Tool
- Use the same credential name in your Godot application

### 2. Epic Account Login Issues
**Error**: `Invalid parameters (10) - Check email/password format for Epic Account login`

**Cause**: Invalid email format or empty credentials when using email/password login.

**Solution**:
- Ensure email is in valid format (user@domain.com)
- Verify password is not empty
- For Account Portal login (browser/launcher), ensure Epic Launcher is installed

### 3. Connect Service Issues
**Error**: `Invalid parameters (10) - Connect login requires valid Epic Account ID from Auth login`

**Cause**: The Connect service requires a valid Epic Account ID from the Auth service.

**Solution**: This is now automatically handled - the Connect login uses the Epic Account ID from successful Auth login.

## Authentication Flow

The GodotEpic extension uses a two-step authentication process:

1. **Auth Service Login**: Authenticates with Epic Account or Device ID
   - Provides Epic Account ID
   - Enables basic Epic services

2. **Connect Service Login**: Links to cross-platform services
   - Provides Product User ID
   - Enables cross-platform features (friends, achievements, etc.)

## User Info Structure

After successful login, the `login_completed` signal provides a Dictionary with:

```gdscript
{
    "display_name": "Username",
    "epic_account_id": "Epic Account ID string",
    "product_user_id": "Product User ID string (empty if Connect failed)"
}
```

## Troubleshooting Steps

1. **Check EOS Configuration**:
   - Verify `product_id`, `sandbox_id`, `deployment_id` in EOS Developer Portal
   - Ensure client credentials are correct

2. **For Device ID Login**:
   - Install and run EOS Dev Auth Tool
   - Create test users in the tool
   - Verify `localhost:7777` is accessible

3. **For Epic Account Login**:
   - Use valid email/password combination
   - For Account Portal: ensure Epic Launcher is installed
   - Check internet connection

4. **Monitor Authentication Status**:
   - Epic Account ID should be set after Auth login
   - Product User ID should be set after Connect login
   - If Product User ID is empty, cross-platform features are disabled

## Error Code Reference

- **10 (EOS_InvalidParameters)**: Invalid request parameters
- **2 (EOS_InvalidCredentials)**: Invalid email/password
- **32 (EOS_Invalid_Deployment)**: Check deployment_id
- **31 (EOS_Invalid_Sandbox)**: Check sandbox_id
- **33 (EOS_Invalid_Product)**: Check product_id

## Testing Login

The demo scene provides helpful output showing:
- Authentication status messages
- Epic Account ID and Product User ID values
- Cross-platform feature availability
- Common error solutions

Use the demo to test your authentication setup and troubleshoot any issues.