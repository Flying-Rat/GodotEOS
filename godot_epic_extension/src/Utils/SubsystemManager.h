#pragma once

#include "ISubsystem.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <typeindex>
#include <unordered_map>
#include <memory>
#include <string>

namespace godot {

/**
 * @brief Singleton manager for all subsystems in the GodotEpic extension.
 * 
 * The SubsystemManager handles the lifecycle of all subsystems:
 * - Registration of subsystem implementations
 * - Initialization in registration order
 * - Regular ticking of all initialized subsystems
 * - Shutdown in reverse initialization order
 * 
 * Subsystems are identified by their interface type using std::type_index.
 * This allows for type-safe Get<T>() calls throughout the codebase.
 */
class SubsystemManager {
private:
    // Singleton instance
    static SubsystemManager* instance;

    // Storage for registered subsystems
    std::unordered_map<std::type_index, std::unique_ptr<ISubsystem>> subsystems;
    std::unordered_map<std::type_index, std::string> subsystem_names;

    // Initialization state
    bool initialized = false;

    // Private constructor for singleton
    SubsystemManager() = default;

public:
    // Prevent copying
    SubsystemManager(const SubsystemManager&) = delete;
    SubsystemManager& operator=(const SubsystemManager&) = delete;

    /**
     * @brief Get the singleton instance of the SubsystemManager.
     * @return Pointer to the SubsystemManager instance.
     * 
     * Uses lazy initialization - creates the instance on first access.
     */
    static SubsystemManager* GetInstance();

    /**
     * @brief Register a subsystem implementation.
     * @tparam InterfaceType The subsystem interface class (must inherit from ISubsystem).
     * @tparam ImplType The concrete implementation class (must inherit from InterfaceType).
     * @param name Human-readable name for the subsystem (used for logging).
     * 
     * Registers a subsystem by creating an instance of ImplType and associating it
     * with InterfaceType. The subsystem will be initialized when InitializeAll() is called.
     * 
     * Example: RegisterSubsystem<IPlatform, Platform>("Platform");
     */
    template<typename InterfaceType, typename ImplType>
    void RegisterSubsystem(const std::string& name) {
        static_assert(std::is_base_of_v<ISubsystem, InterfaceType>, "InterfaceType must inherit from ISubsystem");
        static_assert(std::is_base_of_v<InterfaceType, ImplType>, "ImplType must inherit from InterfaceType");

        auto type_index = std::type_index(typeid(InterfaceType));

        // Check for duplicate registration
        if (subsystems.find(type_index) != subsystems.end()) {
            UtilityFunctions::printerr("Subsystem already registered for interface: ", name.c_str());
            return;
        }

        subsystems[type_index] = std::make_unique<ImplType>();
        subsystem_names[type_index] = name;

        UtilityFunctions::print("Registered subsystem: ", name.c_str());
    }

    /**
     * @brief Get a subsystem instance by its interface type.
     * @tparam T The subsystem interface type to retrieve.
     * @return Pointer to the subsystem instance, or nullptr if not registered.
     * 
     * Returns a typed pointer to the requested subsystem. Returns nullptr
     * if the subsystem is not registered or not of the requested type.
     * 
     * Example: GetSubsystem<IPlatform>() returns IPlatform*
     */
    template<typename T>
    T* GetSubsystem() {
        static_assert(std::is_base_of_v<ISubsystem, T>, "T must inherit from ISubsystem");

        auto type_index = std::type_index(typeid(T));
        auto it = subsystems.find(type_index);
        if (it != subsystems.end()) {
            return static_cast<T*>(it->second.get());
        }
        return nullptr;
    }

    /**
     * @brief Get a subsystem instance by its interface type (const version).
     * @tparam T The subsystem interface type to retrieve.
     * @return Pointer to the subsystem instance, or nullptr if not registered.
     * 
     * Returns a typed pointer to the requested subsystem. Returns nullptr
     * if the subsystem is not registered or not of the requested type.
     * 
     * Example: GetSubsystem<IPlatform>() returns IPlatform*
     */
    template<typename T>
    const T* GetSubsystem() const {
        static_assert(std::is_base_of_v<ISubsystem, T>, "T must inherit from ISubsystem");

        auto type_index = std::type_index(typeid(T));
        auto it = subsystems.find(type_index);
        if (it != subsystems.end()) {
            return static_cast<const T*>(it->second.get());
        }
        return nullptr;
    }

    /**
     * @brief Initialize all registered subsystems.
     * @return true if all subsystems initialized successfully, false otherwise.
     * 
     * Calls Init() on each registered subsystem in registration order.
     * If any subsystem fails to initialize, stops and shuts down all
     * previously initialized subsystems, then returns false.
     */
    bool InitializeAll();

    /**
     * @brief Tick all initialized subsystems.
     * @param delta_time Time elapsed since last tick in seconds.
     * 
     * Calls Tick() on each initialized subsystem. Does nothing if
     * subsystems are not initialized.
     */
    void TickAll(float delta_time);

    /**
     * @brief Shutdown all subsystems.
     * 
     * Calls Shutdown() on all subsystems in reverse initialization order,
     * then clears the initialized state. Safe to call multiple times.
     */
    void ShutdownAll();

    /**
     * @brief Check if subsystems are initialized.
     * @return true if InitializeAll() was called successfully.
     */
    bool IsInitialized() const { return initialized; }

    /**
     * @brief Get the number of registered subsystems.
     * @return Number of subsystems currently registered.
     */
    size_t GetSubsystemCount() const { return subsystems.size(); }

    /**
     * @brief Check if the subsystem manager is healthy (initialized and platform is online).
     * @return true if initialized and platform subsystem is healthy, false otherwise.
     */
    bool IsHealthy() const;

    /**
     * @brief Reset the subsystem manager for reinitialization if unhealthy.
     * 
     * If the manager is initialized but unhealthy, shuts down all subsystems
     * to prepare for reinitialization. Does nothing if not initialized or healthy.
     */
    void ResetForReinitialization();
};

// Global convenience function for subsystem access
/**
 * @brief Global function to get a subsystem instance.
 * @tparam T The subsystem interface type to retrieve.
 * @return Pointer to the subsystem instance, or nullptr if not registered.
 * 
 * Convenience function that forwards to SubsystemManager::GetInstance()->GetSubsystem<T>().
 * Can be used anywhere in the codebase for easy subsystem access.
 * 
 * Example: Get<IPlatform>()->IsOnline();
 */
template<typename T>
T* Get() {
    static_assert(std::is_base_of_v<ISubsystem, T>, "T must inherit from ISubsystem");
    return SubsystemManager::GetInstance()->GetSubsystem<T>();
}

} // namespace godot