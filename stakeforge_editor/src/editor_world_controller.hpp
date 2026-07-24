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
#include "world/editor_world_handle.hpp"
#include <sfg/common/size_definitions.hpp>
#include <sfg/data/atomic.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/memory/dynamic_gen_pool.hpp>
#include <sfg/memory/pool_handle.hpp>

namespace sfg
{
	class editor_asset_manager_t;
	class editor_command_system_t;
	class editor_world_t;
	class world_t;
	enum class editor_play_mode_e : u8;
	struct world_init_config_t;
	struct world_render_snapshot_t;
	struct editor_asset_deletion_listener_tag_t;
	struct editor_command_listener_tag_t;
	struct editor_command_t;

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

		editor_world_handle_t create_world(const world_init_config_t& init_config, editor_world_edit_type_e edit_type = editor_world_edit_type_e::full_control, editor_world_tick_callback_t tick_callback = nullptr, void* tick_callback_user_data = nullptr);
		void				  destroy_world(editor_world_handle_t handle);
		void				  destroy_worlds();
		void				  resize_world(editor_world_handle_t handle, vec2u16_t render_resolution);
		bool				  acquire_render_worlds();
		bool				  render_worlds(gfx_handle_t queue, gfx_handle_t signal, u64 signal_value, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);
		void				  tick(u32 world_tick_rate, u32 world_physics_rate, u32 max_sim_steps);
		void				  update_physics_settings(const u64* collision_masks, u64 active_layers, u32 physics_rate, u32 max_sub_steps);
		void				  install_default_world(editor_world_handle_t handle);
		void				  load_dummy_world();
		bool				  load_main_world(sid_t asset_guid);
		bool				  save_main_world();
		void				  mark_world_dirty(editor_world_handle_t handle);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------
		inline editor_world_t* get_editor_world(editor_world_handle_t handle)
		{
			return _worlds.get(handle);
		}

		inline const editor_world_t* get_editor_world(editor_world_handle_t handle) const
		{
			return _worlds.get(handle);
		}

		inline editor_world_handle_t get_main_world_handle() const
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

		inline bool is_world_valid(editor_world_handle_t handle) const
		{
			return _worlds.is_valid(handle);
		}

		inline bool is_main_world_dirty() const
		{
			return _main_world_dirty;
		}

		static inline editor_world_controller_t& get()
		{
			return *s_instance;
		}

	private:
		struct acquired_render_world_t
		{
			editor_world_t*				   world	= nullptr;
			const world_render_snapshot_t* snapshot = nullptr;
		};

		f32	 calculate_render_alpha() const;
		void destroy_world_internal(editor_world_handle_t handle);
		void destroy_main_world_internal();
		void destroy_worlds_internal(bool notify_panels);
		void stop_main_world_play_mode();
		void set_main_world(editor_world_handle_t handle, sid_t asset_guid, const char* name);
		bool load_main_world_now(sid_t asset_guid);
		void notify_main_world_changed();
		void notify_main_world_dirty_changed();
		void set_main_world_dirty(bool dirty);
		void update_main_world_play_mode(editor_play_mode_e mode);
		void tick_editor_world(editor_world_t& editor_world, bool is_main_world, editor_play_mode_e mode, f32 dt_seconds, bool force_simulation);

		static void on_save_dirty_world_modal(void* user_data);
		static void on_dont_save_dirty_world_modal(void* user_data);
		static void on_cancel_dirty_world_modal(void* user_data);
		static void on_asset_deletion(editor_asset_manager_t& asset_manager, span_t<const sid_t> asset_ids, void* user_data);
		static void on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data);

	private:
		dynamic_gen_pool_t<editor_world_t*, u32, editor_world_handle_tag_t> _worlds;
		string_t															_main_world_name;
		vector_t<acquired_render_world_t>									_render_worlds;
		sid_t																_main_world_asset_guid		   = NULL_SID;
		sid_t																_pending_main_world_asset_guid = NULL_SID;
		i64																	_previous_time_us			   = 0;
		i64																	_accumulator_us				   = 0;
		editor_world_handle_t												_main_world					   = {};
		pool_handle_t<u32, editor_asset_deletion_listener_tag_t>			_asset_deletion_listener	   = {};
		pool_handle_t<u32, editor_command_listener_tag_t>					_command_listener			   = {};
		atomic_t<i64>														_last_fixed_step_us			   = 0;
		atomic_t<i64>														_fixed_step_us				   = 0;
		f32																	_render_alpha				   = 0.0f;
		bool																_main_world_dirty			   = false;
		bool																_play_main_world_dirty		   = false;

		static inline editor_world_controller_t* s_instance = nullptr;
	};
}
