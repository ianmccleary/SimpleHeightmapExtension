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
	gizmo->set_visible(selected_heightmap != nullptr); // Hide when de-selecting
}

namespace
{
	bool try_pick_heightmap(const Camera3D& camera, const Vector2& mouse_position, const SimpleHeightmap* expected_heightmap, Vector3& out_collision)
	{
		out_collision = Vector3();

		const auto world = camera.get_world_3d();
		const auto space_state = world.is_valid() ? world->get_direct_space_state() : nullptr;
		if (space_state != nullptr)
		{
			const auto a = camera.project_ray_origin(mouse_position);
			const auto b = a + camera.project_ray_normal(mouse_position) * 1000.0;
			const auto raycast_result = space_state->intersect_ray(PhysicsRayQueryParameters3D::create(a, b));

			const auto collider = Object::cast_to<CollisionObject3D>(raycast_result["collider"]);
			if (collider != nullptr && collider->get_parent())
			{
				const auto picked_heightmap = Object::cast_to<SimpleHeightmap>(collider->get_parent());
				if (picked_heightmap != nullptr && picked_heightmap == expected_heightmap)
				{
					out_collision = static_cast<Vector3>(raycast_result["position"]);
					return true;
				}
			}
		}

		return false;
	}
}

void SimpleHeightmapEditorPlugin::PickedPixels::update(const SimpleHeightmap& heightmap, const Vector3& collision, real_t brush_radius)
{
	const auto local_position = heightmap.to_local(collision);

	// Convert the position and range to pixel space
	center = Vector2(
		(local_position.x / heightmap.get_mesh_size()) * heightmap.get_data_resolution(),
		(local_position.z / heightmap.get_mesh_size()) * heightmap.get_data_resolution()
	);

	// Calculate the min and max bounds
	radius = (brush_radius / heightmap.get_mesh_size()) * heightmap.get_data_resolution();

	// Gather picked coordinates
	coordinates.clear();
	const auto min = Vector2i(
		Math::clamp(static_cast<int>(Math::round(center.x - radius)), 0, heightmap.get_data_resolution()),
		Math::clamp(static_cast<int>(Math::round(center.y - radius)), 0, heightmap.get_data_resolution())
	);
	const auto max = Vector2i(
		Math::clamp(static_cast<int>(Math::round(center.x + radius)), 0, heightmap.get_data_resolution()),
		Math::clamp(static_cast<int>(Math::round(center.y + radius)), 0, heightmap.get_data_resolution())
	);
	for (auto x = min.x; x <= max.x; ++x)
	{
		for (auto y = min.y; y <= max.y; ++y)
		{
			coordinates.push_back(Vector2i(x, y));
		}
	}
}

int32_t SimpleHeightmapEditorPlugin::_forward_3d_gui_input(Camera3D* p_viewport_camera, const Ref<InputEvent>& p_event)
{
	auto mouse_event = Ref<InputEventMouse>(p_event);
	if (mouse_event.is_valid() && p_viewport_camera != nullptr && selected_heightmap != nullptr)
	{
		Vector3 hit_position;
		if (try_pick_heightmap(*p_viewport_camera, mouse_event->get_position(), selected_heightmap, hit_position))
		{
			mouse_over = true;

			// Update picked pixels
			picked_pixels.update(*selected_heightmap, hit_position, heightmap_panel->get_brush_radius());

			// Update gizmos
			update_gizmo();
		}
		else
		{
			mouse_over = false;
			gizmo->set_visible(false);
		}

		auto mouse_button_event = Ref<InputEventMouseButton>(mouse_event);
		if (mouse_button_event != nullptr)
		{
			if (mouse_button_event->get_button_index() == MOUSE_BUTTON_LEFT)
			{
				if (mouse_button_event->is_pressed() && !mouse_pressed && mouse_over)
				{
					mouse_pressed = true;
					return AFTER_GUI_INPUT_STOP;
				}
				else if (mouse_button_event->is_released() && mouse_pressed)
				{
					mouse_pressed = false;
					return AFTER_GUI_INPUT_STOP;
				}
			}
		}
	}
	return AFTER_GUI_INPUT_PASS;
}

void SimpleHeightmapEditorPlugin::_process(double p_delta)
{
	if (selected_heightmap != nullptr && mouse_pressed && mouse_over)
	{
		// Adjust terrain if mouse is pressed
		for (const auto& coordinate : picked_pixels.coordinates)
		{
			const auto d = picked_pixels.center.distance_to(coordinate);
			const auto p = Math::max((real_t)1.0 - (d / picked_pixels.radius), (real_t)0.0);
			selected_heightmap->adjust_height(coordinate, (real_t)1.0 * p_delta * p);
		}
		selected_heightmap->rebuild();
		update_gizmo();
	}
}

void SimpleHeightmapEditorPlugin::update_gizmo()
{
	if (selected_heightmap != nullptr)
	{
		int32_t i = 0;
		for (const auto& coordinate : picked_pixels.coordinates)
		{
			const auto d = picked_pixels.center.distance_to(coordinate);
			const auto p = Math::max(static_cast<real_t>(1.0) - (d / picked_pixels.radius), (real_t)0.0);
			if (i < gizmo_multimesh->get_instance_count())
			{
				const auto scale = Math::max(p, (real_t)0.0);
				Transform3D t;
				t.set_basis(Basis(Quaternion(), Vector3(scale, scale, scale)));
				t.set_origin(selected_heightmap->pixel_coordinates_to_global_position(coordinate));
				gizmo_multimesh->set_instance_transform(i, t);
			}
			++i;
		}
		gizmo_multimesh->set_visible_instance_count(Math::min(i, gizmo_multimesh->get_instance_count()));
		gizmo->set_visible(true);
	}
	else
	{
		gizmo->set_visible(false);
	}
}

#endif // TOOLS_ENABLED