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

#include "editor_widget_shader_editor.hpp"
#include "editor/assets/editor_asset_io.hpp"
#include "editor/assets/editor_asset_manager.hpp"
#include "editor/commands/editor_commands_shader.hpp"

#include <sfg/core/common/hashing.hpp>
#include <sfg/core/data/frame_vector.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/reflection/reflection_registry.hpp>
#include <sfg/core/runtime/ui/ui_context.hpp>
#include <sfg/core/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void editor_widget_shader_editor_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui	  = &ui;
		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "shader_editor");

		ui::layout_tree_t& tree = ui.get_tree();
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		root_in.pos_mode_y		 = ui::pos_mode_e::flow;
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::column;

		_info.init(ui, _root, {});
		_name.init(ui, _root, {});
	}

	void editor_widget_shader_editor_t::uninit()
	{
		_name.uninit();
		_info.uninit();
		_ui->deallocate_widget(_root);
		_configs.resize(0);
		_previous.resize(0);
		_shader_ids.resize(0);
		_ui	  = nullptr;
		_root = NULL_WIDGET;
	}

	void editor_widget_shader_editor_t::set_shaders(span_t<const sid_t> shaders)
	{
		_info.set_reflection({});
		_name.set_reflection({});
		_previous.resize(0);
		_configs.resize(0);
		_shader_ids.resize(0);
		_configs.reserve(shaders.size);
		_shader_ids.reserve(shaders.size);

		bool all_custom = true;

		for (const sid_t shader : shaders)
		{
			const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(shader);
			SFG_ASSERT(asset->asset_type == editor_asset_type_e::shader);

			shader_cook_config_t config	 = {};
			nlohmann::json		 options = {};
			const bool			 parsed	 = editor_asset_io_t::parse_cook_options_json(*asset, options);

			if (!parsed)
				continue;

			try
			{
				const bool converted = reflection_registry_t::get().type_from_json(type_id_t<shader_cook_config_t>::value, &config, nullptr, options);

				if (!converted)
				{
					SFG_ERR("failed to read shader cook options for asset {0}", shader);
					continue;
				}
			}
			catch (const nlohmann::json::exception& exception)
			{
				SFG_ERR("invalid shader cook options for asset {0}: {1}", shader, exception.what());
				continue;
			}

			if (config.type != shader_type_e::custom_shader)
				all_custom = false;

			_configs.push_back(std::move(config));
			_shader_ids.push_back(shader);
		}

		frame_vector_t<void*> objects = {};
		objects.reserve(_configs.size());

		for (shader_cook_config_t& config : _configs)
			objects.push_back(&config);

		const sid_t			info_excluded[] = {"custom_type_name"_hs};
		const sid_t			name_excluded[] = {"type"_hs, "include_dirs"_hs};
		const span_t<void*> object_span{.data = objects.data(), .size = objects.size()};

		_info.set_reflection({
			.excluded_fields = {.data = info_excluded, .size = std::size(info_excluded)},
			.objects		 = object_span,
			.type_id		 = type_id_t<shader_cook_config_t>::value,
			.block_edits	 = true,
		});
		_name.set_reflection({
			.callbacks		 = {.edit_begin = on_edit_begin, .edit_submitted = on_edit_submitted, .user_data = this},
			.excluded_fields = {.data = name_excluded, .size = std::size(name_excluded)},
			.objects		 = object_span,
			.type_id		 = type_id_t<shader_cook_config_t>::value,
			.block_edits	 = !all_custom,
		});
	}

	void editor_widget_shader_editor_t::on_edit_begin(void* user_data)
	{
		editor_widget_shader_editor_t& editor = *static_cast<editor_widget_shader_editor_t*>(user_data);
		editor._previous					  = editor._configs;
	}

	void editor_widget_shader_editor_t::on_edit_submitted(void* user_data)
	{
		editor_widget_shader_editor_t& editor = *static_cast<editor_widget_shader_editor_t*>(user_data);
		const bool					   saved  = editor_command_shader_options_edit_t::edit(
			{.data = editor._shader_ids.data(), .size = editor._shader_ids.size()}, {.data = editor._previous.data(), .size = editor._previous.size()}, {.data = editor._configs.data(), .size = editor._configs.size()});

		if (!saved)
			SFG_ERR("failed to save shader type names");

		editor._previous.resize(0);
	}
}
