/*
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

#include "gui/vekt_defines.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "math/matrix4x4.hpp"
#include "world/world_constants.hpp"

namespace vekt
{
	class builder;
}

namespace SFG
{
	class vector2;
	class world_screen;
	struct window_event;

	enum class gizmo_axis
	{
		none,
		x,
		y,
		z,
		uniform
	};

	enum class gizmo_style
	{
		move,
		rotate,
		scale
	};

	enum class gizmo_space
	{
		global,
		local
	};

	struct gizmo_draw_context
	{
		vector2		 root_pos	   = vector2::zero;
		vector2		 root_size	   = vector2::zero;
		matrix4x4	 view		   = matrix4x4::identity;
		vector2		 center_screen = vector2::zero;
		vector3		 entity_pos	   = vector3::zero;
		quat		 entity_rot	   = quat::identity;
		quat		 cam_rot	   = quat::identity;
		vector3		 cam_pos	   = vector3::zero;
		f32		 fov		   = 0.0f;
		world_handle selected	   = {};
		world_screen* screen	   = nullptr;
	};

	class editor_gizmo_controls
	{
	public:
		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(vekt::builder* builder);
		void draw(const vector2& root_pos, const vector2& root_size, const vector2& game_render_size);
		bool on_mouse_event(const window_event& ev);
		bool on_mouse_move(const vector2& pos);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline void set_style(gizmo_style s)
		{
			_style = s;
		}

		inline void set_active_axis(gizmo_axis ax)
		{
			_active_axis = ax;
		}

		inline void set_space(gizmo_space space)
		{
			_space = space;
		}

		inline gizmo_style get_style() const
		{
			return _style;
		}

		inline gizmo_space get_space() const
		{
			return _space;
		}

		inline gizmo_axis get_active_axis() const
		{
			return _active_axis;
		}

	private:
		bool get_context_selected(gizmo_draw_context& out);

	private:
		vekt::builder* _builder				  = nullptr;
		gizmo_style	   _style				  = gizmo_style::move;
		gizmo_axis	   _active_axis			  = gizmo_axis::none;
		gizmo_axis	   _hovered_axis		  = gizmo_axis::none;
		gizmo_space	   _space				  = gizmo_space::global;
		vector2		   _last_root_pos		  = vector2::zero;
		vector2		   _last_root_size		  = vector2::zero;
		vector2		   _last_game_render_size = vector2::zero;
		vector2		   _drag_last_mouse		  = vector2::zero;
		vector2		   _drag_start_mouse	  = vector2::zero;
		vector2		   _angle_start_dir		  = vector2::zero;
		vector2		   _angle_current_dir	  = vector2::zero;
		vector3		   _drag_start_pos		  = vector3::zero;
		quat		   _drag_start_rot		  = quat::identity;
		vector3		   _drag_start_scale	  = vector3::one;
		vector3		   _drag_offset			  = vector3::zero;
		f32		   _drag_amount			  = 0.0f;
	};
}
