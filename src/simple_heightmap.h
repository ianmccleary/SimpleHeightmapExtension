#pragma once

#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/height_map_shape3d.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>
#include "square_image.h"

namespace godot
{
	using HeightmapImage = SquareImage<real_t>;
	using SplatmapImage = SquareImage<Color>;

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

		void set_mesh_size(const real_t value);
		void set_mesh_resolution(const real_t value);

		void set_image_size(int value);

		[[nodiscard]] real_t get_mesh_size() const { return mesh_size; }
		[[nodiscard]] real_t get_mesh_resolution() const { return mesh_resolution; }

		[[nodiscard]] int get_image_size() const { return image_size; }

		HeightmapImage& get_heightmap_image() { return heightmap_image; }
		SplatmapImage& get_splatmap_image() { return splatmap_image; }

		const HeightmapImage& get_heightmap_image() const { return heightmap_image; }
		const SplatmapImage& get_splatmap_image() const { return splatmap_image; }
		
		Vector2 local_position_to_image_position(const Vector3& local_position) const;
		Vector2 global_position_to_image_position(const Vector3& global_position) const;

		Vector3 image_position_to_local_position(const Vector2& image_position) const;
		Vector3 image_position_to_global_position(const Vector2& image_position) const;
	
	private:

		void generate_default_heightmap_data();
		void generate_default_splatmap_data();

		real_t mesh_size = 4.0; // Mesh size
		real_t mesh_resolution = 1.0; // Mesh density
		
		int image_size = 16; // Size of the heightmap image (e.g., 64x64)
		HeightmapImage heightmap_image;
		SplatmapImage splatmap_image;

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