#ifdef TOOLS_ENABLED
#include "simple_heightmap_editor_plugin.h"
#include "simple_heightmap.h"

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_spin_slider.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/world3d.hpp>

void SimpleHeightmapEditorPlugin::_bind_methods()
{
}

void SimpleHeightmapEditorPlugin::_enter_tree()
{
	auto box_mesh = godot::Ref<godot::BoxMesh>(memnew(godot::BoxMesh));
	box_mesh->set_size(godot::Vector3(1.0, 1.0, 1.0));

	gizmo_multimesh = godot::Ref<godot::MultiMesh>(memnew(godot::MultiMesh));
	gizmo_multimesh->set_transform_format(godot::MultiMesh::TRANSFORM_3D);
	gizmo_multimesh->set_instance_count(64 * 64);
	gizmo_multimesh->set_visible_instance_count(0);
	gizmo_multimesh->set_mesh(box_mesh);

	gizmo = memnew(godot::MultiMeshInstance3D);
	gizmo->set_multimesh(gizmo_multimesh);
	add_child(gizmo);

	selected_tool = Tool::Heightmap_Raise;

	brush_radius = 5.0;
	brush_strength = 1.0;
	brush_ease = 1.0;

	create_ui();
}

namespace UIHelpers
{
	godot::Button* create_button(const char* text, bool toggle_mode, bool pressed)
	{
		constexpr auto THEME_FLAT_BUTTON = "FlatButton";

		auto button = memnew(godot::Button);
		button->set_text(text);
		button->set_toggle_mode(toggle_mode);
		button->set_pressed(pressed);
		if (toggle_mode)
		{
			button->set_theme_type_variation(THEME_FLAT_BUTTON);
		}
		button->set_pressed(false);
		return button;
	}

	godot::Label* create_label(const char* text)
	{
		auto label = memnew(godot::Label);
		label->set_text(text);
		return label;
	}

	godot::EditorSpinSlider* create_editor_spin_slider(double value, double min, double max, double step, bool allow_greater)
	{
		auto slider = memnew(godot::EditorSpinSlider);
		slider->set_min(min);
		slider->set_max(max);
		slider->set_step(step);
		slider->set_value(value);
		slider->set_allow_greater(allow_greater);
		return slider;
	}
}

void SimpleHeightmapEditorPlugin::create_ui()
{
	if (ui == nullptr)
	{
		constexpr auto SIGNAL_PRESSED = "pressed";
		constexpr auto SIGNAL_VALUE_CHANGED = "value_changed";

		ui = memnew(godot::VBoxContainer);

		auto hbox_a = memnew(godot::HBoxContainer);
		auto hbox_b = memnew(godot::HBoxContainer);

		button_raise = UIHelpers::create_button("Raise/Lower", true, true);
		button_smooth = UIHelpers::create_button("Smooth", true, false);
		button_flatten = UIHelpers::create_button("Flatten", true, false);
		button_texture_1 = UIHelpers::create_button("Texture 1", true, false);
		button_texture_2 = UIHelpers::create_button("Texture 2", true, false);
		button_texture_3 = UIHelpers::create_button("Texture 3", true, false);
		button_texture_4 = UIHelpers::create_button("Texture 4", true, false);

		auto radius_slider = UIHelpers::create_editor_spin_slider(brush_radius, 0.0, 25.0, 0.1, true);
		auto strength_slider = UIHelpers::create_editor_spin_slider(brush_strength, 0.0, 10.0, 0.1, true);
		auto ease_slider = UIHelpers::create_editor_spin_slider(brush_ease, 0.0, 2.0, 0.01, true);

		hbox_a->add_child(button_raise);
		hbox_a->add_child(button_smooth);
		hbox_a->add_child(button_flatten);
		ui->add_child(hbox_a);
		
		hbox_b->add_child(button_texture_1);
		hbox_b->add_child(button_texture_2);
		hbox_b->add_child(button_texture_3);
		hbox_b->add_child(button_texture_4);
		ui->add_child(hbox_b);
		
		ui->add_child(UIHelpers::create_label("Radius"));
		ui->add_child(radius_slider);

		ui->add_child(UIHelpers::create_label("Strength"));
		ui->add_child(strength_slider);

		ui->add_child(UIHelpers::create_label("Ease"));
		ui->add_child(ease_slider);

		button_raise->connect(SIGNAL_PRESSED, callable_mp(this, &SimpleHeightmapEditorPlugin::on_tool_selected).bind(static_cast<uint8_t>(Tool::Heightmap_Raise)));
		button_smooth->connect(SIGNAL_PRESSED, callable_mp(this, &SimpleHeightmapEditorPlugin::on_tool_selected).bind(static_cast<uint8_t>(Tool::Heightmap_Smooth)));
		button_flatten->connect(SIGNAL_PRESSED, callable_mp(this, &SimpleHeightmapEditorPlugin::on_tool_selected).bind(static_cast<uint8_t>(Tool::Heightmap_Flatten)));
		button_texture_1->connect(SIGNAL_PRESSED, callable_mp(this, &SimpleHeightmapEditorPlugin::on_tool_selected).bind(static_cast<uint8_t>(Tool::Splatmap_Texture1)));
		button_texture_2->connect(SIGNAL_PRESSED, callable_mp(this, &SimpleHeightmapEditorPlugin::on_tool_selected).bind(static_cast<uint8_t>(Tool::Splatmap_Texture2)));
		button_texture_3->connect(SIGNAL_PRESSED, callable_mp(this, &SimpleHeightmapEditorPlugin::on_tool_selected).bind(static_cast<uint8_t>(Tool::Splatmap_Texture3)));
		button_texture_4->connect(SIGNAL_PRESSED, callable_mp(this, &SimpleHeightmapEditorPlugin::on_tool_selected).bind(static_cast<uint8_t>(Tool::Splatmap_Texture4)));
		radius_slider->connect(SIGNAL_VALUE_CHANGED, callable_mp(this, &SimpleHeightmapEditorPlugin::on_brush_radius_changed));
		strength_slider->connect(SIGNAL_VALUE_CHANGED, callable_mp(this, &SimpleHeightmapEditorPlugin::on_brush_strength_changed));
		ease_slider->connect(SIGNAL_VALUE_CHANGED, callable_mp(this, &SimpleHeightmapEditorPlugin::on_brush_ease_changed));
	}
}

void SimpleHeightmapEditorPlugin::on_tool_selected(uint8_t tool_index)
{
	const auto tool = static_cast<Tool>(tool_index);
	selected_tool = tool;
	button_raise->set_pressed(selected_tool == Tool::Heightmap_Raise);
	button_smooth->set_pressed(selected_tool == Tool::Heightmap_Smooth);
	button_flatten->set_pressed(selected_tool == Tool::Heightmap_Flatten);
	button_texture_1->set_pressed(selected_tool == Tool::Splatmap_Texture1);
	button_texture_2->set_pressed(selected_tool == Tool::Splatmap_Texture2);
	button_texture_3->set_pressed(selected_tool == Tool::Splatmap_Texture3);
	button_texture_4->set_pressed(selected_tool == Tool::Splatmap_Texture4);
}

void SimpleHeightmapEditorPlugin::on_brush_radius_changed(double value)
{
	brush_radius = value;
}

void SimpleHeightmapEditorPlugin::on_brush_strength_changed(double value)
{
	brush_strength = value;
}

void SimpleHeightmapEditorPlugin::on_brush_ease_changed(double value)
{
	brush_ease = value;
}

void SimpleHeightmapEditorPlugin::_exit_tree()
{
	destroy_ui();

	gizmo->queue_free();
	gizmo = nullptr;

	gizmo_multimesh = godot::Ref<godot::BoxMesh>();
}

void SimpleHeightmapEditorPlugin::destroy_ui()
{
	if (ui != nullptr)
	{
		if (ui->is_inside_tree())
		{
			remove_control_from_container(CONTAINER_SPATIAL_EDITOR_SIDE_RIGHT, ui);
		}
		ui->queue_free();
		ui = nullptr;
	}
}

bool SimpleHeightmapEditorPlugin::_handles(godot::Object *p_object) const
{
	return godot::Object::cast_to<SimpleHeightmap>(p_object) != nullptr;
}

void SimpleHeightmapEditorPlugin::_make_visible(bool p_visible)
{
	// TODO
}

void SimpleHeightmapEditorPlugin::_edit(godot::Object *p_object)
{
	selected_heightmap = godot::Object::cast_to<SimpleHeightmap>(p_object);

	// Hide gizmo when de-selecting
	gizmo->set_visible(selected_heightmap != nullptr);

	// Only show panel if a heightmap is selected
	if (!ui->is_inside_tree() && selected_heightmap != nullptr)
		add_control_to_container(CONTAINER_SPATIAL_EDITOR_SIDE_RIGHT, ui);
	else if (ui->is_inside_tree() && selected_heightmap == nullptr)
		remove_control_from_container(CONTAINER_SPATIAL_EDITOR_SIDE_RIGHT, ui);
}

namespace
{
	bool try_pick_heightmap(const godot::Camera3D& camera, const godot::Vector2& mouse_position, const SimpleHeightmap* expected_heightmap, godot::Vector3& out_collision)
	{
		out_collision = godot::Vector3();

		const auto world = camera.get_world_3d();
		const auto space_state = world.is_valid() ? world->get_direct_space_state() : nullptr;
		if (space_state != nullptr)
		{
			const auto a = camera.project_ray_origin(mouse_position);
			const auto b = a + camera.project_ray_normal(mouse_position) * 1000.0;
			const auto raycast_result = space_state->intersect_ray(godot::PhysicsRayQueryParameters3D::create(a, b));

			const auto collider = godot::Object::cast_to<godot::CollisionObject3D>(raycast_result["collider"]);
			if (collider != nullptr && collider->get_parent())
			{
				const auto picked_heightmap = godot::Object::cast_to<SimpleHeightmap>(collider->get_parent());
				if (picked_heightmap != nullptr && picked_heightmap == expected_heightmap)
				{
					out_collision = static_cast<godot::Vector3>(raycast_result["position"]);
					return true;
				}
			}
		}

		return false;
	}
}

int32_t SimpleHeightmapEditorPlugin::_forward_3d_gui_input(godot::Camera3D* p_viewport_camera, const godot::Ref<godot::InputEvent>& p_event)
{
	auto mouse_event = godot::Ref<godot::InputEventMouse>(p_event);
	if (mouse_event.is_valid() && p_viewport_camera != nullptr && selected_heightmap != nullptr)
	{
		godot::Vector3 hit_position;
		if (try_pick_heightmap(*p_viewport_camera, mouse_event->get_position(), selected_heightmap, hit_position))
		{
			mouse_over = true;
			mouse_global_position = hit_position;
			mouse_image_position = selected_heightmap->global_position_to_image_position(hit_position);
		}
		else
		{
			mouse_over = false;
		}

		auto mouse_button_event = godot::Ref<godot::InputEventMouseButton>(mouse_event);
		if (mouse_button_event != nullptr)
		{
			if (mouse_button_event->get_button_index() == godot::MOUSE_BUTTON_LEFT)
			{
				if (mouse_button_event->is_pressed() && !mouse_pressed && mouse_over)
				{
					flatten_target = mouse_global_position.y;
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

	auto key_event = godot::Ref<godot::InputEventKey>(p_event);
	if (key_event.is_valid())
	{
		if (key_event->get_keycode() == godot::KEY_SHIFT)
		{
			alt_pressed = key_event->is_pressed();
			return AFTER_GUI_INPUT_STOP;
		}
	}

	return AFTER_GUI_INPUT_PASS;
}

void SimpleHeightmapEditorPlugin::_process(double p_delta)
{
	if (selected_heightmap != nullptr && selected_tool != Tool::None && mouse_over)
	{
		auto image = get_affected_image(selected_tool, *selected_heightmap);

		const auto min = godot::Vector2i(
			godot::Math::clamp(static_cast<int32_t>(godot::Math::round(mouse_image_position.x - brush_radius)), 0, image->get_width()),
			godot::Math::clamp(static_cast<int32_t>(godot::Math::round(mouse_image_position.y - brush_radius)), 0, image->get_height())
		);
		const auto max = godot::Vector2i(
			godot::Math::clamp(static_cast<int32_t>(godot::Math::round(mouse_image_position.x + brush_radius)), 0, image->get_width()),
			godot::Math::clamp(static_cast<int32_t>(godot::Math::round(mouse_image_position.y + brush_radius)), 0, image->get_height())
		);
		const auto size = max - min;

		godot::Vector<godot::Color> buffer;
		buffer.resize(size.x * size.y);

		int32_t gizmo_count = 0;

		// Operate on image, save changes to buffer
		for (auto x = 0; x < size.x; ++x)
		{
			for (auto y = 0; y < size.y; ++y)
			{
				const auto ix = x + min.x;
				const auto iy = y + min.y;
				const auto d = mouse_image_position.distance_to(godot::Vector2(ix, iy));
				const auto t = godot::UtilityFunctions::ease(godot::Math::max(1.0 - (d / brush_radius), 0.0), brush_ease);
				if (mouse_pressed)
				{
					buffer.set(x + y * size.x, modify_pixel(image, ix, iy, t, p_delta));
				}
				if (gizmo_count < gizmo_multimesh->get_instance_count())
				{
					godot::Transform3D transform;
					transform.set_basis(godot::Basis(godot::Quaternion(), godot::Vector3(t, t, t)));
					transform.set_origin(selected_heightmap->image_position_to_global_position(godot::Vector2(ix, iy)));
					gizmo_multimesh->set_instance_transform(gizmo_count, transform);
					++gizmo_count;
				}
			}
		}

		// Write changes to image
		if (mouse_pressed)
		{
			for (auto x = 0; x < size.x; ++x)
			{
				for (auto y = 0; y < size.y; ++y)
				{
					image->set_pixel(x + min.x, y + min.y, buffer[x + y * size.x]);
				}
			}
			selected_heightmap->rebuild();
		}

		gizmo_multimesh->set_visible_instance_count(godot::Math::min(gizmo_count, gizmo_multimesh->get_instance_count()));
		gizmo->set_visible(true);
	}
	else
	{
		gizmo->set_visible(false);
	}
}

godot::Color SimpleHeightmapEditorPlugin::modify_pixel(const godot::Ref<godot::Image>& image, int32_t x, int32_t y, double t, double delta)
{
	auto pixel = image->get_pixel(x, y);
	if (selected_tool == Tool::Heightmap_Raise)
	{
		if (alt_pressed)
		{
			// Lower
			pixel -= godot::Color(brush_strength, 0.0, 0.0, 0.0) * t * delta;
		}
		else
		{
			// Raise
			pixel += godot::Color(brush_strength, 0.0, 0.0, 0.0) * t * delta;
		}
	}
	else if (selected_tool == Tool::Heightmap_Smooth)
	{
		if (alt_pressed)
		{
			// Add noise
		}
		else
		{
			// Smooth
			const auto cr = get_pixel_safe(image, x + 1, y);
			const auto cu = get_pixel_safe(image, x, y + 1);
			const auto cl = get_pixel_safe(image, x - 1, y);
			const auto cd = get_pixel_safe(image, x, y - 1);
			const auto average = (pixel + cr + cu + cl + cd) * 0.2;
			pixel = color_move_towards(pixel, average, brush_strength * t * delta);
		}
	}
	else if (selected_tool == Tool::Heightmap_Flatten)
	{
		// Flatten
		pixel = color_move_towards(pixel, godot::Color(flatten_target, 0.0, 0.0, 0.0), brush_strength * t * delta);
	}
	else if (selected_tool == Tool::Splatmap_Texture1)
	{
		// Paint texture 1
		pixel = color_move_towards(pixel, godot::Color(1.0, 0.0, 0.0, 0.0), brush_strength * t * delta);
	}
	else if (selected_tool == Tool::Splatmap_Texture2)
	{
		// Paint texture 2
		pixel = color_move_towards(pixel, godot::Color(0.0, 1.0, 0.0, 0.0), brush_strength * t * delta);
	}
	else if (selected_tool == Tool::Splatmap_Texture3)
	{
		// Paint texture 3
		pixel = color_move_towards(pixel, godot::Color(0.0, 0.0, 1.0, 0.0), brush_strength * t * delta);
	}
	else if (selected_tool == Tool::Splatmap_Texture4)
	{
		// Paint texture 4
		pixel = color_move_towards(pixel, godot::Color(0.0, 0.0, 0.0, 1.0), brush_strength * t * delta);
	}
	return pixel;
}

godot::Color SimpleHeightmapEditorPlugin::get_pixel_safe(const godot::Ref<godot::Image>& image, int32_t x, int32_t y)
{
	x = godot::Math::clamp(x, 0, image->get_width() - 1);
	y = godot::Math::clamp(y, 0, image->get_height() - 1);
	return image->get_pixel(x, y);
}

godot::Color SimpleHeightmapEditorPlugin::color_move_towards(const godot::Color& input, const godot::Color& target, double delta)
{
	const auto deltaf = static_cast<godot::real_t>(delta);
	godot::Color output;
	output.r = godot::Math::move_toward(input.r, target.r, deltaf);
	output.g = godot::Math::move_toward(input.g, target.g, deltaf);
	output.b = godot::Math::move_toward(input.b, target.b, deltaf);
	output.a = godot::Math::move_toward(input.a, target.a, deltaf);
	return output;
}

#endif // TOOLS_ENABLED