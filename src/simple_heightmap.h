#pragma once

#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/height_map_shape3d.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>

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

		void set_mesh_width(const real_t value);
		void set_mesh_depth(const real_t value);
		void set_mesh_resolution(const real_t value);

		void set_data_resolution(int value);

		[[nodiscard]] real_t get_mesh_width() const { return mesh_width; }
		[[nodiscard]] real_t get_mesh_depth() const { return mesh_depth; }
		[[nodiscard]] real_t get_mesh_resolution() const { return mesh_resolution; }

		[[nodiscard]] int get_data_resolution() const { return data_resolution; }

		real_t sample_height(const Vector3& local_position) const;

		Vector<Vector2i> get_pixel_coordinates_in_range(const Vector3& local_position, const real_t range_radius) const;
		
		Vector2i local_position_to_pixel_coordinates(const Vector3& local_position) const;

		Vector3 pixel_coordinates_to_local_position(const Vector2i& pixel_coordinates) const;
	
	private:

		[[nodiscard]] int get_desired_heightmap_data_size() const { return data_resolution * data_resolution; }

		real_t get_height_at(const Vector2i& p) const;

		void generate_default_heightmap_data();
		void generate_default_splatmap_data();

		real_t mesh_width = 4.0;
		real_t mesh_depth = 4.0;
		real_t mesh_resolution = 1.0; // Mesh density
		
		int data_resolution = 16; // Size of the heightmap image (e.g., 64x64)
		PackedRealArray heightmap_data;
		PackedColorArray splatmap_data;

		RID mesh_id;
		PackedVector3Array vertex_positions;
		PackedVector2Array vertex_uvs;
		PackedVector3Array vertex_normals;
		PackedInt32Array indices;

		StaticBody3D* collision_body = nullptr;
		CollisionShape3D* collision_shape_node = nullptr;
		Ref<HeightMapShape3D> collision_shape = nullptr;
	};
}