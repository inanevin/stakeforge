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
#include "ui/panels/editor_panel_world.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/runtime/render/world_render_context.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		ui::ui_resource_ref_t make_null_world_texture_ref()
		{
			ui::ui_resource_ref_t ref = {
				.handle = NULL_RESOURCE_HANDLE,
				.type	= ui::ui_resource_type_e::gpu_index_fof,
			};
			for (u8 i = 0; i < BACK_BUFFER_COUNT; ++i)
				ref.gpu_indices[i] = NULL_GPU_INDEX;
			return ref;
		}
	}

	editor_panel_world_t::editor_panel_world_t()
	{
		set_type(editor_panel_type_e::world);
		set_title(editor_panel_type_to_string(editor_panel_type_e::world));
	}

	void editor_panel_world_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_world_view = ui.allocate_widget();
		ui.set_widget_debug_name(_world_view, "world_view");
		tree.attach(_root, _world_view);

		ui::layout_in_t& in = tree.in(_world_view);
		in.size_mode_x		= ui::axis_mode_e::parent_relative;
		in.size_mode_y		= ui::axis_mode_e::parent_relative;
		in.size_value		= {1.0f, 1.0f};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = {1.0f, 1.0f, 1.0f, 1.0f};
		rect.fill_color_b		 = rect.fill_color_a;
		rect.filled				 = true;

		ui::ui_render_state_t state = {};
		state.pipeline				= "editor/shaders/editor_ui_texture.hlsl"_hs;
		state.constants[0]			= make_null_world_texture_ref();

		paint.set_rect(_world_view, rect, state);

		_empty_label = ui.allocate_widget();
		ui.set_widget_debug_name(_empty_label, "empty_label");
		tree.attach(_root, _empty_label);
		tree.draw_order(_empty_label) = tree.draw_order_const(_world_view);

		ui::layout_in_t& label_in = tree.in(_empty_label);
		label_in.flags			  = ui::wf_visible;
		label_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value		  = {0.5f, 0.5f};
		label_in.anchor_x		  = ui::anchor_e::center;
		label_in.anchor_y		  = ui::anchor_e::center;

		ui.set_widget_text(_empty_label, "NO WORLD AVAILABLE");
		paint.set_text(_empty_label,
					   ui.widget_text(_empty_label),
					   ui.widget_text_len(_empty_label),
					   {.font = theme.font_title_bold, .color = theme.color_text2, .point_size = theme.text_big_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
	}

	void editor_panel_world_t::uninit()
	{
		_ui->deallocate_widget(_empty_label);
		_ui->deallocate_widget(_world_view);
		_empty_label = NULL_WIDGET;
		_world_view	 = NULL_WIDGET;
		editor_panel_t::uninit();
	}

	void editor_panel_world_t::set_world(const world_render_context_t& world)
	{
		SFG_ASSERT(_ui != nullptr);
		SFG_ASSERT(_world_view != NULL_WIDGET);

		ui::ui_resource_ref_t texture_ref = {
			.handle = NULL_RESOURCE_HANDLE,
			.type	= ui::ui_resource_type_e::gpu_index_fof,
		};
		for (u8 i = 0; i < BACK_BUFFER_COUNT; ++i)
			texture_ref.gpu_indices[i] = world.get_world_texture_index(i);

		ui::paint_def_t& def		  = _ui->get_paint().def(_world_view);
		def.render_state.pipeline	  = "editor/shaders/editor_ui_texture.hlsl"_hs;
		def.render_state.constants[0] = texture_ref;

		ui::layout_tree_t& tree = _ui->get_tree();
		tree.set_visible(_world_view, true);
		tree.set_visible(_empty_label, false);
	}

	void editor_panel_world_t::clear_world()
	{
		SFG_ASSERT(_ui != nullptr);
		SFG_ASSERT(_world_view != NULL_WIDGET);

		ui::paint_def_t& def		  = _ui->get_paint().def(_world_view);
		def.render_state.pipeline	  = "editor/shaders/editor_ui_texture.hlsl"_hs;
		def.render_state.constants[0] = make_null_world_texture_ref();

		ui::layout_tree_t& tree = _ui->get_tree();
		tree.set_visible(_world_view, false);
		tree.set_visible(_empty_label, true);
	}
}
