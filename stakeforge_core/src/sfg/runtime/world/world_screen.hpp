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
