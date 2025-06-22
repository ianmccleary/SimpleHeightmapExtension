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
		const auto vertices_per_side = static_cast<int>(Math::round(data_resolution * mesh_resolution));

		const auto dx = (mesh_width / static_cast<real_t>(vertices_per_side));
		const auto dz = (mesh_depth / static_cast<real_t>(vertices_per_side));

		// Calculate Position and UV
		// Reset Normal
		const auto vertex_count = (vertices_per_side + 1) * (vertices_per_side + 1);
		vertex_positions.resize(vertex_count);
		vertex_uvs.resize(vertex_count);
		vertex_normals.resize(vertex_count);
		for (int64_t z = 0; z < vertices_per_side + 1; ++z)
		{
			for (int64_t x = 0; x < vertices_per_side + 1; ++x)
			{
				const auto i = (x % (vertices_per_side + 1)) + (z * (vertices_per_side + 1));
				const auto px = x * dx;
				const auto pz = z * dz;
				vertex_positions[i] = Vector3(px, sample_height(px, pz), pz);
				vertex_uvs[i] = Vector2(px, pz);
				vertex_normals[i] = Vector3();
			}
		}

		// Calculate Triangles
		const auto index_count = (vertices_per_side * vertices_per_side * 6);
		indices.resize(index_count);
		int64_t ti = 0;
		int64_t vi = 0;
		for (int64_t z = 0; z < vertices_per_side; ++z)
		{
			for (int64_t x = 0; x < vertices_per_side; ++x)
			{
				indices[ti + 0] = vi + 1;
				indices[ti + 1] = vi + vertices_per_side + 1;
				indices[ti + 2] = vi;

				indices[ti + 3] = vi + vertices_per_side + 2;
				indices[ti + 4] = vi + vertices_per_side + 1;
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

real_t SimpleHeightmap::sample_height(real_t x, real_t y) const
{
	x = Math::clamp(x, static_cast<real_t>(0.0), mesh_width);
	y = Math::clamp(y, static_cast<real_t>(0.0), mesh_depth);
	const auto px = static_cast<int>(Math::round((x / mesh_width) * data_resolution));
	const auto py = static_cast<int>(Math::round((y / mesh_depth) * data_resolution));
	return (
		get_height_at(px, py) + 
		get_height_at(px - 1, py) + 
		get_height_at(px + 1, py) +
		get_height_at(px, py - 1) +
		get_height_at(px, py + 1)
	) / static_cast<real_t>(5.0);
}

real_t SimpleHeightmap::get_height_at(int x, int y) const
{
	const auto px = Math::clamp(x, 0, data_resolution - 1);
	const auto py = Math::clamp(y, 0, data_resolution - 1);
	const auto i = px + py * data_resolution;
	return heightmap_data[i];
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