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

#include "gui/vekt_defines.hpp"
#include "math/vector4.hpp"
#include "data/vector.hpp"
#include "data/string.hpp"
#include "common/string_id.hpp"
#include "reflection/common_reflection.hpp"

namespace vekt
{
	struct font;
	class builder;
}

namespace SFG
{
	class font;

	enum class gui_text_field_type : unsigned char
	{
		text_only,
		number,
	};

	class gui_builder
	{
	public:
		size_t MAX_TEXT_FIELD_SIZE	   = 256;
		size_t MAX_RESOURCE_FIELD_SIZE = 512;
		size_t MAX_CHECKBOX_SIZE	   = 4;

		typedef void (*input_field_fn)(void* callback_ud, vekt::builder* b, vekt::id id, const char* txt, f32 value);
		typedef void (*checkbox_fn)(void* callback_ud, vekt::builder* b, vekt::id id, unsigned char value);
		typedef void (*resource_fn)(void* callback_ud, vekt::builder* b, vekt::id id, const string& value);
		typedef void (*dropdown_fn)(void* callback_ud, vekt::builder* b, vekt::id dropdown_widget, unsigned int index);
		typedef void (*control_button_fn)(void* callback_ud, void* object_ptr, string_id type_id, string_id button_id);
		typedef void (*toggle_button_fn)(void* callback_ud, vekt::builder* b, vekt::id id, bool toggled);

		struct gui_builder_callbacks
		{
			void*			  user_data				 = nullptr;
			void*			  callback_ud			 = nullptr;
			vekt::mouse_func  on_mouse				 = nullptr;
			vekt::key_func	  on_key				 = nullptr;
			input_field_fn	  on_input_field_changed = nullptr;
			checkbox_fn		  on_checkbox_changed	 = nullptr;
			resource_fn		  on_resource_changed	 = nullptr;
			control_button_fn on_control_button		 = nullptr;
			toggle_button_fn  on_toggle_button		 = nullptr;
		};

		struct id_pair
		{
			vekt::id first;
			vekt::id second;
		};

		struct id_trip
		{
			vekt::id first;
			vekt::id second;
			vekt::id third;
		};

		struct id_quat
		{
			vekt::id first;
			vekt::id second;
			vekt::id third;
			vekt::id fourth;
		};

		struct id_penth
		{
			vekt::id first;
			vekt::id second;
			vekt::id third;
			vekt::id fourth;
			vekt::id fifth;
		};

		struct gui_text_field
		{
			const char*			buffer			= nullptr;
			vekt::id			widget			= NULL_WIDGET_ID;
			vekt::id			text_widget		= NULL_WIDGET_ID;
			vekt::id			sliding_widget	= NULL_WIDGET_ID;
			unsigned int		caret_pos		= 0;
			unsigned int		buffer_size		= 0;
			unsigned int		caret_end_pos	= 0;
			unsigned int		buffer_capacity = 0;
			unsigned int		decimals		= 0;
			unsigned int		sub_index		= 0;
			f32				value			= 0.0f;
			f32				value_increment = 0.0f;
			f32				min				= 0.0f;
			f32				max				= 0.0f;
			gui_text_field_type type			= gui_text_field_type::text_only;
			unsigned char		is_editing		= 0;
			unsigned char		is_slider		= 0;

			unsigned int selection_min() const;
			unsigned int selection_max() const;
			void		 collapse_caret_to(unsigned int p);
			void		 delete_range(unsigned int from, unsigned int to);
			bool		 delete_selection_if_any();
			void		 insert_string_at_caret(const char* s, unsigned int len);
			void		 insert_char_at_caret(char c);
		};

		struct gui_checkbox
		{
			vekt::id	  widget	  = NULL_WIDGET_ID;
			vekt::id	  text_widget = NULL_WIDGET_ID;
			unsigned char state		  = 0;
		};

		struct gui_resource
		{
			const char* extension	= nullptr;
			string_id	type		= 0;
			vekt::id	widget		= NULL_WIDGET_ID;
			vekt::id	text_widget = NULL_WIDGET_ID;
		};

		struct gui_slider
		{
			f32	 min		   = 0.0f;
			f32	 max		   = 0.0;
			f32	 value		   = 0.0f;
			vekt::id widget		   = NULL_WIDGET_ID;
			vekt::id slider_widget = NULL_WIDGET_ID;
			vekt::id text_widget   = NULL_WIDGET_ID;
		};

		struct gui_dropdown
		{
			vekt::id			widget		= NULL_WIDGET_ID;
			vekt::id			text_widget = NULL_WIDGET_ID;
			vector<const char*> items;
			u8				selected = 0;
		};

		struct reflected_property
		{
			void*		 obj		= nullptr;
			string_id	 type		= 0;
			field_base*	 field		= nullptr;
			vekt::id	 widget		= 0;
			unsigned int list_index = 0;
		};

		struct control_button_binding
		{
			void*	  obj		= nullptr;
			string_id type		= 0;
			string_id button_id = 0;
			vekt::id  widget	= 0;
		};

		struct dropdown_ctx_binding
		{
			vekt::id	 item_widget	 = NULL_WIDGET_ID;
			vekt::id	 dropdown_widget = NULL_WIDGET_ID;
			unsigned int index			 = 0;
		};

		struct gui_toggle_button
		{
			vekt::id widget	 = NULL_WIDGET_ID;
			bool	 toggled = false;
		};
		gui_builder_callbacks callbacks = {};

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(vekt::builder* b);
		void uninit();
		void on_context_item(vekt::id widget);

		// -----------------------------------------------------------------------------
		// big layout
		// -----------------------------------------------------------------------------

		void deallocate_children(vekt::id id);
		void deallocate(vekt::id id);

		vekt::id begin_area(bool fill = true, bool scroller = true);
		void	 end_area();
		vekt::id begin_row();
		void	 end_row();
		vekt::id begin_column();
		void	 end_column();

		// -----------------------------------------------------------------------------
		// properties
		// -----------------------------------------------------------------------------

		vekt::id add_reflected_field(field_base* field, string_id type_id, void* object_ptr);
		vekt::id add_control_button(const char* title, string_id type_id, void* object_ptr, string_id button_id);

		id_pair	 add_property_row_label(const char* label, const char* label2, size_t buffer_capacity = 0);
		vekt::id add_property_single_label(const char* label, size_t buffer_capacity = 0);
		id_pair	 add_property_single_button(const char* label, size_t buffer_capacity = 0);
		vekt::id add_property_single_hyperlink(const char* label, size_t buffer_capacity = 0);
		id_pair	 add_property_row_checkbox(const char* label, bool initial_state);
		id_pair	 add_property_row_resource(const char* label, const char* extension, const char* initial_resource, string_id type_id, size_t buffer_capacity = 0);
		id_pair	 add_property_row_slider(const char* label, size_t buffer_capacity = 0, f32 min = 0.0f, f32 max = 0.0f, f32 val = 0.0f, bool is_int = false);
		id_pair	 add_property_row_dropdown(const char* label, const char* initial_text = "", size_t buffer_capacity = 0);

		// property row variants for fields
		id_trip	 add_property_row_text_field(const char* label, const char* text, size_t buffer_capacity = 0, gui_text_field_type type = gui_text_field_type::text_only, unsigned int decimals = 0, f32 increment = 0.0f);
		id_trip	 add_property_row_vector2(const char* label, const char* text, size_t buffer_capacity = 0, unsigned int decimals = 3, f32 increment = 0.1f);
		id_quat	 add_property_row_vector3(const char* label, const char* text, size_t buffer_capacity = 0, unsigned int decimals = 3, f32 increment = 0.1f);
		id_penth add_property_row_vector4(const char* label, const char* text, size_t buffer_capacity = 0, unsigned int decimals = 3, f32 increment = 0.1f);
		vekt::id add_property_row();
		vekt::id add_row_cell(f32 size);
		vekt::id add_row_separator(const vector4& color);
		vekt::id add_col_separator(const vector4& color);

		// -----------------------------------------------------------------------------
		// raw items
		// -----------------------------------------------------------------------------

		vekt::id add_sub_title(const char* title);
		id_trip	 add_component_title(const char* title);
		vekt::id add_title(const char* title);
		vekt::id add_label(const char* label, size_t buffer_capacity = 0);
		vekt::id add_hyperlink(const char* label, size_t buffer_capacity = 0);
		id_pair	 add_button(const char* title, size_t buffer_capacity = 0);
		id_pair	 add_icon_button(const char* icon, size_t buffer_capacity = 0, f32 size = 1.0f, bool is_row = false, vector4 color = vector4());
		id_pair	 add_toggle_button(const char* icon, bool toggled_on, size_t buffer_capacity = 0, f32 size = 1.0f, bool is_row = false, vector4 color = vector4());

		id_pair	 add_text_field(const char*			text,
								size_t				buffer_capacity = 0,
								gui_text_field_type type			= gui_text_field_type::text_only,
								unsigned int		decimals		= 0,
								f32				increment		= 0.0f,
								f32				min				= 0.0f,
								f32				max				= 0.0f,
								f32				val				= 0.0f,
								unsigned char		is_slider		= 0,
								unsigned int		sub_index		= 0);
		vekt::id add_checkbox(bool initial_state);
		vekt::id add_resource(const char* res, const char* extension, string_id type_id, size_t buffer_capacity = 0);
		vekt::id add_dropdown(const char* initial_text = "", size_t buffer_capacity = 0);
		void	 add_dropdown_item(vekt::id dropdown_widget, const char* label, bool is_selected);
		// vekt::id add_slider(f32 val, f32 min, f32 max, size_t buffer_capacity = 0);

		// -----------------------------------------------------------------------------
		// util
		// -----------------------------------------------------------------------------

		vekt::id set_fill_x(vekt::id id);
		void	 set_text_field_text(gui_text_field& tf, const char* text);
		void	 set_text_field_text(vekt::id id, const char* text, bool skip_if_focused);
		void	 set_text_field_value(vekt::id id, f32 f, bool skip_if_focused, bool is_int = false);
		void	 text_field_edit_complete(gui_text_field& tf);
		void	 set_checkbox_value(vekt::id id, unsigned char value);
		void	 set_widget_enabled(vekt::id id, bool enabled, const vector4& enabled_col, const vector4& disabled_col);
		bool	 invoke_control_button(vekt::id widget);
		void	 set_toggle_button_state(vekt::id widget, bool toggled);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline vekt::id get_root() const
		{
			return _root;
		}

		inline void set_draw_order(unsigned int o)
		{
			_draw_order = o;
		}

		inline void set_indent(f32 f)
		{
			_indent = f;
		}

		static vekt::input_event_result on_text_field_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase);
		static vekt::input_event_result on_text_field_key(vekt::builder* b, vekt::id widget, const vekt::key_event& ev);
		static void						on_text_field_draw(vekt::builder* b, vekt::id widget);
		static void						on_text_field_drag(vekt::builder* b, vekt::id widget, f32 mp_x, f32 mp_y, f32 delta_x, f32 delta_y, unsigned int button);
		static void						on_text_field_focus_lost(vekt::builder* b, vekt::id widget);
		static void						on_text_field_focus_gained(vekt::builder* b, vekt::id widget, bool from_nav);
		static void						on_context_item_hover_begin(vekt::builder* b, vekt::id widget);
		static void						on_context_item_hover_end(vekt::builder* b, vekt::id widget);
		static vekt::input_event_result on_toggle_button_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase);
		static vekt::input_event_result on_checkbox_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase);
		static vekt::input_event_result on_resource_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase);
		static vekt::input_event_result on_dropdown_mouse(vekt::builder* b, vekt::id widget, const vekt::mouse_event& ev, vekt::input_event_phase phase);

		vekt::id new_widget(bool push_to_stack = false);
		vekt::id pop_stack();
		vekt::id stack();
		void	 push_stack(vekt::id s);

	private:
		void invoke_reflection(vekt::id widget, void* data_params, unsigned int sub_index = 0);
		void remove_impl(vekt::id id);

	private:
		static constexpr unsigned int STACK_SIZE = 512;

		vekt::builder*				   _builder				  = nullptr;
		vector<gui_text_field>		   _text_fields			  = {};
		vector<gui_checkbox>		   _checkboxes			  = {};
		vector<gui_resource>		   _resources			  = {};
		vector<gui_dropdown>		   _dropdowns			  = {};
		vector<dropdown_ctx_binding>   _dropdown_ctx_bindings = {};
		vector<reflected_property>	   _reflected			  = {};
		vector<control_button_binding> _control_buttons		  = {};
		vector<gui_toggle_button>	   _toggle_buttons		  = {};

		vekt::id _stack[STACK_SIZE] = {NULL_WIDGET_ID};
		vekt::id _root				= NULL_WIDGET_ID;
		vekt::id _stack_ptr			= 0;

		f32		 _indent	 = 0.0f;
		unsigned int _draw_order = 0;
	};

}
