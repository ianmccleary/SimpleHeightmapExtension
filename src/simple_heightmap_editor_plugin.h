#pragma once

#ifdef TOOLS_ENABLED

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/ref.hpp>

#include "simple_heightmap.h"
#include "simple_heightmap_gizmo_plugin.h"

class SimpleHeightmapEditorPlugin : public godot::EditorPlugin
{
	GDCLASS(SimpleHeightmapEditorPlugin, godot::EditorPlugin);

public:
	static void _bind_methods();

	void _enter_tree() override;
	void _exit_tree() override;

	godot::String _get_plugin_name() const override { return "SimpleHeightmapEditor"; }

	void _process(double p_delta) override;
	bool _handles(godot::Object *p_object) const override;
	void _edit(godot::Object *p_object) override;

	int32_t _forward_3d_gui_input(godot::Camera3D *p_viewport_camera, const godot::Ref<godot::InputEvent> &p_event) override;

private:
	enum class Tool : uint8_t
	{
		None,
		Heightmap_Raise,
		Heightmap_Smooth,
		Heightmap_Flatten,
		Splatmap_Texture1,
		Splatmap_Texture2,
		Splatmap_Texture3,
		Splatmap_Texture4
	};

	static bool is_heightmap_tool(Tool tool)
	{
		switch (tool)
		{
			case Tool::Heightmap_Raise:
			case Tool::Heightmap_Smooth:
			case Tool::Heightmap_Flatten:
			return true;
		}
		return false;
	}

	static bool is_splatmap_tool(Tool tool)
	{
		switch (tool)
		{
			case Tool::Splatmap_Texture1:
			case Tool::Splatmap_Texture2:
			case Tool::Splatmap_Texture3:
			case Tool::Splatmap_Texture4:
			return true;
		}
		return false;
	}

	static SimpleHeightmap::ChangeType get_change_type(Tool tool)
	{
		if (is_heightmap_tool(tool)) return SimpleHeightmap::HEIGHTMAP;
		if (is_splatmap_tool(tool)) return SimpleHeightmap::SPLATMAP;
		return SimpleHeightmap::NONE;
	}

	static godot::Ref<godot::Image> get_affected_image(Tool tool, SimpleHeightmap& heightmap)
	{
		if (is_heightmap_tool(tool)) return heightmap.get_heightmap_image();
		if (is_splatmap_tool(tool)) return heightmap.get_splatmap_image();
		return godot::Ref<godot::Image>();
	}

	void create_ui();
	void destroy_ui();

	void refresh_texture_icons();
	void refresh_texture_icon(const godot::Ref<godot::Texture2D>& texture, godot::Button* button);

	godot::Ref<godot::Texture2D> empty_texture_icon;

	void on_tool_selected(uint8_t tool);
	void on_brush_radius_changed(double value);
	void on_brush_strength_changed(double value);
	void on_brush_ease_changed(double value);

	static godot::Color get_pixel_safe(const godot::Ref<godot::Image>& image, int32_t x, int32_t y);
	static godot::Color color_move_towards(const godot::Color& input, const godot::Color& target, double delta);

	godot::Color modify_pixel(const godot::Ref<godot::Image>& image, int32_t x, int32_t y, double t, double delta);

	godot::Vector3 mouse_global_position;
	godot::Vector2 mouse_image_position;

	godot::Ref<SimpleHeightmapGizmoPlugin> gizmo_plugin;

	SimpleHeightmap* selected_heightmap = nullptr;
	Tool selected_tool = Tool::None;

	struct UndoRedoCache
	{
		godot::Ref<godot::Image> image;
		int32_t width;
		int32_t height;
		bool mipmaps;
		godot::Image::Format format;
		godot::PackedByteArray data;
	};
	UndoRedoCache undo_redo_cache;

	godot::Control* ui = nullptr;
	godot::Button* button_raise = nullptr;
	godot::Button* button_smooth = nullptr;
	godot::Button* button_flatten = nullptr;
	godot::Button* button_texture_1 = nullptr;
	godot::Button* button_texture_2 = nullptr;
	godot::Button* button_texture_3 = nullptr;
	godot::Button* button_texture_4 = nullptr;
	godot::Callable texture_1_changed_callable;
	godot::Callable texture_2_changed_callable;
	godot::Callable texture_3_changed_callable;
	godot::Callable texture_4_changed_callable;

	godot::MultiMeshInstance3D* brush_node = nullptr;
	godot::Ref<godot::MultiMesh> brush_multimesh = nullptr;
	double brush_radius;
	double brush_strength;
	double brush_ease;

	godot::real_t flatten_target;

	bool mouse_over = false;
	bool mouse_pressed = false;
	bool alt_pressed = false;
};

#endif // TOOLS_ENABLED