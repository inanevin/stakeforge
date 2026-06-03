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
#include "ui/widgets/editor_widgets_scrollbar.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
#define EDITOR_SCROLLBAR_THICKNESS	6.0f
#define EDITOR_SCROLLBAR_MIN_THUMB	20.0f
#define EDITOR_SCROLLBAR_WHEEL_STEP 32.0f

	namespace
	{
		void set_scrollbar_rect(ui::paint_layer_t& paint, ui::widget_id_t id, const vec4f_t& color, f32 rounding)
		{
			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = color;
			rect.fill_color_b		 = color;
			rect.rounding			 = rounding;
			rect.rounding_segs		 = 4;
			paint.set_rect(id, rect);
		}
	}

	void editor_scrollbar_t::init(ui::ui_context& ui, const editor_scrollbar_config_t& config)
	{
		_ui		= &ui;
		_config = config;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "scrollbar");
		tree.attach(tree.node(config.target).parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(config.target) + 128;
		ui.set_pre_layout_tick(_root, on_tick, this);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible | ui::wf_overlay;
		root_in.pos_mode_x		 = ui::pos_mode_e::absolute_screen;
		root_in.pos_mode_y		 = ui::pos_mode_e::absolute_screen;
		root_in.size_mode_x		 = ui::axis_mode_e::fixed;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;

		_x = {.owner = this, .axis = axis_e::x};
		_y = {.owner = this, .axis = axis_e::y};

		axis_state_t* axes[2] = {&_x, &_y};
		for (axis_state_t* axis : axes)
		{
			axis->track = ui.allocate_widget();
			ui.set_widget_debug_name(axis->track, axis->axis == axis_e::x ? "scrollbar_track_x" : "scrollbar_track_y");
			tree.attach(_root, axis->track);

			ui::layout_in_t& track_in = tree.in(axis->track);
			track_in.flags			  = 0;
			track_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
			track_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
			if (axis->axis == axis_e::x)
			{
				track_in.pos_value	 = {0.0f, 1.0f};
				track_in.anchor_y	 = ui::anchor_e::end;
				track_in.size_mode_x = ui::axis_mode_e::parent_relative;
				track_in.size_mode_y = ui::axis_mode_e::fixed;
				track_in.size_value	 = {1.0f, EDITOR_SCROLLBAR_THICKNESS};
			}
			else
			{
				track_in.pos_value	 = {1.0f, 0.0f};
				track_in.anchor_x	 = ui::anchor_e::end;
				track_in.size_mode_x = ui::axis_mode_e::fixed;
				track_in.size_mode_y = ui::axis_mode_e::parent_relative;
				track_in.size_value	 = {EDITOR_SCROLLBAR_THICKNESS, 1.0f};
			}
			set_scrollbar_rect(paint, axis->track, theme.color_frame_light, theme.item_rounding);

			ui::listener_bundle_t track_listener = {};
			track_listener.user_data			 = axis;
			track_listener.on_press				 = on_track_press;
			ui.get_input().set_listener(axis->track, track_listener);

			axis->thumb = ui.allocate_widget();
			ui.set_widget_debug_name(axis->thumb, axis->axis == axis_e::x ? "scrollbar_thumb_x" : "scrollbar_thumb_y");
			tree.attach(axis->track, axis->thumb);
			tree.draw_order(axis->thumb) = tree.draw_order_const(axis->track) + 1;

			ui::layout_in_t& thumb_in = tree.in(axis->thumb);
			thumb_in.flags			  = 0;
			thumb_in.pos_mode_x		  = ui::pos_mode_e::offset_in_parent;
			thumb_in.pos_mode_y		  = ui::pos_mode_e::offset_in_parent;
			set_scrollbar_rect(paint, axis->thumb, theme.color_accent0_dim, theme.item_rounding);
			paint.set_hover_color(axis->thumb, theme.color_accent0);
			paint.set_press_color(axis->thumb, theme.color_accent0);

			ui::listener_bundle_t thumb_listener = {};
			thumb_listener.user_data			 = axis;
			thumb_listener.on_drag				 = on_thumb_drag;
			ui.get_input().set_listener(axis->thumb, thumb_listener);
		}

		ui::listener_bundle_t target_listener = {};
		target_listener.user_data			  = this;
		target_listener.on_wheel			  = on_target_wheel;
		ui.get_input().set_listener(config.target, target_listener);
	}

	void editor_scrollbar_t::uninit()
	{
		_ui->get_input().clear_listener(_config.target);
		_ui->deallocate_widget(_root);

		_ui		 = nullptr;
		_root	 = NULL_WIDGET;
		_config	 = {};
		_x		 = {};
		_y		 = {};
		_stick_y = false;
	}

	void editor_scrollbar_t::scroll_to_end_y()
	{
		_stick_y = true;
	}

	void editor_scrollbar_t::update_axis(axis_state_t& axis)
	{
		const bool				enabled	   = axis.axis == axis_e::x ? ((_config.axes & editor_scrollbar_axis_x) != 0) : ((_config.axes & editor_scrollbar_axis_y) != 0);
		const f32				ui_scale   = _ui->get_ui_scale() > 0.0f ? _ui->get_ui_scale() : 1.0f;
		ui::layout_tree_t&		tree	   = _ui->get_tree();
		ui::layout_in_t&		target_in  = tree.in(_config.target);
		const ui::layout_out_t& target_out = tree.out(_config.target);
		const ui::layout_out_t& track_out  = tree.out(axis.track);
		ui::layout_in_t&		track_in   = tree.in(axis.track);
		ui::layout_in_t&		thumb_in   = tree.in(axis.thumb);

		const f32 max_scroll = axis.axis == axis_e::x ? target_out.max_scroll.x : target_out.max_scroll.y;
		if (!enabled || max_scroll <= 0.0f)
		{
			track_in.flags = 0;
			thumb_in.flags = 0;
			if (axis.axis == axis_e::x)
				target_in.scroll_offset.x = 0.0f;
			else
				target_in.scroll_offset.y = 0.0f;
			return;
		}

		track_in.flags = ui::wf_visible | ui::wf_input;
		thumb_in.flags = ui::wf_visible | ui::wf_input;

		const f32 viewport	= axis.axis == axis_e::x ? track_out.size.x : track_out.size.y;
		const f32 scroll_px = max_scroll * ui_scale;
		const f32 content	= viewport + scroll_px;
		const f32 thumb		= math::max(EDITOR_SCROLLBAR_MIN_THUMB * ui_scale, viewport * viewport / content);
		const f32 range		= math::max(1.0f, viewport - thumb);
		const f32 offset	= axis.axis == axis_e::x ? -target_in.scroll_offset.x : -target_in.scroll_offset.y;
		const f32 pos		= max_scroll > 0.0f ? range * math::clamp(offset / max_scroll, 0.0f, 1.0f) : 0.0f;

		if (axis.axis == axis_e::x)
		{
			thumb_in.size_mode_x = ui::axis_mode_e::fixed;
			thumb_in.size_mode_y = ui::axis_mode_e::parent_relative;
			thumb_in.size_value	 = {thumb / ui_scale, 1.0f};
			thumb_in.pos_value	 = {pos / ui_scale, 0.0f};
		}
		else
		{
			thumb_in.size_mode_x = ui::axis_mode_e::parent_relative;
			thumb_in.size_mode_y = ui::axis_mode_e::fixed;
			thumb_in.size_value	 = {1.0f, thumb / ui_scale};
			thumb_in.pos_value	 = {0.0f, pos / ui_scale};
		}
	}

	void editor_scrollbar_t::set_scroll(axis_e axis, f32 value)
	{
		ui::layout_tree_t&		tree = _ui->get_tree();
		ui::layout_in_t&		in	 = tree.in(_config.target);
		const ui::layout_out_t& out	 = tree.out(_config.target);
		if (axis == axis_e::x)
			in.scroll_offset.x = math::clamp(value, -out.max_scroll.x, 0.0f);
		else
			in.scroll_offset.y = math::clamp(value, -out.max_scroll.y, 0.0f);
	}

	void editor_scrollbar_t::scroll_track_to(axis_state_t& axis, const vec2f_t& pos)
	{
		ui::layout_tree_t&		tree	   = _ui->get_tree();
		const ui::layout_out_t& target_out = tree.out(_config.target);
		const ui::layout_out_t& track_out  = tree.out(axis.track);
		const ui::layout_out_t& thumb_out  = tree.out(axis.thumb);
		const f32				max_scroll = axis.axis == axis_e::x ? target_out.max_scroll.x : target_out.max_scroll.y;
		const f32				track_pos  = axis.axis == axis_e::x ? track_out.pos.x : track_out.pos.y;
		const f32				track_size = axis.axis == axis_e::x ? track_out.size.x : track_out.size.y;
		const f32				thumb_size = axis.axis == axis_e::x ? thumb_out.size.x : thumb_out.size.y;
		const f32				mouse_pos  = axis.axis == axis_e::x ? pos.x : pos.y;
		const f32				range	   = math::max(1.0f, track_size - thumb_size);
		const f32				ratio	   = math::clamp((mouse_pos - track_pos - thumb_size * 0.5f) / range, 0.0f, 1.0f);
		set_scroll(axis.axis, -ratio * max_scroll);
	}

	void editor_scrollbar_t::on_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		editor_scrollbar_t&		scrollbar  = *static_cast<editor_scrollbar_t*>(user_data);
		ui::layout_tree_t&		tree	   = scrollbar._ui->get_tree();
		ui::layout_in_t&		root_in	   = tree.in(scrollbar._root);
		const ui::layout_out_t& target_out = tree.out(scrollbar._config.target);
		const f32				ui_scale   = scrollbar._ui->get_ui_scale() > 0.0f ? scrollbar._ui->get_ui_scale() : 1.0f;
		root_in.pos_value				   = target_out.pos;
		root_in.size_value				   = target_out.size / ui_scale;

		if (scrollbar._stick_y)
			scrollbar.set_scroll(axis_e::y, -target_out.max_scroll.y);
		scrollbar.update_axis(scrollbar._x);
		scrollbar.update_axis(scrollbar._y);
		if (scrollbar._stick_y)
			scrollbar._stick_y = target_out.max_scroll.y <= 0.0f;
	}

	void editor_scrollbar_t::on_target_wheel(ui::input_router_t&, ui::widget_id_t, f32 delta, void* user_data)
	{
		editor_scrollbar_t& scrollbar = *static_cast<editor_scrollbar_t*>(user_data);
		ui::layout_tree_t&	tree	  = scrollbar._ui->get_tree();
		ui::layout_in_t&	in		  = tree.in(scrollbar._config.target);
		const f32			current	  = in.scroll_offset.y;
		scrollbar.set_scroll(axis_e::y, current + delta * EDITOR_SCROLLBAR_WHEEL_STEP);
		scrollbar._stick_y = false;
	}

	void editor_scrollbar_t::on_track_press(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		axis_state_t& axis = *static_cast<axis_state_t*>(user_data);
		axis.owner->scroll_track_to(axis, pos);
		axis.owner->_stick_y = false;
	}

	void editor_scrollbar_t::on_thumb_drag(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, const vec2f_t& delta, void* user_data)
	{
		axis_state_t&			axis	   = *static_cast<axis_state_t*>(user_data);
		ui::layout_tree_t&		tree	   = axis.owner->_ui->get_tree();
		const ui::layout_out_t& target_out = tree.out(axis.owner->_config.target);
		const ui::layout_out_t& track_out  = tree.out(axis.track);
		const ui::layout_out_t& thumb_out  = tree.out(axis.thumb);
		const f32				max_scroll = axis.axis == axis_e::x ? target_out.max_scroll.x : target_out.max_scroll.y;
		const f32				track_size = axis.axis == axis_e::x ? track_out.size.x : track_out.size.y;
		const f32				thumb_size = axis.axis == axis_e::x ? thumb_out.size.x : thumb_out.size.y;
		const f32				range	   = math::max(1.0f, track_size - thumb_size);
		const f32				d		   = axis.axis == axis_e::x ? delta.x : delta.y;
		ui::layout_in_t&		in		   = tree.in(axis.owner->_config.target);
		const f32				current	   = axis.axis == axis_e::x ? in.scroll_offset.x : in.scroll_offset.y;
		axis.owner->set_scroll(axis.axis, current - d * max_scroll / range);
		axis.owner->_stick_y = false;
	}
}
