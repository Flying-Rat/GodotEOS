# GodotEOS log prefix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Every addon log line starts with `[GodotEOS] <System>: <message>` so game developers and support can identify plugin output.

**Architecture:** `Logger` owns the prefix. C++ call sites pass a short system name plus the message. GDScript addon files use `_log` / `_log_error`. EOS SDK callback uses the EOS category as the system name. No `WARN_PRINT` / `push_warning` / `push_error`.

**Tech Stack:** Godot 4.3+ GDExtension (C++ via godot-cpp), GDScript addon wrapper.

## Global Constraints

- Line format is exactly `[GodotEOS] <System>: <message>` with `[GodotEOS]` first.
- System names: `EpicOS`, `Auth`, `Platform`, `UserInfo`, `Friends`, `Achievements`, `Leaderboards`, `Core`; EOS callback uses category as-is or `EOS` if empty.
- Do not use `GodotEOS` as a system name.
- Do not use `WARN_PRINT`, `ERR_PRINT`, `push_warning`, or `push_error` for addon messages.
- Demo scenes out of scope. Do not delete existing messages. Do not change `set_debug_mode` gating. Do not add a separate log file.
- Do not commit unless the user asks.

---

### Task 1: Logger API

**Files:**
- Create: `godot_eos_extension/src/Utils/Logger.h`
- Create: `godot_eos_extension/src/Utils/Logger.cpp`
- Already compiled via `Glob("src/Utils/*.cpp")` in `godot_eos_extension/SConstruct`

**Interfaces:**
- Consumes: `godot::String`, `godot::UtilityFunctions`
- Produces:
  - `static String Logger::Format(const String& system, const String& message)`
  - `static void Logger::Error(const String& system, const String& message)` → `printerr(Format(...))`
  - `static void Logger::Warning(const String& system, const String& message)` → `print(Format(...))`
  - `static void Logger::Info(const String& system, const String& message)` → `print(Format(...))`
  - `static void Logger::Verbose(const String& system, const String& message)` → `print(Format(...))`

- [ ] **Step 1: Add Logger.h / Logger.cpp**

```cpp
// Logger.h
#pragma once
#include <godot_cpp/variant/string.hpp>
namespace godot {
class Logger {
public:
    static String Format(const String& system, const String& message);
    static void Error(const String& system, const String& message);
    static void Warning(const String& system, const String& message);
    static void Info(const String& system, const String& message);
    static void Verbose(const String& system, const String& message);
};
}
```

```cpp
// Logger.cpp
String Logger::Format(const String& system, const String& message) {
    return String("[GodotEOS] ") + system + ": " + message;
}
```

`Format("Auth", "Login successful")` must equal `[GodotEOS] Auth: Login successful`. Empty EOS category callers pass `"EOS"`.

---

### Task 2: EOS callback + Platform + Core

**Files:**
- Modify: `godot_eos_extension/src/Platform/PlatformSubsystem.cpp`
- Modify: `godot_eos_extension/src/Utils/SubsystemManager.h`
- Modify: `godot_eos_extension/src/Utils/SubsystemManager.cpp`
- Modify: `godot_eos_extension/src/Authentication/AuthenticationSubsystem.h` (remove unused `logging_callback`)
- Modify: `godot_eos_extension/src/Authentication/AuthenticationSubsystem.cpp` (remove unused `logging_callback`)

**Interfaces:**
- Consumes: `Logger::Info/Error/Warning`, EOS `Category` / `Message`
- Produces: single EOS callback through `platform_logging_callback`

- [ ] **Step 1: EOS callback**

```cpp
String category = message->Category ? String::utf8(message->Category) : "EOS";
if (category.is_empty()) {
    category = "EOS";
}
Logger::Info(category, String::utf8(message->Message)); // Error/Warning by EOS level
```

- [ ] **Step 2: Replace Platform and Core prints; include Logger.h**
- [ ] **Step 3: Grep src (excluding godot-cpp) for remaining `WARN_PRINT` / `push_warning` / raw `UtilityFunctions::print` in these files**

---

### Task 3: Subsystem and facade logs

**Files:**
- Modify: `godot_eos_extension/src/Authentication/AuthenticationSubsystem.cpp`
- Modify: `godot_eos_extension/src/UserInfo/UserInfoSubsystem.cpp`
- Modify: `godot_eos_extension/src/Friends/FriendsSubsystem.cpp`
- Modify: `godot_eos_extension/src/Achievements/AchievementsSubsystem.cpp`
- Modify: `godot_eos_extension/src/Leaderboards/LeaderboardsSubsystem.cpp`
- Modify: `godot_eos_extension/src/godotepic.cpp`

**Interfaces:**
- Consumes: `Logger::{Error,Warning,Info,Verbose}(system, message)`
- Produces: all C++ addon logs in the spec format

Strip existing prefixes (`AuthenticationSubsystem: `, `GodotEOS: `, `GodotEOS::... - `) from the message. Map facade methods: login/logout → `Auth`; init/shutdown/destructor/singleton → `Platform` or `Core`; friends/userinfo/achievements/leaderboards → matching system.

- [ ] **Step 1: Replace every print/printerr/push_warning/WARN_PRINT in these files**
- [ ] **Step 2: Grep `godot_eos_extension/src` (not godot-cpp) — zero remaining raw prints except inside `Logger.cpp`**

---

### Task 4: GDScript addon helpers

**Files:**
- Modify: `addons/godoteos/epic_os.gd`
- Modify: `addons/godoteos/plugin.gd`

**Interfaces:**
- Consumes: none
- Produces: `_log(message: String)` → `print("[GodotEOS] EpicOS: ", message)`; `_log_error(message: String)` → `printerr("[GodotEOS] EpicOS: ", message)`

- [ ] **Step 1: Add helpers; replace all `print` / `print_rich` / `printerr`. Keep `set_debug_mode` gates. Do not touch demo scripts.**
- [ ] **Step 2: Grep `addons/godoteos` for raw `print(` / `print_rich(` / `printerr(` — only the helpers remain**
