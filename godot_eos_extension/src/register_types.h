#ifndef GODOTEOS_REGISTER_TYPES_H
#define GODOTEOS_REGISTER_TYPES_H

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_godoteos_module(ModuleInitializationLevel p_level);
void uninitialize_godoteos_module(ModuleInitializationLevel p_level);

#endif // GDEXAMPLE_REGISTER_TYPES_H