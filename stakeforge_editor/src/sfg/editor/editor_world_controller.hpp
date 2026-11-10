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

#include "editor/editor_exports.hpp"

#include "editor/common_editor.hpp"
#include "editor/editor_play_session.hpp"
#include "editor/world/editor_world_handle.hpp"
#include <sfg/core/common/size_definitions.hpp>
#include <sfg/core/data/atomic.hpp>
#include <sfg/core/data/span.hpp>
#include <sfg/core/data/string.hpp>
#include <sfg/core/data/vector.hpp>
#include <sfg/core/gfx/common/gfx_constants.hpp>
#include <sfg/core/io/assert.hpp>
#include <sfg/core/math/vec2u16.hpp>
#include <sfg/core/memory/dynamic_gen_pool.hpp>
#include <sfg/core/memory/pool_handle.hpp>

namespace sfg
{
	class editor_asset_manager_t;
	class editor_command_system_t;
	class editor_world_t;
	class world_t;
	struct world_config_t;
	enum class editor_world_camera_type_e : u8;
	struct editor_asset_deletion_listener_tag_t;
	struct editor_command_listener_tag_t;
	struct editor_command_t;
	class plugin_type_schema_t;
	struct plugin_type_schema_delta_t;

	class SFG_EDITOR_API editor_world_controller_t final
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

		editor_world_handle_t create_world(
			const world_config_t& init_config, editor_world_camera_type_e camera_type, editor_world_edit_type_e edit_type = editor_world_edit_type_e::full_control, editor_world_tick_callback_t tick_callback = nullptr, void* tick_callback_user_data = nullptr);
		void destroy_world(editor_world_handle_t handle);
		void destroy_worlds();
		void resize_world(editor_world_handle_t handle, vec2u16_t render_resolution);
		bool acquire_render_worlds();
		bool render_worlds(gfx_handle_t queue, gfx_handle_t signal, u64 signal_value, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);
		void tick(u32 world_tick_rate, u32 world_physics_rate, u32 max_sim_steps);
		void update_physics_settings(const u64* collision_masks, u64 active_layers, u32 physics_rate, u32 max_sub_steps, bool kinematic_sensors_collide_with_non_dynamic);
		void load_dummy_world();
		bool load_main_world(sid_t asset_guid);
		bool save_main_world();
		void mark_world_dirty(editor_world_handle_t handle);
		void apply_plugin_type_schema(const plugin_type_schema_t& previous, const plugin_type_schema_t& current, const plugin_type_schema_delta_t& delta);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------
		inline editor_play_session_t& get_play_session()
		{
			return _play_session;
		}

		inline const editor_play_session_t& get_play_session() const
		{
			return _play_session;
		}

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

		static inline bool is_initialized()
		{
			return s_instance != nullptr;
		}

	private:
		friend class editor_play_session_t;

		f32	 calculate_render_alpha() const;
		void tick_preview_world(editor_world_t& editor_world, f32 dt_seconds);
		void destroy_world_internal(editor_world_handle_t handle);
		void destroy_main_world_internal();
		void destroy_worlds_internal(bool notify_panels);
		void set_main_world(editor_world_handle_t handle, sid_t asset_guid, const char* name);
		bool load_main_world_now(sid_t asset_guid);
		void notify_main_world_changed();
		void notify_main_world_dirty_changed();
		void set_main_world_dirty(bool dirty);

		static void on_save_dirty_world_modal(void* user_data);
		static void on_dont_save_dirty_world_modal(void* user_data);
		static void on_cancel_dirty_world_modal(void* user_data);
		static void on_asset_deletion(editor_asset_manager_t& asset_manager, span_t<const sid_t> asset_ids, void* user_data);
		static void on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data);

	private:
		dynamic_gen_pool_t<editor_world_t*, u32, editor_world_handle_tag_t> _worlds;
		editor_play_session_t												_play_session = {};
		string_t															_main_world_name;
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

		static editor_world_controller_t* s_instance;
	};
}
