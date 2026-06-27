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

#include "physical_material_def.hpp"
#include <iterator>
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{

}

namespace sfg
{
	physical_material_reflection_t::physical_material_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<physical_material_def_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{
				.name		  = "restitution",
				.display_name = "Restitution",
				.type		  = reflected_value_type_e::f32,
				.offset		  = offsetof(physical_material_def_t, restitution),
				.size		  = sizeof(f32),
			},
			{
				.name		  = "friction",
				.display_name = "Friction",
				.type		  = reflected_value_type_e::f32,
				.offset		  = offsetof(physical_material_def_t, friction),
				.size		  = sizeof(f32),
			},
			{
				.name		  = "angular_damping",
				.display_name = "Angular Damping",
				.type		  = reflected_value_type_e::f32,
				.offset		  = offsetof(physical_material_def_t, angular_damping),
				.size		  = sizeof(f32),
			},
			{
				.name		  = "linear_damping",
				.display_name = "Linear Damping",
				.type		  = reflected_value_type_e::f32,
				.offset		  = offsetof(physical_material_def_t, linear_damping),
				.size		  = sizeof(f32),
			},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "physical_material_def_t",
			.type_id   = type_id_t<physical_material_def_t>::value,
			.size	   = sizeof(physical_material_def_t),
			.alignment = alignof(physical_material_def_t),
		});
	}
}
