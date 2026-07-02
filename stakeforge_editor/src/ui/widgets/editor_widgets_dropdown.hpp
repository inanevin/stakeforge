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

#pragma once

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
		u16			value = 0;
	};

	using editor_dropdown_selected_fn = u16 (*)(void* user_data);
	using editor_dropdown_pressed_fn  = void (*)(u16 value, void* user_data);

	struct editor_dropdown_field_t
	{
		span_t<u8*> fields	   = {};
		size_t		field_size = 0;
	};

	struct editor_dropdown_config_t
	{
		const editor_dropdown_item_t* items				   = nullptr;
		const char*					  title				   = nullptr;
		editor_dropdown_selected_fn	  selected			   = nullptr;
		editor_dropdown_pressed_fn	  pressed			   = nullptr;
		void*						  user_data			   = nullptr;
		editor_dropdown_field_t		  field				   = {};
		u16							  item_count		   = 0;
		editor_dropdown_width_e		  width				   = editor_dropdown_width_e::sum_children;
		editor_dropdown_pos_y_e		  pos_y				   = editor_dropdown_pos_y_e::flow;
		f32							  fixed_width		   = 0.0f;
		bool						  title_from_selection = true;
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
		void refresh_field_data();
		void refresh_title();
		void set_mixed(bool mixed);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		bool		is_field_bound() const;
		u16			read_field_value(const u8* field) const;
		void		write_field_value(u8* field, u16 value) const;
		void		modify_field(u16 value);
		u16			get_selected() const;
		const char* get_selected_text() const;
		void		open_popup();

		static void on_root_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_root_key(ui::input_router_t& router, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data);
		static void on_popup_item_pressed(u16 value, void* user_data);

	private:
		ui::ui_context*					 _ui		 = nullptr;
		ui::widget_id_t					 _root		 = NULL_WIDGET;
		ui::widget_id_t					 _title		 = NULL_WIDGET;
		ui::widget_id_t					 _icon_frame = NULL_WIDGET;
		editor_dropdown_config_t		 _config	 = {};
		vector_t<editor_dropdown_item_t> _items;
		vector_t<u8*>					 _fields;
		u16								 _selected_value = 0;
		bool							 _mixed			 = false;
	};
}
