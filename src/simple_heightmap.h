#pragma once

#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/texture2d.hpp>

class SimpleHeightmap : public godot::GeometryInstance3D
{
	GDCLASS(SimpleHeightmap, godot::GeometryInstance3D)

protected:
	static void _bind_methods();

public:
	SimpleHeightmap();
	~SimpleHeightmap();
	
	void _notification(int32_t what);

	enum RebuildFlags : uint8_t
	{
		REBUILD_NONE = 0x0,
		REBUILD_HEIGHTMAP = 0x1,
		REBUILD_SPLATMAP = 0x2,
		REBUILD_UV = 0x4,
		REBUILD_ALL = REBUILD_HEIGHTMAP | REBUILD_SPLATMAP | REBUILD_UV
	};

	void rebuild(RebuildFlags flags);

	void set_mesh_size(const godot::real_t value);
	void set_mesh_resolution(const godot::real_t value);
	void set_image_size(int value);
	void set_texture_size(const godot::real_t value);
	void set_heightmap_image(const godot::Ref<godot::Image>& new_heightmap);
	void set_splatmap_image(const godot::Ref<godot::Image>& new_splatmap);
	void set_texture_1(const godot::Ref<godot::Texture2D>& new_texture);
	void set_texture_2(const godot::Ref<godot::Texture2D>& new_texture);
	void set_texture_3(const godot::Ref<godot::Texture2D>& new_texture);
	void set_texture_4(const godot::Ref<godot::Texture2D>& new_texture);
	void set_collider_layer(uint32_t layer);
	void set_collider_mask(uint32_t mask);
	void set_collider_priority(float priority);

	[[nodiscard]] godot::real_t get_mesh_size() const { return mesh_size; }
	[[nodiscard]] godot::real_t get_half_mesh_size() const { return mesh_size * static_cast<godot::real_t>(0.5); }
	[[nodiscard]] godot::real_t get_mesh_resolution() const { return mesh_resolution; }
	[[nodiscard]] int get_image_size() const { return image_size; }
	[[nodiscard]] godot::real_t get_texture_size() const { return texture_size; }
	[[nodiscard]] godot::Ref<godot::Image> get_heightmap_image() const { return heightmap; }
	[[nodiscard]] godot::Ref<godot::Image> get_splatmap_image() const { return splatmap; }
	[[nodiscard]] godot::Ref<godot::Texture2D> get_texture_1() const { return texture_1; }
	[[nodiscard]] godot::Ref<godot::Texture2D> get_texture_2() const { return texture_2; }
	[[nodiscard]] godot::Ref<godot::Texture2D> get_texture_3() const { return texture_3; }
	[[nodiscard]] godot::Ref<godot::Texture2D> get_texture_4() const { return texture_4; }
	[[nodiscard]] uint32_t get_collider_layer() const { return collider_layer; }
	[[nodiscard]] uint32_t get_collider_mask() const { return collider_mask; }
	[[nodiscard]] float get_collider_priority() const { return collider_priority; }
	
	godot::Vector2 local_position_to_image_position(const godot::Vector3& local_position) const;
	godot::Vector2 global_position_to_image_position(const godot::Vector3& global_position) const;

	godot::Vector3 image_position_to_local_position(const godot::Vector2& image_position) const;
	godot::Vector3 image_position_to_global_position(const godot::Vector2& image_position) const;

#ifdef TOOLS_ENABLED
	uint32_t get_collider_shape_data_size() const { return get_vertices_per_side(); }
	const godot::PackedRealArray& get_collider_shape_data() const { return collider_shape_data; }
	godot::real_t get_collider_size() const { return static_cast<godot::real_t>(get_quads_per_side()); } // Each quad is 1 unit wide/deep, total size is all quads
#endif // TOOLS_ENABLED

private:
	static void initialize_image(const godot::Ref<godot::Image>& image, godot::Image::Format format, int32_t size, godot::Color default_color = godot::Color());
	static godot::Color bilinear_sample(const godot::Ref<godot::Image>& image, const godot::Vector2& point);
	
	void update_material_texture_parameter(const char* parameter_name, const godot::Ref<godot::Texture2D>& texture);

	uint32_t get_quads_per_side() const { return static_cast<int32_t>(godot::Math::round(image_size * mesh_resolution)); }
	uint32_t get_vertices_per_side() const { return get_quads_per_side() + 1; }
	uint32_t get_vertex_count() const { const auto n = get_vertices_per_side(); return n * n; }
	uint32_t get_index_count() const { const auto n = get_quads_per_side(); return n * n * 6; }
	godot::real_t get_quad_size() const { return mesh_size / static_cast<godot::real_t>(get_quads_per_side()); }

	godot::real_t mesh_size = 4.0; // Mesh size
	godot::real_t mesh_resolution = 1.0; // Mesh density
	
	int image_size = 16; // Size of the heightmap image (e.g., 64x64)
	godot::Ref<godot::Image> heightmap;

	godot::real_t texture_size = 1.0;
	godot::Ref<godot::Image> splatmap;

	godot::RID shader_id;
	godot::RID material_id;
	godot::Ref<godot::Texture2D> texture_1;
	godot::Ref<godot::Texture2D> texture_2;
	godot::Ref<godot::Texture2D> texture_3;
	godot::Ref<godot::Texture2D> texture_4;

	godot::RID mesh_id;
	uint32_t cached_vertex_count = 0;
	uint32_t cached_index_count = 0;

	godot::Vector<uint32_t> surface_offsets;
	godot::PackedByteArray surface_vertex_buffer;
	godot::PackedByteArray surface_attribute_buffer;
	uint32_t surface_vertex_stride;
	uint32_t surface_normal_tangent_stride;
	uint32_t surface_attribute_stride;

	uint32_t collider_layer = 1;
	uint32_t collider_mask = 1;
	float collider_priority = 1.0f;
	godot::RID collider_body_id;
	godot::RID collider_shape_id;
	godot::PackedRealArray collider_shape_data;
	godot::real_t collider_shape_min_height;
	godot::real_t collider_shape_max_height;
};

VARIANT_ENUM_CAST(SimpleHeightmap::RebuildFlags);