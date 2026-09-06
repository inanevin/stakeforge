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

#include "world/editor_transform.hpp"
#include <sfg/math/mat4x3.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec2u16.hpp>

namespace sfg
{
	class color_t;
	class world_debug_draw_t;
	struct world_ray_t;
	struct world_render_view_t;

	enum class editor_gizmo_axis_e : u8
	{
		x,
		y,
		z,
		central,
		xy,
		yz,
		zx,
		invalid,
	};

	struct editor_gizmo_target_t
	{
		quat_t	rotation	  = quat_t::identity;
		quat_t	prev_rotation = quat_t::identity;
		vec3f_t position	  = vec3f_t::zero;
		vec3f_t prev_position = vec3f_t::zero;
		u32		generation	  = 0;
	};

	struct editor_gizmo_callbacks_t
	{
		bool (*get_target)(void* user_data, editor_gizmo_target_t& target) = nullptr;
		bool (*begin)(void* user_data)									   = nullptr;
		void (*update)(void* user_data, const mat4x3_t& delta)			   = nullptr;
		void (*commit)(void* user_data)									   = nullptr;
		void (*cancel)(void* user_data)									   = nullptr;
		void* user_data													   = nullptr;
		bool  allow_scale												   = true;
	};

	struct editor_gizmo_input_t
	{
		editor_gizmo_target_t			target		   = {};
		const world_render_view_t*		view		   = nullptr;
		vec2u16_t						resolution	   = vec2u16_t::zero;
		f32								snap_translate = 0.0f;
		f32								snap_rotate	   = 0.0f;
		f32								snap_scale	   = 0.0f;
		editor_transform_control_type_e control_type   = editor_transform_control_type_e::invalid;
		editor_transform_locality_e		locality	   = editor_transform_locality_e::world;
	};

	class editor_world_gizmo_t final
	{
	public:
		static inline constexpr f32 PIXEL_SIZE			  = 90.0f;
		static inline constexpr f32 MIN_WORLD_SIZE		  = 0.05f;
		static inline constexpr f32 HIT_RADIUS_PX		  = 8.0f;
		static inline constexpr f32 CENTRAL_SIZE		  = 0.15f;
		static inline constexpr f32 CENTRAL_HIT_RADIUS_PX = 9.0f;
		static inline constexpr f32 PLANE_CENTER		  = 0.30f;
		static inline constexpr f32 PLANE_SIZE			  = 0.18f;
		static inline constexpr f32 PLANE_THICKNESS		  = 0.0125f;

		editor_world_gizmo_t()										 = default;
		~editor_world_gizmo_t()										 = default;
		editor_world_gizmo_t(const editor_world_gizmo_t&)			 = delete;
		editor_world_gizmo_t& operator=(const editor_world_gizmo_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void update_hover(const editor_gizmo_input_t& input, vec2f_t relative_position);
		void clear_hover();
		bool begin_action(const editor_gizmo_input_t& input, const editor_gizmo_callbacks_t& callbacks, vec2f_t relative_position);
		void update_action(const editor_gizmo_input_t& input, vec2f_t relative_position);
		void end_action(const editor_gizmo_input_t& input);
		void cancel_action();
		void draw_rotation_visualization(world_debug_draw_t& debug_draw, const color_t& line_color, const color_t& text_color, f32 text_size_px) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline editor_gizmo_axis_e get_hovered_axis() const
		{
			return _hovered_axis;
		}

		inline editor_gizmo_axis_e get_active_axis() const
		{
			return _active_axis;
		}

		inline bool is_action_active() const
		{
			return _action_active;
		}

	private:
		struct frame_t;
		struct hit_t;

		bool  calculate_frame(const editor_gizmo_input_t& input, frame_t& out_frame) const;
		hit_t pick(const frame_t& frame, editor_transform_control_type_e control_type, vec2f_t relative_position) const;
		bool  project_point(const frame_t& frame, const vec3f_t& point, vec2f_t& out_position) const;
		bool  calculate_axis_parameter(const world_ray_t& ray, const vec3f_t& pivot, const vec3f_t& axis, f32& out_parameter) const;
		bool  calculate_rotation_direction(const world_ray_t& ray, const vec3f_t& pivot, const vec3f_t& axis, vec3f_t& out_direction) const;
		bool  calculate_plane_point(const world_ray_t& ray, const vec3f_t& pivot, const vec3f_t& normal, vec3f_t& out_point) const;
		void  clear_action();

	private:
		editor_gizmo_callbacks_t		_callbacks					= {};
		quat_t							_orientation				= quat_t::identity;
		vec3f_t							_initial_rotation_direction = vec3f_t::zero;
		vec3f_t							_initial_plane_point		= vec3f_t::zero;
		vec3f_t							_central_plane_normal		= vec3f_t::zero;
		vec3f_t							_central_plane_right		= vec3f_t::zero;
		vec3f_t							_central_plane_up			= vec3f_t::zero;
		vec3f_t							_axis_world					= vec3f_t::zero;
		vec3f_t							_pivot						= vec3f_t::zero;
		vec2f_t							_initial_mouse_pixels		= vec2f_t::zero;
		vec2f_t							_rotation_screen_tangent	= vec2f_t::zero;
		vec2f_t							_axis_screen_direction		= vec2f_t::zero;
		f32								_initial_axis_parameter		= 0.0f;
		f32								_axis_pixels_per_world		= 0.0f;
		f32								_world_scale				= 0.0f;
		f32								_rotation_angle_degrees		= 0.0f;
		f32								_snap_translate				= 0.0f;
		f32								_snap_rotate				= 0.0f;
		f32								_snap_scale					= 0.0f;
		u32								_target_generation			= 0;
		editor_transform_control_type_e _control_type				= {};
		editor_transform_locality_e		_locality					= {};
		editor_gizmo_axis_e				_hovered_axis				= editor_gizmo_axis_e::invalid;
		editor_gizmo_axis_e				_active_axis				= editor_gizmo_axis_e::invalid;
		bool							_axis_parameter_valid		= false;
		bool							_rotation_plane_valid		= false;
		bool							_central_plane_valid		= false;
		bool							_action_active				= false;
	};
}
