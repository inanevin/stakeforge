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

#include <sfg/common/size_definitions.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class world_t;

	enum class editor_world_camera_type_e : u8
	{
		fly,
		orbit,
	};

	struct editor_world_camera_input_t
	{
		vec3f_t direction_delta = vec3f_t::zero;
		vec2f_t mouse_delta		= vec2f_t::zero;
		f32		move_speed		= 0.0f;
		bool	set_move_speed	= false;
		bool	reset			= false;
	};

	class editor_world_camera_t
	{
	public:
		editor_world_camera_t()											= default;
		virtual ~editor_world_camera_t()								= default;
		editor_world_camera_t(const editor_world_camera_t&)				= delete;
		editor_world_camera_t& operator=(const editor_world_camera_t&)	= delete;
		editor_world_camera_t(editor_world_camera_t&& other)			= delete;
		editor_world_camera_t& operator=(editor_world_camera_t&& other) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		virtual void init(world_t& world)	= 0;
		virtual void uninit(world_t& world) = 0;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		virtual void pass_input(const editor_world_camera_input_t& input) = 0;
		virtual void tick(world_t& world, f32 dt_seconds)				  = 0;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		virtual entity_id_t get_entity() const = 0;
	};
}
