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

#include "texture_cook_reflection.hpp"

#include <sfg/math/vec2u.hpp>
#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>
#include <iterator>

namespace sfg
{
	namespace
	{
		static const reflected_enum_value_desc_t texture_payload_type_values[] = {
			{.name = "uncompressed", .display_name = "Uncompressed", .value = static_cast<i64>(texture_payload_type_e::uncompressed)},
			{.name = "ktx2_uastc", .display_name = "KTX2 UASTC", .value = static_cast<i64>(texture_payload_type_e::ktx2_uastc)},
		};

		bool get_texture_cook_size(const void* object, const reflected_field_desc_t&, void* out_value, void*)
		{
			const vec2u16_t& size			  = static_cast<const texture_cook_config_t*>(object)->size;
			*static_cast<vec2u_t*>(out_value) = {size.x, size.y};
			return true;
		}

		bool set_texture_cook_size(void* object, const reflected_field_desc_t&, const void* value, void*)
		{
			const vec2u_t& size								  = *static_cast<const vec2u_t*>(value);
			static_cast<texture_cook_config_t*>(object)->size = {static_cast<u16>(size.x), static_cast<u16>(size.y)};
			return true;
		}
	}

	texture_payload_type_reflection_t::texture_payload_type_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		registry.register_type({
			.enum_values = {.data = texture_payload_type_values, .size = std::size(texture_payload_type_values)},
			.name		 = "texture_payload_type_e",
			.type_id	 = TYPE_ID,
			.size		 = sizeof(texture_payload_type_e),
			.alignment	 = alignof(texture_payload_type_e),
		});
	}

	texture_cook_config_reflection_t::texture_cook_config_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.get		   = get_texture_cook_size,
			 .set		   = set_texture_cook_size,
			 .name		   = "size",
			 .display_name = "Size",
			 .type		   = reflected_value_type_e::vec2u,
			 .offset	   = offsetof(texture_cook_config_t, size),
			 .size		   = sizeof(vec2u16_t),
			 .flags		   = reflected_field_flags_no_ui},
			{.name			= "payload_type",
			 .display_name	= "Payload Type",
			 .type			= reflected_value_type_e::enum8,
			 .value_type_id = texture_payload_type_reflection_t::TYPE_ID,
			 .offset		= offsetof(texture_cook_config_t, payload_type),
			 .size			= sizeof(texture_payload_type_e)},
			{.name = "generate_mipmaps", .display_name = "Generate Mipmaps", .type = reflected_value_type_e::bool8, .offset = offsetof(texture_cook_config_t, generate_mipmaps), .size = sizeof(bool)},
			{.name = "is_linear", .display_name = "Linear", .type = reflected_value_type_e::bool8, .offset = offsetof(texture_cook_config_t, is_linear), .size = sizeof(bool)},
		};

		registry.register_type({
			.fields		  = {.data = fields, .size = std::size(fields)},
			.name		  = "texture_cook_config_t",
			.display_name = "Texture Cook Config",
			.type_id	  = TYPE_ID,
			.size		  = sizeof(texture_cook_config_t),
			.alignment	  = alignof(texture_cook_config_t),
		});
	}
}
