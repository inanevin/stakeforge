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

#include "editor_project_cook_options.hpp"
#include "ui/editor_modal_controller.hpp"
#include "ui/widgets/editor_widget_reflection.hpp"

namespace sfg
{
	class editor_modal_project_cooker_t final
	{
	public:
		editor_modal_project_cooker_t()												   = default;
		~editor_modal_project_cooker_t()											   = default;
		editor_modal_project_cooker_t(const editor_modal_project_cooker_t&)			   = delete;
		editor_modal_project_cooker_t& operator=(const editor_modal_project_cooker_t&) = delete;

		void request(editor_modal_controller_t& modal, const editor_project_cook_options_t& options);

	private:
		void init(ui::ui_context& ui, ui::widget_id_t parent);
		void uninit();

		static void init_content(ui::ui_context& ui, ui::widget_id_t parent, void* user_data);
		static void uninit_content(void* user_data);
		static void on_cancel(void* user_data);
		static void on_cook(void* user_data);

	private:
		editor_project_cook_options_t _options	  = {};
		editor_widget_reflection_t	  _reflection = {};
	};
}
