// Copyright (c) 2025 Inan Evin

#include "panels/editor_base.hpp"
#include "panels/editor_theme.hpp"
#include "widgets/editor_dividers.hpp"
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_base_t::init(ui::ui_context& ui)
	{
		_ui								  = &ui;
		const editor_theme_t& theme		  = editor_theme_t::get();
		ui::layout_tree_t&	  tree		  = ui.get_tree();
		ui::paint_layer_t&	  paint		  = ui.get_paint();
		const f32			  item_height = theme.item_height;

		// base
		{
			_base = tree.allocate();
			tree.attach(ui.get_root(), _base);

			ui::layout_in_t& in = tree.in(_base);
			in.size_mode_x		= ui::axis_mode_e::parent_relative;
			in.size_mode_y		= ui::axis_mode_e::parent_relative;
			in.size_value		= {1.0f, 1.0f};
			in.flow				= ui::flow_e::column;
			in.child_spacing	= 0.0f;
			in.child_margins	= {0.0f, 0.0f, 0.0f, 0.0f};
		}

		// top
		{
			_top_section = tree.allocate();
			tree.attach(_base, _top_section);

			ui::layout_in_t& in = tree.in(_top_section);
			in.size_mode_x		= ui::axis_mode_e::parent_relative;
			in.size_mode_y		= ui::axis_mode_e::fixed;
			in.size_value		= {1.0f, item_height * 2.0f};
			in.flow				= ui::flow_e::row;
			in.child_spacing	= 0.0f;
			in.child_margins	= {0.0f, 0.0f, 0.0f, 0.0f};

			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = theme.color_bg2;
			rect.fill_color_b		 = theme.color_bg2;
			paint.set_rect(_top_section, rect);
		}

		// top-left title
		{
			_top_row_left = tree.allocate();
			tree.attach(_top_section, _top_row_left);

			ui::layout_in_t& in = tree.in(_top_row_left);
			in.size_mode_x		= ui::axis_mode_e::sum_children;
			in.size_mode_y		= ui::axis_mode_e::parent_relative;
			in.size_value		= {0.0f, 1.0f};
			in.flow				= ui::flow_e::row;
			in.child_spacing	= 0.0f;
			in.child_margins	= {0.0f, 0.0f, 0.0f, theme.margin_horizontal};

			_title_label = tree.allocate();
			tree.attach(_top_row_left, _title_label);

			ui::layout_in_t& title_in = tree.in(_title_label);
			title_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
			title_in.pos_value.y	  = 0.5f;
			title_in.anchor_y		  = ui::anchor_e::center;

			ui.set_widget_text(_title_label, "Stakeforge");
			paint.set_text(_title_label,
						   ui.widget_text(_title_label),
						   ui.widget_text_len(_title_label),
						   {.font = theme.font_big_title, .color = theme.color_fg4, .point_size = theme.text_big_title_px_size, .spacing = 0, .raster_mode = ui::glyph_raster_mode_e::grayscale});
		}

		// top-mid
		{
			_top_row_mid = tree.allocate();
			tree.attach(_top_section, _top_row_mid);

			ui::layout_in_t& in = tree.in(_top_row_mid);
			in.size_mode_x		= ui::axis_mode_e::fill;
			in.size_mode_y		= ui::axis_mode_e::parent_relative;
			in.size_value		= {1.0f, 1.0f};
		}

		// top-right
		{
			_top_row_right = tree.allocate();
			tree.attach(_top_section, _top_row_right);

			ui::layout_in_t& in = tree.in(_top_row_right);
			in.size_mode_x		= ui::axis_mode_e::parent_relative;
			in.size_mode_y		= ui::axis_mode_e::parent_relative;
			in.size_value		= {0.2f, 1.0f};
		}

		editor_dividers_t::make_divider_horizontal_dropshadow(ui, _base, {0.0f, 0.0f, 0.0f, 1.0f});

		// mid
		{
			_mid_section = tree.allocate();
			tree.attach(_base, _mid_section);

			ui::layout_in_t& in = tree.in(_mid_section);
			in.size_mode_x		= ui::axis_mode_e::parent_relative;
			in.size_mode_y		= ui::axis_mode_e::fill;
			in.size_value		= {1.0f, 1.0f};

			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = theme.color_bg1;
			rect.fill_color_b		 = theme.color_bg1;
			paint.set_rect(_mid_section, rect);
		}

		editor_dividers_t::make_divider_horizontal_dropshadow(ui, _base, {0.0f, 0.0f, 0.0f, 1.0f}, true);

		// bottom
		{
			_bottom_section = tree.allocate();
			tree.attach(_base, _bottom_section);

			ui::layout_in_t& in = tree.in(_bottom_section);
			in.size_mode_x		= ui::axis_mode_e::parent_relative;
			in.size_mode_y		= ui::axis_mode_e::fixed;
			in.size_value		= {1.0f, item_height};

			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = theme.color_bg2;
			rect.fill_color_b		 = theme.color_bg2;
			paint.set_rect(_bottom_section, rect);
		}
	}

	void editor_base_t::uninit()
	{
		_ui				= nullptr;
		_base			= NULL_WIDGET;
		_top_section	= NULL_WIDGET;
		_top_row_left	= NULL_WIDGET;
		_top_row_mid	= NULL_WIDGET;
		_top_row_right	= NULL_WIDGET;
		_title_label	= NULL_WIDGET;
		_mid_section	= NULL_WIDGET;
		_bottom_section = NULL_WIDGET;
	}
}
