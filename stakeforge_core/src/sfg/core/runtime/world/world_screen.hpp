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

#include "core/common/sfg_exports.hpp"

#include "core/math/mat4x4.hpp"
#include "core/math/vec2f.hpp"
#include "core/math/vec2i16.hpp"
#include "core/math/vec2u16.hpp"
#include "core/math/vec4f.hpp"

namespace sfg
{
	struct window_runtime_t;
	struct world_renderer_view_t;
	enum class window_cursor_confinement_e : u8;

	struct world_ray_t
	{
		vec3f_t origin	  = vec3f_t::zero;
		vec3f_t direction = vec3f_t::forward;
	};

	class SFG_API world_screen_t final
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
		void	update_camera(const world_renderer_view_t& view);
		void	clear_camera();
		bool	world_to_screen(const vec3f_t& world_position, vec2f_t& out_screen_position) const;
		bool	screen_to_world(const vec2f_t& screen_position, f32 ndc_depth, vec3f_t& out_world_position) const;
		vec2f_t screen_to_render_position(const vec2f_t& screen_position) const;
		void	set_cursor_confinement(window_cursor_confinement_e confinement);
		void	set_cursor_confinement_position(const vec2i16_t& position);
		void	set_cursor_visible(bool visible);
		void	reset_cursor();
		void	apply_cursor(window_runtime_t& runtime, bool enabled = true);

		static bool world_position_to_relative_position(const mat4x4_t& view_proj, const vec3f_t& world_position, vec2f_t& out_relative_position);
		static bool relative_position_to_world_position(const mat4x4_t& inv_view_proj, const vec2f_t& relative_position, f32 ndc_depth, vec3f_t& out_world_position);
		static bool relative_position_to_world_ray(const mat4x4_t& inv_view_proj, const vec2f_t& relative_position, world_ray_t& out_ray);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const vec4f_t& get_input_rect() const
		{
			return _viewport_rect;
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

		inline window_cursor_confinement_e get_cursor_confinement() const
		{
			return _cursor_confinement;
		}

		inline const vec2i16_t& get_cursor_confinement_position() const
		{
			return _cursor_confinement_position;
		}

		inline bool is_cursor_visible() const
		{
			return _cursor_visible;
		}

	private:
		mat4x4_t					_view_proj					 = mat4x4_t::identity;
		mat4x4_t					_inv_view_proj				 = mat4x4_t::identity;
		vec4f_t						_viewport_rect				 = vec4f_t::zero;
		vec2i16_t					_cursor_confinement_position = vec2i16_t::zero;
		vec2u16_t					_render_size				 = vec2u16_t::zero;
		f32							_dpi_scale					 = 1.0f;
		bool						_has_camera					 = false;
		window_cursor_confinement_e _cursor_confinement			 = {};
		bool						_cursor_visible				 = true;
		bool						_cursor_position_pending	 = false;
	};
}
