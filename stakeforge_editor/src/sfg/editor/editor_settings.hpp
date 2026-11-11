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

#include "editor/editor_exports.hpp"

#include "editor/assets/editor_glb_importer.hpp"
#include "editor/editor_project_cook_options.hpp"
#include "editor/ui/panels/editor_panel_types.hpp"

#include <sfg/core/runtime/resources/audio_cook.hpp>
#include <sfg/core/runtime/resources/cubemap_cook.hpp>
#include <sfg/core/runtime/resources/sprite_cook.hpp>
#include <sfg/core/runtime/resources/texture_cook.hpp>
#include <sfg/core/vendor/nhlohmann/json_fwd.hpp>

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

	struct SFG_EDITOR_API editor_settings_configurable_t
	{
		f32 editor_ui_scale = 1.0f;

		void normalize();

		bool operator==(const editor_settings_configurable_t&) const = default;
	};

	struct editor_panel_window_size_t
	{
		// logical pixels; zero means no saved size
		f32 width  = 0.0f;
		f32 height = 0.0f;
	};

	struct SFG_EDITOR_API editor_settings_t
	{
		static constexpr u32 WIRE_VERSION = 0;

		inline static editor_settings_t& get()
		{
			static editor_settings_t instance = {};

			return instance;
		}

		bool save();

		editor_import_settings_t	   import														 = {};
		editor_project_cook_options_t  project_cook													 = {};
		string_t					   last_project_path											 = "";
		editor_settings_configurable_t configurable													 = {};
		editor_panel_window_size_t	   panel_window_sizes[static_cast<u8>(editor_panel_type_e::max)] = {};
	};

	SFG_DEFINE_TYPE_ID(editor_settings_configurable_t);

	struct editor_settings_reflection_t
	{
		editor_settings_reflection_t();
	};

	SFG_EDITOR_API void to_json(nlohmann::json& j, const editor_import_settings_t& settings);
	SFG_EDITOR_API void from_json(const nlohmann::json& j, editor_import_settings_t& settings);
	SFG_EDITOR_API void to_json(nlohmann::json& j, const editor_panel_window_size_t& size);
	SFG_EDITOR_API void from_json(const nlohmann::json& j, editor_panel_window_size_t& size);
	SFG_EDITOR_API void to_json(nlohmann::json& j, const editor_settings_t& settings);
	SFG_EDITOR_API void from_json(const nlohmann::json& j, editor_settings_t& settings);
}
