#include "SubsystemManager.h"

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
        UtilityFunctions::print("SubsystemManager: Already initialized");
        return true;
    }

    if (subsystems.empty()) {
        UtilityFunctions::print("SubsystemManager: No subsystems registered");
        initialized = true; // Empty is still "initialized"
        return true;
    }

    UtilityFunctions::print("SubsystemManager: Initializing ", subsystems.size(), " subsystems...");

    // Initialize subsystems in registration order
    for (auto& [type_index, subsystem] : subsystems) {
        const std::string& name = subsystem_names[type_index];

        UtilityFunctions::print("SubsystemManager: Initializing subsystem: ", name.c_str());

        if (!subsystem->Init()) {
            UtilityFunctions::printerr("SubsystemManager: Failed to initialize subsystem: ", name.c_str());

            // Shutdown all previously initialized subsystems
            ShutdownAll();
            return false;
        }

        UtilityFunctions::print("SubsystemManager: Successfully initialized subsystem: ", name.c_str());
    }

    initialized = true;
    UtilityFunctions::print("SubsystemManager: All subsystems initialized successfully");
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

    UtilityFunctions::print("SubsystemManager: Shutting down subsystems...");

    // Shutdown subsystems in reverse order (though order doesn't really matter for shutdown)
    for (auto& [type_index, subsystem] : subsystems) {
        const std::string& name = subsystem_names[type_index];
        UtilityFunctions::print("SubsystemManager: Shutting down subsystem: ", name.c_str());
        subsystem->Shutdown();
    }

    initialized = false;
    UtilityFunctions::print("SubsystemManager: All subsystems shut down");
}

} // namespace godot