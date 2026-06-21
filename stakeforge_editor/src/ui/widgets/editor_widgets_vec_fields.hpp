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

#include "ui/widgets/editor_widget_width.hpp"
#include "ui/widgets/editor_widgets_input_field.hpp"
#include <sfg/math/vec3f.hpp>
#include <sfg/math/vec4f.hpp>

namespace sfg
{
	using editor_vec2_field_changed_fn = void (*)(const vec2f_t& value, void* user_data);
	using editor_vec3_field_changed_fn = void (*)(const vec3f_t& value, void* user_data);
	using editor_vec4_field_changed_fn = void (*)(const vec4f_t& value, void* user_data);

	struct editor_vec2_field_config_t
	{
		editor_vec2_field_changed_fn on_changed	  = nullptr;
		editor_vec2_field_changed_fn on_submitted = nullptr;
		void*						 user_data	  = nullptr;
		editor_widget_width_config_t width		  = {};
		vec2f_t						 value		  = {0.0f, 0.0f};
		f32							 increment	  = 0.1f;
		bool						 integer	  = false;
	};

	struct editor_vec3_field_config_t
	{
		editor_vec3_field_changed_fn on_changed	  = nullptr;
		editor_vec3_field_changed_fn on_submitted = nullptr;
		void*						 user_data	  = nullptr;
		editor_widget_width_config_t width		  = {};
		vec3f_t						 value		  = {0.0f, 0.0f, 0.0f};
		f32							 increment	  = 0.1f;
		bool						 integer	  = false;
	};

	struct editor_vec4_field_config_t
	{
		editor_vec4_field_changed_fn on_changed	  = nullptr;
		editor_vec4_field_changed_fn on_submitted = nullptr;
		void*						 user_data	  = nullptr;
		editor_widget_width_config_t width		  = {};
		vec4f_t						 value		  = {0.0f, 0.0f, 0.0f, 0.0f};
		f32							 increment	  = 0.1f;
		bool						 integer	  = false;
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

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline const vec2f_t& get_value() const
		{
			return _value;
		}

	private:
		struct component_t
		{
			editor_vec2_field_t* owner = nullptr;
			u8					 index = 0;
		};

		static void on_component_changed(f32 value, void* user_data);
		static void on_component_submitted(const char* text, f32 value, void* user_data);

	private:
		ui::ui_context*			   _ui			  = nullptr;
		ui::widget_id_t			   _root		  = NULL_WIDGET;
		editor_vec2_field_config_t _config		  = {};
		editor_input_field_t	   _inputs[2]	  = {};
		component_t				   _components[2] = {};
		vec2f_t					   _value		  = {0.0f, 0.0f};
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

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline const vec3f_t& get_value() const
		{
			return _value;
		}

	private:
		struct component_t
		{
			editor_vec3_field_t* owner = nullptr;
			u8					 index = 0;
		};

		static void on_component_changed(f32 value, void* user_data);
		static void on_component_submitted(const char* text, f32 value, void* user_data);

	private:
		ui::ui_context*			   _ui			  = nullptr;
		ui::widget_id_t			   _root		  = NULL_WIDGET;
		editor_vec3_field_config_t _config		  = {};
		editor_input_field_t	   _inputs[3]	  = {};
		component_t				   _components[3] = {};
		vec3f_t					   _value		  = {0.0f, 0.0f, 0.0f};
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

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

		inline const vec4f_t& get_value() const
		{
			return _value;
		}

	private:
		struct component_t
		{
			editor_vec4_field_t* owner = nullptr;
			u8					 index = 0;
		};

		static void on_component_changed(f32 value, void* user_data);
		static void on_component_submitted(const char* text, f32 value, void* user_data);

	private:
		ui::ui_context*			   _ui			  = nullptr;
		ui::widget_id_t			   _root		  = NULL_WIDGET;
		editor_vec4_field_config_t _config		  = {};
		editor_input_field_t	   _inputs[4]	  = {};
		component_t				   _components[4] = {};
		vec4f_t					   _value		  = {0.0f, 0.0f, 0.0f, 0.0f};
	};
}
