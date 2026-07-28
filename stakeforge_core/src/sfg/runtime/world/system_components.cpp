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

#include "system_components.hpp"
#include <sfg/reflection/reflection_registry.hpp>

#include <memory>

namespace sfg
{
	system_component_reflection_t::system_component_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name			 = "component_system_transform",
			.display_name	 = "System Transform",
			.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_system_transform_t*>(ptr), component_system_transform_t{}); },
			.type_id		 = type_id_t<component_system_transform_t>::value,
			.size			 = sizeof(component_system_transform_t),
			.alignment		 = alignof(component_system_transform_t),
			.flags			 = reflected_type_flag_system_component | reflected_type_flag_no_ui | reflected_type_flag_no_serialization,
		});

		registry.register_type({
			.name			 = "component_system_skinned_mesh_renderer",
			.display_name	 = "System Skinned Mesh Renderer",
			.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_system_skinned_mesh_renderer_t*>(ptr), component_system_skinned_mesh_renderer_t{}); },
			.type_id		 = type_id_t<component_system_skinned_mesh_renderer_t>::value,
			.size			 = sizeof(component_system_skinned_mesh_renderer_t),
			.alignment		 = alignof(component_system_skinned_mesh_renderer_t),
			.flags			 = reflected_type_flag_system_component | reflected_type_flag_no_ui | reflected_type_flag_no_serialization,
		});

		registry.register_type({
			.name			 = "component_system_sprite_renderer",
			.display_name	 = "System Sprite Renderer",
			.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_system_sprite_renderer_t*>(ptr), component_system_sprite_renderer_t{}); },
			.type_id		 = type_id_t<component_system_sprite_renderer_t>::value,
			.size			 = sizeof(component_system_sprite_renderer_t),
			.alignment		 = alignof(component_system_sprite_renderer_t),
			.flags			 = reflected_type_flag_system_component | reflected_type_flag_no_ui | reflected_type_flag_no_serialization,
		});

		registry.register_type({
			.name			 = "component_system_animation_player",
			.display_name	 = "System Animation Player",
			.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_system_animation_player_t*>(ptr), component_system_animation_player_t{}); },
			.type_id		 = type_id_t<component_system_animation_player_t>::value,
			.size			 = sizeof(component_system_animation_player_t),
			.alignment		 = alignof(component_system_animation_player_t),
			.flags			 = reflected_type_flag_system_component | reflected_type_flag_no_ui | reflected_type_flag_no_serialization,
		});

		registry.register_type({
			.name			 = "component_system_animation_graph",
			.display_name	 = "System Animation Graph",
			.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_system_animation_graph_t*>(ptr), component_system_animation_graph_t{}); },
			.type_id		 = type_id_t<component_system_animation_graph_t>::value,
			.size			 = sizeof(component_system_animation_graph_t),
			.alignment		 = alignof(component_system_animation_graph_t),
			.flags			 = reflected_type_flag_system_component | reflected_type_flag_no_ui | reflected_type_flag_no_serialization,
		});

		registry.register_type({
			.name			 = "component_system_audio_source",
			.display_name	 = "System Audio Source",
			.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_system_audio_source_t*>(ptr), component_system_audio_source_t{}); },
			.type_id		 = type_id_t<component_system_audio_source_t>::value,
			.size			 = sizeof(component_system_audio_source_t),
			.alignment		 = alignof(component_system_audio_source_t),
			.flags			 = reflected_type_flag_system_component | reflected_type_flag_no_ui | reflected_type_flag_no_serialization,
		});

		registry.register_type({
			.name			 = "component_system_physics",
			.display_name	 = "System Physics",
			.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_system_physics_t*>(ptr), component_system_physics_t{}); },
			.type_id		 = type_id_t<component_system_physics_t>::value,
			.size			 = sizeof(component_system_physics_t),
			.alignment		 = alignof(component_system_physics_t),
			.flags			 = reflected_type_flag_system_component | reflected_type_flag_no_ui | reflected_type_flag_no_serialization,
		});

		registry.register_type({
			.name			 = "component_system_constraints",
			.display_name	 = "System Constraints",
			.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_system_constraints_t*>(ptr), component_system_constraints_t{}); },
			.type_id		 = type_id_t<component_system_constraints_t>::value,
			.size			 = sizeof(component_system_constraints_t),
			.alignment		 = alignof(component_system_constraints_t),
			.flags			 = reflected_type_flag_system_component | reflected_type_flag_no_ui | reflected_type_flag_no_serialization,
		});

		registry.register_type({
			.name			 = "component_system_destroyer",
			.display_name	 = "System Destroyer",
			.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_system_destroyer_t*>(ptr), component_system_destroyer_t{}); },
			.type_id		 = type_id_t<component_system_destroyer_t>::value,
			.size			 = sizeof(component_system_destroyer_t),
			.alignment		 = alignof(component_system_destroyer_t),
			.flags			 = reflected_type_flag_system_component | reflected_type_flag_no_ui | reflected_type_flag_no_serialization,
		});
	}
}
