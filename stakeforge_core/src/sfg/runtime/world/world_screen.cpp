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

#include "world_screen.hpp"
#include "world_util.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/runtime/render/render_view.hpp>

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
		_camera		   = {};
		_view_proj	   = mat4x4_t::identity;
		_inv_view_proj = mat4x4_t::identity;
		_input_rect	   = vec4f_t::zero;
		_render_size   = vec2u16_t::zero;
		_dpi_scale	   = 1.0f;
		_has_camera	   = false;
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
