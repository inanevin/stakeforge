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
		void			refresh_display();
		void			refresh_display_common();
		void			refresh_display_data();
		void			clear_display();
		void			fit_control(ui::widget_id_t widget);
		void			append_property_row(ui::widget_id_t row);
		ui::widget_id_t make_section_label(const char* text);
		ui::widget_id_t make_value_label(ui::widget_id_t parent, const char* text, bool warn);
		bool			can_mutate_ui_topology() const;
		bool			load_shared_shader_definition();
		void			normalize_materials_to_shader_definition();
		void			sync_pass_flags();
		void			begin_material_edit();
		void			submit_material_edit();
		void			clear_material_edit();
		void			begin_shader_edit();
		void			submit_shader_edit();
		void			clear_shader_edit();
		void			request_materials_refresh(span_t<const sid_t> materials);
		void			request_display_refresh();
		void			flush_pending_ui_mutations();
		void			on_material_edit_begin();
		void			on_material_edited();
		void			on_material_edit_submitted();
		void			on_shader_edit_begin();
		void			on_shader_edited();
		void			on_shader_edit_submitted();

		static void on_material_edit_begin(void* user_data);
		static void on_material_edited(void* user_data);
		static void on_material_edit_submitted(void* user_data);
		static void on_shader_edit_begin(void* user_data);
		static void on_shader_edited(void* user_data);
		static void on_shader_edit_submitted(void* user_data);
		static void on_ui_mutation(ui::ui_context& ui, void* user_data);

	private:
		ui::ui_context* _ui	  = nullptr;
		ui::widget_id_t _root = NULL_WIDGET;

		shader_data_definition_t			 _shader_definition				 = {};
		vector_t<editor_widget_reference_t*> _references					 = {};
		vector_t<editor_dropdown_t*>		 _dropdowns						 = {};
		vector_t<editor_checkbox_t*>		 _checkboxes					 = {};
		vector_t<editor_color_field_t*>		 _color_fields					 = {};
		vector_t<editor_input_field_t*>		 _inputs						 = {};
		vector_t<editor_vec2_field_t*>		 _vec2_fields					 = {};
		vector_t<editor_vec4_field_t*>		 _vec4_fields					 = {};
		vector_t<ui::widget_id_t>			 _rows							 = {};
		vector_t<ui::widget_id_t>			 _dividers						 = {};
		vector_t<ui::widget_id_t>			 _labels						 = {};
		vector_t<sid_t>						 _pending_material_ids			 = {};
		vector_t<material_def_t>			 _materials						 = {};
		vector_t<sid_t>						 _material_ids					 = {};
		vector_t<material_def_t>			 _edit_previous_materials		 = {};
		vector_t<sid_t>						 _edit_material_ids				 = {};
		vector_t<material_def_t>			 _shader_edit_previous_materials = {};
		vector_t<sid_t>						 _shader_edit_material_ids		 = {};
		vector_t<u32>						 _pass_flags					 = {};
		bool								 _has_shared_shader				 = false;
		bool								 _edit_active					 = false;
		bool								 _shader_edit_active			 = false;
		bool								 _refresh_materials_pending		 = false;
	};
}
