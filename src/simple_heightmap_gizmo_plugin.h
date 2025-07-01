#pragma once

#ifdef TOOLS_ENABLED

#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>

class SimpleHeightmapGizmoPlugin : public godot::EditorNode3DGizmoPlugin
{
	GDCLASS(SimpleHeightmapGizmoPlugin, godot::EditorNode3DGizmoPlugin);
public:
	static void _bind_methods();

	SimpleHeightmapGizmoPlugin();
	godot::String _get_gizmo_name() const override;
	bool _has_gizmo(godot::Node3D *p_for_node_3d) const override;
	void _redraw(const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo) override;
};

#endif // TOOLS_ENABLED