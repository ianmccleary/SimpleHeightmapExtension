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

	auto tools_hbox_top = memnew(HBoxContainer);
	add_child(tools_hbox_top);
	create_tool_button(tools_hbox_top, TOOL_RAISE, "Raise");
	create_tool_button(tools_hbox_top, TOOL_LOWER, "Lower");

	auto tools_hbox_bottom = memnew(HBoxContainer);
	add_child(tools_hbox_bottom);
	create_tool_button(tools_hbox_bottom, TOOL_SMOOTH, "Smooth");
	create_tool_button(tools_hbox_bottom, TOOL_SET, "Set");

	add_child(memnew(VSeparator));

	auto brush_size_label = memnew(Label);
	brush_size_label->set_text("Brush Size");
	add_child(brush_size_label);

	brush_size_slider = memnew(EditorSpinSlider);
	brush_size_slider->set_max(25.0);
	brush_size_slider->set_min(0.0);
	brush_size_slider->set_step(0.1);
	brush_size_slider->set_allow_greater(true);
	brush_size_slider->set_value(brush_size);
	brush_size_slider->connect("value_changed", callable_mp(this, &SimpleHeightmapPanel::brush_size_changed).unbind(1));
	add_child(brush_size_slider);
}

void SimpleHeightmapPanel::create_tool_button(Control* parent, Tools tool, const char* pname)
{
	auto button = memnew(Button);
	button->set_text(pname);
	button->set_toggle_mode(true);
	button->set_theme_type_variation("FlatButton");
	button->set_pressed(false);
	button->connect("pressed", callable_mp(this, &SimpleHeightmapPanel::tool_button_pressed).bind(tool));
	tool_button[tool] = button;
	parent->add_child(button);
}

void SimpleHeightmapPanel::tool_button_pressed(int tool)
{
	for (int i = 0; i < TOOL_MAX; ++i)
	{
		tool_button[i]->set_pressed(i == tool);
	}
}

void SimpleHeightmapPanel::brush_size_changed()
{
	if (brush_size_slider != nullptr)
		brush_size = brush_size_slider->get_value();
}

SimpleHeightmapPanel::~SimpleHeightmapPanel()
{ }

#endif // TOOLS_ENABLED