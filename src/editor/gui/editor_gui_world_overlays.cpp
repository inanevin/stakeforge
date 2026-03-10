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

#include "editor_gui_world_overlays.hpp"
#include "editor/editor_theme.hpp"
#include "editor/editor.hpp"
#include "math/vector2ui16.hpp"
#include "math/vector2.hpp"
#include "app/app.hpp"
// gfx
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/renderer.hpp"

// vekt
#include "gui/vekt.hpp"

namespace SFG
{
	void editor_gui_world_overlays::init(vekt::builder* builder)
	{
		_builder = builder;
	}

	void editor_gui_world_overlays::draw(const vector2ui16& size)
	{
		proxy_manager& pm = editor::get().get_app().get_renderer().get_proxy_manager();

		const world_id cam_id = pm.get_main_camera();
		if (cam_id == NULL_WORLD_ID)
			return;

		const render_proxy_camera& cam		  = pm.get_camera(cam_id);
		const render_proxy_entity& cam_entity = pm.get_entity(cam.entity);
		if (cam_entity.status != render_proxy_status::rps_active)
			return;

		const vector3 cam_forward = cam_entity.rotation.get_forward();

		const vector3 x_cam = (cam_entity.rotation.inverse() * vector3::right).normalized();
		const vector2 x_dir = vector2(x_cam.x, -x_cam.y);

		const vector3 y_cam = (cam_entity.rotation.inverse() * vector3::up).normalized();
		const vector2 y_dir = vector2(y_cam.x, -y_cam.y);

		const vector3 z_cam = (cam_entity.rotation.inverse() * vector3::forward).normalized();
		const vector2 z_dir = vector2(z_cam.x, -z_cam.y);

		constexpr f32 box_size_multiplier = 0.03f;
		constexpr f32 thickness			= 2.0f;

		const vector2 box_size = vector2(static_cast<f32>(size.x) * box_size_multiplier, static_cast<f32>(size.x) * box_size_multiplier);
		const vector2 min	   = vector2(editor_theme::get().outer_margin + box_size.x * 0.5f, size.y - editor_theme::get().outer_margin - box_size.y);
		const vector2 max	   = min + box_size;
		const vector2 center   = (min + max) * 0.5f;

		_builder->add_line_aa({
			.p0			  = vector2(center),
			.p1			  = center + x_dir * box_size.x,
			.color		  = editor_theme::get().color_axis_x,
			.thickness	  = thickness,
			.aa_thickness = 2,
		});

		_builder->add_line_aa({
			.p0			  = vector2(center),
			.p1			  = center + y_dir * box_size.x,
			.color		  = editor_theme::get().color_axis_y,
			.thickness	  = thickness,
			.aa_thickness = 2,
		});

		_builder->add_line_aa({
			.p0			  = vector2(center),
			.p1			  = center + z_dir * box_size.x,
			.color		  = editor_theme::get().color_axis_z,
			.thickness	  = thickness,
			.aa_thickness = 2,
		});
	}
}