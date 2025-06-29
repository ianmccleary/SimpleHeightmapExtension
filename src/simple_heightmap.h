#pragma once

#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/height_map_shape3d.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>

class SimpleHeightmap : public godot::GeometryInstance3D
{
	GDCLASS(SimpleHeightmap, godot::GeometryInstance3D)

protected:
	static void _bind_methods();

public:
	SimpleHeightmap() = default;
	~SimpleHeightmap() = default;

	void _enter_tree() override;
	void _exit_tree() override;

	void rebuild();

	void set_mesh_size(const godot::real_t value);
	void set_mesh_resolution(const godot::real_t value);

	void set_image_size(int value);

	[[nodiscard]] godot::real_t get_mesh_size() const { return mesh_size; }
	[[nodiscard]] godot::real_t get_mesh_resolution() const { return mesh_resolution; }

	[[nodiscard]] int get_image_size() const { return image_size; }

	godot::Ref<godot::Image> get_heightmap_image() const { return heightmap; }
	godot::Ref<godot::Image> get_splatmap_image() const { return splatmap; }
	
	godot::Vector2 local_position_to_image_position(const godot::Vector3& local_position) const;
	godot::Vector2 global_position_to_image_position(const godot::Vector3& global_position) const;

	godot::Vector3 image_position_to_local_position(const godot::Vector2& image_position) const;
	godot::Vector3 image_position_to_global_position(const godot::Vector2& image_position) const;

private:
	void generate_default_heightmap_data();
	void generate_default_splatmap_data();

	static godot::Color bilinear_sample(const godot::Ref<godot::Image>& image, const godot::Vector2& point);

	godot::real_t mesh_size = 4.0; // Mesh size
	godot::real_t mesh_resolution = 1.0; // Mesh density
	
	int image_size = 16; // Size of the heightmap image (e.g., 64x64)
	godot::Ref<godot::Image> heightmap;
	godot::Ref<godot::Image> splatmap;

	godot::RID mesh_id;
	godot::PackedVector3Array vertex_positions;
	godot::PackedVector2Array vertex_uvs;
	godot::PackedVector3Array vertex_normals;
	godot::PackedInt32Array indices;

	godot::StaticBody3D* collision_body = nullptr;
	godot::CollisionShape3D* collision_shape_node = nullptr;
	godot::Ref<godot::HeightMapShape3D> collision_shape = nullptr;
};