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
	auto mouse_event = Ref<InputEventMouse>(p_event);
	if (mouse_event.is_valid())
	{
		const auto result = perform_raycast(p_viewport_camera, mouse_event->get_position());
		const auto hit_heightmap = get_heightmap_from_collider(Object::cast_to<Node>(result["collider"]));
		if (hit_heightmap != nullptr && hit_heightmap == selected_heightmap)
		{
			// Collect points to affect
			const auto collision_point = static_cast<Vector3>(result["position"]);
			const auto affected_pixel_coordinates = hit_heightmap->get_pixel_coordinates_in_range(collision_point, heightmap_panel->get_brush_radius());

			// Handle input and Update gizmos
			auto mouse_button_event = Ref<InputEventMouseButton>(mouse_event);
			auto adjust_terrain = mouse_button_event != nullptr && mouse_button_event->get_button_index() == MOUSE_BUTTON_LEFT && mouse_button_event->is_pressed();
			int32_t i = 0;
			for (const auto& pixel_coordinate : affected_pixel_coordinates)
			{
				const auto global_position = hit_heightmap->pixel_coordinates_to_global_position(pixel_coordinate);
				const auto d = global_position.distance_to(collision_point);
				const auto p = Math::max(static_cast<real_t>(1.0) - (d / heightmap_panel->get_brush_radius()), (real_t)0.0);

				// Adjust terrain
				if (adjust_terrain)
				{
					hit_heightmap->adjust_height(pixel_coordinate, (real_t)0.1 * p);
				}

				// Update Gizmos
				if (i < gizmo_multimesh->get_instance_count())
				{
					const auto scale = Math::max(p, (real_t)0.0);
					Transform3D t;
					t.set_basis(Basis(Quaternion(), Vector3(scale, scale, scale)));
					t.set_origin(global_position);
					gizmo_multimesh->set_instance_transform(i, t);
				}
				++i;
			}
			gizmo_multimesh->set_visible_instance_count(Math::min(i, gizmo_multimesh->get_instance_count()));
			gizmo->set_visible(true);

			if (adjust_terrain)
			{
				hit_heightmap->rebuild();
				return AFTER_GUI_INPUT_STOP;
			}
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