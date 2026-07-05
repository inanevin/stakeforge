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

#include "ui/docking/dock_widget.hpp"
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	struct input_router_t;
	class ui_context;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	struct window_runtime_t;
	struct vec2i16_t;

	class editor_secondary_base_t final
	{
	public:
		editor_secondary_base_t()											   = default;
		~editor_secondary_base_t()											   = default;
		editor_secondary_base_t(const editor_secondary_base_t&)				   = delete;
		editor_secondary_base_t& operator=(const editor_secondary_base_t&)	   = delete;
		editor_secondary_base_t(editor_secondary_base_t&&) noexcept			   = default;
		editor_secondary_base_t& operator=(editor_secondary_base_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, window_runtime_t& runtime);
		void uninit();

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		bool is_window_drag_region(const vec2i16_t& pos) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline dock_widget_t& get_dock_widget()
		{
			return _dock_widget;
		}

		inline const dock_widget_t& get_dock_widget() const
		{
			return _dock_widget;
		}

	private:
		static void on_minimize_window(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_maximize_window(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_close_window(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);

	private:
		ui::ui_context*	  _ui			  = nullptr;
		window_runtime_t* _runtime		  = nullptr;
		ui::widget_id_t	  _root			  = NULL_WIDGET;
		ui::widget_id_t	  _title_frame	  = NULL_WIDGET;
		ui::widget_id_t	  _window_buttons = NULL_WIDGET;
		dock_widget_t	  _dock_widget;
	};
}
