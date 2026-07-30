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

#include "ui/widgets/editor_widgets_common.hpp"
#include "world/editor_world_handle.hpp"
#include <sfg/common/size_definitions.hpp>
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
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
	class editor_color_field_t;
	class editor_dropdown_t;
	class editor_input_field_t;
	class editor_quat_field_t;
	class editor_vec2_field_t;
	class editor_vec2u16_field_t;
	class editor_vec3_field_t;
	class editor_vec4_field_t;
	class editor_widget_fold_label_t;
	class editor_widget_reference_t;
	struct reflected_field_t;
	struct reflected_type_t;

	struct editor_widget_reflection_dropdown_item_t
	{
		const char* text  = nullptr;
		u64			value = 0;
	};

	using editor_widget_reflection_dropdown_items_fn = span_t<const editor_widget_reflection_dropdown_item_t> (*)(sid_t field_id, sid_t owner_field_id, u32 element_index, void* user_data);

	struct editor_widget_reflection_fold_state_t
	{
		sid_t type_id  = 0;
		sid_t field_id = 0;
		bool  folded   = false;
	};

	struct editor_widget_reflection_config_t
	{
		vector_t<editor_widget_reflection_fold_state_t>* fold_states			  = nullptr;
		editor_widget_callbacks_t						 callbacks				  = {};
		span_t<void*>									 objects				  = {};
		sid_t											 type_id				  = 0;
		editor_world_handle_t							 world					  = {};
		editor_widget_reflection_dropdown_items_fn		 dropdown_items			  = nullptr;
		void*											 dropdown_items_user_data = nullptr;
		bool											 block_edits			  = false;
		bool											 elevate_draw_order		  = false;
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
		void save_fold_states();

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
			editor_world_handle_t		world		= {};
			sid_t						type_id		= 0;
			f32							indentation = 0.0f;
		};

		struct container_element_user_data_t
		{
			container_user_data_t* container_data = nullptr;
			ui::widget_id_t		   button		  = NULL_WIDGET;
			u32					   element_index  = 0;
		};

		struct path_picker_user_data_t
		{
			editor_input_field_t* input		  = nullptr;
			sid_t				  sub_type_id = 0;
			ui::widget_id_t		  button	  = NULL_WIDGET;
		};

		struct field_fold_t
		{
			editor_widget_fold_label_t* fold	 = nullptr;
			sid_t						type_id	 = 0;
			sid_t						field_id = 0;
		};

		struct dependent_field_t
		{
			vector_t<void*>			 objects;
			const reflected_type_t*	 type	= nullptr;
			const reflected_field_t* field	= nullptr;
			ui::widget_id_t			 widget = NULL_WIDGET;
		};

		struct field_divider_t
		{
			ui::widget_id_t widget	= NULL_WIDGET;
			ui::widget_id_t divider = NULL_WIDGET;
		};

		void clear_widgets();
		void set_block_edits(bool block_edits);
		void fit_control(ui::widget_id_t widget);
		bool is_field_visible(const reflected_type_t& type, const reflected_field_t& field, span_t<void*> objects, u32 dependency_depth) const;
		void refresh_dependency_visibility();
		void create_fields(ui::widget_id_t parent, span_t<void*> objects, sid_t type_id, editor_world_handle_t world, bool track_rows, bool sub_item, f32 indentation, bool add_divider, u32 element_index = 0);
		void create_checkbox(ui::widget_id_t parent, const reflected_field_t* const field, span_t<u8*> fields, bool track_row, bool sub_item, bool removable_item, f32 indentation, container_user_data_t* container_data = nullptr, u32 element_index = 0);
		bool create_dropdown(
			ui::widget_id_t parent, const reflected_field_t* const field, sid_t owner_field_id, span_t<u8*> fields, bool track_row, bool sub_item, bool removable_item, f32 indentation, container_user_data_t* container_data = nullptr, u32 element_index = 0);
		void create_bitmask_dropdown(
			ui::widget_id_t parent, const reflected_type_t& type, const reflected_field_t& field, span_t<u8*> fields, bool track_row, bool sub_item, bool removable_item, f32 indentation, container_user_data_t* container_data = nullptr, u32 element_index = 0);
		void create_input_field(ui::widget_id_t parent, const reflected_field_t* const field, span_t<u8*> fields, bool track_row, bool sub_item, bool removable_item, f32 indentation, container_user_data_t* container_data = nullptr, u32 element_index = 0);
		bool create_reference(ui::widget_id_t				 parent,
							  const reflected_field_t* const field,
							  span_t<u64*>					 fields,
							  editor_world_handle_t			 world,
							  bool							 track_row,
							  bool							 sub_item,
							  bool							 removable_item,
							  f32							 indentation,
							  container_user_data_t*		 container_data = nullptr,
							  u32							 element_index	= 0);
		bool create_quat_field(ui::widget_id_t parent, const reflected_field_t* const field, span_t<u8*> fields, bool track_row, bool sub_item, bool removable_item, f32 indentation, container_user_data_t* container_data = nullptr, u32 element_index = 0);
		bool create_vector_field(ui::widget_id_t parent, const reflected_field_t* const field, span_t<u8*> fields, bool track_row, bool sub_item, bool removable_item, f32 indentation, container_user_data_t* container_data = nullptr, u32 element_index = 0);
		bool create_color_field(ui::widget_id_t parent, const reflected_field_t* const field, span_t<u8*> fields, bool track_row, bool sub_item, bool removable_item, f32 indentation, container_user_data_t* container_data = nullptr, u32 element_index = 0);
		void create_object(ui::widget_id_t				  parent,
						   sid_t						  type_id,
						   const reflected_field_t* const field,
						   span_t<void*>				  objects,
						   editor_world_handle_t		  world,
						   bool							  track_row,
						   bool							  sub_item,
						   bool							  removable_item,
						   f32							  indentation,
						   container_user_data_t*		  container_data = nullptr,
						   u32							  element_index	 = 0);
		void create_container(ui::widget_id_t parent, sid_t type_id, const reflected_field_t* const field, span_t<void*> containers, editor_world_handle_t world, bool track_row, bool sub_item, f32 indentation);
		void create_container_elements(ui::widget_id_t parent, container_user_data_t* container_data);
		bool get_fold_state(sid_t type_id, sid_t field_id, bool& out_folded) const;
		void set_fold_state(sid_t type_id, sid_t field_id, bool folded);
		container_user_data_t*		   create_container_user_data(const reflected_field_t* field, span_t<void*> containers, editor_world_handle_t world, sid_t type_id, f32 indentation, editor_widget_fold_label_t* fold);
		container_element_user_data_t* create_container_element_user_data(container_user_data_t* container_data, u32 element_index, ui::widget_id_t button);
		void						   request_container_refresh(container_user_data_t& data);
		void						   refresh_container(container_user_data_t& data);
		void						   clear_container_widgets(ui::widget_id_t parent);
		void						   clear_child_tooltips(ui::widget_id_t parent);
		bool						   is_child_widget(ui::widget_id_t widget, ui::widget_id_t parent) const;
		ui::widget_id_t				   install_sub_item_button(ui::widget_id_t parent, ui::widget_id_t control = NULL_WIDGET, container_user_data_t* container_data = nullptr, u32 element_index = 0);
		void						   install_container_element_remove_listener(ui::widget_id_t button, container_user_data_t* container_data, u32 element_index);
		void						   install_path_picker_button(ui::widget_id_t parent, editor_input_field_t* input, sid_t sub_type_id);
		void						   install_tooltip(ui::widget_id_t owner, const char* text);
		void						   clear_tooltips();

		static void on_container_add(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_container_reset(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_container_element_remove(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_path_picker(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_container_refresh(ui::ui_context& ui, void* user_data);
		static void on_field_edit_begin(void* user_data);
		static void on_field_edited(void* user_data);
		static void on_field_edit_submitted(void* user_data);

	private:
		editor_widget_callbacks_t						 _callbacks		  = {};
		editor_widget_callbacks_t						 _field_callbacks = {};
		vector_t<void*>									 _objects;
		vector_t<editor_input_field_t*>					 _inputs;
		vector_t<editor_checkbox_t*>					 _checkboxes;
		vector_t<editor_dropdown_t*>					 _dropdowns;
		vector_t<editor_color_field_t*>					 _color_fields;
		vector_t<editor_quat_field_t*>					 _quat_fields;
		vector_t<editor_vec2_field_t*>					 _vec2_fields;
		vector_t<editor_vec2u16_field_t*>				 _vec2u16_fields;
		vector_t<editor_vec3_field_t*>					 _vec3_fields;
		vector_t<editor_vec4_field_t*>					 _vec4_fields;
		vector_t<editor_widget_fold_label_t*>			 _fold_labels;
		vector_t<editor_widget_reference_t*>			 _references;
		vector_t<container_user_data_t*>				 _container_user_data;
		vector_t<container_element_user_data_t*>		 _container_element_user_data;
		vector_t<path_picker_user_data_t*>				 _path_picker_user_data;
		vector_t<field_fold_t>							 _field_folds;
		vector_t<dependent_field_t>						 _dependent_fields;
		vector_t<field_divider_t>						 _field_dividers;
		vector_t<ui::widget_id_t>						 _dividers;
		vector_t<ui::widget_id_t>						 _rows;
		vector_t<ui::widget_id_t>						 _tooltip_owners;
		vector_t<editor_widget_reflection_fold_state_t>* _fold_states			   = nullptr;
		editor_widget_reflection_dropdown_items_fn		 _dropdown_items		   = nullptr;
		void*											 _dropdown_items_user_data = nullptr;
		ui::ui_context*									 _ui					   = nullptr;
		sid_t											 _type_id				   = 0;
		editor_world_handle_t							 _world					   = {};
		ui::widget_id_t									 _root					   = NULL_WIDGET;
		ui::widget_id_t									 _blocker				   = NULL_WIDGET;
	};
}
