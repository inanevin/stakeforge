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

#include "ui/widgets/editor_widget_input_field.hpp"
#include "ui/widgets/editor_widget_button.hpp"
#include "ui/widgets/editor_widgets_vec_fields.hpp"
#include "world/editor_world_handle.hpp"
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg::ui
{
	class input_router_t;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	class world_t;

	using editor_widget_entity_info_name_submitted_fn = void (*)(entity_id_t entity, void* user_data);
	using editor_widget_entity_info_break_prefab_fn	  = void (*)(void* user_data);

	struct editor_widget_entity_info_config_t
	{
		editor_widget_entity_info_break_prefab_fn break_prefab = nullptr;
		void*									  user_data	   = nullptr;
		editor_world_handle_t					  world		   = {};
		bool									  is_prefab	   = false;
		bool									  block_edits  = false;
	};

	class editor_widget_entity_info_t final
	{
	public:
		editor_widget_entity_info_t()											   = default;
		~editor_widget_entity_info_t()											   = default;
		editor_widget_entity_info_t(const editor_widget_entity_info_t&)			   = delete;
		editor_widget_entity_info_t& operator=(const editor_widget_entity_info_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_entity_info_config_t& config);
		void uninit();
		void set_entity(world_t& world, entity_id_t entity);
		void set_entities(world_t& world, span_t<const entity_id_t> entities);
		void set_name_submitted_callback(editor_widget_entity_info_name_submitted_fn callback, void* user_data);
		void set_edit_callbacks(const editor_widget_callbacks_t& callbacks);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		static void on_edit_begin(void* user_data);
		static void on_name_input_submitted(void* user_data);
		static void on_position_changed(void* user_data);
		static void on_rotation_changed(void* user_data);
		static void on_scale_changed(void* user_data);
		static void on_edit_submitted(void* user_data);
		static void on_break_prefab_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_pre_layout_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);

		void refresh_controls();
		void refresh_transform_controls(bool preserve_edits);
		void begin_edit();
		void submit_edit();
		void apply_position_values();
		void apply_rotation_values();
		void apply_scale_values();
		void submit_names();

	private:
		editor_quat_field_t							_rotation_field			  = {};
		editor_vec3_field_t							_position_field			  = {};
		editor_vec3_field_t							_scale_field			  = {};
		editor_widget_button_t						_break_prefab_button	  = {};
		editor_input_field_t						_name_input				  = {};
		char										_name_fallback[64]		  = {};
		vector_t<entity_id_t>						_entities				  = {};
		ui::ui_context*								_ui						  = nullptr;
		world_t*									_world					  = nullptr;
		editor_widget_entity_info_name_submitted_fn _name_submitted_callback  = nullptr;
		editor_widget_entity_info_break_prefab_fn	_break_prefab_callback	  = nullptr;
		editor_widget_callbacks_t					_callbacks				  = {};
		void*										_name_submitted_user_data = nullptr;
		void*										_break_prefab_user_data	  = nullptr;
		ui::widget_id_t								_root					  = NULL_WIDGET;
		ui::widget_id_t								_guid_label				  = NULL_WIDGET;
		ui::widget_id_t								_prefab_frame			  = NULL_WIDGET;
		ui::widget_id_t								_prefab_label			  = NULL_WIDGET;
		ui::widget_id_t								_fields_root			  = NULL_WIDGET;
		ui::widget_id_t								_blocker				  = NULL_WIDGET;
		entity_id_t									_entity					  = NULL_ENTITY_ID;
	};
}
