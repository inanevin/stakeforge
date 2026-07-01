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
#include <sfg/runtime/engine/common_engine.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class input_router_t;
	class ui_context;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	class editor_checkbox_t;
	class editor_input_field_t;
	class editor_vec2_field_t;
	class editor_vec3_field_t;
	class editor_vec4_field_t;
	class editor_widget_fold_label_t;
	class editor_widget_reference_t;
	struct reflected_field_t;

	struct editor_widget_reflection_config_t
	{
		span_t<void*>  objects = {};
		sid_t		   type_id = 0;
		world_handle_t world   = {};
	};

	class editor_widget_reflection_t final
	{
	public:
		editor_widget_reflection_t()											 = default;
		~editor_widget_reflection_t()											 = default;
		editor_widget_reflection_t(const editor_widget_reflection_t&)			 = delete;
		editor_widget_reflection_t& operator=(const editor_widget_reflection_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_reflection_config_t& config);
		void uninit();
		void set_reflection(const editor_widget_reflection_config_t& config);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		struct container_user_data_t
		{
			const reflected_field_t*	field	   = nullptr;
			editor_widget_reflection_t* reflection = nullptr;
			editor_widget_fold_label_t* fold	   = nullptr;
			vector_t<void*>				containers;
			world_handle_t				world		= {};
			f32							indentation = 0.0f;
		};

		void				   clear_widgets();
		void				   fit_control(ui::widget_id_t widget);
		void				   create_fields(ui::widget_id_t parent, span_t<void*> objects, sid_t type_id, world_handle_t world, bool track_rows, bool sub_item, f32 indentation, bool add_divider);
		void				   create_checkbox(ui::widget_id_t parent, const reflected_field_t* const field, span_t<u8*> fields, bool track_row, bool sub_item, f32 indentation);
		void				   create_input_field(ui::widget_id_t parent, const reflected_field_t* const field, span_t<u8*> fields, bool track_row, bool sub_item, f32 indentation);
		bool				   create_reference(ui::widget_id_t parent, const reflected_field_t* const field, span_t<u64*> fields, world_handle_t world, bool track_row, bool sub_item, f32 indentation);
		bool				   create_vector_field(ui::widget_id_t parent, const reflected_field_t* const field, span_t<u8*> fields, bool track_row, bool sub_item, f32 indentation);
		void				   create_object(ui::widget_id_t parent, const reflected_field_t* const field, span_t<void*> objects, world_handle_t world, bool track_row, bool sub_item, f32 indentation);
		void				   create_container(ui::widget_id_t parent, const reflected_field_t* const field, span_t<void*> containers, world_handle_t world, bool track_row, bool sub_item, f32 indentation);
		void				   create_container_elements(ui::widget_id_t parent, const reflected_field_t* const field, span_t<void*> containers, world_handle_t world, f32 indentation);
		container_user_data_t* create_container_user_data(const reflected_field_t* field, span_t<void*> containers, world_handle_t world, f32 indentation, editor_widget_fold_label_t* fold);
		void				   refresh_container(container_user_data_t& data);
		void				   clear_container_widgets(ui::widget_id_t parent);
		void				   clear_child_tooltips(ui::widget_id_t parent);
		bool				   is_child_widget(ui::widget_id_t widget, ui::widget_id_t parent) const;
		void				   install_sub_item_button(ui::widget_id_t parent, ui::widget_id_t control = NULL_WIDGET);
		void				   install_tooltip(ui::widget_id_t owner, const char* text);
		void				   clear_tooltips();

		static void on_container_add(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_container_reset(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);

	private:
		ui::ui_context*						  _ui	= nullptr;
		ui::widget_id_t						  _root = NULL_WIDGET;
		vector_t<editor_input_field_t*>		  _inputs;
		vector_t<editor_checkbox_t*>		  _checkboxes;
		vector_t<editor_vec2_field_t*>		  _vec2_fields;
		vector_t<editor_vec3_field_t*>		  _vec3_fields;
		vector_t<editor_vec4_field_t*>		  _vec4_fields;
		vector_t<editor_widget_fold_label_t*> _fold_labels;
		vector_t<editor_widget_reference_t*>  _references;
		vector_t<container_user_data_t*>	  _container_user_data;
		vector_t<ui::widget_id_t>			  _dividers;
		vector_t<ui::widget_id_t>			  _rows;
		vector_t<ui::widget_id_t>			  _tooltip_owners;
	};
}
