// Copyright (c) 2025 Inan Evin

#include "widgets/editor_widgets_file_menu.hpp"
#include "editor_text_rasterization.hpp"
#include "panels/editor_theme.hpp"
#include "widgets/editor_widgets_icons.hpp"
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
	namespace
	{
		constexpr u32 FILE_MENU_DRAW_ORDER = 50000u;

		void set_widget_visible(ui::layout_tree_t& tree, ui::widget_id_t id, bool visible, bool input)
		{
			ui::layout_in_t& in = tree.in(id);
			in.flags			= visible ? static_cast<u8>(ui::wf_visible | (input ? ui::wf_input : 0)) : static_cast<u8>(ui::wf_overlay);
		}

		void set_rect_color(ui::paint_layer_t& paint, ui::widget_id_t id, const vec4f_t& color)
		{
			ui::paint_def_t& def  = paint.def(id);
			def.rect.fill_color_a = color;
			def.rect.fill_color_b = color;
			def.rect.gradient	  = ui::vg_gradient_e::none;
		}

		u32 text_len(const char* text)
		{
			return text != nullptr ? static_cast<u32>(strlen(text)) : 0;
		}

	}

	void editor_file_menu_t::handle_top_click(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		static_cast<editor_file_menu_t*>(user_data)->on_top_click(id, btn);
	}

	void editor_file_menu_t::handle_top_hover(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		static_cast<editor_file_menu_t*>(user_data)->on_top_hover(id);
	}

	void editor_file_menu_t::handle_row_click(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		static_cast<editor_file_menu_t*>(user_data)->on_row_click(id, btn);
	}

	void editor_file_menu_t::handle_row_hover(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, const vec2f_t&, void* user_data)
	{
		static_cast<editor_file_menu_t*>(user_data)->on_row_hover(id);
	}

	void editor_file_menu_t::handle_popup_outside(ui::input_router_t&, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn == ui::mouse_button_e::left)
			static_cast<editor_file_menu_t*>(user_data)->close();
	}

	void editor_file_menu_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_file_menu_item_desc_t* items, u16 item_count, const editor_file_menu_style_t& style, editor_file_menu_command_fn command_fn, void* command_user_data)
	{
		SFG_ASSERT(item_count <= MAX_TOP_ITEMS);

		_ui				   = &ui;
		_items			   = items;
		_item_count		   = item_count;
		_style			   = style;
		_command_fn		   = command_fn;
		_command_user_data = command_user_data;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = tree.allocate();
		ui.set_widget_debug_name(_root, "file_menu");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::row;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, 0.0f, 0.0f, 0.0f};

		ui::listener_bundle_t top_listener = {};
		top_listener.user_data			   = this;
		top_listener.on_click			   = handle_top_click;
		top_listener.on_hover_enter		   = handle_top_hover;

		for (u32 i = 0; i < item_count; ++i)
		{
			const ui::widget_id_t frame = tree.allocate();
			_top_frames[i]				= frame;
			ui.set_widget_debug_name(frame, "file_menu_top_item");
			tree.attach(_root, frame);
			tree.draw_order(frame) = tree.draw_order_const(_root) + 1;

			ui::layout_in_t& in = tree.in(frame);
			in.flags			= ui::wf_visible | ui::wf_input;
			in.size_mode_x		= ui::axis_mode_e::fixed;
			in.size_mode_y		= ui::axis_mode_e::parent_relative;
			in.size_value		= {style.button_width, 1.0f};

			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = style.frame_color;
			rect.fill_color_b		 = style.frame_color;
			paint.set_rect(frame, rect);
			paint.set_hover_color(frame, style.hover_color);
			paint.set_press_color(frame, style.press_color);
			ui.get_input().set_listener(frame, top_listener);

			const ui::widget_id_t label = tree.allocate();
			_top_labels[i]				= label;
			ui.set_widget_debug_name(label, "file_menu_top_label");
			tree.attach(frame, label);
			tree.draw_order(label) = tree.draw_order_const(frame) + 1;

			ui::layout_in_t& label_in = tree.in(label);
			label_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
			label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
			label_in.pos_value		  = {0.5f, 0.5f};
			label_in.anchor_x		  = ui::anchor_e::center;
			label_in.anchor_y		  = ui::anchor_e::center;

			ui.set_widget_text(label, items[i].text);
			paint.set_text(label, ui.widget_text(label), ui.widget_text_len(label), {.font = theme.font_default, .color = style.text_color, .point_size = style.text_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		}

		_foreground = tree.allocate();
		ui.set_widget_debug_name(_foreground, "file_menu_foreground");
		tree.attach(ui.get_root(), _foreground);
		tree.draw_order(_foreground) = FILE_MENU_DRAW_ORDER;

		ui::layout_in_t& fg_in = tree.in(_foreground);
		fg_in.flags			   = ui::wf_overlay;
		fg_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		fg_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		fg_in.size_value	   = {1.0f, 1.0f};
		fg_in.flow			   = ui::flow_e::none;

		ui::listener_bundle_t row_listener = {};
		row_listener.user_data			   = this;
		row_listener.on_click			   = handle_row_click;
		row_listener.on_hover_enter		   = handle_row_hover;

		for (u32 d = 0; d < MAX_DEPTH; ++d)
		{
			const ui::widget_id_t panel = tree.allocate();
			_panels[d]					= panel;
			ui.set_widget_debug_name(panel, "file_menu_dropdown");
			tree.attach(_foreground, panel);
			tree.draw_order(panel) = FILE_MENU_DRAW_ORDER + d * 64u + 1u;

			ui::layout_in_t& panel_in = tree.in(panel);
			panel_in.flags			  = ui::wf_overlay;
			panel_in.pos_mode_x		  = ui::pos_mode_e::absolute_screen;
			panel_in.pos_mode_y		  = ui::pos_mode_e::absolute_screen;
			panel_in.size_mode_x	  = ui::axis_mode_e::fixed;
			panel_in.size_mode_y	  = ui::axis_mode_e::sum_children;
			panel_in.flow			  = ui::flow_e::column;
			panel_in.child_spacing	  = 0.0f;
			panel_in.child_margins	  = {0.0f, 0.0f, 0.0f, 0.0f};

			ui::vg_rect_paint_t panel_rect = {};
			panel_rect.fill_color_a		   = style.dropdown_color;
			panel_rect.fill_color_b		   = style.dropdown_color;
			paint.set_rect(panel, panel_rect);

			for (u32 r = 0; r < MAX_ROWS; ++r)
			{
				const ui::widget_id_t row = tree.allocate();
				_row_frames[d][r]		  = row;
				ui.set_widget_debug_name(row, "file_menu_dropdown_row");
				tree.attach(panel, row);
				tree.draw_order(row) = tree.draw_order_const(panel) + 1;

				ui::layout_in_t& row_in = tree.in(row);
				row_in.flags			= ui::wf_overlay;
				row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
				row_in.size_mode_y		= ui::axis_mode_e::fixed;
				row_in.size_value		= {1.0f, style.row_height};

				ui::vg_rect_paint_t row_rect = {};
				row_rect.fill_color_a		 = {0, 0, 0, 0};
				row_rect.fill_color_b		 = {0, 0, 0, 0};
				paint.set_rect(row, row_rect);
				paint.set_hover_color(row, style.hover_color);
				paint.set_press_color(row, style.press_color);
				ui.get_input().set_listener(row, row_listener);

				const ui::widget_id_t label = tree.allocate();
				_row_labels[d][r]			= label;
				ui.set_widget_debug_name(label, "file_menu_dropdown_label");
				tree.attach(row, label);
				tree.draw_order(label) = tree.draw_order_const(row) + 1;

				ui::layout_in_t& label_in = tree.in(label);
				label_in.flags			  = ui::wf_overlay;
				label_in.pos_mode_x		  = ui::pos_mode_e::offset_in_parent;
				label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
				label_in.pos_value		  = {style.padding_x, 0.5f};
				label_in.anchor_y		  = ui::anchor_e::center;
				paint.set_text(label, nullptr, 0, {.font = theme.font_default, .color = style.text_color, .point_size = style.text_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

				const ui::widget_id_t shortcut = tree.allocate();
				_row_shortcuts[d][r]		   = shortcut;
				ui.set_widget_debug_name(shortcut, "file_menu_dropdown_shortcut");
				tree.attach(row, shortcut);
				tree.draw_order(shortcut) = tree.draw_order_const(row) + 1;

				ui::layout_in_t& shortcut_in = tree.in(shortcut);
				shortcut_in.flags			 = ui::wf_overlay;
				shortcut_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
				shortcut_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
				shortcut_in.pos_value		 = {1.0f, 0.5f};
				shortcut_in.anchor_x		 = ui::anchor_e::end;
				shortcut_in.anchor_y		 = ui::anchor_e::center;
				paint.set_text(shortcut, nullptr, 0, {.font = theme.font_title_bold, .color = style.shortcut_color, .point_size = style.shortcut_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

				const ui::widget_id_t icon = tree.allocate();
				_row_icons[d][r]		   = icon;
				ui.set_widget_debug_name(icon, "file_menu_dropdown_icon");
				tree.attach(row, icon);
				tree.draw_order(icon) = tree.draw_order_const(row) + 1;

				ui::layout_in_t& icon_in = tree.in(icon);
				icon_in.flags			 = ui::wf_overlay;
				icon_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
				icon_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
				icon_in.pos_value		 = {1.0f, 0.5f};
				icon_in.anchor_x		 = ui::anchor_e::end;
				icon_in.anchor_y		 = ui::anchor_e::center;
				ui.set_widget_text(icon, ICON_DD_RIGHT);
				paint.set_text(icon, ui.widget_text(icon), ui.widget_text_len(icon), {.font = theme.font_icons, .color = style.icon_color, .point_size = style.icon_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

				const ui::widget_id_t title_line = tree.allocate();
				_row_title_lines[d][r]			 = title_line;
				ui.set_widget_debug_name(title_line, "file_menu_dropdown_title_line");
				tree.attach(row, title_line);
				tree.draw_order(title_line) = tree.draw_order_const(row) + 1;

				ui::layout_in_t& title_line_in = tree.in(title_line);
				title_line_in.flags			   = ui::wf_overlay;
				title_line_in.pos_mode_x	   = ui::pos_mode_e::offset_in_parent;
				title_line_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
				title_line_in.pos_value		   = {0.0f, 0.5f};
				title_line_in.anchor_y		   = ui::anchor_e::center;
				title_line_in.size_mode_x	   = ui::axis_mode_e::fixed;
				title_line_in.size_mode_y	   = ui::axis_mode_e::fixed;
				title_line_in.size_value	   = {0.0f, style.title_line_thickness};

				vec4f_t title_line_end = style.title_line_color;
				title_line_end.w	   = 0.0f;

				ui::vg_rect_paint_t title_line_rect = {};
				title_line_rect.fill_color_a		= style.title_line_color;
				title_line_rect.fill_color_b		= title_line_end;
				title_line_rect.gradient			= ui::vg_gradient_e::horizontal;
				paint.set_rect(title_line, title_line_rect);
			}
		}
	}

	void editor_file_menu_t::uninit()
	{
		if (_ui != nullptr)
		{
			ui::input_router_t& input = _ui->get_input();
			input.clear_popup_scope();
			for (u32 i = 0; i < _item_count; ++i)
			{
				input.clear_listener(_top_frames[i]);
				_ui->clear_widget_text(_top_labels[i]);
				_ui->clear_widget_debug_name(_top_frames[i]);
				_ui->clear_widget_debug_name(_top_labels[i]);
			}
			for (u32 d = 0; d < MAX_DEPTH; ++d)
			{
				_ui->clear_widget_debug_name(_panels[d]);
				for (u32 r = 0; r < MAX_ROWS; ++r)
				{
					input.clear_listener(_row_frames[d][r]);
					_ui->clear_widget_text(_row_labels[d][r]);
					_ui->clear_widget_text(_row_shortcuts[d][r]);
					_ui->clear_widget_text(_row_icons[d][r]);
					_ui->clear_widget_debug_name(_row_frames[d][r]);
					_ui->clear_widget_debug_name(_row_labels[d][r]);
					_ui->clear_widget_debug_name(_row_shortcuts[d][r]);
					_ui->clear_widget_debug_name(_row_icons[d][r]);
					_ui->clear_widget_debug_name(_row_title_lines[d][r]);
				}
			}
			_ui->clear_widget_debug_name(_root);
			_ui->clear_widget_debug_name(_foreground);
		}

		_ui				   = nullptr;
		_items			   = nullptr;
		_item_count		   = 0;
		_style			   = {};
		_command_fn		   = nullptr;
		_command_user_data = nullptr;
		_root			   = NULL_WIDGET;
		_foreground		   = NULL_WIDGET;
		_active_depth	   = 0;
		_selected_top	   = 0;
		_open			   = false;
		for (u32 i = 0; i < MAX_TOP_ITEMS; ++i)
		{
			_top_frames[i] = NULL_WIDGET;
			_top_labels[i] = NULL_WIDGET;
		}
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
				_row_title_lines[d][r] = NULL_WIDGET;
			}
		}
	}

	void editor_file_menu_t::close()
	{
		if (!_open)
			return;

		_open = false;
		set_widget_visible(_ui->get_tree(), _foreground, false, false);
		hide_dropdowns_from(0);
		refresh_top_frames();
		_ui->get_input().clear_popup_scope();
	}

	void editor_file_menu_t::on_top_click(ui::widget_id_t id, ui::mouse_button_e btn)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		open_top(find_top_index(id));
	}

	void editor_file_menu_t::on_top_hover(ui::widget_id_t id)
	{
		if (_open)
			open_top(find_top_index(id));
	}

	void editor_file_menu_t::on_row_click(ui::widget_id_t id, ui::mouse_button_e btn)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		u32 depth = 0;
		u32 row	  = 0;
		if (!find_row_index(id, depth, row))
			return;
		const editor_file_menu_row_desc_t& desc = _active_rows[depth][row];
		if (desc.kind == editor_file_menu_row_kind_e::title)
			return;
		if (desc.command != 0 && _command_fn != nullptr)
			_command_fn(desc.command, _command_user_data);
		if (desc.child_count == 0)
			close();
	}

	void editor_file_menu_t::on_row_hover(ui::widget_id_t id)
	{
		u32 depth = 0;
		u32 row	  = 0;
		if (!find_row_index(id, depth, row))
			return;

		const editor_file_menu_row_desc_t& desc = _active_rows[depth][row];
		if (desc.kind == editor_file_menu_row_kind_e::title)
		{
			hide_dropdowns_from(depth + 1);
			refresh_popup_scope();
			return;
		}
		if (desc.child_count > 0)
			show_dropdown(depth + 1, desc.children, desc.child_count, _ui->get_tree().bounds(id));
		else
			hide_dropdowns_from(depth + 1);
		refresh_popup_scope();
	}

	void editor_file_menu_t::open_top(u32 index)
	{
		SFG_ASSERT(index < _item_count);

		_open		  = true;
		_selected_top = index;
		set_widget_visible(_ui->get_tree(), _foreground, true, false);
		refresh_top_frames();
		show_dropdown(0, _items[index].rows, _items[index].row_count, _ui->get_tree().bounds(_top_frames[index]));
		refresh_popup_scope();
	}

	void editor_file_menu_t::show_dropdown(u32 depth, const editor_file_menu_row_desc_t* rows, u16 row_count, const vec4f_t& anchor)
	{
		SFG_ASSERT(depth < MAX_DEPTH);
		SFG_ASSERT(row_count <= MAX_ROWS);

		ui::layout_tree_t&	  tree	= _ui->get_tree();
		ui::paint_layer_t&	  paint = _ui->get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		hide_dropdowns_from(depth);

		_active_rows[depth]		  = rows;
		_active_row_counts[depth] = row_count;
		_active_depth			  = depth;

		f32 width = 0.0f;
		for (u32 i = 0; i < row_count; ++i)
		{
			const bool is_title	  = rows[i].kind == editor_file_menu_row_kind_e::title;
			const f32  label_w	  = measure_text_width(rows[i].text, is_title ? theme.font_title : theme.font_default, is_title ? _style.title_size : _style.text_size);
			const f32  shortcut_w = is_title ? 0.0f : measure_text_width(rows[i].shortcut, theme.font_title_bold, _style.shortcut_size);
			f32		   row_w	  = _style.padding_x * 2.0f + label_w;
			if (is_title)
				row_w += _style.title_gap + _style.shortcut_gap * 1.25f;
			else if (shortcut_w > 0.0f)
				row_w += _style.shortcut_gap + shortcut_w;
			if (!is_title && rows[i].child_count > 0)
				row_w += (shortcut_w > 0.0f ? _style.padding_x : _style.shortcut_gap) + _style.icon_size;
			width = math::max(width, row_w);
		}
		width = math::max(width, _style.button_width);

		const f32				scale	  = _ui->get_ui_scale() > 0.0f ? _ui->get_ui_scale() : 1.0f;
		const f32				width_px  = width * scale;
		const f32				height	  = static_cast<f32>(row_count) * _style.row_height;
		const f32				height_px = height * scale;
		const ui::layout_out_t& root_out  = tree.out(tree.get_root());
		const f32				screen_x0 = root_out.clip.x;
		const f32				screen_y0 = root_out.clip.y;
		const f32				screen_x1 = root_out.clip.x + root_out.clip.z;
		const f32				screen_y1 = root_out.clip.y + root_out.clip.w;

		f32 x = depth == 0 ? anchor.x : anchor.x + anchor.z;
		f32 y = depth == 0 ? anchor.y + anchor.w : anchor.y;

		if (x + width_px > screen_x1)
			x = depth == 0 ? screen_x1 - width_px : anchor.x - width_px;
		if (y + height_px > screen_y1)
			y = screen_y1 - height_px;
		x = math::clamp(x, screen_x0, math::max(screen_x0, screen_x1 - width_px));
		y = math::clamp(y, screen_y0, math::max(screen_y0, screen_y1 - height_px));

		ui::layout_in_t& panel_in = tree.in(_panels[depth]);
		panel_in.flags			  = ui::wf_visible;
		panel_in.pos_value		  = {x, y};
		panel_in.size_value		  = {width, 0.0f};
		set_rect_color(paint, _panels[depth], _style.dropdown_color);

		for (u32 i = 0; i < row_count; ++i)
		{
			const bool			  is_title = rows[i].kind == editor_file_menu_row_kind_e::title;
			const ui::widget_id_t row	   = _row_frames[depth][i];
			set_widget_visible(tree, row, true, true);
			set_rect_color(paint, row, {0, 0, 0, 0});
			paint.def(row).state_flags = is_title ? 0 : static_cast<u8>(ui::psf_has_hover | ui::psf_has_press);

			_ui->set_widget_text(_row_labels[depth][i], rows[i].text);
			ui::layout_in_t& label_in = tree.in(_row_labels[depth][i]);
			if (is_title)
			{
				label_in.pos_mode_x	 = ui::pos_mode_e::relative_in_parent;
				label_in.pos_value.x = 1.0f - (_style.padding_x / width);
				label_in.anchor_x	 = ui::anchor_e::end;
			}
			else
			{
				label_in.pos_mode_x	 = ui::pos_mode_e::offset_in_parent;
				label_in.pos_value.x = _style.padding_x;
				label_in.anchor_x	 = ui::anchor_e::start;
			}
			paint.set_text(_row_labels[depth][i],
						   _ui->widget_text(_row_labels[depth][i]),
						   _ui->widget_text_len(_row_labels[depth][i]),
						   {.font		 = is_title ? theme.font_title : theme.font_default,
							.color		 = is_title ? _style.title_color : _style.text_color,
							.point_size	 = is_title ? _style.title_size : _style.text_size,
							.spacing	 = 0,
							.raster_mode = editor_text_rasterization_t::get_rasterization_type()});
			set_widget_visible(tree, _row_labels[depth][i], true, false);

			if (!is_title && text_len(rows[i].shortcut) > 0)
			{
				_ui->set_widget_text(_row_shortcuts[depth][i], rows[i].shortcut);
				set_widget_visible(tree, _row_shortcuts[depth][i], true, false);
				ui::layout_in_t& shortcut_in = tree.in(_row_shortcuts[depth][i]);
				shortcut_in.pos_value.x		 = 1.0f - (_style.padding_x / width);
				if (rows[i].child_count > 0)
					shortcut_in.pos_value.x = 1.0f - ((_style.padding_x * 2.0f + _style.icon_size) / width);
			}
			else
			{
				_ui->clear_widget_text(_row_shortcuts[depth][i]);
				set_widget_visible(tree, _row_shortcuts[depth][i], false, false);
			}

			if (is_title)
			{
				const f32		 title_w = measure_text_width(rows[i].text, theme.font_title, _style.title_size);
				const f32		 line_x	 = _style.padding_x;
				ui::layout_in_t& line_in = tree.in(_row_title_lines[depth][i]);
				line_in.pos_value.x		 = line_x;
				line_in.size_value.x	 = math::max(0.0f, width - title_w - _style.title_gap - _style.padding_x * 2.0f);
				line_in.size_value.y	 = _style.title_line_thickness;

				vec4f_t title_line_end	  = _style.title_line_color;
				title_line_end.w		  = 0.0f;
				ui::paint_def_t& line_pd  = paint.def(_row_title_lines[depth][i]);
				line_pd.rect.fill_color_a = _style.title_line_color;
				line_pd.rect.fill_color_b = title_line_end;
				line_pd.rect.gradient	  = ui::vg_gradient_e::horizontal;
			}
			set_widget_visible(tree, _row_title_lines[depth][i], is_title, false);

			if (!is_title && rows[i].child_count > 0)
			{
				ui::layout_in_t& icon_in = tree.in(_row_icons[depth][i]);
				icon_in.pos_value.x		 = 1.0f - (_style.padding_x / width);
			}
			set_widget_visible(tree, _row_icons[depth][i], !is_title && rows[i].child_count > 0, false);
		}

		for (u32 i = row_count; i < MAX_ROWS; ++i)
		{
			set_widget_visible(tree, _row_frames[depth][i], false, false);
			set_widget_visible(tree, _row_labels[depth][i], false, false);
			set_widget_visible(tree, _row_shortcuts[depth][i], false, false);
			set_widget_visible(tree, _row_icons[depth][i], false, false);
			set_widget_visible(tree, _row_title_lines[depth][i], false, false);
		}
	}

	void editor_file_menu_t::hide_dropdowns_from(u32 depth)
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
				set_widget_visible(tree, _row_title_lines[d][r], false, false);
			}
		}
		if (depth == 0)
			_active_depth = 0;
		else if (_active_depth >= depth)
			_active_depth = depth - 1;
	}

	void editor_file_menu_t::refresh_popup_scope()
	{
		ui::widget_id_t popup_roots[MAX_DEPTH] = {};
		u32				count				   = 0;
		for (u32 d = 0; d < MAX_DEPTH; ++d)
		{
			if (_active_row_counts[d] > 0)
				popup_roots[count++] = _panels[d];
		}
		_ui->get_input().set_popup_scope(_root, popup_roots, count, handle_popup_outside, this);
	}

	void editor_file_menu_t::refresh_top_frames()
	{
		ui::paint_layer_t& paint = _ui->get_paint();
		for (u32 i = 0; i < _item_count; ++i)
		{
			const vec4f_t color = (_open && i == _selected_top) ? _style.selected_color : _style.frame_color;
			set_rect_color(paint, _top_frames[i], color);
		}
	}

	u32 editor_file_menu_t::find_top_index(ui::widget_id_t id) const
	{
		for (u32 i = 0; i < _item_count; ++i)
		{
			if (_top_frames[i] == id)
				return i;
		}
		SFG_ASSERT(false);
		return 0;
	}

	bool editor_file_menu_t::find_row_index(ui::widget_id_t id, u32& out_depth, u32& out_row) const
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

	f32 editor_file_menu_t::measure_text_width(const char* text, resource_handle_t font_handle, f32 point_size) const
	{
		const u32 len = text_len(text);
		if (len == 0)
			return 0.0f;

		const font_runtime_t* font = resource_manager_t::get().find_runtime<font_runtime_t>(font_handle);
		if (font == nullptr || font->face == nullptr)
			return static_cast<f32>(len) * point_size * 0.6f;

		ui::vg_text_paint_t paint = {};
		paint.font				  = font;
		paint.color				  = _style.text_color;
		paint.size_px			  = point_size;
		paint.raster_px			  = static_cast<u32>(math::max(1.0f, point_size * _ui->get_ui_scale() * _ui->get_dpi_scale() + 0.5f));
		paint.raster_mode		  = editor_text_rasterization_t::get_rasterization_type();

		return ui::vg_canvas_t::measure_text(text, len, paint).x;
	}
}
