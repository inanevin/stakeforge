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

#include "world_audio_controller.hpp"
#include "engine_components.hpp"
#include "system_components.hpp"
#include "world.hpp"

#include <sfg/audio/audio_engine.hpp>
#include <sfg/memory/frame_allocator.hpp>
#include <sfg/runtime/resources/audio.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/world/ecs.hpp>

namespace sfg
{
	void world_audio_controller_t::init(world_t& world)
	{
		SFG_ASSERT(_world == nullptr);

		_world = &world;
	}

	void world_audio_controller_t::uninit()
	{
		SFG_ASSERT(!_is_playing);

		_world				  = nullptr;
		_time_scale			  = 1.0f;
		_is_explicitly_paused = false;
		_is_time_scale_paused = false;
	}

	void world_audio_controller_t::clear()
	{
		ecs_component_table_t&			system_table = _world->get_component_table(type_id_t<component_system_audio_source_t>::value);
		frame_vector_t<entity_id_t>		entities	 = {};
		const ecs_component_table_ref_t refs[]		 = {
			system_table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
			entities.push_back(row.id);

		for (const entity_id_t entity : entities)
			destroy_voice(entity);
	}

	void world_audio_controller_t::begin_play()
	{
		SFG_ASSERT(_world != nullptr);
		SFG_ASSERT(!_is_playing);

		_is_playing			  = true;
		_is_explicitly_paused = false;
		_is_time_scale_paused = _time_scale == 0.0f;

		sync_sources(0.0f);
		sync_listener(0.0f);
	}

	void world_audio_controller_t::end_play()
	{
		SFG_ASSERT(_is_playing);

		clear();

		_is_playing			  = false;
		_is_explicitly_paused = false;
		_is_time_scale_paused = false;
	}

	void world_audio_controller_t::destroy_entity(entity_id_t entity)
	{
		const ecs_component_table_t& system_table = _world->get_component_table(type_id_t<component_system_audio_source_t>::value);

		if (!system_table.has(entity))
			return;

		destroy_voice(entity);
	}

	void world_audio_controller_t::tick(f32 delta_time)
	{
		if (!_is_playing)
			return;

		const bool was_paused = is_paused();

		_is_time_scale_paused = _time_scale == 0.0f;

		const bool resume_after_sync = was_paused && !is_paused();

		if (!was_paused && is_paused())
			pause_voices();

		sync_sources(delta_time);
		sync_listener(delta_time);

		if (resume_after_sync)
			resume_voices();
	}

	void world_audio_controller_t::set_time_scale(f32 time_scale)
	{
		SFG_ASSERT(time_scale >= 0.0f);

		_time_scale = time_scale;
	}

	bool world_audio_controller_t::play(entity_id_t entity)
	{
		SFG_ASSERT(_is_playing);

		ecs_component_table_t&			 system_table = _world->get_component_table(type_id_t<component_system_audio_source_t>::value);
		component_system_audio_source_t* system		  = system_table.find_as<component_system_audio_source_t>(entity);

		if (system == nullptr || !audio_engine_t::get().is_voice_valid(system->voice))
			return create_voice(entity, true);

		system->play_requested = true;

		if (is_playback_paused())
		{
			system->resume_after_pause = true;
			return true;
		}

		return audio_engine_t::get().start_voice(system->voice);
	}

	void world_audio_controller_t::pause(entity_id_t entity)
	{
		SFG_ASSERT(_is_playing);

		ecs_component_table_t&			 system_table = _world->get_component_table(type_id_t<component_system_audio_source_t>::value);
		component_system_audio_source_t* system		  = system_table.find_as<component_system_audio_source_t>(entity);

		if (system == nullptr)
		{
			SFG_ERR("can't pause audio source for entity {0}: system audio source is missing", entity);
			return;
		}

		system->resume_after_pause = false;
		audio_engine_t::get().pause_voice(system->voice);
	}

	void world_audio_controller_t::stop(entity_id_t entity)
	{
		SFG_ASSERT(_is_playing);

		ecs_component_table_t&			 system_table = _world->get_component_table(type_id_t<component_system_audio_source_t>::value);
		component_system_audio_source_t* system		  = system_table.find_as<component_system_audio_source_t>(entity);

		if (system == nullptr)
		{
			SFG_ERR("can't stop audio source for entity {0}: system audio source is missing", entity);
			return;
		}

		system->play_requested	   = false;
		system->resume_after_pause = false;
		audio_engine_t::get().stop_voice(system->voice);
	}

	void world_audio_controller_t::pause_all()
	{
		SFG_ASSERT(_is_playing);

		if (_is_explicitly_paused)
			return;

		const bool was_paused = is_paused();

		_is_explicitly_paused = true;

		if (!was_paused)
			pause_voices();
	}

	void world_audio_controller_t::resume_all()
	{
		SFG_ASSERT(_is_playing);

		if (!_is_explicitly_paused)
			return;

		_is_explicitly_paused = false;

		if (!is_paused())
			resume_voices();
	}

	void world_audio_controller_t::pause_voices()
	{
		const ecs_component_table_t&	system_table = _world->get_component_table(type_id_t<component_system_audio_source_t>::value);
		const ecs_component_table_ref_t refs[]		 = {
			system_table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			component_system_audio_source_t& system = system_table.get_as<component_system_audio_source_t>(row.id);

			if (audio_engine_t::get().is_voice_valid(system.voice) && audio_engine_t::get().is_voice_playing(system.voice))
			{
				system.resume_after_pause = true;
				audio_engine_t::get().pause_voice(system.voice);
			}
		}
	}

	void world_audio_controller_t::resume_voices()
	{
		const ecs_component_table_t&	system_table = _world->get_component_table(type_id_t<component_system_audio_source_t>::value);
		const ecs_component_table_ref_t refs[]		 = {
			system_table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			component_system_audio_source_t& system = system_table.get_as<component_system_audio_source_t>(row.id);

			if (system.resume_after_pause && audio_engine_t::get().is_voice_valid(system.voice) && !audio_engine_t::get().is_voice_at_end(system.voice))
				audio_engine_t::get().start_voice(system.voice);

			system.resume_after_pause = false;
		}
	}

	bool world_audio_controller_t::create_voice(entity_id_t entity, bool start)
	{
		const ecs_component_table_t& source_table	 = _world->get_component_table(type_id_t<component_audio_source_t>::value);
		const ecs_component_table_t& transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);

		ecs_component_table_t&				system_table = _world->get_component_table(type_id_t<component_system_audio_source_t>::value);
		const component_audio_source_t&		source		 = source_table.get_as_const<component_audio_source_t>(entity);
		const component_system_transform_t& transform	 = transform_table.get_as_const<component_system_transform_t>(entity);
		const audio_runtime_t*				audio		 = resource_manager_t::get().find_runtime<audio_runtime_t>(source.audio);

		if (audio == nullptr)
			return false;

		component_system_audio_source_t& system = system_table.add_or_get_as<component_system_audio_source_t>(entity);

		if (audio_engine_t::get().is_voice_valid(system.voice))
			audio_engine_t::get().destroy_voice(system.voice);

		const audio_voice_settings_t settings{
			.position		= transform.abs_pos,
			.direction		= transform.abs_rot * vec3f_t::forward,
			.velocity		= vec3f_t::zero,
			.volume			= source.volume,
			.pitch			= _time_scale > 0.0f ? source.pitch * _time_scale : source.pitch,
			.min_distance	= source.min_distance,
			.max_distance	= source.max_distance,
			.rolloff		= source.rolloff,
			.doppler_factor = source.doppler_factor,
			.attenuation	= source.attenuation,
			.bus			= source.bus,
			.spatialized	= source.spatialized != 0,
			.looping		= source.looping != 0,
		};
		const audio_voice_create_desc_t desc{
			.settings	  = settings,
			.resource	  = source.audio,
			.preview	  = false,
			.auto_destroy = false,
		};

		system.voice			  = audio_engine_t::get().create_voice(*audio, desc);
		system.audio			  = source.audio;
		system.bus				  = source.bus;
		system.play_requested	  = start;
		system.resume_after_pause = start && is_playback_paused();

		if (!audio_engine_t::get().is_voice_valid(system.voice))
			return false;

		if (start && !is_playback_paused())
			return audio_engine_t::get().start_voice(system.voice);

		return true;
	}

	void world_audio_controller_t::destroy_voice(entity_id_t entity)
	{
		ecs_component_table_t&				   system_table = _world->get_component_table(type_id_t<component_system_audio_source_t>::value);
		const component_system_audio_source_t& system		= system_table.get_as_const<component_system_audio_source_t>(entity);

		if (audio_engine_t::get().is_voice_valid(system.voice))
			audio_engine_t::get().destroy_voice(system.voice);

		system_table.remove(entity);
	}

	void world_audio_controller_t::sync_sources(f32 delta_time)
	{
		const ecs_component_table_t& alive_table	 = _world->get_component_table(type_id_t<component_alive_t>::value);
		const ecs_component_table_t& disabled_table	 = _world->get_component_table(type_id_t<component_disabled_t>::value);
		const ecs_component_table_t& source_table	 = _world->get_component_table(type_id_t<component_audio_source_t>::value);
		const ecs_component_table_t& transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);
		ecs_component_table_t&		 system_table	 = _world->get_component_table(type_id_t<component_system_audio_source_t>::value);

		{
			const ecs_component_table_ref_t refs[] = {
				alive_table.ref(),
				!disabled_table.ref(),
				source_table.ref(),
				transform_table.ref(),
				!system_table.ref(),
			};
			frame_vector_t<entity_id_t> entities = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				entities.push_back(row.id);

			for (const entity_id_t entity : entities)
			{
				const component_audio_source_t& source = source_table.get_as_const<component_audio_source_t>(entity);
				create_voice(entity, source.play_on_start != 0);
			}
		}

		{
			const ecs_component_table_ref_t refs[] = {
				alive_table.ref(),
				system_table.ref(),
				!source_table.ref(),
			};
			frame_vector_t<entity_id_t> entities = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				entities.push_back(row.id);

			for (const entity_id_t entity : entities)
				destroy_voice(entity);
		}

		{
			const ecs_component_table_ref_t refs[] = {
				alive_table.ref(),
				disabled_table.ref(),
				system_table.ref(),
			};
			frame_vector_t<entity_id_t> entities = {};

			for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
				entities.push_back(row.id);

			for (const entity_id_t entity : entities)
				destroy_voice(entity);
		}

		const ecs_component_table_ref_t refs[] = {
			alive_table.ref(),
			!disabled_table.ref(),
			source_table.ref(),
			transform_table.ref(),
			system_table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			const component_audio_source_t&		source	  = row.get<component_audio_source_t>(2);
			const component_system_transform_t& transform = row.get<component_system_transform_t>(3);
			component_system_audio_source_t&	system	  = row.get_mutable<component_system_audio_source_t>(4);

			if (system.audio != source.audio || system.bus != source.bus || !audio_engine_t::get().is_voice_valid(system.voice))
			{
				create_voice(row.id, system.play_requested);
				continue;
			}

			const vec3f_t				 velocity = delta_time > 0.0f ? (transform.abs_pos - transform.prev_abs_pos) / delta_time : vec3f_t::zero;
			const audio_voice_settings_t settings{
				.position		= transform.abs_pos,
				.direction		= transform.abs_rot * vec3f_t::forward,
				.velocity		= velocity,
				.volume			= source.volume,
				.pitch			= _time_scale > 0.0f ? source.pitch * _time_scale : source.pitch,
				.min_distance	= source.min_distance,
				.max_distance	= source.max_distance,
				.rolloff		= source.rolloff,
				.doppler_factor = source.doppler_factor,
				.attenuation	= source.attenuation,
				.bus			= source.bus,
				.spatialized	= source.spatialized != 0,
				.looping		= source.looping != 0,
			};
			audio_engine_t::get().set_voice_settings(system.voice, settings);
		}
	}

	void world_audio_controller_t::sync_listener(f32 delta_time)
	{
		const ecs_component_table_t&	alive_table		= _world->get_component_table(type_id_t<component_alive_t>::value);
		const ecs_component_table_t&	disabled_table	= _world->get_component_table(type_id_t<component_disabled_t>::value);
		const ecs_component_table_t&	listener_table	= _world->get_component_table(type_id_t<component_audio_listener_t>::value);
		const ecs_component_table_t&	transform_table = _world->get_component_table(type_id_t<component_system_transform_t>::value);
		const ecs_component_table_ref_t refs[]			= {
			alive_table.ref(),
			!disabled_table.ref(),
			listener_table.ref(),
			transform_table.ref(),
		};
		const component_audio_listener_t*	selected_listener  = nullptr;
		const component_system_transform_t* selected_transform = nullptr;

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = refs, .size = std::size(refs)}))
		{
			const component_audio_listener_t& listener = row.get<component_audio_listener_t>(2);

			if (selected_listener != nullptr && listener.priority <= selected_listener->priority)
				continue;

			selected_listener  = &listener;
			selected_transform = &row.get<component_system_transform_t>(3);
		}

		if (selected_listener == nullptr)
		{
			audio_engine_t::get().set_listener(vec3f_t::zero, vec3f_t::forward, vec3f_t::zero);
			return;
		}

		const vec3f_t velocity = delta_time > 0.0f ? (selected_transform->abs_pos - selected_transform->prev_abs_pos) / delta_time : vec3f_t::zero;
		audio_engine_t::get().set_listener(selected_transform->abs_pos, selected_transform->abs_rot * vec3f_t::forward, velocity);
	}
}
