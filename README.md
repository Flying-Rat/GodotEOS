# GodotEOS
**Epic Online Services (EOS) Integration for Godot Engine**

A comprehensive GDExtension plugin that brings Epic Games Online Services to Godot Engine, enabling developers to integrate achievements, leaderboards, authentication, friends, and more into their games.

## 🚀 Features

- **Authentication**: Epic Games login and license validation
- **Achievements**: Unlock and query player achievements
- **Statistics**: Track and update player stats
- **Leaderboards**: Submit scores and retrieve rankings
- **Friends**: Query and manage player friends lists
- **Cross-Platform**: Windows and Linux support
- **GDScript Integration**: Simple API calls with signal-based callbacks

## 📋 Prerequisites

- Godot Engine 4.3 or higher
- Epic Games Developer Account
- EOS SDK (included in releases)
- Visual Studio 2019/2022 (Windows) or VS Code or GCC (Linux)

## 🛠️ Installation

### Method 1: Download Release
1. Download the latest release from [GitHub Releases](https://github.com/Flying-Rat/GodotEpic/releases)
2. Extract the plugin to your project's `addons/` folder
3. Enable the plugin in Project Settings → Plugins

### Method 2: Build from Source
1. Clone this repository
2. Ensure you have the EOS SDK in the `godot_eos_extension/eos_sdk/` folder
3. Build using SCons:
   ```bash
   scons platform=windows target=template_debug
   ```
4. Copy the built library to your project

## ⚙️ Setup

### 1. Epic Developer Portal Configuration
1. Create a project in the [Epic Developer Portal](https://dev.epicgames.com/)
2. Configure your application settings
3. Note your Product ID, Sandbox ID, and Deployment ID

### 2. Godot Project Setup
1. Add the GodotEOS plugin to your project
2. Create an autoload for `EpicOS` (the plugin will handle this automatically)
3. Configure your EOS credentials in the project settings

### 3. Initialize EOS
```gdscript
extends Node

func _ready():
    # Connect to EOS signals
    EpicOS.login_completed.connect(_on_login_completed)
    EpicOS.achievements_unlocked_completed.connect(_on_achievements_unlocked)

    # Initialize EOS with your Epic credentials
    var config = {
        "product_name": "YourGameName",
        "product_version": "1.0.0",
        "product_id": "your_product_id_here",
        "sandbox_id": "your_sandbox_id_here",
        "deployment_id": "your_deployment_id_here",
        "client_id": "your_client_id_here",
        "client_secret": "your_client_secret_here"
    }
    EpicOS.initialize(config)

func _on_login_completed(success: bool, user_info: Dictionary):
    if success:
        print("Login successful: ", EpicOS.get_current_username())
    else:
        print("Login failed")

func _on_achievements_unlocked(success: bool, unlocked_achievement_ids: Array):
    print("Achievements unlocked: ", unlocked_achievement_ids)
```

## 🎯 Quick Start

### Authentication
```gdscript
# Initialize EOS with your credentials
var config = {
    "product_id": "your_product_id",
    "sandbox_id": "your_sandbox_id",
    "deployment_id": "your_deployment_id",
    "client_id": "your_client_id",
    "client_secret": "your_client_secret"
}
EpicOS.initialize(config)

# Login with Epic Games account portal (recommended for production)
EpicOS.login_with_account_portal()

# Alternative login methods:
# Login with Epic Games email and password
EpicOS.login_with_epic_account("user@example.com", "password")

# Development-only login (for testing without real accounts)
EpicOS.login_with_dev("DevPlayerName")

# Check if user is logged in
if EpicOS.is_user_logged_in():
    print("User is authenticated: ", EpicOS.get_current_username())
```

### Achievements
```gdscript
# Unlock an achievement
EpicOS.unlock_achievement("first_victory")

# Query achievement progress
EpicOS.query_player_achievements()
```

### Statistics
```gdscript
# Update a player statistic
EpicOS.ingest_achievement_stat("games_played", 1)

# Get current stats
var stats = EpicOS.get_achievement_stats()
for stat in stats:
    if stat.stat_name == "games_played":
        print("Games played: ", stat.current_value)
```

### Leaderboards
```gdscript
# Submit a score
EpicOS.ingest_stat("high_score", 1500)

# Get leaderboard data
EpicOS.query_leaderboard_ranks("high_scores", 10)  # Top 10 scores
```

## 📡 API Reference

### Signals
- `login_completed(success: bool, user_info: Dictionary)`
- `logout_completed(success: bool)`
- `achievement_definitions_completed(success: bool, definitions: Array)`
- `player_achievements_completed(success: bool, achievements: Array)`
- `achievements_unlocked_completed(success: bool, unlocked_achievement_ids: Array)`
- `achievement_stats_completed(success: bool, stats: Array)`
- `stats_ingested(success: bool, stat_names: Array)`
- `leaderboard_definitions_completed(success: bool, definitions: Array)`
- `leaderboard_ranks_completed(success: bool, ranks: Array)`
- `leaderboard_user_scores_completed(success: bool, user_scores: Dictionary)`
- `friends_query_completed(success: bool, friends_list: Array)`
- `user_info_query_completed(success: bool, user_info: Dictionary)`

### Methods
- `initialize(config: Dictionary = {})` - Initialize the EOS SDK with configuration
- `login_with_account_portal()` - Authenticate with Epic Games account portal
- `login_with_epic_account(email: String, password: String)` - Login with email/password
- `login_with_device_id(display_name: String)` - Login with device ID
- `login_with_dev(display_name: String)` - Login with developer credentials
- `logout()` - Sign out the current user
- `is_user_logged_in() -> bool` - Check authentication status
- `get_current_username() -> String` - Get current user's display name
- `unlock_achievement(achievement_id: String)` - Unlock an achievement
- `unlock_achievements(achievement_ids: Array)` - Unlock multiple achievements
- `query_player_achievements()` - Query player's achievement progress
- `query_achievement_definitions()` - Query all achievement definitions
- `ingest_achievement_stat(stat_name: String, amount: int)` - Update achievement statistic
- `query_achievement_stats()` - Query achievement statistics
- `query_leaderboard_ranks(leaderboard_id: String, limit: int = 100)` - Query leaderboard ranks
- `ingest_stat(stat_name: String, value: int)` - Submit a statistic for leaderboards
- `ingest_stats(stats: Dictionary)` - Submit multiple statistics
- `query_friends()` - Query user's friends list
- `query_user_info(target_user_id: String)` - Query information about a user

## 🎮 Demo Project

This repository **IS** a complete demo project showcasing all EOS features! You can run it directly to test the plugin functionality. The demo includes:

- **Authentication UI**: Login/logout with Epic Games account
- **Achievement Testing**: Unlock achievements and track progress
- **Statistics Tracking**: Update and retrieve player statistics
- **Leaderboard Integration**: Submit scores and view leaderboard rankings
- **Friends Management**: Query and display friends lists
- **Real-time Output Log**: Monitor EOS operations and responses

### How to Run the Demo

1. Open this project in Godot Engine 4.x
2. Enable the GodotEOS plugin in Project Settings → Plugins
3. Run the project (scenes/demos/demo_menu.tscn)
4. Follow the on-screen instructions to test EOS features

#### Demo Authentication

![Authentication Flow](screenshots/gifs/godoteos_auth.gif)

#### Demo Achievements & Stats

![Achievement Stats Animation](screenshots/gifs/godoteos_achievement_stats.gif)

#### Demo Friends

![Friends Interface](screenshots/gifs/godoteos_friends.gif)

#### Demo Leaderboards

![Leaderboards Interaction](screenshots/gifs/godoteos_leaderboards.gif)

### Demo Workflow

1. **Initialize**: The demo automatically initializes EOS when started
2. **Login**: Click "Login with Epic Games" to authenticate
3. **Test Features**: Once logged in, use the feature buttons to test different EOS APIs
4. **Monitor Output**: Watch the output log to see EOS responses and status updates


### Demo Project Structure

- `scenes/demos/demo_menu.tscn` - Main demo scene, entry point for all demos
- `scenes/demos/` - One scene per feature: authentication, achievements, friends, leaderboards
- `scripts/demos/` - Demo logic and UI event handling for each scene
- `addons/godoteos/` - GodotEOS plugin files
  - `epic_os.gd` - Main EOS interface singleton
  - `plugin.cfg` - Plugin configuration
  - `plugin.gd` - Plugin activation/deactivation logic

### For Plugin Users

When the plugin is complete, developers can:
- Copy the `addons/godoteos/` folder to their own projects
- Use the same API calls demonstrated in `scripts/demos/`
- Reference this demo as a complete integration example

## 🔧 Building

### Windows
```bash
# Debug build
scons platform=windows target=template_debug

# Release build
scons platform=windows target=template_release
```

### Linux
```bash
# Debug build
scons platform=linux target=template_debug

# Release build
scons platform=linux target=template_release
```

## 🐛 Troubleshooting

### Common Issues

**Plugin not loading:**
- Ensure the GDExtension library is in the correct path
- Check that all EOS SDK dependencies are available
- Verify Godot version compatibility (4.x required)

**Authentication failing:**
- Verify your Epic Developer Portal configuration
- Check Product ID, Sandbox ID, and Deployment ID
- Ensure your application is properly configured in the portal

**Features not working:**
- Make sure the corresponding features are enabled in Epic Developer Portal
- Check that your application has the required permissions
- Verify network connectivity and firewall settings

### Debug Mode
Enable debug logging to see detailed EOS operations:
```gdscript
EpicOS.set_debug_mode(true)
```

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Epic Games for the EOS SDK
- GodotSteam project for inspiration
- Godot Engine community

## 📞 Support

- Create an issue on GitHub for bug reports
- Join the discussion in GitHub Discussions
- Check the [Epic Developer Documentation](https://dev.epicgames.com/docs/) for EOS-specific questions

---

**Status**: 🚧 In Development - See [GUIDE.md](GUIDE.md) for setup instructions
