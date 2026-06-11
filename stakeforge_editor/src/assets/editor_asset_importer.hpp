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

#include "assets/editor_asset.hpp"

#include <sfg/data/frame_vector.hpp>
#include <sfg/runtime/resources/audio_cook.hpp>
#include <sfg/runtime/resources/glb_cook.hpp>
#include <sfg/runtime/resources/texture_cook.hpp>

namespace sfg
{
	enum class editor_asset_import_type_e : u8
	{
		invalid,
		audio,
		texture,
		model,
	};

	struct editor_asset_import_options_t
	{
		texture_cook_config_t	   texture_cook_config = {};
		audio_cook_config_t		   audio_cook_config   = {};
		glb_cook_config_t		   glb_cook_config	   = {};
		editor_asset_import_type_e type				   = editor_asset_import_type_e::invalid;
	};

	class editor_asset_importer_t final
	{
	public:
		editor_asset_importer_t()										   = delete;
		~editor_asset_importer_t()										   = delete;
		editor_asset_importer_t(const editor_asset_importer_t&)			   = delete;
		editor_asset_importer_t& operator=(const editor_asset_importer_t&) = delete;

		static bool make_import_options(editor_asset_import_options_t& out_options, const char* asset_name);
		static bool import_asset(editor_asset_node_handle_t directory_node, const char* source_full_path, span_t<const editor_asset_import_options_t> options, vector_t<editor_asset_t>& out_assets);

	private:
		static bool make_asset(editor_asset_node_handle_t directory_node, const char* asset_name, editor_asset_t& asset, editor_asset_type_e asset_type, editor_asset_source_type_e source_type, const char* source_full_path = nullptr);
	};
}
