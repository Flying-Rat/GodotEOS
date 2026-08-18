# GodotEOS log prefix

Date: 2026-08-18

## Goal

Every message from the GodotEOS addon must start with `[GodotEOS]` so a game developer can tell the plugin is speaking, and so support can grep `godot.log` for `[GodotEOS]`.

Logs still go to Godot’s output / `user://logs/godot.log`. No separate plugin log file.

## Line format

Exact shape, always:

```
[GodotEOS] <System>: <message>
```

- `[GodotEOS]` is first on the line. Nothing may appear before it (no `WARNING:`, no `ERROR:`, no `at:` stack line).
- `<System>` is a short name for who is talking.
- `<message>` is only the fact. It must not repeat `[GodotEOS]` or a second bracket tag.

Examples:

```
[GodotEOS] Auth: Login successful
[GodotEOS] EpicOS: Initializing EOS SDK...
[GodotEOS] LogEOS: Connect User Logged In
[GodotEOS] Platform: Failed to create EOS Platform
```

Errors and successes use the same shape. `printerr` may still send errors to stderr (red in the editor); the text does not use an `ERROR:` system name.

## System names

| Source | System name |
|---|---|
| `addons/godoteos/epic_os.gd`, `addons/godoteos/plugin.gd` | `EpicOS` |
| AuthenticationSubsystem; GodotEOS login/logout facade lines | `Auth` |
| PlatformSubsystem; EOS SDK init/create/shutdown | `Platform` |
| UserInfoSubsystem | `UserInfo` |
| FriendsSubsystem | `Friends` |
| AchievementsSubsystem; achievement/stat facade lines | `Achievements` |
| LeaderboardsSubsystem; leaderboard facade lines | `Leaderboards` |
| SubsystemManager; subsystem register/init orchestration | `Core` |
| EOS SDK logging callback | EOS `Category` string as-is (`LogEOS`, `LogEOSAuth`, `LogEOSConnect`, …). If category is empty, use `EOS`. |

Do not use `GodotEOS` as a system name (it is already the prefix). Do not use long names like `AuthenticationSubsystem`.

## Scope

In scope:

- `addons/godoteos/*.gd`
- `godot_eos_extension/src/**` plugin C++ (not `godot-cpp`)
- EOS SDK log callback(s)

Out of scope:

- Demo scenes under `scripts/` and `scenes/`
- A dedicated `godoteos.log` file
- Changing which messages exist (no deleting duplicate Auth + EpicOS lines; only reformat)
- Changing EOS `SetLogLevel` (stays Verbose)
- Changing `EpicOS.set_debug_mode()` visibility rules (same lines gated as today; when they print, they use the new format)

## How it is produced

C++: all addon logging goes through `Logger`. Call sites pass system + message, for example `Logger::Info("Auth", "Login successful")`. `Logger` is the only place that concatenates `[GodotEOS] ` + system + `: ` + message.

- `Logger::Error` → `printerr` (prefixed line)
- `Logger::Warning` / `Info` / `Verbose` → `print` (prefixed line)

Do not use `WARN_PRINT`, `ERR_PRINT`, `push_warning`, or `push_error` for addon messages. Those make Godot emit `WARNING:` / `ERROR:` and an `at:` line, so the log would not start with `[GodotEOS]`.

GDScript: `epic_os.gd` and `plugin.gd` must not call `print` / `print_rich` / `printerr` directly. Use a helper, e.g. `_log(message)` / `_log_error(message)`, that always prints `[GodotEOS] EpicOS: …`. Drop `print_rich` color wrappers so `godot.log` stays plain text matching the format.

EOS callback: one callback path. Format with `Logger` using the EOS category as the system name:

```
[GodotEOS] LogEOS: Connect User Logged In
```

If both `GodotEOS::logging_callback` and `platform_logging_callback` (or Auth’s copy) are registered, keep a single callback so SDK lines are not printed twice.

## Existing Logger

`godot_eos_extension/src/Utils/Logger` already exists and is unused. Extend it to take a system name and own the prefix. Wire every current `UtilityFunctions::print` / `printerr` / `WARN_PRINT` / `push_warning` in plugin C++ through it.

`EpicOS.set_debug_mode()` stays a GDScript-only gate for the verbose wrapper prints it already controls. Do not require C++ `Logger` debug gating in this change.

## Support ask

Ask users for `%APPDATA%\Godot\app_userdata\<game name>\logs\godot.log` (this demo: `GodotEOS Plugin`) and for all lines containing `[GodotEOS]`.
