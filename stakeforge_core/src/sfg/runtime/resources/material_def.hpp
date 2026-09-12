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
#include "shader_data_definition.hpp"
#include <sfg/common/type_id.hpp>

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/inplace_vector.hpp>
#include <sfg/data/string.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>
#include <sfg/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	enum class material_blend_mode_e : u8
	{
		opaque,
		alpha,
		premultiplied_alpha,
		additive,
	};

	SFG_DEFINE_TYPE_ID(material_blend_mode_e);

	class istream_t;
	class ostream_t;

	struct material_texture_value_t
	{
		string_t			  name		= {};
		resource_handle_t	  texture	= NULL_RESOURCE_HANDLE;
		sid_t				  name_hash = NULL_SID;
		shader_texture_type_e type		= shader_texture_type_e::texture2d;
	};

	struct material_sampler_value_t
	{
		string_t		  name		= {};
		resource_handle_t sampler	= NULL_RESOURCE_HANDLE;
		sid_t			  name_hash = NULL_SID;
	};

	struct material_param_value_t
	{
		string_t name	   = {};
		sid_t	 name_hash = NULL_SID;
		union {
			f32 value[4] = {};
			u32 value_u32[4];
		};
		shader_param_type_e type = shader_param_type_e::invalid;
		shader_param_hint_e hint = shader_param_hint_e::none;
	};

	struct material_def_t
	{
		inplace_vector_t<material_texture_value_t, SFG_MATERIAL_MAX_TEXTURES> textures			= {};
		inplace_vector_t<material_sampler_value_t, SFG_MATERIAL_MAX_TEXTURES> samplers			= {};
		inplace_vector_t<material_param_value_t, SFG_MATERIAL_MAX_PARAMS>	  parameters		= {};
		resource_handle_t													  shader			= NULL_RESOURCE_HANDLE;
		material_blend_mode_e												  blend_mode		= material_blend_mode_e::opaque;
		bool																  write_shadows		= false;
		bool																  write_reflections = false;
		bool																  double_sided		= false;
		bool																  use_alpha_cutoff	= false;

		void serialize(ostream_t& stream) const;
		void deserialize(istream_t& stream);
	};

	SFG_DEFINE_TYPE_ID(material_def_t);

	material_def_t material_def_from_shader_def(const shader_data_definition_t& shader_def, resource_handle_t shader);

	void to_json(nlohmann::json& j, const material_texture_value_t& value);
	void from_json(const nlohmann::json& j, material_texture_value_t& value);
	void to_json(nlohmann::json& j, const material_sampler_value_t& value);
	void from_json(const nlohmann::json& j, material_sampler_value_t& value);
	void to_json(nlohmann::json& j, const material_param_value_t& value);
	void from_json(const nlohmann::json& j, material_param_value_t& value);
	void to_json(nlohmann::json& j, material_blend_mode_e value);
	void from_json(const nlohmann::json& j, material_blend_mode_e& value);
	void to_json(nlohmann::json& j, const material_def_t& value);
	void from_json(const nlohmann::json& j, material_def_t& value);
}
