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

#include "ui/widgets/popups/editor_popup_color_wheel.hpp"
#include "ui/panels/editor_theme.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
#define COLOR_WHEEL_POPUP_MIN_WIDTH 280.0f

	void editor_popup_color_wheel_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_popup_color_wheel_config_t& config)
	{
		SFG_ASSERT(config.fields.size > 0);

		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();
		const vec2f_t		  size	= calculate_size(ui);

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "color_wheel_popup");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::fixed;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;
		root_in.size_value		 = {size.x - theme.margin_horizontal * 2.0f, size.y - theme.margin_vertical * 2.0f};

		_color_wheel.init(ui,
						  _root,
						  {
							  .field		   = {.fields = config.fields},
							  .edit_begin	   = config.edit_begin,
							  .on_data_changed = config.on_data_changed,
							  .user_data	   = config.user_data,
						  });
		tree.set_visible(_color_wheel.get_root(), true, false);
	}

	void editor_popup_color_wheel_t::uninit()
	{
		_color_wheel.uninit();
		_ui->deallocate_widget(_root);

		_ui	  = nullptr;
		_root = NULL_WIDGET;
	}

	vec2f_t editor_popup_color_wheel_t::calculate_size(ui::ui_context& ui)
	{
		const ui::layout_tree_t& tree	  = ui.get_tree();
		const ui::layout_out_t&	 screen	  = tree.out(tree.get_root());
		const editor_theme_t&	 theme	  = editor_theme_t::get();
		const f32				 scale	  = ui.get_ui_scale() > 0.0f ? ui.get_ui_scale() : 1.0f;
		const f32				 screen_w = screen.clip.z / scale;
		const f32				 max_w	  = math::max(theme.item_width, screen_w - theme.margin_horizontal * 2.0f);
		return {math::min(math::max(screen_w * 0.2f, COLOR_WHEEL_POPUP_MIN_WIDTH), max_w), editor_widget_color_wheel_t::calculate_min_height()};
	}

	vec2f_t editor_popup_color_wheel_t::calculate_position(ui::ui_context& ui, const vec2f_t& requested_position)
	{
		const ui::layout_tree_t& tree	  = ui.get_tree();
		const ui::layout_out_t&	 screen	  = tree.out(tree.get_root());
		const f32				 scale	  = ui.get_ui_scale() > 0.0f ? ui.get_ui_scale() : 1.0f;
		const vec2f_t			 size	  = calculate_size(ui) * scale;
		vec2f_t					 position = requested_position;
		if (position.x + size.x > screen.clip.x + screen.clip.z)
			position.x = screen.clip.x + screen.clip.z - size.x;
		if (position.y + size.y > screen.clip.y + screen.clip.w)
			position.y = requested_position.y - size.y;
		position.x = math::clamp(position.x, screen.clip.x, math::max(screen.clip.x, screen.clip.x + screen.clip.z - size.x));
		position.y = math::clamp(position.y, screen.clip.y, math::max(screen.clip.y, screen.clip.y + screen.clip.w - size.y));
		return position;
	}
}
