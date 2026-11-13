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

#include "world_screen.hpp"

#include "core/io/assert.hpp"
#include "core/math/math.hpp"
#include "core/platform/common_window.hpp"
#include "core/platform/process.hpp"
#include "core/runtime/world_renderer/world_renderer_view.hpp"

namespace sfg
{
	void world_screen_t::init(vec2u16_t render_size)
	{
		SFG_ASSERT(render_size.x > 0 && render_size.y > 0);

		_render_size   = render_size;
		_viewport_rect = {0.0f, 0.0f, static_cast<f32>(render_size.x), static_cast<f32>(render_size.y)};
	}

	void world_screen_t::uninit()
	{
		reset_cursor();

		_view_proj	   = mat4x4_t::identity;
		_inv_view_proj = mat4x4_t::identity;
		_viewport_rect = vec4f_t::zero;
		_render_size   = vec2u16_t::zero;
		_dpi_scale	   = 1.0f;
		_has_camera	   = false;
	}

	void world_screen_t::set_cursor_confinement(window_cursor_confinement_e confinement)
	{
		if (_cursor_confinement == confinement)
			return;

		_cursor_confinement		 = confinement;
		_cursor_position_pending = confinement == window_cursor_confinement_e::pointer;
	}

	void world_screen_t::set_cursor_confinement_position(const vec2i16_t& position)
	{
		_cursor_confinement			 = window_cursor_confinement_e::position;
		_cursor_confinement_position = position;
		_cursor_position_pending	 = false;
	}

	void world_screen_t::set_cursor_visible(bool visible)
	{
		_cursor_visible = visible;
	}

	void world_screen_t::reset_cursor()
	{
		_cursor_confinement			 = window_cursor_confinement_e::none;
		_cursor_confinement_position = vec2i16_t::zero;
		_cursor_visible				 = true;
		_cursor_position_pending	 = false;
	}

	void world_screen_t::apply_cursor(window_runtime_t& runtime, bool enabled)
	{
		window_cursor_confinement_e confinement = enabled ? _cursor_confinement : window_cursor_confinement_e::none;
		vec2i16_t					confinement_position = _cursor_confinement_position;
		const bool					visible		= !enabled || _cursor_visible;

		if (confinement == window_cursor_confinement_e::center)
		{
			confinement_position = {
				static_cast<i16>(_viewport_rect.x + _viewport_rect.z * 0.5f),
				static_cast<i16>(_viewport_rect.y + _viewport_rect.w * 0.5f),
			};
			confinement = window_cursor_confinement_e::position;
		}
		else if (confinement == window_cursor_confinement_e::pointer)
		{
			if (_cursor_position_pending)
			{
				_cursor_confinement_position = runtime.mouse_position;
				_cursor_position_pending	 = false;
			}

			confinement_position = _cursor_confinement_position;
			confinement = window_cursor_confinement_e::position;
		}

		if (confinement == window_cursor_confinement_e::position)
		{
			if (runtime.cursor_confinement != confinement || runtime.cursor_confinement_position != confinement_position)
				process::set_cursor_confinement_position(runtime.window_handle, confinement_position);
		}
		else if (runtime.cursor_confinement != confinement)
			process::set_cursor_confinement(runtime.window_handle, confinement);

		if (runtime.cursor_visible != visible)
			process::set_cursor_visible(runtime.window_handle, visible);
	}

	void world_screen_t::set_viewport(const vec4f_t& input_rect, vec2u16_t render_size, f32 dpi_scale)
	{
		SFG_ASSERT(input_rect.z > 0.0f && input_rect.w > 0.0f);
		SFG_ASSERT(render_size.x > 0 && render_size.y > 0);
		SFG_ASSERT(dpi_scale > 0.0f);

		_viewport_rect = input_rect;
		_dpi_scale	   = dpi_scale;

		set_render_size(render_size);
	}

	void world_screen_t::set_render_size(vec2u16_t render_size)
	{
		SFG_ASSERT(render_size.x > 0 && render_size.y > 0);

		if (_render_size == render_size)
			return;

		_render_size = render_size;
	}

	void world_screen_t::update_camera(const world_renderer_view_t& view)
	{
		_view_proj	   = view.view_proj;
		_inv_view_proj = view.inv_view_proj;
		_has_camera	   = true;
	}

	void world_screen_t::clear_camera()
	{
		_view_proj	   = mat4x4_t::identity;
		_inv_view_proj = mat4x4_t::identity;
		_has_camera	   = false;
	}

	bool world_screen_t::world_to_screen(const vec3f_t& world_position, vec2f_t& out_screen_position) const
	{
		if (!_has_camera)
			return false;

		vec2f_t relative_position = vec2f_t::zero;

		if (!world_position_to_relative_position(_view_proj, world_position, relative_position))
			return false;

		out_screen_position = {
			_viewport_rect.x + relative_position.x * _viewport_rect.z,
			_viewport_rect.y + relative_position.y * _viewport_rect.w,
		};

		return true;
	}

	bool world_screen_t::screen_to_world(const vec2f_t& screen_position, f32 ndc_depth, vec3f_t& out_world_position) const
	{
		if (!_has_camera)
			return false;

		const vec2f_t relative_position = {
			(screen_position.x - _viewport_rect.x) / _viewport_rect.z,
			(screen_position.y - _viewport_rect.y) / _viewport_rect.w,
		};

		return relative_position_to_world_position(_inv_view_proj, relative_position, ndc_depth, out_world_position);
	}

	vec2f_t world_screen_t::screen_to_render_position(const vec2f_t& screen_position) const
	{
		return {
			(screen_position.x - _viewport_rect.x) * static_cast<f32>(_render_size.x) / _viewport_rect.z,
			(screen_position.y - _viewport_rect.y) * static_cast<f32>(_render_size.y) / _viewport_rect.w,
		};
	}

	bool world_screen_t::world_position_to_relative_position(const mat4x4_t& view_proj, const vec3f_t& world_position, vec2f_t& out_relative_position)
	{
		const vec4f_t clip = view_proj * vec4f_t(world_position.x, world_position.y, world_position.z, 1.0f);

		if (clip.w <= MATH_EPS)
			return false;

		const f32 inv_w = 1.0f / clip.w;

		out_relative_position = {
			clip.x * inv_w * 0.5f + 0.5f,
			0.5f - clip.y * inv_w * 0.5f,
		};

		return true;
	}

	bool world_screen_t::relative_position_to_world_position(const mat4x4_t& inv_view_proj, const vec2f_t& relative_position, f32 ndc_depth, vec3f_t& out_world_position)
	{
		const f32 ndc_x = relative_position.x * 2.0f - 1.0f;
		const f32 ndc_y = 1.0f - relative_position.y * 2.0f;
		vec4f_t	  world = inv_view_proj * vec4f_t(ndc_x, ndc_y, ndc_depth, 1.0f);

		if (math::abs(world.w) <= MATH_EPS)
			return false;

		world /= world.w;

		out_world_position = {world.x, world.y, world.z};

		return true;
	}

	bool world_screen_t::relative_position_to_world_ray(const mat4x4_t& inv_view_proj, const vec2f_t& relative_position, world_ray_t& out_ray)
	{
		if (!relative_position_to_world_position(inv_view_proj, relative_position, 1.0f, out_ray.origin))
			return false;

		const vec4f_t far_point = inv_view_proj * vec4f_t(relative_position.x * 2.0f - 1.0f, 1.0f - relative_position.y * 2.0f, 0.0f, 1.0f);

		out_ray.direction = vec3f_t(far_point.x - out_ray.origin.x * far_point.w, far_point.y - out_ray.origin.y * far_point.w, far_point.z - out_ray.origin.z * far_point.w).normalized();

		return !out_ray.direction.is_zero();
	}
}
