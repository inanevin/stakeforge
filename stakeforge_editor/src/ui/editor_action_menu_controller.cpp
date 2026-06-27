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
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/resources/font.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>
#include <cstring>

namespace sfg
{
	editor_action_menu_style_t make_default_action_menu_style(const editor_theme_t& theme)
	{
		editor_action_menu_style_t style = {};
		style.dropdown_color			 = theme.color_frame;
		style.hover_color				 = theme.color_panel_light;
		style.press_color				 = theme.color_light;
		style.text_color				 = theme.color_text0;
		style.shortcut_color			 = theme.color_text2;
		style.disabled_text_color		 = theme.color_text_disabled;
		style.title_color				 = theme.color_text2;
		style.title_line_color			 = theme.color_text2;
		style.icon_color				 = theme.color_text0;
		style.min_width					 = theme.item_width * 1.4f;
		style.row_height				 = theme.item_height;
		style.text_size					 = theme.text_default_px_size;
		style.shortcut_size				 = theme.text_small_title_px_size;
		style.title_size				 = theme.text_small_title_px_size;
		style.title_line_thickness		 = theme.divider_thickness;
		style.icon_size					 = theme.icon_default_px_size;
		style.padding_x					 = theme.margin_horizontal;
		style.padding_y					 = theme.margin_vertical;
		style.shortcut_gap				 = theme.item_spacing * 4.0f;
		style.title_gap					 = theme.item_spacing;
		return style;
	}

	namespace
	{
		constexpr u32 ACTION_MENU_FG_DRAW_ORDER = 51000u;
		constexpr u32 ACTION_MENU_DRAW_ORDER	= 51001u;

		editor_action_menu_controller_t* s_controllers[editor_action_menu_controller_t::MAX_CONTROLLERS] = {};
		u32								 s_controller_count												 = 0;

		void set_widget_visible(ui::layout_tree_t& tree, ui::widget_id_t id, bool visible, bool input)
		{
			ui::layout_in_t& in = tree.in(id);
			in.flags			= visible ? static_cast<u16>(ui::wf_visible | (input ? ui::wf_input : 0)) : 0;
		}

		void set_rect_color(ui::paint_layer_t& paint, ui::widget_id_t id, const vec4f_t& color)
		{
			ui::paint_def_t& def  = paint.def(id);
			def.rect.fill_color_a = color;
			def.rect.fill_color_b = color;
			def.rect.gradient	  = ui::vg_gradient_e::none;
		}

		bool is_toggled(const editor_action_menu_row_desc_t& row)
		{
			if (row.toggle_query != nullptr)
				return row.toggle_query(row.toggle_user_data);
			SFG_ASSERT(row.toggle_value != nullptr);
			return *row.toggle_value;
		}

		const char* row_icon(const editor_action_menu_row_desc_t& row)
		{
			if (!row.disabled && row.child_count > 0)
				return ICON_DD_RIGHT;
			if (row.kind == editor_action_menu_row_kind_e::toggle && is_toggled(row))
				return ICON_CHECK;
			return row.icon;
		}

		vec4f_t row_icon_color(const editor_action_menu_row_desc_t& row, const editor_action_menu_style_t& style)
		{
			if (row.disabled)
				return style.disabled_text_color;
			if (row.icon != nullptr && row.has_icon_color && (row.disabled || row.child_count == 0) && !(row.kind == editor_action_menu_row_kind_e::toggle && is_toggled(row)))
				return row.icon_color;
			return style.icon_color;
		}
	}

	void editor_action_menu_controller_t::init(ui::ui_context& ui)
	{
		SFG_ASSERT(_ui == nullptr);
		SFG_ASSERT(s_controller_count < MAX_CONTROLLERS);

		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_foreground = ui.allocate_widget();
		ui.set_widget_debug_name(_foreground, "action_menu_foreground");
		tree.attach(ui.get_root(), _foreground);
		tree.draw_order(_foreground) = ACTION_MENU_FG_DRAW_ORDER;

		ui::layout_in_t& fg_in = tree.in(_foreground);
		fg_in.flags			   = 0;
		fg_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		fg_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		fg_in.size_value	   = {1.0f, 1.0f};

		ui::listener_bundle_t row_listener = {};
		row_listener.user_data			   = this;
		row_listener.on_click			   = handle_row_click;
		row_listener.on_hover_enter		   = handle_row_hover;

		ui::listener_bundle_t panel_listener = {};
		panel_listener.user_data			 = this;
		panel_listener.on_hover_enter		 = handle_panel_hover;

		for (u32 d = 0; d < MAX_DEPTH; ++d)
		{
			_panels[d] = ui.allocate_widget();
			ui.set_widget_debug_name(_panels[d], "action_menu_panel");
			tree.attach(_foreground, _panels[d]);
			tree.draw_order(_panels[d]) = ACTION_MENU_DRAW_ORDER + d;

			ui::layout_in_t& panel_in = tree.in(_panels[d]);
			panel_in.flags			  = 0;
			panel_in.pos_mode_x		  = ui::pos_mode_e::absolute_screen;
			panel_in.pos_mode_y		  = ui::pos_mode_e::absolute_screen;
			panel_in.size_mode_x	  = ui::axis_mode_e::fixed;
			panel_in.size_mode_y	  = ui::axis_mode_e::sum_children;
			panel_in.flow			  = ui::flow_e::column;
			panel_in.child_spacing	  = 0.0f;
			panel_in.child_margins	  = {theme.item_spacing, 0.0f, theme.item_spacing, 0.0f};

			ui::vg_rect_paint_t panel_rect = {};
			panel_rect.fill_color_a		   = theme.color_frame;
			panel_rect.fill_color_b		   = theme.color_frame;
			panel_rect.outline_color	   = theme.color_outline;
			panel_rect.outline_thickness   = theme.outline_thickness;
			paint.set_rect(_panels[d], panel_rect);
			ui.get_input().set_listener(_panels[d], panel_listener);

			for (u32 r = 0; r < MAX_ROWS; ++r)
			{
				_row_frames[d][r] = ui.allocate_widget();
				ui.set_widget_debug_name(_row_frames[d][r], "action_menu_row");
				tree.attach(_panels[d], _row_frames[d][r]);
				tree.draw_order(_row_frames[d][r]) = tree.draw_order_const(_panels[d]) + 1;

				ui::layout_in_t& row_in = tree.in(_row_frames[d][r]);
				row_in.flags			= 0;
				row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
				row_in.size_mode_y		= ui::axis_mode_e::fixed;
				row_in.size_value		= {1.0f, theme.item_height};

				ui::vg_rect_paint_t row_rect = {};
				row_rect.fill_color_a		 = {0.0f, 0.0f, 0.0f, 0.0f};
				row_rect.fill_color_b		 = {0.0f, 0.0f, 0.0f, 0.0f};
				paint.set_rect(_row_frames[d][r], row_rect);
				ui.get_input().set_listener(_row_frames[d][r], row_listener);

				_row_labels[d][r] = ui.allocate_widget();
				ui.set_widget_debug_name(_row_labels[d][r], "action_menu_label");
				tree.attach(_row_frames[d][r], _row_labels[d][r]);

				ui::layout_in_t& label_in = tree.in(_row_labels[d][r]);
				label_in.flags			  = 0;
				label_in.pos_mode_x		  = ui::pos_mode_e::offset_in_parent;
				label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
				label_in.anchor_y		  = ui::anchor_e::center;
				paint.set_text(_row_labels[d][r], nullptr, 0, {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

				_row_shortcuts[d][r] = ui.allocate_widget();
				ui.set_widget_debug_name(_row_shortcuts[d][r], "action_menu_shortcut");
				tree.attach(_row_frames[d][r], _row_shortcuts[d][r]);

				ui::layout_in_t& shortcut_in = tree.in(_row_shortcuts[d][r]);
				shortcut_in.flags			 = 0;
				shortcut_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
				shortcut_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
				shortcut_in.anchor_x		 = ui::anchor_e::end;
				shortcut_in.anchor_y		 = ui::anchor_e::center;
				paint.set_text(_row_shortcuts[d][r], nullptr, 0, {.font = theme.font_title_bold, .color = theme.color_text2, .point_size = theme.text_small_title_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

				_row_icons[d][r] = ui.allocate_widget();
				ui.set_widget_debug_name(_row_icons[d][r], "action_menu_icon");
				tree.attach(_row_frames[d][r], _row_icons[d][r]);

				ui::layout_in_t& icon_in = tree.in(_row_icons[d][r]);
				icon_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
				icon_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
				icon_in.anchor_x		 = ui::anchor_e::end;
				icon_in.anchor_y		 = ui::anchor_e::center;
				icon_in.size_mode_x		 = ui::axis_mode_e::fixed;
				icon_in.size_mode_y		 = ui::axis_mode_e::fixed;

				_row_icon_labels[d][r] = ui.allocate_widget();
				ui.set_widget_debug_name(_row_icon_labels[d][r], "action_menu_icon_label");
				tree.attach(_row_icons[d][r], _row_icon_labels[d][r]);

				ui::layout_in_t& icon_label_in = tree.in(_row_icon_labels[d][r]);
				icon_label_in.flags			   = 0;
				icon_label_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
				icon_label_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
				icon_label_in.pos_value		   = {0.5f, 0.5f};
				icon_label_in.anchor_x		   = ui::anchor_e::center;
				icon_label_in.anchor_y		   = ui::anchor_e::center;
				paint.set_text(_row_icon_labels[d][r], nullptr, 0, {.font = theme.font_icons, .color = theme.color_text0, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

				_row_title_lines[d][r] = ui.allocate_widget();
				ui.set_widget_debug_name(_row_title_lines[d][r], "action_menu_title_line");
				tree.attach(_row_frames[d][r], _row_title_lines[d][r]);

				ui::layout_in_t& title_line_in = tree.in(_row_title_lines[d][r]);
				title_line_in.flags			   = 0;
				title_line_in.pos_mode_x	   = ui::pos_mode_e::offset_in_parent;
				title_line_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
				title_line_in.anchor_y		   = ui::anchor_e::center;
				title_line_in.size_mode_x	   = ui::axis_mode_e::fixed;
				title_line_in.size_mode_y	   = ui::axis_mode_e::fixed;

				ui::vg_rect_paint_t title_line_rect = {};
				title_line_rect.fill_color_a		= theme.color_text2;
				title_line_rect.fill_color_b		= {theme.color_text2.x, theme.color_text2.y, theme.color_text2.z, 0.0f};
				title_line_rect.gradient			= ui::vg_gradient_e::horizontal;
				paint.set_rect(_row_title_lines[d][r], title_line_rect);
			}
		}

		s_controllers[s_controller_count++] = this;
		hide_panels_from(0);
		set_widget_visible(tree, _foreground, false, false);
	}

	void editor_action_menu_controller_t::uninit()
	{
		close_action_menu();
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

		_ui			= nullptr;
		_foreground = NULL_WIDGET;
		_desc		= {};
		_open		= false;
		_closing	= false;
		for (u32 d = 0; d < MAX_DEPTH; ++d)
		{
			_panels[d]			  = NULL_WIDGET;
			_active_rows[d]		  = nullptr;
			_active_row_counts[d] = 0;
			for (u32 r = 0; r < MAX_ROWS; ++r)
			{
				_row_frames[d][r]	   = NULL_WIDGET;
				_row_labels[d][r]	   = NULL_WIDGET;
				_row_shortcuts[d][r]   = NULL_WIDGET;
				_row_icons[d][r]	   = NULL_WIDGET;
				_row_icon_labels[d][r] = NULL_WIDGET;
				_row_title_lines[d][r] = NULL_WIDGET;
			}
		}
	}

	void editor_action_menu_controller_t::request_action_menu(const editor_action_menu_desc_t& desc)
	{
		SFG_ASSERT(desc.rows != nullptr);
		SFG_ASSERT(desc.row_count <= MAX_ROWS);

		close_action_menu();
		_desc = desc;
		_open = true;
		set_widget_visible(_ui->get_tree(), _foreground, true, false);
		show_panel(0, desc.rows, desc.row_count, {desc.pos.x, desc.pos.y, 0.0f, 0.0f});
		refresh_popup_scope();
	}

	void editor_action_menu_controller_t::close_action_menu()
	{
		if (_ui == nullptr || !_open || _closing)
			return;

		_closing = true;
		hide_panels_from(0);
		set_widget_visible(_ui->get_tree(), _foreground, false, false);
		_ui->get_input().clear_popup_scope();
		_open										 = false;
		const editor_action_menu_closed_fn closed_fn = _desc.closed_fn;
		void*							   closed_ud = _desc.closed_user_data;
		_desc										 = {};
		if (closed_fn != nullptr)
			closed_fn(closed_ud);
		_closing = false;
	}

	bool editor_action_menu_controller_t::is_open() const
	{
		return _open;
	}

	editor_action_menu_controller_t* editor_action_menu_controller_t::find(ui::ui_context& ui)
	{
		for (u32 i = 0; i < s_controller_count; ++i)
		{
			if (s_controllers[i]->_ui == &ui)
				return s_controllers[i];
		}
		return nullptr;
	}

	void editor_action_menu_controller_t::handle_row_click(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_action_menu_controller_t& menu  = *static_cast<editor_action_menu_controller_t*>(user_data);
		u32								 depth = 0;
		u32								 row   = 0;
		if (!menu.find_row_index(id, depth, row))
			return;

		const editor_action_menu_row_desc_t& desc = menu._active_rows[depth][row];
		if (desc.kind == editor_action_menu_row_kind_e::title || desc.disabled)
			return;

		if (desc.kind == editor_action_menu_row_kind_e::toggle)
		{
			SFG_ASSERT(desc.toggle_value != nullptr || desc.toggle_query != nullptr);
			const bool toggled = !is_toggled(desc);
			if (desc.toggle_value != nullptr)
				*desc.toggle_value = toggled;
			if (desc.close_on_toggle)
				menu.close_action_menu();
			if (desc.toggle_callback != nullptr)
				desc.toggle_callback(desc.command, toggled, desc.toggle_user_data);
			if (desc.command != 0 && menu._desc.command_fn != nullptr)
				menu._desc.command_fn(desc.command, menu._desc.command_user_data);
			if (desc.close_on_toggle)
				return;

			menu.hide_panels_from(depth + 1);
			const char* icon = row_icon(desc);
			if (icon != nullptr)
				menu._ui->set_widget_text(menu._row_icon_labels[depth][row], icon);
			else
				menu._ui->clear_widget_text(menu._row_icon_labels[depth][row]);
			set_widget_visible(menu._ui->get_tree(), menu._row_icons[depth][row], icon != nullptr, false);
			set_widget_visible(menu._ui->get_tree(), menu._row_icon_labels[depth][row], icon != nullptr, false);
			menu.refresh_popup_scope();
			return;
		}

		if (desc.command != 0 && menu._desc.command_fn != nullptr)
			menu._desc.command_fn(desc.command, menu._desc.command_user_data);
		if (desc.disabled || desc.child_count == 0)
			menu.close_action_menu();
	}

	void editor_action_menu_controller_t::handle_row_hover(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		editor_action_menu_controller_t& menu  = *static_cast<editor_action_menu_controller_t*>(user_data);
		u32								 depth = 0;
		u32								 row   = 0;
		if (!menu.find_row_index(id, depth, row))
			return;

		const editor_action_menu_row_desc_t& desc = menu._active_rows[depth][row];
		if (desc.kind == editor_action_menu_row_kind_e::title || desc.disabled)
		{
			menu.hide_panels_from(depth + 1);
			menu.refresh_popup_scope();
			return;
		}
		if (!desc.disabled && desc.child_count > 0)
			menu.show_panel(depth + 1, desc.children, desc.child_count, menu._ui->get_tree().bounds(id));
		else
			menu.hide_panels_from(depth + 1);
		menu.refresh_popup_scope();
	}

	void editor_action_menu_controller_t::handle_panel_hover(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		editor_action_menu_controller_t& menu = *static_cast<editor_action_menu_controller_t*>(user_data);
		for (u32 d = 0; d < MAX_DEPTH; ++d)
		{
			if (menu._panels[d] != id)
				continue;

			menu.hide_panels_from(d + 1);
			menu.refresh_popup_scope();
			return;
		}
	}

	void editor_action_menu_controller_t::handle_popup_outside(ui::input_router_t&, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		static_cast<editor_action_menu_controller_t*>(user_data)->close_action_menu();
	}

	void editor_action_menu_controller_t::show_panel(u32 depth, const editor_action_menu_row_desc_t* rows, u16 row_count, const vec4f_t& anchor)
	{
		SFG_ASSERT(depth < MAX_DEPTH);
		SFG_ASSERT(row_count <= MAX_ROWS);

		ui::layout_tree_t&				  tree	= _ui->get_tree();
		ui::paint_layer_t&				  paint = _ui->get_paint();
		const editor_theme_t&			  theme = editor_theme_t::get();
		const editor_action_menu_style_t& style = _desc.style;

		hide_panels_from(depth);

		_active_rows[depth]		  = rows;
		_active_row_counts[depth] = row_count;
		_active_depth			  = depth;

		f32 width = 0.0f;
		for (u32 i = 0; i < row_count; ++i)
		{
			const bool is_title	  = rows[i].kind == editor_action_menu_row_kind_e::title;
			const f32  label_w	  = measure_text_width(rows[i].text, is_title ? theme.font_title : theme.font_default, is_title ? style.title_size : style.text_size);
			const f32  shortcut_w = is_title ? 0.0f : measure_text_width(rows[i].shortcut, theme.font_title_bold, style.shortcut_size);
			f32		   row_w	  = style.padding_x * 2.0f + label_w;
			if (is_title)
				row_w += style.title_gap + style.shortcut_gap * 1.25f;
			else if (shortcut_w > 0.0f)
				row_w += style.shortcut_gap + shortcut_w;
			if (!is_title && (rows[i].icon != nullptr || (!rows[i].disabled && rows[i].child_count > 0) || rows[i].kind == editor_action_menu_row_kind_e::toggle))
				row_w += (shortcut_w > 0.0f ? style.padding_x : style.shortcut_gap) + style.icon_size;
			width = math::max(width, row_w);
		}
		width = math::max(width, style.min_width);

		const f32				scale	  = _ui->get_ui_scale() > 0.0f ? _ui->get_ui_scale() : 1.0f;
		const f32				width_px  = width * scale;
		const f32				height	  = static_cast<f32>(row_count) * style.row_height + style.padding_y * 2.0f;
		const f32				height_px = height * scale;
		const ui::layout_out_t& root_out  = tree.out(tree.get_root());
		const f32				screen_x0 = root_out.clip.x;
		const f32				screen_y0 = root_out.clip.y;
		const f32				screen_x1 = root_out.clip.x + root_out.clip.z;
		const f32				screen_y1 = root_out.clip.y + root_out.clip.w;

		f32 x = depth == 0 ? anchor.x : anchor.x + anchor.z;
		f32 y = depth == 0 ? anchor.y : anchor.y;

		if (x + width_px > screen_x1)
			x = depth == 0 ? screen_x1 - width_px : anchor.x - width_px;
		if (y + height_px > screen_y1)
			y = screen_y1 - height_px;
		x = math::clamp(x, screen_x0, math::max(screen_x0, screen_x1 - width_px));
		y = math::clamp(y, screen_y0, math::max(screen_y0, screen_y1 - height_px));

		ui::layout_in_t& panel_in = tree.in(_panels[depth]);
		panel_in.flags			  = ui::wf_visible | ui::wf_input;
		panel_in.pos_value		  = {x, y};
		panel_in.size_value		  = {width, 0.0f};
		panel_in.child_margins	  = {style.padding_y, 0.0f, style.padding_y, 0.0f};
		set_rect_color(paint, _panels[depth], style.dropdown_color);
		ui::paint_def_t& def	   = paint.def(_panels[depth]);
		def.rect.outline_color	   = theme.color_outline_light;
		def.rect.outline_thickness = theme.outline_thickness;

		for (u32 i = 0; i < row_count; ++i)
		{
			const bool is_title = rows[i].kind == editor_action_menu_row_kind_e::title;
			const bool disabled = rows[i].disabled;
			set_widget_visible(tree, _row_frames[depth][i], true, !is_title && !disabled);
			tree.in(_row_frames[depth][i]).size_value.y = style.row_height;
			set_rect_color(paint, _row_frames[depth][i], {0.0f, 0.0f, 0.0f, 0.0f});
			paint.def(_row_frames[depth][i]).state_flags = is_title || disabled ? 0 : static_cast<u8>(ui::psf_has_hover | ui::psf_has_press);
			paint.set_hover_color(_row_frames[depth][i], style.hover_color);
			paint.set_press_color(_row_frames[depth][i], style.press_color);

			_ui->set_widget_text(_row_labels[depth][i], rows[i].text);
			ui::layout_in_t& label_in = tree.in(_row_labels[depth][i]);
			if (is_title)
			{
				label_in.pos_mode_x = ui::pos_mode_e::relative_in_parent;
				label_in.pos_value	= {1.0f - (style.padding_x / width), 0.5f};
				label_in.anchor_x	= ui::anchor_e::end;
			}
			else
			{
				label_in.pos_mode_x = ui::pos_mode_e::offset_in_parent;
				label_in.pos_value	= {style.padding_x, 0.5f};
				label_in.anchor_x	= ui::anchor_e::start;
			}
			label_in.anchor_y = ui::anchor_e::center;
			paint.set_text(_row_labels[depth][i],
						   _ui->widget_text(_row_labels[depth][i]),
						   _ui->widget_text_len(_row_labels[depth][i]),
						   {.font		 = is_title ? theme.font_title : theme.font_default,
							.color		 = is_title ? style.title_color : (disabled ? style.disabled_text_color : style.text_color),
							.point_size	 = is_title ? style.title_size : style.text_size,
							.spacing	 = 0,
							.raster_mode = editor_text_rasterization_t::get_rasterization_type()});
			set_widget_visible(tree, _row_labels[depth][i], true, false);

			if (!is_title && (rows[i].shortcut != nullptr ? static_cast<u32>(strlen(rows[i].shortcut)) : 0) > 0)
			{
				_ui->set_widget_text(_row_shortcuts[depth][i], rows[i].shortcut);
				set_widget_visible(tree, _row_shortcuts[depth][i], true, false);
				ui::layout_in_t& shortcut_in = tree.in(_row_shortcuts[depth][i]);
				shortcut_in.pos_value		 = {1.0f - (style.padding_x / width), 0.5f};
				if (rows[i].icon != nullptr || (!rows[i].disabled && rows[i].child_count > 0) || rows[i].kind == editor_action_menu_row_kind_e::toggle)
					shortcut_in.pos_value.x = 1.0f - ((style.padding_x * 2.0f + style.icon_size) / width);
				paint.def(_row_shortcuts[depth][i]).text.color		= disabled ? style.disabled_text_color : style.shortcut_color;
				paint.def(_row_shortcuts[depth][i]).text.point_size = style.shortcut_size;
			}
			else
			{
				_ui->clear_widget_text(_row_shortcuts[depth][i]);
				set_widget_visible(tree, _row_shortcuts[depth][i], false, false);
			}

			if (is_title)
			{
				const f32		 title_w  = measure_text_width(rows[i].text, theme.font_title, style.title_size);
				ui::layout_in_t& line_in  = tree.in(_row_title_lines[depth][i]);
				line_in.pos_value		  = {style.padding_x, 0.5f};
				line_in.size_value		  = {math::max(0.0f, width - title_w - style.title_gap - style.padding_x * 2.0f), style.title_line_thickness};
				vec4f_t title_line_end	  = style.title_line_color;
				title_line_end.w		  = 0.0f;
				ui::paint_def_t& line_pd  = paint.def(_row_title_lines[depth][i]);
				line_pd.rect.fill_color_a = style.title_line_color;
				line_pd.rect.fill_color_b = title_line_end;
				line_pd.rect.gradient	  = ui::vg_gradient_e::horizontal;
			}
			set_widget_visible(tree, _row_title_lines[depth][i], is_title, false);

			const char* icon = !is_title ? row_icon(rows[i]) : nullptr;
			if (icon != nullptr)
			{
				ui::layout_in_t& icon_in = tree.in(_row_icons[depth][i]);
				icon_in.pos_value		 = {1.0f - (style.padding_x / width), 0.5f};
				icon_in.size_value		 = {style.icon_size, style.row_height};
				_ui->set_widget_text(_row_icon_labels[depth][i], icon);
				paint.def(_row_icon_labels[depth][i]).text.color	  = row_icon_color(rows[i], style);
				paint.def(_row_icon_labels[depth][i]).text.point_size = style.icon_size;
			}
			else
				_ui->clear_widget_text(_row_icon_labels[depth][i]);
			set_widget_visible(tree, _row_icons[depth][i], icon != nullptr, false);
			set_widget_visible(tree, _row_icon_labels[depth][i], icon != nullptr, false);
		}

		for (u32 i = row_count; i < MAX_ROWS; ++i)
		{
			set_widget_visible(tree, _row_frames[depth][i], false, false);
			set_widget_visible(tree, _row_labels[depth][i], false, false);
			set_widget_visible(tree, _row_shortcuts[depth][i], false, false);
			set_widget_visible(tree, _row_icons[depth][i], false, false);
			set_widget_visible(tree, _row_icon_labels[depth][i], false, false);
			set_widget_visible(tree, _row_title_lines[depth][i], false, false);
		}
	}

	void editor_action_menu_controller_t::hide_panels_from(u32 depth)
	{
		ui::layout_tree_t& tree = _ui->get_tree();
		for (u32 d = depth; d < MAX_DEPTH; ++d)
		{
			set_widget_visible(tree, _panels[d], false, false);
			_active_rows[d]		  = nullptr;
			_active_row_counts[d] = 0;
			for (u32 r = 0; r < MAX_ROWS; ++r)
			{
				set_widget_visible(tree, _row_frames[d][r], false, false);
				set_widget_visible(tree, _row_labels[d][r], false, false);
				set_widget_visible(tree, _row_shortcuts[d][r], false, false);
				set_widget_visible(tree, _row_icons[d][r], false, false);
				set_widget_visible(tree, _row_icon_labels[d][r], false, false);
				set_widget_visible(tree, _row_title_lines[d][r], false, false);
			}
		}
		if (depth == 0)
			_active_depth = 0;
		else if (_active_depth >= depth)
			_active_depth = depth - 1;
	}

	void editor_action_menu_controller_t::refresh_popup_scope()
	{
		ui::widget_id_t popup_roots[MAX_DEPTH] = {};
		u32				count				   = 0;
		for (u32 d = 0; d < MAX_DEPTH; ++d)
		{
			if (_active_row_counts[d] > 0)
				popup_roots[count++] = _panels[d];
		}
		_ui->get_input().set_popup_scope(_desc.owner_root != NULL_WIDGET ? _desc.owner_root : _foreground, popup_roots, count, handle_popup_outside, this);
	}

	bool editor_action_menu_controller_t::find_row_index(ui::widget_id_t id, u32& out_depth, u32& out_row) const
	{
		for (u32 d = 0; d < MAX_DEPTH; ++d)
		{
			for (u32 r = 0; r < _active_row_counts[d]; ++r)
			{
				if (_row_frames[d][r] == id)
				{
					out_depth = d;
					out_row	  = r;
					return true;
				}
			}
		}
		return false;
	}

	f32 editor_action_menu_controller_t::measure_text_width(const char* text, resource_handle_t font_handle, f32 point_size) const
	{
		const u32 len = text != nullptr ? static_cast<u32>(strlen(text)) : 0;
		if (len == 0)
			return 0.0f;

		const font_runtime_t* font = resource_manager_t::get().find_runtime<font_runtime_t>(font_handle);
		if (font == nullptr || font->face == nullptr)
			return static_cast<f32>(len) * point_size * 0.6f;

		ui::vg_text_paint_t paint = {};
		paint.font				  = font;
		paint.color				  = _desc.style.text_color;
		paint.size_px			  = point_size;
		paint.raster_px			  = ui::get_text_raster_px(point_size * ui::get_valid_scale(_ui->get_ui_scale()), _ui->get_dpi_scale());
		paint.raster_mode		  = editor_text_rasterization_t::get_rasterization_type();

		return ui::vg_canvas_t::measure_text(text, len, paint).x;
	}
}
