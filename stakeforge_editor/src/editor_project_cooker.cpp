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

#include "editor_project_cooker.hpp"
#include "editor_project_cook_options.hpp"
#include "editor_settings.hpp"
#include "editor_surface_controller.hpp"
#include "ui/editor_modal_progress_bar.hpp"
#include "ui/editor_modal_project_cooker.hpp"

#include <sfg/io/assert.hpp>

namespace sfg
{
	editor_project_cooker_t::editor_project_cooker_t()	= default;
	editor_project_cooker_t::~editor_project_cooker_t() = default;

	void editor_project_cooker_t::init()
	{
		SFG_ASSERT(!_initialized);

		_cook_options	= make_unique<editor_project_cook_options_t>();
		_options_modal	= make_unique<editor_modal_project_cooker_t>();
		_progress_modal = make_unique<editor_modal_progress_bar_t>();
		_initialized	= true;
	}

	void editor_project_cooker_t::uninit()
	{
		SFG_ASSERT(_initialized);

		_progress_modal.reset();
		_options_modal.reset();
		_cook_options.reset();

		_is_cooking	 = false;
		_initialized = false;
	}

	void editor_project_cooker_t::tick()
	{
		SFG_ASSERT(_initialized);

		if (!_is_cooking)
			return;
	}

	void editor_project_cooker_t::request_cook()
	{
		SFG_ASSERT(_initialized);
		SFG_ASSERT(!_is_cooking);

		editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;

		_options_modal->request(modal, editor_settings_t::get().project_cook);
	}

	void editor_project_cooker_t::cook_project(const editor_project_cook_options_t& options)
	{
		SFG_ASSERT(_initialized);
		SFG_ASSERT(!_is_cooking);

		*_cook_options = options;
		_is_cooking	   = true;

		editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;

		_progress_modal->set_progress(0.0f);
		const editor_modal_content_desc_t content = _progress_modal->get_content_desc();
		modal.request_modal("Cooking Project", "Preparing the project package.", false, nullptr, 0, &content);
	}
}
