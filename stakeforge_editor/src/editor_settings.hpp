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

#include "assets/editor_glb_importer.hpp"
#include "editor_layout.hpp"

#include <sfg/runtime/resources/audio_cook.hpp>
#include <sfg/runtime/resources/skybox_hdr_cook.hpp>
#include <sfg/runtime/resources/texture_cook.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	struct editor_import_settings_t
	{
		texture_cook_config_t	 texture	= {};
		audio_cook_config_t		 audio		= {};
		skybox_hdr_cook_config_t skybox_hdr = {};
		glb_cook_config_t		 glb		= {};
	};

	struct editor_settings_t
	{
		inline static editor_settings_t& get()
		{
			static editor_settings_t instance;
			return instance;
		}

		bool save();
		bool ensure_loaded();

		editor_layout_t			 layout			   = {};
		editor_import_settings_t import			   = {};
		string_t				 last_project_path = "";
	};

	void to_json(nlohmann::json& j, const editor_import_settings_t& settings);
	void from_json(const nlohmann::json& j, editor_import_settings_t& settings);
	void to_json(nlohmann::json& j, const editor_settings_t& settings);
	void from_json(const nlohmann::json& j, editor_settings_t& settings);
}
