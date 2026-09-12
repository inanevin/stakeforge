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

#include "assets/editor_glb_importer.hpp"
#include "editor_layout.hpp"
#include "editor_project_cook_options.hpp"

#include <sfg/runtime/resources/audio_cook.hpp>
#include <sfg/runtime/resources/cubemap_cook.hpp>
#include <sfg/runtime/resources/sprite_cook.hpp>
#include <sfg/runtime/resources/texture_cook.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	struct editor_import_settings_t
	{
		texture_cook_config_t texture = {};
		audio_cook_config_t	  audio	  = {};
		cubemap_cook_config_t cubemap = {};
		glb_cook_config_t	  glb	  = {};
		sprite_cook_config_t  sprite  = {};
	};

	struct editor_settings_configurable_t
	{
		f32 editor_ui_scale = 1.0f;

		void normalize();

		bool operator==(const editor_settings_configurable_t&) const = default;
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

		editor_layout_t				   layout			 = {};
		editor_import_settings_t	   import			 = {};
		editor_project_cook_options_t  project_cook		 = {};
		string_t					   last_project_path = "";
		editor_settings_configurable_t configurable		 = {};
	};

	SFG_DEFINE_TYPE_ID(editor_settings_configurable_t);

	struct editor_settings_reflection_t
	{
		editor_settings_reflection_t();
	};

	inline editor_settings_reflection_t g_reflect_editor_settings;

	void to_json(nlohmann::json& j, const editor_import_settings_t& settings);
	void from_json(const nlohmann::json& j, editor_import_settings_t& settings);
	void to_json(nlohmann::json& j, const editor_settings_t& settings);
	void from_json(const nlohmann::json& j, editor_settings_t& settings);
}
