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
#include "simple_heightmap_panel.h"

namespace godot
{
	class SimpleHeightmapEditorPlugin : public EditorPlugin
	{
		GDCLASS(SimpleHeightmapEditorPlugin, EditorPlugin);

	public:
		static void _bind_methods();

		void _enter_tree() override;
		void _exit_tree() override;

		String _get_plugin_name() const override { return "SimpleHeightmapEditor"; }

		void _process(double p_delta) override;
		bool _handles(Object *p_object) const override;
		void _make_visible(bool p_visible) override;
		void _edit(Object *p_object) override;

		int32_t _forward_3d_gui_input(Camera3D *p_viewport_camera, const Ref<InputEvent> &p_event) override;

	private:
		void update_gizmo();

		Vector3 mouse_global_position;
		Vector2 mouse_image_position;

		SimpleHeightmap* selected_heightmap = nullptr;
		SimpleHeightmapPanel* heightmap_panel = nullptr;

		MultiMeshInstance3D* gizmo = nullptr;
		Ref<MultiMesh> gizmo_multimesh = nullptr;

		bool mouse_over = false;
		bool mouse_pressed = false;
	};
}

#endif // TOOLS_ENABLED