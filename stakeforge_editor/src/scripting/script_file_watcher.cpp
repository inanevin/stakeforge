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

#include "script_file_watcher.hpp"
#include "editor_script_manager.hpp"
#include "assets/editor_asset_manager.hpp"
#include "editor_project.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>

namespace sfg
{
#define SCRIPT_FILE_WATCH_SCAN_TICKS   100
#define SCRIPT_FILE_WATCH_STABLE_SCANS 2

	script_file_watcher_t& script_file_watcher_t::get()
	{
		static script_file_watcher_t instance = {};

		return instance;
	}

	void script_file_watcher_t::init()
	{
		_files.reserve(64);
		accept_current_state();
	}

	void script_file_watcher_t::uninit()
	{
		_files.resize(0);
		_accepted_signature	 = 0;
		_pending_signature	 = 0;
		_accepted_file_count = 0;
		_pending_file_count	 = 0;
		_scan_tick			 = 0;
		_stable_scan_count	 = 0;
	}

	void script_file_watcher_t::tick()
	{
		_scan_tick++;

		if (_scan_tick < SCRIPT_FILE_WATCH_SCAN_TICKS)
			return;

		_scan_tick = 0;

		u32		  file_count = 0;
		const u64 signature	 = scan_signature(file_count);

		if (signature == _accepted_signature && file_count == _accepted_file_count)
		{
			_pending_signature	= signature;
			_pending_file_count = file_count;
			_stable_scan_count	= 0;
			return;
		}

		if (signature != _pending_signature || file_count != _pending_file_count)
		{
			_pending_signature	= signature;
			_pending_file_count = file_count;
			_stable_scan_count	= 1;
			return;
		}

		_stable_scan_count++;

		if (_stable_scan_count < SCRIPT_FILE_WATCH_STABLE_SCANS)
			return;

		editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();

		asset_manager.sync_directory_from_disk(asset_manager.get_root_node());
		editor_script_manager_t::get().compile_scripts();
	}

	void script_file_watcher_t::accept_current_state()
	{
		_accepted_signature = scan_signature(_accepted_file_count);
		_pending_signature	= _accepted_signature;
		_pending_file_count = _accepted_file_count;
		_scan_tick			= 0;
		_stable_scan_count	= 0;
	}

	u64 script_file_watcher_t::scan_signature(u32& out_file_count)
	{
		_files.resize(0);
		file_system_t::get_files_recursive(editor_project_t::get()._runtime.assets_path.c_str(), _files);

		u64 signature  = hashing_t::hash_u64("stakeforge_csharp_files");
		out_file_count = 0;

		for (const string_t& file : _files)
		{
			if (file_system_t::get_file_extension(file) != "cs")
				continue;

			if (file.find("/_cache/") != string_t::npos || file.find("/_sfg_assets/") != string_t::npos || file.find("/_game_assets/") != string_t::npos)
				continue;

			const u64 modified_ticks = file_system_t::get_last_modified_ticks(file.c_str());
			const u64 file_size		 = file_system_t::get_file_size(file.c_str());
			const u64 file_hash		 = hashing_t::hash_u64(file.c_str());

			signature ^= hashing_t::hash_u64_combine(file_hash, modified_ticks, file_size);
			out_file_count++;
		}

		return hashing_t::hash_u64_combine(signature, out_file_count);
	}
}
