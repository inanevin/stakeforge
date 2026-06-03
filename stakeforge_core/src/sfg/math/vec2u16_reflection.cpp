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

#include "vec2u16_reflection.hpp"

#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>
#include <iterator>

namespace sfg
{
	namespace
	{
		bool get_vec2u16_x(const void* object, const reflected_field_desc_t&, void* out_value, void*)
		{
			*static_cast<u32*>(out_value) = static_cast<u32>(static_cast<const vec2u16_t*>(object)->x);
			return true;
		}

		bool set_vec2u16_x(void* object, const reflected_field_desc_t&, const void* value, void*)
		{
			static_cast<vec2u16_t*>(object)->x = static_cast<u16>(*static_cast<const u32*>(value));
			return true;
		}

		bool get_vec2u16_y(const void* object, const reflected_field_desc_t&, void* out_value, void*)
		{
			*static_cast<u32*>(out_value) = static_cast<u32>(static_cast<const vec2u16_t*>(object)->y);
			return true;
		}

		bool set_vec2u16_y(void* object, const reflected_field_desc_t&, const void* value, void*)
		{
			static_cast<vec2u16_t*>(object)->y = static_cast<u16>(*static_cast<const u32*>(value));
			return true;
		}
	}

	vec2u16_reflection_t::vec2u16_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.get = get_vec2u16_x, .set = set_vec2u16_x, .name = "x", .type = reflected_value_type_e::u32, .offset = offsetof(vec2u16_t, x), .size = sizeof(u16)},
			{.get = get_vec2u16_y, .set = set_vec2u16_y, .name = "y", .type = reflected_value_type_e::u32, .offset = offsetof(vec2u16_t, y), .size = sizeof(u16)},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "vec2u16_t",
			.type_id   = TYPE_ID,
			.size	   = sizeof(vec2u16_t),
			.alignment = alignof(vec2u16_t),
		});
	}
}
