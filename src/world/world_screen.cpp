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
#include "math/math.hpp"
#include "math/vector4.hpp"
#include "world/world.hpp"
#include "world/components/comp_camera.hpp"

namespace SFG
{
	void world_screen::set_world_resolution(const vector2ui16& res)
	{
		_world_resolution = res;
	}

	const vector2ui16& world_screen::get_world_resolution() const
	{
		return _world_resolution;
	}

	void world_screen::fetch_camera_data(world& w)
	{
		entity_manager& em = w.get_entity_manager();

		const world_handle mc	   = em.get_main_camera_entity();
		const world_handle mc_comp = em.get_main_camera_comp();

		if (mc.is_null() || mc_comp.is_null())
			return;

		component_manager& cm		   = w.get_comp_manager();
		comp_camera&	   camera_comp = cm.get_component<comp_camera>(mc_comp);

		const f32 aspect = static_cast<f32>(_world_resolution.x) / static_cast<f32>(_world_resolution.y);
		_cam_pos		 = em.get_entity_position_abs(mc);

		const matrix4x4 view = matrix4x4::view(em.get_entity_rotation_abs(mc), _cam_pos);
		const matrix4x4 proj = matrix4x4::perspective_reverse_z(camera_comp.get_fov_degrees(), aspect, camera_comp.get_near(), camera_comp.get_far());
		_cam_view_proj		 = proj * view;
		_cam_inv_view_proj	 = _cam_view_proj.inverse();
	}

	void world_screen::set_camera_data(const matrix4x4& view_proj, const matrix4x4& inv_view_proj, const vector3& cam_pos)
	{
		const i8 in_use = _snapshot_in_use.load(std::memory_order_acquire);
		u8		 write	= _snapshot_write;
		if (static_cast<i8>(write) == in_use)
		{
			write = (write + 1) % 3;
			if (static_cast<i8>(write) == in_use)
				write = (write + 1) % 3;
		}

		camera_snapshot& snap = _snapshots[write];
		snap.view_proj		  = view_proj;
		snap.inv_view_proj	  = inv_view_proj;
		snap.cam_pos		  = cam_pos;
		snap.valid			  = 1;

		_snapshot_latest.store(static_cast<i8>(write), std::memory_order_release);
		_snapshot_write = (write + 1) % 3;
	}

	bool world_screen::get_camera_snapshot(camera_snapshot& out) const
	{
		const i8 idx = _snapshot_latest.load(std::memory_order_acquire);
		if (idx < 0)
			return false;

		//_snapshot_in_use.store(idx, std::memory_order_release);
		out = _snapshots[static_cast<u8>(idx)];
		//_snapshot_in_use.store(-1, std::memory_order_release);
		return out.valid != 0;
	}

	bool world_screen::world_to_screen(const vector3& world_pos, vector2& out) const
	{
		if (_world_resolution.x == 0 || _world_resolution.y == 0)
			return false;

		vector4 clip = _cam_view_proj * vector4(world_pos.x, world_pos.y, world_pos.z, 1.0f);
		if (math::abs(clip.w) < MATH_EPS)
			return false;

		if (clip.w < 0.0f)
			return false;

		const f32 inv_w = 1.0f / clip.w;
		const f32 ndc_x = clip.x * inv_w;
		const f32 ndc_y = clip.y * inv_w;

		out.x = (ndc_x * 0.5f + 0.5f) * static_cast<f32>(_world_resolution.x);
		out.y = (1.0f - (ndc_y * 0.5f + 0.5f)) * static_cast<f32>(_world_resolution.y);
		return true;
	}

	bool world_screen::world_to_screen(const vector3& world_pos, vector2& out, f32& out_distance) const
	{
		if (!world_to_screen(world_pos, out))
			return false;

		out_distance = (world_pos - _cam_pos).magnitude();

		return true;
	}

	bool world_screen::world_to_screen_render_thread(const vector3& world_pos, vector2& out) const
	{
		camera_snapshot snap = {};
		if (!get_camera_snapshot(snap))
			return false;

		if (_world_resolution.x == 0 || _world_resolution.y == 0)
			return false;

		vector4 clip = snap.view_proj * vector4(world_pos.x, world_pos.y, world_pos.z, 1.0f);
		if (math::abs(clip.w) < MATH_EPS)
			return false;

		if (clip.w < 0.0f)
			return false;

		const f32 inv_w = 1.0f / clip.w;
		const f32 ndc_x = clip.x * inv_w;
		const f32 ndc_y = clip.y * inv_w;

		out.x = (ndc_x * 0.5f + 0.5f) * static_cast<f32>(_world_resolution.x);
		out.y = (1.0f - (ndc_y * 0.5f + 0.5f)) * static_cast<f32>(_world_resolution.y);
		return true;
	}

	bool world_screen::world_to_screen_render_thread(const vector3& world_pos, vector2& out, f32& out_distance) const
	{
		camera_snapshot snap = {};
		if (!get_camera_snapshot(snap))
			return false;

		if (!world_to_screen_render_thread(world_pos, out))
			return false;

		out_distance = (world_pos - snap.cam_pos).magnitude();
		return true;
	}

	bool world_screen::screen_to_world(const vector2& screen_pos, vector3& out_origin, vector3& out_dir) const
	{
		if (_world_resolution.x == 0 || _world_resolution.y == 0)
			return false;

		const f32 nx = math::clamp(screen_pos.x / static_cast<f32>(_world_resolution.x), 0.0f, 1.0f);
		const f32 ny = math::clamp(screen_pos.y / static_cast<f32>(_world_resolution.y), 0.0f, 1.0f);

		const f32 ndc_x = nx * 2.0f - 1.0f;
		const f32 ndc_y = 1.0f - ny * 2.0f;

		const vector4 near_v = _cam_inv_view_proj * vector4(ndc_x, ndc_y, 0.0f, 1.0f);
		const vector4 far_v	 = _cam_inv_view_proj * vector4(ndc_x, ndc_y, 1.0f, 1.0f);

		const vector3 near_ws = vector3(near_v.x, near_v.y, near_v.z) / near_v.w;
		const vector3 far_ws  = vector3(far_v.x, far_v.y, far_v.z) / far_v.w;

		out_origin = _cam_pos;
		out_dir	   = (far_ws - near_ws).normalized();
		return true;
	}

	bool world_screen::screen_to_world(const vector2& screen_pos, vector3& out_pos, f32 distance) const
	{
		vector3 out_cam = vector3::zero;
		vector3 out_dir = vector3::zero;

		if (!screen_to_world(screen_pos, out_cam, out_dir))
			return false;

		out_pos = out_cam + out_dir * distance;
		return true;
	}

	bool world_screen::screen_to_world_render_thread(const vector2& screen_pos, vector3& out_origin, vector3& out_dir) const
	{
		camera_snapshot snap = {};
		if (!get_camera_snapshot(snap))
			return false;

		if (_world_resolution.x == 0 || _world_resolution.y == 0)
			return false;

		const f32 nx = math::clamp(screen_pos.x / static_cast<f32>(_world_resolution.x), 0.0f, 1.0f);
		const f32 ny = math::clamp(screen_pos.y / static_cast<f32>(_world_resolution.y), 0.0f, 1.0f);

		const f32 ndc_x = nx * 2.0f - 1.0f;
		const f32 ndc_y = 1.0f - ny * 2.0f;

		const vector4 near_v = snap.inv_view_proj * vector4(ndc_x, ndc_y, 0.0f, 1.0f);
		const vector4 far_v	 = snap.inv_view_proj * vector4(ndc_x, ndc_y, 1.0f, 1.0f);

		const vector3 near_ws = vector3(near_v.x, near_v.y, near_v.z) / near_v.w;
		const vector3 far_ws  = vector3(far_v.x, far_v.y, far_v.z) / far_v.w;

		out_origin = snap.cam_pos;
		out_dir	   = (far_ws - near_ws).normalized();
		return true;
	}

	bool world_screen::screen_to_world_render_thread(const vector2& screen_pos, vector3& out_pos, f32 distance) const
	{
		vector3 out_cam = vector3::zero;
		vector3 out_dir = vector3::zero;

		if (!screen_to_world_render_thread(screen_pos, out_cam, out_dir))
			return false;

		out_pos = out_cam + out_dir * distance;
		return true;
	}
}
