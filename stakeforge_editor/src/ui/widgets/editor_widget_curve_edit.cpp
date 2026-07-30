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

#include "ui/widgets/editor_widget_curve_edit.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/editor_tooltip_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/rectf.hpp>
#include <sfg/runtime/resources/font.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>

namespace sfg
{
#define CURVE_EDIT_HEIGHT		  260.0f
#define CURVE_EDIT_KEY_RADIUS	  3.0f
#define CURVE_EDIT_KEY_HIT_RADIUS 12.0f
#define CURVE_EDIT_SEGMENTS		  96
#define CURVE_EDIT_AXIS_LEFT	  38.0f
#define CURVE_EDIT_AXIS_RIGHT	  16.0f
#define CURVE_EDIT_AXIS_TOP		  18.0f
#define CURVE_EDIT_AXIS_BOTTOM	  20.0f

	void editor_widget_curve_edit_t::init(ui::ui_context& ui, ui::widget_id_t parent, span_t<curve_def_t> curves, const editor_widget_callbacks_t& callbacks)
	{
		SFG_ASSERT(curves.size != 0);

		_ui		   = &ui;
		_curves	   = curves;
		_callbacks = callbacks;

		editor_theme_t& theme = editor_theme_t::get();

		ui::layout_tree_t& tree = ui.get_tree();
		_root					= ui.allocate_widget();
		ui.set_widget_debug_name(_root, "curve_edit");
		tree.attach(parent, _root);

		ui::layout_in_t& input = tree.in(_root);
		input.flags			   = ui::wf_visible | ui::wf_input | ui::wf_focusable;
		input.size_mode_x	   = ui::axis_mode_e::parent_relative;
		input.size_mode_y	   = ui::axis_mode_e::fixed;
		input.size_value	   = {1.0f, CURVE_EDIT_HEIGHT};
		input.child_clip_mode  = ui::clip_mode_e::scissor_rect;
		input.child_margins	   = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		ui.get_paint().set_custom(_root, draw, this);

		ui::listener_bundle_t listener = {};
		listener.user_data			   = this;
		listener.on_press			   = on_press;
		listener.on_double_click	   = on_double_click;
		listener.on_drag_begin		   = on_drag_begin;
		listener.on_drag			   = on_drag;
		listener.on_drag_end		   = on_drag_end;
		ui.get_input().set_listener(_root, listener);
		editor_tooltip_controller_t::find(ui)->set_tooltip(_root, {.text = "Double-click to add a key. Drag a channel key to edit it. Right-click a key to remove it."});
	}

	void editor_widget_curve_edit_t::uninit()
	{
		editor_tooltip_controller_t::find(*_ui)->clear_tooltip(_root);
		_ui->deallocate_widget(_root);
		_ui				  = nullptr;
		_curves			  = {};
		_callbacks		  = {};
		_root			  = NULL_WIDGET;
		_selected_key	  = UINT32_MAX;
		_selected_channel = 0;
		_dragging		  = false;
	}

	void editor_widget_curve_edit_t::set_curves(span_t<curve_def_t> curves)
	{
		SFG_ASSERT(curves.size != 0);

		_curves			  = curves;
		_selected_key	  = UINT32_MAX;
		_selected_channel = 0;
		_dragging		  = false;
	}

	void editor_widget_curve_edit_t::copy_primary_keys()
	{
		for (size_t curve_index = 1; curve_index < _curves.size; ++curve_index)
			_curves.data[curve_index].keys = _curves.data[0].keys;
	}

	u32 editor_widget_curve_edit_t::get_channel_count() const
	{
		switch (_curves.data[0].type)
		{
		case curve_type_e::x:
			return 1;
		case curve_type_e::xy:
			return 2;
		case curve_type_e::xyz:
			return 3;
		default:
			return 4;
		}
	}

	rectf_t editor_widget_curve_edit_t::get_plot_rect() const
	{
		const ui::layout_out_t& out	  = _ui->get_tree().out(_root);
		const f32				scale = ui::get_valid_scale(_ui->get_ui_scale());

		return {
			out.pos.x + CURVE_EDIT_AXIS_LEFT * scale,
			out.pos.y + CURVE_EDIT_AXIS_TOP * scale,
			out.size.x - (CURVE_EDIT_AXIS_LEFT + CURVE_EDIT_AXIS_RIGHT) * scale,
			out.size.y - (CURVE_EDIT_AXIS_TOP + CURVE_EDIT_AXIS_BOTTOM) * scale,
		};
	}

	bool editor_widget_curve_edit_t::find_key(const vec2f_t& position, f32 radius, u32& out_key, u32& out_channel) const
	{
		const rectf_t	   plot					 = get_plot_rect();
		const curve_def_t& curve				 = _curves.data[0];
		f32				   best_distance_squared = radius * radius;
		bool			   found				 = false;

		for (u32 key_index = 0; key_index < curve.keys.size(); ++key_index)
		{
			for (u32 channel = 0; channel < get_channel_count(); ++channel)
			{
				const curve_key_t& key				= curve.keys[key_index];
				const f32		   value			= (&key.value.x)[channel];
				const vec2f_t	   key_position		= {plot.x + key.time * plot.w, plot.y + (1.0f - (value + 1.0f) * 0.5f) * plot.h};
				const f32		   distance_squared = (key_position - position).magnitude_sqr();

				if (distance_squared > best_distance_squared)
					continue;

				best_distance_squared = distance_squared;
				out_key				  = key_index;
				out_channel			  = channel;
				found				  = true;
			}
		}

		return found;
	}

	void editor_widget_curve_edit_t::apply_position(const vec2f_t& position)
	{
		const rectf_t plot				  = get_plot_rect();
		curve_def_t&  curve				  = _curves.data[0];
		curve_key_t&  key				  = curve.keys[_selected_key];
		key.time						  = math::clamp((position.x - plot.x) / plot.w, 0.0f, 1.0f);
		(&key.value.x)[_selected_channel] = math::clamp(1.0f - 2.0f * (position.y - plot.y) / plot.h, -1.0f, 1.0f);
		const f32 selected_time			  = key.time;
		std::sort(curve.keys.begin(), curve.keys.end(), [](const curve_key_t& left, const curve_key_t& right) { return left.time < right.time; });

		for (u32 index = 0; index < curve.keys.size(); ++index)
		{
			if (curve.keys[index].time == selected_time)
			{
				_selected_key = index;
				break;
			}
		}

		copy_primary_keys();

		if (_callbacks.edited != nullptr)
			_callbacks.edited(_callbacks.user_data);
	}

	void editor_widget_curve_edit_t::on_press(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		editor_widget_curve_edit_t& edit	= *static_cast<editor_widget_curve_edit_t*>(user_data);
		u32							key		= UINT32_MAX;
		u32							channel = 0;

		if (!edit.find_key(pos, CURVE_EDIT_KEY_HIT_RADIUS * ui::get_valid_scale(edit._ui->get_ui_scale()), key, channel))
			return;

		edit._selected_key	   = key;
		edit._selected_channel = channel;

		if (button != ui::mouse_button_e::right || edit._curves.data[0].keys.size() <= 1)
			return;

		if (edit._callbacks.edit_begin != nullptr)
			edit._callbacks.edit_begin(edit._callbacks.user_data);

		edit._curves.data[0].keys.erase(edit._curves.data[0].keys.begin() + key);
		edit.copy_primary_keys();
		edit._selected_key = UINT32_MAX;

		if (edit._callbacks.edited != nullptr)
			edit._callbacks.edited(edit._callbacks.user_data);
		if (edit._callbacks.edit_submitted != nullptr)
			edit._callbacks.edit_submitted(edit._callbacks.user_data);
	}

	void editor_widget_curve_edit_t::on_double_click(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left)
			return;

		editor_widget_curve_edit_t& edit  = *static_cast<editor_widget_curve_edit_t*>(user_data);
		const rectf_t				plot  = edit.get_plot_rect();
		const f32					time  = math::clamp((pos.x - plot.x) / plot.w, 0.0f, 1.0f);
		vec4f_t						value = edit._curves.data[0].evaluate(time);
		value.x							  = math::clamp(1.0f - 2.0f * (pos.y - plot.y) / plot.h, -1.0f, 1.0f);

		if (edit._callbacks.edit_begin != nullptr)
			edit._callbacks.edit_begin(edit._callbacks.user_data);

		edit._curves.data[0].keys.push_back({.value = value, .time = time});
		std::sort(edit._curves.data[0].keys.begin(), edit._curves.data[0].keys.end(), [](const curve_key_t& left, const curve_key_t& right) { return left.time < right.time; });
		edit.copy_primary_keys();

		if (edit._callbacks.edited != nullptr)
			edit._callbacks.edited(edit._callbacks.user_data);
		if (edit._callbacks.edit_submitted != nullptr)
			edit._callbacks.edit_submitted(edit._callbacks.user_data);
	}

	void editor_widget_curve_edit_t::on_drag_begin(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		editor_widget_curve_edit_t& edit	= *static_cast<editor_widget_curve_edit_t*>(user_data);
		u32							key		= UINT32_MAX;
		u32							channel = 0;

		if (!edit.find_key(pos, CURVE_EDIT_KEY_HIT_RADIUS * ui::get_valid_scale(edit._ui->get_ui_scale()), key, channel))
			return;

		edit._selected_key	   = key;
		edit._selected_channel = channel;
		edit._dragging		   = true;

		if (edit._callbacks.edit_begin != nullptr)
			edit._callbacks.edit_begin(edit._callbacks.user_data);

		edit.apply_position(pos);
	}

	void editor_widget_curve_edit_t::on_drag(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		editor_widget_curve_edit_t& edit = *static_cast<editor_widget_curve_edit_t*>(user_data);

		if (edit._dragging)
			edit.apply_position(pos);
	}

	void editor_widget_curve_edit_t::on_drag_end(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		editor_widget_curve_edit_t& edit = *static_cast<editor_widget_curve_edit_t*>(user_data);

		if (!edit._dragging)
			return;

		edit._dragging = false;

		if (edit._callbacks.edit_submitted != nullptr)
			edit._callbacks.edit_submitted(edit._callbacks.user_data);
	}

	void editor_widget_curve_edit_t::draw(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		const editor_widget_curve_edit_t& edit		 = *static_cast<editor_widget_curve_edit_t*>(user_data);
		const ui::layout_out_t&			  out		 = edit._ui->get_tree().out(id);
		const editor_theme_t&			  theme		 = editor_theme_t::get();
		const f32						  scale		 = ui::get_valid_scale(edit._ui->get_ui_scale());
		const u32						  draw_order = edit._ui->get_tree().draw_order_const(id);
		const rectf_t					  plot		 = edit.get_plot_rect();
		ui::ui_render_state_t			  state		 = {};
		state.pipeline								 = paint.get_pipelines().default_pipeline;

		canvas.add_rect(out.pos, out.pos + out.size, {.fill_color_a = theme.color_frame, .fill_color_b = theme.color_frame, .outline_color = theme.color_outline_light, .outline_thickness = theme.outline_thickness * scale}, state, draw_order);

		const ui::vg_line_paint_t grid{.color = theme.color_panel_light, .thickness = scale};

		for (u32 grid_index = 1; grid_index < 5; ++grid_index)
		{
			const f32 fraction = static_cast<f32>(grid_index) * 0.2f;
			canvas.add_line({plot.x + plot.w * fraction, plot.y}, {plot.x + plot.w * fraction, plot.y + plot.h}, grid, state, draw_order);
		}

		for (u32 grid_index = 1; grid_index < 10; ++grid_index)
		{
			const f32 fraction = static_cast<f32>(grid_index) * 0.1f;
			canvas.add_line({plot.x, plot.y + plot.h * fraction}, {plot.x + plot.w, plot.y + plot.h * fraction}, grid, state, draw_order);
		}

		const vec4f_t colors[] = {
			{1.0f, 0.2f, 0.2f, 1.0f},
			{0.2f, 1.0f, 0.2f, 1.0f},
			{0.2f, 0.45f, 1.0f, 1.0f},
			{1.0f, 1.0f, 1.0f, 1.0f},
		};
		const curve_def_t& curve = edit._curves.data[0];

		for (u32 channel = 0; channel < edit.get_channel_count(); ++channel)
		{
			const ui::vg_line_paint_t line{.color = colors[channel], .thickness = 1.0f * scale, .aa_thickness = theme.aa_thickness * scale};
			vec4f_t					  value	   = curve.evaluate(0.0f);
			vec2f_t					  previous = {plot.x, plot.y + (1.0f - ((&value.x)[channel] + 1.0f) * 0.5f) * plot.h};

			for (u32 segment = 1; segment <= CURVE_EDIT_SEGMENTS; ++segment)
			{
				const f32 time		  = static_cast<f32>(segment) / static_cast<f32>(CURVE_EDIT_SEGMENTS);
				value				  = curve.evaluate(time);
				const vec2f_t current = {plot.x + time * plot.w, plot.y + (1.0f - ((&value.x)[channel] + 1.0f) * 0.5f) * plot.h};
				canvas.add_line(previous, current, line, state, draw_order);
				previous = current;
			}

			for (u32 key_index = 0; key_index < curve.keys.size(); ++key_index)
			{
				const curve_key_t& key		 = curve.keys[key_index];
				const f32		   key_value = (&key.value.x)[channel];
				const vec2f_t	   position	 = {plot.x + key.time * plot.w, plot.y + (1.0f - (key_value + 1.0f) * 0.5f) * plot.h};
				const vec4f_t	   color	 = key_index == edit._selected_key && channel == edit._selected_channel ? theme.color_highlight : colors[channel];
				canvas.add_circle(position, CURVE_EDIT_KEY_RADIUS * scale, {.color = color, .aa_thickness = theme.aa_thickness * scale}, state, draw_order);
			}
		}

		const font_runtime_t* font = resource_manager_t::get().find_runtime<font_runtime_t>(theme.font_default);

		if (font == nullptr || font->face == nullptr)
			return;

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

		const ui::vg_text_paint_t text_paint{
			.font		 = font,
			.color		 = theme.color_text1,
			.size_px	 = theme.text_small_px_size * scale,
			.raster_px	 = ui::get_text_raster_px(theme.text_small_px_size * scale),
			.raster_mode = raster_mode,
		};
		const char* amplitude_labels[] = {
			"-1.0",
			"-0.8",
			"-0.6",
			"-0.4",
			"-0.2",
			"0.0",
			"0.2",
			"0.4",
			"0.6",
			"0.8",
			"1.0",
		};
		const size_t amplitude_label_lengths[] = {4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3};
		const char*	 time_labels[]			   = {
			"0.0",
			"0.2",
			"0.4",
			"0.6",
			"0.8",
			"1.0",
		};
		const size_t time_label_lengths[] = {3, 3, 3, 3, 3, 3};
		const f32	 label_margin		  = 5.0f * scale;

		for (u32 label_index = 0; label_index < std::size(amplitude_labels); ++label_index)
		{
			const vec2f_t label_size	 = ui::vg_canvas_t::measure_text(amplitude_labels[label_index], amplitude_label_lengths[label_index], text_paint);
			const vec2f_t label_position = {
				plot.x - label_size.x - label_margin,
				math::clamp(plot.y + plot.h * (1.0f - static_cast<f32>(label_index) * 0.1f) - label_size.y * 0.5f, out.pos.y, out.pos.y + out.size.y - label_size.y),
			};

			canvas.add_text(amplitude_labels[label_index], amplitude_label_lengths[label_index], label_position, text_paint, text_state, draw_order + 2);
		}

		for (u32 label_index = 0; label_index < std::size(time_labels); ++label_index)
		{
			const vec2f_t label_size	 = ui::vg_canvas_t::measure_text(time_labels[label_index], time_label_lengths[label_index], text_paint);
			const vec2f_t label_position = {
				math::clamp(plot.x + plot.w * static_cast<f32>(label_index) * 0.2f - label_size.x * 0.5f, out.pos.x, out.pos.x + out.size.x - label_size.x),
				plot.y + plot.h + label_margin,
			};

			canvas.add_text(time_labels[label_index], time_label_lengths[label_index], label_position, text_paint, text_state, draw_order + 2);
		}

		if (edit._dragging)
		{
			const curve_key_t& key					= curve.keys[edit._selected_key];
			const f32		   key_value			= (&key.value.x)[edit._selected_channel];
			const vec2f_t	   key_position			= {plot.x + key.time * plot.w, plot.y + (1.0f - (key_value + 1.0f) * 0.5f) * plot.h};
			char			   value_label[32]		= {};
			const int		   value_label_length	= std::snprintf(value_label, sizeof(value_label), "%.2f", static_cast<double>(key_value));
			const vec2f_t	   value_label_size		= ui::vg_canvas_t::measure_text(value_label, static_cast<size_t>(value_label_length), text_paint);
			const vec2f_t	   value_label_position = {
				math::clamp(key_position.x - value_label_size.x * 0.5f, out.pos.x, out.pos.x + out.size.x - value_label_size.x),
				math::max(out.pos.y, key_position.y - value_label_size.y - label_margin),
			};

			canvas.add_text(value_label, static_cast<size_t>(value_label_length), value_label_position, text_paint, text_state, draw_order + 3);
		}
	}
}
