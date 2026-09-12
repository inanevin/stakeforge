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
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class world_t;

	class world_audio_controller_t final
	{
	public:
		world_audio_controller_t()											 = default;
		~world_audio_controller_t()											 = default;
		world_audio_controller_t(const world_audio_controller_t&)			 = delete;
		world_audio_controller_t& operator=(const world_audio_controller_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(world_t& world);
		void uninit();
		void clear();
		void begin_play();
		void end_play();
		void destroy_entity(entity_id_t entity);
		void tick(f32 delta_time);
		void set_time_scale(f32 time_scale);

		// -----------------------------------------------------------------------------
		// playback
		// -----------------------------------------------------------------------------

		bool play(entity_id_t entity);
		void pause(entity_id_t entity);
		void stop(entity_id_t entity);
		void pause_all();
		void resume_all();

	private:
		bool		create_voice(entity_id_t entity, bool start);
		void		destroy_voice(entity_id_t entity);
		void		pause_voices();
		void		resume_voices();
		void		sync_sources(f32 delta_time);
		void		sync_listener(f32 delta_time);
		inline bool is_paused() const
		{
			return _is_explicitly_paused || _is_time_scale_paused;
		}

		inline bool is_playback_paused() const
		{
			return is_paused() || _time_scale == 0.0f;
		}

	private:
		world_t* _world				   = nullptr;
		f32		 _time_scale		   = 1.0f;
		bool	 _is_playing		   = false;
		bool	 _is_explicitly_paused = false;
		bool	 _is_time_scale_paused = false;
	};
}
