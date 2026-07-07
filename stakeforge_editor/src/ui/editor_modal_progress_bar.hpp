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

#include "ui/editor_modal_controller.hpp"
#include "ui/widgets/editor_widget_progress_bar.hpp"
#include "ui/widgets/editor_widget_spinner.hpp"

namespace sfg
{
	class editor_modal_progress_bar_t final
	{
	public:
		editor_modal_progress_bar_t()												   = default;
		~editor_modal_progress_bar_t()												   = default;
		editor_modal_progress_bar_t(const editor_modal_progress_bar_t&)				   = delete;
		editor_modal_progress_bar_t& operator=(const editor_modal_progress_bar_t&)	   = delete;
		editor_modal_progress_bar_t(editor_modal_progress_bar_t&&) noexcept			   = default;
		editor_modal_progress_bar_t& operator=(editor_modal_progress_bar_t&&) noexcept = default;

		void						init(ui::ui_context& ui, ui::widget_id_t parent);
		void						uninit();
		void						set_progress(f32 progress);
		editor_modal_content_desc_t get_content_desc();

		inline f32 get_progress() const
		{
			return _progress;
		}

	private:
		static void init_content(ui::ui_context& ui, ui::widget_id_t parent, void* user_data);
		static void uninit_content(void* user_data);

	private:
		editor_widget_progress_bar_t _bar	  = {};
		editor_widget_spinner_t		 _spinner = {};

		ui::ui_context* _ui				= nullptr;
		ui::widget_id_t _root			= NULL_WIDGET;
		ui::widget_id_t _spinner_row	= NULL_WIDGET;
		ui::widget_id_t _spinner_holder = NULL_WIDGET;
		f32				_progress		= 0.0f;
	};
}
