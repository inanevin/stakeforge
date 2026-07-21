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
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_HDR_SKYBOX			   "reflection_resource_subtype_hdr_skybox"_hs
#define SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_PHYSICS_COLLISION_MESH "reflection_resource_subtype_physics_collision_mesh"_hs

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
		hdr_skybox,
		physics_collision_mesh,
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
		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_HDR_SKYBOX)
			return resource_type_e::hdr_skybox;
		if (sub_type_id == SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_PHYSICS_COLLISION_MESH)
			return resource_type_e::physics_collision_mesh;
		return resource_type_e::invalid;
	}

	SFG_DEFINE_TYPE_ID(resource_type_e);
}
