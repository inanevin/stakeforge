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

#pragma once

#include "ui/widgets/editor_widgets_checkbox.hpp"
#include "ui/widgets/editor_widgets_color_field.hpp"
#include "ui/widgets/editor_widgets_dropdown.hpp"
#include "ui/widgets/editor_widgets_input_field.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include "ui/widgets/editor_widgets_vec_fields.hpp"
#include <sfg/data/vector.hpp>
#include <sfg/common/size_definitions.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class input_router_t;
	class ui_context;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	struct reflected_field_desc_t;

	class editor_widget_reflect_type_t final
	{
	public:
		editor_widget_reflect_type_t()												 = default;
		~editor_widget_reflect_type_t()												 = default;
		editor_widget_reflect_type_t(const editor_widget_reflect_type_t&)			 = delete;
		editor_widget_reflect_type_t& operator=(const editor_widget_reflect_type_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent);
		void uninit();
		void set_reflected_obj(void* object, sid_t type_id);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		struct enum_control_t
		{
			vector_t<editor_dropdown_item_t> items	  = {};
			editor_dropdown_t*				 dropdown = nullptr;
		};

		struct reflected_control_t
		{
			editor_widget_reflect_type_t* owner	 = nullptr;
			const reflected_field_desc_t* field	 = nullptr;
			void*						  object = nullptr;
		};

		struct vector_fold_state_t
		{
			sid_t field_id = 0;
			bool  unfolded = false;
		};

		struct vector_control_t
		{
			editor_widget_reflect_type_t* owner = nullptr;
			const reflected_field_desc_t* field = nullptr;
		};

	private:
		void		clear_reflected_controls();
		void		rebuild_reflected_controls();
		void		install_reflected_control(ui::widget_id_t parent, const reflected_field_desc_t& field, void* object);
		void		install_vector_field(const reflected_field_desc_t& field, const char* label);
		u32			get_vector_item_count(const reflected_field_desc_t& field) const;
		bool		is_vector_unfolded(sid_t field_id) const;
		void		toggle_vector_unfolded(sid_t field_id);
		void		reset_vector_field(const reflected_field_desc_t& field);
		void		add_vector_item(const reflected_field_desc_t& field);
		static void on_number_changed(f32 value, void* user_data);
		static void on_text_changed(const char* value, void* user_data);
		static void on_checkbox_changed(bool checked, void* user_data);
		static void on_color_changed(const vec4f_t& value, void* user_data);
		static void on_vec2_changed(const vec2f_t& value, void* user_data);
		static void on_vec3_changed(const vec3f_t& value, void* user_data);
		static void on_vec4_changed(const vec4f_t& value, void* user_data);
		static u16	on_enum_selected(void* user_data);
		static void on_enum_pressed(u16 value, void* user_data);
		static void on_vector_header_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_vector_reset_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_vector_add_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);

	private:
		vector_t<editor_property_row_t> _rows			 = {};
		vector_t<reflected_control_t>	_controls		 = {};
		vector_t<vector_fold_state_t>	_vector_states	 = {};
		vector_t<vector_control_t>		_vector_controls = {};
		vector_t<editor_input_field_t*> _input_fields	 = {};
		vector_t<editor_checkbox_t*>	_checkboxes		 = {};
		vector_t<editor_color_field_t*> _color_fields	 = {};
		vector_t<editor_vec2_field_t*>	_vec2_fields	 = {};
		vector_t<editor_vec3_field_t*>	_vec3_fields	 = {};
		vector_t<editor_vec4_field_t*>	_vec4_fields	 = {};
		vector_t<enum_control_t>		_dropdowns		 = {};
		ui::ui_context*					_ui				 = nullptr;
		void*							_object			 = nullptr;
		sid_t							_type_id		 = 0;
		ui::widget_id_t					_root			 = NULL_WIDGET;
	};
}
