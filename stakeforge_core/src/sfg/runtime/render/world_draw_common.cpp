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

#include "world_draw_common.hpp"
#include <iterator>
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
}

namespace sfg
{
	namespace
	{
		static const reflected_enum_value_desc_t world_pass_flags_values[] = {
			{.name = "none", .display_name = "None", .value = wpf_none},
			{.name = "gbuffer", .display_name = "GBuffer", .value = wpf_gbuffer},
			{.name = "forward", .display_name = "Forward", .value = wpf_forward},
			{.name = "depth", .display_name = "Depth", .value = wpf_depth},
			{.name = "shadow", .display_name = "Shadow", .value = wpf_shadow},
		};
	}

	world_pass_flags_reflection_t::world_pass_flags_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<world_pass_flags_e>::value) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = world_pass_flags_values, .size = std::size(world_pass_flags_values)},
			.name		 = "world_pass_flags_e",
			.type_id	 = type_id_t<world_pass_flags_e>::value,
			.size		 = sizeof(world_pass_flags_e),
			.alignment	 = alignof(world_pass_flags_e),
		});
	}

}
