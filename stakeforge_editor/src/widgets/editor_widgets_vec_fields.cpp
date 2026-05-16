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
#include "widgets/editor_widgets_vec_fields.hpp"
#include "panels/editor_theme.hpp"
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		void make_root_row(ui::ui_context& ui, ui::widget_id_t id, const editor_widget_width_config_t& width)
		{
			const editor_theme_t& theme = editor_theme_t::get();
			ui::layout_in_t&	  in	= ui.get_tree().in(id);
			in.flags					= ui::wf_visible;
			apply_editor_widget_width(in, width);
			in.size_mode_y	 = ui::axis_mode_e::fixed;
			in.size_value.y	 = theme.item_height;
			in.flow			 = ui::flow_e::row;
			in.child_spacing = theme.item_spacing;
		}

		void set_input_fill(ui::ui_context& ui, ui::widget_id_t id)
		{
			ui::layout_in_t& in = ui.get_tree().in(id);
			in.size_mode_x		= ui::axis_mode_e::fill;
			in.size_mode_y		= ui::axis_mode_e::parent_relative;
			in.size_value		= {1.0f, 1.0f};
		}

		void init_number_input(ui::ui_context& ui, ui::widget_id_t parent, editor_input_field_t& input, const char* placeholder, f32 value, f32 increment, bool integer, editor_input_field_number_fn callback, void* user_data)
		{
			editor_input_field_config_t input_config = {};
			input_config.placeholder				 = placeholder;
			input_config.type						 = editor_input_field_type_e::number;
			input_config.number_value				 = value;
			input_config.increment					 = increment;
			input_config.integer					 = integer;
			input_config.on_number_changed			 = callback;
			input_config.user_data					 = user_data;
			input.init(ui, parent, input_config);
			set_input_fill(ui, input.get_root());
		}
	}

	void editor_vec2_field_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_vec2_field_config_t& config)
	{
		_ui		= &ui;
		_config = config;
		_value	= config.value;

		ui::layout_tree_t& tree = ui.get_tree();
		_root					= ui.allocate_widget();
		ui.set_widget_debug_name(_root, "vec2_field");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;
		make_root_row(ui, _root, config.width);

		const char* names[2] = {"X", "Y"};
		for (u8 i = 0; i < 2; ++i)
		{
			_components[i] = {this, i};
			init_number_input(ui, _root, _inputs[i], names[i], (&_value.x)[i], config.increment, config.integer, on_component_changed, &_components[i]);
		}
	}

	void editor_vec2_field_t::uninit()
	{
		for (u8 i = 0; i < 2; ++i)
			_inputs[1 - i].uninit();
		_ui->deallocate_widget(_root);

		_ui		= nullptr;
		_root	= NULL_WIDGET;
		_config = {};
		_value	= {0.0f, 0.0f};
		for (component_t& component : _components)
			component = {};
	}

	void editor_vec2_field_t::set_value(const vec2f_t& value)
	{
		_value = value;
		for (u8 i = 0; i < 2; ++i)
			_inputs[i].set_number((&_value.x)[i]);
	}

	void editor_vec2_field_t::on_component_changed(f32 value, void* user_data)
	{
		component_t& component						  = *static_cast<component_t*>(user_data);
		(&component.owner->_value.x)[component.index] = value;
		if (component.owner->_config.on_changed != nullptr)
			component.owner->_config.on_changed(component.owner->_value, component.owner->_config.user_data);
	}

	void editor_vec3_field_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_vec3_field_config_t& config)
	{
		_ui		= &ui;
		_config = config;
		_value	= config.value;

		ui::layout_tree_t& tree = ui.get_tree();
		_root					= ui.allocate_widget();
		ui.set_widget_debug_name(_root, "vec3_field");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;
		make_root_row(ui, _root, config.width);

		const char* names[3] = {"X", "Y", "Z"};
		for (u8 i = 0; i < 3; ++i)
		{
			_components[i] = {this, i};
			init_number_input(ui, _root, _inputs[i], names[i], (&_value.x)[i], config.increment, config.integer, on_component_changed, &_components[i]);
		}
	}

	void editor_vec3_field_t::uninit()
	{
		for (u8 i = 0; i < 3; ++i)
			_inputs[2 - i].uninit();
		_ui->deallocate_widget(_root);

		_ui		= nullptr;
		_root	= NULL_WIDGET;
		_config = {};
		_value	= {0.0f, 0.0f, 0.0f};
		for (component_t& component : _components)
			component = {};
	}

	void editor_vec3_field_t::set_value(const vec3f_t& value)
	{
		_value = value;
		for (u8 i = 0; i < 3; ++i)
			_inputs[i].set_number((&_value.x)[i]);
	}

	void editor_vec3_field_t::on_component_changed(f32 value, void* user_data)
	{
		component_t& component						  = *static_cast<component_t*>(user_data);
		(&component.owner->_value.x)[component.index] = value;
		if (component.owner->_config.on_changed != nullptr)
			component.owner->_config.on_changed(component.owner->_value, component.owner->_config.user_data);
	}

	void editor_vec4_field_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_vec4_field_config_t& config)
	{
		_ui		= &ui;
		_config = config;
		_value	= config.value;

		ui::layout_tree_t& tree = ui.get_tree();
		_root					= ui.allocate_widget();
		ui.set_widget_debug_name(_root, "vec4_field");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;
		make_root_row(ui, _root, config.width);

		const char* names[4] = {"X", "Y", "Z", "W"};
		for (u8 i = 0; i < 4; ++i)
		{
			_components[i] = {this, i};
			init_number_input(ui, _root, _inputs[i], names[i], (&_value.x)[i], config.increment, config.integer, on_component_changed, &_components[i]);
		}
	}

	void editor_vec4_field_t::uninit()
	{
		for (u8 i = 0; i < 4; ++i)
			_inputs[3 - i].uninit();
		_ui->deallocate_widget(_root);

		_ui		= nullptr;
		_root	= NULL_WIDGET;
		_config = {};
		_value	= {0.0f, 0.0f, 0.0f, 0.0f};
		for (component_t& component : _components)
			component = {};
	}

	void editor_vec4_field_t::set_value(const vec4f_t& value)
	{
		_value = value;
		for (u8 i = 0; i < 4; ++i)
			_inputs[i].set_number((&_value.x)[i]);
	}

	void editor_vec4_field_t::on_component_changed(f32 value, void* user_data)
	{
		component_t& component						  = *static_cast<component_t*>(user_data);
		(&component.owner->_value.x)[component.index] = value;
		if (component.owner->_config.on_changed != nullptr)
			component.owner->_config.on_changed(component.owner->_value, component.owner->_config.user_data);
	}
}
