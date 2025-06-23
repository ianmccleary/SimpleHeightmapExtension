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

#include "simple_heightmap.h"

using namespace godot;

void SimpleHeightmapEditorPlugin::_bind_methods()
{
}

void SimpleHeightmapEditorPlugin::_enter_tree()
{
	box_mesh = Ref<BoxMesh>(memnew(BoxMesh));
	box_mesh->set_size(Vector3(1.0, 1.0, 1.0));

	gizmo = memnew(MeshInstance3D);
	gizmo->set_mesh(box_mesh);
	gizmo->set_visible(false);
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

	box_mesh = Ref<BoxMesh>();
}

bool SimpleHeightmapEditorPlugin::_handles(Object *p_object) const
{
	return Object::cast_to<SimpleHeightmap>(p_object) != nullptr;
}

void SimpleHeightmapEditorPlugin::_make_visible(bool p_visible)
{
	// TODO
	printf("make_visible");
}

void SimpleHeightmapEditorPlugin::_edit(Object *p_object)
{
	selected_heightmap = Object::cast_to<SimpleHeightmap>(p_object);
	gizmo->set_visible(selected_heightmap != nullptr);
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
			auto collision_point = static_cast<Vector3>(result["position"]);
			gizmo->set_global_position(hit_heightmap->snap_world_position_to_pixel(collision_point));
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