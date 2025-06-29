#include "simple_heightmap.h"
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

void SimpleHeightmap::_bind_methods()
{
	godot::ClassDB::bind_method(godot::D_METHOD("get_mesh_size"), &SimpleHeightmap::get_mesh_size);
	godot::ClassDB::bind_method(godot::D_METHOD("get_mesh_resolution"), &SimpleHeightmap::get_mesh_resolution);
	godot::ClassDB::bind_method(godot::D_METHOD("get_image_size"), &SimpleHeightmap::get_image_size);
	godot::ClassDB::bind_method(godot::D_METHOD("get_texture_1"), &SimpleHeightmap::get_texture_1);
	godot::ClassDB::bind_method(godot::D_METHOD("get_texture_2"), &SimpleHeightmap::get_texture_2);
	godot::ClassDB::bind_method(godot::D_METHOD("get_texture_3"), &SimpleHeightmap::get_texture_3);
	godot::ClassDB::bind_method(godot::D_METHOD("get_texture_4"), &SimpleHeightmap::get_texture_4);

	godot::ClassDB::bind_method(godot::D_METHOD("set_mesh_size", "value"), &SimpleHeightmap::set_mesh_size);
	godot::ClassDB::bind_method(godot::D_METHOD("set_mesh_resolution", "value"), &SimpleHeightmap::set_mesh_resolution);
	godot::ClassDB::bind_method(godot::D_METHOD("set_image_size", "value"), &SimpleHeightmap::set_image_size);
	godot::ClassDB::bind_method(godot::D_METHOD("set_texture_1", "new_texture"), &SimpleHeightmap::set_texture_1);
	godot::ClassDB::bind_method(godot::D_METHOD("set_texture_2", "new_texture"), &SimpleHeightmap::set_texture_2);
	godot::ClassDB::bind_method(godot::D_METHOD("set_texture_3", "new_texture"), &SimpleHeightmap::set_texture_3);
	godot::ClassDB::bind_method(godot::D_METHOD("set_texture_4", "new_texture"), &SimpleHeightmap::set_texture_4);

	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "mesh_size"), "set_mesh_size", "get_mesh_size");
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "mesh_resolution"), "set_mesh_resolution", "get_mesh_resolution");
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "image_size"), "set_image_size", "get_image_size");
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "texture_1", godot::PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture_1", "get_texture_1");
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "texture_2", godot::PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture_2", "get_texture_2");
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "texture_3", godot::PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture_3", "get_texture_3");
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "texture_4", godot::PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture_4", "get_texture_4");
}

void SimpleHeightmap::_enter_tree()
{
	// TODO: Save and reload data
	generate_default_heightmap_data();
	generate_default_splatmap_data();

	const auto rserver = godot::RenderingServer::get_singleton();
	if (rserver != nullptr)
	{
		mesh_id = rserver->mesh_create();
		set_base(mesh_id);
		rebuild();
	}
}

void SimpleHeightmap::_exit_tree()
{
	const auto rserver = godot::RenderingServer::get_singleton();
	if (rserver != nullptr)
	{
		rserver->free_rid(mesh_id);
	}
}

void SimpleHeightmap::rebuild()
{
	const auto rserver = godot::RenderingServer::get_singleton();
	if (is_inside_tree() && rserver != nullptr && mesh_id.is_valid() && image_size > 0 && mesh_resolution > 0.0)
	{
		rserver->mesh_clear(mesh_id);

		// The expected number of vertices on a given side
		const auto quads_per_side = static_cast<int>(godot::Math::round(image_size * mesh_resolution));
		const auto vertices_per_side = quads_per_side + 1;

		const auto quad_size = (mesh_size / static_cast<godot::real_t>(quads_per_side));

		auto collision_data = godot::PackedFloat32Array();

		// Calculate Position, UV and Collision Height
		// Reset Normal
		const auto vertex_count = vertices_per_side * vertices_per_side;
		vertex_positions.resize(vertex_count);
		vertex_uvs.resize(vertex_count);
		vertex_normals.resize(vertex_count);
		collision_data.resize(vertex_count);
		for (int64_t z = 0; z < vertices_per_side; ++z)
		{
			for (int64_t x = 0; x < vertices_per_side; ++x)
			{
				const auto i = (x % vertices_per_side) + (z * vertices_per_side);
				const auto px = x * quad_size;
				const auto pz = z * quad_size;
				const auto ph = bilinear_sample(heightmap, local_position_to_image_position(godot::Vector3(px, 0.0, pz))).r;
				vertex_positions[i] = godot::Vector3(px, ph, pz);
				vertex_uvs[i] = godot::Vector2(px, pz);
				vertex_normals[i] = godot::Vector3();
				collision_data[i] = ph;
			}
		}

		// Calculate Triangles
		const auto index_count = (quads_per_side * quads_per_side * 6);
		indices.resize(index_count);
		int64_t ti = 0;
		int64_t vi = 0;
		for (int64_t z = 0; z < quads_per_side; ++z)
		{
			for (int64_t x = 0; x < quads_per_side; ++x)
			{
				indices[ti + 0] = vi + 1;
				indices[ti + 1] = vi + quads_per_side + 1;
				indices[ti + 2] = vi;

				indices[ti + 3] = vi + quads_per_side + 2;
				indices[ti + 4] = vi + quads_per_side + 1;
				indices[ti + 5] = vi + 1;

				ti += 6;
				vi += 1;
			}
			vi += 1;
		}

		// Calculate Normals
		for (int64_t i = 0; i < index_count; i += 3)
		{
			const auto i1 = indices[i + 0];
			const auto i2 = indices[i + 1];
			const auto i3 = indices[i + 2];
			const auto p1 = vertex_positions[i1];
			const auto p2 = vertex_positions[i2];
			const auto p3 = vertex_positions[i3];
			const auto v1 = p2 - p1;
			const auto v2 = p3 - p1;
			const auto n = v2.cross(v1).normalized();
			vertex_normals[i1] += n;
			vertex_normals[i2] += n;
			vertex_normals[i3] += n;
		}
		for (int64_t i = 0; i < vertex_count; ++i)
			vertex_normals[i].normalize();
		
		// Submit to data rendering server
		godot::Array arrays;
		arrays.resize(godot::Mesh::ARRAY_MAX);
		arrays[godot::Mesh::ARRAY_VERTEX] = vertex_positions;
		arrays[godot::Mesh::ARRAY_TEX_UV] = vertex_uvs;
		arrays[godot::Mesh::ARRAY_NORMAL] = vertex_normals;
		arrays[godot::Mesh::ARRAY_INDEX] = indices;
		rserver->mesh_add_surface_from_arrays(mesh_id, godot::RenderingServer::PrimitiveType::PRIMITIVE_TRIANGLES, arrays);

		// Rebuild collision
		if (collision_body == nullptr)
		{
			collision_body = memnew(godot::StaticBody3D);
			add_child(collision_body);
			collision_body->set_owner(this);
		}
		if (collision_shape_node == nullptr)
		{
			collision_shape_node = memnew(godot::CollisionShape3D);
			collision_body->add_child(collision_shape_node);
			collision_shape_node->set_owner(this);
		}
		if (collision_shape.is_null())
		{
			collision_shape.instantiate();
			collision_shape_node->set_shape(collision_shape);
		}

		// Assign data to collision shape
		collision_shape->set_map_width(vertices_per_side);
		collision_shape->set_map_depth(vertices_per_side);
		collision_shape->set_map_data(collision_data);

		// The heightmap collision shape is always centered and its quads are always a specific size
		// Move it to the center to align with this heightmap mesh
		collision_shape_node->set_position(godot::Vector3(mesh_size * (godot::real_t)0.5, 0.0, mesh_size * (godot::real_t)0.5));

		// Rescale it so it matches the size of the heightmap mesh
		constexpr godot::real_t COLLISION_QUAD_SIZE = 1.0;
		const godot::real_t actual_heightmap_collision_size = COLLISION_QUAD_SIZE * quads_per_side;
		collision_shape_node->set_scale(
			godot::Vector3(mesh_size / actual_heightmap_collision_size, 1.0, mesh_size / actual_heightmap_collision_size));
	}
}

godot::Vector2 SimpleHeightmap::local_position_to_image_position(const godot::Vector3& local_position) const
{
	return godot::Vector2(
		godot::Math::clamp((local_position.x / mesh_size) * image_size, (godot::real_t)0.0, static_cast<godot::real_t>(image_size)),
		godot::Math::clamp((local_position.z / mesh_size) * image_size, (godot::real_t)0.0, static_cast<godot::real_t>(image_size))
	);
}

godot::Vector2 SimpleHeightmap::global_position_to_image_position(const godot::Vector3& global_position) const
{
	return local_position_to_image_position(to_local(global_position));
}

godot::Vector3 SimpleHeightmap::image_position_to_local_position(const godot::Vector2& image_position) const
{
	return godot::Vector3(
		image_position.x / static_cast<godot::real_t>(image_size) * mesh_size,
		bilinear_sample(heightmap, image_position).r,
		image_position.y / static_cast<godot::real_t>(image_size) * mesh_size
	);
}

godot::Vector3 SimpleHeightmap::image_position_to_global_position(const godot::Vector2& image_position) const
{
	return to_global(image_position_to_local_position(image_position));
}

void SimpleHeightmap::set_mesh_size(const godot::real_t value)
{
	mesh_size = value;
	rebuild();
}

void SimpleHeightmap::set_mesh_resolution(const godot::real_t value)
{
	mesh_resolution = godot::Math::max(value, static_cast<godot::real_t>(0.1));
	rebuild();
}

void SimpleHeightmap::set_image_size(int value)
{
	image_size = godot::Math::max(value, 1);
	if (heightmap.is_valid())
		heightmap->resize(image_size, image_size);
	if (splatmap.is_valid())
		splatmap->resize(image_size, image_size);
	rebuild();
}

void SimpleHeightmap::generate_default_heightmap_data()
{
	heightmap = godot::Image::create(image_size, image_size, false, godot::Image::FORMAT_RF);
	heightmap->resize(image_size, image_size);
	for (int x = 0; x < image_size; ++x)
	{
		for (int y = 0; y < image_size; ++y)
		{
			const auto value = (godot::real_t)(
				godot::Math::sin(static_cast<godot::real_t>(x) / static_cast<godot::real_t>(image_size) * Math_TAU) *
				godot::Math::cos(static_cast<godot::real_t>(y) / static_cast<godot::real_t>(image_size) * Math_TAU));
			heightmap->set_pixel(x, y, godot::Color(value, 0.0, 0.0, 0.0));
		}
	}
}

void SimpleHeightmap::generate_default_splatmap_data()
{
	splatmap = godot::Image::create(image_size, image_size, false, godot::Image::FORMAT_RGBA8);
	splatmap->fill(godot::Color(0.f, 0.f, 0.f, 0.f));
}

namespace
{
	godot::Vector2i get_offset_coordinate(const godot::Vector2i& origin, int32_t offset_x, int32_t offset_y, godot::Ref<godot::Image> image)
	{
		return godot::Vector2i(
			godot::Math::clamp(origin.x + offset_x, 0, image->get_width() - 1),
			godot::Math::clamp(origin.y + offset_y, 0, image->get_height() - 1)
		);
	}
}

godot::Color SimpleHeightmap::bilinear_sample(const godot::Ref<godot::Image>& image, const godot::Vector2& point)
{
	const auto w = image->get_width();
	const auto h = image->get_height();

	const auto pi = godot::Vector2i(
		godot::Math::clamp(static_cast<int>(point.x), 0, w - 1),
		godot::Math::clamp(static_cast<int>(point.y), 0, h - 1)
	);

	const auto v1 = image->get_pixelv(get_offset_coordinate(pi, 0, 0, image));
	const auto v2 = image->get_pixelv(get_offset_coordinate(pi, 1, 0, image));
	const auto v3 = image->get_pixelv(get_offset_coordinate(pi, 0, 1, image));
	const auto v4 = image->get_pixelv(get_offset_coordinate(pi, 1, 1, image));

	const auto tx = (point.x - godot::Math::floor(point.x));
	const auto ty = (point.y - godot::Math::floor(point.y));

	const auto a = v1.lerp(v2, tx);
	const auto b = v3.lerp(v4, tx);
	return a.lerp(b, ty);
}