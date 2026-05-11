// Copyright (c) 2025 Inan Evin
#pragma once

#include "widgets/editor_widgets_file_menu.hpp"
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	struct editor_surface_t;

	enum class editor_project_prompt_action_e : u8
	{
		none,
		new_project,
		load_project,
	};

	class editor_base_t final
	{
	public:
		editor_base_t()									   = default;
		~editor_base_t()								   = default;
		editor_base_t(const editor_base_t&)				   = delete;
		editor_base_t& operator=(const editor_base_t&)	   = delete;
		editor_base_t(editor_base_t&&) noexcept			   = default;
		editor_base_t& operator=(editor_base_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, editor_surface_t& surface);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void prompt_no_project_modal();
		void prompt_project_save_modal(editor_project_prompt_action_e action);
		void complete_project_save_prompt(bool save);
		void cancel_project_save_prompt();
		void set_current_project_name(const char* name);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::ui_context& get_ui()
		{
			return *_ui;
		}
		inline const ui::ui_context& get_ui() const
		{
			return *_ui;
		}
		inline editor_surface_t& get_surface()
		{
			return *_surface;
		}
		inline const editor_surface_t& get_surface() const
		{
			return *_surface;
		}

	private:
		ui::ui_context*				   _ui					  = nullptr;
		editor_surface_t*			   _surface				  = nullptr;
		ui::widget_id_t				   _base				  = NULL_WIDGET;
		ui::widget_id_t				   _top_section			  = NULL_WIDGET;
		ui::widget_id_t				   _top_row_left		  = NULL_WIDGET;
		ui::widget_id_t				   _top_row_strikes		  = NULL_WIDGET;
		ui::widget_id_t				   _top_row_mid			  = NULL_WIDGET;
		ui::widget_id_t				   _top_mid_file		  = NULL_WIDGET;
		ui::widget_id_t				   _top_mid_divider		  = NULL_WIDGET;
		ui::widget_id_t				   _top_mid_util		  = NULL_WIDGET;
		ui::widget_id_t				   _top_row_right		  = NULL_WIDGET;
		ui::widget_id_t				   _top_row_right_buttons = NULL_WIDGET;
		ui::widget_id_t				   _window_minimize		  = NULL_WIDGET;
		ui::widget_id_t				   _window_maximize		  = NULL_WIDGET;
		ui::widget_id_t				   _window_close		  = NULL_WIDGET;
		ui::widget_id_t				   _project_label		  = NULL_WIDGET;
		ui::widget_id_t				   _title_group			  = NULL_WIDGET;
		ui::widget_id_t				   _title_label			  = NULL_WIDGET;
		ui::widget_id_t				   _version_label		  = NULL_WIDGET;
		ui::widget_id_t				   _build_label			  = NULL_WIDGET;
		ui::widget_id_t				   _mid_section			  = NULL_WIDGET;
		ui::widget_id_t				   _bottom_section		  = NULL_WIDGET;
		editor_file_menu_t			   _file_menu;
		editor_project_prompt_action_e _pending_project_prompt_action = editor_project_prompt_action_e::none;

		static void on_no_project_open(void* user_data);
		static void on_no_project_create(void* user_data);
	};
}
