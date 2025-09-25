#include "IPlatform.h"

namespace godot {

// Single-definition of the IPlatform static instance and its accessors.
// Keeping these in this .cpp ensures only one translation unit defines them.
IPlatform* IPlatform::_instance = nullptr;

IPlatform* IPlatform::get() {
	return IPlatform::_instance;
}

void IPlatform::set(IPlatform* instance) {
	IPlatform::_instance = instance;
}

} // namespace godot