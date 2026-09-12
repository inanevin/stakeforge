/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

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
		const vec2f_t		  size	= calculate_size(ui, config.hdr);

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
							  .hdr			   = config.hdr,
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

	vec2f_t editor_popup_color_wheel_t::calculate_size(ui::ui_context& ui, bool hdr)
	{
		const ui::layout_tree_t& tree	  = ui.get_tree();
		const ui::layout_out_t&	 screen	  = tree.out(tree.get_root());
		const editor_theme_t&	 theme	  = editor_theme_t::get();
		const f32				 scale	  = ui.get_ui_scale() > 0.0f ? ui.get_ui_scale() : 1.0f;
		const f32				 screen_w = screen.clip.z / scale;
		const f32				 max_w	  = math::max(theme.item_width, screen_w - theme.margin_horizontal * 2.0f);
		return {math::min(math::max(screen_w * 0.2f, COLOR_WHEEL_POPUP_MIN_WIDTH), max_w), editor_widget_color_wheel_t::calculate_min_height(hdr)};
	}

	vec2f_t editor_popup_color_wheel_t::calculate_position(ui::ui_context& ui, const vec2f_t& requested_position, bool hdr)
	{
		const ui::layout_tree_t& tree	  = ui.get_tree();
		const ui::layout_out_t&	 screen	  = tree.out(tree.get_root());
		const f32				 scale	  = ui.get_ui_scale() > 0.0f ? ui.get_ui_scale() : 1.0f;
		const vec2f_t			 size	  = calculate_size(ui, hdr) * scale;
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
