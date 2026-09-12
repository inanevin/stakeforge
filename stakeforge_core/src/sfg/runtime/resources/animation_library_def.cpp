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

#include "animation_library_def.hpp"
#include "resource_type.hpp"

#include <sfg/reflection/reflection_container_ops.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	animation_library_def_reflection_t::animation_library_def_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "animation_library_blend_type_e",
			.display_name = "Blend type",
			.fields =
				{
					{
						.name		  = "no_blend",
						.display_name = "No blend",
					},
					{
						.name		  = "blend_1d",
						.display_name = "1D blend",
					},
					{
						.name		  = "blend_2d",
						.display_name = "2D blend",
					},
				},
			.type_id   = type_id_t<animation_library_blend_type_e>::value,
			.size	   = sizeof(animation_library_blend_type_e),
			.alignment = alignof(animation_library_blend_type_e),
			.flags	   = reflected_type_flag_enum,
		});

		registry.register_type({
			.name		  = "animation_library_clip_def_t",
			.display_name = "Animation clip",
			.fields =
				{
					{
						.name		  = "animation_clip",
						.display_name = "Animation clip",
						.sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_ANIMATION,
						.offset		  = offsetof(animation_library_clip_def_t, animation_clip),
						.size		  = sizeof(animation_library_clip_def_t::animation_clip),
						.type		  = reflected_value_type_e::u64,
					},
					{
						.name		  = "weight_value",
						.display_name = "Weight value",
						.sub_type_id  = type_id_t<vec2f_t>::value,
						.offset		  = offsetof(animation_library_clip_def_t, weight_value),
						.size		  = sizeof(animation_library_clip_def_t::weight_value),
						.type		  = reflected_value_type_e::object,
					},
				},
			.type_id   = type_id_t<animation_library_clip_def_t>::value,
			.size	   = sizeof(animation_library_clip_def_t),
			.alignment = alignof(animation_library_clip_def_t),
		});

		registry.register_type({
			.name		  = "animation_library_state_def_t",
			.display_name = "Animation state",
			.fields =
				{
					{
						.name		  = "name",
						.display_name = "Name",
						.offset		  = offsetof(animation_library_state_def_t, name),
						.size		  = sizeof(animation_library_state_def_t::name),
						.type		  = reflected_value_type_e::char_array,
					},
					{
						.container_ops =
							reflection_container_ops_t::sized_array_ops<animation_library_state_def_t, animation_library_clip_def_t, MAX_ANIMATION_LIBRARY_STATE_CLIPS, &animation_library_state_def_t::clips, &animation_library_state_def_t::clip_count>(
								reflected_value_type_e::object, type_id_t<animation_library_clip_def_t>::value),
						.name		  = "clips",
						.display_name = "Clips",
						.offset		  = 0,
						.size		  = sizeof(animation_library_state_def_t),
						.type		  = reflected_value_type_e::container,
					},
					{
						.name		  = "blend_value",
						.display_name = "Blend value",
						.sub_type_id  = type_id_t<vec2f_t>::value,
						.offset		  = offsetof(animation_library_state_def_t, blend_value),
						.size		  = sizeof(animation_library_state_def_t::blend_value),
						.type		  = reflected_value_type_e::object,
					},
					{
						.name		  = "blend_type",
						.display_name = "Blend type",
						.sub_type_id  = type_id_t<animation_library_blend_type_e>::value,
						.offset		  = offsetof(animation_library_state_def_t, blend_type),
						.size		  = sizeof(animation_library_state_def_t::blend_type),
						.type		  = reflected_value_type_e::u8,
					},
				},
			.type_id   = type_id_t<animation_library_state_def_t>::value,
			.size	   = sizeof(animation_library_state_def_t),
			.alignment = alignof(animation_library_state_def_t),
		});

		registry.register_type({
			.name		  = "animation_library_layer_def_t",
			.display_name = "Animation layer",
			.fields =
				{
					{
						.name		  = "name",
						.display_name = "Name",
						.offset		  = offsetof(animation_library_layer_def_t, name),
						.size		  = sizeof(animation_library_layer_def_t::name),
						.type		  = reflected_value_type_e::char_array,
					},
					{
						.name		  = "mask_name",
						.display_name = "Mask name",
						.offset		  = offsetof(animation_library_layer_def_t, mask_name),
						.size		  = sizeof(animation_library_layer_def_t::mask_name),
						.type		  = reflected_value_type_e::char_array,
					},
					{
						.container_ops = reflection_container_ops_t::vector_ops<animation_library_state_def_t>(reflected_value_type_e::object, type_id_t<animation_library_state_def_t>::value),
						.name		   = "states",
						.display_name  = "States",
						.offset		   = offsetof(animation_library_layer_def_t, states),
						.size		   = sizeof(animation_library_layer_def_t::states),
						.type		   = reflected_value_type_e::container,
					},
					{
						.name		  = "default_active_state",
						.display_name = "Default active state",
						.offset		  = offsetof(animation_library_layer_def_t, default_active_state),
						.size		  = sizeof(animation_library_layer_def_t::default_active_state),
						.type		  = reflected_value_type_e::u32,
					},
					{
						.name		  = "weight",
						.display_name = "Weight",
						.offset		  = offsetof(animation_library_layer_def_t, weight),
						.size		  = sizeof(animation_library_layer_def_t::weight),
						.type		  = reflected_value_type_e::f32,
					},
					{
						.name		  = "use_mask",
						.display_name = "Use mask",
						.offset		  = offsetof(animation_library_layer_def_t, use_mask),
						.size		  = sizeof(animation_library_layer_def_t::use_mask),
						.type		  = reflected_value_type_e::boolean,
					},
				},
			.type_id   = type_id_t<animation_library_layer_def_t>::value,
			.size	   = sizeof(animation_library_layer_def_t),
			.alignment = alignof(animation_library_layer_def_t),
		});

		registry.register_type({
			.name		  = "animation_library_def_t",
			.display_name = "Animation library",
			.fields =
				{
					{
						.name		  = "skeleton",
						.display_name = "Skeleton",
						.sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_SKELETON,
						.offset		  = offsetof(animation_library_def_t, skeleton),
						.size		  = sizeof(animation_library_def_t::skeleton),
						.type		  = reflected_value_type_e::u64,
					},
					{
						.container_ops = reflection_container_ops_t::vector_ops<animation_library_layer_def_t>(reflected_value_type_e::object, type_id_t<animation_library_layer_def_t>::value),
						.name		   = "layers",
						.display_name  = "Layers",
						.offset		   = offsetof(animation_library_def_t, layers),
						.size		   = sizeof(animation_library_def_t::layers),
						.type		   = reflected_value_type_e::container,
					},
				},
			.type_id   = type_id_t<animation_library_def_t>::value,
			.size	   = sizeof(animation_library_def_t),
			.alignment = alignof(animation_library_def_t),
		});
	}
}
