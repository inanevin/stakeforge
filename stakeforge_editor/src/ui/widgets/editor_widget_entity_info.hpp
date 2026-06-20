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

#include "ui/widgets/editor_widget_text_id.hpp"
#include "ui/widgets/editor_widgets_vec_fields.hpp"
#include <sfg/math/quat.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class world_t;

	class editor_widget_entity_info_t final
	{
	public:
		editor_widget_entity_info_t()											   = default;
		~editor_widget_entity_info_t()											   = default;
		editor_widget_entity_info_t(const editor_widget_entity_info_t&)			   = delete;
		editor_widget_entity_info_t& operator=(const editor_widget_entity_info_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, world_handle_t world);
		void uninit();
		void set_entity(world_t& world, entity_id_t entity);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		static u32	on_name_selected(void* user_data);
		static void on_name_submitted(const char* value, void* user_data);
		static void on_position_changed(const vec3f_t& value, void* user_data);
		static void on_position_submitted(const vec3f_t& value, void* user_data);
		static void on_rotation_changed(const vec3f_t& value, void* user_data);
		static void on_rotation_submitted(const vec3f_t& value, void* user_data);
		static void on_scale_changed(const vec3f_t& value, void* user_data);
		static void on_scale_submitted(const vec3f_t& value, void* user_data);

	private:
		editor_widget_text_id_t _name_input		= {};
		editor_vec3_field_t		_position_field = {};
		editor_vec3_field_t		_rotation_field = {};
		editor_vec3_field_t		_scale_field	= {};
		ui::ui_context*			_ui				= nullptr;
		world_t*				_world			= nullptr;
		ui::widget_id_t			_root			= NULL_WIDGET;
		world_handle_t			_world_handle	= {};
		quat_t					_command_rot	= {};
		vec3f_t					_command_pos	= vec3f_t::zero;
		vec3f_t					_command_scale	= vec3f_t::one;
		entity_id_t				_entity			= NULL_ENTITY_ID;
		bool					_refreshing		= false;
	};
}
