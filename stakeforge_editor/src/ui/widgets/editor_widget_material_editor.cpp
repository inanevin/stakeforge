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

#include "ui/widgets/editor_widget_material_editor.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"

#include <sfg/common/type_id.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void editor_widget_material_editor_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui = &ui;

		ui::layout_tree_t& tree = ui.get_tree();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "material_editor");
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

		_reflection.init(ui,
						 _root,
						 {
							 .fold_states = &_fold_states,
							 .callbacks =
								 {
									 .edit_begin	 = on_material_edit_begin,
									 .edited		 = on_material_edited,
									 .edit_submitted = on_material_edit_submitted,
									 .user_data		 = this,
								 },
							 .objects = {.data = _objects.data(), .size = _objects.size()},
							 .type_id = type_id_t<material_def_t>::value,
						 });
	}

	void editor_widget_material_editor_t::uninit()
	{
		_reflection.uninit();
		_ui->deallocate_widget(_root);

		_fold_states.resize(0);
		_materials.resize(0);
		_material_ids.resize(0);
		_objects.resize(0);
		_ui	  = nullptr;
		_root = NULL_WIDGET;
	}

	void editor_widget_material_editor_t::set_materials(span_t<const sid_t> materials)
	{
		_materials.resize(0);
		_material_ids.resize(0);
		_objects.resize(0);
		_materials.reserve(materials.size);
		_material_ids.reserve(materials.size);

		const editor_asset_manager_t& assets = editor_asset_manager_t::get();
		for (size_t i = 0; i < materials.size; ++i)
		{
			const sid_t			  material_id = materials.data[i];
			const editor_asset_t* asset		  = assets.find_asset(material_id);
			if (asset == nullptr || asset->asset_type != editor_asset_type_e::material)
				continue;

			material_def_t		 material		 = {};
			const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(*asset);
			if (!reflection_registry_t::get().type_from_json(type_id_t<material_def_t>::value, &material, nullptr, embedded_source))
			{
				SFG_ERR("failed to deserialize material definition for asset {0}", material_id);
				continue;
			}

			_material_ids.push_back(material_id);
			_materials.push_back(material);
		}

		_objects.reserve(_materials.size());
		for (material_def_t& material : _materials)
			_objects.push_back(&material);

		refresh_reflection();
	}

	void editor_widget_material_editor_t::refresh_reflection()
	{
		_reflection.set_reflection({
			.fold_states = &_fold_states,
			.callbacks =
				{
					.edit_begin		= on_material_edit_begin,
					.edited			= on_material_edited,
					.edit_submitted = on_material_edit_submitted,
					.user_data		= this,
				},
			.objects = {.data = _objects.data(), .size = _objects.size()},
			.type_id = type_id_t<material_def_t>::value,
		});
	}

	void editor_widget_material_editor_t::on_material_edit_begin()
	{
	}

	void editor_widget_material_editor_t::on_material_edited()
	{
	}

	void editor_widget_material_editor_t::on_material_edit_submitted()
	{
	}

	void editor_widget_material_editor_t::on_material_edit_begin(void* user_data)
	{
		static_cast<editor_widget_material_editor_t*>(user_data)->on_material_edit_begin();
	}

	void editor_widget_material_editor_t::on_material_edited(void* user_data)
	{
		static_cast<editor_widget_material_editor_t*>(user_data)->on_material_edited();
	}

	void editor_widget_material_editor_t::on_material_edit_submitted(void* user_data)
	{
		static_cast<editor_widget_material_editor_t*>(user_data)->on_material_edit_submitted();
	}
}
