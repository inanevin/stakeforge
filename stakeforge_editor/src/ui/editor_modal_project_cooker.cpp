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

#include "ui/editor_modal_project_cooker.hpp"
#include "editor_project_cooker.hpp"
#include "editor_settings.hpp"

namespace sfg
{
#define PROJECT_COOK_MODAL_FRAME_WIDTH_X 0.4f

	void editor_modal_project_cooker_t::request(editor_modal_controller_t& modal, const editor_project_cook_options_t& options)
	{
		_options = options;

		const editor_modal_button_desc_t buttons[] = {
			{.text = "Cancel", .callback = on_cancel, .user_data = this},
			{.text = "Cook", .callback = on_cook, .user_data = this},
		};

		const editor_modal_content_desc_t content{
			.init		   = init_content,
			.uninit		   = uninit_content,
			.user_data	   = this,
			.frame_width_x = PROJECT_COOK_MODAL_FRAME_WIDTH_X,
			.fill_x		   = true,
		};

		modal.request_modal("Cook Project", "Configure the packaged worlds, resources, and window.", true, buttons, static_cast<u16>(std::size(buttons)), &content);
	}

	void editor_modal_project_cooker_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		void* object = &_options;

		_reflection.init(ui, parent, {.objects = {.data = &object, .size = 1}, .type_id = type_id_t<editor_project_cook_options_t>::value});
	}

	void editor_modal_project_cooker_t::uninit()
	{
		_reflection.uninit();
	}

	void editor_modal_project_cooker_t::init_content(ui::ui_context& ui, ui::widget_id_t parent, void* user_data)
	{
		static_cast<editor_modal_project_cooker_t*>(user_data)->init(ui, parent);
	}

	void editor_modal_project_cooker_t::uninit_content(void* user_data)
	{
		static_cast<editor_modal_project_cooker_t*>(user_data)->uninit();
	}

	void editor_modal_project_cooker_t::on_cancel(void* user_data)
	{
		static_cast<editor_modal_project_cooker_t*>(user_data)->_options = {};
	}

	void editor_modal_project_cooker_t::on_cook(void* user_data)
	{
		editor_modal_project_cooker_t& cooker_modal = *static_cast<editor_modal_project_cooker_t*>(user_data);
		editor_settings_t&			   settings		= editor_settings_t::get();

		settings.project_cook = cooker_modal._options;
		settings.save();

		editor_project_cooker_t::get().cook_project(settings.project_cook);

		cooker_modal._options = {};
	}
}
