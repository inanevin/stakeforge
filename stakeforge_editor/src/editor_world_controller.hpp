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

#include "common_editor.hpp"
#include <sfg/common/size_definitions.hpp>
#include <sfg/data/atomic.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/memory/dynamic_gen_pool.hpp>
#include <sfg/runtime/engine/common_engine.hpp>
#include <sfg/runtime/render/world_render_context.hpp>
#include <sfg/runtime/render/world_render_snapshot.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	class editor_command_system_t;
	class world_t;
	struct editor_command_listener_tag_t;
	struct editor_command_t;
	struct window_event_t;
	struct window_runtime_t;

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
		void init();
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------
		world_handle_t create_world(vec2u16_t render_resolution);
		void		   destroy_world(world_handle_t handle);
		void		   destroy_worlds();
		void		   resize_world(world_handle_t handle, vec2u16_t render_resolution);
		bool		   render_worlds(gfx_handle_t queue, gfx_handle_t signal, u64 signal_value, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);
		void		   tick(u32 world_tick_rate, u32 world_physics_rate, u32 max_sim_steps);
		void		   install_default_world(world_handle_t handle);
		void		   load_dummy_world();
		bool		   load_main_world(sid_t asset_guid);
		bool		   save_main_world();
		void		   reset_input(window_runtime_t& runtime);
		bool		   on_window_event(surface_handle_t surface_handle, window_runtime_t& runtime, const window_event_t& ev);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------
		const world_render_context_t& get_world_render_context(world_handle_t handle) const;

		inline world_t& get_world(world_handle_t handle)
		{
			return _worlds.get(handle);
		}

		inline const world_t& get_world(world_handle_t handle) const
		{
			return _worlds.get(handle);
		}

		inline world_handle_t get_main_world() const
		{
			return _main_world;
		}

		inline const char* get_main_world_name() const
		{
			return _main_world_name.c_str();
		}

		inline f32 get_alpha() const
		{
			return calculate_render_alpha();
		}

		inline bool is_world_valid(world_handle_t handle) const
		{
			return _worlds.is_valid(handle);
		}

		inline bool is_world_panel_focused() const
		{
			return _world_panel_focused;
		}

		static inline editor_world_controller_t& get()
		{
			SFG_ASSERT(s_instance != nullptr);
			return *s_instance;
		}

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
			vector_t<u64>			world_resources	  = {};
			atomic_t<u8>			snapshot_mailbox  = {};
			world_handle_t			handle			  = {};
			u8						producer_slot	  = 0;
			u8						consumer_slot	  = 0;
		};

	private:
		void						   publish_world_snapshot(world_container_t& container);
		const world_render_snapshot_t& acquire_render_snapshot(world_container_t& container);
		f32							   calculate_render_alpha() const;
		void						   destroy_worlds_internal(bool notify_panels);
		void						   set_main_world(world_handle_t handle, sid_t asset_guid, const char* name);
		bool						   load_main_world_now(sid_t asset_guid);
		void						   notify_main_world_changed();
		void						   install_editor_camera(world_t& world);
		void						   reset_camera_input();
		void						   tick_editor_camera(f32 dt_seconds);
		void						   set_main_world_dirty();
		static void					   on_save_dirty_world_modal(void* user_data);
		static void					   on_dont_save_dirty_world_modal(void* user_data);
		static void					   on_cancel_dirty_world_modal(void* user_data);
		static void					   on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data);

	private:
		dynamic_gen_pool_t<world_t, u32, world_handle_tag> _worlds;
		vector_t<world_container_t>						   _world_containers;
		string_t										   _main_world_name;
		vec3f_t											   _direction_input				  = vec3f_t::zero;
		vec2f_t											   _mouse_delta					  = vec2f_t::zero;
		world_handle_t									   _main_world					  = {};
		sid_t											   _main_world_asset_guid		  = NULL_SID;
		sid_t											   _pending_main_world_asset_guid = NULL_SID;
		entity_id_t										   _main_camera_entity			  = NULL_ENTITY_ID;
		pool_handle_t<u32, editor_command_listener_tag_t>  _command_listener			  = {};
		i64												   _previous_time_us			  = 0;
		i64												   _accumulator_us				  = 0;
		atomic_t<i64>									   _last_fixed_step_us			  = 0;
		atomic_t<i64>									   _fixed_step_us				  = 0;
		f32												   _camera_yaw_degrees			  = 0.0f;
		f32												   _camera_pitch_degrees		  = 0.0f;
		f32												   _current_move_speed			  = 12.0f;
		u32												   _world_physics_rate			  = 100;
		bool											   _world_panel_focused			  = false;
		bool											   _main_world_dirty			  = false;
		bool											   _is_looking					  = false;

		static inline editor_world_controller_t* s_instance = nullptr;
	};
}
