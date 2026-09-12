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

#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class input_router_t;
	class ui_context;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	class editor_widget_button_t;

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
		editor_modal_content_init_fn   init			 = nullptr;
		editor_modal_content_uninit_fn uninit		 = nullptr;
		void*						   user_data	 = nullptr;
		f32							   frame_width_x = 0.0f;
		bool						   fill_x		 = false;
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

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		bool is_visible() const;

	private:
		static constexpr u32 MAX_BUTTONS = 4;

		static void handle_button_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		void		on_button_click(ui::widget_id_t id, ui::mouse_button_e btn);
		void		set_visible(bool visible);
		void		close_content();
		u32			find_button_index(ui::widget_id_t id) const;

	private:
		editor_modal_button_desc_t	_buttons[MAX_BUTTONS]		 = {};
		editor_modal_content_desc_t _content					 = {};
		ui::ui_context*				_ui							 = nullptr;
		ui::widget_id_t				_foreground					 = NULL_WIDGET;
		ui::widget_id_t				_window						 = NULL_WIDGET;
		ui::widget_id_t				_title						 = NULL_WIDGET;
		ui::widget_id_t				_description				 = NULL_WIDGET;
		ui::widget_id_t				_container					 = NULL_WIDGET;
		ui::widget_id_t				_button_row					 = NULL_WIDGET;
		editor_widget_button_t*		_button_widgets[MAX_BUTTONS] = {};
		u16							_button_count				 = 0;
		bool						_buttons_visible			 = false;
		bool						_content_active				 = false;
		bool						_visible					 = false;
	};
}
