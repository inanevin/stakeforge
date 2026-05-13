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

#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class input_router_t;
	class ui_context;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	using editor_file_menu_command_fn = void (*)(u16 command, void* user_data);

	enum class editor_file_menu_row_kind_e : u8
	{
		item,
		title,
	};

	struct editor_file_menu_row_desc_t
	{
		editor_file_menu_row_kind_e		   kind		   = editor_file_menu_row_kind_e::item;
		const char*						   text		   = nullptr;
		const char*						   shortcut	   = nullptr;
		const editor_file_menu_row_desc_t* children	   = nullptr;
		u16								   child_count = 0;
		u16								   command	   = 0;
	};

	struct editor_file_menu_item_desc_t
	{
		const char*						   text		 = nullptr;
		const editor_file_menu_row_desc_t* rows		 = nullptr;
		u16								   row_count = 0;
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

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_file_menu_item_desc_t* items, u16 item_count, const editor_file_menu_style_t& style, editor_file_menu_command_fn command_fn, void* command_user_data);
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
		static constexpr u32 MAX_DEPTH	   = 4;
		static constexpr u32 MAX_ROWS	   = 16;

		static void handle_top_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void handle_top_hover(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void handle_row_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void handle_row_hover(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void handle_popup_outside(ui::input_router_t& router, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		void		on_top_click(ui::widget_id_t id, ui::mouse_button_e btn);
		void		on_top_hover(ui::widget_id_t id);
		void		on_row_click(ui::widget_id_t id, ui::mouse_button_e btn);
		void		on_row_hover(ui::widget_id_t id);
		void		open_top(u32 index);
		void		show_dropdown(u32 depth, const editor_file_menu_row_desc_t* rows, u16 row_count, const vec4f_t& anchor);
		void		hide_dropdowns_from(u32 depth);
		void		refresh_popup_scope();
		void		refresh_top_frames();
		u32			find_top_index(ui::widget_id_t id) const;
		bool		find_row_index(ui::widget_id_t id, u32& out_depth, u32& out_row) const;
		f32			measure_text_width(const char* text, resource_handle_t font_handle, f32 point_size) const;

	private:
		ui::ui_context*						_ui									  = nullptr;
		const editor_file_menu_item_desc_t* _items								  = nullptr;
		u16									_item_count							  = 0;
		editor_file_menu_style_t			_style								  = {};
		editor_file_menu_command_fn			_command_fn							  = nullptr;
		void*								_command_user_data					  = nullptr;
		ui::widget_id_t						_root								  = NULL_WIDGET;
		ui::widget_id_t						_foreground							  = NULL_WIDGET;
		ui::widget_id_t						_top_frames[MAX_TOP_ITEMS]			  = {};
		ui::widget_id_t						_top_labels[MAX_TOP_ITEMS]			  = {};
		ui::widget_id_t						_panels[MAX_DEPTH]					  = {};
		ui::widget_id_t						_row_frames[MAX_DEPTH][MAX_ROWS]	  = {};
		ui::widget_id_t						_row_labels[MAX_DEPTH][MAX_ROWS]	  = {};
		ui::widget_id_t						_row_shortcuts[MAX_DEPTH][MAX_ROWS]	  = {};
		ui::widget_id_t						_row_icons[MAX_DEPTH][MAX_ROWS]		  = {};
		ui::widget_id_t						_row_title_lines[MAX_DEPTH][MAX_ROWS] = {};
		const editor_file_menu_row_desc_t*	_active_rows[MAX_DEPTH]				  = {};
		u16									_active_row_counts[MAX_DEPTH]		  = {};
		u32									_active_depth						  = 0;
		u32									_selected_top						  = 0;
		bool								_open								  = false;
	};
}
