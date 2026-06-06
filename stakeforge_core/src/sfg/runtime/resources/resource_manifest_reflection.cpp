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

#include "resource_manifest_reflection.hpp"
#include "common_resources_reflection.hpp"

#include <sfg/reflection/reflection_registry.hpp>

#include <cstddef>
#include <iterator>

namespace sfg
{
	resource_manifest_entry_reflection_t::resource_manifest_entry_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(TYPE_ID) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name = "name", .display_name = "Name", .type = reflected_value_type_e::string, .offset = offsetof(resource_manifest_entry_t, name), .size = sizeof(string_t)},
			{.name = "path", .display_name = "Path", .type = reflected_value_type_e::string, .offset = offsetof(resource_manifest_entry_t, path), .size = sizeof(string_t)},
			{.name = "type", .display_name = "Type", .type = reflected_value_type_e::enum8, .sub_type_id = resource_type_reflection_t::TYPE_ID, .offset = offsetof(resource_manifest_entry_t, type), .size = sizeof(resource_type_e)},
			{.name = "config", .display_name = "Config", .type = reflected_value_type_e::json, .offset = offsetof(resource_manifest_entry_t, config), .size = sizeof(nlohmann::json), .flags = reflected_field_flags_no_ui},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "resource_manifest_entry_t",
			.type_id   = TYPE_ID,
			.size	   = sizeof(resource_manifest_entry_t),
			.alignment = alignof(resource_manifest_entry_t),
		});
	}
}
