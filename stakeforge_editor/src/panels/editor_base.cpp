// Copyright (c) 2025 Inan Evin

#include "panels/editor_base.hpp"
#include "panels/editor_theme.hpp"
#include "widgets/editor_custom_draws.hpp"
#include "widgets/editor_dividers.hpp"
#include "widgets/editor_misc_widgets.hpp"
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

#ifndef SFG_MAJOR
#define SFG_MAJOR 0
#endif

#ifndef SFG_MINOR
#define SFG_MINOR 0
#endif

#ifndef SFG_PATCH
#define SFG_PATCH 0
#endif

#ifndef SFG_BUILD
#define SFG_BUILD "unknown"
#endif

#define SFG_EDITOR_STRINGIFY_IMPL(x) #x
#define SFG_EDITOR_STRINGIFY(x)		 SFG_EDITOR_STRINGIFY_IMPL(x)
#define SFG_EDITOR_VERSION_TEXT		 "v." SFG_EDITOR_STRINGIFY(SFG_MAJOR) "." SFG_EDITOR_STRINGIFY(SFG_MINOR) "." SFG_EDITOR_STRINGIFY(SFG_PATCH)
#define SFG_EDITOR_BUILD_TEXT		 "b." SFG_BUILD

namespace sfg
{
	namespace
	{
		void draw_top_row_strikes(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
		{
			ui::ui_context&			ui	  = *static_cast<ui::ui_context*>(user_data);
			const editor_theme_t&	theme = editor_theme_t::get();
			const ui::layout_out_t& out	  = ui.get_tree().out(id);

			ui::ui_render_state_t state = {};
			state.pipeline				= paint.get_pipelines().default_pipeline;
			ui::vg_convex_paint_t p		= {};
			p.fill_color_b				= theme.color_accent0;
			p.fill_color_a				= theme.color_accent0_dim;
			p.gradient					= ui::vg_gradient_e::vertical;
			p.aa_thickness				= theme.aa_thickness;

			canvas.push_clip({out.pos.x, out.pos.y, out.size.x, out.size.y});

			const f32 width = out.size.x * 0.25f;
			const f32 lean	= 0.35f;

			{
				const f32 x_start = out.pos.x;
				const f32 y_start = out.pos.y + out.size.y * 0.5f;
				const f32 y_end	  = out.pos.y + out.size.y;
				editor_custom_draws_t::add_leaned_convex_rect(canvas, {x_start, y_start}, {width, y_end - y_start}, lean, p, state, ui.get_tree().draw_order_const(id));
			}

			{
				const f32 x_start = out.pos.x + width + width * 0.3f;
				const f32 y_start = out.pos.y;
				const f32 y_end	  = out.pos.y + out.size.y;
				editor_custom_draws_t::add_leaned_convex_rect(canvas, {x_start, y_start}, {width, y_end - y_start}, lean, p, state, ui.get_tree().draw_order_const(id));
			}

			canvas.pop_clip();
		}
	}

	void editor_base_t::init(ui::ui_context& ui)
	{
		_ui													 = &ui;
		const editor_theme_t& theme							 = editor_theme_t::get();
		ui::layout_tree_t&	  tree							 = ui.get_tree();
		ui::paint_layer_t&	  paint							 = ui.get_paint();
		const f32			  item_height					 = theme.item_height;
		const vec4f_t		  color_divider_dark_transparent = {theme.color_divider_dark.x, theme.color_divider_dark.y, theme.color_divider_dark.z, 0.0f};
		ui.set_debug_font(theme.font_default);

		// base
		{
			_base = tree.allocate();
			ui.set_widget_debug_name(_base, "base");
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
			ui.set_widget_debug_name(_top_section, "top_section");
			tree.attach(_base, _top_section);

			ui::layout_in_t& in = tree.in(_top_section);
			in.size_mode_x		= ui::axis_mode_e::parent_relative;
			in.size_mode_y		= ui::axis_mode_e::fixed;
			in.size_value		= {1.0f, item_height * 2.5f};
			in.flow				= ui::flow_e::row;
			in.child_spacing	= 0.0f;
			in.child_margins	= {0.0f, 0.0f, 0.0f, 0.0f};

			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = theme.color_bg3;
			rect.fill_color_b		 = theme.color_bg3;
			paint.set_rect(_top_section, rect);
		}

		// top-left title
		{
			_top_row_left = tree.allocate();
			ui.set_widget_debug_name(_top_row_left, "top_row_left");
			tree.attach(_top_section, _top_row_left);

			ui::layout_in_t& in = tree.in(_top_row_left);
			in.size_mode_x		= ui::axis_mode_e::sum_children;
			in.size_mode_y		= ui::axis_mode_e::parent_relative;
			in.size_value		= {0.0f, 1.0f};
			in.flow				= ui::flow_e::row;
			in.child_spacing	= 0.0f;
			in.child_margins	= {0.0f, theme.margin_horizontal * 2, 0.0f, theme.margin_horizontal * 2};

			_title_group = tree.allocate();
			ui.set_widget_debug_name(_title_group, "title_group");
			tree.attach(_top_row_left, _title_group);

			ui::layout_in_t& group_in = tree.in(_title_group);
			group_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
			group_in.pos_value.y	  = 0.5f;
			group_in.anchor_y		  = ui::anchor_e::center;
			group_in.size_mode_x	  = ui::axis_mode_e::max_children;
			group_in.size_mode_y	  = ui::axis_mode_e::sum_children;
			group_in.flow			  = ui::flow_e::column;
			group_in.child_spacing	  = 0.0f;
			group_in.child_margins	  = {0.0f, 0.0f, 0.0f, 0.0f};

			_title_label = tree.allocate();
			ui.set_widget_debug_name(_title_label, "title_label");
			tree.attach(_title_group, _title_label);

			ui.set_widget_text(_title_label, "stakeforge");
			ui::ui_render_state_t title_state = {};
			title_state.pipeline			  = theme.shader_big_title;
			paint.set_text(_title_label,
						   ui.widget_text(_title_label),
						   ui.widget_text_len(_title_label),
						   {.font = theme.font_big_title, .color = theme.color_fg2, .point_size = theme.text_big_title_px_size, .spacing = 0, .raster_mode = ui::glyph_raster_mode_e::lcd},
						   title_state);

			_version_label = tree.allocate();
			ui.set_widget_debug_name(_version_label, "version_label");
			tree.attach(_title_group, _version_label);

			ui.set_widget_text(_version_label, SFG_EDITOR_VERSION_TEXT);
			paint.set_text(_version_label,
						   ui.widget_text(_version_label),
						   ui.widget_text_len(_version_label),
						   {.font = theme.font_big_title, .color = theme.color_fg1, .point_size = theme.text_small_title_px_size, .spacing = 0, .raster_mode = ui::glyph_raster_mode_e::grayscale});

			_build_label = tree.allocate();
			ui.set_widget_debug_name(_build_label, "build_label");
			tree.attach(_title_group, _build_label);

			ui.set_widget_text(_build_label, SFG_EDITOR_BUILD_TEXT);
			paint.set_text(_build_label,
						   ui.widget_text(_build_label),
						   ui.widget_text_len(_build_label),
						   {.font = theme.font_big_title, .color = theme.color_fg0, .point_size = theme.text_small_title_px_size, .spacing = 0, .raster_mode = ui::glyph_raster_mode_e::grayscale});
		}

		// top-left strikes
		{
			_top_row_strikes = tree.allocate();
			ui.set_widget_debug_name(_top_row_strikes, "top_row_strikes");
			tree.attach(_top_section, _top_row_strikes);

			ui::layout_in_t& in = tree.in(_top_row_strikes);
			in.size_mode_x		= ui::axis_mode_e::fixed;
			in.size_mode_y		= ui::axis_mode_e::parent_relative;
			in.size_value		= {item_height * 3.0f, 1.0f};

			paint.set_custom(_top_row_strikes, draw_top_row_strikes, &ui);
		}

		// top-mid
		{
			_top_row_mid = tree.allocate();
			ui.set_widget_debug_name(_top_row_mid, "top_row_mid");
			tree.attach(_top_section, _top_row_mid);

			ui::layout_in_t& in = tree.in(_top_row_mid);
			in.size_mode_x		= ui::axis_mode_e::fill;
			in.size_mode_y		= ui::axis_mode_e::parent_relative;
			in.size_value		= {1.0f, 1.0f};
			in.flow				= ui::flow_e::column;
			in.child_spacing	= 0.0f;
			in.child_margins	= {0.0f, 0.0f, 0.0f, 0.0f};

			_top_mid_file = tree.allocate();
			ui.set_widget_debug_name(_top_mid_file, "top_mid_file");
			tree.attach(_top_row_mid, _top_mid_file);

			ui::layout_in_t& file_in = tree.in(_top_mid_file);
			file_in.size_mode_x		 = ui::axis_mode_e::fill;
			file_in.size_mode_y		 = ui::axis_mode_e::fill;
			file_in.size_value		 = {1.0f, 1.0f};

			_top_mid_divider = editor_dividers_t::add_divider_hor(ui, _top_row_mid, theme.divider_thickness, theme.color_fg0, theme.color_bg3, ui::vg_gradient_e::horizontal);
			ui.set_widget_debug_name(_top_mid_divider, "top_mid_divider");

			_top_mid_util = tree.allocate();
			ui.set_widget_debug_name(_top_mid_util, "top_mid_util");
			tree.attach(_top_row_mid, _top_mid_util);

			ui::layout_in_t& util_in = tree.in(_top_mid_util);
			util_in.size_mode_x		 = ui::axis_mode_e::fill;
			util_in.size_mode_y		 = ui::axis_mode_e::fill;
			util_in.size_value		 = {1.0f, 1.0f};
		}

		// top-right
		{
			_top_row_right = tree.allocate();
			ui.set_widget_debug_name(_top_row_right, "top_row_right");
			tree.attach(_top_section, _top_row_right);

			ui::layout_in_t& in = tree.in(_top_row_right);
			in.size_mode_x		= ui::axis_mode_e::fixed;
			in.size_mode_y		= ui::axis_mode_e::parent_relative;
			in.size_value		= {theme.item_height * 6, 1.0f};
			in.flow				= ui::flow_e::row;
			in.child_spacing	= 0.0f;
			in.child_margins	= {0.0f, 0.0f, 0.0f, 0.0f};

			_top_row_right_buttons = tree.allocate();
			ui.set_widget_debug_name(_top_row_right_buttons, "top_row_right_buttons");
			tree.attach(_top_row_right, _top_row_right_buttons);

			ui::layout_in_t& buttons_in = tree.in(_top_row_right_buttons);
			buttons_in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
			buttons_in.pos_value.y		= 0.0f;
			buttons_in.anchor_y			= ui::anchor_e::start;
			buttons_in.size_mode_x		= ui::axis_mode_e::parent_relative;
			buttons_in.size_mode_y		= ui::axis_mode_e::parent_relative;
			buttons_in.size_value		= {1.0f, 0.5f};
			buttons_in.flow				= ui::flow_e::row;
			buttons_in.child_spacing	= 0.0f;
			buttons_in.child_margins	= {0.0f, 0.0f, 0.0f, 0.0f};

			editor_misc_widgets_t::add_window_buttons(ui, _top_row_right_buttons, theme.color_bg0, theme.color_accent_err, theme.color_bg4, theme.color_bg2, theme.color_fg3, theme.icon_default_px_size);
		}

		editor_dividers_t::add_divider_hor(ui, _base, theme.divider_thickness * 4.0f, theme.color_divider_dark, color_divider_dark_transparent, ui::vg_gradient_e::vertical);

		// mid
		{
			_mid_section = tree.allocate();
			ui.set_widget_debug_name(_mid_section, "mid_section");
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

		editor_dividers_t::add_divider_hor(ui, _base, theme.divider_thickness * 4.0f, color_divider_dark_transparent, theme.color_divider_dark, ui::vg_gradient_e::vertical);

		// bottom
		{
			_bottom_section = tree.allocate();
			ui.set_widget_debug_name(_bottom_section, "bottom_section");
			tree.attach(_base, _bottom_section);

			ui::layout_in_t& in = tree.in(_bottom_section);
			in.size_mode_x		= ui::axis_mode_e::parent_relative;
			in.size_mode_y		= ui::axis_mode_e::fixed;
			in.size_value		= {1.0f, item_height};

			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = theme.color_bg3;
			rect.fill_color_b		 = theme.color_bg3;
			paint.set_rect(_bottom_section, rect);
		}
	}

	void editor_base_t::uninit()
	{
		if (_ui != nullptr)
		{
			_ui->clear_widget_debug_name(_base);
			_ui->clear_widget_debug_name(_top_section);
			_ui->clear_widget_debug_name(_top_row_left);
			_ui->clear_widget_debug_name(_top_row_strikes);
			_ui->clear_widget_debug_name(_top_row_mid);
			_ui->clear_widget_debug_name(_top_mid_file);
			_ui->clear_widget_debug_name(_top_mid_divider);
			_ui->clear_widget_debug_name(_top_mid_util);
			_ui->clear_widget_debug_name(_top_row_right);
			_ui->clear_widget_debug_name(_top_row_right_buttons);
			_ui->clear_widget_debug_name(_title_group);
			_ui->clear_widget_debug_name(_title_label);
			_ui->clear_widget_debug_name(_version_label);
			_ui->clear_widget_debug_name(_build_label);
			_ui->clear_widget_debug_name(_mid_section);
			_ui->clear_widget_debug_name(_bottom_section);
		}

		_ui					   = nullptr;
		_base				   = NULL_WIDGET;
		_top_section		   = NULL_WIDGET;
		_top_row_left		   = NULL_WIDGET;
		_top_row_strikes	   = NULL_WIDGET;
		_top_row_mid		   = NULL_WIDGET;
		_top_mid_file		   = NULL_WIDGET;
		_top_mid_divider	   = NULL_WIDGET;
		_top_mid_util		   = NULL_WIDGET;
		_top_row_right		   = NULL_WIDGET;
		_top_row_right_buttons = NULL_WIDGET;
		_title_group		   = NULL_WIDGET;
		_title_label		   = NULL_WIDGET;
		_version_label		   = NULL_WIDGET;
		_build_label		   = NULL_WIDGET;
		_mid_section		   = NULL_WIDGET;
		_bottom_section		   = NULL_WIDGET;
	}
}
