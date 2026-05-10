// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
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

		void init(ui::ui_context& ui);
		void uninit();

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

	private:
		ui::ui_context* _ui				= nullptr;
		ui::widget_id_t _base			= NULL_WIDGET;
		ui::widget_id_t _top_section	= NULL_WIDGET;
		ui::widget_id_t _top_row_left	= NULL_WIDGET;
		ui::widget_id_t _top_row_mid	= NULL_WIDGET;
		ui::widget_id_t _top_row_right	= NULL_WIDGET;
		ui::widget_id_t _title_label	= NULL_WIDGET;
		ui::widget_id_t _mid_section	= NULL_WIDGET;
		ui::widget_id_t _bottom_section = NULL_WIDGET;
	};
}
