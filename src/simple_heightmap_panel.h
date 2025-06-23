#pragma once

#ifdef TOOLS_ENABLED

#include <godot_cpp/classes/editor_spin_slider.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace godot
{
	class Button;

	class SimpleHeightmapPanel : public VBoxContainer
	{
		GDCLASS(SimpleHeightmapPanel, VBoxContainer);
	
	public:
		static void _bind_methods();

		SimpleHeightmapPanel();
		~SimpleHeightmapPanel();

		[[nodiscard]] real_t get_brush_size() const { return brush_size; }

	private:
		enum Tools
		{
			TOOL_RAISE,
			TOOL_LOWER,
			TOOL_SMOOTH,
			TOOL_SET,
			TOOL_MAX
		};

		void create_tool_button(Control* parent, Tools tool, const char* pname);

		void tool_button_pressed(int tool);
		void brush_size_changed();

		Button* tool_button[TOOL_MAX];

		EditorSpinSlider* brush_size_slider = nullptr;

		real_t brush_size;
	};
}

#endif // TOOLS_ENABLED