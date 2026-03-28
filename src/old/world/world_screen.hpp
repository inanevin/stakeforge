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

#include "common/size_definitions.hpp"
#include "data/atomic.hpp"
#include "math/matrix4x4.hpp"
#include "math/vector2.hpp"
#include "math/vector2ui16.hpp"
#include "math/vector3.hpp"
#include "data/atomic.hpp"

namespace SFG
{
	class world;

	class world_screen
	{
	public:
		struct camera_snapshot
		{
			matrix4x4 view_proj		= matrix4x4::identity;
			matrix4x4 inv_view_proj = matrix4x4::identity;
			vector3	  cam_pos		= vector3::zero;
			u8		  valid			= 0;
		};

	public:
		void			   set_world_resolution(const vector2ui16& res);
		const vector2ui16& get_world_resolution() const;

		void fetch_camera_data(world& w);

		void set_camera_data(const matrix4x4& view_proj, const matrix4x4& inv_view_proj, const vector3& cam_pos);

		bool world_to_screen(const vector3& world_pos, vector2& out) const;
		bool world_to_screen(const vector3& world_pos, vector2& out, f32& out_distance) const;
		bool world_to_screen_render_thread(const vector3& world_pos, vector2& out) const;
		bool world_to_screen_render_thread(const vector3& world_pos, vector2& out, f32& out_distance) const;
		bool screen_to_world(const vector2& screen_pos, vector3& out_origin, vector3& out_dir) const;
		bool screen_to_world(const vector2& screen_pos, vector3& out_pos, f32 distance) const;
		bool screen_to_world_render_thread(const vector2& screen_pos, vector3& out_origin, vector3& out_dir) const;
		bool screen_to_world_render_thread(const vector2& screen_pos, vector3& out_pos, f32 distance) const;

	private:
		bool get_camera_snapshot(camera_snapshot& out) const;

	private:
		vector2ui16 _world_resolution  = vector2ui16::zero;
		matrix4x4	_cam_view_proj	   = {};
		matrix4x4	_cam_inv_view_proj = {};
		vector3		_cam_pos		   = vector3::zero;

		camera_snapshot _snapshots[3]	 = {};
		atomic<i8>		_snapshot_latest = -1;
		atomic<i8>		_snapshot_in_use = -1;
		u8				_snapshot_write	 = 0;
	};
}
