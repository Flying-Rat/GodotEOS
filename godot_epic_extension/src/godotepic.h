#ifndef GODOTEPIC_H
#define GODOTEPIC_H

#include <godot_cpp/classes/sprite2d.hpp>

namespace godot {

class GodotEpic : public Object {
	GDCLASS(GodotEpic, Object)

private:
	double time_passed;

protected:
	static void _bind_methods();

public:
	GodotEpic();
	~GodotEpic();
};

}

#endif