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
#include <sfg/data/atomic.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/engine/common_engine.hpp>
#include <sfg/runtime/render/world_render_context.hpp>
#include <sfg/runtime/render/world_render_snapshot.hpp>

namespace sfg
{
	class engine_runtime_t;
	class world_t;

	class editor_world_controller_t final
	{
	public:
		editor_world_controller_t()											   = default;
		~editor_world_controller_t()										   = default;
		editor_world_controller_t(const editor_world_controller_t&)			   = delete;
		editor_world_controller_t& operator=(const editor_world_controller_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------
		void init(engine_runtime_t& runtime);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------
		world_handle_t create_world(vec2u16_t render_resolution);
		void		   destroy_world(world_handle_t handle);
		void		   destroy_worlds();
		void		   resize_world(world_handle_t handle, vec2u16_t render_resolution);
		bool		   render_worlds(gfx_queue_handle queue, gfx_semaphore_handle signal, u64 signal_value, u8 frame_index, gpu_index_t global_cbv_index, gfx_bind_layout_handle global_layout);
		void		   tick(u32 world_tick_rate, u32 world_physics_rate, u32 max_sim_steps);
		void		   install_default_world(world_handle_t handle);
		void		   set_main_world(world_handle_t handle);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------
		const world_render_context_t& get_world_render_context(world_handle_t handle) const;
		world_handle_t				  get_main_world() const;
		f32							  get_alpha() const;

	private:
		struct world_container_t
		{
			world_container_t()									   = default;
			~world_container_t()								   = default;
			world_container_t(const world_container_t&)			   = delete;
			world_container_t& operator=(const world_container_t&) = delete;
			world_container_t(world_container_t&& other) noexcept
			{
				*this = static_cast<world_container_t&&>(other);
			}
			world_container_t& operator=(world_container_t&& other) noexcept;

			world_render_snapshot_t snapshot_slots[3] = {};
			world_render_context_t	render_context	  = {};
			atomic_t<u8>			snapshot_mailbox  = {};
			world_handle_t			handle			  = {};
			u8						producer_slot	  = 0;
			u8						consumer_slot	  = 0;
		};

		void						   publish_world_snapshot(world_container_t& container);
		const world_render_snapshot_t& acquire_render_snapshot(world_container_t& container);
		f32							   calculate_render_alpha() const;
		void						   install_editor_camera(world_t& world);

		engine_runtime_t*			_runtime = nullptr;
		vector_t<world_container_t> _worlds;
		world_handle_t				_main_world			= {};
		i64							_previous_time_us	= 0;
		i64							_accumulator_us		= 0;
		atomic_t<i64>				_last_fixed_step_us = 0;
		atomic_t<i64>				_fixed_step_us		= 0;
		u32							_world_physics_rate = 100;
	};
}
