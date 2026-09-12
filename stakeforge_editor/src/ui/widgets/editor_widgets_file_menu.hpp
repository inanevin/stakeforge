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

#pragma once

#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class input_router_t;
	class ui_context;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	struct editor_action_menu_row_desc_t;
	struct editor_action_menu_style_t;

	struct editor_file_menu_item_desc_t
	{
		const char*							 text	   = nullptr;
		const editor_action_menu_row_desc_t* rows	   = nullptr;
		u16									 row_count = 0;
	};

	struct editor_file_menu_style_t
	{
		vec4f_t frame_color			 = {0, 0, 0, 0};
		vec4f_t hover_color			 = {0, 0, 0, 0};
		vec4f_t press_color			 = {0, 0, 0, 0};
		vec4f_t selected_color		 = {0, 0, 0, 0};
		vec4f_t dropdown_color		 = {0, 0, 0, 0};
		vec4f_t text_color			 = {1, 1, 1, 1};
		vec4f_t shortcut_color		 = {1, 1, 1, 1};
		vec4f_t title_color			 = {1, 1, 1, 1};
		vec4f_t title_line_color	 = {1, 1, 1, 1};
		vec4f_t icon_color			 = {1, 1, 1, 1};
		f32		button_width		 = 72.0f;
		f32		row_height			 = 24.0f;
		f32		text_size			 = 12.0f;
		f32		shortcut_size		 = 12.0f;
		f32		title_size			 = 10.0f;
		f32		title_line_thickness = 1.0f;
		f32		icon_size			 = 8.0f;
		f32		padding_x			 = 8.0f;
		f32		padding_y			 = 4.0f;
		f32		shortcut_gap		 = 32.0f;
		f32		title_gap			 = 8.0f;
	};

	class editor_file_menu_t final
	{
	public:
		editor_file_menu_t()										 = default;
		~editor_file_menu_t()										 = default;
		editor_file_menu_t(const editor_file_menu_t&)				 = delete;
		editor_file_menu_t& operator=(const editor_file_menu_t&)	 = delete;
		editor_file_menu_t(editor_file_menu_t&&) noexcept			 = default;
		editor_file_menu_t& operator=(editor_file_menu_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_file_menu_item_desc_t* items, u16 item_count, const editor_file_menu_style_t& style, void (*command_fn)(u16 command, void* user_data), void* command_user_data);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void close();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		static constexpr u32 MAX_TOP_ITEMS = 8;

		static void				   handle_top_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void				   handle_top_hover(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void				   handle_action_menu_closed(void* user_data);
		void					   on_top_click(ui::widget_id_t id, ui::mouse_button_e btn);
		void					   on_top_hover(ui::widget_id_t id);
		void					   open_top(u32 index);
		void					   refresh_top_frames();
		u32						   find_top_index(ui::widget_id_t id) const;
		editor_action_menu_style_t get_action_menu_style() const;

	private:
		ui::widget_id_t						_root					   = NULL_WIDGET;
		ui::widget_id_t						_top_frames[MAX_TOP_ITEMS] = {};
		ui::widget_id_t						_top_labels[MAX_TOP_ITEMS] = {};
		ui::ui_context*						_ui						   = nullptr;
		const editor_file_menu_item_desc_t* _items					   = nullptr;
		void (*_command_fn)(u16 command, void* user_data)			   = nullptr;
		void*					 _command_user_data					   = nullptr;
		u32						 _selected_top						   = 0;
		u16						 _item_count						   = 0;
		editor_file_menu_style_t _style								   = {};
		bool					 _open								   = false;
	};
}
