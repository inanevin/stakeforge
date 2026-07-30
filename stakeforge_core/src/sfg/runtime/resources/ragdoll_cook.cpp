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

#include "ragdoll_cook.hpp"

#include "ragdoll.hpp"
#include "ragdoll_def.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	bool ragdoll_cooker::cook_from_def(const ragdoll_def_t& def, resource_header_t& out_header, ostream_t& stream)
	{
		if (def.target_skeleton == NULL_RESOURCE_HANDLE)
		{
			SFG_ERR("ragdoll requires a target skeleton");
			return false;
		}

		if (def.parts.empty() || def.parts.size() > RAGDOLL_PART_MAX)
		{
			SFG_ERR("ragdoll part count must be between 1 and {0}", RAGDOLL_PART_MAX);
			return false;
		}

		for (u32 i = 0; i < def.parts.size(); ++i)
		{
			const ragdoll_part_def_t& part = def.parts[i];

			if (part.joint_index == UINT32_MAX || part.radius <= 0.0f || part.half_height <= 0.0f || part.mass <= 0.0f)
			{
				SFG_ERR("ragdoll part {0} has invalid joint or capsule values", i);
				return false;
			}

			if ((i == 0 && part.parent_part_index != RAGDOLL_PART_NO_PARENT) || (i != 0 && part.parent_part_index == RAGDOLL_PART_NO_PARENT))
			{
				SFG_ERR("ragdoll must have exactly one root in the first part");
				return false;
			}

			if (part.parent_part_index != RAGDOLL_PART_NO_PARENT && part.parent_part_index >= i)
			{
				SFG_ERR("ragdoll part {0} must reference an earlier parent part", i);
				return false;
			}

			if (part.twist_axis.is_zero() || part.plane_axis.is_zero() || math::abs(vec3f_t::dot(part.twist_axis.normalized(), part.plane_axis.normalized())) > 0.999f || part.normal_half_cone_angle_degrees < 0.0f ||
				part.normal_half_cone_angle_degrees > 180.0f || part.plane_half_cone_angle_degrees < 0.0f || part.plane_half_cone_angle_degrees > 180.0f || part.twist_min_angle_degrees < -180.0f || part.twist_max_angle_degrees > 180.0f ||
				part.twist_min_angle_degrees > part.twist_max_angle_degrees || part.max_friction_torque < 0.0f)
			{
				SFG_ERR("ragdoll part {0} has invalid constraint values", i);
				return false;
			}

			for (u32 other = 0; other < i; ++other)
			{
				if (def.parts[other].joint_index == part.joint_index)
				{
					SFG_ERR("ragdoll joint {0} is used by more than one part", part.joint_index);
					return false;
				}
			}
		}

		if (def.linear_damping < 0.0f || def.linear_damping > 1.0f || def.angular_damping < 0.0f || def.angular_damping > 1.0f)
		{
			SFG_ERR("ragdoll damping values must be between 0 and 1");
			return false;
		}

		ostream_t def_stream = {};

		if (!reflection_registry_t::get().type_to_stream(type_id_t<ragdoll_def_t>::value, const_cast<ragdoll_def_t*>(&def), nullptr, def_stream))
		{
			SFG_ERR("failed to serialize ragdoll definition");
			return false;
		}

		out_header = {
			.type		 = resource_type_e::ragdoll,
			.magic		 = ragdoll_loader_t::WIRE_MAGIC,
			.version	 = ragdoll_loader_t::WIRE_VERSION,
			.source_tick = hashing_t::hash_u64(def_stream.get_raw(), def_stream.get_size()),
		};

		const resource_dependency_t skeleton_dependency{
			.handle = def.target_skeleton,
			.type	= resource_type_e::skeleton,
		};

		stream << skeleton_dependency;
		out_header.dependency_count = 1;

		if (def.physical_material != NULL_RESOURCE_HANDLE)
		{
			const resource_dependency_t material_dependency{
				.handle = def.physical_material,
				.type	= resource_type_e::physical_material,
			};

			stream << material_dependency;
			out_header.dependency_count++;
		}

		stream.write_raw(def_stream.get_raw(), def_stream.get_size());
		return true;
	}
}
