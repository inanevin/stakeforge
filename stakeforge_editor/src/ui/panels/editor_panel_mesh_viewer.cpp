/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ui/panels/editor_panel_mesh_viewer.hpp"
#include "assets/editor_asset.hpp"
#include "editor_surface_controller.hpp"
#include "editor_world_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include "world/editor_world.hpp"
#include <sfg/math/math.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/resources/mesh.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define MESH_VIEWER_PANE_SPLIT_MIN				0.45f
#define MESH_VIEWER_PANE_SPLIT_MAX				0.85f
#define MESH_VIEWER_SPLIT_BORDER_THICKNESS_MULT 2.0f

	editor_panel_mesh_viewer_t::editor_panel_mesh_viewer_t()
	{
		set_type(editor_panel_type_e::mesh_viewer);
		refresh_title();
		set_icon(ICON_MESH);
	}

	void editor_panel_mesh_viewer_t::serialize(nlohmann::json& j) const
	{
		j				= nlohmann::json::object();
		j["mesh_guid"]	= _mesh_guid;
		j["asset_name"] = _asset_name;
		j["pane_split"] = _pane_split;
	}

	void editor_panel_mesh_viewer_t::deserialize(const nlohmann::json& j)
	{
		_mesh_guid = j.value<sid_t>("mesh_guid", 0);
		set_sub_item_id(_mesh_guid);
		_asset_name = j.value<string_t>("asset_name", {});
		_pane_split = math::clamp(j.value<f32>("pane_split", _pane_split), MESH_VIEWER_PANE_SPLIT_MIN, MESH_VIEWER_PANE_SPLIT_MAX);
		refresh_title(_asset_name.c_str(), "M: ");
	}

	void editor_panel_mesh_viewer_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::row;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, 0.0f, theme.margin_vertical, 0.0f};

		_left_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_left_pane, "mesh_viewer_left_pane");
		tree.attach(_root, _left_pane);

		ui::layout_in_t& left_in = tree.in(_left_pane);
		left_in.flow			 = ui::flow_e::none;
		left_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};
		left_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		left_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		left_in.size_value		 = {_pane_split, 1.0f};

		ui::vg_rect_paint_t left_rect = {};
		left_rect.fill_color_a		  = theme.color_frame;
		left_rect.fill_color_b		  = theme.color_frame;
		paint.set_rect(_left_pane, left_rect);

		_world_view.init(ui, _left_pane);

		editor_split_border_t::config_t split_config = {};
		split_config.direction						 = editor_split_border_direction_e::horizontal;
		split_config.on_drag						 = on_split_border_drag;
		split_config.user_data						 = this;
		_split_border.init(ui, _root, split_config);

		ui::layout_in_t& border_in = tree.in(_split_border.get_root());
		border_in.size_mode_x	   = ui::axis_mode_e::fixed;
		border_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		border_in.size_value	   = {theme.border_thickness * MESH_VIEWER_SPLIT_BORDER_THICKNESS_MULT, 1.0f};

		_right_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_right_pane, "mesh_viewer_right_pane");
		tree.attach(_root, _right_pane);

		ui::layout_in_t& right_in = tree.in(_right_pane);
		right_in.flow			  = ui::flow_e::column;
		right_in.child_margins	  = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};
		right_in.size_mode_x	  = ui::axis_mode_e::fill;
		right_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		right_in.size_value		  = {1.0f, 1.0f};

		_vertex_count_value = append_property_value_row("Vertices");
		editor_dividers_t::add_divider_hor(ui, _right_pane, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
		_index_count_value = append_property_value_row("Indices");
		editor_dividers_t::add_divider_hor(ui, _right_pane, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
		_primitive_count_value = append_property_value_row("Primitives");
		editor_dividers_t::add_divider_hor(ui, _right_pane, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
		_vertex_stride_value = append_property_value_row("Vertex Stride");
		editor_dividers_t::add_divider_hor(ui, _right_pane, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
		_is_skinned_value = append_property_value_row("Skinned");

		create_preview_world();
		if (_mesh_guid != 0)
			set_mesh(_mesh_guid, _asset_name.c_str());
		else
			refresh_info();
		apply_pane_split();
	}

	void editor_panel_mesh_viewer_t::uninit()
	{
		_world_view.uninit();
		_split_border.uninit();
		_ui->deallocate_widget(_left_pane);
		_ui->deallocate_widget(_right_pane);
		destroy_preview_world();
		editor_panel_t::uninit();
	}

	void editor_panel_mesh_viewer_t::set_mesh(sid_t mesh_guid, const char* asset_name)
	{
		_mesh_guid = mesh_guid;
		set_sub_item_id(mesh_guid);
		_asset_name = asset_name;

		if (!_world.is_null())
			create_display_entity();

		refresh_info();
		refresh_title(_asset_name.c_str(), "M: ");
	}

	void editor_panel_mesh_viewer_t::create_preview_world()
	{
		const world_init_config_t init_config{
			.render_resolution				= editor_surface_controller_t::get().get_main_surface().swapchain_size,
			.render_entity_max				= 10,
			.render_bone_max				= 128,
			.render_bone_reserve			= 128,
			.animation_graph_memory_reserve = 64 * 1024,
			.component_table_reserve		= 64,
			.free_list_reserve				= 128,
			.used_resource_reserve			= 64,
			.text_allocation_reserve		= 256,
			.physics_enabled				= false,
		};

		editor_world_controller_t& controller = editor_world_controller_t::get();
		_world								  = controller.create_world(init_config, editor_world_edit_type_e::view_with_debug);
		editor_world_t* const editor_world	  = controller.get_editor_world(_world);

		editor_world->install_camera(editor_world_camera_type_e::orbit);
		create_environment();
		_world_view.set_edit_world(_world);
	}

	void editor_panel_mesh_viewer_t::destroy_preview_world()
	{
		if (_world.is_null())
			return;

		editor_world_controller_t::get().destroy_world(_world);
		_world				= {};
		_display_entity		= NULL_ENTITY_ID;
		_environment_entity = NULL_ENTITY_ID;
	}

	void editor_panel_mesh_viewer_t::create_environment()
	{
		world_t& world			   = editor_world_controller_t::get().get_editor_world(_world)->get_world();
		_environment_entity		   = world.create_entity("mesh_viewer_environment");
		component_skybox_t& skybox = ecs_helpers_t::table_add_or_get_as<component_skybox_t>(world.get_component_table(type_id_t<component_skybox_t>::value), _environment_entity);
		skybox.skybox_asset		   = DEFAULT_QWANTANI_DUSK_SKYBOX_ASSET_GUID;
		skybox.exposure			   = 0.25f;
		world.scan_for_resources(_environment_entity, true);
	}

	void editor_panel_mesh_viewer_t::clear_display_entity()
	{
		if (_display_entity == NULL_ENTITY_ID)
			return;

		world_t& world = editor_world_controller_t::get().get_editor_world(_world)->get_world();
		world.destroy_entity(_display_entity);
		_display_entity = NULL_ENTITY_ID;
	}

	void editor_panel_mesh_viewer_t::create_display_entity()
	{
		clear_display_entity();
		if (_mesh_guid == 0)
			return;

		world_t& world							 = editor_world_controller_t::get().get_editor_world(_world)->get_world();
		_display_entity							 = world.create_entity("mesh_viewer_mesh");
		component_mesh_renderer_t& mesh_renderer = ecs_helpers_t::table_add_or_get_as<component_mesh_renderer_t>(world.get_component_table(type_id_t<component_mesh_renderer_t>::value), _display_entity);
		mesh_renderer.mesh						 = _mesh_guid;
		for (size_t i = 0; i < decltype(mesh_renderer.materials)::capacity; ++i)
			mesh_renderer.materials.push_back(DEFAULT_GBUFFER_MATERIAL_ASSET_GUID);
		world.scan_for_resources(_display_entity, true);

		if (const mesh_internals_t* internals = resource_manager_t::get().find_internals<mesh_internals_t>(_mesh_guid))
			editor_world_controller_t::get().get_editor_world(_world)->fit_camera_to_bounds(internals->local_bounds);
	}

	void editor_panel_mesh_viewer_t::refresh_info()
	{
		if (_ui == nullptr)
			return;

		_vertex_count_text	  = "-";
		_index_count_text	  = "-";
		_primitive_count_text = "-";
		_vertex_stride_text	  = "-";
		_is_skinned_text	  = "-";

		if (_mesh_guid != 0)
		{
			if (const mesh_internals_t* internals = resource_manager_t::get().find_internals<mesh_internals_t>(_mesh_guid))
			{
				_vertex_count_text	  = std::to_string(internals->vertex_count);
				_index_count_text	  = std::to_string(internals->index_count);
				_primitive_count_text = std::to_string(internals->primitive_count);
				_vertex_stride_text	  = std::to_string(internals->vertex_stride);
				_is_skinned_text	  = internals->is_skinned ? "true" : "false";
			}
			else
			{
				_vertex_count_text = "Failed";
			}
		}

		_ui->set_widget_text(_vertex_count_value, _vertex_count_text.c_str());
		_ui->set_widget_text(_index_count_value, _index_count_text.c_str());
		_ui->set_widget_text(_primitive_count_value, _primitive_count_text.c_str());
		_ui->set_widget_text(_vertex_stride_value, _vertex_stride_text.c_str());
		_ui->set_widget_text(_is_skinned_value, _is_skinned_text.c_str());

		ui::paint_layer_t&		  paint		  = _ui->get_paint();
		const editor_theme_t&	  theme		  = editor_theme_t::get();
		const ui::vg_text_style_t value_paint = {
			.font = theme.font_default, .color = _vertex_count_text == "Failed" ? theme.color_accent_warn : theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()};
		paint.set_text(_vertex_count_value, _ui->widget_text(_vertex_count_value), _ui->widget_text_len(_vertex_count_value), value_paint);
		paint.set_text(_index_count_value, _ui->widget_text(_index_count_value), _ui->widget_text_len(_index_count_value), value_paint);
		paint.set_text(_primitive_count_value, _ui->widget_text(_primitive_count_value), _ui->widget_text_len(_primitive_count_value), value_paint);
		paint.set_text(_vertex_stride_value, _ui->widget_text(_vertex_stride_value), _ui->widget_text_len(_vertex_stride_value), value_paint);
		paint.set_text(_is_skinned_value, _ui->widget_text(_is_skinned_value), _ui->widget_text_len(_is_skinned_value), value_paint);
	}

	void editor_panel_mesh_viewer_t::apply_pane_split()
	{
		if (_ui != nullptr)
			_ui->get_tree().in(_left_pane).size_value.x = _pane_split;
	}

	ui::widget_id_t editor_panel_mesh_viewer_t::append_property_value_row(const char* label)
	{
		const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _right_pane, label);
		return append_value_label(row.right);
	}

	ui::widget_id_t editor_panel_mesh_viewer_t::append_value_label(ui::widget_id_t parent)
	{
		const editor_theme_t& theme = editor_theme_t::get();

		ui::widget_id_t label = _ui->allocate_widget();
		_ui->set_widget_debug_name(label, "mesh_viewer_property_value");
		_ui->get_tree().attach(parent, label);

		ui::layout_in_t& label_in = _ui->get_tree().in(label);
		label_in.flags			  = ui::wf_visible;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;
		label_in.size_mode_x	  = ui::axis_mode_e::fill;
		label_in.size_mode_y	  = ui::axis_mode_e::fixed;
		label_in.size_value		  = {1.0f, theme.text_default_px_size};

		_ui->set_widget_text(label, "");
		_ui->get_paint().set_text(
			label, _ui->widget_text(label), _ui->widget_text_len(label), {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		return label;
	}

	void editor_panel_mesh_viewer_t::on_split_border_drag(editor_split_border_t&, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		editor_panel_mesh_viewer_t& panel = *static_cast<editor_panel_mesh_viewer_t*>(user_data);
		const ui::layout_out_t&		out	  = panel._ui->get_tree().out(panel._root);

		panel._pane_split = math::clamp((pos.x - out.pos.x) / out.size.x, MESH_VIEWER_PANE_SPLIT_MIN, MESH_VIEWER_PANE_SPLIT_MAX);
		panel.apply_pane_split();
	}
}
