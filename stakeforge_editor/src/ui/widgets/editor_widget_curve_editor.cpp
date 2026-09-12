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

#include "ui/widgets/editor_widget_curve_editor.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"
#include "commands/editor_commands_curve.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/data/frame_vector.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void editor_widget_curve_editor_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui = &ui;

		ui::layout_tree_t& tree = ui.get_tree();
		_root					= ui.allocate_widget();
		ui.set_widget_debug_name(_root, "curve_editor");
		tree.attach(parent, _root);

		editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& input = tree.in(_root);
		input.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		input.pos_mode_y	   = ui::pos_mode_e::flow;
		input.pos_value		   = {0.0f, 0.0f};
		input.size_mode_x	   = ui::axis_mode_e::parent_relative;
		input.size_mode_y	   = ui::axis_mode_e::sum_children;
		input.size_value	   = {1.0f, 1.0f};
		input.flow			   = ui::flow_e::column;
		input.child_spacing	   = 0.0f;
		input.child_margins	   = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};
	}

	void editor_widget_curve_editor_t::uninit()
	{
		clear_display();
		_ui->deallocate_widget(_root);
		_curves.resize(0);
		_curve_ids.resize(0);
		_edit_previous_curves.resize(0);
		_edit_curve_ids.resize(0);
		_field_states.resize(0);
		_ui			 = nullptr;
		_root		 = NULL_WIDGET;
		_edit_active = false;
	}

	void editor_widget_curve_editor_t::set_curves(span_t<const sid_t> curves)
	{
		clear_curve_edit();
		clear_display();
		_curves.resize(0);
		_curve_ids.resize(0);
		_curves.reserve(curves.size);
		_curve_ids.reserve(curves.size);

		const editor_asset_manager_t& assets = editor_asset_manager_t::get();

		for (size_t index = 0; index < curves.size; ++index)
		{
			const sid_t			  curve_id = curves.data[index];
			const editor_asset_t* asset	   = assets.find_asset(curve_id);

			if (asset == nullptr || asset->asset_type != editor_asset_type_e::curve)
				continue;

			curve_def_t			 definition = {};
			const nlohmann::json embedded	= editor_asset_io_t::get_embedded_source_json(*asset);

			if (!reflection_registry_t::get().type_from_json(type_id_t<curve_def_t>::value, &definition, nullptr, embedded))
				continue;

			std::sort(definition.keys.begin(), definition.keys.end(), [](const curve_key_t& left, const curve_key_t& right) { return left.time < right.time; });
			_curve_ids.push_back(curve_id);
			_curves.push_back(std::move(definition));
		}

		refresh_display();
	}

	void editor_widget_curve_editor_t::refresh_display()
	{
		if (_curves.empty())
			return;

		_labels.push_back(editor_misc_widgets_t::make_section_label(*_ui, _root, "Curve"));

		const editor_widget_callbacks_t callbacks{
			.edit_begin		= on_curve_edit_begin,
			.edited			= on_curve_edited,
			.edit_submitted = on_curve_edit_submitted,
			.user_data		= this,
		};
		frame_vector_t<void*> objects = {};
		objects.reserve(_curves.size());

		for (curve_def_t& curve : _curves)
			objects.push_back(&curve);

		_reflection.init(*_ui,
						 _root,
						 {
							 .fold_states = &_field_states,
							 .callbacks	  = callbacks,
							 .objects	  = {.data = objects.data(), .size = objects.size()},
							 .type_id	  = type_id_t<curve_def_t>::value,
						 });
		_reflection_initialized = true;
		_curve_edit.init(*_ui, _root, {.data = _curves.data(), .size = _curves.size()}, callbacks);
		_curve_edit_initialized = true;
	}

	void editor_widget_curve_editor_t::clear_display()
	{
		if (_curve_edit_initialized)
		{
			_curve_edit.uninit();
			_curve_edit_initialized = false;
		}

		if (_reflection_initialized)
		{
			_reflection.uninit();
			_reflection_initialized = false;
		}

		for (ui::widget_id_t label : _labels)
			_ui->deallocate_widget(label);

		_labels.resize(0);
	}

	void editor_widget_curve_editor_t::begin_curve_edit()
	{
		clear_curve_edit();
		_edit_previous_curves.assign(_curves.begin(), _curves.end());
		_edit_curve_ids.assign(_curve_ids.begin(), _curve_ids.end());
		_edit_active = true;
	}

	void editor_widget_curve_editor_t::submit_curve_edit()
	{
		if (!_edit_active)
			return;

		editor_command_curve_edit_t::edit({.data = _edit_curve_ids.data(), .size = _edit_curve_ids.size()}, {.data = _edit_previous_curves.data(), .size = _edit_previous_curves.size()}, {.data = _curves.data(), .size = _curves.size()});
		clear_curve_edit();

		vector_t<sid_t> curve_ids = {};
		curve_ids.assign(_curve_ids.begin(), _curve_ids.end());
		set_curves({.data = curve_ids.data(), .size = curve_ids.size()});
	}

	void editor_widget_curve_editor_t::clear_curve_edit()
	{
		_edit_previous_curves.resize(0);
		_edit_curve_ids.resize(0);
		_edit_active = false;
	}

	void editor_widget_curve_editor_t::on_curve_edit_begin(void* user_data)
	{
		static_cast<editor_widget_curve_editor_t*>(user_data)->begin_curve_edit();
	}

	void editor_widget_curve_editor_t::on_curve_edited(void* user_data)
	{
	}

	void editor_widget_curve_editor_t::on_curve_edit_submitted(void* user_data)
	{
		static_cast<editor_widget_curve_editor_t*>(user_data)->submit_curve_edit();
	}

}
