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

#include "ui/panels/animation_graph/editor_animation_graph_widget_inspector.hpp"
#include "ui/panels/animation_graph/editor_animation_graph_context.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_animation_graph_widget_inspector_t::init(ui::ui_context& ui, ui::widget_id_t parent, const config_t& config)
	{
		SFG_ASSERT(config.context != nullptr);

		_ui		= &ui;
		_config = config;
		_root	= ui.allocate_widget();

		ui.set_widget_debug_name(_root, "animation_graph_inspector");
		ui.get_tree().attach(parent, _root);

		ui::layout_in_t& root_in = ui.get_tree().in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = vec2f_t::one;
		root_in.flow			 = ui::flow_e::column;

		const editor_property_row_t asset_row = editor_misc_widgets_t::make_property_row_with_label(ui, _root, "Asset");
		const editor_theme_t&		theme	  = editor_theme_t::get();

		_asset_name_label = ui.allocate_widget();
		ui.set_widget_debug_name(_asset_name_label, "animation_graph_inspector_asset_name");
		ui.get_tree().attach(asset_row.right, _asset_name_label);

		ui::layout_in_t& asset_name_in = ui.get_tree().in(_asset_name_label);
		asset_name_in.flags			   = ui::wf_visible;
		asset_name_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		asset_name_in.pos_value.y	   = 0.5f;
		asset_name_in.anchor_y		   = ui::anchor_e::center;
		asset_name_in.size_mode_x	   = ui::axis_mode_e::fill;
		asset_name_in.size_mode_y	   = ui::axis_mode_e::fixed;
		asset_name_in.size_value	   = {1.0f, theme.text_default_px_size};

		set_asset_name("");
		editor_dividers_t::add_divider_hor(ui, _root, theme.divider_thickness * 2.0f, theme.color_frame, theme.color_frame, ui::vg_gradient_e::none);

		_reflection.init(ui,
						 _root,
						 {
							 .fold_states		 = &_fold_states,
							 .elevate_draw_order = true,
						 });
		_asm_node_reflection.init(ui,
								  _root,
								  {
									  .fold_states		  = &_asm_node_fold_states,
									  .elevate_draw_order = true,
								  });
		ui.get_tree().set_visible(_asm_node_reflection.get_root(), false, false);
	}

	void editor_animation_graph_widget_inspector_t::uninit()
	{
		_ui->cancel_mutations(this);
		_asm_node_reflection.uninit();
		_reflection.uninit();
		_ui->deallocate_widget(_root);

		_fold_states.resize(0);
		_asm_node_fold_states.resize(0);
		_ui				  = nullptr;
		_config			  = {};
		_asset_name_label = NULL_WIDGET;
		_root			  = NULL_WIDGET;
	}

	void editor_animation_graph_widget_inspector_t::refresh_inspector()
	{
		_ui->request_unique_mutation(on_refresh_mutation, this);
	}

	void editor_animation_graph_widget_inspector_t::set_asset_name(const char* asset_name)
	{
		const editor_theme_t& theme = editor_theme_t::get();

		_ui->set_widget_text(_asset_name_label, asset_name != nullptr ? asset_name : "");
		_ui->get_paint().set_text(_asset_name_label,
								  _ui->widget_text(_asset_name_label),
								  _ui->widget_text_len(_asset_name_label),
								  {
									  .font		   = theme.font_default,
									  .color	   = theme.color_text0,
									  .point_size  = theme.text_default_px_size,
									  .spacing	   = 0,
									  .raster_mode = editor_text_rasterization_t::get_rasterization_type(),
								  });
	}

	void editor_animation_graph_widget_inspector_t::refresh_inspector_immediate()
	{
		animation_graph_def_t& graph			= _config.context->get_graph();
		const u32			   selected_node_id = _config.context->get_selected_node_id();
		const auto			   selected_node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [selected_node_id](const animation_graph_node_def_t& node) { return node.id == selected_node_id; });
		const bool			   display_asm_node = _config.context->get_display_mode() == editor_animation_graph_display_mode_e::display_nodes && selected_node_it != graph.nodes.end();

		_ui->get_tree().set_visible(_reflection.get_root(), !display_asm_node, false);
		_ui->get_tree().set_visible(_asm_node_reflection.get_root(), display_asm_node, false);

		if (display_asm_node)
		{
			void* asm_node = &selected_node_it->asm_node;
			_asm_node_reflection.save_fold_states();
			_asm_node_reflection.set_reflection({
				.fold_states = &_asm_node_fold_states,
				.objects	 = {.data = &asm_node, .size = 1},
				.type_id	 = type_id_t<animation_graph_node_asm_def_t>::value,
			});
			return;
		}

		void* graph_object = &graph;
		_reflection.save_fold_states();
		_reflection.set_reflection({
			.fold_states = &_fold_states,
			.objects	 = {.data = &graph_object, .size = 1},
			.type_id	 = type_id_t<animation_graph_def_t>::value,
		});
	}

	void editor_animation_graph_widget_inspector_t::on_refresh_mutation(ui::ui_context& ui, void* user_data)
	{
		static_cast<editor_animation_graph_widget_inspector_t*>(user_data)->refresh_inspector_immediate();
	}
}
