// Copyright (c) 2025 Inan Evin

#include "panels/editor_base.hpp"
#include "panels/editor_theme.hpp"
#include "widgets/editor_dividers.hpp"
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

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

			const vec4f_t color		= theme.color_accent1;
			const f32	  thick		= theme.item_height * 0.54f;
			const f32	  angle_run = 2.75f;
			const f32	  pad_y		= theme.item_height * 0.45f;
			const f32	  full_h	= out.size.y - pad_y * 2.0f;
			const f32	  half_h	= full_h * 0.50f;
			const f32	  center_y	= out.pos.y + out.size.y * 0.5f;
			const f32	  full_w	= full_h * angle_run;
			const f32	  half_w	= half_h * angle_run;
			const f32	  full_x0	= out.pos.x + out.size.x * 0.38f - full_w * 0.5f;
			const f32	  full_x1	= full_x0 + full_w;
			const f32	  half_x0	= out.pos.x + out.size.x * 0.66f - half_w * 0.5f;
			const f32	  half_x1	= half_x0 + half_w;
			const f32	  half_y0	= center_y + half_h * 0.5f;
			const f32	  half_y1	= center_y - half_h * 0.5f;
			const f32	  full_y0	= center_y + full_h * 0.5f;
			const f32	  full_y1	= center_y - full_h * 0.5f;

			canvas.push_clip({out.pos.x, out.pos.y, out.size.x, out.size.y});
			ui::vg_draw_buffer_t* db = canvas.get_draw_buffer(ui.get_tree().draw_order_const(id), state);

			const vec2f_t half_p0	   = {half_x0, half_y0};
			const vec2f_t half_p1	   = {half_x1, half_y1};
			const vec2f_t half_dir	   = (half_p1 - half_p0).normalized();
			const vec2f_t half_off	   = {-half_dir.y * thick * 0.5f, half_dir.x * thick * 0.5f};
			const vec2f_t half_path[4] = {
				half_p0 - half_off,
				half_p1 - half_off,
				half_p1 + half_off,
				half_p0 + half_off,
			};
			const u32 half_base = db->vertex_count;
			ui::vg_canvas_t::emit_path_solid(db, {half_path, 4}, color, {out.pos.x, out.pos.y}, {out.pos.x + out.size.x, out.pos.y + out.size.y});
			ui::vg_canvas_t::emit_quad_indices(db, half_base);

			const vec2f_t full_p0	   = {full_x0, full_y0};
			const vec2f_t full_p1	   = {full_x1, full_y1};
			const vec2f_t full_dir	   = (full_p1 - full_p0).normalized();
			const vec2f_t full_off	   = {-full_dir.y * thick * 0.5f, full_dir.x * thick * 0.5f};
			const vec2f_t full_path[4] = {
				full_p0 - full_off,
				full_p1 - full_off,
				full_p1 + full_off,
				full_p0 + full_off,
			};
			const u32 full_base = db->vertex_count;
			ui::vg_canvas_t::emit_path_solid(db, {full_path, 4}, color, {out.pos.x, out.pos.y}, {out.pos.x + out.size.x, out.pos.y + out.size.y});
			ui::vg_canvas_t::emit_quad_indices(db, full_base);
			canvas.pop_clip();
		}
	}

	void editor_base_t::init(ui::ui_context& ui)
	{
		_ui								  = &ui;
		const editor_theme_t& theme		  = editor_theme_t::get();
		ui::layout_tree_t&	  tree		  = ui.get_tree();
		ui::paint_layer_t&	  paint		  = ui.get_paint();
		const f32			  item_height = theme.item_height;
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
			in.size_value		= {1.0f, item_height * 3.0f};
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

			_title_label = tree.allocate();
			ui.set_widget_debug_name(_title_label, "title_label");
			tree.attach(_top_row_left, _title_label);

			ui::layout_in_t& title_in = tree.in(_title_label);
			title_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
			title_in.pos_value.y	  = 0.5f;
			title_in.anchor_y		  = ui::anchor_e::center;

			ui.set_widget_text(_title_label, "stakeforge");
			paint.set_text(_title_label,
						   ui.widget_text(_title_label),
						   ui.widget_text_len(_title_label),
						   {.font = theme.font_big_title, .color = theme.color_fg3, .point_size = theme.text_big_title_px_size, .spacing = 0, .raster_mode = ui::glyph_raster_mode_e::grayscale});
		}

		// top-left strikes
		{
			_top_row_strikes = tree.allocate();
			ui.set_widget_debug_name(_top_row_strikes, "top_row_strikes");
			tree.attach(_top_section, _top_row_strikes);

			ui::layout_in_t& in = tree.in(_top_row_strikes);
			in.size_mode_x		= ui::axis_mode_e::fixed;
			in.size_mode_y		= ui::axis_mode_e::parent_relative;
			in.size_value		= {item_height * 4.0f, 1.0f};

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
		}

		// top-right
		{
			_top_row_right = tree.allocate();
			ui.set_widget_debug_name(_top_row_right, "top_row_right");
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

		editor_dividers_t::make_divider_horizontal_dropshadow(ui, _base, {0.0f, 0.0f, 0.0f, 1.0f}, true);

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
			_ui->clear_widget_debug_name(_top_row_right);
			_ui->clear_widget_debug_name(_title_label);
			_ui->clear_widget_debug_name(_mid_section);
			_ui->clear_widget_debug_name(_bottom_section);
		}

		_ui				 = nullptr;
		_base			 = NULL_WIDGET;
		_top_section	 = NULL_WIDGET;
		_top_row_left	 = NULL_WIDGET;
		_top_row_strikes = NULL_WIDGET;
		_top_row_mid	 = NULL_WIDGET;
		_top_row_right	 = NULL_WIDGET;
		_title_label	 = NULL_WIDGET;
		_mid_section	 = NULL_WIDGET;
		_bottom_section	 = NULL_WIDGET;
	}
}
