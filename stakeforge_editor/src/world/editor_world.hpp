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

#include "world/editor_world_camera.hpp"
#include "world/editor_world_edit_context.hpp"
#include "world/editor_world_gizmo.hpp"
#include "world/editor_world_handle.hpp"
#include "world/editor_world_render_context.hpp"
#include <sfg/data/atomic.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/runtime/render/world_render_snapshot.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	struct aabb_t;
	struct world_init_config_t;

	struct editor_world_pick_request_t
	{
		vec2f_t relative_position	  = vec2f_t::zero;
		u32		id					  = 0;
		bool	incremental_selection = false;
	};

	struct editor_world_gizmo_snapshot_data_t
	{
		quat_t							prev_rotation = quat_t::identity;
		quat_t							rotation	  = quat_t::identity;
		vec3f_t							prev_position = vec3f_t::zero;
		vec3f_t							position	  = vec3f_t::zero;
		editor_transform_control_type_e control_type  = editor_transform_control_type_e::invalid;
		editor_gizmo_axis_e				hovered_axis  = editor_gizmo_axis_e::invalid;
		editor_gizmo_axis_e				active_axis	  = editor_gizmo_axis_e::invalid;
	};

	struct editor_world_grid_snapshot_data_t
	{
		f32	 scale	 = 1.0f;
		bool enabled = false;
	};

	struct editor_world_snapshot_data_t
	{
		vector_t<entity_id_t>			   selected_entities;
		editor_world_gizmo_snapshot_data_t gizmo		= {};
		editor_world_pick_request_t		   pick_request = {};
		editor_world_grid_snapshot_data_t  grid			= {};
	};

	class editor_world_t final
	{
	public:
		editor_world_t()								  = default;
		~editor_world_t()								  = default;
		editor_world_t(const editor_world_t&)			  = delete;
		editor_world_t& operator=(const editor_world_t&)  = delete;
		editor_world_t(editor_world_t&& other)			  = delete;
		editor_world_t& operator=(editor_world_t&& other) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(const world_init_config_t& init_config, editor_world_handle_t handle);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void						   resize(vec2u16_t render_resolution);
		void						   install_camera(editor_world_camera_type_e type);
		void						   uninstall_camera();
		void						   pass_camera_input(const editor_world_camera_input_t& input);
		void						   reset_camera_input();
		void						   tick_camera(f32 dt_seconds);
		void						   fit_camera_to_bounds(const aabb_t& bounds);
		void						   update_gizmo_hover(vec2f_t relative_position);
		void						   clear_gizmo_hover();
		bool						   begin_gizmo_action(vec2f_t relative_position);
		void						   update_gizmo_action(vec2f_t relative_position);
		void						   end_gizmo_action();
		void						   cancel_gizmo_action();
		void						   request_entity_pick(vec2f_t relative_position, bool incremental_selection);
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

		inline editor_world_render_context_t& get_render_context()
		{
			return _render_context;
		}

		inline const editor_world_render_context_t& get_render_context() const
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

		inline vec2u16_t get_render_resolution() const
		{
			return _render_resolution;
		}

	private:
		void publish_snapshot();
		void consume_entity_pick_result();

	private:
		world_render_snapshot_t _snapshot_slots[3] = {};
		editor_world_camera_t*	_camera			   = nullptr;

		editor_world_render_context_t _render_context = {};

		editor_world_gizmo_t		_gizmo										 = {};
		editor_world_edit_context_t _edit_context								 = {};
		world_t						_world										 = {};
		atomic_t<u64>				_pick_result								 = {};
		editor_world_pick_request_t _pending_pick_request						 = {};
		atomic_t<u8>				_snapshot_mailbox							 = {};
		u32							_last_render_pick_request_id				 = 0;
		u32							_next_pick_request_id						 = 0;
		vec2u16_t					_render_resolution							 = vec2u16_t::zero;
		bool						_object_id_readback_valid[BACK_BUFFER_COUNT] = {};
		u8							_producer_slot								 = 0;
		u8							_consumer_slot								 = 0;
	};
}
