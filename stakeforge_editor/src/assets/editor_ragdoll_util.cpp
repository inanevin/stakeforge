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

#include "editor_ragdoll_util.hpp"
#include <sfg/data/string_util.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/resources/ragdoll_def.hpp>
#include <sfg/runtime/resources/skeleton_def.hpp>

namespace sfg
{
	bool editor_ragdoll_util_t::generate(const skeleton_def_t& skeleton, ragdoll_def_t& ragdoll)
	{
		if (skeleton.joints.empty() || !skeleton.is_evaluation_order_valid())
		{
			SFG_ERR("cannot generate a ragdoll from an empty or invalid skeleton");
			return false;
		}

		const u32		   count = static_cast<u32>(skeleton.joints.size());
		vector_t<mat4x3_t> globals(count, mat4x3_t::identity);
		vector_t<string_t> names(count);
		vector_t<u32>	   mapping(count, UINT32_MAX);
		u32				   root		= skeleton.root_joint_index;
		bool			   humanoid = false;

		for (const u32 joint : skeleton.evaluation_order)
		{
			const skeleton_joint_def_t& source = skeleton.joints[joint];
			globals[joint]					   = source.parent_index == SKELETON_JOINT_NO_PARENT ? source.local : globals[source.parent_index] * source.local;
			names[joint]					   = source.name;
			string_util::to_lower(names[joint]);

			if (names[joint].find("pelvis") != string_t::npos || names[joint].find("hips") != string_t::npos)
			{
				root	 = joint;
				humanoid = true;
			}
		}

		for (mat4x3_t& global : globals)
			global = skeleton.skinning_transform * global;

		vec3f_t bounds_min = globals[root].get_translation();
		vec3f_t bounds_max = bounds_min;

		for (const mat4x3_t& global : globals)
		{
			bounds_min = vec3f_t::min(bounds_min, global.get_translation());
			bounds_max = vec3f_t::max(bounds_max, global.get_translation());
		}

		const f32					 extent = math::max((bounds_max - bounds_min).magnitude(), 0.01f);
		vector_t<ragdoll_part_def_t> parts	= {};
		parts.reserve(RAGDOLL_PART_MAX);
		f32 total_volume = 0.0f;

		for (const u32 joint : skeleton.evaluation_order)
		{
			const string_t& name	 = names[joint];
			const bool		torso	 = name.find("spine") != string_t::npos || name.find("chest") != string_t::npos || joint == root;
			const bool		head	 = name.find("head") != string_t::npos;
			const bool		neck	 = name.find("neck") != string_t::npos;
			const bool		arm		 = name.find("arm") != string_t::npos;
			const bool		leg		 = name.find("leg") != string_t::npos || name.find("thigh") != string_t::npos || name.find("calf") != string_t::npos;
			const bool		hand	 = name.find("hand") != string_t::npos;
			const bool		foot	 = name.find("foot") != string_t::npos;
			const bool		shoulder = name.find("shoulder") != string_t::npos || name.find("clavicle") != string_t::npos;
			bool			skip	 = false;

			for (const char* token : {"twist", "helper", "ik", "target", "finger", "thumb", "index", "middle", "ring", "pinky", "toe", "end", "nub"})
				skip |= name.find(token) != string_t::npos;

			if (joint != root && (skip || (humanoid && !torso && !head && !neck && !arm && !leg && !hand && !foot && !shoulder)))
				continue;

			u32 ancestor = skeleton.joints[joint].parent_index;

			while (ancestor != SKELETON_JOINT_NO_PARENT && mapping[ancestor] == UINT32_MAX)
				ancestor = skeleton.joints[ancestor].parent_index;

			if (joint != root && ancestor == SKELETON_JOINT_NO_PARENT)
				continue;

			const vec3f_t origin	  = globals[joint].get_translation();
			vec3f_t		  endpoint	  = origin;
			f32			  best_length = 0.0f;

			for (u32 child = 0; child < count; ++child)
			{
				if (skeleton.joints[child].parent_index != joint)
					continue;

				const vec3f_t candidate	  = globals[child].get_translation();
				const f32	  length	  = (candidate - origin).magnitude();
				const bool	  torso_child = torso && (names[child].find("spine") != string_t::npos || names[child].find("chest") != string_t::npos || names[child].find("neck") != string_t::npos);

				if (length > best_length)
				{
					endpoint	= candidate;
					best_length = length;
				}

				if (torso_child && length > extent * 0.0001f)
				{
					endpoint	= candidate;
					best_length = length;
					break;
				}
			}

			if (best_length < extent * 0.0001f)
			{
				if (!head && !hand && !foot && joint != root)
					continue;

				const u32	  parent	= skeleton.joints[joint].parent_index;
				const vec3f_t incoming	= parent == SKELETON_JOINT_NO_PARENT ? vec3f_t::up : origin - globals[parent].get_translation();
				const vec3f_t direction = incoming.is_zero() ? vec3f_t::up : incoming.normalized();
				best_length				= extent * (head ? 0.09f : 0.045f);
				endpoint				= origin + direction * best_length;
			}

			if (parts.size() == RAGDOLL_PART_MAX)
			{
				SFG_ERR("generated ragdoll exceeds {0} parts; simplify the skeleton before generating", RAGDOLL_PART_MAX);
				return false;
			}

			vec3f_t position = vec3f_t::zero;
			quat_t	rotation = quat_t::identity;
			vec3f_t scale	 = vec3f_t::one;
			globals[joint].decompose(position, rotation, scale);

			const f32 shape_scale = math::max(math::abs(scale.x), math::max(math::abs(scale.y), math::abs(scale.z)));

			if (shape_scale < 0.00001f)
			{
				SFG_ERR("cannot generate a ragdoll for a joint with zero scale");
				return false;
			}

			const vec3f_t direction		   = rotation.inverse() * ((endpoint - origin) / best_length);
			const f32	  dot			   = math::clamp(vec3f_t::dot(vec3f_t::up, direction), -1.0f, 1.0f);
			const vec3f_t axis			   = vec3f_t::cross(vec3f_t::up, direction);
			const quat_t  capsule_rotation = axis.is_zero() ? (dot < 0.0f ? quat_t::angle_axis(180.0f, vec3f_t::right) : quat_t::identity) : quat_t::angle_axis(math::radians_to_degrees(std::acos(dot)), axis.normalized());
			const f32	  length		   = best_length / shape_scale;
			const f32	  radius		   = math::max(length * (torso ? 0.38f : head ? 0.4f : 0.18f), extent * 0.006f / shape_scale);
			const f32	  half_height	   = math::max(length * 0.5f - radius, length * 0.08f);
			const bool	  hinge			   = name.find("forearm") != string_t::npos || name.find("lowerarm") != string_t::npos || name.find("calf") != string_t::npos || name.find("lowerleg") != string_t::npos;
			const f32	  volume		   = radius * radius * (half_height * 2.0f + radius * 4.0f / 3.0f) * shape_scale * shape_scale * shape_scale;

			mapping[joint] = static_cast<u32>(parts.size());
			parts.push_back({
				.local_position					= globals[joint].inverse() * ((origin + endpoint) * 0.5f),
				.local_rotation					= capsule_rotation,
				.twist_axis						= capsule_rotation.get_up(),
				.plane_axis						= capsule_rotation.get_right(),
				.radius							= radius,
				.half_height					= half_height,
				.mass							= volume,
				.normal_half_cone_angle_degrees = hinge	  ? 70.0f
												  : torso ? 20.0f
														  : 45.0f,
				.plane_half_cone_angle_degrees	= hinge	  ? 5.0f
												  : torso ? 20.0f
														  : 45.0f,
				.twist_min_angle_degrees		= hinge ? -5.0f : -25.0f,
				.twist_max_angle_degrees		= hinge ? 5.0f : 25.0f,
				.joint_index					= joint,
				.parent_part_index				= joint == root ? RAGDOLL_PART_NO_PARENT : mapping[ancestor],
			});
			total_volume += volume;
		}

		for (ragdoll_part_def_t& part : parts)
			part.mass = math::max(0.1f, 70.0f * part.mass / total_volume);

		ragdoll.parts = std::move(parts);

		return ragdoll.is_valid(count);
	}
}
