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

#include "editor_panels_world_view.hpp"
#include "math/vector2ui16.hpp"
#include "editor/editor.hpp"
#include "editor/gui/editor_panel_entities.hpp"
#include "editor/editor_settings.hpp"
#include "editor/editor_theme.hpp"
#include "app/app.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/renderer.hpp"
#include "gui/vekt.hpp"
#include "gui/icon_defs.hpp"
#include "game/game_world_renderer.hpp"
#include "world/world.hpp"
#include "world/components/comp_camera.hpp"
#include "math/matrix4x4.hpp"
#include "math/vector4.hpp"
#include "math/math.hpp"
#include "platform/window.hpp"
#include "platform/window_common.hpp"
#include "input/input_mappings.hpp"
#include "platform/process.hpp"
#include "memory/memory_tracer.hpp"
#include "common/system_info.hpp"

namespace SFG
{
	namespace
	{
		struct gizmo_context
		{
			vector2	  panel_pos	  = vector2::zero;
			vector2	  panel_size  = vector2::zero;
			matrix4x4 view		  = matrix4x4::identity;
			matrix4x4 proj		  = matrix4x4::identity;
			matrix4x4 view_proj	  = matrix4x4::identity;
			vector3	  cam_pos	  = vector3::zero;
			f32		  fov_degrees = 60.0f;
		};

		vector3 axis_from_gizmo(editor_panels_world_view::gizmo_axis axis)
		{
			switch (axis)
			{
			case editor_panels_world_view::gizmo_axis::x:
				return vector3::right;
			case editor_panels_world_view::gizmo_axis::y:
				return vector3::up;
			case editor_panels_world_view::gizmo_axis::z:
				return vector3::forward;
			default:
				return vector3::zero;
			}
		}

		vector3 axis_from_gizmo(editor_panels_world_view::gizmo_axis axis, const quat& rot, editor_panels_world_view::gizmo_space space)
		{
			const vector3 axis_vec = axis_from_gizmo(axis);
			if (space == editor_panels_world_view::gizmo_space::local)
				return rot * axis_vec;
			return axis_vec;
		}

		bool project_point(const matrix4x4& view_proj, const vector2& panel_pos, const vector2& panel_size, const vector3& world_pos, vector2& out)
		{
			vector4 clip = view_proj * vector4(world_pos.x, world_pos.y, world_pos.z, 1.0f);
			if (math::abs(clip.w) < MATH_EPS)
				return false;

			const f32 inv_w = 1.0f / clip.w;
			const f32 ndc_x = clip.x * inv_w;
			const f32 ndc_y = clip.y * inv_w;

			out.x = panel_pos.x + (ndc_x * 0.5f + 0.5f) * panel_size.x;
			out.y = panel_pos.y + (1.0f - (ndc_y * 0.5f + 0.5f)) * panel_size.y;
			return true;
		}

		f32 distance_point_segment(const vector2& p, const vector2& a, const vector2& b)
		{
			const vector2 ab		 = b - a;
			const f32	  ab_len_sqr = ab.magnitude_sqr();
			if (ab_len_sqr < MATH_EPS)
				return vector2::distance(p, a);

			const f32	  t		  = math::clamp(vector2::dot(p - a, ab) / ab_len_sqr, 0.0f, 1.0f);
			const vector2 closest = a + ab * t;
			return vector2::distance(p, closest);
		}
	}

	void editor_panels_world_view::init(vekt::builder* b)
	{
		_builder = b;
		_gui_builder.init(b);
		_root = _gui_builder.get_root();

		vekt::widget_gfx& gfx = _builder->widget_get_gfx(_root);
		gfx.user_data		  = &_user_data;
		gfx.flags			  = vekt::gfx_flags::gfx_is_rect | vekt::gfx_flags::gfx_focusable | vekt::gfx_flags::gfx_custom_pass;

		_user_data.type = editor_gui_user_data_type::world_rt;

		_gui_builder.callbacks.user_data   = this;
		_gui_builder.callbacks.callback_ud = this;
		_gui_builder.callbacks.on_mouse	   = on_widget_mouse;

		vekt::custom_passes& cp = _builder->widget_get_custom_pass(_root);
		cp.custom_draw_pass		= on_widget_draw;

		vekt::key_callback& kc = _builder->widget_get_key_callbacks(_root);
		kc.on_key			   = on_key;

		vekt::mouse_callback& mc = _builder->widget_get_mouse_callbacks(_root);
		mc.on_mouse				 = on_widget_mouse;

		_builder->widget_get_user_data(_root).ptr = this;

		_gui_builder.set_draw_order(1);

		_icon_row			   = _gui_builder.begin_row();
		_btn_gizmo_translation = _gui_builder.add_icon_button(ICON_EYE, 0, 1.15f, true).first;
		_btn_gizmo_rotation	   = _gui_builder.add_icon_button(ICON_EYE, 0, 1.15f, true).first;
		_btn_gizmo_scale	   = _gui_builder.add_icon_button(ICON_EYE, 0, 1.15f, true).first;
		_gui_builder.add_row_separator(editor_theme::get().col_frame_bg);
		_btn_audio_on		  = _gui_builder.add_icon_button(ICON_AUDIO, 0, 1.15f, true).first;
		_btn_physics_debug_on = _gui_builder.add_icon_button(ICON_CUBES, 0, 1.15f, true).first;
		_gui_builder.end_row();
		_gui_builder.set_draw_order(0);

		update_gizmo_buttons();
		update_toggle_button(_btn_audio_on, _audio_enabled);
		update_toggle_button(_btn_physics_debug_on, _physics_debug_enabled);

		_gui_builder.set_draw_order(1);

		_icon_column = _gui_builder.begin_column();
		{
			vekt::pos_props& pp = _builder->widget_get_pos_props(_icon_column);
			pp.pos.x			= 1.0f;
			pp.flags |= vekt::pos_flags::pf_x_anchor_end;
		}
		_btn_menu = _gui_builder.add_icon_button(ICON_FILE, 0, 1.15f, false).first;
		_gui_builder.add_col_separator(editor_theme::get().col_frame_bg);

		_btn_view  = _gui_builder.add_icon_button(ICON_EYE, 0, 1.15f, false).first;
		_btn_stats = _gui_builder.add_icon_button(ICON_INFO, 0, 1.15f, false).first;
		_gui_builder.add_col_separator(editor_theme::get().col_frame_bg);
		_btn_play = _gui_builder.add_icon_button(ICON_PLAY, 0, 1.15f, false, editor_theme::get().col_accent_third).first;

		{
			vekt::pos_props& pp = _builder->widget_get_pos_props(_btn_menu);
			pp.flags			= vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_x_anchor_center;
			pp.pos.x			= 0.5f;
		}

		{
			vekt::pos_props& pp = _builder->widget_get_pos_props(_btn_stats);
			pp.flags			= vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_x_anchor_center;
			pp.pos.x			= 0.5f;
		}
		_gui_builder.set_draw_order(0);

		_gui_builder.end_column();
	}

	void editor_panels_world_view::uninit()
	{
		_gui_builder.uninit();
	}

	void editor_panels_world_view::draw(const vector2ui16& window_size)
	{
		static f32 stat_fetch_time = 0.0f;
		static f32 mem_fetch_time  = 0.0f;

		static f32 stat_main_thread	  = static_cast<f32>(g_frame_info.main_thread_time_milli.load());
		static f32 stat_render_thread = static_cast<f32>(g_frame_info.render_thread_time_milli.load());
		static u32 stat_fps			  = g_frame_info.fps.load();
		static u32 stat_dc			  = static_cast<u32>(g_frame_info.draw_calls_ui.load(std::memory_order_acquire));
		static u32 stat_ram			  = 0;
		static u32 stat_vram		  = 0;
		static u32 stat_vram_txt	  = 0;
		static u32 stat_vram_res	  = 0;

		if (_stats_area != NULL_WIDGET_ID)
		{
			if (stat_fetch_time > 1500.0f)
			{
				stat_main_thread   = static_cast<f32>(g_frame_info.main_thread_time_milli.load());
				stat_render_thread = static_cast<f32>(g_frame_info.render_thread_time_milli.load());
				stat_fps		   = g_frame_info.fps.load();
				stat_dc			   = static_cast<u32>(g_frame_info.draw_calls_ui.load(std::memory_order_acquire));
				stat_fetch_time	   = 0.0f;

				const vector2ui16& game_size = editor::get().get_app().get_game_resolution();

				_builder->widget_append_text_start(_wv_game_res);
				_builder->widget_append_text(_wv_game_res, game_size.x, 0);
				_builder->widget_append_text(_wv_game_res, "x");
				_builder->widget_append_text(_wv_game_res, game_size.y, 0);

				_builder->widget_append_text_start(_wv_window_res);
				_builder->widget_append_text(_wv_window_res, window_size.x, 0);
				_builder->widget_append_text(_wv_window_res, "x");
				_builder->widget_append_text(_wv_window_res, window_size.y, 0);

				_builder->widget_append_text_start(_wv_fps);
				_builder->widget_append_text(_wv_fps, stat_fps);

				_builder->widget_append_text_start(_wv_main);
				_builder->widget_append_text(_wv_main, stat_main_thread);
				_builder->widget_append_text(_wv_main, " ms");

				_builder->widget_append_text_start(_wv_render);
				_builder->widget_append_text(_wv_render, stat_render_thread);
				_builder->widget_append_text(_wv_render, " ms");

				_builder->widget_append_text_start(_wv_loaded_project);
				_builder->widget_append_text(_wv_loaded_project, editor_settings::get().working_dir.c_str());

				_builder->widget_append_text_start(_wv_loaded_level);
				_builder->widget_append_text(_wv_loaded_level, editor::get().get_loaded_level().c_str());

				_builder->widget_append_text_start(_wv_draw_calls);
				_builder->widget_append_text(_wv_draw_calls, stat_dc);
			}

			if (mem_fetch_time > 6000)
			{
#ifdef SFG_ENABLE_MEMORY_TRACER
				memory_tracer& tracer = memory_tracer::get();
				LOCK_GUARD(tracer.get_category_mtx());

				for (const memory_category& cat : tracer.get_categories())
				{
					if (TO_SID(cat.name) == TO_SID("General"))
						stat_ram = static_cast<u32>(static_cast<f32>(cat.total_size) / (1024 * 1024));
					else if (TO_SID(cat.name) == TO_SID("Gfx"))
						stat_vram = static_cast<u32>(static_cast<f32>(cat.total_size) / (1024 * 1024));
					else if (TO_SID(cat.name) == TO_SID("GfxTxt"))
						stat_vram_txt = static_cast<u32>(static_cast<f32>(cat.total_size) / (1024 * 1024));
					else if (TO_SID(cat.name) == TO_SID("GfxRes"))
						stat_vram_res = static_cast<u32>(static_cast<f32>(cat.total_size) / (1024 * 1024));
				}
#endif
				mem_fetch_time = 0.0f;

				_builder->widget_append_text_start(_wv_ram);
				_builder->widget_append_text(_wv_ram, stat_ram);
				_builder->widget_append_text(_wv_ram, " mb");

				_builder->widget_append_text_start(_wv_vram);
				_builder->widget_append_text(_wv_vram, stat_vram);
				_builder->widget_append_text(_wv_vram, " mb");

				_builder->widget_append_text_start(_wv_vram_txt);
				_builder->widget_append_text(_wv_vram_txt, stat_vram_txt);
				_builder->widget_append_text(_wv_vram_txt, " mb");

				_builder->widget_append_text_start(_wv_vram_res);
				_builder->widget_append_text(_wv_vram_res, stat_vram_res);
				_builder->widget_append_text(_wv_vram_res, " mb");
			}

			const f32 ms = static_cast<f32>(g_frame_info.main_thread_time_milli.load());
			mem_fetch_time += ms;
			stat_fetch_time += ms;
		}

		if (!_gizmo_dragging)
		{
			_gizmo_center_handle = false;
			return;
		}

		editor_panel_entities* entities = editor::get().get_gui_controller().get_entities();
		if (!entities)
			return;

		world_handle selected = entities->get_selected();

		const vector2i16 mp_i  = editor::get().get_app().get_main_window().get_mouse_position();
		const vector2	 mp	   = vector2(static_cast<f32>(mp_i.x), static_cast<f32>(mp_i.y));
		const vector2	 delta = mp - _gizmo_last_mouse;
		if (delta.is_zero())
			return;

		const vector2 panel_pos	 = _builder->widget_get_pos(_root);
		const vector2 panel_size = _builder->widget_get_size(_root);
		if (panel_size.x <= 0.0f || panel_size.y <= 0.0f)
			return;

		world&			   w		  = editor::get().get_app().get_world();
		entity_manager&	   em		  = w.get_entity_manager();
		component_manager& cm		  = w.get_comp_manager();
		const world_handle cam_entity = em.get_main_camera_entity();
		const world_handle cam_comp	  = em.get_main_camera_comp();
		if (cam_entity.is_null() || cam_comp.is_null())
			return;

		comp_camera&	cam		  = cm.get_component<comp_camera>(cam_comp);
		const vector3	cam_pos	  = em.get_entity_position_abs(cam_entity);
		const quat		cam_rot	  = em.get_entity_rotation_abs(cam_entity);
		const matrix4x4 view	  = matrix4x4::view(cam_rot, cam_pos);
		const f32		aspect	  = panel_size.y > 0.0f ? panel_size.x / panel_size.y : 1.0f;
		const matrix4x4 proj	  = matrix4x4::perspective_reverse_z(cam.get_fov_degrees(), aspect, cam.get_near(), cam.get_far());
		const matrix4x4 view_proj = proj * view;

		const vector3 center		= em.get_entity_position_abs(selected);
		vector2		  center_screen = vector2::zero;
		if (!project_point(view_proj, panel_pos, panel_size, center, center_screen))
			return;

		const f32 dist		= vector3::distance(center, cam_pos);
		const f32 gizmo_len = math::max(0.5f, dist * 0.1f);

		vector3 axis_world		= vector3::zero;
		vector2 axis_dir_screen = vector2::zero;
		f32		axis_pixels		= 0.0f;
		if (!_gizmo_center_handle && _gizmo_active_axis != gizmo_axis::none)
		{
			axis_world				= axis_from_gizmo(_gizmo_active_axis, _gizmo_start_rot, _gizmo_space);
			vector2 axis_end_screen = vector2::zero;
			if (!project_point(view_proj, panel_pos, panel_size, center + axis_world * gizmo_len, axis_end_screen))
				return;

			axis_dir_screen	   = axis_end_screen - center_screen;
			const f32 axis_len = axis_dir_screen.magnitude();
			if (axis_len < MATH_EPS)
				return;
			axis_dir_screen /= axis_len;
			axis_pixels = vector2::dot(delta, axis_dir_screen);
		}

		if (_gizmo_mode == gizmo_mode::position)
		{
			const vector4 view_pos		  = view * vector4(center.x, center.y, center.z, 1.0f);
			const f32	  z_dist		  = math::abs(view_pos.z);
			const f32	  fov_rad		  = math::degrees_to_radians(cam.get_fov_degrees());
			const f32	  world_per_pixel = (2.0f * math::tan(0.5f * fov_rad) * z_dist) / panel_size.y;
			if (_gizmo_center_handle)
			{
				const vector3 cam_right = cam_rot.get_right();
				const vector3 cam_up	= cam_rot.get_up();
				_gizmo_drag_offset += (cam_right * (delta.x * world_per_pixel)) + (cam_up * (-delta.y * world_per_pixel));
				const vector3 new_pos = _gizmo_start_pos + _gizmo_drag_offset;
				em.set_entity_position_abs(selected, new_pos);
				_gizmo_last_mouse = mp;
				return;
			}
			_gizmo_drag_amount += axis_pixels * world_per_pixel;
			const vector3 new_pos = _gizmo_start_pos + axis_world * _gizmo_drag_amount;
			em.set_entity_position_abs(selected, new_pos);
		}
		else if (_gizmo_mode == gizmo_mode::rotation)
		{
			const vector2 tangent	   = vector2(-axis_dir_screen.y, axis_dir_screen.x);
			const f32	  angle_pixels = vector2::dot(delta, tangent);
			_gizmo_drag_amount += angle_pixels * 0.4f;
			const quat rot_delta = quat::angle_axis(_gizmo_drag_amount, axis_world);
			em.set_entity_rotation_abs(selected, rot_delta * _gizmo_start_rot);
		}
		else if (_gizmo_mode == gizmo_mode::scale)
		{
			if (_gizmo_center_handle)
			{
				_gizmo_drag_amount += (-delta.y) * 0.01f;
				const f32 uniform_factor = math::max(0.01f, 1.0f + _gizmo_drag_amount);
				vector3	  new_scale		 = _gizmo_start_scale * uniform_factor;
				new_scale.x				 = math::max(0.01f, new_scale.x);
				new_scale.y				 = math::max(0.01f, new_scale.y);
				new_scale.z				 = math::max(0.01f, new_scale.z);
				em.set_entity_scale_abs(selected, new_scale);
				_gizmo_last_mouse = mp;
				return;
			}
			_gizmo_drag_amount += axis_pixels * 0.01f;
			vector3 new_scale = _gizmo_start_scale;
			if (_gizmo_active_axis == gizmo_axis::x)
				new_scale.x = math::max(0.01f, _gizmo_start_scale.x + _gizmo_drag_amount);
			else if (_gizmo_active_axis == gizmo_axis::y)
				new_scale.y = math::max(0.01f, _gizmo_start_scale.y + _gizmo_drag_amount);
			else if (_gizmo_active_axis == gizmo_axis::z)
				new_scale.z = math::max(0.01f, _gizmo_start_scale.z + _gizmo_drag_amount);
			em.set_entity_scale_abs(selected, new_scale);
		}

		_gizmo_last_mouse = mp;
	}

	vekt::input_event_result editor_panels_world_view::on_widget_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase)
	{
		if (ev.type == vekt::input_event_type::released && ev.button == input_code::mouse_0)
		{
			editor_panels_world_view* self = static_cast<editor_panels_world_view*>(b->widget_get_user_data(widget).ptr);
			if (!self)
				return vekt::input_event_result::not_handled;

			const popup_action action = self->_popup_action;

			if (widget == self->_popup_save_yes)
			{
				editor::get().get_gui_controller().kill_popup();
				editor::get().save_lavel();

				if (action == popup_action::new_world)
					editor::get().new_level();
				else if (action == popup_action::open_world)
					editor::get().load_level_prompt();

				return vekt::input_event_result::handled;
			}

			if (widget == self->_popup_save_no)
			{
				editor::get().get_gui_controller().kill_popup();

				if (action == popup_action::new_world)
					editor::get().new_level();
				else if (action == popup_action::open_world)
					editor::get().load_level_prompt();

				return vekt::input_event_result::handled;
			}
		}

		if (ev.type != vekt::input_event_type::pressed)
			return vekt::input_event_result::not_handled;

		editor_panels_world_view* self = static_cast<editor_panels_world_view*>(b->widget_get_user_data(widget).ptr);

		if (widget == self->_btn_gizmo_translation)
		{
			self->_gizmo_mode = gizmo_mode::position;
			self->update_gizmo_buttons();
			return vekt::input_event_result::handled;
		}

		if (widget == self->_btn_gizmo_rotation)
		{
			self->_gizmo_mode = gizmo_mode::rotation;
			self->update_gizmo_buttons();
			return vekt::input_event_result::handled;
		}

		if (widget == self->_btn_gizmo_scale)
		{
			self->_gizmo_mode = gizmo_mode::scale;
			self->update_gizmo_buttons();
			return vekt::input_event_result::handled;
		}

		if (widget == self->_btn_audio_on)
		{
			self->_audio_enabled = !self->_audio_enabled;
			self->update_toggle_button(self->_btn_audio_on, self->_audio_enabled);
			return vekt::input_event_result::handled;
		}

		if (widget == self->_btn_physics_debug_on)
		{
			self->_physics_debug_enabled = !self->_physics_debug_enabled;
			self->update_toggle_button(self->_btn_physics_debug_on, self->_physics_debug_enabled);
			return vekt::input_event_result::handled;
		}

		if (widget == self->_btn_view)
		{
			editor::get().get_gui_controller().begin_context_menu(ev.position.x, ev.position.y);

			self->_ctx_world_rt	   = editor::get().get_gui_controller().add_context_menu_item("world");
			self->_ctx_colors_rt   = editor::get().get_gui_controller().add_context_menu_item("gbuffer_color");
			self->_ctx_normals_rt  = editor::get().get_gui_controller().add_context_menu_item("gbuffer_normal");
			self->_ctx_orm_rt	   = editor::get().get_gui_controller().add_context_menu_item("gbuffer_orm");
			self->_ctx_emissive_rt = editor::get().get_gui_controller().add_context_menu_item("gbuffer_emissive");
			self->_ctx_depth_rt	   = editor::get().get_gui_controller().add_context_menu_item("depth");
			self->_ctx_lighting_rt = editor::get().get_gui_controller().add_context_menu_item("lighting");
			self->_ctx_ssao_rt	   = editor::get().get_gui_controller().add_context_menu_item("ssao");
			self->_ctx_bloom_rt	   = editor::get().get_gui_controller().add_context_menu_item("bloom");

			auto hook = [&](vekt::id id) {
				vekt::mouse_callback& cb		= b->widget_get_mouse_callbacks(id);
				cb.on_mouse						= on_widget_mouse;
				b->widget_get_user_data(id).ptr = self;
			};

			hook(self->_ctx_world_rt);
			hook(self->_ctx_colors_rt);
			hook(self->_ctx_normals_rt);
			hook(self->_ctx_orm_rt);
			hook(self->_ctx_emissive_rt);
			hook(self->_ctx_depth_rt);
			hook(self->_ctx_lighting_rt);
			hook(self->_ctx_ssao_rt);
			hook(self->_ctx_bloom_rt);

			editor::get().get_gui_controller().end_context_menu();
			return vekt::input_event_result::handled;
		}

		if (widget == self->_btn_menu)
		{
			editor::get().get_gui_controller().begin_context_menu(ev.position.x, ev.position.y);

			self->_ctx_new_project	   = editor::get().get_gui_controller().add_context_menu_item("new project");
			self->_ctx_open_project	   = editor::get().get_gui_controller().add_context_menu_item("open project");
			self->_ctx_package_project = editor::get().get_gui_controller().add_context_menu_item("package project");
			self->_ctx_new_world	   = editor::get().get_gui_controller().add_context_menu_item("new world");
			self->_ctx_save_world	   = editor::get().get_gui_controller().add_context_menu_item("save world");
			self->_ctx_open_world	   = editor::get().get_gui_controller().add_context_menu_item("open world");

			auto hook = [&](vekt::id id) {
				vekt::mouse_callback& cb		= b->widget_get_mouse_callbacks(id);
				cb.on_mouse						= on_widget_mouse;
				b->widget_get_user_data(id).ptr = self;
			};

			hook(self->_ctx_new_project);
			hook(self->_ctx_open_project);
			hook(self->_ctx_package_project);
			hook(self->_ctx_new_world);
			hook(self->_ctx_save_world);
			hook(self->_ctx_open_world);

			editor::get().get_gui_controller().end_context_menu();
			return vekt::input_event_result::handled;
		}

		if (widget == self->_btn_play)
		{
			editor::get().enter_playmode(false);
			return vekt::input_event_result::handled;
		}

		if (widget == self->_btn_stats)
		{
			if (self->_stats_area != NULL_WIDGET_ID)
			{
				self->_gui_builder.deallocate(self->_stats_area);
				self->_stats_area = NULL_WIDGET_ID;
				return vekt::input_event_result::handled;
			}
			self->_gui_builder.set_draw_order(2);

			self->_stats_area = self->_gui_builder.begin_area(false, false);

			{
				vekt::pos_props& pp = b->widget_get_pos_props(self->_stats_area);
				pp.flags			= vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_y_anchor_end | vekt::pos_flags::pf_x_anchor_end | vekt::pos_flags::pf_child_pos_column;
				pp.pos				= {1.0f, 1.0f};

				vekt::size_props& sz = b->widget_get_size_props(self->_stats_area);
				sz.flags			 = vekt::size_flags::sf_x_abs | vekt::size_flags::sf_y_abs;
				sz.spacing			 = editor_theme::get().item_spacing;
				sz.child_margins	 = {editor_theme::get().outer_margin, editor_theme::get().outer_margin, editor_theme::get().outer_margin, editor_theme::get().outer_margin};
				sz.size.x			 = 400;
				sz.size.y			 = 300;

				vekt::widget_gfx& gfx = b->widget_get_gfx(self->_stats_area);
				gfx.color.w			  = 0.75f;

				self->_gui_builder.set_draw_order(2);

				self->_wv_game_res		 = self->_gui_builder.add_property_row_label("game_res:", "fetching...", 12).second;
				self->_wv_window_res	 = self->_gui_builder.add_property_row_label("window_res:", "fetching...", 12).second;
				self->_wv_fps			 = self->_gui_builder.add_property_row_label("fps:", "fetching...", 12).second;
				self->_wv_main			 = self->_gui_builder.add_property_row_label("main:", "fetching...", 12).second;
				self->_wv_render		 = self->_gui_builder.add_property_row_label("render:", "fetching...", 12).second;
				self->_wv_ram			 = self->_gui_builder.add_property_row_label("ram:", "fetching...", 12).second;
				self->_wv_vram			 = self->_gui_builder.add_property_row_label("vram:", "fetching...", 12).second;
				self->_wv_vram_txt		 = self->_gui_builder.add_property_row_label("vram_texture:", "fetching...", 12).second;
				self->_wv_vram_res		 = self->_gui_builder.add_property_row_label("vram_buffer:", "fetching...", 12).second;
				self->_wv_draw_calls	 = self->_gui_builder.add_property_row_label("draw_calls:", "fetching...", 12).second;
				self->_wv_loaded_project = self->_gui_builder.add_property_row_label("project:", editor_settings::get().working_dir.c_str(), 1024).second;
				self->_wv_loaded_level	 = self->_gui_builder.add_property_row_label("world:", editor::get().get_loaded_level().c_str(), 1024).second;
			}
			self->_gui_builder.end_area();
			self->_gui_builder.set_draw_order(0);

			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_world_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::world_rt;
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_colors_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::colors_rt;
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_normals_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::normals_rt;
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_orm_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::orm_rt;
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_emissive_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::emissive_rt;
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_lighting_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::lighting_rt;
			return vekt::input_event_result::handled;
		}
		if (widget == self->_ctx_bloom_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::bloom_rt;
			return vekt::input_event_result::handled;
		}
		if (widget == self->_ctx_depth_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::depth_rt;
			return vekt::input_event_result::handled;
		}
		if (widget == self->_ctx_ssao_rt)
		{
			self->_user_data.type = editor_gui_user_data_type::ssao_rt;
			return vekt::input_event_result::handled;
		}
		if (widget == self->_ctx_new_project)
		{
			const string dir = process::select_folder("select project directory");
			if (!dir.empty())
				editor::get().new_project(dir.c_str());
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_open_project)
		{
			process::open_directory(editor_settings::get().working_dir.c_str());
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_package_project)
		{
			// Not implemented in panel controls either; handle click.
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_new_world)
		{
			self->open_save_popup(popup_action::new_world);
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_save_world)
		{
			editor::get().save_lavel();
			return vekt::input_event_result::handled;
		}

		if (widget == self->_ctx_open_world)
		{
			self->open_save_popup(popup_action::open_world);
			return vekt::input_event_result::handled;
		}

		return vekt::input_event_result::not_handled;
	}

	vekt::input_event_result editor_panels_world_view::on_key(vekt::builder* b, vekt::id widget, const vekt::key_event& ev)
	{
		if (ev.type != vekt::input_event_type::pressed)
			return vekt::input_event_result::not_handled;

		const bool ctrl_down = window::is_key_down(input_code::key_lctrl) || window::is_key_down(input_code::key_rctrl);
		if (!ctrl_down)
			return vekt::input_event_result::not_handled;

		editor_panels_world_view* self = static_cast<editor_panels_world_view*>(b->widget_get_user_data(widget).ptr);
		if (!self)
			return vekt::input_event_result::not_handled;

		if (ev.key == input_code::key_alpha1)
		{
			self->_gizmo_mode = gizmo_mode::position;
			self->update_gizmo_buttons();
			return vekt::input_event_result::handled;
		}
		if (ev.key == input_code::key_alpha2)
		{
			self->_gizmo_mode = gizmo_mode::rotation;
			self->update_gizmo_buttons();
			return vekt::input_event_result::handled;
		}
		if (ev.key == input_code::key_alpha3)
		{
			self->_gizmo_mode = gizmo_mode::scale;
			self->update_gizmo_buttons();
			return vekt::input_event_result::handled;
		}
		if (ev.key == input_code::key_g)
		{
			self->_gizmo_space = self->_gizmo_space == gizmo_space::global ? gizmo_space::local : gizmo_space::global;
			return vekt::input_event_result::handled;
		}

		return vekt::input_event_result::not_handled;
	}

	void editor_panels_world_view::on_widget_draw(vekt::builder* b, vekt::id widget)
	{
		editor_panels_world_view* self = static_cast<editor_panels_world_view*>(b->widget_get_user_data(widget).ptr);
		if (!self)
			return;

		const vector2 panel_pos	 = b->widget_get_pos(widget);
		const vector2 panel_size = b->widget_get_size(widget);
		if (panel_size.x <= 0.0f || panel_size.y <= 0.0f)
			return;

		world&			   w		  = editor::get().get_app().get_world();
		entity_manager&	   em		  = w.get_entity_manager();
		component_manager& cm		  = w.get_comp_manager();
		const world_handle cam_entity = em.get_main_camera_entity();
		const world_handle cam_comp	  = em.get_main_camera_comp();
		if (cam_entity.is_null() || cam_comp.is_null())
			return;

		comp_camera&	cam		  = cm.get_component<comp_camera>(cam_comp);
		const vector3	cam_pos	  = em.get_entity_position_abs(cam_entity);
		const quat		cam_rot	  = em.get_entity_rotation_abs(cam_entity);
		const matrix4x4 view	  = matrix4x4::view(cam_rot, cam_pos);
		const f32		aspect	  = panel_size.y > 0.0f ? panel_size.x / panel_size.y : 1.0f;
		const matrix4x4 proj	  = matrix4x4::perspective_reverse_z(cam.get_fov_degrees(), aspect, cam.get_near(), cam.get_far());
		const matrix4x4 view_proj = proj * view;

		const vector3 x_cam				  = (cam_rot.inverse() * vector3::right).normalized();
		const vector2 x_dir				  = vector2(x_cam.x, -x_cam.y);
		const vector3 y_cam				  = (cam_rot.inverse() * vector3::up).normalized();
		const vector2 y_dir				  = vector2(y_cam.x, -y_cam.y);
		const vector3 z_cam				  = (cam_rot.inverse() * vector3::forward).normalized();
		const vector2 z_dir				  = vector2(z_cam.x, -z_cam.y);
		const f32	  box_size_multiplier = 0.03f;
		const f32	  axis_thickness	  = 2.0f;
		const vector2 box_size			  = vector2(panel_size.x * box_size_multiplier, panel_size.x * box_size_multiplier);
		const vector2 box_min			  = panel_pos + vector2(editor_theme::get().outer_margin + box_size.x * 0.5f, panel_size.y - editor_theme::get().outer_margin - box_size.y);
		const vector2 box_max			  = box_min + box_size;
		const vector2 box_center		  = (box_min + box_max) * 0.5f;

		b->add_line_aa({
			.p0			  = box_center,
			.p1			  = box_center + x_dir * box_size.x,
			.color		  = editor_theme::get().color_axis_x,
			.thickness	  = axis_thickness,
			.aa_thickness = 2,
			.draw_order	  = 6,
		});

		b->add_line_aa({
			.p0			  = box_center,
			.p1			  = box_center + y_dir * box_size.x,
			.color		  = editor_theme::get().color_axis_y,
			.thickness	  = axis_thickness,
			.aa_thickness = 2,
			.draw_order	  = 6,
		});

		b->add_line_aa({
			.p0			  = box_center,
			.p1			  = box_center + z_dir * box_size.x,
			.color		  = editor_theme::get().color_axis_z,
			.thickness	  = axis_thickness,
			.aa_thickness = 2,
			.draw_order	  = 6,
		});

		editor_panel_entities* entities = editor::get().get_gui_controller().get_entities();
		if (!entities)
			return;

		const world_handle selected = entities->get_selected();
		if (selected.is_null())
			return;

		const vector3 center		= em.get_entity_position_abs(selected);
		const quat	  entity_rot	= em.get_entity_rotation_abs(selected);
		vector2		  center_screen = vector2::zero;
		if (!project_point(view_proj, panel_pos, panel_size, center, center_screen))
			return;

		const f32	  dist			  = vector3::distance(center, cam_pos);
		const f32	  gizmo_len		  = math::max(0.5f, dist * 0.1f);
		const f32	  thickness		  = 2.0f;
		f32			  screen_radius	  = 12.0f;
		vector2		  axis_ref_screen = vector2::zero;
		const vector3 axis_ref		  = axis_from_gizmo(gizmo_axis::x, entity_rot, self->_gizmo_space);
		if (project_point(view_proj, panel_pos, panel_size, center + axis_ref * gizmo_len, axis_ref_screen))
			screen_radius = (axis_ref_screen - center_screen).magnitude();

		self->_gizmo_hover_axis			 = self->_gizmo_dragging ? self->_gizmo_active_axis : gizmo_axis::none;
		self->_gizmo_center_hover		 = self->_gizmo_dragging ? self->_gizmo_center_handle : false;
		const vector2i16 mp_i			 = editor::get().get_app().get_main_window().get_mouse_position();
		const vector2	 mp				 = vector2(static_cast<f32>(mp_i.x), static_cast<f32>(mp_i.y));
		const f32		 hover_threshold = 8.0f;
		if (!self->_gizmo_dragging && mp.x >= panel_pos.x && mp.y >= panel_pos.y && mp.x <= panel_pos.x + panel_size.x && mp.y <= panel_pos.y + panel_size.y)
		{
			const f32	  f			= math::clamp(screen_radius * 0.15f, 8.0f, 18.0f);
			const vector2 rect_size = vector2(f, f);
			const vector2 half_size = rect_size * 0.5f;
			const vector2 rect_min	= center_screen - half_size;
			const vector2 rect_max	= center_screen + half_size;
			if (mp.x >= rect_min.x && mp.x <= rect_max.x && mp.y >= rect_min.y && mp.y <= rect_max.y)
			{
				self->_gizmo_center_hover = true;
			}
			else if (self->_gizmo_mode == gizmo_mode::position || self->_gizmo_mode == gizmo_mode::scale)
			{
				struct hover_axis
				{
					gizmo_axis id;
					vector3	   axis;
				};

				const hover_axis axes[] = {
					{gizmo_axis::x, axis_from_gizmo(gizmo_axis::x, entity_rot, self->_gizmo_space)},
					{gizmo_axis::y, axis_from_gizmo(gizmo_axis::y, entity_rot, self->_gizmo_space)},
					{gizmo_axis::z, axis_from_gizmo(gizmo_axis::z, entity_rot, self->_gizmo_space)},
				};

				f32 best_dist = hover_threshold;
				for (const hover_axis& axis : axes)
				{
					vector2 end_screen = vector2::zero;
					if (!project_point(view_proj, panel_pos, panel_size, center + axis.axis * gizmo_len, end_screen))
						continue;

					const f32 d = distance_point_segment(mp, center_screen, end_screen);
					if (d < best_dist)
					{
						best_dist				= d;
						self->_gizmo_hover_axis = axis.id;
					}
				}
			}
			else if (self->_gizmo_mode == gizmo_mode::rotation)
			{
				struct hover_circle
				{
					gizmo_axis id;
					vector3	   basis0;
					vector3	   basis1;
				};

				const hover_circle circles[] = {
					{gizmo_axis::x, vector3::up, vector3::forward},
					{gizmo_axis::y, vector3::right, vector3::forward},
					{gizmo_axis::z, vector3::right, vector3::up},
				};

				const unsigned int segments	 = 48;
				const f32		   radius	 = gizmo_len * 0.75f;
				f32				   best_dist = hover_threshold;

				for (const hover_circle& circle : circles)
				{
					vector2 prev	 = vector2::zero;
					bool	has_prev = false;
					f32		min_dist = hover_threshold;

					for (unsigned int i = 0; i <= segments; ++i)
					{
						const f32	  t			= static_cast<f32>(i) / static_cast<f32>(segments);
						const f32	  ang		= t * 2.0f * MATH_PI;
						const f32	  c			= math::cos(ang);
						const f32	  s			= math::sin(ang);
						const vector3 world_pos = center + (circle.basis0 * c + circle.basis1 * s) * radius;
						vector2		  cur		= vector2::zero;
						if (!project_point(view_proj, panel_pos, panel_size, world_pos, cur))
						{
							has_prev = false;
							continue;
						}

						if (has_prev)
						{
							const f32 d = distance_point_segment(mp, prev, cur);
							if (d < min_dist)
								min_dist = d;
						}
						prev	 = cur;
						has_prev = true;
					}

					if (min_dist < best_dist)
					{
						best_dist				= min_dist;
						self->_gizmo_hover_axis = circle.id;
					}
				}
			}
		}

		if (self->_gizmo_mode == gizmo_mode::position || self->_gizmo_mode == gizmo_mode::scale)
		{
			struct axis_draw
			{
				gizmo_axis id;
				vector3	   axis;
				vector4	   color;
			};

			const axis_draw axes[] = {
				{gizmo_axis::x, axis_from_gizmo(gizmo_axis::x, entity_rot, self->_gizmo_space), editor_theme::get().color_axis_x},
				{gizmo_axis::y, axis_from_gizmo(gizmo_axis::y, entity_rot, self->_gizmo_space), editor_theme::get().color_axis_y},
				{gizmo_axis::z, axis_from_gizmo(gizmo_axis::z, entity_rot, self->_gizmo_space), editor_theme::get().color_axis_z},
			};

			for (const axis_draw& axis : axes)
			{
				vector2 end_screen = vector2::zero;
				if (!project_point(view_proj, panel_pos, panel_size, center + axis.axis * gizmo_len, end_screen))
					continue;

				const vector4 axis_color = self->_gizmo_hover_axis == axis.id ? editor_theme::get().col_accent : axis.color;
				b->add_line_aa({
					.p0			  = center_screen,
					.p1			  = end_screen,
					.color		  = axis_color,
					.thickness	  = thickness,
					.aa_thickness = 2,
					.draw_order	  = 5,
				});

				const vector2 axis_dir	 = (end_screen - center_screen).normalized();
				const vector2 axis_ortho = vector2(-axis_dir.y, axis_dir.x);

				if (self->_gizmo_mode == gizmo_mode::scale)
				{
					const f32			   cap		= math::clamp(screen_radius * 0.1f, 6.0f, 12.0f);
					const vector2		   cap_half = vector2(cap * 0.5f, cap * 0.5f);
					const vector2		   cap_min	= end_screen - cap_half;
					const vector2		   cap_max	= end_screen + cap_half;
					const vekt::widget_gfx gfx		= {
							 .color		 = axis_color,
							 .draw_order = 6,
							 .flags		 = vekt::gfx_flags::gfx_is_rect,
					 };

					b->add_filled_rect({
						.gfx			 = gfx,
						.min			 = cap_min,
						.max			 = cap_max,
						.color_start	 = axis_color,
						.color_end		 = axis_color,
						.color_direction = vekt::direction::horizontal,
						.widget_id		 = widget,
						.multi_color	 = false,
					});
				}
				else if (self->_gizmo_mode == gizmo_mode::position)
				{
					const f32	  arrow_len	  = math::clamp(screen_radius * 0.12f, 6.0f, 14.0f);
					const f32	  arrow_width = arrow_len * 0.6f;
					const vector2 arrow_base  = end_screen - axis_dir * arrow_len;
					const vector2 arrow_left  = arrow_base + axis_ortho * (arrow_width * 0.5f);
					const vector2 arrow_right = arrow_base - axis_ortho * (arrow_width * 0.5f);

					b->add_line_aa({.p0 = end_screen, .p1 = arrow_left, .color = axis_color, .thickness = thickness, .aa_thickness = 2, .draw_order = 6});
					b->add_line_aa({.p0 = end_screen, .p1 = arrow_right, .color = axis_color, .thickness = thickness, .aa_thickness = 2, .draw_order = 6});
				}
			}

			const f32			   f		  = math::clamp(screen_radius * 0.15f, 8.0f, 18.0f);
			const vector2		   rect_size  = vector2(f, f);
			const vector2		   half_size  = rect_size * 0.5f;
			const vector2		   rect_min	  = center_screen - half_size;
			const vector2		   rect_max	  = center_screen + half_size;
			const vector4		   rect_color = self->_gizmo_center_hover ? editor_theme::get().col_accent : editor_theme::get().col_text;
			const vekt::widget_gfx gfx		  = {
					   .color	   = rect_color,
					   .draw_order = 6,
					   .flags	   = vekt::gfx_flags::gfx_is_rect,
			   };

			b->add_filled_rect({
				.gfx			 = gfx,
				.min			 = rect_min,
				.max			 = rect_max,
				.color_start	 = rect_color,
				.color_end		 = rect_color,
				.color_direction = vekt::direction::horizontal,
				.widget_id		 = widget,
				.multi_color	 = false,
			});
		}

		if (self->_gizmo_mode == gizmo_mode::rotation)
		{
			struct axis_circle
			{
				gizmo_axis id;
				vector3	   basis0;
				vector3	   basis1;
				vector4	   color;
			};

			const axis_circle circles[] = {
				{gizmo_axis::x, vector3::up, vector3::forward, editor_theme::get().color_axis_x},
				{gizmo_axis::y, vector3::right, vector3::forward, editor_theme::get().color_axis_y},
				{gizmo_axis::z, vector3::right, vector3::up, editor_theme::get().color_axis_z},
			};

			const unsigned int segments = 48;
			const f32		   radius	= gizmo_len * 0.75f;

			for (const axis_circle& circle : circles)
			{
				vector2 prev	 = vector2::zero;
				bool	has_prev = false;
				for (unsigned int i = 0; i <= segments; ++i)
				{
					const f32	  t			= static_cast<f32>(i) / static_cast<f32>(segments);
					const f32	  ang		= t * 2.0f * MATH_PI;
					const f32	  c			= math::cos(ang);
					const f32	  s			= math::sin(ang);
					const vector3 world_pos = center + (circle.basis0 * c + circle.basis1 * s) * radius;
					vector2		  cur		= vector2::zero;
					if (!project_point(view_proj, panel_pos, panel_size, world_pos, cur))
					{
						has_prev = false;
						continue;
					}

					if (has_prev)
					{
						const vector4 circle_color = self->_gizmo_hover_axis == circle.id ? editor_theme::get().col_accent : circle.color;
						b->add_line_aa({
							.p0			  = prev,
							.p1			  = cur,
							.color		  = circle_color,
							.thickness	  = thickness,
							.aa_thickness = 2,
							.draw_order	  = 5,
						});
					}
					prev	 = cur;
					has_prev = true;
				}
			}

			if (self->_gizmo_dragging && self->_gizmo_active_axis != gizmo_axis::none)
			{
				const vector2i16 mp_i	 = editor::get().get_app().get_main_window().get_mouse_position();
				const vector2	 mp		 = vector2(static_cast<f32>(mp_i.x), static_cast<f32>(mp_i.y));
				vector2			 cur_dir = mp - center_screen;
				if (!cur_dir.is_zero())
					cur_dir = cur_dir.normalized();
				else
					cur_dir = self->_gizmo_angle_current_dir;

				self->_gizmo_angle_current_dir = cur_dir;

				const axis_circle* circle_ptr = nullptr;
				for (const axis_circle& circle : circles)
				{
					if (self->_gizmo_active_axis == circle.id)
					{
						circle_ptr = &circle;
						break;
					}
				}

				if (circle_ptr)
				{
					vector2 radius_screen = vector2::zero;
					if (project_point(view_proj, panel_pos, panel_size, center + circle_ptr->basis0 * radius, radius_screen))
					{
						const f32	  screen_radius = (radius_screen - center_screen).magnitude();
						const vector2 start_point	= center_screen + self->_gizmo_angle_start_dir * screen_radius;
						const vector2 cur_point		= center_screen + self->_gizmo_angle_current_dir * screen_radius;
						const vector4 line_color	= editor_theme::get().col_text;
						b->add_line_aa({.p0 = center_screen, .p1 = start_point, .color = line_color, .thickness = thickness, .aa_thickness = 2, .draw_order = 6});
						b->add_line_aa({.p0 = center_screen, .p1 = cur_point, .color = line_color, .thickness = thickness, .aa_thickness = 2, .draw_order = 6});
					}
				}
			}
		}

		const char* name = em.get_entity_meta(selected).name;
		if (name && name[0] != '\0')
		{
			vekt::text_props tp = {};
			tp.text				= name;
			tp.font				= editor_theme::get().font_default;

			const vector2		   text_size = vekt::builder::get_text_size(tp);
			const vector2		   padding	 = vector2(6.0f, 4.0f);
			const vector2		   text_pos	 = vector2(center_screen.x - text_size.x * 0.5f, center_screen.y + 50);
			const vector2		   rect_min	 = text_pos - padding;
			const vector2		   rect_max	 = text_pos + text_size + padding;
			const vekt::widget_gfx gfx		 = {
					  .color	  = editor_theme::get().col_area_bg,
					  .draw_order = 6,
					  .flags	  = vekt::gfx_flags::gfx_is_rect,
			  };

			b->add_filled_rect({
				.gfx			 = gfx,
				.min			 = rect_min,
				.max			 = rect_max,
				.color_start	 = editor_theme::get().col_area_bg,
				.color_end		 = editor_theme::get().col_area_bg,
				.color_direction = vekt::direction::horizontal,
				.widget_id		 = widget,
				.multi_color	 = false,
			});

			b->add_text(tp, editor_theme::get().col_text, text_pos, text_size, 7, nullptr);
		}
	}

	void editor_panels_world_view::update_gizmo_buttons()
	{
		return;
		update_toggle_button(_btn_gizmo_translation, _gizmo_mode == gizmo_mode::position);
		update_toggle_button(_btn_gizmo_rotation, _gizmo_mode == gizmo_mode::rotation);
		update_toggle_button(_btn_gizmo_scale, _gizmo_mode == gizmo_mode::scale);
	}

	void editor_panels_world_view::update_toggle_button(vekt::id button, bool enabled)
	{
		if (button == NULL_WIDGET_ID || !_builder)
			return;

		vekt::widget_gfx&		 gfx = _builder->widget_get_gfx(button);
		vekt::input_color_props& icp = _builder->widget_get_input_colors(button);
		if (enabled)
		{
			gfx.color		  = editor_theme::get().col_button_press;
			icp.hovered_color = editor_theme::get().col_button_press;
			icp.pressed_color = editor_theme::get().col_button_press;
			return;
		}

		gfx.color		  = editor_theme::get().col_button;
		icp.hovered_color = editor_theme::get().col_button_hover;
		icp.pressed_color = editor_theme::get().col_button_press;
	}

	void editor_panels_world_view::open_save_popup(popup_action action)
	{
		_popup_action = action;

		editor_gui_controller& ctr = editor::get().get_gui_controller();
		ctr.begin_popup("do you want to save the current level");
		_popup_save_yes = ctr.popup_add_button("yes");
		_popup_save_no	= ctr.popup_add_button("no");

		vekt::mouse_callback& yes_cb						= _builder->widget_get_mouse_callbacks(_popup_save_yes);
		yes_cb.on_mouse										= on_widget_mouse;
		_builder->widget_get_user_data(_popup_save_yes).ptr = this;

		vekt::mouse_callback& no_cb						   = _builder->widget_get_mouse_callbacks(_popup_save_no);
		no_cb.on_mouse									   = on_widget_mouse;
		_builder->widget_get_user_data(_popup_save_no).ptr = this;

		ctr.end_popup();
	}

	bool editor_panels_world_view::on_mouse_event(const window_event& ev)
	{
		if (ev.type != window_event_type::mouse)
			return false;
		if (ev.button != static_cast<u16>(input_code::mouse_0))
			return false;

		if (ev.sub_type == window_event_sub_type::release)
		{
			if (_gizmo_dragging)
			{
				_gizmo_dragging		 = false;
				_gizmo_active_axis	 = gizmo_axis::none;
				_gizmo_center_handle = false;
				return true;
			}
			return false;
		}

		if (ev.sub_type != window_event_sub_type::press)
			return false;

		// if (_builder->widget_get_hover_callbacks(_btn_view).is_hovered || _builder->widget_get_hover_callbacks(_btn_menu).is_hovered || _builder->widget_get_hover_callbacks(_btn_stats).is_hovered || _builder->widget_get_hover_callbacks(_btn_play).is_hovered
		// || 	_builder->widget_get_hover_callbacks(_btn_gizmo_translation).is_hovered || _builder->widget_get_hover_callbacks(_btn_gizmo_rotation).is_hovered || _builder->widget_get_hover_callbacks(_btn_gizmo_scale).is_hovered ||
		//	_builder->widget_get_hover_callbacks(_btn_audio_on).is_hovered || _builder->widget_get_hover_callbacks(_btn_physics_debug_on).is_hovered || (_stats_area != NULL_WIDGET_ID && _builder->widget_get_hover_callbacks(_stats_area).is_hovered))
		//	return false;

		const vector2 panel_pos	 = _builder->widget_get_pos(_root);
		const vector2 panel_size = _builder->widget_get_size(_root);
		const vector2 mp		 = vector2(static_cast<f32>(ev.value.x), static_cast<f32>(ev.value.y));
		if (mp.x < panel_pos.x || mp.y < panel_pos.y || mp.x > panel_pos.x + panel_size.x || mp.y > panel_pos.y + panel_size.y)
			return false;

		editor_panel_entities* entities = editor::get().get_gui_controller().get_entities();
		if (!entities)
			return false;

		const world_handle selected = entities->get_selected();
		if (selected.is_null())
			return false;

		world&			   w		  = editor::get().get_app().get_world();
		entity_manager&	   em		  = w.get_entity_manager();
		component_manager& cm		  = w.get_comp_manager();
		const world_handle cam_entity = em.get_main_camera_entity();
		const world_handle cam_comp	  = em.get_main_camera_comp();
		if (cam_entity.is_null() || cam_comp.is_null())
			return false;

		comp_camera&	cam		  = cm.get_component<comp_camera>(cam_comp);
		const vector3	cam_pos	  = em.get_entity_position_abs(cam_entity);
		const quat		cam_rot	  = em.get_entity_rotation_abs(cam_entity);
		const matrix4x4 view	  = matrix4x4::view(cam_rot, cam_pos);
		const f32		aspect	  = panel_size.y > 0.0f ? panel_size.x / panel_size.y : 1.0f;
		const matrix4x4 proj	  = matrix4x4::perspective_reverse_z(cam.get_fov_degrees(), aspect, cam.get_near(), cam.get_far());
		const matrix4x4 view_proj = proj * view;

		const vector3 center		= em.get_entity_position_abs(selected);
		const quat	  entity_rot	= em.get_entity_rotation_abs(selected);
		vector2		  center_screen = vector2::zero;
		if (!project_point(view_proj, panel_pos, panel_size, center, center_screen))
			return false;

		const f32 dist			= vector3::distance(center, cam_pos);
		const f32 gizmo_len		= math::max(0.5f, dist * 0.1f);
		const f32 hit_threshold = 8.0f;

		f32			  screen_radius	  = 12.0f;
		vector2		  axis_ref_screen = vector2::zero;
		const vector3 axis_ref		  = axis_from_gizmo(gizmo_axis::x, entity_rot, _gizmo_space);
		if (project_point(view_proj, panel_pos, panel_size, center + axis_ref * gizmo_len, axis_ref_screen))
			screen_radius = (axis_ref_screen - center_screen).magnitude();
		const f32	  f			 = math::clamp(screen_radius * 0.15f, 8.0f, 18.0f);
		const vector2 rect_size	 = vector2(f, f);
		const vector2 half_size	 = rect_size * 0.5f;
		const vector2 rect_min	 = center_screen - half_size;
		const vector2 rect_max	 = center_screen + half_size;
		const bool	  hit_center = mp.x >= rect_min.x && mp.x <= rect_max.x && mp.y >= rect_min.y && mp.y <= rect_max.y;

		gizmo_axis picked	 = gizmo_axis::none;
		f32		   best_dist = hit_threshold;

		if (_gizmo_mode == gizmo_mode::position || _gizmo_mode == gizmo_mode::scale)
		{
			if (hit_center)
			{
				_gizmo_center_handle = true;
				_gizmo_active_axis	 = gizmo_axis::none;
				_gizmo_dragging		 = true;
				_gizmo_drag_amount	 = 0.0f;
				_gizmo_drag_offset	 = vector3::zero;
				_gizmo_start_pos	 = em.get_entity_position_abs(selected);
				_gizmo_start_rot	 = em.get_entity_rotation_abs(selected);
				_gizmo_start_scale	 = em.get_entity_scale_abs(selected);
				_gizmo_last_mouse	 = mp;
				return true;
			}

			const gizmo_axis axes[] = {gizmo_axis::x, gizmo_axis::y, gizmo_axis::z};
			for (gizmo_axis axis : axes)
			{
				const vector3 axis_world = axis_from_gizmo(axis, entity_rot, _gizmo_space);
				vector2		  end_screen = vector2::zero;
				if (!project_point(view_proj, panel_pos, panel_size, center + axis_world * gizmo_len, end_screen))
					continue;

				const f32 d = distance_point_segment(mp, center_screen, end_screen);
				if (d < best_dist)
				{
					best_dist = d;
					picked	  = axis;
				}
			}
		}
		else if (_gizmo_mode == gizmo_mode::rotation)
		{
			struct axis_circle
			{
				gizmo_axis axis;
				vector3	   basis0;
				vector3	   basis1;
			};

			const axis_circle circles[] = {
				{gizmo_axis::x, vector3::up, vector3::forward},
				{gizmo_axis::y, vector3::right, vector3::forward},
				{gizmo_axis::z, vector3::right, vector3::up},
			};

			const unsigned int segments = 48;
			const f32		   radius	= gizmo_len * 0.75f;

			for (const axis_circle& circle : circles)
			{
				vector2 prev	 = vector2::zero;
				bool	has_prev = false;
				f32		min_dist = hit_threshold;

				for (unsigned int i = 0; i <= segments; ++i)
				{
					const f32	  t			= static_cast<f32>(i) / static_cast<f32>(segments);
					const f32	  ang		= t * 2.0f * MATH_PI;
					const f32	  c			= math::cos(ang);
					const f32	  s			= math::sin(ang);
					const vector3 world_pos = center + (circle.basis0 * c + circle.basis1 * s) * radius;
					vector2		  cur		= vector2::zero;
					if (!project_point(view_proj, panel_pos, panel_size, world_pos, cur))
					{
						has_prev = false;
						continue;
					}

					if (has_prev)
					{
						const f32 d = distance_point_segment(mp, prev, cur);
						if (d < min_dist)
							min_dist = d;
					}
					prev	 = cur;
					has_prev = true;
				}

				if (min_dist < best_dist)
				{
					best_dist = min_dist;
					picked	  = circle.axis;
				}
			}
		}

		if (picked == gizmo_axis::none)
		{
			renderer&			 rend			= editor::get().get_app().get_renderer();
			game_world_renderer* world_renderer = rend.get_world_renderer();
			if (!world_renderer)
				return false;

			const vector2 local = mp - panel_pos;
			if (local.x < 0.0f || local.y < 0.0f || local.x >= panel_size.x || local.y >= panel_size.y)
				return false;

			const u16 x			  = static_cast<u16>(math::clamp(local.x, 0.0f, panel_size.x - 1.0f));
			const u16 y			  = static_cast<u16>(math::clamp(local.y, 0.0f, panel_size.y - 1.0f));
			const u8  frame_index = rend.get_frame_index();
			const u32 object_id	  = world_renderer->get_render_pass_object_id().read_location(x, y, frame_index);
			if (object_id == NULL_WORLD_ID)
			{
				editor::get().get_gui_controller().set_selected_entity({});
				return true;
			}

			const world_handle handle = em.get_valid_handle_by_index(object_id);
			if (em.is_valid(handle))
			{
				editor::get().get_gui_controller().set_selected_entity(handle);
				return true;
			}

			return false;
		}

		_gizmo_active_axis	 = picked;
		_gizmo_dragging		 = true;
		_gizmo_center_handle = false;
		_gizmo_drag_amount	 = 0.0f;
		_gizmo_start_pos	 = em.get_entity_position_abs(selected);
		_gizmo_start_rot	 = em.get_entity_rotation_abs(selected);
		_gizmo_start_scale	 = em.get_entity_scale_abs(selected);
		_gizmo_last_mouse	 = mp;
		_gizmo_drag_offset	 = vector3::zero;

		if (_gizmo_mode == gizmo_mode::rotation)
		{
			vector2 start_dir = mp - center_screen;
			if (!start_dir.is_zero())
				start_dir = start_dir.normalized();
			else
				start_dir = vector2(1.0f, 0.0f);
			_gizmo_angle_start_dir	 = start_dir;
			_gizmo_angle_current_dir = start_dir;
		}
		return true;
	}

}
