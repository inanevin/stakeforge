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
	using editor_modal_button_fn = void (*)(void* user_data);

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

	class editor_modal_controller_t final
	{
	public:
		static constexpr u32 MAX_CONTROLLERS		  = 16;
		static constexpr u32 MAX_SUB_DESCRIPTION_ROWS = 16;

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
			const char* title, const char* description, const char* const* sub_description_rows, u16 sub_description_row_count, const editor_modal_button_desc_t* buttons, u16 button_count, editor_modal_severity_e severity = editor_modal_severity_e::normal);
		void start_loading_bar(const char* title, const char* description, f32 progress = 0.0f);
		void progress_loading_bar(f32 progress);
		void end_loading_bar();
		void close_modal();

		static editor_modal_controller_t* find(ui::ui_context& ui);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline f32 get_loading_bar_progress() const
		{
			return _loading_bar_progress;
		}

		bool is_loading_bar_active() const;

	private:
		static constexpr u32 MAX_BUTTONS = 4;

		enum class modal_mode_e : u8
		{
			none,
			buttons,
			loading_bar,
		};

		static void handle_button_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		void		on_button_click(ui::widget_id_t id, ui::mouse_button_e btn);
		void		set_visible(bool visible);
		void		update_loading_bar(f32 progress);
		u32			find_button_index(ui::widget_id_t id) const;

	private:
		ui::ui_context*			   _ui											   = nullptr;
		ui::widget_id_t			   _foreground									   = NULL_WIDGET;
		ui::widget_id_t			   _dimmer										   = NULL_WIDGET;
		ui::widget_id_t			   _window										   = NULL_WIDGET;
		ui::widget_id_t			   _title										   = NULL_WIDGET;
		ui::widget_id_t			   _description									   = NULL_WIDGET;
		ui::widget_id_t			   _loading_bar									   = NULL_WIDGET;
		ui::widget_id_t			   _loading_bar_fill							   = NULL_WIDGET;
		ui::widget_id_t			   _loading_bar_label							   = NULL_WIDGET;
		ui::widget_id_t			   _button_row									   = NULL_WIDGET;
		ui::widget_id_t			   _sub_description_rows[MAX_SUB_DESCRIPTION_ROWS] = {};
		ui::widget_id_t			   _button_frames[MAX_BUTTONS]					   = {};
		ui::widget_id_t			   _button_labels[MAX_BUTTONS]					   = {};
		editor_modal_button_desc_t _buttons[MAX_BUTTONS]						   = {};
		f32						   _loading_bar_progress						   = 0.0f;
		u16						   _sub_description_row_count					   = 0;
		u16						   _button_count								   = 0;
		modal_mode_e			   _mode										   = modal_mode_e::none;
		bool					   _visible										   = false;
	};
}
