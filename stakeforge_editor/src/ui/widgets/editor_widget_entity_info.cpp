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
#include "editor_app.hpp"
#include "ui/panels/editor_panel_entities.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include <sfg/math/quat.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	namespace
	{
		void fit_control(ui::ui_context& ui, ui::widget_id_t id)
		{
			const editor_theme_t& theme = editor_theme_t::get();

			ui::layout_in_t& in = ui.get_tree().in(id);
			in.size_mode_x		= ui::axis_mode_e::parent_relative;
			in.size_mode_y		= ui::axis_mode_e::fixed;
			in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
			in.anchor_y			= ui::anchor_e::center;
			in.pos_value.y		= 0.5f;
			in.size_value		= {1.0f, theme.item_height};
		}
	}

	void editor_widget_entity_info_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "entity_info");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible;
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::column;
		root_in.child_margins	 = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		const editor_property_row_t name_row	= editor_misc_widgets_t::make_property_row_with_label(ui, _root, "Name");
		editor_input_field_config_t name_config = {};
		name_config.type						= editor_input_field_type_e::text;
		name_config.on_submitted				= on_name_submitted;
		name_config.user_data					= this;
		_name_input.init(ui, name_row.right, name_config);
		fit_control(ui, _name_input.get_root());

		const editor_property_row_t position_row	= editor_misc_widgets_t::make_property_row_with_label(ui, _root, "Position");
		editor_vec3_field_config_t	position_config = {};
		position_config.on_changed					= on_position_changed;
		position_config.user_data					= this;
		_position_field.init(ui, position_row.right, position_config);
		fit_control(ui, _position_field.get_root());

		const editor_property_row_t rotation_row	= editor_misc_widgets_t::make_property_row_with_label(ui, _root, "Rotation");
		editor_vec3_field_config_t	rotation_config = {};
		rotation_config.on_changed					= on_rotation_changed;
		rotation_config.user_data					= this;
		_rotation_field.init(ui, rotation_row.right, rotation_config);
		fit_control(ui, _rotation_field.get_root());

		const editor_property_row_t scale_row	 = editor_misc_widgets_t::make_property_row_with_label(ui, _root, "Scale");
		editor_vec3_field_config_t	scale_config = {};
		scale_config.value						 = vec3f_t::one;
		scale_config.on_changed					 = on_scale_changed;
		scale_config.user_data					 = this;
		_scale_field.init(ui, scale_row.right, scale_config);
		fit_control(ui, _scale_field.get_root());
	}

	void editor_widget_entity_info_t::uninit()
	{
		_scale_field.uninit();
		_rotation_field.uninit();
		_position_field.uninit();
		_name_input.uninit();
		_ui->deallocate_widget(_root);

		_ui			= nullptr;
		_world		= nullptr;
		_root		= NULL_WIDGET;
		_entity		= NULL_ENTITY_ID;
		_refreshing = false;
	}

	void editor_widget_entity_info_t::set_entity(world_t& world, entity_id_t entity)
	{
		_world		= &world;
		_entity		= entity;
		_refreshing = true;

		const char* name = world.get_entity_name(entity);
		_name_input.set_text(name != nullptr ? name : "");
		_position_field.set_value(world.get_entity_pos_local(entity));
		_rotation_field.set_value(quat_t::to_euler(world.get_entity_rot_local(entity)));
		_scale_field.set_value(world.get_entity_scale_local(entity));

		_refreshing = false;
	}

	void editor_widget_entity_info_t::on_name_submitted(const char* value, f32, void* user_data)
	{
		editor_widget_entity_info_t& widget = *static_cast<editor_widget_entity_info_t*>(user_data);
		if (widget._refreshing)
			return;

		widget._world->set_entity_name(widget._entity, value);

		editor_panel_t* panel = editor_app_t::get().find_panel(editor_panel_type_e::entities);
		if (panel != nullptr)
			static_cast<editor_panel_entities_t*>(panel)->refresh_entity_name(widget._entity);
	}

	void editor_widget_entity_info_t::on_position_changed(const vec3f_t& value, void* user_data)
	{
		editor_widget_entity_info_t& widget = *static_cast<editor_widget_entity_info_t*>(user_data);
		if (widget._refreshing)
			return;

		widget._world->set_entity_pos_local(widget._entity, value);
	}

	void editor_widget_entity_info_t::on_rotation_changed(const vec3f_t& value, void* user_data)
	{
		editor_widget_entity_info_t& widget = *static_cast<editor_widget_entity_info_t*>(user_data);
		if (widget._refreshing)
			return;

		widget._world->set_entity_rot_local(widget._entity, quat_t::from_euler(value.x, value.y, value.z));
	}

	void editor_widget_entity_info_t::on_scale_changed(const vec3f_t& value, void* user_data)
	{
		editor_widget_entity_info_t& widget = *static_cast<editor_widget_entity_info_t*>(user_data);
		if (widget._refreshing)
			return;

		widget._world->set_entity_scale_local(widget._entity, value);
	}
}
