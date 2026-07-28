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

#include "ui/widgets/editor_widget_color_wheel.hpp"

namespace sfg
{
	struct editor_popup_color_wheel_config_t
	{
		span_t<color_t*>				   fields		   = {};
		editor_color_wheel_edit_begin_fn   edit_begin	   = nullptr;
		editor_color_wheel_data_changed_fn on_data_changed = nullptr;
		void*							   user_data	   = nullptr;
		bool							   hdr			   = false;
	};

	class editor_popup_color_wheel_t final
	{
	public:
		editor_popup_color_wheel_t()											 = default;
		~editor_popup_color_wheel_t()											 = default;
		editor_popup_color_wheel_t(const editor_popup_color_wheel_t&)			 = delete;
		editor_popup_color_wheel_t& operator=(const editor_popup_color_wheel_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_popup_color_wheel_config_t& config);
		void uninit();

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		static vec2f_t calculate_size(ui::ui_context& ui, bool hdr);
		static vec2f_t calculate_position(ui::ui_context& ui, const vec2f_t& requested_position, bool hdr = false);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		editor_widget_color_wheel_t _color_wheel = {};
		ui::ui_context*				_ui			 = nullptr;
		ui::widget_id_t				_root		 = NULL_WIDGET;
	};
}
