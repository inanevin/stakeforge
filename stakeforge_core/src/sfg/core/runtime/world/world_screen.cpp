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

#include "core/runtime/world/world_screen.hpp"
#include "core/runtime/world/world_util.hpp"

#include "core/io/assert.hpp"
#include "core/platform/common_window.hpp"
#include "core/platform/process.hpp"
#include "core/runtime/render/render_view.hpp"

namespace sfg
{
	void world_screen_t::init(vec2u16_t render_size)
	{
		SFG_ASSERT(render_size.x > 0 && render_size.y > 0);

		_render_size = render_size;
		_input_rect	 = {0.0f, 0.0f, static_cast<f32>(render_size.x), static_cast<f32>(render_size.y)};
	}

	void world_screen_t::uninit()
	{
		reset_cursor();

		_camera		   = {};
		_view_proj	   = mat4x4_t::identity;
		_inv_view_proj = mat4x4_t::identity;
		_input_rect	   = vec4f_t::zero;
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
		const bool					visible		= !enabled || _cursor_visible;

		if (confinement == window_cursor_confinement_e::pointer)
		{
			if (_cursor_position_pending)
			{
				_cursor_confinement_position = runtime.mouse_position;
				_cursor_position_pending	 = false;
			}

			confinement = window_cursor_confinement_e::position;
		}

		if (confinement == window_cursor_confinement_e::position)
		{
			if (runtime.cursor_confinement != confinement || runtime.cursor_confinement_position != _cursor_confinement_position)
				process::set_cursor_confinement_position(runtime.window_handle, _cursor_confinement_position);
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

		_input_rect = input_rect;
		_dpi_scale	= dpi_scale;

		set_render_size(render_size);
	}

	void world_screen_t::set_render_size(vec2u16_t render_size)
	{
		SFG_ASSERT(render_size.x > 0 && render_size.y > 0);

		if (_render_size == render_size)
			return;

		_render_size = render_size;

		if (_has_camera)
			update_camera(_camera);
	}

	void world_screen_t::update_camera(const world_render_view_t& camera)
	{
		render_view_t render_view = {};
		render_view.calculate(camera, _render_size, 1.0f);

		_camera		   = camera;
		_view_proj	   = render_view.view_proj;
		_inv_view_proj = render_view.inv_view_proj;
		_has_camera	   = true;
	}

	void world_screen_t::clear_camera()
	{
		_camera		   = {};
		_view_proj	   = mat4x4_t::identity;
		_inv_view_proj = mat4x4_t::identity;
		_has_camera	   = false;
	}

	bool world_screen_t::world_to_screen(const vec3f_t& world_position, vec2f_t& out_screen_position) const
	{
		if (!_has_camera)
			return false;

		vec2f_t relative_position = vec2f_t::zero;

		if (!world_util_t::world_position_to_relative_position(_view_proj, world_position, relative_position))
			return false;

		out_screen_position = {
			_input_rect.x + relative_position.x * _input_rect.z,
			_input_rect.y + relative_position.y * _input_rect.w,
		};

		return true;
	}

	bool world_screen_t::screen_to_world(const vec2f_t& screen_position, f32 ndc_depth, vec3f_t& out_world_position) const
	{
		if (!_has_camera)
			return false;

		const vec2f_t relative_position = {
			(screen_position.x - _input_rect.x) / _input_rect.z,
			(screen_position.y - _input_rect.y) / _input_rect.w,
		};

		return world_util_t::relative_position_to_world_position(_inv_view_proj, relative_position, ndc_depth, out_world_position);
	}

	vec2f_t world_screen_t::screen_to_render_position(const vec2f_t& screen_position) const
	{
		return {
			(screen_position.x - _input_rect.x) * static_cast<f32>(_render_size.x) / _input_rect.z,
			(screen_position.y - _input_rect.y) * static_cast<f32>(_render_size.y) / _input_rect.w,
		};
	}
}
