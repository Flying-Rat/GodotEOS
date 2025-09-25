#pragma once

#include <godot_cpp/variant/string.hpp>

namespace godot {

/**
 * @brief Base interface for all subsystems in the GodotEpic extension.
 * 
 * All subsystems must inherit from this interface and implement the required
 * lifecycle methods. The SubsystemManager uses this interface to manage
 * subsystem initialization, ticking, and shutdown.
 */
class ISubsystem {
public:
    virtual ~ISubsystem() = default;

    /**
     * @brief Initialize the subsystem.
     * @return true if initialization succeeded, false otherwise.
     * 
     * This method is called once during subsystem initialization.
     * If it returns false, the subsystem manager will fail initialization
     * and shut down all previously initialized subsystems.
     */
    virtual bool Init() = 0;

    /**
     * @brief Tick/update the subsystem.
     * @param delta_time Time elapsed since last tick in seconds.
     * 
     * This method is called regularly (typically every frame) to allow
     * the subsystem to process updates, handle callbacks, etc.
     * Only called if the subsystem was successfully initialized.
     */
    virtual void Tick(float delta_time) = 0;

    /**
     * @brief Shutdown the subsystem.
     * 
     * This method is called during subsystem shutdown to allow
     * cleanup of resources. Called in reverse order of initialization.
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Get the human-readable name of this subsystem.
     * @return C-string containing the subsystem name.
     * 
     * Used for logging and debugging purposes.
     */
    virtual const char* GetSubsystemName() const = 0;
};

} // namespace godot