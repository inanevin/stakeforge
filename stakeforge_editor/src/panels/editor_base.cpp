// Copyright (c) 2025 Inan Evin

#include "panels/editor_base.hpp"
#include "panels/editor_panel_factory.hpp"
#include "panels/editor_panel_types.hpp"
#include "panels/editor_theme.hpp"
#include "editor_app.hpp"
#include "editor_text_rasterization.hpp"
#include "widgets/editor_widgets_dividers.hpp"
#include "widgets/editor_widgets_draws.hpp"
#include "widgets/editor_widgets_file_menu.hpp"
#include "widgets/editor_widgets_misc.hpp"
#include <sfg/platform/process.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
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
		enum class editor_file_menu_commands_e : u16
		{
			none,
			project_new,
			project_load,
			project_save,
			project_save_as,
			scene_new,
			scene_save,
			scene_save_as,
			scene_load,
			file_exit,
			edit_undo,
			edit_redo,
			edit_cut,
			edit_copy,
			edit_paste,
			view_command_palette,
			view_debug_bounds,
			entity_create_empty,
			entity_create_camera,
			entity_create_light,
			entity_create_mesh,
			entity_duplicate,
			entity_delete,
			entity_rename,
			debug_toggle_bounds,
			debug_toggle_wireframe,
			debug_freeze_culling,
			debug_reload_shaders,
			debug_dump_ui_tree,
			debug_capture_frame,
			help_github,
			help_website,
			help_version,
			help_build_info,
		};

		constexpr u16 PANEL_MENU_COMMAND_BASE = 1000;

		constexpr u16 panel_menu_command(editor_panel_type_e type)
		{
			return static_cast<u16>(PANEL_MENU_COMMAND_BASE + static_cast<u16>(type));
		}

		bool is_panel_menu_command(u16 command)
		{
			return command >= PANEL_MENU_COMMAND_BASE && command < panel_menu_command(editor_panel_type_e::max);
		}

		bool is_editor_debug_bounds_enabled(void*)
		{
			return editor_app_t::get().is_debug_mode_enabled();
		}

		void set_editor_debug_bounds_enabled(u16, bool enabled, void*)
		{
			editor_app_t::get().set_debug_mode(enabled);
		}

		bool is_editor_subpixel_text_enabled(void*)
		{
			return editor_app_t::get().is_text_subpixel_enabled();
		}

		void set_editor_subpixel_text_enabled(u16, bool enabled, void*)
		{
			editor_app_t::get().set_text_subpixel_enabled(enabled);
		}

		const editor_file_menu_row_desc_t FILE_ROWS[] = {
			{.kind = editor_file_menu_row_kind_e::title, .text = "Project"},
			{.text = "New", .command = static_cast<u16>(editor_file_menu_commands_e::project_new)},
			{.text = "Load", .shortcut = "Ctrl+O", .command = static_cast<u16>(editor_file_menu_commands_e::project_load)},
			{.text = "Save", .shortcut = "Ctrl+S", .command = static_cast<u16>(editor_file_menu_commands_e::project_save)},
			{.text = "Save As", .command = static_cast<u16>(editor_file_menu_commands_e::project_save_as)},
			{.kind = editor_file_menu_row_kind_e::title, .text = "Scene"},
			{.text = "New", .command = static_cast<u16>(editor_file_menu_commands_e::scene_new)},
			{.text = "Save", .command = static_cast<u16>(editor_file_menu_commands_e::scene_save)},
			{.text = "Save As", .command = static_cast<u16>(editor_file_menu_commands_e::scene_save_as)},
			{.text = "Load", .command = static_cast<u16>(editor_file_menu_commands_e::scene_load)},
			{.kind = editor_file_menu_row_kind_e::title, .text = "Session"},
			{.text = "Exit", .shortcut = "Alt+F4", .command = static_cast<u16>(editor_file_menu_commands_e::file_exit)},
		};

		const editor_file_menu_row_desc_t EDIT_ROWS[] = {
			{.kind = editor_file_menu_row_kind_e::title, .text = "History"},
			{.text = "Undo", .shortcut = "Ctrl+Z", .command = static_cast<u16>(editor_file_menu_commands_e::edit_undo)},
			{.text = "Redo", .shortcut = "Ctrl+Y", .command = static_cast<u16>(editor_file_menu_commands_e::edit_redo)},
			{.kind = editor_file_menu_row_kind_e::title, .text = "Clipboard"},
			{.text = "Cut", .shortcut = "Ctrl+X", .command = static_cast<u16>(editor_file_menu_commands_e::edit_cut)},
			{.text = "Copy", .shortcut = "Ctrl+C", .command = static_cast<u16>(editor_file_menu_commands_e::edit_copy)},
			{.text = "Paste", .shortcut = "Ctrl+V", .command = static_cast<u16>(editor_file_menu_commands_e::edit_paste)},
		};

		const editor_file_menu_row_desc_t VIEW_ROWS[] = {
			{.kind = editor_file_menu_row_kind_e::title, .text = "Tools"},
			{.text = "Command Palette", .shortcut = "Ctrl+Shift+P", .command = static_cast<u16>(editor_file_menu_commands_e::view_command_palette)},
			{.kind = editor_file_menu_row_kind_e::title, .text = "Panels"},
			{.text = editor_panel_type_to_string(editor_panel_type_e::entities), .command = panel_menu_command(editor_panel_type_e::entities)},
			{.text = editor_panel_type_to_string(editor_panel_type_e::assets), .command = panel_menu_command(editor_panel_type_e::assets)},
			{.text = editor_panel_type_to_string(editor_panel_type_e::log), .command = panel_menu_command(editor_panel_type_e::log)},
			{.text = editor_panel_type_to_string(editor_panel_type_e::world), .command = panel_menu_command(editor_panel_type_e::world)},
			{.text = editor_panel_type_to_string(editor_panel_type_e::inspector), .command = panel_menu_command(editor_panel_type_e::inspector)},
			{.text = editor_panel_type_to_string(editor_panel_type_e::animation), .command = panel_menu_command(editor_panel_type_e::animation)},
			{.text = editor_panel_type_to_string(editor_panel_type_e::profiling), .command = panel_menu_command(editor_panel_type_e::profiling)},
			{.kind = editor_file_menu_row_kind_e::title, .text = "Editor"},
			{.kind = editor_file_menu_row_kind_e::toggle, .text = "Subpixel Text", .toggle_query = is_editor_subpixel_text_enabled, .toggle_callback = set_editor_subpixel_text_enabled},
			{.kind = editor_file_menu_row_kind_e::toggle, .text = "Debug Bounds", .toggle_query = is_editor_debug_bounds_enabled, .toggle_callback = set_editor_debug_bounds_enabled, .close_on_toggle = true},
		};

		const editor_file_menu_row_desc_t ENTITY_ROWS[] = {
			{.kind = editor_file_menu_row_kind_e::title, .text = "Create"},
			{.text = "Empty Entity", .command = static_cast<u16>(editor_file_menu_commands_e::entity_create_empty)},
			{.text = "Camera", .command = static_cast<u16>(editor_file_menu_commands_e::entity_create_camera)},
			{.text = "Light", .command = static_cast<u16>(editor_file_menu_commands_e::entity_create_light)},
			{.text = "Mesh", .command = static_cast<u16>(editor_file_menu_commands_e::entity_create_mesh)},
			{.kind = editor_file_menu_row_kind_e::title, .text = "Selection"},
			{.text = "Duplicate", .shortcut = "Ctrl+D", .command = static_cast<u16>(editor_file_menu_commands_e::entity_duplicate)},
			{.text = "Delete", .shortcut = "Del", .command = static_cast<u16>(editor_file_menu_commands_e::entity_delete)},
			{.text = "Rename", .shortcut = "F2", .command = static_cast<u16>(editor_file_menu_commands_e::entity_rename)},
		};

		const editor_file_menu_row_desc_t DEBUG_ROWS[] = {
			{.kind = editor_file_menu_row_kind_e::title, .text = "Viewport"},
			{.text = "Toggle Bounds", .command = static_cast<u16>(editor_file_menu_commands_e::debug_toggle_bounds)},
			{.text = "Toggle Wireframe", .command = static_cast<u16>(editor_file_menu_commands_e::debug_toggle_wireframe)},
			{.text = "Freeze Culling", .command = static_cast<u16>(editor_file_menu_commands_e::debug_freeze_culling)},
			{.kind = editor_file_menu_row_kind_e::title, .text = "Runtime"},
			{.text = "Reload Shaders", .shortcut = "Ctrl+R", .command = static_cast<u16>(editor_file_menu_commands_e::debug_reload_shaders)},
			{.text = "Dump UI Tree", .command = static_cast<u16>(editor_file_menu_commands_e::debug_dump_ui_tree)},
			{.text = "Capture Frame", .command = static_cast<u16>(editor_file_menu_commands_e::debug_capture_frame)},
		};

		const editor_file_menu_row_desc_t HELP_ROWS[] = {
			{.kind = editor_file_menu_row_kind_e::title, .text = "Links"},
			{.text = "GitHub", .command = static_cast<u16>(editor_file_menu_commands_e::help_github)},
			{.text = "Website", .command = static_cast<u16>(editor_file_menu_commands_e::help_website)},
			{.kind = editor_file_menu_row_kind_e::title, .text = "About"},
			{.text = "Version", .command = static_cast<u16>(editor_file_menu_commands_e::help_version)},
			{.text = "Build Info", .command = static_cast<u16>(editor_file_menu_commands_e::help_build_info)},
		};

		const editor_file_menu_item_desc_t FILE_MENU_ITEMS[] = {
			{.text = "File", .rows = FILE_ROWS, .row_count = static_cast<u16>(sizeof(FILE_ROWS) / sizeof(FILE_ROWS[0]))},
			{.text = "Edit", .rows = EDIT_ROWS, .row_count = static_cast<u16>(sizeof(EDIT_ROWS) / sizeof(EDIT_ROWS[0]))},
			{.text = "View", .rows = VIEW_ROWS, .row_count = static_cast<u16>(sizeof(VIEW_ROWS) / sizeof(VIEW_ROWS[0]))},
			{.text = "Entity", .rows = ENTITY_ROWS, .row_count = static_cast<u16>(sizeof(ENTITY_ROWS) / sizeof(ENTITY_ROWS[0]))},
			{.text = "Debug", .rows = DEBUG_ROWS, .row_count = static_cast<u16>(sizeof(DEBUG_ROWS) / sizeof(DEBUG_ROWS[0]))},
			{.text = "Help", .rows = HELP_ROWS, .row_count = static_cast<u16>(sizeof(HELP_ROWS) / sizeof(HELP_ROWS[0]))},
		};

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

		void on_minimize_window(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void*)
		{
			if (btn != ui::mouse_button_e::left)
				return;

			process::minimize_window(editor_app_t::get().get_main_surface().runtime->window_handle);
		}

		void on_maximize_window(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void*)
		{
			if (btn != ui::mouse_button_e::left)
				return;

			process::toggle_maximize_window(editor_app_t::get().get_main_surface().runtime->window_handle);
		}

		void on_close_window(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void*)
		{
			if (btn != ui::mouse_button_e::left)
				return;

			editor_app_t::get().get_main_surface().runtime->set_flag(window_runtime_flags_e::close_requested);
		}

		void request_error_modal(const char* title, const char* description)
		{
			editor_modal_button_desc_t buttons[] = {
				{.text = "Ok"},
			};
			editor_app_t::get().get_modal_controller().request_modal(title, description, buttons, static_cast<u16>(sizeof(buttons) / sizeof(buttons[0])), editor_modal_severity_e::error);
		}

		void open_project_from_dialog()
		{
			const string_t path = process::select_file("Open Project", "sfg_project");
			if (path.empty())
				return;
			if (!editor_app_t::get().load_project(path.c_str()))
				request_error_modal("Failed Loading Project", "Selected file could not be loaded as a Stakeforge project.");
		}

		void create_project_from_dialog()
		{
			const string_t path = process::save_file("Create Project", "sfg_project");
			if (path.empty())
				return;
			if (!editor_app_t::get().create_project(path.c_str()))
				request_error_modal("Failed Creating Project", "New project could not be created.");
		}

		void save_project_as_from_dialog()
		{
			const string_t path = process::save_file("Save Project As", "sfg_project");
			if (path.empty())
				return;
			if (!editor_app_t::get().save_project_as(path.c_str()))
				request_error_modal("Failed Saving Project", "Current project could not be saved.");
		}

		void on_open_modal_save(void* user_data)
		{
			editor_base_t& base = *static_cast<editor_base_t*>(user_data);
			base.complete_project_save_prompt(true);
		}

		void on_open_modal_dont_save(void* user_data)
		{
			editor_base_t& base = *static_cast<editor_base_t*>(user_data);
			base.complete_project_save_prompt(false);
		}

		void on_open_modal_cancel(void* user_data)
		{
			editor_base_t& base = *static_cast<editor_base_t*>(user_data);
			base.cancel_project_save_prompt();
		}

		void on_file_menu_command(u16 command, void* user_data)
		{
			editor_base_t& base = *static_cast<editor_base_t*>(user_data);

			if (is_panel_menu_command(command))
				return;

			switch (static_cast<editor_file_menu_commands_e>(command))
			{
			case editor_file_menu_commands_e::project_new:
				base.prompt_project_save_modal(editor_project_prompt_action_e::new_project);
				break;
			case editor_file_menu_commands_e::project_load:
				base.prompt_project_save_modal(editor_project_prompt_action_e::load_project);
				break;
			case editor_file_menu_commands_e::project_save:
				if (!editor_app_t::get().save_project())
					request_error_modal("Failed Saving Project", "Current project could not be saved.");
				break;
			case editor_file_menu_commands_e::project_save_as:
				save_project_as_from_dialog();
				break;
			case editor_file_menu_commands_e::scene_new:
			case editor_file_menu_commands_e::scene_save:
			case editor_file_menu_commands_e::scene_save_as:
			case editor_file_menu_commands_e::scene_load:
				break;
			case editor_file_menu_commands_e::file_exit:
				editor_app_t::get().get_main_surface().runtime->set_flag(window_runtime_flags_e::close_requested);
				break;
			case editor_file_menu_commands_e::view_debug_bounds:
			case editor_file_menu_commands_e::debug_toggle_bounds:
				editor_app_t::get().set_debug_mode(!editor_app_t::get().is_debug_mode_enabled());
				break;
			case editor_file_menu_commands_e::help_github:
				process::open_url("https://github.com/inanevin/stakeforge");
				break;
			case editor_file_menu_commands_e::help_website:
				process::open_url("https://www.inanevin.com");
				break;
			default:
				break;
			}
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
		ui::widget_id_t		  top_section					 = NULL_WIDGET;
		ui::widget_id_t		  top_row_left					 = NULL_WIDGET;
		ui::widget_id_t		  top_row_strikes				 = NULL_WIDGET;
		ui::widget_id_t		  top_row_mid					 = NULL_WIDGET;
		ui::widget_id_t		  top_mid_file					 = NULL_WIDGET;
		ui::widget_id_t		  top_mid_divider				 = NULL_WIDGET;
		ui::widget_id_t		  top_mid_util					 = NULL_WIDGET;
		ui::widget_id_t		  top_row_right					 = NULL_WIDGET;
		ui::widget_id_t		  top_row_right_buttons			 = NULL_WIDGET;
		ui::widget_id_t		  window_minimize				 = NULL_WIDGET;
		ui::widget_id_t		  window_maximize				 = NULL_WIDGET;
		ui::widget_id_t		  window_close					 = NULL_WIDGET;
		ui::widget_id_t		  label_wrap					 = NULL_WIDGET;
		ui::widget_id_t		  title_group					 = NULL_WIDGET;
		ui::widget_id_t		  title_label					 = NULL_WIDGET;
		ui::widget_id_t		  version_label					 = NULL_WIDGET;
		ui::widget_id_t		  build_label					 = NULL_WIDGET;
		ui::widget_id_t		  bottom_section				 = NULL_WIDGET;
		ui.set_debug_font(theme.font_default);

		// base
		{
			_base = ui.allocate_widget();
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
			top_section = ui.allocate_widget();
			ui.set_widget_debug_name(top_section, "top_section");
			tree.attach(_base, top_section);

			ui::layout_in_t& in = tree.in(top_section);
			in.size_mode_x		= ui::axis_mode_e::parent_relative;
			in.size_mode_y		= ui::axis_mode_e::fixed;
			in.size_value		= {1.0f, item_height * 3.0f};
			in.flow				= ui::flow_e::row;
			in.child_spacing	= 0.0f;
			in.child_margins	= {0.0f, 0.0f, 0.0f, 0.0f};

			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = theme.color_frame_light;
			rect.fill_color_b		 = theme.color_frame_light;
			paint.set_rect(top_section, rect);
		}

		// top-left title
		{
			top_row_left  = ui.allocate_widget();
			_top_row_left = top_row_left;
			ui.set_widget_debug_name(top_row_left, "top_row_left");
			tree.attach(top_section, top_row_left);

			ui::layout_in_t& in = tree.in(top_row_left);
			in.size_mode_x		= ui::axis_mode_e::sum_children;
			in.size_mode_y		= ui::axis_mode_e::parent_relative;
			in.size_value		= {0.0f, 1.0f};
			in.flow				= ui::flow_e::row;
			in.child_spacing	= 0.0f;
			in.child_margins	= {0.0f, theme.margin_horizontal * 2, 0.0f, theme.margin_horizontal * 2};

			title_group = ui.allocate_widget();
			ui.set_widget_debug_name(title_group, "title_group");
			tree.attach(top_row_left, title_group);

			ui::layout_in_t& group_in = tree.in(title_group);
			group_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
			group_in.pos_value.y	  = 0.5f;
			group_in.anchor_y		  = ui::anchor_e::center;
			group_in.size_mode_x	  = ui::axis_mode_e::max_children;
			group_in.size_mode_y	  = ui::axis_mode_e::sum_children;
			group_in.flow			  = ui::flow_e::column;
			group_in.child_spacing	  = theme.item_spacing * 0.5f;
			group_in.child_margins	  = {0.0f, 0.0f, 0.0f, 0.0f};

			title_label = ui.allocate_widget();
			ui.set_widget_debug_name(title_label, "title_label");
			tree.attach(title_group, title_label);

			ui.set_widget_text(title_label, "stakeforge");
			ui::ui_render_state_t title_state = {};
			title_state.pipeline			  = theme.shader_glitch_lcd;
			paint.set_text(title_label, ui.widget_text(title_label), ui.widget_text_len(title_label), {.font = theme.font_sfg, .color = theme.color_text0, .point_size = 20.0f, .spacing = 0, .raster_mode = ui::glyph_raster_mode_e::lcd}, title_state);

			version_label = ui.allocate_widget();
			ui.set_widget_debug_name(version_label, "version_label");
			tree.attach(title_group, version_label);

			ui.set_widget_text(version_label, SFG_EDITOR_VERSION_TEXT);
			paint.set_text(
				version_label, ui.widget_text(version_label), ui.widget_text_len(version_label), {.font = theme.font_sfg, .color = theme.color_text1, .point_size = 10.0f, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

			build_label = ui.allocate_widget();
			ui.set_widget_debug_name(build_label, "build_label");
			tree.attach(title_group, build_label);

			ui.set_widget_text(build_label, SFG_EDITOR_BUILD_TEXT);
			paint.set_text(
				build_label, ui.widget_text(build_label), ui.widget_text_len(build_label), {.font = theme.font_sfg, .color = theme.color_text2, .point_size = 10.0f, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		}

		// top-left strikes
		{
			top_row_strikes	 = ui.allocate_widget();
			_top_row_strikes = top_row_strikes;
			ui.set_widget_debug_name(top_row_strikes, "top_row_strikes");
			tree.attach(top_section, top_row_strikes);

			ui::layout_in_t& in = tree.in(top_row_strikes);
			in.size_mode_x		= ui::axis_mode_e::fixed;
			in.size_mode_y		= ui::axis_mode_e::parent_relative;
			in.size_value		= {item_height * 3.0f, 1.0f};

			paint.set_custom(top_row_strikes, draw_top_row_strikes, &ui);
		}

		// top-mid
		{
			top_row_mid = ui.allocate_widget();
			ui.set_widget_debug_name(top_row_mid, "top_row_mid");
			tree.attach(top_section, top_row_mid);

			ui::layout_in_t& in = tree.in(top_row_mid);
			in.size_mode_x		= ui::axis_mode_e::fill;
			in.size_mode_y		= ui::axis_mode_e::parent_relative;
			in.size_value		= {1.0f, 1.0f};
			in.flow				= ui::flow_e::column;
			in.child_spacing	= 0.0f;
			in.child_margins	= {0.0f, 0.0f, 0.0f, 0.0f};

			top_mid_file  = ui.allocate_widget();
			_top_mid_file = top_mid_file;
			ui.set_widget_debug_name(top_mid_file, "top_mid_file");
			tree.attach(top_row_mid, top_mid_file);

			ui::layout_in_t& file_in = tree.in(top_mid_file);
			file_in.size_mode_x		 = ui::axis_mode_e::fill;
			file_in.size_mode_y		 = ui::axis_mode_e::fixed;
			file_in.size_value		 = {1.0f, theme.item_height};

			editor_file_menu_style_t file_menu_style = {};
			file_menu_style.frame_color				 = {0, 0, 0, 0};
			file_menu_style.hover_color				 = theme.color_light;
			file_menu_style.press_color				 = theme.color_panel;
			file_menu_style.selected_color			 = theme.color_panel;
			file_menu_style.dropdown_color			 = theme.color_frame_light;
			file_menu_style.text_color				 = theme.color_text0;
			file_menu_style.shortcut_color			 = theme.color_text2;
			file_menu_style.title_color				 = theme.color_text1;
			file_menu_style.title_line_color		 = theme.color_text2;
			file_menu_style.icon_color				 = theme.color_text0;
			file_menu_style.button_width			 = theme.item_height * 2.5f;
			file_menu_style.row_height				 = theme.item_height;
			file_menu_style.text_size				 = theme.text_default_px_size;
			file_menu_style.shortcut_size			 = theme.text_small_title_px_size;
			file_menu_style.title_size				 = theme.text_med_title_px_size;
			file_menu_style.title_line_thickness	 = theme.divider_thickness;
			file_menu_style.icon_size				 = theme.icon_default_px_size;
			file_menu_style.padding_x				 = theme.indent_horizontal;
			file_menu_style.padding_y				 = theme.item_spacing;
			file_menu_style.shortcut_gap			 = theme.item_height * 1.75f;
			file_menu_style.title_gap				 = theme.indent_horizontal;
			_file_menu.init(ui, top_mid_file, FILE_MENU_ITEMS, static_cast<u16>(sizeof(FILE_MENU_ITEMS) / sizeof(FILE_MENU_ITEMS[0])), file_menu_style, on_file_menu_command, this);

			top_mid_divider = editor_dividers_t::add_divider_hor(ui, top_row_mid, theme.divider_thickness, theme.color_text2, theme.color_panel_light, ui::vg_gradient_e::horizontal);
			ui.set_widget_debug_name(top_mid_divider, "top_mid_divider");

			top_mid_util  = ui.allocate_widget();
			_top_mid_util = top_mid_util;
			ui.set_widget_debug_name(top_mid_util, "top_mid_util");
			tree.attach(top_row_mid, top_mid_util);

			ui::layout_in_t& util_in = tree.in(top_mid_util);
			util_in.size_mode_x		 = ui::axis_mode_e::fill;
			util_in.size_mode_y		 = ui::axis_mode_e::fill;
			util_in.size_value		 = {1.0f, 1.0f};
		}

		// top-right
		{
			top_row_right = ui.allocate_widget();
			ui.set_widget_debug_name(top_row_right, "top_row_right");
			tree.attach(top_section, top_row_right);

			ui::layout_in_t& in = tree.in(top_row_right);
			in.size_mode_x		= ui::axis_mode_e::fixed;
			in.size_mode_y		= ui::axis_mode_e::parent_relative;
			in.size_value		= {theme.item_height * 6, 1.0f};
			in.flow				= ui::flow_e::column;
			in.child_spacing	= theme.item_spacing;
			in.child_margins	= {0.0f, 0.0f, 0.0f, 0.0f};

			top_row_right_buttons = ui.allocate_widget();
			ui.set_widget_debug_name(top_row_right_buttons, "top_row_right_buttons");
			tree.attach(top_row_right, top_row_right_buttons);

			ui::layout_in_t& buttons_in = tree.in(top_row_right_buttons);
			buttons_in.pos_mode_y		= ui::pos_mode_e::flow;
			buttons_in.pos_value.y		= 0.0f;
			buttons_in.anchor_y			= ui::anchor_e::start;
			buttons_in.size_mode_x		= ui::axis_mode_e::parent_relative;
			buttons_in.size_mode_y		= ui::axis_mode_e::fixed;
			buttons_in.size_value		= {1.0f, theme.item_height};
			buttons_in.flow				= ui::flow_e::row;
			buttons_in.child_spacing	= 0.0f;
			buttons_in.child_margins	= {0.0f, 0.0f, 0.0f, 0.0f};

			const editor_window_buttons_t window_buttons = editor_misc_widgets_t::add_window_buttons(ui, top_row_right_buttons, theme.color_frame, theme.color_accent_err, theme.color_light, theme.color_panel, theme.color_text0, theme.icon_default_px_size);
			window_minimize								 = window_buttons.minimize_frame;
			window_maximize								 = window_buttons.maximize_frame;
			window_close								 = window_buttons.close_frame;

			ui::listener_bundle_t listener = {};
			listener.user_data			   = this;
			listener.on_click			   = on_minimize_window;
			ui.get_input().set_listener(window_minimize, listener);
			listener.on_click = on_maximize_window;
			ui.get_input().set_listener(window_maximize, listener);
			listener.on_click = on_close_window;
			ui.get_input().set_listener(window_close, listener);

			label_wrap	= ui.allocate_widget();
			_label_wrap = label_wrap;
			ui.set_widget_debug_name(label_wrap, "label_wrap");
			tree.attach(top_row_right, label_wrap);

			ui::layout_in_t& label_wrap_in = tree.in(label_wrap);
			label_wrap_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
			label_wrap_in.pos_mode_y	   = ui::pos_mode_e::flow;
			label_wrap_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
			label_wrap_in.size_mode_y	   = ui::axis_mode_e::fill;
			label_wrap_in.size_value.x	   = 1.0f;

			_project_label = ui.allocate_widget();
			ui.set_widget_debug_name(_project_label, "project_label");
			tree.attach(label_wrap, _project_label);
			tree.draw_order(_project_label) = tree.draw_order_const(top_row_right) + 1;

			ui::layout_in_t& project_in = tree.in(_project_label);
			project_in.pos_mode_x		= ui::pos_mode_e::relative_in_parent;
			project_in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
			project_in.pos_value		= {0.5f, 0.5f};
			project_in.anchor_x			= ui::anchor_e::center;
			project_in.anchor_y			= ui::anchor_e::center;

			ui.set_widget_text(_project_label, "");
			paint.set_text(_project_label,
						   ui.widget_text(_project_label),
						   ui.widget_text_len(_project_label),
						   {.font = theme.font_sfg, .color = theme.color_text1, .point_size = theme.text_med_title_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		}

		editor_dividers_t::add_divider_hor(ui, _base, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);

		_dock_widget.init(ui, _base);
		ui::layout_in_t& dock_in		   = tree.in(_dock_widget.get_root());
		dock_in.size_mode_y				   = ui::axis_mode_e::fill;
		const dock_node_handle_t demo_leaf = _dock_widget.create_leaf_node(_dock_widget.get_root());
		_dock_widget.set_root_node(demo_leaf);
		_dock_widget.dock_node_add_panel(demo_leaf, editor_panel_factory_t::create_panel(editor_panel_type_e::entities));
		_dock_widget.dock_node_add_panel(demo_leaf, editor_panel_factory_t::create_panel(editor_panel_type_e::world));

		editor_dividers_t::add_divider_hor(ui, _base, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);

		// bottom
		{
			bottom_section = ui.allocate_widget();
			ui.set_widget_debug_name(bottom_section, "bottom_section");
			tree.attach(_base, bottom_section);

			ui::layout_in_t& in = tree.in(bottom_section);
			in.size_mode_x		= ui::axis_mode_e::parent_relative;
			in.size_mode_y		= ui::axis_mode_e::fixed;
			in.size_value		= {1.0f, item_height};

			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = theme.color_frame_light;
			rect.fill_color_b		 = theme.color_frame_light;
			paint.set_rect(bottom_section, rect);
		}
	}

	void editor_base_t::uninit()
	{
		_dock_widget.uninit();
		_file_menu.uninit();

		_ui->deallocate_widget(_base);

		_ui							   = nullptr;
		_base						   = NULL_WIDGET;
		_project_label				   = NULL_WIDGET;
		_top_row_left				   = NULL_WIDGET;
		_top_row_strikes			   = NULL_WIDGET;
		_top_mid_file				   = NULL_WIDGET;
		_top_mid_util				   = NULL_WIDGET;
		_label_wrap					   = NULL_WIDGET;
		_pending_project_prompt_action = editor_project_prompt_action_e::none;
	}

	void editor_base_t::set_current_project_name(const char* name)
	{
		_ui->set_widget_text(_project_label, name);
	}

	bool editor_base_t::is_window_drag_region(const vec2i16_t& pos) const
	{
		const vec2f_t p = {static_cast<f32>(pos.x), static_cast<f32>(pos.y)};

		const vec4f_t left = _ui->get_tree().bounds(_top_row_left);
		if (p.x >= left.x && p.x <= left.x + left.z && p.y >= left.y && p.y <= left.y + left.w)
			return true;

		const vec4f_t strikes = _ui->get_tree().bounds(_top_row_strikes);
		if (p.x >= strikes.x && p.x <= strikes.x + strikes.z && p.y >= strikes.y && p.y <= strikes.y + strikes.w)
			return true;

		const vec4f_t file_row	= _ui->get_tree().bounds(_top_mid_file);
		const vec4f_t file_menu = _ui->get_tree().bounds(_file_menu.get_root());
		if (p.x >= file_row.x && p.x <= file_row.x + file_row.z && p.y >= file_row.y && p.y <= file_row.y + file_row.w)
			return p.x > file_menu.x + file_menu.z;

		const vec4f_t util = _ui->get_tree().bounds(_top_mid_util);
		if (p.x >= util.x && p.x <= util.x + util.z && p.y >= util.y && p.y <= util.y + util.w)
			return true;

		const vec4f_t label = _ui->get_tree().bounds(_label_wrap);
		return p.x >= label.x && p.x <= label.x + label.z && p.y >= label.y && p.y <= label.y + label.w;
	}

	void editor_base_t::prompt_project_save_modal(editor_project_prompt_action_e action)
	{
		_pending_project_prompt_action		 = action;
		editor_modal_button_desc_t buttons[] = {
			{.text = "Save", .callback = on_open_modal_save, .user_data = this},
			{.text = "Don't Save", .callback = on_open_modal_dont_save, .user_data = this},
			{.text = "Cancel", .callback = on_open_modal_cancel, .user_data = this},
		};
		editor_app_t::get().get_modal_controller().request_modal("Would you like to save?", "Save current changes before continuing?", buttons, static_cast<u16>(sizeof(buttons) / sizeof(buttons[0])));
	}

	void editor_base_t::complete_project_save_prompt(bool save)
	{
		const editor_project_prompt_action_e action = _pending_project_prompt_action;
		_pending_project_prompt_action				= editor_project_prompt_action_e::none;

		if (save && !editor_app_t::get().save_project())
		{
			request_error_modal("Failed Saving Project", "Current project could not be saved.");
			return;
		}

		switch (action)
		{
		case editor_project_prompt_action_e::new_project:
			create_project_from_dialog();
			break;
		case editor_project_prompt_action_e::load_project:
			open_project_from_dialog();
			break;
		default:
			break;
		}
	}

	void editor_base_t::cancel_project_save_prompt()
	{
		_pending_project_prompt_action = editor_project_prompt_action_e::none;
	}

	void editor_base_t::prompt_no_project_modal()
	{
		editor_modal_button_desc_t buttons[] = {
			{.text = "Open", .callback = on_no_project_open, .user_data = this},
			{.text = "Create", .callback = on_no_project_create, .user_data = this},
		};
		editor_app_t::get().get_modal_controller().request_modal("No Project", "No last project found. Open an existing project or create a new one.", buttons, static_cast<u16>(sizeof(buttons) / sizeof(buttons[0])), editor_modal_severity_e::warning);
	}

	void editor_base_t::on_no_project_open(void* user_data)
	{
		editor_base_t& base = *static_cast<editor_base_t*>(user_data);
		const string_t path = process::select_file("Open Project", "sfg_project");
		if (!editor_app_t::get().load_project(path.c_str()))
			base.prompt_no_project_modal();
	}

	void editor_base_t::on_no_project_create(void* user_data)
	{
		editor_base_t& base = *static_cast<editor_base_t*>(user_data);
		const string_t path = process::save_file("Create Project", "sfg_project");
		if (path.empty())
		{
			base.prompt_no_project_modal();
			return;
		}

		if (!editor_app_t::get().create_project(path.c_str()))
			base.prompt_no_project_modal();
	}
}
