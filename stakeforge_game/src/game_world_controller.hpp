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

#include <sfg/data/atomic.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/runtime/render/world_render_context.hpp>
#include <sfg/runtime/render/world_render_snapshot.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	class game_world_controller_t final
	{
	public:
		game_world_controller_t()										   = default;
		~game_world_controller_t()										   = default;
		game_world_controller_t(const game_world_controller_t&)			   = delete;
		game_world_controller_t& operator=(const game_world_controller_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(vec2u16_t render_resolution);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void tick();
		void resize(vec2u16_t render_resolution);
		bool load_world(sid_t world);
		bool load_world_by_name_hash(sid_t name_hash);
		bool acquire_render_world();
		bool render_world(gfx_handle_t queue, gfx_handle_t signal, u64 signal_value, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline world_t& get_main_world()
		{
			return _main_world;
		}

		inline const world_t& get_main_world() const
		{
			return _main_world;
		}

		inline gpu_index_t get_world_texture_index(u8 frame_index) const
		{
			return _render_context.get_world_texture_index(frame_index);
		}

		inline bool is_initialized() const
		{
			return _initialized;
		}

	private:
		void produce_snapshot();
		void publish_snapshot();
		f32	 calculate_render_alpha() const;

		world_render_snapshot_t		   _snapshot_slots[3]  = {};
		world_render_prep_data_t	   _render_prep_data   = {};
		world_render_context_t		   _render_context	   = {};
		world_t						   _main_world		   = {};
		const world_render_snapshot_t* _render_snapshot	   = nullptr;
		atomic_t<i64>				   _last_fixed_step_us = 0;
		atomic_t<i64>				   _fixed_step_us	   = 0;
		atomic_t<u8>				   _snapshot_mailbox   = {};
		i64							   _previous_time_us   = 0;
		i64							   _accumulator_us	   = 0;
		f32							   _render_alpha	   = 0.0f;
		u8							   _producer_slot	   = 0;
		u8							   _consumer_slot	   = 0;
		bool						   _initialized		   = false;
	};
}
