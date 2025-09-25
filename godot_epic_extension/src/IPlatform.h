#pragma once

#include "EpicInitOptions.h"
#include "../eos_sdk/Include/eos_types.h"
#include <memory>

namespace godot {

class IPlatform {
public:
    virtual ~IPlatform() = default;
    virtual bool initialize(const EpicInitOptions& options) = 0;
    virtual void shutdown() = 0;
    virtual EOS_HPlatform get_platform_handle() const = 0;
    virtual bool is_initialized() const = 0;
};

} // namespace godot