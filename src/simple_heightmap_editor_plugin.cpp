#ifdef TOOLS_ENABLED
#include "simple_heightmap_editor_plugin.h"
#include <godot_cpp/classes/object.hpp>

using namespace godot;

void SimpleHeightmapEditorPlugin::_bind_methods()
{
}

void SimpleHeightmapEditorPlugin::_enter_tree()
{
}

void SimpleHeightmapEditorPlugin::_exit_tree()
{
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
	simple_heightmap = Object::cast_to<SimpleHeightmap>(p_object);
	if (simple_heightmap != nullptr)
	{
		// start editing a new one
	}
	else
	{
		// stop editing everything
	}
}

void SimpleHeightmapEditorPlugin::_forward_3d_draw_over_viewport(Control* p_viewport_control)
{
	// TODO
}

int32_t SimpleHeightmapEditorPlugin::_forward_3d_gui_input(Camera3D* p_viewport_camera, const Ref<InputEvent>& p_event)
{
	// TODO
	return 0;
}

#endif // TOOLS_ENABLED