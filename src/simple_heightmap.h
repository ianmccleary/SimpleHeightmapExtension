#pragma once

#include <godot_cpp/classes/geometry_instance3d.hpp>

namespace godot
{
	class SimpleHeightmap : public GeometryInstance3D
	{
		GDCLASS(SimpleHeightmap, GeometryInstance3D)
	
	protected:
		static void _bind_methods();

	public:
		SimpleHeightmap() = default;
		~SimpleHeightmap() = default;
	};
}