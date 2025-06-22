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

		void _enter_tree() override;
		void _exit_tree() override;

		void rebuild();

		void set_heightmap_width(const real_t value);
		void set_heightmap_depth(const real_t value);
		void set_distance_between_vertices(const real_t value);

		[[nodiscard]] real_t get_heightmap_width() const { return heightmap_width; }
		[[nodiscard]] real_t get_heightmap_depth() const { return heightmap_depth; }
		[[nodiscard]] real_t get_distance_between_vertices() const { return distance_between_vertices; }
	
	private:
		real_t heightmap_width = 4.0;
		real_t heightmap_depth = 4.0;
		real_t distance_between_vertices = 1.0;

		RID mesh_id;
		PackedVector3Array vertex_positions;
		PackedVector2Array vertex_uvs;
		PackedVector3Array vertex_normals;
		PackedInt32Array indices;
	};
}