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

#include "assets/editor_asset_importer.hpp"
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>

namespace tf
{
	class Executor;
}

namespace sfg
{
	class editor_asset_manager_t;

	class editor_asset_manager_util_t final
	{
	public:
		editor_asset_manager_util_t()											   = delete;
		~editor_asset_manager_util_t()											   = delete;
		editor_asset_manager_util_t(const editor_asset_manager_util_t&)			   = delete;
		editor_asset_manager_util_t& operator=(const editor_asset_manager_util_t&) = delete;

		using on_complete_fn	 = void (*)(void* user_data);
		using import_progress_fn = void (*)(void* user_data, f32 progress, const char* text, bool is_completed);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		static void rescan(editor_asset_manager_t& asset_manager, const char* assets_dir);
		static void ensure_integrity(editor_asset_manager_t& asset_manager);
		static void ensure_project_assets_async(editor_asset_manager_t& asset_manager, tf::Executor& executor, on_complete_fn on_complete, void* user_data);
		static void import_assets_async(const char* target_directory, span_t<const string_t> paths, span_t<const editor_asset_import_options_t> import_options, tf::Executor& executor, import_progress_fn callback, void* user_data);
		static void ensure_thumbnails_loaded(editor_asset_manager_t& asset_manager);
		static void ensure_default_meshes();
	};
}
