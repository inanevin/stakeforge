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

#include "common_resources_reflection.hpp"

#include <sfg/reflection/reflection_registry.hpp>

#include <iterator>

namespace sfg
{
	namespace
	{
		static const reflected_enum_value_desc_t resource_type_values[] = {
			{.name = "invalid", .display_name = "Invalid", .value = static_cast<i64>(resource_type_e::invalid)},
			{.name = "audio", .display_name = "Audio", .value = static_cast<i64>(resource_type_e::audio)},
			{.name = "font", .display_name = "Font", .value = static_cast<i64>(resource_type_e::font)},
			{.name = "mesh", .display_name = "Mesh", .value = static_cast<i64>(resource_type_e::mesh)},
			{.name = "skeleton", .display_name = "Skeleton", .value = static_cast<i64>(resource_type_e::skeleton)},
			{.name = "animation", .display_name = "Animation", .value = static_cast<i64>(resource_type_e::animation)},
			{.name = "material", .display_name = "Material", .value = static_cast<i64>(resource_type_e::material)},
			{.name = "shader", .display_name = "Shader", .value = static_cast<i64>(resource_type_e::shader)},
			{.name = "texture", .display_name = "Texture", .value = static_cast<i64>(resource_type_e::texture)},
			{.name = "texture_sampler", .display_name = "Texture Sampler", .value = static_cast<i64>(resource_type_e::texture_sampler)},
			{.name = "physical_material", .display_name = "Physical Material", .value = static_cast<i64>(resource_type_e::physical_material)},
			{.name = "prefab", .display_name = "Prefab", .value = static_cast<i64>(resource_type_e::prefab)},
			{.name = "animation_state_machine", .display_name = "Animation State Machine", .value = static_cast<i64>(resource_type_e::animation_state_machine)},
		};
	}

	resource_type_reflection_t::resource_type_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = resource_type_values, .size = std::size(resource_type_values)},
			.name		 = "resource_type_e",
			.type_id	 = TYPE_ID,
			.size		 = sizeof(resource_type_e),
			.alignment	 = alignof(resource_type_e),
		});
	}
}
