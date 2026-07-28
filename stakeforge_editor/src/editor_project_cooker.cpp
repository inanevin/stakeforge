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
#include "editor_app.hpp"
#include "editor_project.hpp"
#include "editor_project_cook_options.hpp"
#include "editor_settings.hpp"
#include "editor_surface_controller.hpp"
#include "ui/editor_modal_progress_bar.hpp"
#include "ui/editor_modal_project_cooker.hpp"

#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/runtime/project/project_package_meta.hpp>
#include <sfg/serialization/serialization.hpp>
#include <sfg/vendor/taskflow/taskflow.hpp>

namespace sfg
{
	editor_project_cooker_t::editor_project_cooker_t()	= default;
	editor_project_cooker_t::~editor_project_cooker_t() = default;

	void editor_project_cooker_t::init()
	{
		SFG_ASSERT(!_initialized);

		_cook_options	= make_unique<editor_project_cook_options_t>();
		_package_meta	= make_unique<project_package_meta_t>();
		_options_modal	= make_unique<editor_modal_project_cooker_t>();
		_progress_modal = make_unique<editor_modal_progress_bar_t>();
		_initialized	= true;
	}

	void editor_project_cooker_t::uninit()
	{
		SFG_ASSERT(_initialized);

		if (_cook_state.load(std::memory_order_acquire) == cook_state_e::cooking)
			editor_app_t::get().get_editor_work_executor().wait_for_all();

		_progress_modal.reset();
		_options_modal.reset();
		_package_meta.reset();
		_cook_options.reset();

		_cook_state.store(cook_state_e::idle, std::memory_order_relaxed);
		_initialized = false;
	}

	void editor_project_cooker_t::tick()
	{
		SFG_ASSERT(_initialized);

		const cook_state_e cook_state = _cook_state.load(std::memory_order_acquire);

		if (cook_state == cook_state_e::idle || cook_state == cook_state_e::cooking)
			return;

		editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;
		modal.close_modal();
		_cook_state.store(cook_state_e::idle, std::memory_order_relaxed);

		if (cook_state == cook_state_e::failed)
		{
			const editor_modal_button_desc_t buttons[] = {
				{.text = "Close"},
			};

			modal.request_modal("Cook Project Failed", "Could not write project_meta.sfg_bin.", buttons, static_cast<u16>(std::size(buttons)), editor_modal_severity_e::error);
		}
	}

	void editor_project_cooker_t::request_cook()
	{
		SFG_ASSERT(_initialized);
		SFG_ASSERT(_cook_state.load(std::memory_order_relaxed) == cook_state_e::idle);

		editor_modal_controller_t& modal = *editor_surface_controller_t::get().get_main_surface().modal_controller;

		_options_modal->request(modal, editor_settings_t::get().project_cook);
	}

	void editor_project_cooker_t::cook_project(const editor_project_cook_options_t& options)
	{
		SFG_ASSERT(_initialized);
		SFG_ASSERT(_cook_state.load(std::memory_order_relaxed) == cook_state_e::idle);

		editor_modal_controller_t&		 modal	   = *editor_surface_controller_t::get().get_main_surface().modal_controller;
		const editor_modal_button_desc_t buttons[] = {
			{.text = "Close"},
		};

		if (options.main_world == NULL_SID || options.main_world == 0)
		{
			modal.request_modal("Cook Project", "Select a main world.", buttons, static_cast<u16>(std::size(buttons)), editor_modal_severity_e::error);
			return;
		}

		*_cook_options = options;

		*_package_meta					 = {};
		_package_meta->project_settings	 = editor_project_t::get().settings.project_settings;
		_package_meta->main_world		 = options.main_world;
		_package_meta->window_resolution = options.resolution;
		_package_meta->window_style		 = options.is_borderless ? window_style_e::borderless : window_style_e::app_window;
		_package_meta->is_fullscreen	 = options.is_fullscreen;

		_cook_state.store(cook_state_e::cooking, std::memory_order_relaxed);

		_progress_modal->set_progress(0.0f);
		const editor_modal_content_desc_t content = _progress_modal->get_content_desc();
		modal.request_modal("Cooking Project", "Preparing the project package.", false, nullptr, 0, &content);

		const string_t target_path = editor_project_t::get()._runtime.cook_path + project_package_meta_t::FILE_NAME;

		editor_app_t::get().get_editor_work_executor().silent_async([this, target_path]() {
			const cook_state_e result = cook_project_worker(target_path.c_str()) ? cook_state_e::succeeded : cook_state_e::failed;
			_cook_state.store(result, std::memory_order_release);
		});
	}

	bool editor_project_cooker_t::cook_project_worker(const char* target_path)
	{
		ostream_t stream = {};

		if (!_package_meta->serialize(stream))
		{
			SFG_ERR("failed to serialize project package metadata");
			return false;
		}

		return serializer_t::save_to_file_atomic(target_path, stream);
	}
}
