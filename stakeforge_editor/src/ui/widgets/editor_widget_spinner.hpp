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

#include <sfg/common/size_definitions.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
	class paint_layer_t;
	class vg_canvas_t;
}

namespace sfg
{
	struct editor_widget_spinner_config_t
	{
		vec4f_t outer_color = {1.0f, 1.0f, 1.0f, 1.0f};
		vec4f_t inner_color = {1.0f, 1.0f, 1.0f, 1.0f};
	};

	class editor_widget_spinner_t final
	{
	public:
		editor_widget_spinner_t()										   = default;
		~editor_widget_spinner_t()										   = default;
		editor_widget_spinner_t(const editor_widget_spinner_t&)			   = delete;
		editor_widget_spinner_t& operator=(const editor_widget_spinner_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_spinner_config_t& config);
		void uninit();

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		static void draw(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data);

	private:
		ui::ui_context* _ui			 = nullptr;
		ui::widget_id_t _root		 = NULL_WIDGET;
		vec4f_t			_outer_color = {};
		vec4f_t			_inner_color = {};
	};
}
