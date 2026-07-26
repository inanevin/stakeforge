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

#include "world/editor_world_handle.hpp"
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class editor_world_t;
	struct editor_payload_t;
	struct editor_world_camera_input_t;
	struct window_event_t;
	struct window_runtime_t;

	enum class editor_world_input_pointer_button_e : u8
	{
		left,
		right,
	};

	class editor_world_input_controller_t final
	{
	public:
		editor_world_input_controller_t()												   = default;
		~editor_world_input_controller_t()												   = default;
		editor_world_input_controller_t(const editor_world_input_controller_t&)			   = delete;
		editor_world_input_controller_t& operator=(const editor_world_input_controller_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(editor_world_t& world, editor_world_handle_t handle);
		void uninit();
		void deactivate();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void		tick(const vec2f_t& relative_position, bool hovered);
		void		pointer_press(const vec2f_t& relative_position, editor_world_input_pointer_button_e button);
		void		pointer_release(const vec2f_t& relative_position, editor_world_input_pointer_button_e button, bool hovered);
		void		pointer_hover_move(const vec2f_t& relative_position);
		void		pointer_hover_exit();
		void		pointer_drag(const vec2f_t& relative_position);
		void		focus_lost();
		void		key_press(u16 key);
		void		wheel(f32 delta);
		void		hide_selection();
		void		hide(entity_id_t entity);
		void		toggle_show_alone();
		bool		payload_drop(const editor_payload_t& payload, const vec2f_t& screen_position);
		static bool on_window_event(window_runtime_t& runtime, const window_event_t& ev);
		static void reset_camera_input(window_runtime_t& runtime);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline u32 get_visibility_generation() const
		{
			return _visibility_generation;
		}

		inline bool is_show_alone_active() const
		{
			return _show_alone_active;
		}

	private:
		struct show_alone_entity_state_t
		{
			entity_guid_t entity   = NULL_ENTITY_GUID;
			bool		  disabled = false;
		};

		void begin_camera_control(window_runtime_t& runtime);
		void end_camera_control();
		void pass_camera_input(const editor_world_camera_input_t& input);
		bool pass_camera_key(const window_event_t& ev);
		void cancel_gizmo_action();
		void focus_selection();
		void hide(span_t<const entity_id_t> entities);

	private:
		vector_t<show_alone_entity_state_t> _show_alone_states		  = {};
		editor_world_t*						_world					  = nullptr;
		window_runtime_t*					_camera_runtime			  = nullptr;
		editor_world_handle_t				_handle					  = {};
		u32									_visibility_generation	  = 0;
		bool								_show_alone_active		  = false;
		bool								_camera_control			  = false;
		bool								_gizmo_press_consumed	  = false;
		bool								_shoot_ray_press_consumed = false;

		static inline editor_world_input_controller_t* s_active_camera_controller = nullptr;
		static inline window_runtime_t*				   s_event_runtime			  = nullptr;
	};
}
