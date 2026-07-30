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

#include <sfg/math/mat4x4.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/runtime/render/world_render_view.hpp>

namespace sfg
{
	class world_screen_t final
	{
	public:
		world_screen_t()								 = default;
		~world_screen_t()								 = default;
		world_screen_t(const world_screen_t&)			 = delete;
		world_screen_t& operator=(const world_screen_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(vec2u16_t render_size);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void	set_viewport(const vec4f_t& input_rect, vec2u16_t render_size, f32 dpi_scale);
		void	set_render_size(vec2u16_t render_size);
		void	update_camera(const world_render_view_t& camera);
		void	clear_camera();
		bool	world_to_screen(const vec3f_t& world_position, vec2f_t& out_screen_position) const;
		bool	screen_to_world(const vec2f_t& screen_position, f32 ndc_depth, vec3f_t& out_world_position) const;
		vec2f_t screen_to_render_position(const vec2f_t& screen_position) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const vec4f_t& get_input_rect() const
		{
			return _input_rect;
		}

		inline vec2u16_t get_render_size() const
		{
			return _render_size;
		}

		inline f32 get_dpi_scale() const
		{
			return _dpi_scale;
		}

		inline bool is_camera_valid() const
		{
			return _has_camera;
		}

	private:
		world_render_view_t _camera		   = {};
		mat4x4_t			_view_proj	   = mat4x4_t::identity;
		mat4x4_t			_inv_view_proj = mat4x4_t::identity;
		vec4f_t				_input_rect	   = vec4f_t::zero;
		vec2u16_t			_render_size   = vec2u16_t::zero;
		f32					_dpi_scale	   = 1.0f;
		bool				_has_camera	   = false;
	};
}
