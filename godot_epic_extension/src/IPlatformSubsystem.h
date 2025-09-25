#pragma once

#include "ISubsystem.h"
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
     * @param product_name Product name for EOS SDK.
     * @param product_version Product version for EOS SDK.
     * @param product_id Product ID from Epic Developer Portal.
     * @param sandbox_id Sandbox ID from Epic Developer Portal.
     * @param deployment_id Deployment ID from Epic Developer Portal.
     * @param client_id Client ID from Epic Developer Portal.
     * @param client_secret Client secret from Epic Developer Portal.
     * @param encryption_key Encryption key for data protection.
     * @return true if platform initialized successfully.
     */
    virtual bool InitializePlatform(
        const String& product_name,
        const String& product_version,
        const String& product_id,
        const String& sandbox_id,
        const String& deployment_id,
        const String& client_id,
        const String& client_secret,
        const String& encryption_key
    ) = 0;

    /**
     * @brief Get the EOS platform handle.
     * @return EOS platform handle, or nullptr if not initialized.
     */
    virtual void* GetPlatformHandle() const = 0;

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