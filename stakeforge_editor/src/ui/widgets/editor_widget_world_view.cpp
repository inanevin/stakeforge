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
#include "ui/editor_payload_controller.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"

#include <sfg/gfx/util/render_util.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/rectf.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/runtime/resources/font.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>

namespace sfg
{
#define EDITOR_WORLD_VIEW_AXIS_WIDGET_SCALE	  5.0f
#define EDITOR_WORLD_VIEW_AXIS_LENGTH_SCALE	  1.25f
#define EDITOR_WORLD_VIEW_AXIS_LABEL_OFFSET	  6.0f
#define EDITOR_WORLD_VIEW_AXIS_LINE_THICKNESS 2.0f

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

		_world_axes = ui.allocate_widget();
		ui.set_widget_debug_name(_world_axes, "world_view_axes");
		tree.attach(_world_view, _world_axes);
		tree.draw_order(_world_axes) = tree.draw_order_const(_world_view) + 1;

		ui::layout_in_t& axes_in = tree.in(_world_axes);
		axes_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		axes_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		axes_in.pos_value		 = {0.0f, 1.0f};
		axes_in.anchor_y		 = ui::anchor_e::end;
		axes_in.size_mode_x		 = ui::axis_mode_e::fixed;
		axes_in.size_mode_y		 = ui::axis_mode_e::fixed;
		axes_in.size_value		 = {theme.item_height * EDITOR_WORLD_VIEW_AXIS_WIDGET_SCALE, theme.item_height * EDITOR_WORLD_VIEW_AXIS_WIDGET_SCALE};
		axes_in.flags |= ui::wf_overlay;

		paint.set_custom(_world_axes, draw_world_axes, this);

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
		if (!_edit_world.is_null())
			editor_world_controller_t::get().get_editor_world(_edit_world)->get_input_controller().deactivate();

		editor_payload_controller_t::get().unregister_listener(this);
		_toolbars.uninit();

		_ui->deallocate_widget(_empty_label);
		_ui->deallocate_widget(_world_axes);
		_ui->deallocate_widget(_world_view);

		_edit_world			 = {};
		_last_resize_request = vec2u16_t::zero;
		_empty_label		 = NULL_WIDGET;
		_world_axes			 = NULL_WIDGET;
		_world_view			 = NULL_WIDGET;
		_root				 = NULL_WIDGET;
		_resize_ticks		 = 0;
		_ui					 = nullptr;
	}

	void editor_widget_world_view_t::set_edit_world(editor_world_handle_t world)
	{
		if (!_edit_world.is_null())
			editor_world_controller_t::get().get_editor_world(_edit_world)->get_input_controller().deactivate();

		if (world.is_null())
		{
			_toolbars.set_edit_world(world, editor_world_edit_type_e::view_only);
			_edit_world = world;
			clear_world();
			return;
		}

		_edit_world = world;

		const editor_world_edit_type_e edit_type = editor_world_controller_t::get().get_editor_world(world)->get_edit_context().get_edit_type();

		_toolbars.set_edit_world(world, edit_type);

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
		return editor_world_input_controller_t::on_window_event(runtime, ev);
	}

	void editor_widget_world_view_t::reset_camera_input(window_runtime_t& runtime)
	{
		editor_world_input_controller_t::reset_camera_input(runtime);
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

	vec2f_t editor_widget_world_view_t::calculate_relative_position(const vec2f_t& position) const
	{
		const ui::layout_out_t& out = _ui->get_tree().out(_world_view);
		return {
			(position.x - out.pos.x) / out.size.x,
			(position.y - out.pos.y) / out.size.y,
		};
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

		const bool				 hovered		   = widget._ui->get_input().get_hovered() == widget._world_view;
		const vec2f_t			 relative_position = hovered ? widget.calculate_relative_position(widget._ui->get_input().get_mouse_position()) : vec2f_t::zero;
		const ui::layout_out_t&	 out			   = widget._ui->get_tree().out(widget._world_view);
		editor_world_t* const	 editor_world	   = editor_world_controller_t::get().get_editor_world(widget._edit_world);
		const editor_play_mode_e play_mode		   = editor_world->get_play_mode();
		const bool				 full_play_mode	   = play_mode == editor_play_mode_e::play || play_mode == editor_play_mode_e::play_paused;
		ui::layout_tree_t&		 tree			   = widget._ui->get_tree();

		tree.set_visible(widget._world_axes, !full_play_mode);
		tree.set_visible(widget._toolbars.get_root(), !full_play_mode && editor_world->get_edit_context().get_edit_type() != editor_world_edit_type_e::view_only);

		if (out.size.x > 0.0f && out.size.y > 0.0f)
		{
			editor_world->get_world().get_screen().set_viewport({out.pos.x, out.pos.y, out.size.x, out.size.y}, editor_world->get_render_context().get_size(), widget._ui->get_dpi_scale());
		}

		editor_world->get_input_controller().tick(relative_position, hovered);
		widget.refresh_world_texture();
	}

	void editor_widget_world_view_t::on_world_view_press(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);

		if (widget._edit_world.is_null())
			return;

		editor_world_input_controller_t& input = editor_world_controller_t::get().get_editor_world(widget._edit_world)->get_input_controller();

		if (btn == ui::mouse_button_e::left)
			input.pointer_press(widget.calculate_relative_position(pos), editor_world_input_pointer_button_e::left);
		else if (btn == ui::mouse_button_e::right)
			input.pointer_press(widget.calculate_relative_position(pos), editor_world_input_pointer_button_e::right);
	}

	void editor_widget_world_view_t::on_world_view_release(ui::input_router_t& router, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);

		if (widget._edit_world.is_null())
			return;

		editor_world_input_controller_t& input			   = editor_world_controller_t::get().get_editor_world(widget._edit_world)->get_input_controller();
		const vec2f_t					 relative_position = widget.calculate_relative_position(pos);

		if (btn == ui::mouse_button_e::left)
			input.pointer_release(relative_position, editor_world_input_pointer_button_e::left, router.get_hovered() == widget._world_view);
		else if (btn == ui::mouse_button_e::right)
			input.pointer_release(relative_position, editor_world_input_pointer_button_e::right, router.get_hovered() == widget._world_view);
	}

	void editor_widget_world_view_t::on_world_view_hover_move(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);

		if (!widget._edit_world.is_null())
			editor_world_controller_t::get().get_editor_world(widget._edit_world)->get_input_controller().pointer_hover_move(widget.calculate_relative_position(pos));
	}

	void editor_widget_world_view_t::on_world_view_hover_exit(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);

		if (!widget._edit_world.is_null())
			editor_world_controller_t::get().get_editor_world(widget._edit_world)->get_input_controller().pointer_hover_exit();
	}

	void editor_widget_world_view_t::on_world_view_drag(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);

		if (!widget._edit_world.is_null())
			editor_world_controller_t::get().get_editor_world(widget._edit_world)->get_input_controller().pointer_drag(widget.calculate_relative_position(pos));
	}

	void editor_widget_world_view_t::on_world_view_focus_lost(ui::input_router_t&, ui::widget_id_t, bool, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);

		if (!widget._edit_world.is_null())
			editor_world_controller_t::get().get_editor_world(widget._edit_world)->get_input_controller().focus_lost();
	}

	void editor_widget_world_view_t::on_world_view_key(ui::input_router_t&, ui::widget_id_t, const ui::key_event_t& ev, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);

		if (ev.action != ui::key_action_e::press || widget._edit_world.is_null())
			return;

		editor_world_controller_t::get().get_editor_world(widget._edit_world)->get_input_controller().key_press(ev.key);
		widget._toolbars.refresh();
	}

	void editor_widget_world_view_t::on_world_view_wheel(ui::input_router_t&, ui::widget_id_t, f32 delta, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);

		if (!widget._edit_world.is_null())
			editor_world_controller_t::get().get_editor_world(widget._edit_world)->get_input_controller().wheel(delta);
	}

	void editor_widget_world_view_t::draw_world_axes(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		const editor_widget_world_view_t& widget	   = *static_cast<editor_widget_world_view_t*>(user_data);
		const editor_world_t* const		  editor_world = editor_world_controller_t::get().get_editor_world(widget._edit_world);
		const editor_play_mode_e		  play_mode	   = editor_world->get_play_mode();

		if (play_mode == editor_play_mode_e::play || play_mode == editor_play_mode_e::play_paused)
			return;

		const ui::layout_out_t& out		   = widget._ui->get_tree().out(id);
		const editor_theme_t&	theme	   = editor_theme_t::get();
		const f32				scale	   = ui::get_valid_scale(widget._ui->get_ui_scale());
		const vec2f_t			center	   = {out.pos.x + out.size.x * 0.5f, out.pos.y + out.size.y * 0.5f};
		const f32				length	   = theme.item_height * EDITOR_WORLD_VIEW_AXIS_LENGTH_SCALE * scale;
		const u32				draw_order = widget._ui->get_tree().draw_order_const(id);

		const quat_t  view_inverse = editor_world->get_view_rotation().inverse();
		const vec3f_t world_axes[] = {vec3f_t::right, vec3f_t::up, vec3f_t::forward};
		const vec4f_t colors[]	   = {theme.color_accent0, theme.color_accent_green, theme.color_accent1};
		const char	  labels[]	   = {'X', 'Y', 'Z'};

		ui::ui_render_state_t line_state = {};
		line_state.pipeline				 = paint.get_pipelines().default_pipeline;

		const ui::glyph_raster_mode_e raster_mode = editor_text_rasterization_t::get_rasterization_type();
		ui::ui_render_state_t		  text_state  = {};

		switch (raster_mode)
		{
		case ui::glyph_raster_mode_e::lcd:
			text_state.pipeline = paint.get_pipelines().text_pipeline;
			break;
		case ui::glyph_raster_mode_e::grayscale:
			text_state.pipeline = paint.get_pipelines().grayscale_text_pipeline;
			break;
		case ui::glyph_raster_mode_e::sdf:
			text_state.pipeline = paint.get_pipelines().sdf_pipeline;
			break;
		}

		const font_runtime_t* font = resource_manager_t::get().find_runtime<font_runtime_t>(theme.font_title_bold);

		for (u32 i = 0; i < 3; ++i)
		{
			const vec3f_t			  camera_axis = view_inverse * world_axes[i];
			const vec2f_t			  direction	  = {camera_axis.x, -camera_axis.y};
			const vec2f_t			  endpoint	  = center + direction * length;
			const ui::vg_line_paint_t line_paint  = {
				.color		  = colors[i],
				.thickness	  = EDITOR_WORLD_VIEW_AXIS_LINE_THICKNESS * scale,
				.aa_thickness = theme.aa_thickness * scale,
			};

			canvas.add_line(center, endpoint, line_paint, line_state, draw_order);

			if (font == nullptr || font->face == nullptr)
				continue;

			const ui::vg_text_paint_t text_paint = {
				.font		 = font,
				.color		 = colors[i],
				.size_px	 = theme.text_default_px_size * scale,
				.raster_px	 = ui::get_text_raster_px(theme.text_default_px_size * scale, widget._ui->get_dpi_scale()),
				.raster_mode = raster_mode,
			};
			const vec2f_t text_size = ui::vg_canvas_t::measure_text(labels + i, 1, text_paint);
			vec2f_t		  label_pos = endpoint - text_size * 0.5f;

			if (!direction.is_zero())
				label_pos += direction.normalized() * (EDITOR_WORLD_VIEW_AXIS_LABEL_OFFSET * scale);

			canvas.add_text(labels + i, 1, label_pos, text_paint, text_state, draw_order);
		}
	}

	bool editor_widget_world_view_t::on_payload_drop(const editor_payload_t& payload, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);

		if (widget._edit_world.is_null())
			return false;

		const ui::layout_out_t& out	  = widget._ui->get_tree().out(widget._world_view);
		const vec2f_t&			mouse = widget._ui->get_input().get_mouse_position();

		if (!rectf_t{out.pos.x, out.pos.y, out.size.x, out.size.y}.contains(mouse))
			return false;

		return editor_world_controller_t::get().get_editor_world(widget._edit_world)->get_input_controller().payload_drop(payload, mouse);
	}
}
