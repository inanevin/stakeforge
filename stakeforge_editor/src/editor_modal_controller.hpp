// Copyright (c) 2025 Inan Evin
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
		void close_modal();

	private:
		static constexpr u32 MAX_BUTTONS = 4;

		static void handle_button_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		void		on_button_click(ui::widget_id_t id, ui::mouse_button_e btn);
		void		set_visible(bool visible);
		u32			find_button_index(ui::widget_id_t id) const;

	private:
		ui::ui_context*			   _ui						   = nullptr;
		ui::widget_id_t			   _foreground				   = NULL_WIDGET;
		ui::widget_id_t			   _dimmer					   = NULL_WIDGET;
		ui::widget_id_t			   _window					   = NULL_WIDGET;
		ui::widget_id_t			   _title					   = NULL_WIDGET;
		ui::widget_id_t			   _description				   = NULL_WIDGET;
		ui::widget_id_t			   _button_row				   = NULL_WIDGET;
		ui::widget_id_t			   _button_frames[MAX_BUTTONS] = {};
		ui::widget_id_t			   _button_labels[MAX_BUTTONS] = {};
		editor_modal_button_desc_t _buttons[MAX_BUTTONS]	   = {};
		u16						   _button_count			   = 0;
		bool					   _visible					   = false;
	};
}
