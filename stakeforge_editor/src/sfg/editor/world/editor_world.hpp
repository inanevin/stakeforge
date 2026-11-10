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

#include "editor/world/editor_world_camera.hpp"
#include "editor/world/editor_world_edit_context.hpp"
#include "editor/world/editor_world_gizmo.hpp"
#include "editor/world/editor_world_entity_gizmo.hpp"
#include "editor/world/editor_world_handle.hpp"
#include "editor/world/editor_world_input_controller.hpp"
#include "editor/world/editor_world_render_context.hpp"
#include <sfg/core/data/atomic.hpp>
#include <sfg/core/data/vector.hpp>
#include <sfg/core/math/quat.hpp>
#include <sfg/core/runtime/world_renderer/world_render_config.hpp>
#include <sfg/core/runtime/world/world.hpp>
#include <sfg/core/runtime/world/world_init_config.hpp>
#include <sfg/core/runtime/world_renderer/world_renderer.hpp>
#include <sfg/core/vendor/nhlohmann/json_fwd.hpp>

namespace sfg
{
	struct aabb_t;
	struct physics_raycast_t;
	struct world_renderer_view_t;

	struct SFG_EDITOR_API editor_world_init_config_t
	{
		world_render_context_config_t render_context = {};
		world_init_config_t			  world			 = {};

		static editor_world_init_config_t make_main(vec2u16_t render_resolution);
		static editor_world_init_config_t make_preview(vec2u16_t render_resolution);
	};

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

	struct editor_world_icon_t
	{
		vec3f_t					 previous_position = vec3f_t::zero;
		vec3f_t					 position		   = vec3f_t::zero;
		vec2f_t					 size_px		   = vec2f_t::zero;
		vec2f_t					 screen_offset	   = vec2f_t::zero;
		render_resource_handle_t texture		   = {};
		entity_id_t				 entity_id		   = NULL_ENTITY_ID;
	};

	struct editor_world_snapshot_data_t
	{
		vector_t<editor_world_icon_t>	   icons = {};
		vector_t<entity_id_t>			   selected_entities;
		editor_world_gizmo_snapshot_data_t gizmo		= {};
		editor_gizmo_snap_grid_t		   snap_grid	= {};
		editor_world_pick_request_t		   pick_request = {};
		editor_world_grid_snapshot_data_t  grid			= {};
		editor_world_view_e				   world_view	= editor_world_view_e::lit;
	};

	class SFG_EDITOR_API editor_world_t final
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

		void init(const editor_world_init_config_t& init_config, editor_world_handle_t handle, editor_world_camera_type_e camera_type, editor_world_edit_type_e edit_type, editor_world_tick_callback_t tick_callback, void* tick_callback_user_data);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void resize(vec2u16_t render_resolution);
		void recreate_camera_entity();
		void update_camera_control();
		void serialize_camera(nlohmann::json& out_json) const;
		void deserialize_camera(const nlohmann::json& in_json);
		void pass_camera_input(const editor_world_camera_input_t& input);
		void reset_camera_input();
		void tick_camera(f32 dt_seconds);
		void fit_camera_to_bounds(const aabb_t& bounds);
		void align_camera_to_selection();
		void update_gizmo_hover(vec2f_t relative_position);
		void clear_gizmo_hover();
		bool begin_gizmo_action(vec2f_t relative_position);
		void update_gizmo_action(vec2f_t relative_position);
		void end_gizmo_action();
		void cancel_gizmo_action();
		void set_gizmo_callbacks(const editor_gizmo_callbacks_t& callbacks);
		void request_entity_pick(vec2f_t relative_position, bool incremental_selection);
		bool get_camera_ray(vec2f_t relative_position, physics_raycast_t& ray) const;
		void reset_input();
		void begin_frame();
		void invoke_tick_callback(f32 delta_time);
		void draw_debug();
		void update_world_transforms();
		void update_screen_camera();
		void produce_snapshot();
		void render(f32 interpolation_alpha, u8 frame_index, gpu_index_t global_cbv_index, gfx_handle_t global_layout);

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

		inline world_renderer_t& get_world_renderer()
		{
			return *_world_renderer;
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

		inline editor_world_input_controller_t& get_input_controller()
		{
			return _input_controller;
		}

		inline const editor_world_input_controller_t& get_input_controller() const
		{
			return _input_controller;
		}

		inline vec2u16_t get_render_resolution() const
		{
			return _render_resolution;
		}

		bool is_editor_control_enabled() const;

		inline editor_world_camera_type_e get_camera_type() const
		{
			return _camera_type;
		}

		inline const quat_t& get_view_rotation() const
		{
			return _view_rotation;
		}

		inline bool is_gizmo_editing_supported() const
		{
			return _custom_gizmo || _edit_context.get_edit_type() == editor_world_edit_type_e::full_control;
		}

		inline bool is_gizmo_scale_allowed() const
		{
			return _gizmo_callbacks.allow_scale;
		}

	private:
		bool get_gizmo_input(editor_gizmo_input_t& input) const;
		void build_camera_view(world_renderer_view_t& out_view) const;
		void consume_entity_pick_result();

	private:
		editor_world_snapshot_data_t _snapshot_slots[world_renderer_t::SNAPSHOT_SLOT_COUNT] = {};

		world_renderer_t* _world_renderer = nullptr;

		editor_world_tick_callback_t _tick_callback			  = nullptr;
		void*						 _tick_callback_user_data = nullptr;
		editor_world_camera_t*		 _camera				  = nullptr;
		editor_world_camera_type_e	 _camera_type			  = editor_world_camera_type_e::fly;

		editor_world_render_context_t _render_context = {};

		editor_world_entity_gizmo_t _entity_gizmo	 = {};
		editor_gizmo_callbacks_t	_gizmo_callbacks = {};

		editor_world_gizmo_t			_gizmo										 = {};
		editor_world_edit_context_t		_edit_context								 = {};
		editor_world_input_controller_t _input_controller							 = {};
		world_t							_world										 = {};
		atomic_t<u64>					_pick_result								 = {};
		editor_world_pick_request_t		_pending_pick_request						 = {};
		u32								_last_render_pick_request_id				 = 0;
		u32								_next_pick_request_id						 = 0;
		quat_t							_view_rotation								 = quat_t::identity;
		vec2u16_t						_render_resolution							 = vec2u16_t::zero;
		bool							_object_id_readback_valid[BACK_BUFFER_COUNT] = {};
		bool							_custom_gizmo								 = false;
	};
}
