#include "simple_heightmap.h"
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

void SimpleHeightmap::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_heightmap_width"), &SimpleHeightmap::get_heightmap_width);
	ClassDB::bind_method(D_METHOD("get_heightmap_depth"), &SimpleHeightmap::get_heightmap_depth);
	ClassDB::bind_method(D_METHOD("get_distance_between_vertices"), &SimpleHeightmap::get_distance_between_vertices);

	ClassDB::bind_method(D_METHOD("set_heightmap_width", "value"), &SimpleHeightmap::set_heightmap_width);
	ClassDB::bind_method(D_METHOD("set_heightmap_depth", "value"), &SimpleHeightmap::set_heightmap_depth);
	ClassDB::bind_method(D_METHOD("set_distance_between_vertices", "value"), &SimpleHeightmap::set_distance_between_vertices);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "heightmap_width"), "set_heightmap_width", "get_heightmap_width");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "heightmap_depth"), "set_heightmap_depth", "get_heightmap_depth");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "distance_between_vertices"), "set_distance_between_vertices", "get_distance_between_vertices");
}

void SimpleHeightmap::_enter_tree()
{
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
	if (rserver != nullptr && mesh_id.is_valid())
	{
		rserver->mesh_clear(mesh_id);

		const auto width_i = static_cast<int64_t>(Math::round(heightmap_width / distance_between_vertices));
		const auto depth_i = static_cast<int64_t>(Math::round(heightmap_depth / distance_between_vertices));

		const auto dx = (heightmap_width / static_cast<real_t>(width_i));
		const auto dz = (heightmap_depth / static_cast<real_t>(depth_i));

		// Calculate Position and UV
		// Reset Normal
		const auto vertex_count = (width_i + 1) * (depth_i + 1);
		vertex_positions.resize(vertex_count);
		vertex_uvs.resize(vertex_count);
		vertex_normals.resize(vertex_count);
		for (int64_t z = 0; z < depth_i + 1; ++z)
		{
			for (int64_t x = 0; x < width_i + 1; ++x)
			{
				const auto i = (x % (width_i + 1)) + (z * (width_i + 1));
				const auto px = x * dx;
				const auto pz = z * dz;
				vertex_positions[i] = Vector3(px, 0.0, pz);
				vertex_uvs[i] = Vector2(px / distance_between_vertices, pz / distance_between_vertices);
				vertex_normals[i] = Vector3();
			}
		}

		// Calculate Triangles
		const auto index_count = (width_i * depth_i * 6);
		indices.resize(index_count);
		int64_t ti = 0;
		int64_t vi = 0;
		for (int64_t z = 0; z < depth_i; ++z)
		{
			for (int64_t x = 0; x < width_i; ++x)
			{
				indices[ti + 0] = vi + 1;
				indices[ti + 1] = vi + width_i + 1;
				indices[ti + 2] = vi;

				indices[ti + 3] = vi + width_i + 2;
				indices[ti + 4] = vi + width_i + 1;
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
	}
}

void SimpleHeightmap::set_heightmap_width(const real_t value)
{
	heightmap_width = value;
	rebuild();
}

void SimpleHeightmap::set_heightmap_depth(const real_t value)
{
	heightmap_depth = value;
	rebuild();
}

void SimpleHeightmap::set_distance_between_vertices(const real_t value)
{
	distance_between_vertices = Math::max(value, static_cast<real_t>(0.1));
	rebuild();
}