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
