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

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/common/type_id.hpp>

namespace sfg
{
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_AUDIO				   "reflection_resource_subtype_audio"_hs
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_FONT				   "reflection_resource_subtype_font"_hs
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_MESH				   "reflection_resource_subtype_mesh"_hs
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_SKELETON			   "reflection_resource_subtype_skeleton"_hs
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_ANIMATION			   "reflection_resource_subtype_animation"_hs
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_MATERIAL			   "reflection_resource_subtype_material"_hs
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_SHADER				   "reflection_resource_subtype_shader"_hs
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_TEXTURE				   "reflection_resource_subtype_texture"_hs
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_TEXTURE_SAMPLER		   "reflection_resource_subtype_texture_sampler"_hs
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_PHYSICAL_MATERIAL	   "reflection_resource_subtype_physical_material"_hs
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_PREFAB				   "reflection_resource_subtype_prefab"_hs
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_ANIMATION_GRAPH		   "reflection_resource_subtype_animation_graph"_hs
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_CUBEMAP				   "reflection_resource_subtype_cubemap"_hs
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_PHYSICS_COLLISION_MESH "reflection_resource_subtype_physics_collision_mesh"_hs
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_SPRITE				   "reflection_resource_subtype_sprite"_hs
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_CURVE				   "reflection_resource_subtype_curve"_hs
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_RAGDOLL				   "reflection_resource_subtype_ragdoll"_hs

#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_ANIMATION_LIBRARY "reflection_resource_subtype_animation_library"_hs

	enum class resource_type_e : u8
	{
		invalid,
		audio,
		font,
		mesh,
		skeleton,
		animation,
		material,
		shader,
		texture,
		texture_sampler,
		physical_material,
		prefab,
		animation_graph,
		cubemap,
		physics_collision_mesh,
		sprite,
		curve,
		ragdoll,
		animation_library,
		count,
	};

	inline constexpr u8 RESOURCE_TYPE_MAX = static_cast<u8>(resource_type_e::count);

	inline resource_type_e resource_type_from_reflection_sub_type_id(sid_t sub_type_id)
	{
		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_AUDIO)
			return resource_type_e::audio;

		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_FONT)
			return resource_type_e::font;

		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_MESH)
			return resource_type_e::mesh;

		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_SKELETON)
			return resource_type_e::skeleton;

		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_ANIMATION)
			return resource_type_e::animation;

		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_MATERIAL)
			return resource_type_e::material;

		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_SHADER)
			return resource_type_e::shader;

		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_TEXTURE)
			return resource_type_e::texture;

		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_TEXTURE_SAMPLER)
			return resource_type_e::texture_sampler;

		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_PHYSICAL_MATERIAL)
			return resource_type_e::physical_material;

		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_PREFAB)
			return resource_type_e::prefab;

		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_ANIMATION_GRAPH)
			return resource_type_e::animation_graph;

		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_CUBEMAP)
			return resource_type_e::cubemap;

		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_PHYSICS_COLLISION_MESH)
			return resource_type_e::physics_collision_mesh;

		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_SPRITE)
			return resource_type_e::sprite;

		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_CURVE)
			return resource_type_e::curve;

		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_RAGDOLL)
			return resource_type_e::ragdoll;

		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_ANIMATION_LIBRARY)
			return resource_type_e::animation_library;

		return resource_type_e::invalid;
	}

	SFG_DEFINE_TYPE_ID(resource_type_e);
}
