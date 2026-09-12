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

#include "material_limits.hpp"
#include <sfg/data/inplace_vector.hpp>
#include <sfg/data/string.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
#define SFG_SHADER_MATERIAL_NAME_SIZE 64

	enum class resource_type_e : u8;

	enum class shader_texture_type_e : u8
	{
		invalid,
		texture2d,
		texture_cube,
		sprite,
	};

	enum class shader_param_type_e : u8
	{
		invalid,
		f32,
		vec2,
		vec4,
		u32,
	};

	enum class shader_param_hint_e : u8
	{
		none,
		color,
		color_hdr,
		pack_uint2,
		toggle,
	};

	struct shader_texture_definition_t
	{
		char				  texture_name[SFG_SHADER_MATERIAL_NAME_SIZE] = {};
		shader_texture_type_e type										  = shader_texture_type_e::invalid;
	};

	struct shader_sampler_definition_t
	{
		char sampler_name[SFG_SHADER_MATERIAL_NAME_SIZE] = {};
	};

	struct shader_param_definition_t
	{
		char param_name[SFG_SHADER_MATERIAL_NAME_SIZE] = {};
		union {
			f32 default_value[4] = {};
			u32 default_value_u32[4];
		};
		union {
			f32 min_value[4] = {};
			u32 min_value_u32[4];
		};
		union {
			f32 max_value[4] = {};
			u32 max_value_u32[4];
		};
		shader_param_type_e type = shader_param_type_e::invalid;
		shader_param_hint_e hint = shader_param_hint_e::none;
	};

	struct shader_data_definition_t
	{
		inplace_vector_t<shader_texture_definition_t, SFG_MATERIAL_MAX_TEXTURES> textures	= {};
		inplace_vector_t<shader_sampler_definition_t, SFG_MATERIAL_MAX_TEXTURES> samplers	= {};
		inplace_vector_t<shader_param_definition_t, SFG_MATERIAL_MAX_PARAMS>	 parameters = {};
	};

	const char*			  shader_texture_type_to_string(shader_texture_type_e type);
	resource_type_e		  shader_texture_type_to_resource_type(shader_texture_type_e type);
	const char*			  shader_param_type_to_string(shader_param_type_e type);
	const char*			  shader_param_hint_to_string(shader_param_hint_e hint);
	shader_texture_type_e shader_texture_type_from_string(const string_t& value);
	shader_param_type_e	  shader_param_type_from_string(const string_t& value);
	shader_param_hint_e	  shader_param_hint_from_string(const string_t& value);

	void to_json(nlohmann::json& j, const shader_texture_definition_t& definition);
	void from_json(const nlohmann::json& j, shader_texture_definition_t& definition);
	void to_json(nlohmann::json& j, const shader_sampler_definition_t& definition);
	void from_json(const nlohmann::json& j, shader_sampler_definition_t& definition);
	void to_json(nlohmann::json& j, const shader_param_definition_t& definition);
	void from_json(const nlohmann::json& j, shader_param_definition_t& definition);
	void to_json(nlohmann::json& j, const shader_data_definition_t& definition);
	void from_json(const nlohmann::json& j, shader_data_definition_t& definition);
}
