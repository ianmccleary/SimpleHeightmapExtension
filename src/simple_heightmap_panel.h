#pragma once

#ifdef TOOLS_ENABLED

#include <godot_cpp/classes/editor_spin_slider.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace godot
{
	class Button;
	class Label;

	class SimpleHeightmapPanel : public VBoxContainer
	{
		GDCLASS(SimpleHeightmapPanel, VBoxContainer);
	
	public:
		static void _bind_methods();

		SimpleHeightmapPanel();
		~SimpleHeightmapPanel();

		enum Tools
		{
			TOOL_RAISE,
			TOOL_LOWER,
			TOOL_SMOOTH,
			TOOL_FLATTEN,
			TOOL_MAX
		};

		[[nodiscard]] real_t get_brush_diameter() const { return brush_size; }
		[[nodiscard]] real_t get_brush_radius() const { return brush_size * static_cast<real_t>(0.5); }
		[[nodiscard]] real_t get_brush_exp() const { return brush_exp; }
		[[nodiscard]] real_t get_brush_strength() const { return brush_strength; }
		[[nodiscard]] Tools get_current_tool() const { return current_tool; }

	private:

		[[maybe_unused]] Button* create_tool_button(Control* parent, Tools tool, const char* text, bool pressed = false);
		[[maybe_unused]] Label* create_label(Control* parent, const char* text);

		void tool_button_pressed(int tool);
		void brush_size_changed(double value);
		void brush_strength_changed(double value);

		Button* tool_button[TOOL_MAX];

		real_t brush_size;
		real_t brush_strength;
		real_t brush_exp;
		Tools current_tool;
	};
}

#endif // TOOLS_ENABLED