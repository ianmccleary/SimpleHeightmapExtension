#include "register_types.h"

#include "simple_heightmap.h"

#ifdef TOOLS_ENABLED
#include "simple_heightmap_editor_plugin.h"
#include "simple_heightmap_gizmo_plugin.h"
#endif // TOOLS_ENABLED

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_simple_heightmap_module(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
	{
		GDREGISTER_CLASS(SimpleHeightmap);
	}

#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR)
	{
		GDREGISTER_INTERNAL_CLASS(SimpleHeightmapGizmoPlugin);
		GDREGISTER_INTERNAL_CLASS(SimpleHeightmapEditorPlugin);
		EditorPlugins::add_by_type<SimpleHeightmapEditorPlugin>();
	}
#endif // TOOLS_ENABLED
}

void uninitialize_simple_heightmap_module(ModuleInitializationLevel p_level)
{ }

extern "C"
{
	// Initialization.
	GDExtensionBool GDE_EXPORT simple_heightmap_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{
		godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

		init_obj.register_initializer(initialize_simple_heightmap_module);
		init_obj.register_terminator(uninitialize_simple_heightmap_module);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}