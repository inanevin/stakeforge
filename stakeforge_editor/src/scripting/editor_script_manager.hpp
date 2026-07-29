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

#include "script_compiler.hpp"
#include "ui/editor_modal_progress_bar.hpp"

#include <sfg/data/atomic.hpp>

#include <thread>

namespace sfg
{
	class editor_script_manager_t final
	{
	public:
		editor_script_manager_t()										   = default;
		~editor_script_manager_t()										   = default;
		editor_script_manager_t(const editor_script_manager_t&)			   = delete;
		editor_script_manager_t& operator=(const editor_script_manager_t&) = delete;

		inline static editor_script_manager_t& get()
		{
			static editor_script_manager_t instance = {};

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

		void compile_scripts();

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		inline bool is_initial_activation_completed() const
		{
			return _initial_activation_completed;
		}

	private:
		enum class compile_state_e : u8
		{
			idle,
			compiling,
			succeeded,
			failed,
		};

		void start_compile();
		bool activate_staged_scripts();

	private:
		script_compile_result_t		_compile_result				  = {};
		std::thread					_compile_thread				  = {};
		editor_modal_progress_bar_t _progress_modal				  = {};
		atomic_t<compile_state_e>	_compile_state				  = compile_state_e::idle;
		bool						_initialized				  = false;
		bool						_compile_requested			  = false;
		bool						_modal_open					  = false;
		bool						_initial_activation_completed = false;
	};
}
