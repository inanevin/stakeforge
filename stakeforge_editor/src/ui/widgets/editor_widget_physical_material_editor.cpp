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

#include "ui/widgets/editor_widget_physical_material_editor.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"
#include "commands/editor_commands_physical_material.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include <sfg/common/type_id.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void editor_widget_physical_material_editor_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui = &ui;

		ui::layout_tree_t& tree = ui.get_tree();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "physical_material_editor");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		root_in.pos_mode_y		 = ui::pos_mode_e::flow;
		root_in.pos_value		 = {0.0f, 0.0f};
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;
	}

	void editor_widget_physical_material_editor_t::uninit()
	{
		_ui->cancel_mutations(this);
		clear_display();
		_ui->deallocate_widget(_root);

		_pending_physical_material_ids.resize(0);
		_physical_materials.resize(0);
		_physical_material_ids.resize(0);
		_edit_previous_physical_materials.resize(0);
		_edit_physical_material_ids.resize(0);
		_field_states.resize(0);
		_ui									= nullptr;
		_root								= NULL_WIDGET;
		_reflection_initialized				= false;
		_edit_active						= false;
		_refresh_physical_materials_pending = false;
	}

	void editor_widget_physical_material_editor_t::set_physical_materials(span_t<const sid_t> physical_materials)
	{
		if (!can_mutate_ui_topology())
		{
			request_physical_materials_refresh(physical_materials);
			return;
		}

		clear_physical_material_edit();
		clear_display();
		_physical_materials.resize(0);
		_physical_material_ids.resize(0);
		_physical_materials.reserve(physical_materials.size);
		_physical_material_ids.reserve(physical_materials.size);

		const editor_asset_manager_t& assets = editor_asset_manager_t::get();
		for (size_t i = 0; i < physical_materials.size; ++i)
		{
			const sid_t			  physical_material_id = physical_materials.data[i];
			const editor_asset_t* asset				   = assets.find_asset(physical_material_id);
			if (asset == nullptr || asset->asset_type != editor_asset_type_e::physical_material)
				continue;

			physical_material_def_t material		= {};
			const nlohmann::json	embedded_source = editor_asset_io_t::get_embedded_source_json(*asset);
			if (!reflection_registry_t::get().type_from_json(type_id_t<physical_material_def_t>::value, &material, nullptr, embedded_source))
				continue;

			_physical_material_ids.push_back(physical_material_id);
			_physical_materials.push_back(material);
		}

		refresh_display();
	}

	void editor_widget_physical_material_editor_t::refresh_display()
	{
		if (_physical_materials.empty())
			return;

		_labels.push_back(editor_misc_widgets_t::make_section_label(*_ui, _root, "Physical Material"));

		editor_widget_callbacks_t callbacks = {};
		callbacks.edit_begin				= on_physical_material_edit_begin;
		callbacks.edited					= on_physical_material_edited;
		callbacks.edit_submitted			= on_physical_material_edit_submitted;
		callbacks.user_data					= this;

		vector_t<void*> objects;
		objects.reserve(_physical_materials.size());
		for (physical_material_def_t& material : _physical_materials)
			objects.push_back(&material);

		_reflection.init(*_ui,
						 _root,
						 {
							 .fold_states = &_field_states,
							 .callbacks	  = callbacks,
							 .objects	  = {.data = objects.data(), .size = objects.size()},
							 .type_id	  = type_id_t<physical_material_def_t>::value,
						 });
		_reflection_initialized = true;
	}

	void editor_widget_physical_material_editor_t::clear_display()
	{
		if (_reflection_initialized)
		{
			_reflection.uninit();
			_reflection_initialized = false;
		}

		for (ui::widget_id_t label : _labels)
			_ui->deallocate_widget(label);
		for (ui::widget_id_t row : _rows)
			_ui->deallocate_widget(row);
		for (ui::widget_id_t divider : _dividers)
			_ui->deallocate_widget(divider);

		_rows.resize(0);
		_dividers.resize(0);
		_labels.resize(0);
	}

	void editor_widget_physical_material_editor_t::append_property_row(ui::widget_id_t row)
	{
		_rows.push_back(row);
		_dividers.push_back(editor_dividers_t::add_divider_hor(*_ui, _root, editor_theme_t::get().divider_thickness * 2.0f, editor_theme_t::get().color_frame, editor_theme_t::get().color_frame, ui::vg_gradient_e::none));
	}

	bool editor_widget_physical_material_editor_t::can_mutate_ui_topology() const
	{
		const ui::ui_phase_e phase = _ui->get_phase();
		return phase == ui::ui_phase_e::idle || phase == ui::ui_phase_e::mutation || phase == ui::ui_phase_e::pre_layout;
	}

	void editor_widget_physical_material_editor_t::begin_physical_material_edit()
	{
		clear_physical_material_edit();
		_edit_previous_physical_materials.assign(_physical_materials.begin(), _physical_materials.end());
		_edit_physical_material_ids.assign(_physical_material_ids.begin(), _physical_material_ids.end());
		_edit_active = true;
	}

	void editor_widget_physical_material_editor_t::submit_physical_material_edit()
	{
		if (!_edit_active)
			return;

		editor_command_physical_material_edit_t::edit({.data = _edit_physical_material_ids.data(), .size = _edit_physical_material_ids.size()},
													  {.data = _edit_previous_physical_materials.data(), .size = _edit_previous_physical_materials.size()},
													  {.data = _physical_materials.data(), .size = _physical_materials.size()});
		clear_physical_material_edit();
		request_display_refresh();
	}

	void editor_widget_physical_material_editor_t::clear_physical_material_edit()
	{
		_edit_previous_physical_materials.resize(0);
		_edit_physical_material_ids.resize(0);
		_edit_active = false;
	}

	void editor_widget_physical_material_editor_t::request_physical_materials_refresh(span_t<const sid_t> physical_materials)
	{
		_pending_physical_material_ids.resize(0);
		_pending_physical_material_ids.reserve(physical_materials.size);
		for (size_t i = 0; i < physical_materials.size; ++i)
			_pending_physical_material_ids.push_back(physical_materials.data[i]);
		_refresh_physical_materials_pending = true;
		_ui->request_unique_mutation(on_ui_mutation, this);
	}

	void editor_widget_physical_material_editor_t::request_display_refresh()
	{
		_pending_physical_material_ids.assign(_physical_material_ids.begin(), _physical_material_ids.end());
		_refresh_physical_materials_pending = true;
		_ui->request_unique_mutation(on_ui_mutation, this);
	}

	void editor_widget_physical_material_editor_t::flush_pending_ui_mutations()
	{
		if (!_refresh_physical_materials_pending)
			return;

		_refresh_physical_materials_pending = false;
		vector_t<sid_t> physical_materials;
		physical_materials.assign(_pending_physical_material_ids.begin(), _pending_physical_material_ids.end());
		_pending_physical_material_ids.resize(0);
		set_physical_materials({.data = physical_materials.data(), .size = physical_materials.size()});
	}

	void editor_widget_physical_material_editor_t::on_physical_material_edit_begin()
	{
		begin_physical_material_edit();
	}

	void editor_widget_physical_material_editor_t::on_physical_material_edited()
	{
	}

	void editor_widget_physical_material_editor_t::on_physical_material_edit_submitted()
	{
		submit_physical_material_edit();
	}

	void editor_widget_physical_material_editor_t::on_physical_material_edit_begin(void* user_data)
	{
		static_cast<editor_widget_physical_material_editor_t*>(user_data)->on_physical_material_edit_begin();
	}

	void editor_widget_physical_material_editor_t::on_physical_material_edited(void* user_data)
	{
		static_cast<editor_widget_physical_material_editor_t*>(user_data)->on_physical_material_edited();
	}

	void editor_widget_physical_material_editor_t::on_physical_material_edit_submitted(void* user_data)
	{
		static_cast<editor_widget_physical_material_editor_t*>(user_data)->on_physical_material_edit_submitted();
	}

	void editor_widget_physical_material_editor_t::on_ui_mutation(ui::ui_context&, void* user_data)
	{
		static_cast<editor_widget_physical_material_editor_t*>(user_data)->flush_pending_ui_mutations();
	}
}
