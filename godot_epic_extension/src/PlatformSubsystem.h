#pragma once

#include "IPlatformSubsystem.h"
#include "EpicInitOptions.h"
#include "../eos_sdk/Include/eos_types.h"
#include <godot_cpp/variant/string.hpp>

namespace godot {

/**
 * @brief Platform subsystem implementation.
 *
 * Manages EOS SDK platform initialization, ticking, and provides access
 * to the EOS platform handle for other subsystems.
 */
class PlatformSubsystem : public IPlatformSubsystem {
public:
    PlatformSubsystem();
    ~PlatformSubsystem() override;

    // ISubsystem interface
    bool Init() override;
    void Tick(float delta_time) override;
    void Shutdown() override;
    const char* GetSubsystemName() const override { return "Platform"; }

    // IPlatformSubsystem interface
    bool InitializePlatform(const EpicInitOptions& options) override;
    void* GetPlatformHandle() const override;
    bool IsOnline() const override;
    void SetLogLevel(int level) override;

private:
    EOS_HPlatform platform_handle;
    bool initialized;
    bool online;
};

} // namespace godot