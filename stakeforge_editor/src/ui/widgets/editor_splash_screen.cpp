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
#include "ui/widgets/editor_splash_screen.hpp"
#include "editor_project.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_draws.hpp"

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

#define SFG_EDITOR_SPLASH_STRINGIFY_IMPL(x) #x
#define SFG_EDITOR_SPLASH_STRINGIFY(x)		SFG_EDITOR_SPLASH_STRINGIFY_IMPL(x)
#define SFG_EDITOR_SPLASH_VERSION_TEXT		"v." SFG_EDITOR_SPLASH_STRINGIFY(SFG_MAJOR) "." SFG_EDITOR_SPLASH_STRINGIFY(SFG_MINOR) "." SFG_EDITOR_SPLASH_STRINGIFY(SFG_PATCH)
#define SFG_EDITOR_SPLASH_BUILD_TEXT		"b." SFG_BUILD
#define SFG_EDITOR_SPLASH_BG_TEXTURE		"editor/resource_pack/textures/splash.jpg"_hs

namespace sfg
{
	namespace
	{
		void draw_splash_strikes(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
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

			canvas.push_clip({out.pos.x, out.pos.y, out.size.x, out.size.y}, ui::clip_mode_e::scissor_rect);

			const f32 width		 = out.size.x * 0.08f;
			const f32 lean		 = 0.35f;
			const f32 x_start	 = out.pos.x + out.size.x * 0.65f;
			const f32 y_start	 = out.pos.y;
			const f32 height	 = out.size.y;
			const f32 draw_order = ui.get_tree().draw_order_const(id);

			editor_custom_draws_t::add_leaned_convex_rect(canvas, {x_start, y_start + height * 0.5f}, {width, height * 0.5f}, lean, p, state, draw_order);
			editor_custom_draws_t::add_leaned_convex_rect(canvas, {x_start + width + width * 0.3f, y_start}, {width, height}, lean, p, state, draw_order);

			canvas.pop_clip(ui::clip_mode_e::scissor_rect);
		}
	}

	void editor_splash_screen_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_splash_screen_config_t& config)
	{
		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "splash_screen");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		root_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		root_in.pos_value		 = {0.0f, 0.0f};
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		root_in.size_value		 = {1.0f, 1.0f};

		_texture_bg = ui.allocate_widget();
		ui.set_widget_debug_name(_texture_bg, "texture_bg");
		tree.attach(_root, _texture_bg);

		ui::layout_in_t& bg_in = tree.in(_texture_bg);
		bg_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		bg_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		bg_in.size_value	   = {1.0f, 1.0f};

		ui::vg_rect_paint_t bg_rect = {};
		bg_rect.fill_color_a		= {1.0f, 1.0f, 1.0f, 1.0f};
		bg_rect.fill_color_b		= bg_rect.fill_color_a;

		ui::ui_render_state_t bg_state = {};
		bg_state.pipeline			   = "editor/resource_pack/shaders/editor_ui_texture.hlsl"_hs;
		bg_state.constants[0].handle   = SFG_EDITOR_SPLASH_BG_TEXTURE;
		bg_state.constants[0].type	   = ui::ui_resource_type_e::texture;
		paint.set_rect(_texture_bg, bg_rect, bg_state);

		_strikes = ui.allocate_widget();
		ui.set_widget_debug_name(_strikes, "splash_strikes");
		tree.attach(_root, _strikes);
		tree.draw_order(_strikes) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& strikes_in = tree.in(_strikes);
		strikes_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		strikes_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		strikes_in.size_value		= {1.0f, 1.0f};
		paint.set_custom(_strikes, draw_splash_strikes, &ui);

		_column = ui.allocate_widget();
		ui.set_widget_debug_name(_column, "splash_column");
		tree.attach(_root, _column);
		tree.draw_order(_column) = tree.draw_order_const(_root) + 2;

		ui::layout_in_t& column_in = tree.in(_column);
		column_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		column_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		column_in.size_value	   = {1.0f, 1.0f};
		column_in.flow			   = ui::flow_e::column;
		column_in.child_spacing	   = theme.item_spacing;
		column_in.child_margins	   = {theme.margin_vertical * 4, theme.margin_horizontal * 4, theme.margin_vertical * 4, theme.margin_horizontal * 4};

		_title = ui.allocate_widget();
		ui.set_widget_debug_name(_title, "splash_title");
		tree.attach(_column, _title);
		tree.draw_order(_title) = tree.draw_order_const(_column);
		tree.set_visible(_title, true);

		ui.set_widget_text(_title, "stakeforge");
		ui::ui_render_state_t title_state = {};
		title_state.pipeline			  = theme.shader_glitch_lcd;
		paint.set_text(
			_title, ui.widget_text(_title), ui.widget_text_len(_title), {.font = theme.font_sfg, .color = theme.color_text0, .point_size = static_cast<f32>(config.owner_size.y) * 0.15f, .spacing = 0, .raster_mode = ui::glyph_raster_mode_e::lcd}, title_state);

		_version = ui.allocate_widget();
		ui.set_widget_debug_name(_version, "splash_version");
		tree.attach(_column, _version);
		tree.draw_order(_version) = tree.draw_order_const(_column);
		tree.set_visible(_version, true);

		ui.set_widget_text(_version, SFG_EDITOR_SPLASH_VERSION_TEXT);
		paint.set_text(_version,
					   ui.widget_text(_version),
					   ui.widget_text_len(_version),
					   {.font = theme.font_title, .color = theme.color_text1, .point_size = static_cast<f32>(config.owner_size.y) * 0.07f, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_build = ui.allocate_widget();
		ui.set_widget_debug_name(_build, "splash_build");
		tree.attach(_column, _build);
		tree.draw_order(_build) = tree.draw_order_const(_column);
		tree.set_visible(_build, true);

		ui.set_widget_text(_build, SFG_EDITOR_SPLASH_BUILD_TEXT);
		paint.set_text(_build,
					   ui.widget_text(_build),
					   ui.widget_text_len(_build),
					   {.font = theme.font_title, .color = theme.color_text2, .point_size = static_cast<f32>(config.owner_size.y) * 0.07f, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		const editor_project_runtime_t& project_runtime = editor_project_t::get()._runtime;
		const string_t					project_path	= string_t("Project Path: ") + project_runtime.path;
		const string_t					project_name	= string_t("Project Name: ") + project_runtime.name;

		_project_path = ui.allocate_widget();
		ui.set_widget_debug_name(_project_path, "splash_project_path");
		tree.attach(_column, _project_path);
		tree.draw_order(_project_path) = tree.draw_order_const(_column);
		tree.set_visible(_project_path, true);

		ui.set_widget_text(_project_path, project_path.c_str());
		paint.set_text(_project_path,
					   ui.widget_text(_project_path),
					   ui.widget_text_len(_project_path),
					   {.font = theme.font_default, .color = theme.color_text1, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_project_name = ui.allocate_widget();
		ui.set_widget_debug_name(_project_name, "splash_project_name");
		tree.attach(_column, _project_name);
		tree.draw_order(_project_name) = tree.draw_order_const(_column);
		tree.set_visible(_project_name, true);

		ui.set_widget_text(_project_name, project_name.c_str());
		paint.set_text(_project_name,
					   ui.widget_text(_project_name),
					   ui.widget_text_len(_project_name),
					   {.font = theme.font_default, .color = theme.color_text1, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_progress.init(ui, _column, {.progress_text = "Loading", .progress_amount = 0.0f, .frame_height = theme.item_height});
		ui::layout_in_t& progress_in		  = tree.in(_progress.get_root());
		progress_in.pos_mode_y				  = ui::pos_mode_e::relative_in_parent;
		progress_in.pos_value.y				  = 1.0f;
		progress_in.anchor_y				  = ui::anchor_e::end;
		tree.draw_order(_progress.get_root()) = tree.draw_order_const(_column);
	}

	void editor_splash_screen_t::uninit()
	{
		_progress.uninit();
		_ui->deallocate_widget(_root);
		_ui			  = nullptr;
		_root		  = NULL_WIDGET;
		_texture_bg	  = NULL_WIDGET;
		_strikes	  = NULL_WIDGET;
		_column		  = NULL_WIDGET;
		_title		  = NULL_WIDGET;
		_version	  = NULL_WIDGET;
		_build		  = NULL_WIDGET;
		_project_path = NULL_WIDGET;
		_project_name = NULL_WIDGET;
	}

	void editor_splash_screen_t::update_progress(f32 progress)
	{
		_progress.update_progress(progress);
	}

	void editor_splash_screen_t::update_progress_text(const char* text)
	{
		_progress.update_progress_text(text);
	}
}
