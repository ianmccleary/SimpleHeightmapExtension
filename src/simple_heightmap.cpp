#include "simple_heightmap.h"
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

void SimpleHeightmap::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_mesh_size"), &SimpleHeightmap::get_mesh_size);
	ClassDB::bind_method(D_METHOD("get_mesh_resolution"), &SimpleHeightmap::get_mesh_resolution);
	ClassDB::bind_method(D_METHOD("get_image_size"), &SimpleHeightmap::get_image_size);

	ClassDB::bind_method(D_METHOD("set_mesh_size", "value"), &SimpleHeightmap::set_mesh_size);
	ClassDB::bind_method(D_METHOD("set_mesh_resolution", "value"), &SimpleHeightmap::set_mesh_resolution);
	ClassDB::bind_method(D_METHOD("set_image_size", "value"), &SimpleHeightmap::set_image_size);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mesh_size"), "set_mesh_size", "get_mesh_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mesh_resolution"), "set_mesh_resolution", "get_mesh_resolution");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "image_size"), "set_image_size", "get_image_size");

	//	PackedRealArray heightmap_data;
	//	PackedColorArray splatmap_data;
}

void SimpleHeightmap::_enter_tree()
{
	// TODO: Save and reload data
	generate_default_heightmap_data();
	generate_default_splatmap_data();

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
	if (is_inside_tree() && rserver != nullptr && mesh_id.is_valid() && image_size > 0 && mesh_resolution > 0.0)
	{
		rserver->mesh_clear(mesh_id);

		// The expected number of vertices on a given side
		const auto quads_per_side = static_cast<int>(Math::round(image_size * mesh_resolution));
		const auto vertices_per_side = quads_per_side + 1;

		const auto quad_size = (mesh_size / static_cast<real_t>(quads_per_side));

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
				const auto px = x * quad_size;
				const auto pz = z * quad_size;
				const auto ph = heightmap_image.bilinear_sample(local_position_to_image_position(Vector3(px, 0.0, pz)));
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
		collision_shape_node->set_position(Vector3(mesh_size * (real_t)0.5, 0.0, mesh_size * (real_t)0.5));

		// Rescale it so it matches the size of the heightmap mesh
		constexpr real_t COLLISION_QUAD_SIZE = 1.0;
		const real_t actual_heightmap_collision_size = COLLISION_QUAD_SIZE * quads_per_side;
		collision_shape_node->set_scale(
			Vector3(mesh_size / actual_heightmap_collision_size, 1.0, mesh_size / actual_heightmap_collision_size));
	}
}

Vector2 SimpleHeightmap::local_position_to_image_position(const Vector3& local_position) const
{
	return Vector2(
		Math::clamp((local_position.x / mesh_size) * image_size, (real_t)0.0, static_cast<real_t>(image_size)),
		Math::clamp((local_position.z / mesh_size) * image_size, (real_t)0.0, static_cast<real_t>(image_size))
	);
}

Vector2 SimpleHeightmap::global_position_to_image_position(const Vector3& global_position) const
{
	return local_position_to_image_position(to_local(global_position));
}

Vector3 SimpleHeightmap::image_position_to_local_position(const Vector2& image_position) const
{
	return Vector3(
		image_position.x / static_cast<real_t>(image_size) * mesh_size,
		heightmap_image.bilinear_sample(image_position),
		image_position.y / static_cast<real_t>(image_size) * mesh_size
	);
}

Vector3 SimpleHeightmap::image_position_to_global_position(const Vector2& image_position) const
{
	return to_global(image_position_to_local_position(image_position));
}

void SimpleHeightmap::set_mesh_size(const real_t value)
{
	mesh_size = value;
	rebuild();
}

void SimpleHeightmap::set_mesh_resolution(const real_t value)
{
	mesh_resolution = Math::max(value, static_cast<real_t>(0.1));
	rebuild();
}

void SimpleHeightmap::set_image_size(int value)
{
	image_size = Math::max(value, 1);
	if (image_size != heightmap_image.get_size())
	{
		// TODO: If it's not empty, resize data instead of completely destroying it
		generate_default_heightmap_data();
	}
	if (image_size != splatmap_image.get_size())
	{
		// TODO: If it's not empty, resize data instead of completely destroying it
		generate_default_splatmap_data();
	}
	rebuild();
}

void SimpleHeightmap::generate_default_heightmap_data()
{
	heightmap_image.resize(image_size);
	for (int x = 0; x < image_size; ++x)
	{
		for (int y = 0; y < image_size; ++y)
		{
			const auto value = (real_t)(
				Math::sin(static_cast<real_t>(x) / static_cast<real_t>(image_size) * Math_TAU) *
				Math::cos(static_cast<real_t>(y) / static_cast<real_t>(image_size) * Math_TAU));
			heightmap_image.set_pixel(x, y, value);
		}
	}
}

void SimpleHeightmap::generate_default_splatmap_data()
{
	splatmap_image.resize(image_size);
	splatmap_image.fill(Color());
}