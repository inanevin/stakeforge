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

namespace sfg
{
	struct editor_asset_t;

	class editor_asset_cooker_t final
	{
	public:
		editor_asset_cooker_t()										   = delete;
		~editor_asset_cooker_t()									   = delete;
		editor_asset_cooker_t(const editor_asset_cooker_t&)			   = delete;
		editor_asset_cooker_t& operator=(const editor_asset_cooker_t&) = delete;

		static bool cook_asset(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool is_cookable(editor_asset_type_e asset_type);
		static bool is_asset_cooked(const editor_asset_t& asset);

		static bool cook_audio(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_shader(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_material(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_texture_sampler(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_physical_material(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_animation_state_machine(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_texture(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_font(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_skeleton(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_animation(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_mesh(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_hdr_skybox(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_prefab(const editor_asset_t& asset, const char* asset_name = nullptr);
	};
}
