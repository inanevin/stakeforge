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

#include "ui/editor_action_menu_common.hpp"
#include <sfg/math/vec2f.hpp>

namespace sfg::ui
{
	class input_router_t;
	class paint_layer_t;
	class ui_context;
	class vg_canvas_t;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	struct editor_action_menu_desc_t
	{
		editor_action_menu_style_t			 style			   = {};
		const editor_action_menu_row_desc_t* rows			   = nullptr;
		editor_action_menu_command_fn		 command_fn		   = nullptr;
		void*								 command_user_data = nullptr;
		editor_action_menu_closed_fn		 closed_fn		   = nullptr;
		void*								 closed_user_data  = nullptr;
		vec2f_t								 pos			   = {};
		ui::widget_id_t						 owner_root		   = NULL_WIDGET;
		u16									 row_count		   = 0;
	};

	class editor_action_menu_controller_t final
	{
	public:
		static constexpr u32 MAX_CONTROLLERS = 16;

		editor_action_menu_controller_t()													   = default;
		~editor_action_menu_controller_t()													   = default;
		editor_action_menu_controller_t(const editor_action_menu_controller_t&)				   = delete;
		editor_action_menu_controller_t& operator=(const editor_action_menu_controller_t&)	   = delete;
		editor_action_menu_controller_t(editor_action_menu_controller_t&&) noexcept			   = default;
		editor_action_menu_controller_t& operator=(editor_action_menu_controller_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void request_action_menu(const editor_action_menu_desc_t& desc);
		void close_action_menu();
		bool is_open() const;

		static editor_action_menu_controller_t* find(ui::ui_context& ui);

	private:
		static constexpr u32 MAX_DEPTH = 4;
		static constexpr u32 MAX_ROWS  = 64;

		void show_panel(u32 depth, const editor_action_menu_row_desc_t* rows, u16 row_count, const vec4f_t& anchor);
		void hide_panels_from(u32 depth);
		void refresh_popup_scope();
		bool find_row_index(ui::widget_id_t id, u32& out_depth, u32& out_row) const;
		f32	 measure_text_width(const char* text, resource_handle_t font_handle, f32 point_size) const;

		static void handle_row_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void handle_row_hover(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void handle_panel_hover(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data);
		static void handle_popup_outside(ui::input_router_t& router, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);

	private:
		const editor_action_menu_row_desc_t* _active_rows[MAX_DEPTH]			   = {};
		editor_action_menu_desc_t			 _desc								   = {};
		ui::widget_id_t						 _foreground						   = NULL_WIDGET;
		ui::widget_id_t						 _panels[MAX_DEPTH]					   = {};
		ui::widget_id_t						 _row_frames[MAX_DEPTH][MAX_ROWS]	   = {};
		ui::widget_id_t						 _row_labels[MAX_DEPTH][MAX_ROWS]	   = {};
		ui::widget_id_t						 _row_shortcuts[MAX_DEPTH][MAX_ROWS]   = {};
		ui::widget_id_t						 _row_icons[MAX_DEPTH][MAX_ROWS]	   = {};
		ui::widget_id_t						 _row_icon_labels[MAX_DEPTH][MAX_ROWS] = {};
		ui::widget_id_t						 _row_title_lines[MAX_DEPTH][MAX_ROWS] = {};
		u16									 _active_row_counts[MAX_DEPTH]		   = {};
		ui::ui_context*						 _ui								   = nullptr;
		u32									 _active_depth						   = 0;
		bool								 _open								   = false;
		bool								 _closing							   = false;
	};
}
