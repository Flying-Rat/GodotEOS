#pragma once

#include <godot_cpp/variant/string.hpp>

namespace godot {

// Configuration structure for EOS initialization
struct EpicInitOptions {
    String product_name = "GodotEpic";
    String product_version = "1.0.0";
    String product_id = "";
    String sandbox_id = "";
    String deployment_id = "";
    String client_id = "";
    String client_secret = "";
    String encryption_key = "";
};

}