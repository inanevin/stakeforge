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
#include <sfg/data/string.hpp>
#include <sfg/memory/pool_handle.hpp>

namespace sfg
{
	enum class editor_asset_node_type_e : u8
	{
		folder,
		file,
		asset,
	};

	enum editor_asset_node_flags_e : u8
	{
		editor_asset_node_flag_hidden	= 1 << 0, // folder name begins with '_' — never shown in the assets panel
		editor_asset_node_flag_promoted = 1 << 1, // root assets folder — its rows are collapsed into the parent listing
	};

	struct editor_asset_node_t
	{
		u64						 asset_id = 0;
		string_t				 name;
		string_t				 full_path;
		editor_asset_node_type_e type  = editor_asset_node_type_e::folder;
		u8						 flags = 0;
	};

	using editor_asset_node_handle_t = pool_handle_t<u32, editor_asset_node_t>;
}
