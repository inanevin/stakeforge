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

#include "assets/editor_asset_type.hpp"

#include <sfg/common/size_definitions.hpp>

namespace sfg
{
	struct editor_asset_t;

	enum class editor_asset_thumbnail_source_e : u8
	{
		builtin,
		generated,
	};

	struct editor_asset_thumbnail_t
	{
		sid_t							texture = NULL_SID;
		editor_asset_thumbnail_source_e source	= editor_asset_thumbnail_source_e::builtin;
	};

	class editor_asset_thumbnailer_t final
	{
	public:
		editor_asset_thumbnailer_t()											 = delete;
		~editor_asset_thumbnailer_t()											 = delete;
		editor_asset_thumbnailer_t(const editor_asset_thumbnailer_t&)			 = delete;
		editor_asset_thumbnailer_t& operator=(const editor_asset_thumbnailer_t&) = delete;

		static editor_asset_thumbnail_t get_thumbnail(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool						ensure(const editor_asset_t& asset, const char* asset_name = nullptr, bool force = false);
		static bool						ensure_thumbnail_loaded(const editor_asset_t& asset, const char* asset_name = nullptr);
		static sid_t					get_thumbnail_guid(sid_t asset_guid);
		static sid_t					get_builtin_thumbnail_guid(editor_asset_type_e asset_type);
	};
}
