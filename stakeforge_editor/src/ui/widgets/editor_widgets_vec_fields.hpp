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

#include "ui/widgets/editor_widget_width.hpp"
#include "ui/widgets/editor_widget_input_field.hpp"
#include "ui/widgets/editor_widgets_common.hpp"

#include <sfg/data/vector.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/math/vec4f.hpp>

namespace sfg
{
	struct editor_vec2_field_field_t
	{
		span_t<vec2f_t*> fields = {};
	};

	struct editor_vec2u16_field_field_t
	{
		span_t<vec2u16_t*> fields = {};
	};

	struct editor_vec3_field_field_t
	{
		span_t<vec3f_t*> fields = {};
	};

	struct editor_vec4_field_field_t
	{
		span_t<vec4f_t*> fields = {};
	};

	struct editor_quat_field_field_t
	{
		span_t<quat_t*> fields = {};
	};

	struct editor_vec2_field_config_t
	{
		editor_widget_width_config_t width	   = {};
		editor_vec2_field_field_t	 field	   = {};
		editor_widget_callbacks_t	 callbacks = {};
		f32							 increment = 0.1f;
		bool						 integer   = false;
	};

	struct editor_vec2u16_field_config_t
	{
		editor_widget_width_config_t width	   = {};
		editor_vec2u16_field_field_t field	   = {};
		editor_widget_callbacks_t	 callbacks = {};
	};

	struct editor_vec3_field_config_t
	{
		editor_widget_width_config_t width	   = {};
		editor_vec3_field_field_t	 field	   = {};
		editor_widget_callbacks_t	 callbacks = {};
		f32							 increment = 0.1f;
		bool						 integer   = false;
	};

	struct editor_vec4_field_config_t
	{
		editor_widget_width_config_t width	   = {};
		editor_vec4_field_field_t	 field	   = {};
		editor_widget_callbacks_t	 callbacks = {};
		f32							 increment = 0.1f;
		bool						 integer   = false;
	};

	struct editor_quat_field_config_t
	{
		editor_widget_width_config_t width	   = {};
		editor_quat_field_field_t	 field	   = {};
		editor_widget_callbacks_t	 callbacks = {};
		f32							 increment = 0.1f;
	};

	class editor_vec2_field_t final
	{
	public:
		editor_vec2_field_t()									   = default;
		~editor_vec2_field_t()									   = default;
		editor_vec2_field_t(const editor_vec2_field_t&)			   = delete;
		editor_vec2_field_t& operator=(const editor_vec2_field_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_vec2_field_config_t& config);
		void uninit();
		void set_value(const vec2f_t& value);
		void set_mixed(bool mixed);
		void update_field_data(editor_vec2_field_field_t field);
		void refresh_field_data();

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline const vec2f_t& get_value() const
		{
			return _value;
		}

		inline bool is_editing() const
		{
			return _inputs[0].is_editing() || _inputs[1].is_editing();
		}

	private:
		editor_input_field_t	   _inputs[2] = {};
		editor_vec2_field_config_t _config	  = {};
		vector_t<vec2f_t*>		   _field_values;
		vector_t<u8*>			   _fields[2] = {};
		vec2f_t					   _value	  = {0.0f, 0.0f};

		ui::ui_context* _ui	  = nullptr;
		ui::widget_id_t _root = NULL_WIDGET;
	};

	class editor_vec2u16_field_t final
	{
	public:
		editor_vec2u16_field_t()										 = default;
		~editor_vec2u16_field_t()										 = default;
		editor_vec2u16_field_t(const editor_vec2u16_field_t&)			 = delete;
		editor_vec2u16_field_t& operator=(const editor_vec2u16_field_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_vec2u16_field_config_t& config);
		void uninit();
		void set_value(const vec2u16_t& value);
		void set_mixed(bool mixed);
		void update_field_data(editor_vec2u16_field_field_t field);
		void refresh_field_data();

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline const vec2u16_t& get_value() const
		{
			return _value;
		}

		inline bool is_editing() const
		{
			return _inputs[0].is_editing() || _inputs[1].is_editing();
		}

	private:
		editor_input_field_t		  _inputs[2] = {};
		editor_vec2u16_field_config_t _config	 = {};
		vector_t<vec2u16_t*>		  _field_values;
		vector_t<u8*>				  _fields[2] = {};
		vec2u16_t					  _value	 = vec2u16_t::zero;

		ui::ui_context* _ui	  = nullptr;
		ui::widget_id_t _root = NULL_WIDGET;
	};

	class editor_vec3_field_t final
	{
	public:
		editor_vec3_field_t()									   = default;
		~editor_vec3_field_t()									   = default;
		editor_vec3_field_t(const editor_vec3_field_t&)			   = delete;
		editor_vec3_field_t& operator=(const editor_vec3_field_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_vec3_field_config_t& config);
		void uninit();
		void set_value(const vec3f_t& value);
		void set_mixed(bool mixed);
		void update_field_data(editor_vec3_field_field_t field);
		void refresh_field_data();

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline const vec3f_t& get_value() const
		{
			return _value;
		}

		inline bool is_editing() const
		{
			return _inputs[0].is_editing() || _inputs[1].is_editing() || _inputs[2].is_editing();
		}

	private:
		editor_input_field_t	   _inputs[3] = {};
		editor_vec3_field_config_t _config	  = {};
		vector_t<vec3f_t*>		   _field_values;
		vector_t<u8*>			   _fields[3] = {};
		vec3f_t					   _value	  = {0.0f, 0.0f, 0.0f};

		ui::ui_context* _ui	  = nullptr;
		ui::widget_id_t _root = NULL_WIDGET;
	};

	class editor_vec4_field_t final
	{
	public:
		editor_vec4_field_t()									   = default;
		~editor_vec4_field_t()									   = default;
		editor_vec4_field_t(const editor_vec4_field_t&)			   = delete;
		editor_vec4_field_t& operator=(const editor_vec4_field_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_vec4_field_config_t& config);
		void uninit();
		void set_value(const vec4f_t& value);
		void set_mixed(bool mixed);
		void update_field_data(editor_vec4_field_field_t field);
		void refresh_field_data();

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline const vec4f_t& get_value() const
		{
			return _value;
		}

		inline bool is_editing() const
		{
			return _inputs[0].is_editing() || _inputs[1].is_editing() || _inputs[2].is_editing() || _inputs[3].is_editing();
		}

	private:
		editor_input_field_t	   _inputs[4] = {};
		editor_vec4_field_config_t _config	  = {};
		vector_t<vec4f_t*>		   _field_values;
		vector_t<u8*>			   _fields[4] = {};
		vec4f_t					   _value	  = {0.0f, 0.0f, 0.0f, 0.0f};

		ui::ui_context* _ui	  = nullptr;
		ui::widget_id_t _root = NULL_WIDGET;
	};

	class editor_quat_field_t final
	{
	public:
		editor_quat_field_t()									   = default;
		~editor_quat_field_t()									   = default;
		editor_quat_field_t(const editor_quat_field_t&)			   = delete;
		editor_quat_field_t& operator=(const editor_quat_field_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_quat_field_config_t& config);
		void uninit();
		void set_value(const quat_t& value);
		void set_mixed(bool mixed);
		void update_field_data(editor_quat_field_field_t field);
		void refresh_field_data();

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline const quat_t& get_value() const
		{
			return _value;
		}

		inline bool is_editing() const
		{
			return _inputs[0].is_editing() || _inputs[1].is_editing() || _inputs[2].is_editing();
		}

	private:
		void modify_field();

		static void on_euler_edit_begin(void* user_data);
		static void on_euler_data_changed(void* user_data);
		static void on_euler_edit_submitted(void* user_data);

	private:
		editor_input_field_t	   _inputs[3] = {};
		editor_quat_field_config_t _config	  = {};
		vector_t<quat_t*>		   _field_values;
		vector_t<vec3f_t>		   _euler_values;
		vector_t<u8*>			   _fields[3] = {};
		quat_t					   _value	  = {};

		ui::ui_context* _ui	  = nullptr;
		ui::widget_id_t _root = NULL_WIDGET;
	};
}
