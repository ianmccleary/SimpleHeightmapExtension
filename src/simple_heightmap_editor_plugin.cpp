#ifdef TOOLS_ENABLED
#include "simple_heightmap_editor_plugin.h"
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/box_mesh.hpp>

#include "simple_heightmap.h"

using namespace godot;

void SimpleHeightmapEditorPlugin::_bind_methods()
{
}

void SimpleHeightmapEditorPlugin::_enter_tree()
{
	auto box_mesh = Ref<BoxMesh>(memnew(BoxMesh));
	box_mesh->set_size(Vector3(1.0, 1.0, 1.0));

	gizmo_multimesh = Ref<MultiMesh>(memnew(MultiMesh));
	gizmo_multimesh->set_transform_format(MultiMesh::TRANSFORM_3D);
	gizmo_multimesh->set_instance_count(64 * 64);
	gizmo_multimesh->set_visible_instance_count(0);
	gizmo_multimesh->set_mesh(box_mesh);

	gizmo = memnew(MultiMeshInstance3D);
	gizmo->set_multimesh(gizmo_multimesh);
	add_child(gizmo);

	heightmap_panel = memnew(SimpleHeightmapPanel);
	add_control_to_container(CONTAINER_SPATIAL_EDITOR_SIDE_RIGHT, heightmap_panel);
}

void SimpleHeightmapEditorPlugin::_exit_tree()
{
	remove_control_from_container(CONTAINER_SPATIAL_EDITOR_SIDE_RIGHT, heightmap_panel);
	heightmap_panel->queue_free();
	heightmap_panel = nullptr;

	gizmo->queue_free();
	gizmo = nullptr;

	gizmo_multimesh = Ref<BoxMesh>();
}

bool SimpleHeightmapEditorPlugin::_handles(Object *p_object) const
{
	return Object::cast_to<SimpleHeightmap>(p_object) != nullptr;
}

void SimpleHeightmapEditorPlugin::_make_visible(bool p_visible)
{
	// TODO
}

void SimpleHeightmapEditorPlugin::_edit(Object *p_object)
{
	selected_heightmap = Object::cast_to<SimpleHeightmap>(p_object);
}

namespace
{
	godot::Dictionary perform_raycast(Camera3D* camera, Vector2 mouse_position)
	{
		const auto world = camera ? camera->get_world_3d() : nullptr;
		const auto space_state = world.is_valid() ? world->get_direct_space_state() : nullptr;
		if (camera && space_state)
		{
			const auto a = camera->project_ray_origin(mouse_position);
			const auto b = a + camera->project_ray_normal(mouse_position) * 1000.0;
			const auto query = PhysicsRayQueryParameters3D::create(a, b);
			return space_state->intersect_ray(query);
		}
		return godot::Dictionary();
	}

	SimpleHeightmap* get_heightmap_from_collider(Node* collider)
	{
		if (collider != nullptr && collider->is_inside_tree())
		{
			return Object::cast_to<SimpleHeightmap>(collider->get_parent());
		}
		return nullptr;
	}
}

int32_t SimpleHeightmapEditorPlugin::_forward_3d_gui_input(Camera3D* p_viewport_camera, const Ref<InputEvent>& p_event)
{
	auto mouse_button_event = Ref<InputEventMouseButton>(p_event);
	if (mouse_button_event.is_valid())
	{
		if (selected_heightmap != nullptr)
		{
			// ...
			// ...
			// ...
		}
	}

	auto mouse_move_event = Ref<InputEventMouseMotion>(p_event);
	if (mouse_move_event.is_valid())
	{
		const auto result = perform_raycast(p_viewport_camera, mouse_move_event->get_position());
		const auto hit_heightmap = get_heightmap_from_collider(Object::cast_to<Node>(result["collider"]));
		if (hit_heightmap == selected_heightmap)
		{
			// Show gizmos
			const auto brush_size_radius = heightmap_panel->get_brush_size() * static_cast<real_t>(0.5);

			const auto collision_point = static_cast<Vector3>(result["position"]);
			const auto pixels = hit_heightmap->get_pixel_coordinates_in_range(collision_point, brush_size_radius);	
			
			const auto count = Math::min(gizmo_multimesh->get_instance_count(), (int32_t)pixels.size());
			gizmo_multimesh->set_visible_instance_count(count);
			for (size_t i = 0; i < count; ++i)
			{
				const auto position = hit_heightmap->to_global(hit_heightmap->pixel_coordinates_to_local_position(pixels[i]));

				const auto distance_to_collision_point = position.distance_to(collision_point);
				const auto scale = static_cast<real_t>(1.0) - (distance_to_collision_point / brush_size_radius);

				Transform3D t;
				t.set_basis(Basis(Quaternion(), Vector3(scale, scale, scale))); // TODO: Scale based on distance to collision point
				t.set_origin(hit_heightmap->to_global(hit_heightmap->pixel_coordinates_to_local_position(pixels[i])));
				gizmo_multimesh->set_instance_transform(i, t);
			}
			gizmo->set_visible(true);
		}
		else
		{
			// Hide gizmos
			gizmo->set_visible(false);
		}
	}
	return AFTER_GUI_INPUT_PASS;
}

#endif // TOOLS_ENABLED