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

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/atomic.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/unique.hpp>

namespace sfg
{
	struct editor_project_cook_options_t;
	struct project_package_meta_t;
	class editor_modal_progress_bar_t;
	class editor_modal_project_cooker_t;

	class editor_project_cooker_t final
	{
	public:
		editor_project_cooker_t();
		~editor_project_cooker_t();
		editor_project_cooker_t(const editor_project_cooker_t&)			   = delete;
		editor_project_cooker_t& operator=(const editor_project_cooker_t&) = delete;

		inline static editor_project_cooker_t& get()
		{
			static editor_project_cooker_t instance;
			return instance;
		}

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void tick();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void request_cook();
		void cook_project(const editor_project_cook_options_t& options);

	private:
		enum class cook_state_e : u8
		{
			idle,
			cooking,
			succeeded,
			failed,
		};

		bool cook_project_worker(const char* target_path);
		bool publish_game_files();

	private:
		string_t								_cook_failure_reason = {};
		unique_t<editor_project_cook_options_t> _cook_options;
		unique_t<project_package_meta_t>		_package_meta;
		unique_t<editor_modal_project_cooker_t> _options_modal;
		unique_t<editor_modal_progress_bar_t>	_progress_modal;
		atomic_t<cook_state_e>					_cook_state = cook_state_e::idle;
	};
}
