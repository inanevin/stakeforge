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

#include "ui/widgets/editor_widget_progress_bar.hpp"

#include <sfg/math/vec2u16.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	struct editor_splash_screen_config_t
	{
		vec2u16_t owner_size = {};
	};

	class editor_splash_screen_t final
	{
	public:
		editor_splash_screen_t()											 = default;
		~editor_splash_screen_t()											 = default;
		editor_splash_screen_t(const editor_splash_screen_t&)				 = delete;
		editor_splash_screen_t& operator=(const editor_splash_screen_t&)	 = delete;
		editor_splash_screen_t(editor_splash_screen_t&&) noexcept			 = default;
		editor_splash_screen_t& operator=(editor_splash_screen_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_splash_screen_config_t& config);
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
		ui::ui_context*				 _ui		   = nullptr;
		ui::widget_id_t				 _root		   = NULL_WIDGET;
		ui::widget_id_t				 _texture_bg   = NULL_WIDGET;
		ui::widget_id_t				 _strikes	   = NULL_WIDGET;
		ui::widget_id_t				 _column	   = NULL_WIDGET;
		ui::widget_id_t				 _title		   = NULL_WIDGET;
		ui::widget_id_t				 _version	   = NULL_WIDGET;
		ui::widget_id_t				 _build		   = NULL_WIDGET;
		ui::widget_id_t				 _project_path = NULL_WIDGET;
		ui::widget_id_t				 _project_name = NULL_WIDGET;
		editor_widget_progress_bar_t _progress	   = {};
	};
}
