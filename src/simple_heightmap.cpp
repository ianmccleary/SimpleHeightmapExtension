#include "simple_heightmap.h"
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

void SimpleHeightmap::_bind_methods()
{
	const auto image_usage_flags =
		godot::PROPERTY_USAGE_DEFAULT |
		godot::PROPERTY_USAGE_READ_ONLY |
		godot::PROPERTY_USAGE_EDITOR_INSTANTIATE_OBJECT;

	godot::ClassDB::bind_method(godot::D_METHOD("get_mesh_size"), &SimpleHeightmap::get_mesh_size);
	godot::ClassDB::bind_method(godot::D_METHOD("get_mesh_resolution"), &SimpleHeightmap::get_mesh_resolution);
	godot::ClassDB::bind_method(godot::D_METHOD("get_image_size"), &SimpleHeightmap::get_image_size);
	godot::ClassDB::bind_method(godot::D_METHOD("get_texture_size"), &SimpleHeightmap::get_texture_size);
	godot::ClassDB::bind_method(godot::D_METHOD("get_heightmap_image"), &SimpleHeightmap::get_heightmap_image);
	godot::ClassDB::bind_method(godot::D_METHOD("get_splatmap_image"), &SimpleHeightmap::get_splatmap_image);
	godot::ClassDB::bind_method(godot::D_METHOD("get_texture_1"), &SimpleHeightmap::get_texture_1);
	godot::ClassDB::bind_method(godot::D_METHOD("get_texture_2"), &SimpleHeightmap::get_texture_2);
	godot::ClassDB::bind_method(godot::D_METHOD("get_texture_3"), &SimpleHeightmap::get_texture_3);
	godot::ClassDB::bind_method(godot::D_METHOD("get_texture_4"), &SimpleHeightmap::get_texture_4);

	godot::ClassDB::bind_method(godot::D_METHOD("set_mesh_size", "value"), &SimpleHeightmap::set_mesh_size);
	godot::ClassDB::bind_method(godot::D_METHOD("set_mesh_resolution", "value"), &SimpleHeightmap::set_mesh_resolution);
	godot::ClassDB::bind_method(godot::D_METHOD("set_image_size", "value"), &SimpleHeightmap::set_image_size);
	godot::ClassDB::bind_method(godot::D_METHOD("set_texture_size", "value"), &SimpleHeightmap::set_texture_size);
	godot::ClassDB::bind_method(godot::D_METHOD("set_heightmap_image"), &SimpleHeightmap::set_heightmap_image);
	godot::ClassDB::bind_method(godot::D_METHOD("set_splatmap_image"), &SimpleHeightmap::set_splatmap_image);
	godot::ClassDB::bind_method(godot::D_METHOD("set_texture_1", "new_texture"), &SimpleHeightmap::set_texture_1);
	godot::ClassDB::bind_method(godot::D_METHOD("set_texture_2", "new_texture"), &SimpleHeightmap::set_texture_2);
	godot::ClassDB::bind_method(godot::D_METHOD("set_texture_3", "new_texture"), &SimpleHeightmap::set_texture_3);
	godot::ClassDB::bind_method(godot::D_METHOD("set_texture_4", "new_texture"), &SimpleHeightmap::set_texture_4);

	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "mesh_size"), "set_mesh_size", "get_mesh_size");
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "mesh_resolution"), "set_mesh_resolution", "get_mesh_resolution");
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "image_size"), "set_image_size", "get_image_size");
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "texture_size"), "set_texture_size", "get_texture_size");
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "heightmap_image", godot::PROPERTY_HINT_RESOURCE_TYPE, "Image", image_usage_flags), "set_heightmap_image", "get_heightmap_image");
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "splatmap_image", godot::PROPERTY_HINT_RESOURCE_TYPE, "Image", image_usage_flags), "set_splatmap_image", "get_splatmap_image");
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "texture_1", godot::PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture_1", "get_texture_1");
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "texture_2", godot::PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture_2", "get_texture_2");
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "texture_3", godot::PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture_3", "get_texture_3");
	ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "texture_4", godot::PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_texture_4", "get_texture_4");
}

void SimpleHeightmap::_notification(int what)
{
	if (what == NOTIFICATION_READY)
	{
		const auto rserver = godot::RenderingServer::get_singleton();
		if (rserver != nullptr)
		{
			mesh_id = rserver->mesh_create();
			set_base(mesh_id);
			rebuild(ALL);
		}
	}
	else if (what == NOTIFICATION_PREDELETE)
	{
		const auto rserver = godot::RenderingServer::get_singleton();
		if (rserver != nullptr)
		{
			rserver->free_rid(mesh_id);
		}
	}
}

namespace
{
	constexpr int32_t MAX_UINT_16 = std::numeric_limits<uint16_t>::max();
	
	struct CompressedNormalTangent
	{
		uint16_t na;
		uint16_t nb;
		uint16_t ta;
		uint16_t tb;
	};

	godot::Vector3 generate_tangent_from_normal(const godot::Vector3& normal)
	{
		return godot::Vector3(normal.z, -normal.x, normal.y).cross(normal.normalized()).normalized();
	}

	CompressedNormalTangent compress_normal(const godot::Vector3& normal)
	{
		CompressedNormalTangent output;

		auto normal_encoded = normal.octahedron_encode();
		output.na = static_cast<uint16_t>(godot::Math::clamp(static_cast<int32_t>(normal_encoded.x * MAX_UINT_16), 0, MAX_UINT_16));
		output.nb =	static_cast<uint16_t>(godot::Math::clamp(static_cast<int32_t>(normal_encoded.y * MAX_UINT_16), 0, MAX_UINT_16));

		auto tangent_encoded = generate_tangent_from_normal(normal).octahedron_tangent_encode(1.f);
		output.ta = static_cast<uint16_t>(godot::Math::clamp(static_cast<int32_t>(tangent_encoded.x * MAX_UINT_16), 0, MAX_UINT_16));
		output.tb = static_cast<uint16_t>(godot::Math::clamp(static_cast<int32_t>(tangent_encoded.y * MAX_UINT_16), 0, MAX_UINT_16));
		if (output.ta == 0 && output.tb == MAX_UINT_16)
		{
			output.ta = MAX_UINT_16;
		}
		return output;
	}
}

void SimpleHeightmap::rebuild(ChangeType change_type)
{
	const auto rserver = godot::RenderingServer::get_singleton();
	if (rserver != nullptr && is_inside_tree() && mesh_id.is_valid() && heightmap.is_valid() && splatmap.is_valid() && mesh_size > CMP_EPSILON)
	{
		const auto vertex_count = get_vertex_count();
		const auto index_count = get_index_count();
		if (vertex_count != vertex_positions.size() || index_count != indices.size())
		{
			vertex_positions.resize(vertex_count);
			vertex_uvs.resize(vertex_count);
			vertex_normals.resize(vertex_count);
			vertex_colors.resize(vertex_count);
			collision_data.resize(vertex_count);
			indices.resize(index_count);
			
			// Update data
			const auto quads_per_side = get_quads_per_side();
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
			rebuild_mesh(change_type);
			rebuild_collider_mesh();
			
			// Submit to data rendering server
			godot::Array arrays;
			arrays.resize(godot::Mesh::ARRAY_MAX);
			arrays[godot::Mesh::ARRAY_VERTEX] = vertex_positions;
			arrays[godot::Mesh::ARRAY_TEX_UV] = vertex_uvs;
			arrays[godot::Mesh::ARRAY_NORMAL] = vertex_normals;
			arrays[godot::Mesh::ARRAY_COLOR] = vertex_colors;
			arrays[godot::Mesh::ARRAY_INDEX] = indices;
			rserver->mesh_clear(mesh_id);
			rserver->mesh_add_surface_from_arrays(mesh_id, godot::RenderingServer::PrimitiveType::PRIMITIVE_TRIANGLES, arrays);

			// Cache information to use when updating the mesh
			const auto surface = rserver->mesh_get_surface(mesh_id, 0);
			const auto format = static_cast<godot::RenderingServer::ArrayFormat>(static_cast<int64_t>(surface["format"]));
			surface_offsets.resize(godot::Mesh::ARRAY_MAX);
			surface_offsets.set(godot::Mesh::ARRAY_VERTEX, rserver->mesh_surface_get_format_offset(format, vertex_count, godot::Mesh::ARRAY_VERTEX));
			surface_offsets.set(godot::Mesh::ARRAY_TEX_UV, rserver->mesh_surface_get_format_offset(format, vertex_count, godot::Mesh::ARRAY_TEX_UV));
			surface_offsets.set(godot::Mesh::ARRAY_NORMAL, rserver->mesh_surface_get_format_offset(format, vertex_count, godot::Mesh::ARRAY_NORMAL));
			surface_offsets.set(godot::Mesh::ARRAY_TANGENT, rserver->mesh_surface_get_format_offset(format, vertex_count, godot::Mesh::ARRAY_TANGENT));
			surface_offsets.set(godot::Mesh::ARRAY_COLOR, rserver->mesh_surface_get_format_offset(format, vertex_count, godot::Mesh::ARRAY_COLOR));
			surface_vertex_buffer = surface["vertex_data"];
			surface_attribute_buffer = surface["attribute_data"];
			surface_vertex_stride = rserver->mesh_surface_get_format_vertex_stride(format, vertex_count);
			surface_normal_tangent_stride = rserver->mesh_surface_get_format_normal_tangent_stride(format, vertex_count);
			surface_attribute_stride = rserver->mesh_surface_get_format_attribute_stride(format, vertex_count);
		}
		else
		{
			// Update data
			rebuild_mesh(change_type);
			if (change_type & HEIGHTMAP)
			{
				rebuild_collider_mesh();
			}

			// Update surface data and submit to rendering server
			auto surface_vertex_buffer_p = surface_vertex_buffer.ptrw();
			auto surface_attribute_buffer_p = surface_attribute_buffer.ptrw();
			for (int i = 0; i < vertex_count; ++i)
			{
				const auto vertex_position = vertex_positions[i];
				const auto vertex_normal = compress_normal(vertex_normals[i]);
				const auto color = vertex_colors[i].to_abgr32();
				
				if (change_type & HEIGHTMAP)
				{
					memcpy(&surface_vertex_buffer_p[i * surface_vertex_stride + surface_offsets[godot::Mesh::ARRAY_VERTEX]], &vertex_position, sizeof(godot::Vector3));
					memcpy(&surface_vertex_buffer_p[i * surface_normal_tangent_stride + surface_offsets[godot::Mesh::ARRAY_NORMAL]], &vertex_normal, sizeof(CompressedNormalTangent));
				}
				if (change_type & SPLATMAP)
				{
					memcpy(&surface_attribute_buffer_p[i * surface_attribute_stride + surface_offsets[godot::Mesh::ARRAY_COLOR]], &color, sizeof(decltype(color)));
				}
			}
			if (change_type & HEIGHTMAP)
			{
				rserver->mesh_surface_update_vertex_region(mesh_id, 0, 0, surface_vertex_buffer);
			}
			if (change_type & SPLATMAP)
			{
				rserver->mesh_surface_update_attribute_region(mesh_id, 0, 0, surface_attribute_buffer);
			}
		}
	}
}

void SimpleHeightmap::rebuild_mesh(ChangeType change_type)
{
	if (!(change_type & HEIGHTMAP) && !(change_type & SPLATMAP) && !(change_type & UV))
	{
		return;
	}

	// Calculate Position, UV
	const auto vertices_per_side = get_vertices_per_side();
	const auto quad_size = get_quad_size();
	for (int64_t z = 0; z < vertices_per_side; ++z)
	{
		for (int64_t x = 0; x < vertices_per_side; ++x)
		{
			const auto i = (x % vertices_per_side) + (z * vertices_per_side);
			const auto px = x * quad_size;
			const auto pz = z * quad_size;
			const auto p = godot::Vector3(px, 0.0, pz);
			if (change_type & HEIGHTMAP)
			{
				const auto py = bilinear_sample(heightmap, local_position_to_image_position(p)).r;
				vertex_positions[i] = godot::Vector3(px, py, pz);
			}
			if (change_type & UV)
			{
				vertex_uvs[i] = godot::Vector2(px, pz) * texture_size;
			}
			if (change_type & SPLATMAP)
			{
				vertex_colors[i] = bilinear_sample(splatmap, local_position_to_image_position(p));
			}
		}
	}

	if (change_type & HEIGHTMAP)
	{
		vertex_normals.fill(godot::Vector3());
		const auto quads_per_side = get_quads_per_side();
		const auto vertex_count = get_vertex_count();
		const auto index_count = get_index_count();
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
	}
}

void SimpleHeightmap::rebuild_collider_mesh()
{
	for (int i = 0; i < vertex_positions.size(); ++i)
	{
		collision_data[i] = vertex_positions[i].y;
	}

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
	const auto vertices_per_side = get_vertices_per_side();
	collision_shape->set_map_width(vertices_per_side);
	collision_shape->set_map_depth(vertices_per_side);
	collision_shape->set_map_data(collision_data);

	// The heightmap collision shape is always centered and its quads are always a specific size
	// Move it to the center to align with this heightmap mesh
	collision_shape_node->set_position(godot::Vector3(mesh_size * (godot::real_t)0.5, 0.0, mesh_size * (godot::real_t)0.5));

	// Rescale it so it matches the size of the heightmap mesh
	constexpr godot::real_t COLLISION_QUAD_SIZE = 1.0;
	const auto quads_per_side = get_quads_per_side();
	const auto actual_heightmap_collision_size = COLLISION_QUAD_SIZE * quads_per_side;
	const auto scale = mesh_size / actual_heightmap_collision_size;
	collision_shape_node->set_scale(godot::Vector3(scale, 1.0, scale));
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
	rebuild(ALL);
}

void SimpleHeightmap::set_mesh_resolution(const godot::real_t value)
{
	mesh_resolution = godot::Math::max(value, static_cast<godot::real_t>(0.1));
	rebuild(ALL);
}

void SimpleHeightmap::set_image_size(int value)
{
	image_size = godot::Math::max(value, 1);
	if (heightmap.is_valid())
		heightmap->resize(image_size, image_size);
	if (splatmap.is_valid())
		splatmap->resize(image_size, image_size);
	rebuild(ALL);
}

void SimpleHeightmap::set_texture_size(const godot::real_t value)
{
	texture_size = godot::Math::max(value, static_cast<godot::real_t>(0.01));
	rebuild(UV);
}

void SimpleHeightmap::set_heightmap_image(const godot::Ref<godot::Image>& new_heightmap)
{
	heightmap = new_heightmap;
	initialize_image(heightmap, godot::Image::FORMAT_RF, image_size);
	rebuild(ALL);
}

void SimpleHeightmap::set_splatmap_image(const godot::Ref<godot::Image>& new_splatmap)
{
	splatmap = new_splatmap;
	initialize_image(splatmap, godot::Image::FORMAT_RGBA8, image_size, godot::Color(1.0, 0.0, 0.0, 0.0));
	rebuild(ALL);
}

void SimpleHeightmap::initialize_image(const godot::Ref<godot::Image>& image, godot::Image::Format format, int32_t size, godot::Color default_color)
{
	if (image.is_valid())
	{
		if (image->get_data().is_empty())
		{
			godot::PackedByteArray new_data;
			new_data.resize(size * size * 4);
			image->set_data(size, size, false, format, new_data);
			image->fill(default_color);
		}
		if (image->get_format() != format)
		{
			image->convert(format);
		}
		if (image->get_width() != size || image->get_height() != size)
		{
			image->resize(size, size);
		}
	}
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