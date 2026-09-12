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

#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/resources/material_def.hpp>
#include <sfg/runtime/resources/shader_data_definition.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg
{
	namespace ui
	{
		class ui_context;
	}

	class editor_checkbox_t;
	class editor_color_field_t;
	class editor_dropdown_t;
	class editor_input_field_t;
	class editor_vec2_field_t;
	class editor_vec4_field_t;
	class editor_widget_reference_t;

	class editor_widget_material_editor_t final
	{
	public:
		editor_widget_material_editor_t()												   = default;
		~editor_widget_material_editor_t()												   = default;
		editor_widget_material_editor_t(const editor_widget_material_editor_t&)			   = delete;
		editor_widget_material_editor_t& operator=(const editor_widget_material_editor_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent);
		void uninit();
		void set_materials(span_t<const sid_t> materials);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		void refresh_display();
		void refresh_display_common();
		void refresh_display_data();
		void clear_display();
		void fit_control(ui::widget_id_t widget);
		void append_property_row(ui::widget_id_t row);
		bool load_shared_shader_definition();
		void normalize_materials_to_shader_definition();
		void begin_material_edit();
		void submit_material_edit();
		void clear_material_edit();
		void begin_shader_edit();
		void submit_shader_edit();
		void clear_shader_edit();
		void on_material_edit_begin();
		void on_material_edited();
		void on_material_edit_submitted();
		void on_shader_edit_begin();
		void on_shader_edited();
		void on_shader_edit_submitted();

		static void on_material_edit_begin(void* user_data);
		static void on_material_edited(void* user_data);
		static void on_material_edit_submitted(void* user_data);
		static void on_shader_edit_begin(void* user_data);
		static void on_shader_edited(void* user_data);
		static void on_shader_edit_submitted(void* user_data);

	private:
		ui::ui_context* _ui	  = nullptr;
		ui::widget_id_t _root = NULL_WIDGET;

		shader_data_definition_t			 _shader_definition				 = {};
		vector_t<editor_widget_reference_t*> _references					 = {};
		vector_t<editor_checkbox_t*>		 _checkboxes					 = {};
		vector_t<editor_color_field_t*>		 _color_fields					 = {};
		vector_t<editor_dropdown_t*>		 _dropdowns						 = {};
		vector_t<editor_input_field_t*>		 _inputs						 = {};
		vector_t<editor_vec2_field_t*>		 _vec2_fields					 = {};
		vector_t<editor_vec4_field_t*>		 _vec4_fields					 = {};
		vector_t<ui::widget_id_t>			 _rows							 = {};
		vector_t<ui::widget_id_t>			 _dividers						 = {};
		vector_t<ui::widget_id_t>			 _labels						 = {};
		vector_t<material_def_t>			 _materials						 = {};
		vector_t<sid_t>						 _material_ids					 = {};
		vector_t<material_def_t>			 _edit_previous_materials		 = {};
		vector_t<sid_t>						 _edit_material_ids				 = {};
		vector_t<material_def_t>			 _shader_edit_previous_materials = {};
		vector_t<sid_t>						 _shader_edit_material_ids		 = {};
		bool								 _has_shared_shader				 = false;
		bool								 _edit_active					 = false;
		bool								 _shader_edit_active			 = false;
	};
}
