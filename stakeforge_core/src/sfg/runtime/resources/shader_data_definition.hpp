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

#include "material_limits.hpp"
#include <sfg/data/inplace_vector.hpp>
#include <sfg/data/string.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
#define SFG_SHADER_MATERIAL_NAME_SIZE 64

	enum class shader_texture_type_e : u8
	{
		invalid,
		texture2d,
	};

	enum class shader_param_type_e : u8
	{
		invalid,
		f32,
		vec2,
		vec4,
	};

	enum class shader_param_hint_e : u8
	{
		none,
		color,
		pack_uint2,
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
		char				param_name[SFG_SHADER_MATERIAL_NAME_SIZE] = {};
		f32					default_value[4]						  = {};
		f32					min_value[4]							  = {};
		f32					max_value[4]							  = {};
		shader_param_type_e type									  = shader_param_type_e::invalid;
		shader_param_hint_e hint									  = shader_param_hint_e::none;
	};

	struct shader_data_definition_t
	{
		inplace_vector_t<shader_texture_definition_t, SFG_MATERIAL_MAX_TEXTURES> textures	= {};
		inplace_vector_t<shader_sampler_definition_t, SFG_MATERIAL_MAX_TEXTURES> samplers	= {};
		inplace_vector_t<shader_param_definition_t, SFG_MATERIAL_MAX_PARAMS>	 parameters = {};
	};

	const char*			  shader_texture_type_to_string(shader_texture_type_e type);
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
