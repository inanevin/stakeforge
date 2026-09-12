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
			.name			 = "component_system_ragdoll",
			.display_name	 = "System Ragdoll",
			.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_system_ragdoll_t*>(ptr), component_system_ragdoll_t{}); },
			.type_id		 = type_id_t<component_system_ragdoll_t>::value,
			.size			 = sizeof(component_system_ragdoll_t),
			.alignment		 = alignof(component_system_ragdoll_t),
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
			.name			 = "component_system_animation_library",
			.display_name	 = "System Animation Library",
			.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_system_animation_library_t*>(ptr), component_system_animation_library_t{}); },
			.type_id		 = type_id_t<component_system_animation_library_t>::value,
			.size			 = sizeof(component_system_animation_library_t),
			.alignment		 = alignof(component_system_animation_library_t),
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
			.name			 = "component_system_canvas",
			.display_name	 = "System Canvas",
			.default_init_fn = [](void* ptr) { std::construct_at(static_cast<component_system_canvas_t*>(ptr), component_system_canvas_t{}); },
			.type_id		 = type_id_t<component_system_canvas_t>::value,
			.size			 = sizeof(component_system_canvas_t),
			.alignment		 = alignof(component_system_canvas_t),
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
