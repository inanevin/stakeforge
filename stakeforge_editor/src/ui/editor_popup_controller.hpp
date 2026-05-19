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

#include "ui/widgets/editor_widgets_input_field.hpp"
#include <sfg/math/vec2f.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

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
	using editor_popup_item_pressed_fn = void (*)(u16 id, void* user_data);
	using editor_popup_input_closed_fn = void (*)(const char* value, void* user_data);

	struct editor_popup_item_desc_t
	{
		const char* text	 = nullptr;
		u16			id		 = 0;
		bool		selected = false;
	};

	struct editor_popup_desc_t
	{
		const editor_popup_item_desc_t* items			 = nullptr;
		editor_popup_item_pressed_fn	pressed			 = nullptr;
		void*							user_data		 = nullptr;
		vec2f_t							pos				 = {};
		f32								width			 = 0.0f;
		u16								item_count		 = 0;
		bool							close_on_pressed = true;
	};

	struct editor_input_popup_desc_t
	{
		editor_popup_input_closed_fn closed		 = nullptr;
		void*						 user_data	 = nullptr;
		const char*					 text		 = nullptr;
		const char*					 placeholder = nullptr;
		vec2f_t						 pos		 = {};
		f32							 width		 = 0.0f;
	};

	class editor_popup_controller_t final
	{
	public:
		static constexpr u32 MAX_CONTROLLERS = 16;

		editor_popup_controller_t()												   = default;
		~editor_popup_controller_t()											   = default;
		editor_popup_controller_t(const editor_popup_controller_t&)				   = delete;
		editor_popup_controller_t& operator=(const editor_popup_controller_t&)	   = delete;
		editor_popup_controller_t(editor_popup_controller_t&&) noexcept			   = default;
		editor_popup_controller_t& operator=(editor_popup_controller_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void request_popup(const editor_popup_desc_t& desc);
		void request_input_popup(const editor_input_popup_desc_t& desc);
		void close_popup(bool notify_input = false);

		static editor_popup_controller_t* find(ui::ui_context& ui);

	private:
		static constexpr u32 MAX_ITEMS = 16;

		enum class popup_mode_e : u8
		{
			none,
			items,
			input,
		};

		void set_visible(bool visible);
		void refresh_rows();
		void refresh_layout();
		u32	 find_row(ui::widget_id_t id) const;

		static void on_row_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_popup_outside(ui::input_router_t& router, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_input_submitted(void* user_data);
		static void draw_selected_marker(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data);

	private:
		ui::ui_context*			  _ui					  = nullptr;
		ui::widget_id_t			  _foreground			  = NULL_WIDGET;
		ui::widget_id_t			  _frame				  = NULL_WIDGET;
		ui::widget_id_t			  _row_frames[MAX_ITEMS]  = {};
		ui::widget_id_t			  _row_markers[MAX_ITEMS] = {};
		ui::widget_id_t			  _row_labels[MAX_ITEMS]  = {};
		editor_popup_desc_t		  _desc					  = {};
		editor_input_popup_desc_t _input_desc			  = {};
		editor_input_field_t	  _input				  = {};
		editor_popup_item_desc_t  _items[MAX_ITEMS]		  = {};
		popup_mode_e			  _mode					  = popup_mode_e::none;
		bool					  _visible				  = false;
	};
}
