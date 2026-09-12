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

#include "script_api_resource.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/resources/material.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>

namespace sfg
{
	u8 api_resource_update_material_parameter_f32(resource_handle_t material, sid_t parameter_name, f32 value)
	{
		resource_manager_t& resource_manager = resource_manager_t::get();

		if (!resource_manager.is_material_parameter_valid(material, parameter_name, shader_param_type_e::f32))
			return 0;

		resource_manager.update_material_parameter(material, parameter_name, value);
		return 1;
	}

	u8 api_resource_update_material_parameter_vec2(resource_handle_t material, sid_t parameter_name, const vec2f_t* value)
	{
		SFG_ASSERT(value != nullptr);

		resource_manager_t& resource_manager = resource_manager_t::get();

		if (!resource_manager.is_material_parameter_valid(material, parameter_name, shader_param_type_e::vec2))
			return 0;

		resource_manager.update_material_parameter(material, parameter_name, *value);
		return 1;
	}

	u8 api_resource_update_material_parameter_vec4(resource_handle_t material, sid_t parameter_name, const vec4f_t* value)
	{
		SFG_ASSERT(value != nullptr);

		resource_manager_t& resource_manager = resource_manager_t::get();

		if (!resource_manager.is_material_parameter_valid(material, parameter_name, shader_param_type_e::vec4))
			return 0;

		resource_manager.update_material_parameter(material, parameter_name, *value);
		return 1;
	}

	u8 api_resource_update_material_parameter_u32(resource_handle_t material, sid_t parameter_name, u32 value)
	{
		resource_manager_t& resource_manager = resource_manager_t::get();

		if (!resource_manager.is_material_parameter_valid(material, parameter_name, shader_param_type_e::u32))
			return 0;

		resource_manager.update_material_parameter(material, parameter_name, value);
		return 1;
	}

	u8 api_resource_update_material_texture(resource_handle_t material, sid_t texture_name, resource_handle_t texture)
	{
		resource_manager_t& resource_manager = resource_manager_t::get();

		if (!resource_manager.is_material_texture_valid(material, texture_name, texture))
			return 0;

		resource_manager.update_material_texture(material, texture_name, texture);
		return 1;
	}

	u8 api_resource_update_material_sampler(resource_handle_t material, sid_t sampler_name, resource_handle_t sampler)
	{
		resource_manager_t& resource_manager = resource_manager_t::get();

		if (!resource_manager.is_material_sampler_valid(material, sampler_name, sampler))
			return 0;

		resource_manager.update_material_sampler(material, sampler_name, sampler);
		return 1;
	}

	const script_api_resource_t& get_script_api_resource()
	{
		static const script_api_resource_t api{
			.size							= static_cast<u32>(sizeof(script_api_resource_t)),
			.version						= 1,
			.update_material_parameter_f32	= api_resource_update_material_parameter_f32,
			.update_material_parameter_vec2 = api_resource_update_material_parameter_vec2,
			.update_material_parameter_vec4 = api_resource_update_material_parameter_vec4,
			.update_material_parameter_u32	= api_resource_update_material_parameter_u32,
			.update_material_texture		= api_resource_update_material_texture,
			.update_material_sampler		= api_resource_update_material_sampler,
		};

		return api;
	}
}
