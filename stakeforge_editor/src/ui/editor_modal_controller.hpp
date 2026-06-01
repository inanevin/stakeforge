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

#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class input_router_t;
	class ui_context;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	using editor_modal_button_fn		 = void (*)(void* user_data);
	using editor_modal_content_init_fn	 = void (*)(ui::ui_context& ui, ui::widget_id_t parent, void* user_data);
	using editor_modal_content_uninit_fn = void (*)(void* user_data);

	enum class editor_modal_severity_e : u8
	{
		normal,
		error,
		warning,
	};

	struct editor_modal_button_desc_t
	{
		const char*			   text		 = nullptr;
		editor_modal_button_fn callback	 = nullptr;
		void*				   user_data = nullptr;
	};

	struct editor_modal_content_desc_t
	{
		editor_modal_content_init_fn   init		 = nullptr;
		editor_modal_content_uninit_fn uninit	 = nullptr;
		void*						   user_data = nullptr;
		bool						   fill_x	 = false;
	};

	class editor_modal_controller_t final
	{
	public:
		static constexpr u32 MAX_CONTROLLERS = 16;

		editor_modal_controller_t()												   = default;
		~editor_modal_controller_t()											   = default;
		editor_modal_controller_t(const editor_modal_controller_t&)				   = delete;
		editor_modal_controller_t& operator=(const editor_modal_controller_t&)	   = delete;
		editor_modal_controller_t(editor_modal_controller_t&&) noexcept			   = default;
		editor_modal_controller_t& operator=(editor_modal_controller_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void request_modal(const char* title, const char* description, const editor_modal_button_desc_t* buttons, u16 button_count, editor_modal_severity_e severity = editor_modal_severity_e::normal);
		void request_modal(
			const char* title, const char* description, bool show_buttons, const editor_modal_button_desc_t* buttons, u16 button_count, const editor_modal_content_desc_t* content = nullptr, editor_modal_severity_e severity = editor_modal_severity_e::normal);
		void set_body_text(const char* text);
		void close_modal();

		static editor_modal_controller_t* find(ui::ui_context& ui);

	private:
		static constexpr u32 MAX_BUTTONS = 4;

		static void handle_button_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		void		on_button_click(ui::widget_id_t id, ui::mouse_button_e btn);
		void		set_visible(bool visible);
		void		close_content();
		u32			find_button_index(ui::widget_id_t id) const;

	private:
		ui::ui_context*				_ui							= nullptr;
		ui::widget_id_t				_foreground					= NULL_WIDGET;
		ui::widget_id_t				_dimmer						= NULL_WIDGET;
		ui::widget_id_t				_window						= NULL_WIDGET;
		ui::widget_id_t				_title						= NULL_WIDGET;
		ui::widget_id_t				_description				= NULL_WIDGET;
		ui::widget_id_t				_container					= NULL_WIDGET;
		ui::widget_id_t				_button_row					= NULL_WIDGET;
		ui::widget_id_t				_button_frames[MAX_BUTTONS] = {};
		ui::widget_id_t				_button_labels[MAX_BUTTONS] = {};
		editor_modal_button_desc_t	_buttons[MAX_BUTTONS]		= {};
		editor_modal_content_desc_t _content					= {};
		u16							_button_count				= 0;
		bool						_buttons_visible			= false;
		bool						_content_active				= false;
		bool						_visible					= false;
	};
}
