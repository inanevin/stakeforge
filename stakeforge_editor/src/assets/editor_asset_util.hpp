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

#include "assets/editor_asset_node.hpp"

#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>

namespace sfg
{
	struct editor_asset_t;

	class editor_asset_util_t final
	{
	public:
		editor_asset_util_t()									   = delete;
		~editor_asset_util_t()									   = delete;
		editor_asset_util_t(const editor_asset_util_t&)			   = delete;
		editor_asset_util_t& operator=(const editor_asset_util_t&) = delete;

		static sid_t					  generate_unique_asset_guid(span_t<const sid_t> reserved_guids = {});
		static sid_t					  try_read_existing_guid(const char* path);
		static const editor_asset_node_t* find_asset_node(sid_t guid);
		static string_t					  find_asset_path(sid_t guid);
		static const char*				  find_asset_display_name(sid_t guid);
		static bool						  duplicate_folder(editor_asset_node_handle_t folder_node, string_t* out_duplicated_path = nullptr);
		static bool						  rename_folder(editor_asset_node_handle_t folder_node, const char* new_path);
		static bool						  move_folder(editor_asset_node_handle_t folder_node, editor_asset_node_handle_t target_folder_node);
		static bool						  rename_file(editor_asset_node_handle_t file_node, const char* new_path);
		static bool						  duplicate_asset(const editor_asset_t& asset, editor_asset_node_handle_t asset_node, string_t* out_duplicated_path = nullptr);
		static bool						  rename_asset(const editor_asset_t& asset, editor_asset_node_handle_t asset_node, const char* new_path);
		static bool						  move_asset(const editor_asset_t& asset, editor_asset_node_handle_t asset_node, editor_asset_node_handle_t target_folder_node);
	};
}
