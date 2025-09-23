#ifndef GODOTEPIC_H
#define GODOTEPIC_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <eos_sdk.h>
#include <eos_types.h>
#include <eos_init.h>
#include <eos_logging.h>

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

class GodotEpic : public Object {
	GDCLASS(GodotEpic, Object)

private:
	static GodotEpic* instance;
	static EOS_HPlatform platform_handle;
	static bool is_initialized;
	
	double time_passed;
	
	// EOS logging callback
	static void EOS_CALL logging_callback(const EOS_LogMessage* message);
	
	// Helper methods
	EpicInitOptions _dict_to_init_options(const Dictionary& options_dict);
	bool _validate_init_options(const EpicInitOptions& options);

protected:
	static void _bind_methods();

public:
	GodotEpic();
	~GodotEpic();
	
	// Singleton access
	static GodotEpic* get_singleton();
	
	// Platform management
	bool initialize_platform(const Dictionary& options);
	void shutdown_platform();
	void tick();
	
	// Status methods
	bool is_platform_initialized() const;
	EOS_HPlatform get_platform_handle() const;
};

}

#endif