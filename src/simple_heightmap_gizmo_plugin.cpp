#ifdef TOOLS_ENABLED
#include "simple_heightmap_gizmo_plugin.h"
#include "simple_heightmap.h"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/physics_server3d_extension.hpp>
#include <godot_cpp/classes/project_settings.hpp>

void SimpleHeightmapGizmoPlugin::_bind_methods()
{ }

SimpleHeightmapGizmoPlugin::SimpleHeightmapGizmoPlugin()
{
	const auto project_settings = godot::ProjectSettings::get_singleton();
	create_material("collider_lines", project_settings != nullptr ? static_cast<godot::Color>(project_settings->get_setting_with_override("debug/shapes/collision/shape_color")) : godot::Color());
}

godot::String SimpleHeightmapGizmoPlugin::_get_gizmo_name() const
{
	return "SimpleHeightmap";
}

bool SimpleHeightmapGizmoPlugin::_has_gizmo(godot::Node3D *p_for_node_3d) const
{
	return godot::Object::cast_to<SimpleHeightmap>(p_for_node_3d) != nullptr;
}

void SimpleHeightmapGizmoPlugin::_redraw(const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo)
{
	const auto heightmap = godot::Object::cast_to<SimpleHeightmap>(p_gizmo->get_node_3d());
	if (heightmap != nullptr)
	{		
		godot::PackedVector3Array lines;

		const auto data = heightmap->get_collider_shape_data();
		const auto data_size = heightmap->get_collider_shape_data_size();
		const auto scale = heightmap->get_mesh_size() / heightmap->get_collider_size();
		for (int x = 0; x < data_size - 1; ++x)
		{
			for (int z = 0; z < data_size - 1; ++z)
			{
				const auto a = (x + 0) + ((z + 0) * data_size);
				const auto b = (x + 1) + ((z + 0) * data_size);
				const auto c = (x + 0) + ((z + 1) * data_size);
				const auto pa = godot::Vector3(static_cast<godot::real_t>(x + 0) * scale, data[a], static_cast<godot::real_t>(z + 0) * scale);
				const auto pb = godot::Vector3(static_cast<godot::real_t>(x + 1) * scale, data[b], static_cast<godot::real_t>(z + 0) * scale);
				const auto pc = godot::Vector3(static_cast<godot::real_t>(x + 0) * scale, data[c], static_cast<godot::real_t>(z + 1) * scale);

				lines.push_back(pa);
				lines.push_back(pb);

				lines.push_back(pa);
				lines.push_back(pc);
			}
		}

		p_gizmo->clear();
		p_gizmo->add_lines(lines, get_material("collider_lines", p_gizmo), false);
	}
}

#endif // TOOLS_ENABLED