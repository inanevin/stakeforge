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
#include "ui/widgets/editor_widget_thumbnail.hpp"
#include "ui/panels/editor_theme.hpp"

#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
#define EDITOR_THUMBNAIL_CHECK_TICKS 30u

	void editor_widget_thumbnail_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_thumbnail_config_t& config)
	{
		_ui		   = &ui;
		_thumbnail = config.thumbnail;

		ui::layout_tree_t& tree = ui.get_tree();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "thumbnail");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible;
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {1.0f, 1.0f};

		refresh_frame();
	}

	void editor_widget_thumbnail_t::uninit()
	{
		_ui->clear_pre_layout_tick(_root);
		_ui->deallocate_widget(_root);
		_ui			  = nullptr;
		_root		  = NULL_WIDGET;
		_thumbnail	  = NULL_SID;
		_tick_counter = 0;
		_ticking	  = false;
	}

	void editor_widget_thumbnail_t::set_thumbnail(sid_t thumbnail)
	{
		_ui->clear_pre_layout_tick(_root);
		_thumbnail	  = thumbnail;
		_tick_counter = 0;
		_ticking	  = false;
		refresh_frame();
	}

	void editor_widget_thumbnail_t::set_visible(bool visible)
	{
		_ui->get_tree().set_visible(_root, visible, false);
	}

	void editor_widget_thumbnail_t::refresh_frame()
	{
		set_default_frame();

		if (_thumbnail == NULL_SID)
			return;

		const resource_entry_t* entry = resource_manager_t::get().find_entry(_thumbnail);
		if (entry != nullptr && entry->state == resource_state_e::ready)
		{
			set_texture_frame();
			return;
		}

		if (!_ticking)
		{
			_ui->set_pre_layout_tick(_root, on_tick, this);
			_ticking = true;
		}
	}

	void editor_widget_thumbnail_t::set_default_frame()
	{
		const editor_theme_t& theme = editor_theme_t::get();

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_frame;
		rect.fill_color_b		 = theme.color_frame;
		rect.rounding			 = theme.item_rounding;
		rect.rounding_segs		 = 4;
		_ui->get_paint().set_rect(_root, rect);
	}

	void editor_widget_thumbnail_t::set_texture_frame()
	{
		const editor_theme_t& theme = editor_theme_t::get();

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_frame;
		rect.fill_color_b		 = theme.color_frame;
		rect.rounding			 = theme.item_rounding;
		rect.rounding_segs		 = 4;

		ui::ui_render_state_t state = {};
		state.pipeline				= "editor/resource_pack/shaders/editor_ui_texture.hlsl"_hs;
		state.constants[0].handle	= _thumbnail;
		state.constants[0].type		= ui::ui_resource_type_e::texture;
		_ui->get_paint().set_rect(_root, rect, state);
		_ui->clear_pre_layout_tick(_root);
		_tick_counter = 0;
		_ticking	  = false;
	}

	void editor_widget_thumbnail_t::on_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		editor_widget_thumbnail_t& thumbnail = *static_cast<editor_widget_thumbnail_t*>(user_data);
		thumbnail._tick_counter++;
		if (thumbnail._tick_counter < EDITOR_THUMBNAIL_CHECK_TICKS)
			return;

		thumbnail._tick_counter		  = 0;
		const resource_entry_t* entry = resource_manager_t::get().find_entry(thumbnail._thumbnail);
		if (entry != nullptr && entry->state == resource_state_e::ready)
			thumbnail.set_texture_frame();
	}

#undef EDITOR_THUMBNAIL_CHECK_TICKS
}
