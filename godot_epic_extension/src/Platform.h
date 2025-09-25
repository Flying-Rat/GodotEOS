#pragma once

#include "IPlatform.h"
#include "../eos_sdk/Include/eos_achievements_types.h"
#include "EpicInitOptions.h"

namespace godot {

class Platform : public IPlatform {
public:
    Platform();
    ~Platform() override;

    bool initialize(const EpicInitOptions& options) override;
    void shutdown() override;
    // Process any EOS platform background work. Should be called regularly (eg. in the engine tick).
    void tick() override;
    EOS_HPlatform get_platform_handle() const override;
    bool is_initialized() const override;

private:
    EOS_HPlatform platform_handle;
    bool initialized;
};

} // namespace godot