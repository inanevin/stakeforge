/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/
#include "ui/widgets/editor_splash_screen.hpp"
#include "editor_project.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"

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

		_column = ui.allocate_widget();
		ui.set_widget_debug_name(_column, "splash_column");
		tree.attach(_root, _column);
		tree.draw_order(_column) = tree.draw_order_const(_root) + 1;

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
			_title, ui.widget_text(_title), ui.widget_text_len(_title), {.font = theme.font_sfg, .color = theme.color_text0, .point_size = static_cast<f32>(config.owner_size.x) * 0.05f, .spacing = 0, .raster_mode = ui::glyph_raster_mode_e::lcd}, title_state);

		_version = ui.allocate_widget();
		ui.set_widget_debug_name(_version, "splash_version");
		tree.attach(_column, _version);
		tree.draw_order(_version) = tree.draw_order_const(_column);
		tree.set_visible(_version, true);

		ui.set_widget_text(_version, SFG_EDITOR_SPLASH_VERSION_TEXT);
		paint.set_text(_version,
					   ui.widget_text(_version),
					   ui.widget_text_len(_version),
					   {.font = theme.font_title, .color = theme.color_text1, .point_size = static_cast<f32>(config.owner_size.x) * 0.02f, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_build = ui.allocate_widget();
		ui.set_widget_debug_name(_build, "splash_build");
		tree.attach(_column, _build);
		tree.draw_order(_build) = tree.draw_order_const(_column);
		tree.set_visible(_build, true);

		ui.set_widget_text(_build, SFG_EDITOR_SPLASH_BUILD_TEXT);
		paint.set_text(_build,
					   ui.widget_text(_build),
					   ui.widget_text_len(_build),
					   {.font = theme.font_title, .color = theme.color_text2, .point_size = static_cast<f32>(config.owner_size.x) * 0.02f, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

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
