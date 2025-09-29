#pragma once

#include "ISubsystem.h"
#include "EpicInitOptions.h"
#include "../eos_sdk/Include/eos_types.h"
#include <godot_cpp/variant/string.hpp>

namespace godot {

/**
 * @brief Platform subsystem interface.
 * 
 * Handles EOS SDK platform initialization, ticking, and basic platform services.
 * This is the foundation subsystem that other subsystems may depend on.
 */
class IPlatformSubsystem : public ISubsystem {
public:
    /**
     * @brief Initialize the EOS platform with the given options.
     * @param options EpicInitOptions struct containing all initialization parameters.
     * @return true if platform initialized successfully.
     */
    virtual bool InitializePlatform(const EpicInitOptions& options) = 0;

    /**
     * @brief Get the EOS platform handle.
     * @return EOS platform handle, or nullptr if not initialized.
     */
    virtual EOS_HPlatform GetPlatformHandle() const = 0;

    /**
     * @brief Check if the platform is online.
     * @return true if platform is connected and online.
     */
    virtual bool IsOnline() const = 0;

    /**
     * @brief Set the logging level for EOS SDK.
     * @param level Log level (0=Off, 1=Error, 2=Warning, 3=Info, 4=Verbose).
     */
    virtual void SetLogLevel(int level) = 0;
};

} // namespace godot