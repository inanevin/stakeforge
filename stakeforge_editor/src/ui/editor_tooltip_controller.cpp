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
#include "ui/editor_tooltip_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		constexpr u32 TOOLTIP_DRAW_ORDER  = 59000u;
		constexpr f32 TOOLTIP_OFFSET_X	  = 14.0f;
		constexpr f32 TOOLTIP_OFFSET_Y	  = 18.0f;
		constexpr f32 TOOLTIP_TEXT_FACTOR = 0.7f;

		editor_tooltip_controller_t* s_controllers[editor_tooltip_controller_t::MAX_CONTROLLERS] = {};
		u32							 s_controller_count											 = 0;

		void set_widget_visible(ui::layout_tree_t& tree, ui::widget_id_t id, bool visible)
		{
			ui::layout_in_t& in = tree.in(id);
			in.flags			= visible ? static_cast<u16>(ui::wf_visible) : 0;
		}
	}

	void editor_tooltip_controller_t::init(ui::ui_context& ui)
	{
		SFG_ASSERT(_ui == nullptr);
		SFG_ASSERT(s_controller_count < MAX_CONTROLLERS);

		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_foreground = ui.allocate_widget();
		ui.set_widget_debug_name(_foreground, "tooltip_foreground");
		tree.attach(ui.get_root(), _foreground);
		tree.draw_order(_foreground) = TOOLTIP_DRAW_ORDER;
		ui.set_pre_layout_tick(_foreground, on_pre_layout_tick, this);

		ui::layout_in_t& foreground_in = tree.in(_foreground);
		foreground_in.flags			   = 0;
		foreground_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		foreground_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		foreground_in.size_value	   = {1.0f, 1.0f};

		_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_frame, "tooltip_frame");
		tree.attach(_foreground, _frame);

		ui::layout_in_t& frame_in = tree.in(_frame);
		frame_in.flags			  = 0;
		frame_in.pos_mode_x		  = ui::pos_mode_e::absolute_screen;
		frame_in.pos_mode_y		  = ui::pos_mode_e::absolute_screen;
		frame_in.size_mode_x	  = ui::axis_mode_e::sum_children;
		frame_in.size_mode_y	  = ui::axis_mode_e::fixed;
		frame_in.size_value.y	  = theme.item_height;
		frame_in.child_margins	  = {0.0, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		ui::vg_rect_paint_t frame_rect = {};
		frame_rect.fill_color_a		   = theme.color_frame;
		frame_rect.fill_color_b		   = theme.color_frame;
		frame_rect.outline_color	   = theme.color_outline_light;
		frame_rect.outline_thickness   = theme.outline_thickness;
		frame_rect.rounding			   = theme.item_rounding;
		paint.set_rect(_frame, frame_rect);

		_label = ui.allocate_widget();
		ui.set_widget_debug_name(_label, "tooltip_label");
		tree.attach(_frame, _label);

		ui::layout_in_t& label_in = tree.in(_label);
		label_in.flags			  = 0;
		label_in.pos_mode_x		  = ui::pos_mode_e::flow;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;
		label_in.size_mode_x	  = ui::axis_mode_e::fixed;
		label_in.size_mode_y	  = ui::axis_mode_e::fixed;
		paint.set_text(_label, nullptr, 0, {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_entries.reserve(64);
		s_controllers[s_controller_count++] = this;
		set_visible(false);
	}

	void editor_tooltip_controller_t::uninit()
	{
		set_visible(false);
		_ui->deallocate_widget(_foreground);

		for (u32 i = 0; i < s_controller_count; ++i)
		{
			if (s_controllers[i] == this)
			{
				s_controllers[i]					  = s_controllers[s_controller_count - 1];
				s_controllers[s_controller_count - 1] = nullptr;
				s_controller_count--;
				break;
			}
		}

		_entries.resize(0);
		_ui			   = nullptr;
		_foreground	   = NULL_WIDGET;
		_frame		   = NULL_WIDGET;
		_label		   = NULL_WIDGET;
		_hovered_owner = NULL_WIDGET;
		_hover_seconds = 0.0f;
		_visible	   = false;
	}

	void editor_tooltip_controller_t::set_tooltip(ui::widget_id_t owner, const editor_tooltip_desc_t& desc)
	{
		SFG_ASSERT(_ui != nullptr);
		SFG_ASSERT(owner != NULL_WIDGET);

		tooltip_entry_t* entry = find_entry(owner);
		if (entry == nullptr)
		{
			_entries.push_back({});
			entry		 = &_entries.back();
			entry->owner = owner;
		}
		entry->desc = desc;
	}

	void editor_tooltip_controller_t::clear_tooltip(ui::widget_id_t owner)
	{
		for (auto it = _entries.begin(); it != _entries.end(); ++it)
		{
			if (it->owner == owner)
			{
				_entries.erase(it);
				if (_hovered_owner == owner)
				{
					_hovered_owner = NULL_WIDGET;
					_hover_seconds = 0.0f;
					set_visible(false);
				}
				return;
			}
		}
	}

	editor_tooltip_controller_t* editor_tooltip_controller_t::find(ui::ui_context& ui)
	{
		for (u32 i = 0; i < s_controller_count; ++i)
		{
			if (s_controllers[i]->_ui == &ui)
				return s_controllers[i];
		}
		return nullptr;
	}

	void editor_tooltip_controller_t::tick(f32 dt_seconds)
	{
		ui::layout_tree_t&	tree  = _ui->get_tree();
		ui::input_router_t& input = _ui->get_input();

		const ui::widget_id_t  hovered = input.get_hovered();
		const tooltip_entry_t* entry   = find_entry(hovered);
		if (entry == nullptr || input.is_popup_scope_active() || input.is_pressed(ui::mouse_button_e::left) != NULL_WIDGET)
		{
			_hovered_owner = NULL_WIDGET;
			_hover_seconds = 0.0f;
			set_visible(false);
			return;
		}

		if (_hovered_owner != hovered)
		{
			_hovered_owner = hovered;
			_hover_seconds = 0.0f;
			set_visible(false);
		}

		_hover_seconds += dt_seconds;
		if (_hover_seconds < entry->desc.delay)
			return;

		const editor_theme_t& theme = editor_theme_t::get();
		_ui->set_widget_text(_label, entry->desc.text);
		const f32 text_w = math::min(entry->desc.max_width, static_cast<f32>(_ui->widget_text_len(_label)) * theme.text_default_px_size * TOOLTIP_TEXT_FACTOR);
		const f32 text_h = theme.text_default_px_size + 2.0f;

		ui::layout_in_t& label_in = tree.in(_label);
		label_in.size_value		  = {text_w, text_h};

		const ui::layout_out_t& root_out = tree.out(tree.get_root());
		const vec2f_t&			mouse	 = input.get_mouse_position();
		f32						x		 = mouse.x + TOOLTIP_OFFSET_X;
		f32						y		 = mouse.y + TOOLTIP_OFFSET_Y;

		ui::layout_in_t& frame_in = tree.in(_frame);
		frame_in.pos_value		  = {x, y};
		set_visible(true);
	}

	void editor_tooltip_controller_t::set_visible(bool visible)
	{
		if (_visible == visible)
			return;

		_visible				= visible;
		ui::layout_tree_t& tree = _ui->get_tree();
		set_widget_visible(tree, _foreground, visible);
		set_widget_visible(tree, _frame, visible);
		set_widget_visible(tree, _label, visible);
	}

	editor_tooltip_controller_t::tooltip_entry_t* editor_tooltip_controller_t::find_entry(ui::widget_id_t owner)
	{
		for (tooltip_entry_t& entry : _entries)
		{
			if (entry.owner == owner)
				return &entry;
		}
		return nullptr;
	}

	const editor_tooltip_controller_t::tooltip_entry_t* editor_tooltip_controller_t::find_entry(ui::widget_id_t owner) const
	{
		for (const tooltip_entry_t& entry : _entries)
		{
			if (entry.owner == owner)
				return &entry;
		}
		return nullptr;
	}

	void editor_tooltip_controller_t::on_pre_layout_tick(ui::ui_context&, ui::widget_id_t, f32 dt_seconds, void* user_data)
	{
		static_cast<editor_tooltip_controller_t*>(user_data)->tick(dt_seconds);
	}
}
