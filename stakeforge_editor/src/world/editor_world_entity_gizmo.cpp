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

#include "world/editor_world_entity_gizmo.hpp"
#include "world/editor_world_edit_context.hpp"
#include "commands/editor_command_component_edit.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/world/ecs.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	void editor_world_entity_gizmo_t::init(world_t& world, const editor_world_edit_context_t& context)
	{
		_world	 = &world;
		_context = &context;

		if (context.get_edit_type() == editor_world_edit_type_e::full_control)
		{
			_initial_absolute.reserve(256);
			_initial_parent_inverse.reserve(256);
			_initial_local_rotations.reserve(256);
			_initial_local_positions.reserve(256);
			_initial_local_scales.reserve(256);
			_entities.reserve(256);
		}
	}

	void editor_world_entity_gizmo_t::uninit()
	{
		clear_action();
		_world	 = nullptr;
		_context = nullptr;
	}

	editor_gizmo_callbacks_t editor_world_entity_gizmo_t::get_callbacks()
	{
		return {
			.get_target = [](void* user_data, editor_gizmo_target_t& target) { return static_cast<editor_world_entity_gizmo_t*>(user_data)->get_target(target); },
			.begin		= [](void* user_data) { return static_cast<editor_world_entity_gizmo_t*>(user_data)->begin(); },
			.update		= [](void* user_data, const mat4x3_t& delta) { static_cast<editor_world_entity_gizmo_t*>(user_data)->update(delta); },
			.commit		= [](void* user_data) { static_cast<editor_world_entity_gizmo_t*>(user_data)->commit(); },
			.cancel		= [](void* user_data) { static_cast<editor_world_entity_gizmo_t*>(user_data)->cancel(); },
			.user_data	= this,
		};
	}

	bool editor_world_entity_gizmo_t::get_target(editor_gizmo_target_t& target) const
	{
		const entity_id_t anchor = _context->get_mutable_entity_anchor(*_world);

		if (anchor == NULL_ENTITY_ID)
			return false;

		const component_system_transform_t& transform = _world->get_component_table(type_id_t<component_system_transform_t>::value).get_as_const<component_system_transform_t>(anchor);
		vec3f_t								scale	  = vec3f_t::one;
		_world->calculate_transform_direct(anchor).decompose(target.position, target.rotation, scale);
		target.prev_position = transform.prev_abs_pos;
		target.prev_rotation = transform.prev_abs_rot;
		target.generation	 = _context->get_selection_generation();
		return true;
	}

	bool editor_world_entity_gizmo_t::begin()
	{
		world_t&						   world   = *_world;
		const editor_world_edit_context_t& context = *_context;

		_initial_absolute.resize(0);
		_initial_parent_inverse.resize(0);
		_initial_local_rotations.resize(0);
		_initial_local_positions.resize(0);
		_initial_local_scales.resize(0);
		_entities.resize(0);

		const span_t<const entity_id_t> selected = context.get_selected_entities();
		_entities.resize(selected.size);
		_entities.resize(context.collect_selected_mutable_root_entities(world, {.data = _entities.data(), .size = _entities.size()}));

		for (entity_id_t entity : _entities)
		{
			const entity_id_t parent = world.get_entity_parent(entity);

			_initial_local_rotations.push_back(world.get_entity_rot_local(entity));
			_initial_local_positions.push_back(world.get_entity_pos_local(entity));
			_initial_local_scales.push_back(world.get_entity_scale_local(entity));
			_initial_absolute.push_back(world.calculate_transform_direct(entity));
			_initial_parent_inverse.push_back(parent == NULL_ENTITY_ID ? mat4x3_t::identity : world.calculate_transform_direct(parent).inverse());
		}

		SFG_ASSERT(!_entities.empty());
		return true;
	}

	void editor_world_entity_gizmo_t::update(const mat4x3_t& delta)
	{
		world_t& world = *_world;

		for (size_t i = 0; i < _entities.size(); ++i)
		{
			vec3f_t position = vec3f_t::zero;
			quat_t	rotation = quat_t::identity;
			vec3f_t scale	 = vec3f_t::one;
			(_initial_parent_inverse[i] * delta * _initial_absolute[i]).decompose(position, rotation, scale);

			world.set_entity_pos_local(_entities[i], position);
			world.set_entity_rot_local(_entities[i], rotation);
			world.set_entity_scale_local(_entities[i], scale);
			world.mark_entity_teleported(_entities[i]);
		}

		world.update_world_transforms(false);
	}

	void editor_world_entity_gizmo_t::commit()
	{
		world_t& world = *_world;

		frame_vector_t<ostream_t> previous_streams = {};
		frame_vector_t<ostream_t> post_streams	   = {};
		previous_streams.reserve(_entities.size());
		post_streams.reserve(_entities.size());

		for (size_t i = 0; i < _entities.size(); ++i)
		{
			component_transform_t previous = {
				.pos   = _initial_local_positions[i],
				.rot   = _initial_local_rotations[i],
				.scale = _initial_local_scales[i],
			};

			ostream_t previous_stream = {};

			if (!reflection_registry_t::get().type_to_stream(type_id_t<component_transform_t>::value, &previous, nullptr, previous_stream))
			{
				SFG_ERR("failed to serialize previous gizmo transform for entity {0}", _entities[i]);
				cancel();
				return;
			}

			component_transform_t current = {
				.pos   = world.get_entity_pos_local(_entities[i]),
				.rot   = world.get_entity_rot_local(_entities[i]),
				.scale = world.get_entity_scale_local(_entities[i]),
			};

			ostream_t post_stream = {};

			if (!reflection_registry_t::get().type_to_stream(type_id_t<component_transform_t>::value, &current, nullptr, post_stream))
			{
				SFG_ERR("failed to serialize current gizmo transform for entity {0}", _entities[i]);
				cancel();
				return;
			}

			previous_streams.push_back(std::move(previous_stream));
			post_streams.push_back(std::move(post_stream));
		}

		if (!editor_command_component_edit_t::edit(_context->get_world(),
												   {.data = _entities.data(), .size = _entities.size()},
												   type_id_t<component_transform_t>::value,
												   {.data = previous_streams.data(), .size = previous_streams.size()},
												   {.data = post_streams.data(), .size = post_streams.size()}))
		{
			cancel();
			return;
		}

		clear_action();
	}

	void editor_world_entity_gizmo_t::cancel()
	{
		world_t& world = *_world;

		for (size_t i = 0; i < _entities.size(); ++i)
		{
			world.set_entity_pos_local(_entities[i], _initial_local_positions[i]);
			world.set_entity_rot_local(_entities[i], _initial_local_rotations[i]);
			world.set_entity_scale_local(_entities[i], _initial_local_scales[i]);
			world.mark_entity_teleported(_entities[i]);
		}

		world.update_world_transforms(false);
		clear_action();
	}

	void editor_world_entity_gizmo_t::clear_action()
	{
		_initial_absolute.resize(0);
		_initial_parent_inverse.resize(0);
		_initial_local_rotations.resize(0);
		_initial_local_positions.resize(0);
		_initial_local_scales.resize(0);
		_entities.resize(0);
	}
}
