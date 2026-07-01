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
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class input_router_t;
	class paint_layer_t;
	class ui_context;
	class vg_canvas_t;
	enum class mouse_button_e : u8;
	struct key_event_t;
}

namespace sfg
{
	enum class editor_input_field_field_type_e : u8
	{
		string,
		char_array,
		pod_number,
	};

	struct editor_input_field_field_t
	{
		editor_input_field_field_type_e type	   = editor_input_field_field_type_e::string;
		span_t<u8*>						fields	   = {};
		size_t							field_size = 0;
		bool							is_slider  = false;
	};

	using editor_input_field_data_changed_fn = void (*)(void* user_data);
	using editor_input_field_submitted_fn	 = void (*)(void* user_data);

	struct editor_input_field_config_t
	{
		editor_input_field_field_t		   field		   = {};
		const char*						   placeholder	   = nullptr;
		editor_input_field_data_changed_fn on_data_changed = nullptr;
		editor_input_field_submitted_fn	   on_submitted	   = nullptr;
		void*							   user_data	   = nullptr;
		f32								   increment	   = 0.1f;
		f32								   min_value	   = 0.0f;
		f32								   max_value	   = 1.0f;
	};

	class editor_input_field_t final
	{
	public:
		editor_input_field_t()										 = default;
		~editor_input_field_t()										 = default;
		editor_input_field_t(const editor_input_field_t&)			 = delete;
		editor_input_field_t& operator=(const editor_input_field_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_input_field_config_t& config);
		void uninit();
		void select_all();
		void set_visible(bool visible);
		void update_field_data(editor_input_field_field_t field);
		void refresh_field_data();

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline const char* get_text() const
		{
			return _text;
		}

		inline f32 get_number() const
		{
			return _number_value;
		}

	private:
		static constexpr u32 TEXT_CAPACITY = 256;

		void refresh_text();
		void commit_number_text();
		void update_number_from_text();
		void modify_field();
		void set_text_raw(const char* value);
		void format_number();
		void insert_char(char c);
		void insert_text(const char* text);
		bool insert_char_data(char c);
		void erase_selection();
		void erase_range(u32 start, u32 end);
		void set_caret(u32 index);
		void update_drag_selection(const vec2f_t& pos);
		void apply_number_delta(f32 delta_x);
		void rebuild_text_advances();
		void reset_caret_blink();
		u32	 index_from_pos(const vec2f_t& pos) const;
		f32	 text_width(u32 len) const;
		bool accepts_char(char c) const;
		f32	 read_pod_number(const u8* data) const;
		void write_pod_number(u8* data, f32 value);

		static void on_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_double_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_hover_enter(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_hover_exit(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_focus_gain(ui::input_router_t& router, ui::widget_id_t id, bool from_nav, void* user_data);
		static void on_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void on_focus_lose(ui::input_router_t& router, ui::widget_id_t id, bool from_nav, void* user_data);
		static void on_key(ui::input_router_t& router, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data);
		static void on_pre_layout_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static void draw_slider(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data);
		static void draw_overlay(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data);

	private:
		ui::ui_context*				_ui							  = nullptr;
		ui::widget_id_t				_root						  = NULL_WIDGET;
		ui::widget_id_t				_slider						  = NULL_WIDGET;
		ui::widget_id_t				_label						  = NULL_WIDGET;
		ui::widget_id_t				_overlay					  = NULL_WIDGET;
		vector_t<u8*>				_fields						  = {};
		editor_input_field_config_t _config						  = {};
		char						_text[TEXT_CAPACITY]		  = {};
		f32							_text_advances[TEXT_CAPACITY] = {};
		u32							_text_len					  = 0;
		u32							_caret						  = 0;
		u32							_selection_anchor			  = 0;
		f32							_number_value				  = 0.0f;
		f32							_blink_seconds				  = 0.0f;
		f32							_text_advance_ui_scale		  = 0.0f;
		f32							_text_advance_dpi_scale		  = 0.0f;
		bool						_mixed						  = false;
	};
}
