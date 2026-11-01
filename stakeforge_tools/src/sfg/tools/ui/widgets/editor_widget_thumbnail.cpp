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
#include "tools/ui/widgets/editor_widget_thumbnail.hpp"
#include "tools/ui/panels/editor_theme.hpp"
#include "tools/assets/thumbnail/editor_asset_thumbnail_manager.hpp"
#include "tools/editor_app.hpp"

#include <sfg/core/runtime/resources/resource_manager.hpp>
#include <sfg/core/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_widget_thumbnail_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_thumbnail_config_t& config)
	{
		_ui		   = &ui;
		_thumbnail = config.thumbnail;

		ui::layout_tree_t& tree = ui.get_tree();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "thumbnail");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {1.0f, 1.0f};

		ui.set_post_layout_tick(_root, on_tick, this);
		refresh_frame();
	}

	void editor_widget_thumbnail_t::uninit()
	{
		_ui->clear_post_layout_tick(_root);
		_ui->deallocate_widget(_root);
		_ui				 = nullptr;
		_root			 = NULL_WIDGET;
		_thumbnail		 = NULL_SID;
		_last_check_tick = 0;
	}

	void editor_widget_thumbnail_t::set_thumbnail(sid_t thumbnail)
	{
		_thumbnail = thumbnail;
		refresh_frame();
	}

	void editor_widget_thumbnail_t::set_visible(bool visible)
	{
		_ui->get_tree().set_visible(_root, visible, false);
	}

	void editor_widget_thumbnail_t::refresh_frame()
	{
		_last_check_tick = editor_app_t::get().get_tick_count();
		editor_asset_thumbnail_manager_t::get().request_thumbnail(_thumbnail);

		const resource_entry_t* entry = resource_manager_t::get().find_entry(_thumbnail);

		if (entry != nullptr && entry->state == resource_state_e::ready)
			set_texture_frame();
		else
			set_default_frame();
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
		_ui->get_paint().set_disabled_color(_root, theme.color_frame_dark);
	}

	void editor_widget_thumbnail_t::set_texture_frame()
	{
		const editor_theme_t& theme = editor_theme_t::get();

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = vec4f_t::one;
		rect.fill_color_b		 = vec4f_t::one;
		rect.rounding			 = theme.item_rounding;
		rect.rounding_segs		 = 4;

		ui::ui_render_state_t state = {};
		state.pipeline				= "shaders/editor/editor_ui_texture.hlsl"_hs;
		state.constants[0].handle	= _thumbnail;
		state.constants[0].type		= ui::ui_resource_type_e::evictable_texture;

		_ui->get_paint().set_rect(_root, rect, state);
		_ui->get_paint().set_disabled_color(_root, theme.color_text_disabled);
	}

	void editor_widget_thumbnail_t::on_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data)
	{
		editor_widget_thumbnail_t& thumbnail = *static_cast<editor_widget_thumbnail_t*>(user_data);
		const u64				   tick		 = editor_app_t::get().get_tick_count();

		if (tick - thumbnail._last_check_tick < editor_asset_thumbnail_manager_t::REQUEST_INTERVAL_TICKS)
			return;

		thumbnail._last_check_tick	  = tick;
		const ui::layout_tree_t& tree = ui.get_tree();

		if (!ui.get_input().is_widget_visible(tree, id))
			return;

		const vec4f_t& clip = tree.out(id).clip;

		if (clip.z <= 0.0f || clip.w <= 0.0f)
			return;

		thumbnail.refresh_frame();
	}
}
