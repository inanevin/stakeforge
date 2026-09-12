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

#include "assets/editor_asset_node.hpp"
#include "assets/editor_asset_type.hpp"
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/resources/common_resources.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
#define DEFAULT_ALBEDO_TEXTURE_ASSET_GUID				 1002
#define DEFAULT_ORM_TEXTURE_ASSET_GUID					 1003
#define DEFAULT_NORMAL_TEXTURE_ASSET_GUID				 1004
#define DEFAULT_EMISSIVE_TEXTURE_ASSET_GUID				 1005
#define DEFAULT_PHYSICAL_MATERIAL_ASSET_GUID			 1008
#define DEFAULT_LINEAR_SAMPLER_ASSET_GUID				 1009
#define DEFAULT_NEAREST_SAMPLER_ASSET_GUID				 1010
#define DEFAULT_ANISOTROPIC_SAMPLER_ASSET_GUID			 1011
#define DEFAULT_QWANTANI_DUSK_CUBEMAP_ASSET_GUID		 1012
#define DEFAULT_LINEAR_SAMPLER_REPEAT_ASSET_GUID		 1013
#define DEFAULT_NEAREST_SAMPLER_REPEAT_ASSET_GUID		 1014
#define DEFAULT_ANISOTROPIC_SAMPLER_REPEAT_ASSET_GUID	 1019
#define GIZMO_MESH_TRANSLATION							 1020
#define GIZMO_MESH_ROTATION								 1021
#define GIZMO_MESH_SCALE								 1022
#define DEFAULT_CUBE_SKYBOX_SHADER_ASSET_GUID			 1025
#define DEFAULT_GRADIENT_SKYBOX_SHADER_ASSET_GUID		 1026
#define DEFAULT_CUBE_SKYBOX_MATERIAL_ASSET_GUID			 1027
#define DEFAULT_GRADIENT_SKYBOX_MATERIAL_ASSET_GUID		 1028
#define DEFAULT_LIT_SHADER_ASSET_GUID					 1029
#define DEFAULT_OBJECT_UNLIT_SHADER_ASSET_GUID			 1030
#define DEFAULT_OPAQUE_MATERIAL_ASSET_GUID				 1031
#define DEFAULT_OPAQUE_UNLIT_MATERIAL_ASSET_GUID		 1032
#define DEFAULT_TRANSPARENT_MATERIAL_ASSET_GUID			 1033
#define DEFAULT_TRANSPARENT_UNLIT_MATERIAL_ASSET_GUID	 1034
#define DEFAULT_GRID_DARK_TEXTURE_ASSET_GUID			 1035
#define DEFAULT_GRID_GREEN_TEXTURE_ASSET_GUID			 1036
#define DEFAULT_GRID_LIGHT_TEXTURE_ASSET_GUID			 1037
#define DEFAULT_GRID_ORANGE_TEXTURE_ASSET_GUID			 1038
#define DEFAULT_GRID_PURPLE_TEXTURE_ASSET_GUID			 1039
#define DEFAULT_GRID_RED_TEXTURE_ASSET_GUID				 1040
#define DEFAULT_SPRITE_LIT_SHADER_ASSET_GUID			 1041
#define DEFAULT_SPRITE_LIT_MATERIAL_ASSET_GUID			 1042
#define DEFAULT_SPRITE_UNLIT_SHADER_ASSET_GUID			 1043
#define DEFAULT_SPRITE_UNLIT_MATERIAL_ASSET_GUID		 1044
#define DEFAULT_PARTICLE_SHADER_ASSET_GUID				 1045
#define DEFAULT_PARTICLE_MATERIAL_ASSET_GUID			 1046
#define DEFAULT_GRID_MATERIAL_ASSET_GUID				 1048
#define DEFAULT_SPRITE_CIRCLE_ASSET_GUID				 1049
#define DEFAULT_POST_PROCESS_SHADER_ASSET_GUID			 1050
#define DEFAULT_POST_PROCESS_MATERIAL_ASSET_GUID		 1051
#define DEFAULT_GRID_DARK_MATERIAL_ASSET_GUID			 1052
#define DEFAULT_GRADIENT_DARK_SKYBOX_MATERIAL_ASSET_GUID 1053

	enum class editor_asset_source_type_e : u8
	{
		none,
		file,
		file_blob,
		embedded,
	};

	enum class editor_asset_status_e : u8
	{
		ok,
		missing_dependency,
		missing_embedded_data,
		missing_file_source,
	};

	struct editor_asset_t
	{
		static constexpr u32 VERSION = 0;

		string_t				   embedded_source = {};
		string_t				   cook_options	   = "{}";
		string_t				   source_relative = {};
		u32						   version		   = 0;
		sid_t					   guid			   = NULL_SID;
		sid_t					   thumbnail_guid  = NULL_SID;
		editor_asset_type_e		   asset_type	   = editor_asset_type_e::invalid;
		editor_asset_source_type_e source_type	   = editor_asset_source_type_e::file;
		editor_asset_status_e	   status		   = editor_asset_status_e::ok;
		u8						   sub_type		   = 0;
	};

	struct editor_asset_descriptor_t
	{
		vector_t<string_t>	extensions	 = {};
		string_t			display_name = {};
		vec4f_t				color		 = {};
		editor_asset_type_e asset_type	 = editor_asset_type_e::invalid;
	};

	void to_json(nlohmann::json& j, const editor_asset_source_type_e& t);
	void from_json(const nlohmann::json& j, editor_asset_source_type_e& t);
	void to_json(nlohmann::json& j, const editor_asset_t& asset);
	void from_json(const nlohmann::json& j, editor_asset_t& asset);
}
