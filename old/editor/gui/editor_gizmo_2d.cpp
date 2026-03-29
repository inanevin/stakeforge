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

#include "editor_gizmo_2d.hpp"
#include "editor/editor.hpp"
#include "editor/editor_theme.hpp"
#include "editor/gui/editor_panel_entities.hpp"
#include "world/common_entity.hpp"
#include "gui/icon_defs.hpp"
#include "gui/vekt.hpp"
#include "app/app.hpp"
#include "math/color.hpp"
#include "world/world.hpp"
#include "world/components/comp_light.hpp"
#include "world/components/comp_particle_emitter.hpp"
#include "world/components/comp_sprite.hpp"
#include "world/components/comp_audio.hpp"
#include "world/components/comp_camera.hpp"

namespace SFG
{

	namespace
	{
		color to_color(const vector4& v)
		{
			return color(v.x, v.y, v.z, v.w);
		}
	}

	void editor_gizmo_2d::init(vekt::builder* builder)
	{
		_builder = builder;
		_icons.reserve(256);
	}

	void editor_gizmo_2d::draw(const vector2& root_pos, const vector2& root_size, const vector2& game_render_size)
	{
		(void)game_render_size;
		world&				   w			= editor::get().get_app().get_world();
		entity_manager&		   em			= w.get_entity_manager();
		component_manager&	   cm			= w.get_comp_manager();
		world_debug_rendering& wdr			= w.get_debug_rendering();
		const color			   col_text		= to_color(editor_theme::get().col_accent);
		const color			   col_template = to_color(editor_theme::get().col_accent_third);

		_last_root_pos	= root_pos;
		_last_root_size = root_size;
		_icons.resize(0);

		const world_handle selected = editor::get().get_gui_controller().get_entities()->get_selected();

		cm.view<comp_point_light>([&](comp_point_light& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;

			const vector3 pos = em.get_entity_position_abs(c.get_header().entity);
			wdr.draw_icon(pos, col_text, ICON_LIGHT_BULB);
			_icons.push_back({.entity = c.get_header().entity, .pos = pos, .icon = ICON_LIGHT_BULB});
			return comp_view_result::cont;
		});

		cm.view<comp_spot_light>([&](comp_spot_light& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;

			const vector3 pos = em.get_entity_position_abs(c.get_header().entity);
			wdr.draw_icon(pos, col_text, ICON_SPOT);
			_icons.push_back({.entity = c.get_header().entity, .pos = pos, .icon = ICON_SPOT});
			return comp_view_result::cont;
		});

		cm.view<comp_dir_light>([&](comp_dir_light& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;

			const vector3 pos = em.get_entity_position_abs(c.get_header().entity);
			wdr.draw_icon(pos, col_text, ICON_SUN);
			_icons.push_back({.entity = c.get_header().entity, .pos = pos, .icon = ICON_SUN});
			return comp_view_result::cont;
		});

		cm.view<comp_particle_emitter>([&](comp_particle_emitter& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;

			const vector3 pos = em.get_entity_position_abs(c.get_header().entity);
			wdr.draw_icon(pos, col_text, ICON_EXPLOSION);
			_icons.push_back({.entity = c.get_header().entity, .pos = pos, .icon = ICON_EXPLOSION});
			return comp_view_result::cont;
		});

		cm.view<comp_sprite>([&](comp_sprite& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;

			const vector3 pos = em.get_entity_position_abs(c.get_header().entity);
			wdr.draw_icon(pos, col_text, ICON_GUI);
			_icons.push_back({.entity = c.get_header().entity, .pos = pos, .icon = ICON_GUI});
			return comp_view_result::cont;
		});

		cm.view<comp_audio>([&](comp_audio& c) {
			if (c.get_header().entity == selected)
				return comp_view_result::cont;
			const vector3 pos = em.get_entity_position_abs(c.get_header().entity);
			wdr.draw_icon(pos, col_text, ICON_AUDIO);
			_icons.push_back({.entity = c.get_header().entity, .pos = pos, .icon = ICON_AUDIO});
			return comp_view_result::cont;
		});

		const world_handle editor_camera = editor::get().get_camera().get_entity();

		cm.view<comp_camera>([&](comp_camera& c) {
			const world_handle e = c.get_header().entity;

			if (e == selected || e == editor_camera)
				return comp_view_result::cont;

			const vector3 pos = em.get_entity_position_abs(c.get_header().entity);
			wdr.draw_icon(pos, col_text, ICON_CAMERA);
			_icons.push_back({.entity = c.get_header().entity, .pos = pos, .icon = ICON_CAMERA});
			return comp_view_result::cont;
		});

		auto* pool = em.get_entities();
		for (auto it = pool->handles_begin(); it != pool->handles_end(); ++it)
		{
			const world_handle handle = *it;
			if (handle == selected)
				continue;

			if (!em.get_entity_flags(handle).is_set(entity_flags_template))
				continue;

			const vector3 pos = em.get_entity_position_abs(handle);
			wdr.draw_icon(pos, col_template, ICON_HAMMER);
			_icons.push_back({.entity = handle, .pos = pos, .icon = ICON_HAMMER});
		}
	}

	bool editor_gizmo_2d::on_mouse_event(const window_event& ev)
	{
		if (ev.type != window_event_type::mouse || ev.sub_type != window_event_sub_type::press)
			return false;

		if (_last_root_size.x <= 0.0f || _last_root_size.y <= 0.0f)
			return false;

		const vector2 mp = vector2(static_cast<f32>(ev.value.x), static_cast<f32>(ev.value.y)) - _last_root_pos;
		if (mp.x < 0 || mp.y < 0 || mp.x > _last_root_size.x || mp.y > _last_root_size.y)
			return false;

		editor_panel_entities* entities = editor::get().get_gui_controller().get_entities();
		if (!entities)
			return false;

		world&		  w		 = editor::get().get_app().get_world();
		world_screen& screen = w.get_screen();

		vekt::text_props tp = {};
		tp.font				= editor_theme::get().font_icons;
		tp.scale			= 1.0f;

		for (const icon_entry& icon : _icons)
		{
			vector2 screen_pos = vector2::zero;
			f32		distance   = 0.0f;

			if (!screen.world_to_screen(icon.pos, screen_pos, distance))
				continue;

			tp.text = icon.icon;

			f32			  multip	= 25.0f / distance;
			const vector2 text_size = vekt::builder::get_text_size(tp) * multip;
			const vector2 rect_min	= vector2(screen_pos.x - text_size.x * 0.5f, screen_pos.y - text_size.y * 0.5f);
			const vector2 rect_max	= rect_min + text_size;

			if (mp.x >= rect_min.x && mp.x <= rect_max.x && mp.y >= rect_min.y && mp.y <= rect_max.y)
			{
				editor::get().get_gui_controller().set_selected_entity(icon.entity);
				return true;
			}
		}

		return false;
	}
}
