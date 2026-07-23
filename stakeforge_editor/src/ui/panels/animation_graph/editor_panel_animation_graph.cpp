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

#include "ui/panels/animation_graph/editor_panel_animation_graph.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"

#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define ANIMATION_GRAPH_EDITOR_PANE_SPLIT_MIN			   0.4f
#define ANIMATION_GRAPH_EDITOR_PANE_SPLIT_MAX			   0.85f
#define ANIMATION_GRAPH_EDITOR_SPLIT_BORDER_THICKNESS_MULT 2.0f

	editor_panel_animation_graph_t::editor_panel_animation_graph_t()
	{
		set_type(editor_panel_type_e::animation_graph);
		refresh_title();
		set_icon(ICON_ANIMATION);
	}

	void editor_panel_animation_graph_t::serialize(nlohmann::json& json) const
	{
		json			   = nlohmann::json::object();
		json["graph_id"]   = _graph_id;
		json["asset_name"] = _asset_name;
		json["pane_split"] = _pane_split;
	}

	void editor_panel_animation_graph_t::deserialize(const nlohmann::json& json)
	{
		_graph_id	= json.value<sid_t>("graph_id", NULL_SID);
		_asset_name = json.value<string_t>("asset_name", {});
		_pane_split = math::clamp(json.value<f32>("pane_split", _pane_split), ANIMATION_GRAPH_EDITOR_PANE_SPLIT_MIN, ANIMATION_GRAPH_EDITOR_PANE_SPLIT_MAX);

		set_sub_item_id(_graph_id);
		refresh_title(_asset_name.c_str(), "AG: ");
	}

	void editor_panel_animation_graph_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::row;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = vec4f_t::zero;

		_left_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_left_pane, "animation_graph_left_pane");
		tree.attach(_root, _left_pane);

		ui::layout_in_t& left_in = tree.in(_left_pane);
		left_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		left_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		left_in.size_value		 = {_pane_split, 1.0f};

		_context.init(&_grid, &_inspector);

		_grid.init(ui,
				   _left_pane,
				   {
					   .context		   = &_context,
					   .grid_size	   = 64,
					   .line_thickness = theme.divider_thickness * 2,
				   });

		if (_graph_id != NULL_SID)
			load_graph();

		const editor_split_border_t::config_t split_config{
			.on_drag   = on_split_border_drag,
			.user_data = this,
			.direction = editor_split_border_direction_e::horizontal,
		};
		_split_border.init(ui, _root, split_config);

		ui::layout_in_t& border_in = tree.in(_split_border.get_root());
		border_in.size_mode_x	   = ui::axis_mode_e::fixed;
		border_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		border_in.size_value	   = {theme.border_thickness * ANIMATION_GRAPH_EDITOR_SPLIT_BORDER_THICKNESS_MULT, 1.0f};

		_right_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_right_pane, "animation_graph_right_pane");
		tree.attach(_root, _right_pane);

		ui::layout_in_t& right_in = tree.in(_right_pane);
		right_in.flags			  = ui::wf_visible | ui::wf_input | ui::wf_scroll_y;
		right_in.child_clip_mode  = ui::clip_mode_e::scissor_rect;
		right_in.size_mode_x	  = ui::axis_mode_e::fill;
		right_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		right_in.size_value		  = vec2f_t::one;

		ui.get_paint().set_rect(_right_pane,
								{
									.fill_color_a = theme.color_panel,
									.fill_color_b = theme.color_panel,
								});

		_inspector.init(ui, _right_pane, {.context = &_context});
		_inspector.set_asset_name(_asset_name.c_str());
		_right_scrollbar.init(ui, {.target = _right_pane, .axes = editor_scrollbar_axis_y});
		_inspector.refresh_inspector();

		apply_pane_split();
	}

	void editor_panel_animation_graph_t::uninit()
	{
		_inspector.uninit();
		_right_scrollbar.uninit();
		_split_border.uninit();
		_grid.uninit();
		_context.uninit();
		_ui->deallocate_widget(_left_pane);
		_ui->deallocate_widget(_right_pane);
		editor_panel_t::uninit();

		_left_pane	= NULL_WIDGET;
		_right_pane = NULL_WIDGET;
	}

	void editor_panel_animation_graph_t::set_graph(sid_t graph_id, const char* asset_name)
	{
		_graph_id	= graph_id;
		_asset_name = asset_name;
		_inspector.set_asset_name(_asset_name.c_str());

		_context.set_display_mode(editor_animation_graph_display_mode_e::display_nodes);
		_context.set_display_node_id(ANIMATION_GRAPH_DEF_NULL_ID);
		_context.set_selected_node_id(ANIMATION_GRAPH_DEF_NULL_ID);
		_context.set_selected_sub_node_id(ANIMATION_GRAPH_DEF_NULL_ID);
		_grid.set_mode(editor_animation_graph_display_mode_e::display_nodes);

		set_sub_item_id(graph_id);
		refresh_title(_asset_name.c_str(), "AG: ");
		load_graph();
		_inspector.refresh_inspector();
	}

	void editor_panel_animation_graph_t::apply_pane_split()
	{
		_ui->get_tree().in(_left_pane).size_value.x = _pane_split;
	}

	void editor_panel_animation_graph_t::load_graph()
	{
		const editor_asset_t*  asset = editor_asset_manager_t::get().find_asset(_graph_id);
		animation_graph_def_t& graph = _context.get_graph();
		graph						 = {};

		if (asset == nullptr || asset->asset_type != editor_asset_type_e::animation_graph || asset->embedded_source.empty())
		{
			SFG_ERR("failed to find animation graph asset {0}", _graph_id);
			_grid.refresh_nodes();
			return;
		}

		const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(*asset);

		if (!reflection_registry_t::get().type_from_json(type_id_t<animation_graph_def_t>::value, &graph, nullptr, embedded_source))
		{
			SFG_ERR("failed to deserialize animation graph definition for asset {0}", _graph_id);
			graph = {};
		}

		_grid.refresh_nodes();
	}

	void editor_panel_animation_graph_t::on_split_border_drag(editor_split_border_t&, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		editor_panel_animation_graph_t& panel = *static_cast<editor_panel_animation_graph_t*>(user_data);
		const ui::layout_out_t&			out	  = panel._ui->get_tree().out(panel._root);

		panel._pane_split = math::clamp((pos.x - out.pos.x) / out.size.x, ANIMATION_GRAPH_EDITOR_PANE_SPLIT_MIN, ANIMATION_GRAPH_EDITOR_PANE_SPLIT_MAX);
		panel.apply_pane_split();
	}
}
