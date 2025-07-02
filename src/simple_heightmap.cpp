#include "simple_heightmap.h"
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

constexpr const char* default_texture_1_param = "texture_map_1";
constexpr const char* default_texture_2_param = "texture_map_2";
constexpr const char* default_texture_3_param = "texture_map_3";
constexpr const char* default_texture_4_param = "texture_map_4";

void SimpleHeightmap::_bind_methods()
{
	const auto image_usage_flags =
		godot::PROPERTY_USAGE_STORAGE |
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

	BIND_ENUM_CONSTANT(REBUILD_NONE);
	BIND_ENUM_CONSTANT(REBUILD_ALL);
	BIND_ENUM_CONSTANT(REBUILD_HEIGHTMAP);
	BIND_ENUM_CONSTANT(REBUILD_SPLATMAP);
	BIND_ENUM_CONSTANT(REBUILD_UV);

	godot::ClassDB::bind_method(godot::D_METHOD("rebuild", "change_type"), &SimpleHeightmap::rebuild);

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

	ADD_SIGNAL(godot::MethodInfo("texture_1_changed", godot::PropertyInfo(godot::Variant::OBJECT, "new_texture")));
	ADD_SIGNAL(godot::MethodInfo("texture_2_changed", godot::PropertyInfo(godot::Variant::OBJECT, "new_texture")));
	ADD_SIGNAL(godot::MethodInfo("texture_3_changed", godot::PropertyInfo(godot::Variant::OBJECT, "new_texture")));
	ADD_SIGNAL(godot::MethodInfo("texture_4_changed", godot::PropertyInfo(godot::Variant::OBJECT, "new_texture")));
}

SimpleHeightmap::SimpleHeightmap()
{
	const auto rserver = godot::RenderingServer::get_singleton();
	if (rserver != nullptr)
	{
		shader_id = rserver->shader_create();
		rserver->shader_set_code(shader_id, godot::vformat(R"(
			shader_type spatial;

			uniform sampler2D %s : source_color;
			uniform sampler2D %s : source_color;
			uniform sampler2D %s : source_color;
			uniform sampler2D %s : source_color;

			void fragment()
			{
				vec4 texture_1 = texture(%s, UV);
				vec4 texture_2 = texture(%s, UV);
				vec4 texture_3 = texture(%s, UV);
				vec4 texture_4 = texture(%s, UV);
				vec4 output = normalize((texture_1 * COLOR.r) + (texture_2 * COLOR.g) + (texture_3 * COLOR.b) + (texture_4 * COLOR.a));
				ALBEDO = output.rgb;
			})",
			default_texture_1_param, default_texture_2_param, default_texture_3_param, default_texture_4_param,
			default_texture_1_param, default_texture_2_param, default_texture_3_param, default_texture_4_param));

		material_id = rserver->material_create();
		rserver->material_set_shader(material_id, shader_id);

		mesh_id = rserver->mesh_create();
		set_base(mesh_id);
	}
	const auto pserver = godot::PhysicsServer3D::get_singleton();
	if (pserver)
	{
		auto id = get_instance_id();

		collider_body_id = pserver->body_create();
		pserver->body_set_mode(collider_body_id, godot::PhysicsServer3D::BODY_MODE_STATIC);
		pserver->body_set_ray_pickable(collider_body_id, true);
		pserver->body_attach_object_instance_id(collider_body_id, get_instance_id());

		collider_shape_id = pserver->heightmap_shape_create();
		pserver->body_add_shape(collider_body_id, collider_shape_id);
	}
}

void SimpleHeightmap::_notification(int what)
{
	switch (what)
	{
		case NOTIFICATION_READY:
		{
			rebuild(REBUILD_ALL);
		}
		break;

		case NOTIFICATION_ENTER_WORLD: // Physics World
		{
			const auto pserver = godot::PhysicsServer3D::get_singleton();
			if (pserver && collider_body_id.is_valid())
			{
				const auto world = get_world_3d();
				const auto space = world->get_space();
				pserver->body_set_space(collider_body_id, space);
				pserver->body_set_state(collider_body_id, godot::PhysicsServer3D::BODY_STATE_TRANSFORM, get_global_transform());
			}
		}
		break;

		case NOTIFICATION_TRANSFORM_CHANGED:
		{
			const auto pserver = godot::PhysicsServer3D::get_singleton();
			if (pserver && collider_body_id.is_valid())
			{
				pserver->body_set_state(collider_body_id, godot::PhysicsServer3D::BODY_STATE_TRANSFORM, get_global_transform());
			}
		}
		break;

		case NOTIFICATION_EXIT_WORLD: // Physics World
		{
			const auto pserver = godot::PhysicsServer3D::get_singleton();
			if (pserver && collider_body_id.is_valid())
			{
				pserver->body_set_space(collider_body_id, godot::RID());
			}
		}
		break;
	}
}

SimpleHeightmap::~SimpleHeightmap()
{
	const auto pserver = godot::PhysicsServer3D::get_singleton();
	if (pserver)
	{
		pserver->free_rid(collider_shape_id);
		pserver->free_rid(collider_body_id);
	}
	const auto rserver = godot::RenderingServer::get_singleton();
	if (rserver != nullptr)
	{
		rserver->free_rid(mesh_id);
		rserver->free_rid(material_id);
		rserver->free_rid(shader_id);
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

void SimpleHeightmap::rebuild(RebuildFlags flags)
{
	constexpr auto ELEMENT_SIZE_POSITION = sizeof(godot::Vector3);
	constexpr auto ELEMENT_SIZE_NORMAL_TANGENT = sizeof(CompressedNormalTangent);
	constexpr auto ELEMENT_SIZE_UV = sizeof(godot::Vector2);
	constexpr auto ELEMENT_SIZE_COLOR = sizeof(int32_t);

	constexpr auto VERTEX_ELEMENT_SIZE = ELEMENT_SIZE_POSITION + ELEMENT_SIZE_NORMAL_TANGENT;
	constexpr auto ATTRIB_ELEMENT_SIZE = ELEMENT_SIZE_UV + ELEMENT_SIZE_COLOR;

	const auto rserver = godot::RenderingServer::get_singleton();
	const auto pserver = godot::PhysicsServer3D::get_singleton();
	if (rserver != nullptr && is_inside_tree() && mesh_id.is_valid() && heightmap.is_valid() && splatmap.is_valid() && mesh_size > CMP_EPSILON)
	{
		const auto vertex_count = get_vertex_count();
		const auto index_count = get_index_count();
		if (vertex_count != cached_vertex_count || index_count != cached_index_count)
		{
			cached_vertex_count = vertex_count;
			cached_index_count = index_count;
			
			// Calculate indices
			const auto quads_per_side = get_quads_per_side();
			const auto index_element_size = vertex_count <= std::numeric_limits<uint16_t>::max() ? sizeof(uint16_t) : sizeof(uint32_t);
			godot::PackedByteArray indices;
			indices.resize(index_count * index_element_size);
			const auto indices_p = indices.ptrw();
			uint32_t ti = 0;
			uint32_t vi = 0;
			for (uint32_t z = 0; z < quads_per_side; ++z)
			{
				for (uint32_t x = 0; x < quads_per_side; ++x)
				{
					uint32_t i1 = vi + 1;
					uint32_t i2 = vi + quads_per_side + 1;
					uint32_t i3 = vi;
					uint32_t i4 = vi + quads_per_side + 2;

					memcpy(&indices_p[ti * index_element_size + (index_element_size * 0)], &i1, index_element_size);
					memcpy(&indices_p[ti * index_element_size + (index_element_size * 1)], &i2, index_element_size);
					memcpy(&indices_p[ti * index_element_size + (index_element_size * 2)], &i3, index_element_size);
					
					memcpy(&indices_p[ti * index_element_size + (index_element_size * 3)], &i4, index_element_size);
					memcpy(&indices_p[ti * index_element_size + (index_element_size * 4)], &i2, index_element_size);
					memcpy(&indices_p[ti * index_element_size + (index_element_size * 5)], &i1, index_element_size);
					ti += 6;
					vi += 1;
				}
				vi += 1;
			}

			// GDExtension provides only one interface for creating a surface
			// It must be done through mesh_add_surface_from_arrays or mesh_add_surface
			// Both of these require "raw" data - it is then converted to GL data
			constexpr uint64_t surface_format =
				godot::RenderingServer::ARRAY_FORMAT_VERTEX |
				godot::RenderingServer::ARRAY_FORMAT_NORMAL |
				godot::RenderingServer::ARRAY_FORMAT_TANGENT |
				godot::RenderingServer::ARRAY_FORMAT_COLOR |
				godot::RenderingServer::ARRAY_FORMAT_TEX_UV |
				godot::RenderingServer::ARRAY_FORMAT_INDEX |
				godot::RenderingServer::ARRAY_FLAG_FORMAT_CURRENT_VERSION;

			godot::PackedByteArray temp_vertex_data;
			temp_vertex_data.resize(VERTEX_ELEMENT_SIZE * vertex_count);

			godot::PackedByteArray temp_attrib_data;
			temp_attrib_data.resize(ATTRIB_ELEMENT_SIZE * vertex_count);
			
			// Required fields to create a surface
			godot::Dictionary surface_dict;
			surface_dict["primitive"] = godot::RenderingServer::PrimitiveType::PRIMITIVE_TRIANGLES;
			surface_dict["format"] = surface_format;
			surface_dict["vertex_data"] = temp_vertex_data;
			surface_dict["vertex_count"] = vertex_count;
			surface_dict["attribute_data"] = temp_attrib_data;
			surface_dict["index_data"] = indices;
			surface_dict["index_count"] = index_count;
			surface_dict["aabb"] = godot::AABB();

			rserver->mesh_clear(mesh_id);
			rserver->mesh_add_surface(mesh_id, surface_dict);
			rserver->mesh_surface_set_material(mesh_id, 0, material_id);

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

			if (pserver != nullptr)
			{
				collider_shape_data.resize(vertex_count);
			}
		}

		const auto vertices_per_side = get_vertices_per_side();
		const auto quad_size = get_quad_size();

		auto surface_vertex_buffer_p = surface_vertex_buffer.ptrw();
		auto surface_attribute_buffer_p = surface_attribute_buffer.ptrw();

		collider_shape_min_height = std::numeric_limits<godot::real_t>::max();
		collider_shape_max_height = std::numeric_limits<godot::real_t>::min();

		// The first point will always be at 0,0,0
		auto aabb = godot::AABB(godot::Vector3(), godot::Vector3());

		for (int64_t z = 0; z < vertices_per_side; ++z)
		{
			for (int64_t x = 0; x < vertices_per_side; ++x)
			{
				const auto i = (x % vertices_per_side) + (z * vertices_per_side);
				const auto px = x * quad_size;
				const auto pz = z * quad_size;
				if (flags & REBUILD_HEIGHTMAP)
				{
					auto position = godot::Vector3(px, bilinear_sample(heightmap, local_position_to_image_position(godot::Vector3(px, 0.0, pz))).r, pz);
					auto normal = compress_normal(godot::Vector3(0.0, 1.0, 0.0));
					memcpy(&surface_vertex_buffer_p[i * surface_vertex_stride + surface_offsets[godot::Mesh::ARRAY_VERTEX]], &position, ELEMENT_SIZE_POSITION);
					memcpy(&surface_vertex_buffer_p[i * surface_normal_tangent_stride + surface_offsets[godot::Mesh::ARRAY_NORMAL]], &normal, ELEMENT_SIZE_NORMAL_TANGENT);
					aabb.expand_to(position);
					if (pserver != nullptr)
					{
						collider_shape_data.set(i, position.y);
						collider_shape_min_height = godot::Math::min(position.y, collider_shape_min_height);
						collider_shape_max_height = godot::Math::max(position.y, collider_shape_max_height);
					}
				}
				if (flags & REBUILD_UV)
				{
					auto uv = godot::Vector2(px, pz) * texture_size;
					memcpy(&surface_attribute_buffer_p[i * surface_attribute_stride + surface_offsets[godot::Mesh::ARRAY_TEX_UV]], &uv, ELEMENT_SIZE_UV);
				}
				if (flags & REBUILD_SPLATMAP)
				{
					auto color = bilinear_sample(splatmap, local_position_to_image_position(godot::Vector3(px, 0.0, pz))).to_abgr32();
					memcpy(&surface_attribute_buffer_p[i * surface_attribute_stride + surface_offsets[godot::Mesh::ARRAY_COLOR]], &color, ELEMENT_SIZE_COLOR);
				}
			}
		}
		
		if (flags & REBUILD_HEIGHTMAP)
		{
			rserver->mesh_surface_update_vertex_region(mesh_id, 0, 0, surface_vertex_buffer);
			rserver->mesh_set_custom_aabb(mesh_id, aabb);

			if (pserver != nullptr)
			{
				godot::Dictionary collider_dict;
				collider_dict["width"] = vertices_per_side;
				collider_dict["depth"] = vertices_per_side;
				collider_dict["heights"] = collider_shape_data;
				collider_dict["min_height"] = collider_shape_min_height;
				collider_dict["max_height"] = collider_shape_max_height;
				pserver->shape_set_data(collider_shape_id, collider_dict);

				// Update transform/scale of collider shape
				constexpr godot::real_t COLLIDER_QUAD_SIZE = 1.0;
				const auto collider_size = COLLIDER_QUAD_SIZE * get_quads_per_side();
				const auto scale = mesh_size / collider_size;
				auto collider_shape_transform = godot::Transform3D(
					godot::Basis::from_scale(godot::Vector3(scale, 1.0, scale)),
					godot::Vector3(get_half_mesh_size(), 0.0, get_half_mesh_size()));
				pserver->body_set_shape_transform(collider_body_id, 0, collider_shape_transform);
			}

			update_gizmos();
		}
		if ((flags & REBUILD_UV) || (flags & REBUILD_SPLATMAP))
		{
			rserver->mesh_surface_update_attribute_region(mesh_id, 0, 0, surface_attribute_buffer);
		}
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
	rebuild(REBUILD_ALL);
}

void SimpleHeightmap::set_mesh_resolution(const godot::real_t value)
{
	mesh_resolution = godot::Math::max(value, static_cast<godot::real_t>(0.1));
	rebuild(REBUILD_ALL);
}

void SimpleHeightmap::set_image_size(int value)
{
	image_size = godot::Math::max(value, 1);
	if (heightmap.is_valid())
		heightmap->resize(image_size, image_size);
	if (splatmap.is_valid())
		splatmap->resize(image_size, image_size);
	rebuild(REBUILD_ALL);
}

void SimpleHeightmap::set_texture_size(const godot::real_t value)
{
	texture_size = godot::Math::max(value, static_cast<godot::real_t>(0.01));
	rebuild(REBUILD_UV);
}

void SimpleHeightmap::set_heightmap_image(const godot::Ref<godot::Image>& new_heightmap)
{
	heightmap = new_heightmap;
	initialize_image(heightmap, godot::Image::FORMAT_RF, image_size);
	rebuild(REBUILD_ALL);
}

void SimpleHeightmap::set_splatmap_image(const godot::Ref<godot::Image>& new_splatmap)
{
	splatmap = new_splatmap;
	initialize_image(splatmap, godot::Image::FORMAT_RGBA8, image_size, godot::Color(1.0, 0.0, 0.0, 0.0));
	rebuild(REBUILD_ALL);
}

void SimpleHeightmap::set_texture_1(const godot::Ref<godot::Texture2D>& new_texture)
{
	texture_1 = new_texture;
	update_material_texture_parameter(default_texture_1_param, texture_1);
	emit_signal("texture_1_changed", texture_1);
}

void SimpleHeightmap::set_texture_2(const godot::Ref<godot::Texture2D>& new_texture)
{
	texture_2 = new_texture;
	update_material_texture_parameter(default_texture_2_param, texture_2);
	emit_signal("texture_2_changed", texture_2);
}

void SimpleHeightmap::set_texture_3(const godot::Ref<godot::Texture2D>& new_texture)
{
	texture_3 = new_texture;
	update_material_texture_parameter(default_texture_3_param, texture_3);
	emit_signal("texture_3_changed", texture_3);
}

void SimpleHeightmap::set_texture_4(const godot::Ref<godot::Texture2D>& new_texture)
{
	texture_4 = new_texture;
	update_material_texture_parameter(default_texture_4_param, texture_4);
	emit_signal("texture_4_changed", texture_4);
}

void SimpleHeightmap::update_material_texture_parameter(const char* parameter_name, const godot::Ref<godot::Texture2D>& texture)
{
	const auto rserver = godot::RenderingServer::get_singleton();
	if (rserver != nullptr && material_id.is_valid())
	{
		rserver->material_set_param(material_id, parameter_name, texture.is_valid() ? texture->get_rid() : godot::RID());
	}
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