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
	class ui_context;
}

namespace sfg
{
	struct editor_widget_progress_bar_config_t
	{
		const char* progress_text	= "";
		f32			progress_amount = 0.0f;
		f32			frame_height	= 0.0f;
	};

	class editor_widget_progress_bar_t final
	{
	public:
		editor_widget_progress_bar_t()													 = default;
		~editor_widget_progress_bar_t()													 = default;
		editor_widget_progress_bar_t(const editor_widget_progress_bar_t&)				 = delete;
		editor_widget_progress_bar_t& operator=(const editor_widget_progress_bar_t&)	 = delete;
		editor_widget_progress_bar_t(editor_widget_progress_bar_t&&) noexcept			 = default;
		editor_widget_progress_bar_t& operator=(editor_widget_progress_bar_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_progress_bar_config_t& config);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void update_progress(f32 progress);
		void update_progress_text(const char* text);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		void refresh_progress_amount();

	private:
		ui::ui_context* _ui					   = nullptr;
		ui::widget_id_t _root				   = NULL_WIDGET;
		ui::widget_id_t _progress_text		   = NULL_WIDGET;
		ui::widget_id_t _progress_frame		   = NULL_WIDGET;
		ui::widget_id_t _progress_fill		   = NULL_WIDGET;
		ui::widget_id_t _progress_amount_label = NULL_WIDGET;
		f32				_progress_amount	   = 0.0f;
	};
}
