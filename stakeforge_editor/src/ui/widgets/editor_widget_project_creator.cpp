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
#include "ui/widgets/editor_widget_project_creator.hpp"
#include "editor_directories.hpp"
#include "editor_project.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	editor_widget_project_create_config_reflection_t::editor_widget_project_create_config_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		registry.register_type({
			.name		  = "editor_widget_project_create_config_t",
			.display_name = "Project",
			.fields =
				{
					{.name		   = "directory",
					 .display_name = "Directory",
					 .sub_type_id  = REFLECTION_SUB_TYPE_IDENTIFIER_DIRECTORY,
					 .offset	   = offsetof(editor_widget_project_create_config_t, directory),
					 .size		   = sizeof(string_t),
					 .type		   = reflected_value_type_e::string},
					{.name = "name", .display_name = "Name", .offset = offsetof(editor_widget_project_create_config_t, name), .size = sizeof(string_t), .type = reflected_value_type_e::string},
				},
			.type_id   = type_id_t<editor_widget_project_create_config_t>::value,
			.size	   = sizeof(editor_widget_project_create_config_t),
			.alignment = alignof(editor_widget_project_create_config_t),
		});
	}

	editor_widget_project_load_config_reflection_t::editor_widget_project_load_config_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		registry.register_type({
			.name		  = "editor_widget_project_load_config_t",
			.display_name = "Project",
			.fields =
				{
					{.name = "path", .display_name = "Path", .sub_type_id = REFLECTION_SUB_TYPE_IDENTIFIER_PATH, .offset = offsetof(editor_widget_project_load_config_t, path), .size = sizeof(string_t), .type = reflected_value_type_e::string},
				},
			.type_id   = type_id_t<editor_widget_project_load_config_t>::value,
			.size	   = sizeof(editor_widget_project_load_config_t),
			.alignment = alignof(editor_widget_project_load_config_t),
		});
	}

	void editor_widget_project_creator_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_project_creator_config_t& config)
	{
		_ui							= &ui;
		_creator_config				= config;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "project_creator");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in	  = tree.in(_root);
		root_in.pos_mode_x			  = ui::pos_mode_e::relative_in_parent;
		root_in.pos_mode_y			  = ui::pos_mode_e::relative_in_parent;
		root_in.pos_value			  = {0.0f, 0.0f};
		root_in.size_mode_x			  = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y			  = ui::axis_mode_e::parent_relative;
		root_in.size_value			  = {1.0f, 1.0f};
		root_in.flow				  = ui::flow_e::column;
		const float outline_thickness = theme.outline_thickness * 2;
		root_in.child_margins		  = {outline_thickness, outline_thickness, outline_thickness, outline_thickness};

		_inner_root = ui.allocate_widget();
		ui.set_widget_debug_name(_inner_root, "project_creator_inner");
		tree.attach(_root, _inner_root);

		ui::layout_in_t& inner_root_in = tree.in(_inner_root);
		inner_root_in.pos_mode_x	   = ui::pos_mode_e::flow;
		inner_root_in.pos_mode_y	   = ui::pos_mode_e::flow;
		inner_root_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		inner_root_in.size_mode_y	   = ui::axis_mode_e::fill;
		inner_root_in.size_value	   = {1.0f, 1.0f};
		inner_root_in.flow			   = ui::flow_e::column;
		inner_root_in.child_spacing	   = theme.item_spacing;
		inner_root_in.child_margins	   = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};

		ui::vg_rect_paint_t inner_root_rect = {};
		inner_root_rect.fill_color_a		= theme.color_panel;
		inner_root_rect.fill_color_b		= theme.color_panel;
		inner_root_rect.outline_color		= theme.color_frame;
		inner_root_rect.outline_thickness	= theme.outline_thickness;
		paint.set_rect(_inner_root, inner_root_rect);

		_scroll_area = ui.allocate_widget();
		ui.set_widget_debug_name(_scroll_area, "project_creator_scroll_area");
		tree.attach(_inner_root, _scroll_area);

		ui::layout_in_t& scroll_area_in = tree.in(_scroll_area);
		scroll_area_in.flags			= ui::wf_visible | ui::wf_input | ui::wf_scroll_y;
		scroll_area_in.child_clip_mode	= ui::clip_mode_e::scissor_rect;
		scroll_area_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		scroll_area_in.size_mode_y		= ui::axis_mode_e::fill;
		scroll_area_in.size_value		= {1.0f, 1.0f};

		_wrapper = ui.allocate_widget();
		ui.set_widget_debug_name(_wrapper, "project_creator_wrapper");
		tree.attach(_scroll_area, _wrapper);

		ui::layout_in_t& wrapper_in = tree.in(_wrapper);
		wrapper_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		wrapper_in.size_mode_y		= ui::axis_mode_e::sum_children;
		wrapper_in.size_value		= {1.0f, 1.0f};
		wrapper_in.flow				= ui::flow_e::column;
		wrapper_in.child_spacing	= theme.item_spacing;
		tree.draw_order(_wrapper)	= tree.draw_order_const(_scroll_area) + 1;

		_load_description = ui.allocate_widget();
		ui.set_widget_debug_name(_load_description, "project_creator_load_description");
		tree.attach(_wrapper, _load_description);

		ui::layout_in_t& load_description_in = tree.in(_load_description);
		load_description_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		load_description_in.size_mode_y		 = ui::axis_mode_e::fixed;
		load_description_in.size_value		 = {1.0f, theme.item_area_height};

		ui.set_widget_text(_load_description, "Select a project file to load");
		paint.set_text(_load_description,
					   ui.widget_text(_load_description),
					   ui.widget_text_len(_load_description),
					   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		void* load_object = &_load_config;
		_load_reflection.init(ui, _wrapper, {.objects = {.data = &load_object, .size = 1}, .type_id = type_id_t<editor_widget_project_load_config_t>::value});

		_load_button.init(ui, _wrapper, {.text = "Load", .width = {.mode = editor_widget_width_e::fixed, .value = theme.item_width}});
		ui::layout_in_t& load_button_in = tree.in(_load_button.get_root());
		load_button_in.pos_mode_x		= ui::pos_mode_e::relative_in_parent;
		load_button_in.pos_value.x		= 0.5f;
		load_button_in.anchor_x			= ui::anchor_e::center;

		ui::listener_bundle_t load_listener = {};
		load_listener.on_click				= on_load_pressed;
		load_listener.user_data				= this;
		ui.get_input().set_listener(_load_button.get_root(), load_listener);

		_create_description = ui.allocate_widget();
		ui.set_widget_debug_name(_create_description, "project_creator_create_description");
		tree.attach(_wrapper, _create_description);

		ui::layout_in_t& create_description_in = tree.in(_create_description);
		create_description_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		create_description_in.size_mode_y	   = ui::axis_mode_e::fixed;
		create_description_in.size_value	   = {1.0f, theme.item_area_height};

		ui.set_widget_text(_create_description, "Create a new project by selecting directory and typing a name.");
		paint.set_text(_create_description,
					   ui.widget_text(_create_description),
					   ui.widget_text_len(_create_description),
					   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		void* create_object = &_create_config;
		_create_reflection.init(ui, _wrapper, {.objects = {.data = &create_object, .size = 1}, .type_id = type_id_t<editor_widget_project_create_config_t>::value});

		_create_button.init(ui, _wrapper, {.text = "Create", .width = {.mode = editor_widget_width_e::fixed, .value = theme.item_width}});
		ui::layout_in_t& create_button_in = tree.in(_create_button.get_root());
		create_button_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		create_button_in.pos_value.x	  = 0.5f;
		create_button_in.anchor_x		  = ui::anchor_e::center;

		ui::listener_bundle_t create_listener = {};
		create_listener.on_click			  = on_create_pressed;
		create_listener.user_data			  = this;
		ui.get_input().set_listener(_create_button.get_root(), create_listener);

		_error_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_error_frame, "project_creator_error_frame");
		tree.attach(_wrapper, _error_frame);

		ui::layout_in_t& error_frame_in = tree.in(_error_frame);
		error_frame_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		error_frame_in.size_mode_y		= ui::axis_mode_e::fixed;
		error_frame_in.size_value		= {1.0f, theme.item_area_height};

		ui::vg_rect_paint_t error_rect = {};
		error_rect.fill_color_a		   = theme.color_frame;
		error_rect.fill_color_b		   = theme.color_frame;
		paint.set_rect(_error_frame, error_rect);

		_error_label = ui.allocate_widget();
		ui.set_widget_debug_name(_error_label, "project_creator_error_label");
		tree.attach(_error_frame, _error_label);
		tree.draw_order(_error_label) = tree.draw_order_const(_error_frame) + 1;

		ui::layout_in_t& error_label_in = tree.in(_error_label);
		error_label_in.pos_mode_x		= ui::pos_mode_e::relative_in_parent;
		error_label_in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
		error_label_in.pos_value		= {0.5f, 0.5f};
		error_label_in.anchor_x			= ui::anchor_e::center;
		error_label_in.anchor_y			= ui::anchor_e::center;

		ui.set_widget_text(_error_label, "Project creator error debug");
		paint.set_text(_error_label,
					   ui.widget_text(_error_label),
					   ui.widget_text_len(_error_label),
					   {.font = theme.font_default, .color = theme.color_accent_err, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		hide_error();
		_scrollbar.init(ui, {.target = _scroll_area, .axes = editor_scrollbar_axis_y});
	}

	void editor_widget_project_creator_t::uninit()
	{
		_scrollbar.uninit();
		_ui->get_input().clear_listener(_load_button.get_root());
		_ui->get_input().clear_listener(_create_button.get_root());
		_load_button.uninit();
		_create_button.uninit();
		_load_reflection.uninit();
		_create_reflection.uninit();
		_ui->deallocate_widget(_root);
		_creator_config		= {};
		_load_config		= {};
		_create_config		= {};
		_ui					= nullptr;
		_root				= NULL_WIDGET;
		_inner_root			= NULL_WIDGET;
		_scroll_area		= NULL_WIDGET;
		_wrapper			= NULL_WIDGET;
		_load_description	= NULL_WIDGET;
		_create_description = NULL_WIDGET;
		_error_frame		= NULL_WIDGET;
		_error_label		= NULL_WIDGET;
	}

	void editor_widget_project_creator_t::on_load_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		static_cast<editor_widget_project_creator_t*>(user_data)->try_load();
	}

	void editor_widget_project_creator_t::on_create_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		static_cast<editor_widget_project_creator_t*>(user_data)->try_create();
	}

	void editor_widget_project_creator_t::show_error(const char* text)
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		ui::paint_layer_t&	  paint = _ui->get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_ui->set_widget_text(_error_label, text);
		paint.set_text(_error_label,
					   _ui->widget_text(_error_label),
					   _ui->widget_text_len(_error_label),
					   {.font = theme.font_default, .color = theme.color_accent_err, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		tree.set_visible(_error_frame, true);
	}

	void editor_widget_project_creator_t::hide_error()
	{
		ui::layout_tree_t& tree = _ui->get_tree();
		tree.set_visible(_error_frame, false);
	}

	void editor_widget_project_creator_t::notify_project_ready()
	{
		hide_error();
		_creator_config.on_project_ready(_creator_config.user_data);
	}

	void editor_widget_project_creator_t::try_load()
	{
		if (_load_config.path.empty())
		{
			show_error("Select a project file.");
			return;
		}

		string_t path = _load_config.path;
		file_system_t::fix_path(path);
		if (!file_system_t::exists(path.c_str()))
		{
			SFG_ERR("project file does not exist {0}", path.c_str());
			show_error("Selected project file does not exist.");
			return;
		}

		if (file_system_t::get_file_extension(path) != "sfg_project")
		{
			show_error("Selected file is not a Stakeforge project.");
			return;
		}

		if (!editor_project_t::get().try_load(path.c_str()))
		{
			SFG_ERR("failed loading project {0}", path.c_str());
			show_error("Selected project could not be loaded.");
			return;
		}

		notify_project_ready();
	}

	void editor_widget_project_creator_t::try_create()
	{
		if (_create_config.directory.empty())
		{
			show_error("Select a project directory.");
			return;
		}

		string_t directory = _create_config.directory;
		file_system_t::fix_path(directory);
		if (!file_system_t::is_directory(directory.c_str()))
		{
			show_error("Selected directory does not exist.");
			return;
		}

		if (!editor_directories_t::is_valid_asset_name(_create_config.name.c_str()))
		{
			show_error("Enter a valid project name.");
			return;
		}

		file_system_t::fix_path_end_slash(directory);
		string_t path = directory + _create_config.name;
		if (file_system_t::get_file_extension(path) != "sfg_project")
			path += ".sfg_project";

		if (file_system_t::exists(path.c_str()))
		{
			show_error("Project file already exists.");
			return;
		}

		editor_project_t project = editor_project_t::make_default_project(path.c_str());
		if (!project.save(path.c_str()))
		{
			SFG_ERR("failed saving project {0}", path.c_str());
			show_error("Project file could not be saved.");
			return;
		}

		if (!editor_project_t::get().try_load(path.c_str()))
		{
			SFG_ERR("failed loading project {0}", path.c_str());
			show_error("Created project could not be loaded.");
			return;
		}

		notify_project_ready();
	}
}
