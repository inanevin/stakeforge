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
#include "editor_panel_world.hpp"
#include "editor/editor.hpp"
#include "editor/editor_layout.hpp"
#include "editor/editor_theme.hpp"
#include "editor/editor_settings.hpp"
#include "editor/gui/editor_panel_stats.hpp"

// math
#include "math/vector2ui16.hpp"
#include "math/math.hpp"

// gui
#include "gui/vekt.hpp"
#include "gui/icon_defs.hpp"

// platform
#include "platform/window.hpp"
#include "platform/process.hpp"

// misc
#include "world/world.hpp"
#include "app/app.hpp"
#include "gfx/renderer.hpp"
#include "game/game_world_renderer.hpp"
#include "input/input_mappings.hpp"

#include "app/package_manager.hpp"

namespace SFG
{

	namespace
	{
		vector2 fit_aspect_inside(const vector2& container, f32 aspect_w_over_h)
		{
			// Fit a rectangle of aspect (w/h) fully inside container, maximizing size.
			vector2 out;

			out.x = container.x;
			out.y = out.x / aspect_w_over_h;

			if (out.y > container.y)
			{
				out.y = container.y;
				out.x = out.y * aspect_w_over_h;
			}

			// Safety clamps (handles tiny floating point overshoots)
			out.x = math::min(out.x, container.x);
			out.y = math::min(out.y, container.y);

			return out;
		}

	}
	void editor_panel_world::init(vekt::builder* b)
	{
		_user_data.type = editor_gui_user_data_type::world_rt;

		_builder								= b;
		_gui_builder.callbacks.callback_ud		= this;
		_gui_builder.callbacks.user_data		= this;
		_gui_builder.callbacks.on_toggle_button = on_toggle_button;
		_gui_builder.callbacks.on_mouse			= on_widget_mouse;
		_gui_builder.init(b);
		_gizmo_controls.init(b);
		_gizmo_2d.init(b);

		vekt::widget_gfx& root = b->widget_get_gfx(_gui_builder.get_root());
		root.color			   = editor_theme::get().col_frame_bg;

		// world viewer
		_world_viewer = b->allocate();
		{
			vekt::pos_props& pp = b->widget_get_pos_props(_world_viewer);
			pp.flags			= vekt::pos_flags::pf_x_relative | vekt::pos_flags::pf_y_relative | vekt::pos_flags::pf_x_anchor_center | vekt::pos_flags::pf_y_anchor_center;
			pp.pos.x			= 0.5f;
			pp.pos.y			= 0.5f;

			vekt::size_props& sz = b->widget_get_size_props(_world_viewer);
			sz.flags			 = vekt::size_flags::sf_x_abs | vekt::size_flags::sf_y_abs;
			sz.child_margins	 = {editor_theme::get().outer_margin, editor_theme::get().outer_margin, editor_theme::get().outer_margin, editor_theme::get().outer_margin};

			vekt::widget_gfx& gfx = b->widget_get_gfx(_world_viewer);
			gfx.flags			  = vekt::gfx_flags::gfx_is_rect | vekt::gfx_flags::gfx_custom_pass | vekt::gfx_flags::gfx_clip_children;
			gfx.color			  = editor_theme::get().col_frame_bg;
			gfx.user_data		  = &_user_data;

			vekt::mouse_callback& mb = b->widget_get_mouse_callbacks(_world_viewer);
			mb.on_mouse				 = on_widget_mouse;

			b->widget_get_user_data(_world_viewer).ptr				  = this;
			b->widget_get_custom_pass(_world_viewer).custom_draw_pass = on_widget_draw;

			b->widget_add_child(_gui_builder.get_root(), _world_viewer);
		}

		build_toolbar();

		_gizmo_controls.init(_builder);

		set_aspect(static_cast<aspect_ratio>(math::clamp(editor_layout::get().world_aspect_ratio, (u8)0, static_cast<u8>(aspect_ratio::aspect_1_1))));
		set_gizmo_style(gizmo_style::move);
		set_gizmo_space(gizmo_space::global);
		set_audio_style(audio_style::on);
		set_physics_debug(physics_debug_style::none);
	}

	void editor_panel_world::fetch_stats()
	{
		set_stats_view(static_cast<stats_view_style>(math::clamp(editor_layout::get().world_stats_view, (u8)0, static_cast<u8>(stats_view_style::full))));
	}

	void editor_panel_world::build_toolbar()
	{
		if (_toolbar_row != NULL_WIDGET_ID)
			return;

		_gui_builder.set_draw_order(1);
		_toolbar_row = _gui_builder.begin_row();
		_btn_file	 = _gui_builder.add_icon_button(ICON_FILE, 0, 1.25f, true).first;
		_btn_stats	 = _gui_builder.add_toggle_button(ICON_INFO, false, 0, 1.25f, true).first;
		_gui_builder.add_row_separator(editor_theme::get().col_frame_bg);
		_btn_translate = _gui_builder.add_toggle_button(ICON_MOVE, true, 0, 1.25f, true).first;
		_btn_rotate	   = _gui_builder.add_toggle_button(ICON_ROTATE, false, 0, 1.25f, true).first;
		_btn_scale	   = _gui_builder.add_toggle_button(ICON_SCALE, false, 0, 1.25f, true).first;
		_btn_space	   = _gui_builder.add_toggle_button(ICON_WORLD, true, 0, 1.25f, true).first;
		_gui_builder.add_row_separator(editor_theme::get().col_frame_bg);
		_btn_aspect = _gui_builder.add_icon_button(ICON_GLASSES, 0, 1.25f, true).first;
		_gui_builder.add_row_separator(editor_theme::get().col_frame_bg);
		_btn_mute		   = _gui_builder.add_toggle_button(ICON_AUDIO_MUTE, true, 0, 1.25f, true).first;
		_btn_physics_debug = _gui_builder.add_toggle_button(ICON_CUBES, true, 0, 1.25f, true).first;
		_btn_world_view	   = _gui_builder.add_icon_button(ICON_EYE, 0, 1.25f, true).first;
		_gui_builder.add_row_separator(editor_theme::get().col_frame_bg);
		_btn_play = _gui_builder.add_toggle_button(ICON_PLAY, false, 0, 1.25f, true, editor_theme::get().col_accent_third).first;
		_gui_builder.end_row();
		_gui_builder.set_draw_order(0);
		_builder->build_hierarchy();
	}

	void editor_panel_world::destroy_toolbar()
	{
		if (_toolbar_row == NULL_WIDGET_ID)
			return;

		_gui_builder.deallocate(_toolbar_row);
		_toolbar_row	   = NULL_WIDGET_ID;
		_btn_file		   = NULL_WIDGET_ID;
		_btn_stats		   = NULL_WIDGET_ID;
		_btn_translate	   = NULL_WIDGET_ID;
		_btn_rotate		   = NULL_WIDGET_ID;
		_btn_scale		   = NULL_WIDGET_ID;
		_btn_space		   = NULL_WIDGET_ID;
		_btn_mute		   = NULL_WIDGET_ID;
		_btn_physics_debug = NULL_WIDGET_ID;
		_btn_aspect		   = NULL_WIDGET_ID;
		_btn_world_view	   = NULL_WIDGET_ID;
		_btn_play		   = NULL_WIDGET_ID;
		_builder->build_hierarchy();
	}

	void editor_panel_world::uninit()
	{
		_gui_builder.uninit();
	}

	void editor_panel_world::draw(const vector2ui16& window_size)
	{
		const vector2	root_sz = _builder->widget_get_size(_gui_builder.get_root());
		constexpr f32 ap_16_9 = 16.0f / 9.0f;
		constexpr f32 ap_4_3	= 4.0f / 3.0f;

		vekt::size_props& sz = _builder->widget_get_size_props(_world_viewer);

		switch (_aspect)
		{
		case aspect_ratio::native:
			sz.size = root_sz;
			break;

		case aspect_ratio::aspect_16_9:
			sz.size = fit_aspect_inside(root_sz, ap_16_9);
			break;

		case aspect_ratio::aspect_4_3:
			sz.size = fit_aspect_inside(root_sz, ap_4_3);
			break;

		case aspect_ratio::aspect_1_1: {
			const f32 m = math::min(root_sz.x, root_sz.y);
			sz.size		  = vector2(m, m);
			break;
		}
		}

		draw_world_axes();
	}

	void editor_panel_world::draw_world_axes()
	{
	}

	const vector2ui16 editor_panel_world::get_world_size() const
	{
		const vector2 sz = _builder->widget_get_size(_world_viewer);
		return vector2ui16(sz.x, sz.y);
	}

	void editor_panel_world::set_gizmo_style(gizmo_style style)
	{
		_gizmo_controls.set_style(style);
		_gui_builder.set_toggle_button_state(_btn_translate, style == gizmo_style::move);
		_gui_builder.set_toggle_button_state(_btn_rotate, style == gizmo_style::rotate);
		_gui_builder.set_toggle_button_state(_btn_scale, style == gizmo_style::scale);
	}

	void editor_panel_world::set_gizmo_space(gizmo_space space)
	{
		_gizmo_controls.set_space(space);
		_gui_builder.set_toggle_button_state(_btn_space, space == gizmo_space::global);
	}

	void editor_panel_world::set_aspect(aspect_ratio aspect)
	{
		_aspect									= aspect;
		editor_layout::get().world_aspect_ratio = static_cast<u8>(aspect);
		editor_layout::get().save_last();
	}

	void editor_panel_world::set_physics_debug(physics_debug_style style)
	{
		_physics_style = style;
		_gui_builder.set_toggle_button_state(_btn_physics_debug, style == physics_debug_style::on);
		editor::get().get_app().get_renderer().get_world_renderer()->get_render_pass_debug_rendering().set_physics_off(style == physics_debug_style::none);
	}

	void editor_panel_world::set_audio_style(audio_style aud)
	{
		_audio_style = aud;
		_gui_builder.set_toggle_button_state(_btn_mute, aud == audio_style::mute);
		editor::get().get_app().get_world().get_audio_manager().set_engine_volume(aud == audio_style::mute ? 0.0f : 1.0f);
	}

	void editor_panel_world::set_stats_view(stats_view_style style)
	{
		_stats_style = style;
		_gui_builder.set_toggle_button_state(_btn_stats, style == stats_view_style::full);
		editor_layout::get().world_stats_view = static_cast<u8>(style);
		editor_layout::get().save_last();
		editor::get().get_gui_controller().get_stats()->set_visible(style == stats_view_style::full);
	}

	void editor_panel_world::set_playmode(playmode m)
	{
		_play_mode = m;
		if (_btn_play != NULL_WIDGET_ID)
			_gui_builder.set_toggle_button_state(_btn_play, m == playmode::playing);

		if (m == playmode::playing)
			editor::get().enter_playmode(false);
		else if (m == playmode::physics)
			editor::get().enter_playmode(true);
		else
			editor::get().exit_playmode();

		if (m == playmode::none)
			build_toolbar();
		else
			destroy_toolbar();
	}

	void editor_panel_world::on_context_item(vekt::id widget)
	{
		if (widget == _ctx_aspect_native)
		{
			set_aspect(aspect_ratio::native);
			return;
		}
		if (widget == _ctx_aspect_169)
		{
			set_aspect(aspect_ratio::aspect_16_9);
			return;
		}
		if (widget == _ctx_aspect_43)
		{
			set_aspect(aspect_ratio::aspect_4_3);
			return;
		}
		if (widget == _ctx_aspect_11)
		{
			set_aspect(aspect_ratio::aspect_1_1);
			return;
		}
		if (widget == _ctx_view_rt)
		{
			_user_data.type = editor_gui_user_data_type::world_rt;
			return;
		}
		if (widget == _ctx_view_albedo)
		{
			_user_data.type = editor_gui_user_data_type::colors_rt;
			return;
		}
		if (widget == _ctx_view_normals)
		{
			_user_data.type = editor_gui_user_data_type::normals_rt;
			return;
		}
		if (widget == _ctx_view_orm)
		{
			_user_data.type = editor_gui_user_data_type::orm_rt;
			return;
		}
		if (widget == _ctx_view_emissive)
		{
			_user_data.type = editor_gui_user_data_type::emissive_rt;
			return;
		}
		if (widget == _ctx_view_depth)
		{
			_user_data.type = editor_gui_user_data_type::depth_rt;
			return;
		}
		if (widget == _ctx_view_lighting)
		{
			_user_data.type = editor_gui_user_data_type::lighting_rt;
			return;
		}
		if (widget == _ctx_view_ssao)
		{
			_user_data.type = editor_gui_user_data_type::ssao_rt;
			return;
		}
		if (widget == _ctx_view_bloom)
		{
			_user_data.type = editor_gui_user_data_type::bloom_rt;
			return;
		}
		if (widget == _ctx_new_project)
		{
			const string dir = process::select_folder("select project directory");
			if (!dir.empty())
				editor::get().new_project(dir.c_str());
			return;
		}
		if (widget == _ctx_open_project)
		{
			process::open_directory(editor_settings::get().working_dir.c_str());
			return;
		}
		if (widget == _ctx_package_project)
		{
			vector<string> files;
			process::select_files("select worlds to package", "stkworld", files);
			if (files.empty())
				return;

			vector<string> relatives;
			for (const string& f : files)
			{
				if (editor_settings::get().is_in_work_directory(f))
				{
					SFG_ERR("skipping world to package because it's not inside working directory. {0}", f);
					continue;
				}

				relatives.push_back(editor_settings::get().get_relative(f));
			}

			const string output = process::select_folder("select package output directory");
			if (!output.empty())
				package_manager::get().package_project(relatives, output.c_str());
			return;
		}
		if (widget == _ctx_new_world)
		{
			open_save_popup(popup_action::new_world);
			return;
		}
		if (widget == _ctx_save_world)
		{
			editor::get().save_lavel();
			return;
		}
		if (widget == _ctx_load_world)
		{
			open_save_popup(popup_action::load_world);
			return;
		}
	}

	void editor_panel_world::on_popup_item(vekt::id widget)
	{
		const popup_action action = _popup_action;

		if (widget == _popup_save_yes)
		{
			editor::get().get_gui_controller().kill_popup();
			editor::get().save_lavel();

			if (action == popup_action::new_world)
				editor::get().new_level();
			else if (action == popup_action::load_world)
				editor::get().load_level_prompt();

			return;
		}

		if (widget == _popup_save_no)
		{
			editor::get().get_gui_controller().kill_popup();

			if (action == popup_action::new_world)
				editor::get().new_level();
			else if (action == popup_action::load_world)
				editor::get().load_level_prompt();

			return;
		}
	}

	void editor_panel_world::open_save_popup(popup_action action)
	{
		_popup_action			   = action;
		editor_gui_controller& ctr = editor::get().get_gui_controller();
		ctr.begin_popup("do you want to save the current level?");
		_popup_save_yes = ctr.popup_add_button("yes");
		_popup_save_no	= ctr.popup_add_button("no");
		ctr.end_popup();
	}

	vekt::input_event_result editor_panel_world::on_widget_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase)
	{
		editor_panel_world* self = static_cast<editor_panel_world*>(b->widget_get_user_data(widget).ptr);

		if (ev.button == input_code::mouse_0 && widget == self->_world_viewer)
		{
			const window_event eev = {
				.value	  = vector2i16(ev.position.x, ev.position.y),
				.button	  = static_cast<u16>(ev.button),
				.type	  = window_event_type::mouse,
				.sub_type = static_cast<window_event_sub_type>(ev.type),
			};
			if (self->_gizmo_2d.on_mouse_event(eev))
				return vekt::input_event_result::handled;

			if (self->_gizmo_controls.on_mouse_event(eev))
				return vekt::input_event_result::handled;

			if (ev.type == vekt::input_event_type::pressed)
			{
				const vector2 local		 = ev.position - b->widget_get_pos(self->_world_viewer);
				const vector2 panel_size = b->widget_get_size(self->_world_viewer);
				const u16  x			 = static_cast<u16>(math::clamp(local.x, 0.0f, panel_size.x - 1.0f));
				const u16  y			 = static_cast<u16>(math::clamp(local.y, 0.0f, panel_size.y - 1.0f));

				const renderer& rend		= editor::get().get_app().get_renderer();
				const u8		frame_index = rend.get_frame_index();
				const u32	object_id	= rend.get_world_renderer()->get_render_pass_object_id().read_location(x, y, frame_index);

				editor_gui_controller& controller = editor::get().get_gui_controller();

				if (object_id == NULL_WORLD_ID)
				{
					controller.set_selected_entity({});
					return vekt::input_event_result::handled;
				}

				entity_manager&	   em	  = editor::get().get_app().get_world().get_entity_manager();
				const world_handle handle = em.get_valid_handle_by_index(object_id);
				if (em.is_valid(handle))
					controller.set_selected_entity(handle);
			}

			return vekt::input_event_result::handled;
		}

		if (ev.button == input_code::mouse_1 && widget == self->_world_viewer && ev.type == vekt::input_event_type::pressed)
		{
			editor::get().get_camera().set_is_looking(true);
			return vekt::input_event_result::handled;
		}

		if (widget == self->_btn_file)
		{
			editor_gui_controller& ctr = editor::get().get_gui_controller();
			const vector2		   pos = b->widget_get_pos(widget) + vector2(0.0f, b->widget_get_size(widget).y);
			ctr.begin_context_menu(pos.x, pos.y);
			ctr.add_context_menu_title("project");
			self->_ctx_new_project	   = ctr.add_context_menu_item("new_project");
			self->_ctx_open_project	   = ctr.add_context_menu_item("open_project");
			self->_ctx_save_project	   = ctr.add_context_menu_item("save_project");
			self->_ctx_package_project = ctr.add_context_menu_item("package_project");
			ctr.add_context_menu_title("world");
			self->_ctx_new_world  = ctr.add_context_menu_item("new_world");
			self->_ctx_save_world = ctr.add_context_menu_item("save_world");
			self->_ctx_load_world = ctr.add_context_menu_item("load_world");
			ctr.end_context_menu();
			return vekt::input_event_result::handled;
		}

		if (widget == self->_btn_aspect)
		{
			editor_gui_controller& ctr = editor::get().get_gui_controller();
			const vector2		   pos = b->widget_get_pos(widget) + vector2(0.0f, b->widget_get_size(widget).y);
			ctr.begin_context_menu(pos.x, pos.y);
			self->_ctx_aspect_native = ctr.add_context_menu_item_toggle("native", self->_aspect == aspect_ratio::native);
			self->_ctx_aspect_169	 = ctr.add_context_menu_item_toggle("16:9", self->_aspect == aspect_ratio::aspect_16_9);
			self->_ctx_aspect_43	 = ctr.add_context_menu_item_toggle("4:3", self->_aspect == aspect_ratio::aspect_4_3);
			self->_ctx_aspect_11	 = ctr.add_context_menu_item_toggle("1:1", self->_aspect == aspect_ratio::aspect_1_1);
			ctr.end_context_menu();
			return vekt::input_event_result::handled;
		}

		if (widget == self->_btn_world_view)
		{
			editor_gui_controller& ctr = editor::get().get_gui_controller();
			const vector2		   pos = b->widget_get_pos(widget) + vector2(0.0f, b->widget_get_size(widget).y);
			ctr.begin_context_menu(pos.x, pos.y);

			ctr.add_context_menu_title("result");

			self->_ctx_view_rt		 = ctr.add_context_menu_item_toggle("final_output", self->_user_data.type == editor_gui_user_data_type::world_rt);
			self->_ctx_view_depth	 = ctr.add_context_menu_item_toggle("depth", self->_user_data.type == editor_gui_user_data_type::depth_rt);
			self->_ctx_view_lighting = ctr.add_context_menu_item_toggle("lighting", self->_user_data.type == editor_gui_user_data_type::lighting_rt);

			ctr.add_context_menu_title("gbuffer");
			self->_ctx_view_albedo	 = ctr.add_context_menu_item_toggle("albedo", self->_user_data.type == editor_gui_user_data_type::colors_rt);
			self->_ctx_view_normals	 = ctr.add_context_menu_item_toggle("oct_normal", self->_user_data.type == editor_gui_user_data_type::normals_rt);
			self->_ctx_view_emissive = ctr.add_context_menu_item_toggle("emissive", self->_user_data.type == editor_gui_user_data_type::emissive_rt);
			self->_ctx_view_orm		 = ctr.add_context_menu_item_toggle("orm", self->_user_data.type == editor_gui_user_data_type::orm_rt);

			ctr.add_context_menu_title("compute");

			self->_ctx_view_ssao  = ctr.add_context_menu_item_toggle("ssao", self->_user_data.type == editor_gui_user_data_type::ssao_rt);
			self->_ctx_view_bloom = ctr.add_context_menu_item_toggle("bloom", self->_user_data.type == editor_gui_user_data_type::bloom_rt);

			ctr.end_context_menu();
			return vekt::input_event_result::handled;

			return vekt::input_event_result::handled;
		}

		return vekt::input_event_result::not_handled;
	}

	bool editor_panel_world::on_mouse_move(const vector2& p)
	{
		if (_play_mode != playmode::none)
			return false;
		return _gizmo_controls.on_mouse_move(p);
	}

	bool editor_panel_world::on_key_event(const window_event& ev)
	{
		if (ev.sub_type != window_event_sub_type::press)
			return false;

		const bool is_ctrl = window::is_key_down(input_code::key_lctrl);

		if (ev.button == input_code::key_alpha1 && is_ctrl)
		{
			set_gizmo_style(gizmo_style::move);
			return true;
		}

		if (ev.button == input_code::key_alpha2 && is_ctrl)
		{
			set_gizmo_style(gizmo_style::rotate);
			return true;
		}
		if (ev.button == input_code::key_alpha3 && is_ctrl)
		{
			set_gizmo_style(gizmo_style::scale);
			return true;
		}
		if (ev.button == input_code::key_alpha4 && is_ctrl)
		{
			set_gizmo_space(_gizmo_controls.get_space() == gizmo_space::global ? gizmo_space::local : gizmo_space::global);
			return true;
		}
		if (ev.button == input_code::key_m && is_ctrl)
		{
			set_audio_style(_audio_style == audio_style::on ? audio_style::mute : audio_style::on);
			return true;
		}

		if (ev.button == input_code::key_s && is_ctrl)
		{
			editor::get().save_lavel();
			return true;
		}

		return false;
	}

	void editor_panel_world::on_toggle_button(void* callback_ud, vekt::builder* b, vekt::id id, bool toggled)
	{
		editor_panel_world* self = static_cast<editor_panel_world*>(callback_ud);

		if (id == self->_btn_translate)
			self->set_gizmo_style(gizmo_style::move);
		else if (id == self->_btn_rotate)
			self->set_gizmo_style(gizmo_style::rotate);
		else if (id == self->_btn_scale)
			self->set_gizmo_style(gizmo_style::scale);
		else if (id == self->_btn_space)
			self->set_gizmo_space(toggled ? gizmo_space::global : gizmo_space::local);
		else if (id == self->_btn_mute)
			self->set_audio_style(toggled ? audio_style::mute : audio_style::on);
		else if (id == self->_btn_physics_debug)
			self->set_physics_debug(toggled ? physics_debug_style::on : physics_debug_style::none);
		else if (id == self->_btn_stats)
			self->set_stats_view(toggled ? stats_view_style::full : stats_view_style::none);
		else if (id == self->_btn_play)
			self->set_playmode(toggled ? playmode::playing : playmode::none);
	}

	void editor_panel_world::on_widget_draw(vekt::builder* b, vekt::id widget)
	{
		editor_panel_world* self = static_cast<editor_panel_world*>(b->widget_get_user_data(widget).ptr);
		if (self->_play_mode != playmode::none)
			return;
		const vector2 viewer_sz = b->widget_get_size(self->_world_viewer);
		self->_gizmo_controls.draw(b->widget_get_pos(self->_world_viewer), viewer_sz, viewer_sz);
		self->_gizmo_2d.draw(b->widget_get_pos(self->_world_viewer), viewer_sz, viewer_sz);

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

		const vector2 wv_pos = b->widget_get_pos(self->_world_viewer);

		constexpr f32 box_size_multiplier = 0.03f;
		constexpr f32 thickness			= 2.0f;
		const vector2	size				= viewer_sz;
		const vector2	box_size			= vector2(static_cast<f32>(size.x) * box_size_multiplier, static_cast<f32>(size.x) * box_size_multiplier);
		const vector2	min					= wv_pos + vector2(editor_theme::get().outer_margin + box_size.x * 0.5f, size.y - editor_theme::get().outer_margin - box_size.y);
		const vector2	max					= min + box_size;
		const vector2	center				= (min + max) * 0.5f;

		b->add_line_aa({
			.p0			  = vector2(center),
			.p1			  = center + x_dir * box_size.x,
			.color		  = editor_theme::get().color_axis_x,
			.thickness	  = thickness,
			.aa_thickness = 2,
			.draw_order	  = 15,
		});

		b->add_line_aa({
			.p0			  = vector2(center),
			.p1			  = center + y_dir * box_size.x,
			.color		  = editor_theme::get().color_axis_y,
			.thickness	  = thickness,
			.aa_thickness = 2,
			.draw_order	  = 15,
		});

		b->add_line_aa({
			.p0			  = vector2(center),
			.p1			  = center + z_dir * box_size.x,
			.color		  = editor_theme::get().color_axis_z,
			.thickness	  = thickness,
			.aa_thickness = 2,
			.draw_order	  = 15,

		});
	}
}
