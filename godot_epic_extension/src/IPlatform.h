#pragma once

#include "EpicInitOptions.h"
#include "../eos_sdk/Include/eos_types.h"
#include <memory>

namespace godot {

class IPlatform {
public:
    virtual ~IPlatform() = default;

    // Static accessors for the singleton instance. Definitions are provided
    // in a single translation unit to avoid multiple-definition errors.
    static IPlatform* get();
    static void set(IPlatform* instance);


    virtual bool initialize(const EpicInitOptions& options) = 0;
    virtual void shutdown() = 0;
    virtual EOS_HPlatform get_platform_handle() const = 0;
    // Called regularly (eg. once per frame) to allow the EOS platform to process internal work.
    virtual void tick() = 0;
    virtual bool is_initialized() const = 0;

private:
    // Internal static instance pointer. A single translation unit must provide
    // the definition; Platform.cpp defines it.
    static IPlatform* _instance;
};

} // namespace godot