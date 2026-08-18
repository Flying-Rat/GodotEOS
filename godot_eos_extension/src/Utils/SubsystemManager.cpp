#include "SubsystemManager.h"
#include "Logger.h"
#include "Platform/IPlatformSubsystem.h"
#include <cstdint>
#include <vector>

namespace godot {

// Initialize static member
SubsystemManager* SubsystemManager::instance = nullptr;

SubsystemManager* SubsystemManager::GetInstance() {
    if (!instance) {
        instance = new SubsystemManager();
    }
    return instance;
}

bool SubsystemManager::InitializeAll() {
    if (initialized) {
        Logger::Info("Core", "Already initialized");
        return true;
    }

    if (subsystems.empty()) {
        Logger::Info("Core", "No subsystems registered");
        initialized = true; // Empty is still "initialized"
        return true;
    }

    Logger::Info("Core", "Initializing " + String::num_int64(static_cast<int64_t>(subsystems.size())) + " subsystems...");

    // Initialize subsystems in registration order
    for (auto& [type_index, subsystem] : subsystems) {
        const std::string& name = subsystem_names[type_index];

        Logger::Info("Core", String("Initializing subsystem: ") + name.c_str());

        if (!subsystem->Init()) {
            Logger::Error("Core", String("Failed to initialize subsystem: ") + name.c_str());

            // Shutdown all previously initialized subsystems
            ShutdownAll();
            return false;
        }

        Logger::Info("Core", String("Successfully initialized subsystem: ") + name.c_str());
    }

    initialized = true;
    Logger::Info("Core", "All subsystems initialized successfully");
    return true;
}

void SubsystemManager::TickAll(float delta_time) {
    if (!initialized) {
        return; // Silently do nothing if not initialized
    }

    // Tick all subsystems
    for (auto& [type_index, subsystem] : subsystems) {
        subsystem->Tick(delta_time);
    }
}

void SubsystemManager::ShutdownAll() {
    if (!initialized && subsystems.empty()) {
        return; // Nothing to do
    }

    Logger::Info("Core", "Shutting down subsystems...");

    // Shutdown subsystems in dependency order - dependent subsystems first
    // Order: FriendsSubsystem, LeaderboardsSubsystem, AchievementsSubsystem, AuthenticationSubsystem, UserInfoSubsystem, then PlatformSubsystem last

    // List of subsystem names in shutdown order
    std::vector<std::string> shutdown_order = {
        "FriendsSubsystem",
        "LeaderboardsSubsystem",
        "AchievementsSubsystem",
        "AuthenticationSubsystem",
        "UserInfoSubsystem",
        "PlatformSubsystem"
    };

    // Shutdown in specified order
    for (const std::string& name : shutdown_order) {
        for (auto& [type_index, subsystem] : subsystems) {
            if (subsystem_names[type_index] == name) {
                Logger::Info("Core", String("Shutting down subsystem: ") + name.c_str());
                subsystem->Shutdown();
                break;
            }
        }
    }

    // Shutdown any remaining subsystems (in case we missed any)
    for (auto& [type_index, subsystem] : subsystems) {
        const std::string& name = subsystem_names[type_index];
        bool already_shutdown = false;
        for (const std::string& shutdown_name : shutdown_order) {
            if (name == shutdown_name) {
                already_shutdown = true;
                break;
            }
        }
        if (!already_shutdown) {
            Logger::Info("Core", String("Shutting down remaining subsystem: ") + name.c_str());
            subsystem->Shutdown();
        }
    }

    initialized = false;
    Logger::Info("Core", "All subsystems shut down");
}

bool SubsystemManager::IsHealthy() const {
    if (!initialized) {
        return false;
    }

    // Check if platform subsystem is healthy
    auto platform_subsystem = GetSubsystem<IPlatformSubsystem>();
    return platform_subsystem && platform_subsystem->IsOnline();
}

void SubsystemManager::ResetForReinitialization() {
    if (initialized && !IsHealthy()) {
        Logger::Info("Core", "Resetting for reinitialization - shutting down unhealthy subsystems");
        ShutdownAll();
    }
}

} // namespace godot