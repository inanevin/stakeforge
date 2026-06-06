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

		world_pass_flags_e world_pass_flag_from_string(const string_t& value)
		{
			if (value == "gbuffer" || value == "wpf_gbuffer")
				return wpf_gbuffer;
			if (value == "forward" || value == "wpf_forward")
				return wpf_forward;
			if (value == "depth" || value == "wpf_depth")
				return wpf_depth;
			if (value == "shadow" || value == "wpf_shadow" || value == "wfp_shadow")
				return wpf_shadow;
			return wpf_none;
		}
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

	void to_json(nlohmann::json& j, const world_pass_flags_e& f)
	{
		j				= nlohmann::json::array();
		const u32 flags = static_cast<u32>(f);
		if ((flags & wpf_gbuffer) != 0)
			j.push_back("gbuffer");
		if ((flags & wpf_forward) != 0)
			j.push_back("forward");
		if ((flags & wpf_depth) != 0)
			j.push_back("depth");
		if ((flags & wpf_shadow) != 0)
			j.push_back("shadow");
	}

	void from_json(const nlohmann::json& j, world_pass_flags_e& f)
	{
		u32 flags = wpf_none;
		if (j.is_number_unsigned())
		{
			flags = j.get<u32>();
		}
		else if (j.is_string())
		{
			flags = static_cast<u32>(world_pass_flag_from_string(j.get<string_t>()));
		}
		else if (j.is_array())
		{
			for (const nlohmann::json& item : j)
				flags |= static_cast<u32>(world_pass_flag_from_string(item.get<string_t>()));
		}
		f = static_cast<world_pass_flags_e>(flags);
	}
}
