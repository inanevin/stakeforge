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

#include "ui/widgets/editor_widgets_common.hpp"
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class input_router_t;
	class ui_context;
	struct key_event_t;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	struct editor_popup_item_desc_t;

	enum class editor_dropdown_width_e : u8
	{
		sum_children,
		parent_relative,
		fixed,
	};

	enum class editor_dropdown_pos_y_e : u8
	{
		flow,
		center,
	};

	struct editor_dropdown_item_t
	{
		const char* text  = nullptr;
		u64			value = 0;
	};

	using editor_dropdown_selected_fn	 = u16 (*)(void* user_data);
	using editor_dropdown_pressed_fn	 = void (*)(u16 value, void* user_data);
	using editor_dropdown_build_title_fn = const char* (*)(u64 value, void* user_data);

	struct editor_dropdown_field_t
	{
		span_t<u8*> fields	   = {};
		size_t		field_size = 0;
	};

	struct editor_dropdown_config_t
	{
		const editor_dropdown_item_t*  items				= nullptr;
		const char*					   title				= nullptr;
		editor_dropdown_selected_fn	   selected				= nullptr;
		editor_dropdown_pressed_fn	   pressed				= nullptr;
		editor_dropdown_build_title_fn build_title			= nullptr;
		void*						   user_data			= nullptr;
		void*						   title_user_data		= nullptr;
		editor_dropdown_field_t		   field				= {};
		editor_widget_callbacks_t	   callbacks			= {};
		u16							   item_count			= 0;
		editor_dropdown_width_e		   width				= editor_dropdown_width_e::sum_children;
		editor_dropdown_pos_y_e		   pos_y				= editor_dropdown_pos_y_e::flow;
		f32							   fixed_width			= 0.0f;
		bool						   title_from_selection = true;
		bool						   is_bitmask			= false;
	};

	class editor_dropdown_t final
	{
	public:
		editor_dropdown_t()									   = default;
		~editor_dropdown_t()								   = default;
		editor_dropdown_t(const editor_dropdown_t&)			   = delete;
		editor_dropdown_t& operator=(const editor_dropdown_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_dropdown_config_t& config);
		void uninit();
		void close();
		void update_field_data(editor_dropdown_field_t field);
		void set_items(const editor_dropdown_item_t* items, u16 item_count);
		void refresh_field_data();
		void refresh_title();
		void set_mixed(bool mixed);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		bool		is_field_bound() const;
		u64			read_field_value(const u8* field) const;
		void		write_field_value(u8* field, u64 value) const;
		void		modify_field(u64 value);
		u64			get_selected() const;
		const char* get_selected_text() const;
		void		build_popup_items(editor_popup_item_desc_t* items) const;
		void		refresh_popup_items();
		void		open_popup();

		static void on_root_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_root_key(ui::input_router_t& router, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data);
		static void on_popup_item_pressed(u16 value, void* user_data);
		static void on_popup_closed(void* user_data);

	private:
		editor_dropdown_config_t		 _config = {};
		vector_t<editor_dropdown_item_t> _items;
		vector_t<u8*>					 _fields;
		ui::ui_context*					 _ui				  = nullptr;
		u64								 _selected_value	  = 0;
		ui::widget_id_t					 _root				  = NULL_WIDGET;
		ui::widget_id_t					 _title				  = NULL_WIDGET;
		ui::widget_id_t					 _icon_frame		  = NULL_WIDGET;
		bool							 _mixed				  = false;
		bool							 _bitmask_edit_active = false;
	};
}
