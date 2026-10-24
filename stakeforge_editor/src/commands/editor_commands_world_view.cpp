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

#include "commands/editor_commands_world_view.hpp"
#include "commands/editor_command_component_edit.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"

#include <sfg/data/frame_vector.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/engine_components.hpp>

namespace sfg
{
	bool editor_commands_world_view_t::snap_selection_to_camera(editor_world_handle_t handle)
	{
		editor_world_t&				 editor_world = *editor_world_controller_t::get().get_editor_world(handle);
		editor_world_edit_context_t& context	  = editor_world.get_edit_context();
		world_t&					 world		  = editor_world.get_world();

		if (context.get_play_mode() != editor_play_mode_e::none || context.get_edit_type() != editor_world_edit_type_e::full_control)
			return false;

		editor_world.get_input_controller().deactivate();

		frame_vector_t<entity_id_t> entities = {};
		entities.resize(context.get_selected_entities().size);
		entities.resize(context.collect_selected_mutable_root_entities(world, {.data = entities.data(), .size = entities.size()}));

		if (entities.empty())
			return false;

		world.update_world_transforms(false);

		const entity_id_t camera		  = context.get_editor_camera_entity();
		const vec3f_t	  camera_position = world.get_entity_pos_last_abs(camera);
		const quat_t	  camera_rotation = world.get_entity_rot_last_abs(camera);

		frame_vector_t<component_transform_t> previous		   = {};
		frame_vector_t<component_transform_t> next			   = {};
		frame_vector_t<ostream_t>			  previous_streams = {};
		frame_vector_t<ostream_t>			  next_streams	   = {};
		previous.reserve(entities.size());
		next.reserve(entities.size());
		previous_streams.reserve(entities.size());
		next_streams.reserve(entities.size());

		for (entity_id_t entity : entities)
		{
			previous.push_back({.pos = world.get_entity_pos_local(entity), .rot = world.get_entity_rot_local(entity), .scale = world.get_entity_scale_local(entity)});

			component_transform_t transform{.pos = camera_position, .rot = camera_rotation, .scale = previous.back().scale};
			const entity_id_t	  parent = world.get_entity_parent(entity);

			if (parent != NULL_ENTITY_ID)
			{
				const mat4x3_t parent_matrix = world.calculate_entity_parent_abs_matrix(entity);
				transform.pos				 = world_t::abs_pos_to_local(camera_position, parent_matrix);
				transform.rot				 = world_t::abs_rot_to_local(camera_rotation, world.get_entity_rot_last_abs(parent));
			}

			next.push_back(transform);

			ostream_t before_stream = {};
			ostream_t after_stream	= {};

			if (!reflection_registry_t::get().type_to_stream(type_id_t<component_transform_t>::value, &previous.back(), nullptr, before_stream) ||
				!reflection_registry_t::get().type_to_stream(type_id_t<component_transform_t>::value, &next.back(), nullptr, after_stream))
			{
				SFG_ERR("failed to serialize camera alignment transforms");
				return false;
			}

			previous_streams.push_back(std::move(before_stream));
			next_streams.push_back(std::move(after_stream));
		}

		for (size_t i = 0; i < entities.size(); ++i)
		{
			world.set_entity_pos_local(entities[i], next[i].pos);
			world.set_entity_rot_local(entities[i], next[i].rot);
			world.set_entity_scale_local(entities[i], next[i].scale);
			world.mark_entity_teleported(entities[i]);
		}

		if (!editor_command_component_edit_t::edit(
				handle, {.data = entities.data(), .size = entities.size()}, type_id_t<component_transform_t>::value, {.data = previous_streams.data(), .size = previous_streams.size()}, {.data = next_streams.data(), .size = next_streams.size()}))
		{
			for (size_t i = 0; i < entities.size(); ++i)
			{
				world.set_entity_pos_local(entities[i], previous[i].pos);
				world.set_entity_rot_local(entities[i], previous[i].rot);
				world.set_entity_scale_local(entities[i], previous[i].scale);
				world.mark_entity_teleported(entities[i]);
			}

			world.update_world_transforms(false);
			return false;
		}

		world.update_world_transforms(false);
		return true;
	}
}
