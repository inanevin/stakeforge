/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#pragma once

#include "tools/tools_exports.hpp"

#include <sfg/core/common/size_definitions.hpp>
#include <sfg/core/data/string.hpp>
#include <sfg/core/memory/pool_handle.hpp>

namespace sfg
{
	struct editor_project_cook_options_t;
	struct editor_work_handle_tag_t;
	struct project_package_meta_t;
	class editor_modal_progress_bar_t;
	class editor_modal_project_cooker_t;

	class SFG_TOOLS_API editor_project_cooker_t final
	{
	public:
		editor_project_cooker_t();
		~editor_project_cooker_t();
		editor_project_cooker_t(const editor_project_cooker_t&)			   = delete;
		editor_project_cooker_t& operator=(const editor_project_cooker_t&) = delete;

		inline static editor_project_cooker_t& get()
		{
			static editor_project_cooker_t instance = {};

			return instance;
		}

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init();
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void request_cook();
		void cook_project(const editor_project_cook_options_t& options);

	private:
		enum class cook_state_e : u8
		{
			idle,
			compiling_native,
			cooking,
		};

		bool compile_native_worker();
		bool cook_project_worker();
		void complete_work(pool_handle_t<u32, editor_work_handle_tag_t> work_handle, bool succeeded);
		bool publish_game_files();

	private:
		string_t									 _cook_failure_reason  = {};
		string_t									 _packaged_plugin_path = {};
		string_t									 _target_path		   = {};
		editor_project_cook_options_t*				 _cook_options		   = nullptr;
		project_package_meta_t*						 _package_meta		   = nullptr;
		editor_modal_project_cooker_t*				 _options_modal		   = nullptr;
		editor_modal_progress_bar_t*				 _progress_modal	   = nullptr;
		pool_handle_t<u32, editor_work_handle_tag_t> _work_handle		   = {};
		cook_state_e								 _cook_state		   = cook_state_e::idle;
	};
}
