#ifdef TOOLS_ENABLED

#include "simple_heightmap_panel.h"
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/v_separator.hpp>


using namespace godot;

void SimpleHeightmapPanel::_bind_methods()
{

}

SimpleHeightmapPanel::SimpleHeightmapPanel()
{
	brush_size = 5.0;
	brush_strength = 1.0;

	auto tools_hbox_top = memnew(HBoxContainer);
	add_child(tools_hbox_top);
	create_tool_button(tools_hbox_top, TOOL_RAISE, "Raise", true);
	create_tool_button(tools_hbox_top, TOOL_LOWER, "Lower");

	auto tools_hbox_bottom = memnew(HBoxContainer);
	add_child(tools_hbox_bottom);
	create_tool_button(tools_hbox_bottom, TOOL_SMOOTH, "Smooth");
	create_tool_button(tools_hbox_bottom, TOOL_SET, "Set");

	add_child(memnew(VSeparator));

	create_label(this, "Brush Size");
	auto brush_size_slider = memnew(EditorSpinSlider);
	brush_size_slider->set_max(25.0);
	brush_size_slider->set_min(0.0);
	brush_size_slider->set_step(0.1);
	brush_size_slider->set_allow_greater(true);
	brush_size_slider->set_value(brush_size);
	brush_size_slider->connect("value_changed", callable_mp(this, &SimpleHeightmapPanel::brush_size_changed));
	add_child(brush_size_slider);

	create_label(this, "Brush Strength");
	auto brush_strength_slider = memnew(EditorSpinSlider);
	brush_strength_slider->set_max(10.0);
	brush_strength_slider->set_min(0.0);
	brush_strength_slider->set_step(0.1);
	brush_strength_slider->set_allow_greater(true);
	brush_strength_slider->set_value(brush_strength);
	brush_strength_slider->connect("value_changed", callable_mp(this, &SimpleHeightmapPanel::brush_strength_changed));
	add_child(brush_strength_slider);
}

Button* SimpleHeightmapPanel::create_tool_button(Control* parent, Tools tool, const char* text, bool pressed)
{
	if (parent != nullptr)
	{
		auto button = memnew(Button);
		button->set_text(text);
		button->set_toggle_mode(true);
		button->set_theme_type_variation("FlatButton");
		button->set_pressed(pressed);
		button->connect("pressed", callable_mp(this, &SimpleHeightmapPanel::tool_button_pressed).bind(tool));
		tool_button[tool] = button;
		parent->add_child(button);
		return button;
	}
	return nullptr;
}

Label* SimpleHeightmapPanel::create_label(Control* parent, const char* text)
{
	if (parent != nullptr)
	{
		auto label = memnew(Label);
		label->set_text(text);
		parent->add_child(label);
		return label;
	}
	return nullptr;
}

void SimpleHeightmapPanel::tool_button_pressed(int tool)
{
	for (int i = 0; i < TOOL_MAX; ++i)
	{
		tool_button[i]->set_pressed(i == tool);
	}
}

void SimpleHeightmapPanel::brush_size_changed(double value)
{
	brush_size = value;
}

void SimpleHeightmapPanel::brush_strength_changed(double value)
{
	brush_strength = value;
}

SimpleHeightmapPanel::~SimpleHeightmapPanel()
{ }

#endif // TOOLS_ENABLED