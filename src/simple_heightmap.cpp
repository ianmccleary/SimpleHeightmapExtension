#include "simple_heightmap.h"
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

void SimpleHeightmap::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_mesh_width"), &SimpleHeightmap::get_mesh_width);
	ClassDB::bind_method(D_METHOD("get_mesh_depth"), &SimpleHeightmap::get_mesh_depth);
	ClassDB::bind_method(D_METHOD("get_mesh_resolution"), &SimpleHeightmap::get_mesh_resolution);
	ClassDB::bind_method(D_METHOD("get_data_resolution"), &SimpleHeightmap::get_data_resolution);

	ClassDB::bind_method(D_METHOD("set_mesh_width", "value"), &SimpleHeightmap::set_mesh_width);
	ClassDB::bind_method(D_METHOD("set_mesh_depth", "value"), &SimpleHeightmap::set_mesh_depth);
	ClassDB::bind_method(D_METHOD("set_mesh_resolution", "value"), &SimpleHeightmap::set_mesh_resolution);
	ClassDB::bind_method(D_METHOD("set_data_resolution", "value"), &SimpleHeightmap::set_data_resolution);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mesh_width"), "set_mesh_width", "get_mesh_width");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mesh_depth"), "set_mesh_depth", "get_mesh_depth");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mesh_resolution"), "set_mesh_resolution", "get_mesh_resolution");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "data_resolution"), "set_data_resolution", "get_data_resolution");

	//	PackedRealArray heightmap_data;
	//	PackedColorArray splatmap_data;
}

void SimpleHeightmap::_enter_tree()
{
	if (heightmap_data.is_empty())
	{
		generate_default_heightmap_data();
	}
	if (splatmap_data.is_empty())
	{
		generate_default_splatmap_data();
	}

	const auto rserver = RenderingServer::get_singleton();
	if (rserver != nullptr)
	{
		mesh_id = rserver->mesh_create();
		set_base(mesh_id);
		rebuild();
	}
}

void SimpleHeightmap::_exit_tree()
{
	const auto rserver = RenderingServer::get_singleton();
	if (rserver != nullptr)
	{
		rserver->free_rid(mesh_id);
	}
}

void SimpleHeightmap::rebuild()
{
	const auto rserver = RenderingServer::get_singleton();
	if (is_inside_tree() && rserver != nullptr && mesh_id.is_valid() && data_resolution > 0 && mesh_resolution > 0.0)
	{
		rserver->mesh_clear(mesh_id);

		// The expected number of vertices on a given side
		const auto quads_per_side = static_cast<int>(Math::round(data_resolution * mesh_resolution));
		const auto vertices_per_side = quads_per_side + 1;

		const auto dx = (mesh_width / static_cast<real_t>(quads_per_side));
		const auto dz = (mesh_depth / static_cast<real_t>(quads_per_side));

		auto collision_data = PackedFloat32Array();

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
				const auto px = x * dx;
				const auto pz = z * dz;
				const auto ph = sample_height(Vector3(px, 0.0, pz));
				vertex_positions[i] = Vector3(px, ph, pz);
				vertex_uvs[i] = Vector2(px, pz);
				vertex_normals[i] = Vector3();
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
		Array arrays;
		arrays.resize(Mesh::ARRAY_MAX);
		arrays[Mesh::ARRAY_VERTEX] = vertex_positions;
		arrays[Mesh::ARRAY_TEX_UV] = vertex_uvs;
		arrays[Mesh::ARRAY_NORMAL] = vertex_normals;
		arrays[Mesh::ARRAY_INDEX] = indices;
		rserver->mesh_add_surface_from_arrays(mesh_id, RenderingServer::PrimitiveType::PRIMITIVE_TRIANGLES, arrays);

		// Rebuild collision
		if (collision_body == nullptr)
		{
			collision_body = memnew(StaticBody3D);
			add_child(collision_body);
			collision_body->set_owner(this);
		}
		if (collision_shape_node == nullptr)
		{
			collision_shape_node = memnew(CollisionShape3D);
			collision_body->add_child(collision_shape_node);
			collision_shape_node->set_owner(this);
		}
		if (collision_shape.is_null())
		{
			collision_shape = Ref<HeightMapShape3D>(memnew(HeightMapShape3D));
			collision_shape_node->set_shape(collision_shape);
		}

		// Assign data to collision shape
		collision_shape->set_map_width(vertices_per_side);
		collision_shape->set_map_depth(vertices_per_side);
		collision_shape->set_map_data(collision_data);

		// The heightmap collision shape is always centered and its quads are always a specific size
		// Move it to the center to align with this heightmap mesh
		collision_shape_node->set_position(Vector3(mesh_width * 0.5, 0.0, mesh_depth * 0.5));

		// Rescale it so it matches the size of the heightmap mesh
		constexpr real_t COLLISION_QUAD_SIZE = 1.0;
		const real_t actual_heightmap_collision_width = COLLISION_QUAD_SIZE * quads_per_side;
		const real_t actual_heightmap_collision_depth = COLLISION_QUAD_SIZE* quads_per_side;
		collision_shape_node->set_scale(
			Vector3(mesh_width / actual_heightmap_collision_width, 1.0, mesh_depth / actual_heightmap_collision_depth));
	}
}

real_t SimpleHeightmap::sample_height(const Vector3& local_position) const
{
	const auto p = Vector2(
		(local_position.x / mesh_width) * data_resolution,
		(local_position.z / mesh_depth) * data_resolution
	);
	
	const auto pi = Vector2i(
		static_cast<int>(p.x),
		static_cast<int>(p.y)
	);

	const auto h1 = get_height_at(Vector2i(pi.x, pi.y));
	const auto h2 = get_height_at(Vector2i(pi.x + 1, pi.y));
	const auto h3 = get_height_at(Vector2i(pi.x, pi.y + 1));
	const auto h4 = get_height_at(Vector2i(pi.x + 1, pi.y + 1));

	const auto tx = (p.x - Math::floor(p.x));
	const auto ty = (p.y - Math::floor(p.y));

	const auto a = Math::lerp(h1, h2, tx);
	const auto b = Math::lerp(h3, h4, tx);
	return Math::lerp(a, b, ty);
}

void SimpleHeightmap::adjust_height(const Vector2i& pixel_coordinates, real_t amount)
{
	set_height_at(pixel_coordinates, get_height_at(pixel_coordinates) + amount);
}

Vector<Vector2i> SimpleHeightmap::get_pixel_coordinates_in_range(const Vector3& local_position, const real_t range_radius) const
{
	// Convert the position and range to pixel space
	const auto pixel_position = Vector2(
		(local_position.x / mesh_width) * data_resolution,
		(local_position.z / mesh_depth) * data_resolution
	);
	const auto pixel_range = Vector2(
		(range_radius / mesh_width) * data_resolution,
		(range_radius / mesh_depth) * data_resolution
	);

	// Calculate the min and max bounds
	const auto min = Vector2i(
		Math::clamp(static_cast<int>(Math::round(pixel_position.x - pixel_range.x)), 0, data_resolution),
		Math::clamp(static_cast<int>(Math::round(pixel_position.y - pixel_range.y)), 0, data_resolution)
	);
	const auto max = Vector2i(
		Math::clamp(static_cast<int>(Math::round(pixel_position.x + pixel_range.x)), 0, data_resolution),
		Math::clamp(static_cast<int>(Math::round(pixel_position.y + pixel_range.y)), 0, data_resolution)
	);

	Vector<Vector2i> pixels;
	for (auto x = min.x; x <= max.x; ++x)
	{
		for (auto y = min.y; y <= max.y; ++y)
		{
			pixels.push_back(Vector2i(x, y));
		}
	}
	return pixels;
}

Vector2i SimpleHeightmap::local_position_to_pixel_coordinates(const Vector3& local_position) const
{
	const auto px = static_cast<int>(Math::round((local_position.x / mesh_width) * data_resolution));
	const auto py = static_cast<int>(Math::round((local_position.z / mesh_depth) * data_resolution));
	return Vector2i(
		Math::clamp(px, 0, data_resolution - 1),
		Math::clamp(py, 0, data_resolution - 1)
	);
}

Vector2i SimpleHeightmap::global_position_to_pixel_coordinates(const Vector3& global_position) const
{
	return local_position_to_pixel_coordinates(to_local(global_position));
}

Vector3 SimpleHeightmap::pixel_coordinates_to_local_position(const Vector2i& pixel_coordinates) const
{
	const auto px = (static_cast<real_t>(pixel_coordinates.x) / static_cast<real_t>(data_resolution)) * mesh_width;
	const auto pz = (static_cast<real_t>(pixel_coordinates.y) / static_cast<real_t>(data_resolution)) * mesh_depth;
	return Vector3(
		px,
		get_height_at(pixel_coordinates),
		pz
	);
}

Vector3 SimpleHeightmap::pixel_coordinates_to_global_position(const Vector2i& pixel_coordinates) const
{
	return to_global(pixel_coordinates_to_local_position(pixel_coordinates));
}

real_t SimpleHeightmap::get_height_at(const Vector2i& p) const
{
	const auto px = Math::clamp(p.x, 0, data_resolution - 1);
	const auto py = Math::clamp(p.y, 0, data_resolution - 1);
	const auto i = px + py * data_resolution;
	return heightmap_data[i];
}

void SimpleHeightmap::set_height_at(const Vector2i& p, real_t height)
{
	const auto px = Math::clamp(p.x, 0, data_resolution - 1);
	const auto py = Math::clamp(p.y, 0, data_resolution - 1);
	const auto i = px + py * data_resolution;
	heightmap_data[i] = height;
}

void SimpleHeightmap::set_mesh_width(const real_t value)
{
	mesh_width = value;
	rebuild();
}

void SimpleHeightmap::set_mesh_depth(const real_t value)
{
	mesh_depth = value;
	rebuild();
}

void SimpleHeightmap::set_mesh_resolution(const real_t value)
{
	mesh_resolution = Math::max(value, static_cast<real_t>(0.1));
	rebuild();
}

void SimpleHeightmap::set_data_resolution(int value)
{
	data_resolution = Math::max(value, 1);
	if (heightmap_data.size() != get_desired_heightmap_data_size())
	{
		// TODO: If it's not empty, resize data instead of completely destroying it
		generate_default_heightmap_data();
	}
	rebuild();
}

void SimpleHeightmap::generate_default_heightmap_data()
{
	heightmap_data.resize(get_desired_heightmap_data_size());
	for (int x = 0; x < data_resolution; ++x)
	{
		for (int z = 0; z < data_resolution; ++z)
		{
			int i = x + z * data_resolution;
			heightmap_data[i] = 
				Math::sin(static_cast<real_t>(x) / static_cast<real_t>(data_resolution) * Math_TAU) *
				Math::cos(static_cast<real_t>(z) / static_cast<real_t>(data_resolution) * Math_TAU);
		}
	}
}

void SimpleHeightmap::generate_default_splatmap_data()
{
	splatmap_data.resize(get_desired_heightmap_data_size());
	splatmap_data.fill(Color());
}