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
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "ui/widgets/editor_widget_entity_info.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>

namespace sfg
{
	namespace
	{
		void fit_control(ui::ui_context& ui, ui::widget_id_t id)
		{
			const editor_theme_t& theme = editor_theme_t::get();

			ui::layout_in_t& in = ui.get_tree().in(id);
			in.size_mode_x		= ui::axis_mode_e::fill;
			in.size_mode_y		= ui::axis_mode_e::fixed;
			in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
			in.anchor_y			= ui::anchor_e::center;
			in.pos_value.y		= 0.5f;
			in.size_value		= {1.0f, theme.item_height};
		}

	}

	void editor_widget_entity_info_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_entity_info_config_t& config)
	{
		_ui							= &ui;
		_world						= &editor_world_controller_t::get().get_editor_world(config.world)->get_world();
		_break_prefab_callback		= config.break_prefab;
		_break_prefab_user_data		= config.user_data;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "entity_info");
		tree.attach(parent, _root);
		ui.set_pre_layout_tick(_root, on_pre_layout_tick, this);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::column;
		root_in.child_margins	 = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		_fields_root = ui.allocate_widget();
		ui.set_widget_debug_name(_fields_root, "entity_info_fields");
		tree.attach(_root, _fields_root);
		ui::layout_in_t& fields_in = tree.in(_fields_root);
		fields_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		fields_in.size_mode_y	   = ui::axis_mode_e::sum_children;
		fields_in.size_value	   = {1.0f, 1.0f};
		fields_in.flow			   = ui::flow_e::column;

		const editor_property_row_t name_row	= editor_misc_widgets_t::make_property_row_with_label(ui, _fields_root, "Name");
		u8*							name_field	= reinterpret_cast<u8*>(_name_fallback);
		editor_input_field_config_t name_config = {};
		name_config.placeholder					= "Name";
		name_config.callbacks.edit_begin		= on_edit_begin;
		name_config.callbacks.edit_submitted	= on_name_input_submitted;
		name_config.callbacks.user_data			= this;
		name_config.field						= {
			.fields		= {.data = &name_field, .size = 1},
			.field_size = sizeof(component_name_t::text),
			.type		= editor_input_field_field_type_e::char_array,
		};
		_name_input.init(ui, name_row.right, name_config);
		fit_control(ui, _name_input.get_root());
		editor_dividers_t::add_divider_hor(ui, _fields_root, theme.divider_thickness * 2.0f, theme.color_frame, theme.color_frame, ui::vg_gradient_e::none);

		const editor_property_row_t guid_row = editor_misc_widgets_t::make_property_row_with_label(ui, _fields_root, "GUID");
		_guid_label							 = ui.allocate_widget();
		ui.set_widget_debug_name(_guid_label, "entity_info_guid_label");
		tree.attach(guid_row.right, _guid_label);

		ui::layout_in_t& guid_label_in = tree.in(_guid_label);
		guid_label_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		guid_label_in.size_mode_y	   = ui::axis_mode_e::fixed;
		guid_label_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		guid_label_in.anchor_y		   = ui::anchor_e::center;
		guid_label_in.pos_value.y	   = 0.5f;
		guid_label_in.size_value	   = {1.0f, theme.item_height};

		ui.set_widget_text(_guid_label, "");
		ui.get_paint().set_text(_guid_label,
								ui.widget_text(_guid_label),
								ui.widget_text_len(_guid_label),
								{.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		editor_dividers_t::add_divider_hor(ui, _fields_root, theme.divider_thickness * 2.0f, theme.color_frame, theme.color_frame, ui::vg_gradient_e::none);

		const editor_property_row_t position_row	= editor_misc_widgets_t::make_property_row_with_label(ui, _fields_root, "Position");
		editor_vec3_field_config_t	position_config = {};
		position_config.callbacks.edit_begin		= on_edit_begin;
		position_config.callbacks.edited			= on_position_changed;
		position_config.callbacks.edit_submitted	= on_edit_submitted;
		position_config.callbacks.user_data			= this;
		_position_field.init(ui, position_row.right, position_config);
		fit_control(ui, _position_field.get_root());
		editor_dividers_t::add_divider_hor(ui, _fields_root, theme.divider_thickness * 2.0f, theme.color_frame, theme.color_frame, ui::vg_gradient_e::none);

		const editor_property_row_t rotation_row	= editor_misc_widgets_t::make_property_row_with_label(ui, _fields_root, "Rotation");
		editor_quat_field_config_t	rotation_config = {};
		rotation_config.callbacks.edit_begin		= on_edit_begin;
		rotation_config.callbacks.edited			= on_rotation_changed;
		rotation_config.callbacks.edit_submitted	= on_edit_submitted;
		rotation_config.callbacks.user_data			= this;
		_rotation_field.init(ui, rotation_row.right, rotation_config);
		fit_control(ui, _rotation_field.get_root());
		editor_dividers_t::add_divider_hor(ui, _fields_root, theme.divider_thickness * 2.0f, theme.color_frame, theme.color_frame, ui::vg_gradient_e::none);

		const editor_property_row_t scale_row	 = editor_misc_widgets_t::make_property_row_with_label(ui, _fields_root, "Scale");
		editor_vec3_field_config_t	scale_config = {};
		scale_config.callbacks.edit_begin		 = on_edit_begin;
		scale_config.callbacks.edited			 = on_scale_changed;
		scale_config.callbacks.edit_submitted	 = on_edit_submitted;
		scale_config.callbacks.user_data		 = this;
		_scale_field.init(ui, scale_row.right, scale_config);
		fit_control(ui, _scale_field.get_root());
		editor_dividers_t::add_divider_hor(ui, _fields_root, theme.divider_thickness * 2.0f, theme.color_frame, theme.color_frame, ui::vg_gradient_e::none);

		if (config.block_edits)
			_blocker = editor_misc_widgets_t::add_edit_blocker(ui, _fields_root);

		if (config.is_prefab)
		{
			_prefab_frame = ui.allocate_widget();
			ui.set_widget_debug_name(_prefab_frame, "entity_info_prefab_frame");
			tree.attach(_root, _prefab_frame);

			ui::layout_in_t& frame_in = tree.in(_prefab_frame);
			frame_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
			frame_in.size_mode_y	  = ui::axis_mode_e::fixed;
			frame_in.size_value		  = {1.0f, theme.item_area_height};
			frame_in.flow			  = ui::flow_e::row;
			frame_in.child_spacing	  = theme.item_spacing;
			frame_in.child_margins	  = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

			ui::vg_rect_paint_t frame_rect = {};
			frame_rect.fill_color_a		   = theme.color_frame;
			frame_rect.fill_color_b		   = theme.color_frame;
			frame_rect.outline_color	   = theme.color_outline_light;
			frame_rect.outline_thickness   = theme.outline_thickness;
			frame_rect.rounding			   = theme.item_rounding;
			frame_rect.rounding_segs	   = 4;
			paint.set_rect(_prefab_frame, frame_rect);

			_prefab_label = ui.allocate_widget();
			ui.set_widget_debug_name(_prefab_label, "entity_info_prefab_label");
			tree.attach(_prefab_frame, _prefab_label);
			tree.draw_order(_prefab_label) = tree.draw_order_const(_prefab_frame) + 1;

			ui::layout_in_t& label_in = tree.in(_prefab_label);
			label_in.size_mode_x	  = ui::axis_mode_e::fill;
			label_in.size_mode_y	  = ui::axis_mode_e::fixed;
			label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
			label_in.anchor_y		  = ui::anchor_e::center;
			label_in.pos_value.y	  = 0.5f;
			label_in.size_value		  = {1.0f, theme.item_height};

			ui.set_widget_text(_prefab_label, config.block_edits ? "Prefab child editing is disabled. Break Prefab to edit it." : "Prefab instance. Break Prefab to detach it.");
			paint.set_text(_prefab_label,
						   ui.widget_text(_prefab_label),
						   ui.widget_text_len(_prefab_label),
						   {.font = theme.font_default, .color = theme.color_accent2, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

			editor_misc_widgets_t::add_spacer(ui, _root, {theme.item_spacing, theme.item_spacing});
			_break_prefab_button.init(ui, _root, {.text = "Break Prefab", .width = {.mode = editor_widget_width_e::fixed, .value = theme.item_width}});
			ui::layout_in_t& button_in = tree.in(_break_prefab_button.get_root());
			button_in.pos_mode_y	   = ui::pos_mode_e::flow;
			button_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
			button_in.anchor_x		   = ui::anchor_e::center;
			button_in.pos_value.x	   = 0.5f;

			ui::listener_bundle_t listener = {};
			listener.user_data			   = this;
			listener.on_click			   = on_break_prefab_clicked;
			ui.get_input().set_listener(_break_prefab_button.get_root(), listener);
		}
	}

	void editor_widget_entity_info_t::uninit()
	{
		if (_break_prefab_button.get_root() != NULL_WIDGET)
			_break_prefab_button.uninit();
		_scale_field.uninit();
		_rotation_field.uninit();
		_position_field.uninit();
		_name_input.uninit();
		_ui->deallocate_widget(_root);

		_ui						  = nullptr;
		_world					  = nullptr;
		_root					  = NULL_WIDGET;
		_guid_label				  = NULL_WIDGET;
		_prefab_frame			  = NULL_WIDGET;
		_prefab_label			  = NULL_WIDGET;
		_fields_root			  = NULL_WIDGET;
		_blocker				  = NULL_WIDGET;
		_entity					  = NULL_ENTITY_ID;
		_name_fallback[0]		  = '\0';
		_name_submitted_callback  = nullptr;
		_name_submitted_user_data = nullptr;
		_break_prefab_callback	  = nullptr;
		_break_prefab_user_data	  = nullptr;
		_callbacks				  = {};
		_entities.resize(0);
	}

	void editor_widget_entity_info_t::set_entity(world_t& world, entity_id_t entity)
	{
		const entity_id_t entities[] = {entity};
		set_entities(world, {.data = entities, .size = 1});
	}

	void editor_widget_entity_info_t::set_entities(world_t& world, span_t<const entity_id_t> entities)
	{
		_world	= &world;
		_entity = entities.data[0];
		_entities.assign(entities.data, entities.data + entities.size);
		refresh_controls();
	}

	void editor_widget_entity_info_t::set_name_submitted_callback(editor_widget_entity_info_name_submitted_fn callback, void* user_data)
	{
		_name_submitted_callback  = callback;
		_name_submitted_user_data = user_data;
	}

	void editor_widget_entity_info_t::set_edit_callbacks(const editor_widget_callbacks_t& callbacks)
	{
		_callbacks = callbacks;
	}

	void editor_widget_entity_info_t::refresh_controls()
	{
		if (_entities.size() > 1)
		{
			_ui->set_widget_text(_guid_label, "Mixed");
		}
		else
		{
			char text[32] = {};
			std::snprintf(text, sizeof(text), "%llu", static_cast<unsigned long long>(_world->get_entity_guid(_entity)));
			_ui->set_widget_text(_guid_label, text);
		}

		const editor_theme_t& theme = editor_theme_t::get();
		_ui->get_paint().set_text(_guid_label,
								  _ui->widget_text(_guid_label),
								  _ui->widget_text_len(_guid_label),
								  {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		frame_vector_t<u8*> name_values = {};
		name_values.reserve(_entities.size());

		ecs_component_table_t* name_table = _world->find_component_table(type_id_t<component_name_t>::value);

		for (entity_id_t entity : _entities)
		{
			component_name_t& name = ecs_helpers_t::table_get_as<component_name_t>(*name_table, entity);
			name_values.push_back(reinterpret_cast<u8*>(name.text));
		}

		_name_input.update_field_data({
			.fields		= {.data = name_values.data(), .size = name_values.size()},
			.field_size = sizeof(component_name_t::text),
			.type		= editor_input_field_field_type_e::char_array,
		});

		refresh_transform_controls(false);
	}

	void editor_widget_entity_info_t::refresh_transform_controls(bool preserve_edits)
	{
		const bool refresh_position = !preserve_edits || !_position_field.is_editing();
		const bool refresh_rotation = !preserve_edits || !_rotation_field.is_editing();
		const bool refresh_scale	= !preserve_edits || !_scale_field.is_editing();

		if (!refresh_position && !refresh_rotation && !refresh_scale)
			return;

		frame_vector_t<vec3f_t*> position_values = {};
		frame_vector_t<quat_t*>	 rotation_values = {};
		frame_vector_t<vec3f_t*> scale_values	 = {};
		position_values.reserve(refresh_position ? _entities.size() : 0);
		rotation_values.reserve(refresh_rotation ? _entities.size() : 0);
		scale_values.reserve(refresh_scale ? _entities.size() : 0);

		ecs_component_table_t* transform_table = _world->find_component_table(type_id_t<component_transform_t>::value);

		for (entity_id_t entity : _entities)
		{
			component_transform_t& transform = ecs_helpers_t::table_get_as<component_transform_t>(*transform_table, entity);

			if (refresh_position)
				position_values.push_back(&transform.pos);

			if (refresh_rotation)
				rotation_values.push_back(&transform.rot);

			if (refresh_scale)
				scale_values.push_back(&transform.scale);
		}

		if (refresh_position)
			_position_field.update_field_data({.fields = {.data = position_values.data(), .size = position_values.size()}});

		if (refresh_rotation)
			_rotation_field.update_field_data({.fields = {.data = rotation_values.data(), .size = rotation_values.size()}});

		if (refresh_scale)
			_scale_field.update_field_data({.fields = {.data = scale_values.data(), .size = scale_values.size()}});
	}

	void editor_widget_entity_info_t::on_edit_begin(void* user_data)
	{
		static_cast<editor_widget_entity_info_t*>(user_data)->begin_edit();
	}

	void editor_widget_entity_info_t::on_name_input_submitted(void* user_data)
	{
		editor_widget_entity_info_t& entity_info = *static_cast<editor_widget_entity_info_t*>(user_data);
		entity_info.submit_names();
		entity_info.submit_edit();
	}

	void editor_widget_entity_info_t::on_position_changed(void* user_data)
	{
		static_cast<editor_widget_entity_info_t*>(user_data)->apply_position_values();
	}

	void editor_widget_entity_info_t::on_rotation_changed(void* user_data)
	{
		static_cast<editor_widget_entity_info_t*>(user_data)->apply_rotation_values();
	}

	void editor_widget_entity_info_t::on_scale_changed(void* user_data)
	{
		static_cast<editor_widget_entity_info_t*>(user_data)->apply_scale_values();
	}

	void editor_widget_entity_info_t::on_edit_submitted(void* user_data)
	{
		static_cast<editor_widget_entity_info_t*>(user_data)->submit_edit();
	}

	void editor_widget_entity_info_t::on_break_prefab_clicked(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_widget_entity_info_t& entity_info = *static_cast<editor_widget_entity_info_t*>(user_data);
		if (entity_info._break_prefab_callback != nullptr)
			entity_info._break_prefab_callback(entity_info._break_prefab_user_data);
	}

	void editor_widget_entity_info_t::on_pre_layout_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data)
	{
		static_cast<editor_widget_entity_info_t*>(user_data)->refresh_transform_controls(true);
	}

	void editor_widget_entity_info_t::begin_edit()
	{
		if (_callbacks.edit_begin != nullptr)
			_callbacks.edit_begin(_callbacks.user_data);
	}

	void editor_widget_entity_info_t::submit_edit()
	{
		if (_callbacks.edit_submitted != nullptr)
			_callbacks.edit_submitted(_callbacks.user_data);
	}

	void editor_widget_entity_info_t::apply_position_values()
	{
		if (_entities.empty())
			return;

		ecs_component_table_t* transform_table = _world->find_component_table(type_id_t<component_transform_t>::value);
		for (entity_id_t entity : _entities)
		{
			const component_transform_t& transform = ecs_helpers_t::table_get_as<component_transform_t>(*transform_table, entity);
			_world->set_entity_pos_local(entity, transform.pos);
		}
	}

	void editor_widget_entity_info_t::apply_rotation_values()
	{
		if (_entities.empty())
			return;

		ecs_component_table_t* transform_table = _world->find_component_table(type_id_t<component_transform_t>::value);
		for (entity_id_t entity : _entities)
		{
			const component_transform_t& transform = ecs_helpers_t::table_get_as<component_transform_t>(*transform_table, entity);
			_world->set_entity_rot_local(entity, transform.rot);
		}
	}

	void editor_widget_entity_info_t::apply_scale_values()
	{
		if (_entities.empty())
			return;

		ecs_component_table_t* transform_table = _world->find_component_table(type_id_t<component_transform_t>::value);
		for (entity_id_t entity : _entities)
		{
			const component_transform_t& transform = ecs_helpers_t::table_get_as<component_transform_t>(*transform_table, entity);
			_world->set_entity_scale_local(entity, transform.scale);
		}
	}

	void editor_widget_entity_info_t::submit_names()
	{
		if (_name_submitted_callback == nullptr)
			return;

		for (entity_id_t entity : _entities)
			_name_submitted_callback(entity, _name_submitted_user_data);
	}
}
