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

#include "assets/editor_asset_type.hpp"

namespace sfg
{
	struct editor_asset_t;
	struct shader_data_definition_t;

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
		static bool cook_shader(const editor_asset_t& asset, const char* asset_name = nullptr, shader_data_definition_t* out_definition = nullptr);
		static bool cook_material(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_texture_sampler(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_physical_material(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_animation_library(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_animation_graph(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_texture(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_sprite(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_curve(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_ragdoll(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_font(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_skeleton(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_animation(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_mesh(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_physics_collision_mesh(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_cubemap(const editor_asset_t& asset, const char* asset_name = nullptr);
		static bool cook_prefab(const editor_asset_t& asset, const char* asset_name = nullptr);
	};
}
