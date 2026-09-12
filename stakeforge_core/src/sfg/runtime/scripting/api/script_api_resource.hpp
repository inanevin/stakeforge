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

#include <sfg/common/size_definitions.hpp>
#include <sfg/runtime/resources/resource_handle.hpp>

namespace sfg
{
	struct vec2f_t;
	struct vec4f_t;

	u8 api_resource_update_material_parameter_f32(resource_handle_t material, sid_t parameter_name, f32 value);
	u8 api_resource_update_material_parameter_vec2(resource_handle_t material, sid_t parameter_name, const vec2f_t* value);
	u8 api_resource_update_material_parameter_vec4(resource_handle_t material, sid_t parameter_name, const vec4f_t* value);
	u8 api_resource_update_material_parameter_u32(resource_handle_t material, sid_t parameter_name, u32 value);
	u8 api_resource_update_material_texture(resource_handle_t material, sid_t texture_name, resource_handle_t texture);
	u8 api_resource_update_material_sampler(resource_handle_t material, sid_t sampler_name, resource_handle_t sampler);

	struct script_api_resource_t
	{
		u32													   size							  = 0;
		u32													   version						  = 0;
		decltype(&api_resource_update_material_parameter_f32)  update_material_parameter_f32  = nullptr;
		decltype(&api_resource_update_material_parameter_vec2) update_material_parameter_vec2 = nullptr;
		decltype(&api_resource_update_material_parameter_vec4) update_material_parameter_vec4 = nullptr;
		decltype(&api_resource_update_material_parameter_u32)  update_material_parameter_u32  = nullptr;
		decltype(&api_resource_update_material_texture)		   update_material_texture		  = nullptr;
		decltype(&api_resource_update_material_sampler)		   update_material_sampler		  = nullptr;
	};

	const script_api_resource_t& get_script_api_resource();
}
