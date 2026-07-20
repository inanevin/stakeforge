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
#include "ui/widgets/editor_widget_world_view.hpp"
#include "commands/editor_commands_entity.hpp"
#include "ui/editor_payload_controller.hpp"
#include "ui/editor_global_toolbar.hpp"
#include "editor_surface_controller.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "assets/editor_asset_spawn.hpp"

#include <sfg/gfx/util/render_util.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/rectf.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
#define EDITOR_WORLD_VIEW_CAMERA_BASE_MOVE_SPEED  12.0f
#define EDITOR_WORLD_VIEW_CAMERA_BOOST_MULTIPLIER 8.0f

	namespace
	{
		ui::ui_resource_ref_t make_null_world_texture_ref()
		{
			ui::ui_resource_ref_t ref = {
				.handle = NULL_RESOURCE_HANDLE,
				.type	= ui::ui_resource_type_e::gpu_index_fof,
			};
			for (u8 i = 0; i < BACK_BUFFER_COUNT; ++i)
				ref.gpu_indices[i] = NULL_GPU_INDEX;
			return ref;
		}
	}

	void editor_widget_world_view_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui	  = &ui;
		_root = parent;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_world_view = ui.allocate_widget();
		ui.set_widget_debug_name(_world_view, "world_view");
		tree.attach(_root, _world_view);

		ui::layout_in_t& in = tree.in(_world_view);
		in.size_mode_x		= ui::axis_mode_e::parent_relative;
		in.size_mode_y		= ui::axis_mode_e::parent_relative;
		in.size_value		= {1.0f, 1.0f};
		in.flags |= ui::wf_input | ui::wf_focusable;

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = {1.0f, 1.0f, 1.0f, 1.0f};
		rect.fill_color_b		 = rect.fill_color_a;
		rect.filled				 = true;

		ui::ui_render_state_t state = {};
		state.pipeline				= "editor/resource_pack/shaders/editor_ui_texture.hlsl"_hs;
		state.constants[0]			= make_null_world_texture_ref();

		paint.set_rect(_world_view, rect, state);
		ui.set_pre_layout_tick(_world_view, on_world_view_tick, this);

		ui::listener_bundle_t listener = {};
		listener.on_press			   = on_world_view_press;
		listener.on_release			   = on_world_view_release;
		listener.on_hover_move		   = on_world_view_hover_move;
		listener.on_hover_exit		   = on_world_view_hover_exit;
		listener.on_drag			   = on_world_view_drag;
		listener.on_focus_lose		   = on_world_view_focus_lost;
		listener.on_key				   = on_world_view_key;
		listener.on_wheel			   = on_world_view_wheel;
		listener.user_data			   = this;
		ui.get_input().set_listener(_world_view, listener);

		_toolbars.init(ui, _world_view);

		_empty_label = ui.allocate_widget();
		ui.set_widget_debug_name(_empty_label, "empty_label");
		tree.attach(_root, _empty_label);

		ui::layout_in_t& label_in = tree.in(_empty_label);
		label_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value		  = {0.5f, 0.5f};
		label_in.anchor_x		  = ui::anchor_e::center;
		label_in.anchor_y		  = ui::anchor_e::center;

		ui.set_widget_text(_empty_label, "NO WORLD AVAILABLE");
		paint.set_text(_empty_label,
					   ui.widget_text(_empty_label),
					   ui.widget_text_len(_empty_label),
					   {.font = theme.font_title_bold, .color = theme.color_text2, .point_size = theme.text_big_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		editor_payload_controller_t::get().register_listener(on_payload_drop, nullptr, nullptr, this);
		clear_world();
	}

	void editor_widget_world_view_t::uninit()
	{
		cancel_gizmo_action();
		end_camera_control();
		editor_payload_controller_t::get().unregister_listener(this);
		_toolbars.uninit();
		_ui->deallocate_widget(_empty_label);
		_ui->deallocate_widget(_world_view);
		_edit_world			  = {};
		_last_resize_request  = vec2u16_t::zero;
		_camera_runtime		  = nullptr;
		_empty_label		  = NULL_WIDGET;
		_world_view			  = NULL_WIDGET;
		_root				  = NULL_WIDGET;
		_resize_ticks		  = 0;
		_camera_control		  = false;
		_gizmo_press_consumed = false;
		_ui					  = nullptr;
	}

	void editor_widget_world_view_t::set_edit_world(editor_world_handle_t world)
	{
		cancel_gizmo_action();
		_toolbars.set_edit_world(world);
		if (world.is_null())
		{
			end_camera_control();
			_edit_world = world;
			clear_world();
			return;
		}

		_edit_world = world;
		request_world_resize(true);
		refresh_world_texture();

		ui::layout_tree_t& tree = _ui->get_tree();
		tree.set_visible(_world_view, true);
		tree.set_visible(_empty_label, false);
	}

	vec2i16_t editor_widget_world_view_t::get_center() const
	{
		const ui::layout_out_t& out = _ui->get_tree().out(_world_view);
		return {
			static_cast<i16>(out.pos.x + out.size.x * 0.5f),
			static_cast<i16>(out.pos.y + out.size.y * 0.5f),
		};
	}

	bool editor_widget_world_view_t::on_window_event(window_runtime_t& runtime, const window_event_t& ev)
	{
		s_event_runtime = &runtime;
		if (ev.type == window_event_type_e::focus && ev.sub_type == window_event_sub_type_e::release)
		{
			reset_camera_input(runtime);
			return false;
		}

		editor_widget_world_view_t* const active = s_active_camera_view;
		if (active == nullptr || active->_camera_runtime != &runtime || !active->_camera_control)
			return false;

		if (ev.type == window_event_type_e::delta)
		{
			active->pass_camera_input({.mouse_delta = {static_cast<f32>(ev.value.x), static_cast<f32>(ev.value.y)}});
			return true;
		}

		if (ev.type == window_event_type_e::key)
			return active->pass_camera_key(ev);

		return false;
	}

	void editor_widget_world_view_t::reset_camera_input(window_runtime_t& runtime)
	{
		if (s_active_camera_view != nullptr && s_active_camera_view->_camera_runtime == &runtime)
			s_active_camera_view->end_camera_control();
	}

	void editor_widget_world_view_t::clear_world()
	{
		ui::paint_def_t& def		  = _ui->get_paint().def(_world_view);
		def.render_state.pipeline	  = "editor/resource_pack/shaders/editor_ui_texture.hlsl"_hs;
		def.render_state.constants[0] = make_null_world_texture_ref();

		ui::layout_tree_t& tree = _ui->get_tree();
		tree.set_visible(_world_view, false);
		tree.set_visible(_empty_label, true);
	}

	void editor_widget_world_view_t::request_world_resize(bool force)
	{
		if (_edit_world.is_null())
			return;

		const ui::layout_out_t& out = _ui->get_tree().out(_world_view);

		vec2u16_t resolution{
			.x = static_cast<u16>(math::clamp(out.size.x, 0.0f, 65535.0f) + 0.5f),
			.y = static_cast<u16>(math::clamp(out.size.y, 0.0f, 65535.0f) + 0.5f),
		};

		render_util_t::ensure_world_resolution(resolution);
		if (!force && _last_resize_request == resolution)
			return;

		_last_resize_request = resolution;
		editor_world_controller_t::get().resize_world(_edit_world, resolution);
	}

	void editor_widget_world_view_t::begin_camera_control(window_runtime_t& runtime)
	{
		if (_edit_world.is_null())
			return;

		const editor_play_mode_e play_mode = editor_global_toolbar_t::get().get_play_mode();
		if (play_mode == editor_play_mode_e::play || play_mode == editor_play_mode_e::play_paused)
			return;

		if (s_active_camera_view != nullptr && s_active_camera_view != this)
			s_active_camera_view->end_camera_control();

		editor_surface_controller_t::get().begin_editor_camera_cursor_capture(runtime);
		_camera_runtime		 = &runtime;
		_camera_control		 = true;
		s_active_camera_view = this;
		pass_camera_input({.reset = true});
	}

	void editor_widget_world_view_t::end_camera_control()
	{
		if (!_camera_control)
			return;

		editor_surface_controller_t::get().end_editor_camera_cursor_capture(*_camera_runtime);
		pass_camera_input({.reset = true});

		_camera_runtime = nullptr;
		_camera_control = false;

		if (s_active_camera_view == this)
			s_active_camera_view = nullptr;
	}

	void editor_widget_world_view_t::pass_camera_input(const editor_world_camera_input_t& input)
	{
		editor_world_t* const world = editor_world_controller_t::get().get_editor_world(_edit_world);
		world->pass_camera_input(input);
	}

	bool editor_widget_world_view_t::pass_camera_key(const window_event_t& ev)
	{
		if (ev.sub_type != window_event_sub_type_e::press && ev.sub_type != window_event_sub_type_e::release)
			return false;

		const f32					sign  = ev.sub_type == window_event_sub_type_e::press ? 1.0f : -1.0f;
		editor_world_camera_input_t input = {};
		if (ev.button == static_cast<u16>(input_code::key_w))
			input.direction_delta.z = sign;
		else if (ev.button == static_cast<u16>(input_code::key_s))
			input.direction_delta.z = -sign;
		else if (ev.button == static_cast<u16>(input_code::key_d))
			input.direction_delta.x = sign;
		else if (ev.button == static_cast<u16>(input_code::key_a))
			input.direction_delta.x = -sign;
		else if (ev.button == static_cast<u16>(input_code::key_e))
			input.direction_delta.y = sign;
		else if (ev.button == static_cast<u16>(input_code::key_q))
			input.direction_delta.y = -sign;
		else if (ev.button == static_cast<u16>(input_code::key_lshift) || ev.button == static_cast<u16>(input_code::key_rshift))
		{
			input.set_move_speed = true;
			input.move_speed	 = ev.sub_type == window_event_sub_type_e::press ? EDITOR_WORLD_VIEW_CAMERA_BASE_MOVE_SPEED * EDITOR_WORLD_VIEW_CAMERA_BOOST_MULTIPLIER : EDITOR_WORLD_VIEW_CAMERA_BASE_MOVE_SPEED;
		}
		else
			return false;

		pass_camera_input(input);
		return true;
	}

	vec2f_t editor_widget_world_view_t::calculate_relative_position(const vec2f_t& position) const
	{
		const ui::layout_out_t& out = _ui->get_tree().out(_world_view);
		return {
			(position.x - out.pos.x) / out.size.x,
			(position.y - out.pos.y) / out.size.y,
		};
	}

	void editor_widget_world_view_t::cancel_gizmo_action()
	{
		if (!_edit_world.is_null())
		{
			editor_world_t* world = editor_world_controller_t::get().get_editor_world(_edit_world);
			world->cancel_gizmo_action();
			world->clear_gizmo_hover();
		}
		_gizmo_press_consumed = false;
	}

	void editor_widget_world_view_t::refresh_world_texture()
	{
		ui::ui_resource_ref_t texture_ref = {
			.handle = NULL_RESOURCE_HANDLE,
			.type	= ui::ui_resource_type_e::gpu_index_fof,
		};

		editor_world_t* const world = editor_world_controller_t::get().get_editor_world(_edit_world);
		for (u8 i = 0; i < BACK_BUFFER_COUNT; ++i)
			texture_ref.gpu_indices[i] = world->get_render_context().get_world_texture_index(i);

		ui::paint_def_t& def		  = _ui->get_paint().def(_world_view);
		def.render_state.pipeline	  = "editor/resource_pack/shaders/editor_ui_texture.hlsl"_hs;
		def.render_state.constants[0] = texture_ref;
	}

	void editor_widget_world_view_t::on_world_view_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);
		if (widget._edit_world.is_null())
			return;

		++widget._resize_ticks;
		if (widget._resize_ticks >= 5)
		{
			widget._resize_ticks = 0;
			widget.request_world_resize(false);
		}
		if (!widget._gizmo_press_consumed && widget._ui->get_input().get_hovered() == widget._world_view)
		{
			const vec2f_t& mouse = widget._ui->get_input().get_mouse_position();
			editor_world_controller_t::get().get_editor_world(widget._edit_world)->update_gizmo_hover(widget.calculate_relative_position(mouse));
		}
		widget.refresh_world_texture();
	}

	void editor_widget_world_view_t::on_world_view_press(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);
		SFG_ASSERT(s_event_runtime != nullptr);

		if (btn == ui::mouse_button_e::right)
		{
			SFG_TRACE("press");
			widget.begin_camera_control(*s_event_runtime);
		}
		else if (btn == ui::mouse_button_e::left && !widget._edit_world.is_null())
		{
			editor_world_t* world		 = editor_world_controller_t::get().get_editor_world(widget._edit_world);
			widget._gizmo_press_consumed = world->begin_gizmo_action(widget.calculate_relative_position(pos));
		}
	}

	void editor_widget_world_view_t::on_world_view_release(ui::input_router_t& router, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);

		if (btn == ui::mouse_button_e::right)
			widget.end_camera_control();
		else if (btn == ui::mouse_button_e::left && widget._gizmo_press_consumed)
		{
			editor_world_t* world			  = editor_world_controller_t::get().get_editor_world(widget._edit_world);
			const vec2f_t	relative_position = widget.calculate_relative_position(pos);
			world->update_gizmo_action(relative_position);
			world->end_gizmo_action();
			world->update_gizmo_hover(relative_position);
			widget._gizmo_press_consumed = false;
		}
		else if (btn == ui::mouse_button_e::left && !widget._edit_world.is_null() && router.get_hovered() == widget._world_view)
		{
			const vec2f_t relative_position		= widget.calculate_relative_position(pos);
			const bool	  incremental_selection = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));
			editor_world_controller_t::get().get_editor_world(widget._edit_world)->request_entity_pick(relative_position, incremental_selection);
		}
	}

	void editor_widget_world_view_t::on_world_view_hover_move(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);
		if (!widget._edit_world.is_null() && !widget._gizmo_press_consumed)
			editor_world_controller_t::get().get_editor_world(widget._edit_world)->update_gizmo_hover(widget.calculate_relative_position(pos));
	}

	void editor_widget_world_view_t::on_world_view_hover_exit(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);
		if (!widget._edit_world.is_null())
			editor_world_controller_t::get().get_editor_world(widget._edit_world)->clear_gizmo_hover();
	}

	void editor_widget_world_view_t::on_world_view_drag(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);
		if (widget._gizmo_press_consumed)
			editor_world_controller_t::get().get_editor_world(widget._edit_world)->update_gizmo_action(widget.calculate_relative_position(pos));
	}

	void editor_widget_world_view_t::on_world_view_focus_lost(ui::input_router_t&, ui::widget_id_t, bool, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);
		widget.cancel_gizmo_action();
		widget.end_camera_control();
	}

	void editor_widget_world_view_t::on_world_view_key(ui::input_router_t&, ui::widget_id_t, const ui::key_event_t& ev, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);
		if (ev.action != ui::key_action_e::press || widget._edit_world.is_null())
			return;

		if (ev.key == static_cast<u16>(input_code::key_escape))
		{
			if (widget._gizmo_press_consumed)
				widget.cancel_gizmo_action();
			return;
		}

		const bool ctrl_pressed = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));
		if (ev.key == static_cast<u16>(input_code::key_x) || (ev.key == static_cast<u16>(input_code::key_d) && ctrl_pressed))
		{
			widget.cancel_gizmo_action();
			editor_world_t*					editor_world = editor_world_controller_t::get().get_editor_world(widget._edit_world);
			const span_t<const entity_id_t> selected	 = editor_world->get_edit_context().get_selected_entities();
			if (selected.size == 0)
				return;

			frame_vector_t<entity_id_t> entities;
			entities.resize(selected.size);
			entities.resize(editor_world->get_edit_context().collect_selected_mutable_root_entities(editor_world->get_world(), {.data = entities.data(), .size = entities.size()}));
			if (ev.key == static_cast<u16>(input_code::key_x))
				editor_commands_entity_t::destroy(widget._edit_world, entities);
			else
			{
				frame_vector_t<entity_id_t> duplicates;
				editor_commands_entity_t::duplicate(widget._edit_world, entities, duplicates);
			}
			return;
		}

		if (ev.key == static_cast<u16>(input_code::key_alpha1))
		{
			widget.cancel_gizmo_action();
			widget._toolbars.set_transform_control_type(editor_transform_control_type_e::move);
		}
		else if (ev.key == static_cast<u16>(input_code::key_alpha2))
		{
			widget.cancel_gizmo_action();
			widget._toolbars.set_transform_control_type(editor_transform_control_type_e::rotate);
		}
		else if (ev.key == static_cast<u16>(input_code::key_alpha3))
		{
			widget.cancel_gizmo_action();
			widget._toolbars.set_transform_control_type(editor_transform_control_type_e::scale);
		}
		else if (ev.key == static_cast<u16>(input_code::key_alpha4))
		{
			widget.cancel_gizmo_action();
			widget._toolbars.toggle_transform_locality();
		}
		else if (ev.key == static_cast<u16>(input_code::key_alpha5))
			widget._toolbars.toggle_transform_snapping();
	}

	void editor_widget_world_view_t::on_world_view_wheel(ui::input_router_t&, ui::widget_id_t, f32 delta, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);
		if (!widget._edit_world.is_null())
			widget.pass_camera_input({.wheel_delta = delta});
	}

	bool editor_widget_world_view_t::on_payload_drop(const editor_payload_t& payload, void* user_data)
	{
		if (payload.type != editor_payload_type_e::asset && payload.type != editor_payload_type_e::asset_multi)
			return false;

		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);
		if (widget._edit_world.is_null())
			return false;

		const ui::layout_out_t& out	  = widget._ui->get_tree().out(widget._world_view);
		const vec2f_t&			mouse = widget._ui->get_input().get_mouse_position();
		if (!rectf_t{out.pos.x, out.pos.y, out.size.x, out.size.y}.contains(mouse))
			return false;

		return editor_asset_spawn_t::spawn_from_payload({
			.payload	= &payload,
			.screen_pos = mouse,
			.world		= widget._edit_world,
			.parent		= NULL_ENTITY_ID,
		});
	}
}
