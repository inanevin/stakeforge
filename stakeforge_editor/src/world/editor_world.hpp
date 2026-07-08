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

#include "world_edit/editor_world_edit_context.hpp"
#include "world/editor_world_handle.hpp"
#include <sfg/data/atomic.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/runtime/render/world_render_context.hpp>
#include <sfg/runtime/render/world_render_snapshot.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	class editor_world_t final
	{
	public:
		editor_world_t()								 = default;
		~editor_world_t()								 = default;
		editor_world_t(const editor_world_t&)			 = delete;
		editor_world_t& operator=(const editor_world_t&) = delete;
		editor_world_t(editor_world_t&& other) noexcept;
		editor_world_t& operator=(editor_world_t&& other) noexcept;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(editor_world_handle_t handle, vec2u16_t render_resolution);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void						   resize(vec2u16_t render_resolution);
		void						   tick(f32 dt_seconds);
		void						   update_world_transforms(bool advance_interpolation);
		void						   produce_snapshot();
		const world_render_snapshot_t& acquire_render_snapshot();
		void						   render(const world_render_snapshot_t& snapshot, f32 interpolation_alpha, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline world_t& get_world()
		{
			return _world;
		}

		inline const world_t& get_world() const
		{
			return _world;
		}

		inline world_render_context_t& get_render_context()
		{
			return _render_context;
		}

		inline const world_render_context_t& get_render_context() const
		{
			return _render_context;
		}

		inline editor_world_edit_context_t& get_edit_context()
		{
			return _edit_context;
		}

		inline const editor_world_edit_context_t& get_edit_context() const
		{
			return _edit_context;
		}

		inline gfx_handle_t get_command_buffer(u8 frame_index) const
		{
			return _render_context.get_command_buffer(frame_index);
		}

		inline vec2u16_t get_render_resolution() const
		{
			return _render_resolution;
		}

	private:
		void publish_snapshot();

	private:
		world_render_snapshot_t		_snapshot_slots[3] = {};
		world_render_context_t		_render_context	   = {};
		editor_world_edit_context_t _edit_context	   = {};
		world_t						_world			   = {};
		atomic_t<u8>				_snapshot_mailbox  = {};
		vec2u16_t					_render_resolution = vec2u16_t::zero;
		u8							_producer_slot	   = 0;
		u8							_consumer_slot	   = 0;
	};
}
