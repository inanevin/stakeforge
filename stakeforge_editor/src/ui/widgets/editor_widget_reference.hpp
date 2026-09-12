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

#include "assets/editor_asset_type.hpp"
#include "ui/widgets/editor_widgets_common.hpp"
#include "world/editor_world_handle.hpp"
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/ui/ui_common.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg::ui
{
	class input_router_t;
	class ui_context;
	struct key_event_t;
	enum class mouse_button_e : u8;
}

namespace sfg
{
	struct editor_payload_t;
	class editor_widget_thumbnail_t;

	enum class editor_widget_reference_type_e : u8
	{
		asset,
		entity,
	};

	struct editor_widget_reference_config_t
	{
		editor_widget_callbacks_t	   callbacks			   = {};
		span_t<u64*>				   fields				   = {};
		sid_t						   selected_asset		   = NULL_SID;
		entity_guid_t				   selected_entity		   = NULL_ENTITY_GUID;
		editor_world_handle_t		   world				   = {};
		editor_asset_type_e			   asset_type			   = editor_asset_type_e::invalid;
		editor_widget_reference_type_e type					   = editor_widget_reference_type_e::asset;
		bool						   allow_any_resource_type = false;
	};

	struct vec2i16_t;

	class editor_widget_reference_t final
	{
	public:
		editor_widget_reference_t()											   = default;
		~editor_widget_reference_t()										   = default;
		editor_widget_reference_t(const editor_widget_reference_t&)			   = delete;
		editor_widget_reference_t& operator=(const editor_widget_reference_t&) = delete;

		void init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_reference_config_t& config);
		void uninit();
		void set_reference(const editor_widget_reference_config_t& config);

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		u64			  get_selected_value() const;
		sid_t		  get_payload_asset_guid(const editor_payload_t& payload) const;
		entity_guid_t get_payload_entity_guid(const editor_payload_t& payload) const;
		bool		  can_accept_payload(const editor_payload_t& payload, u64* out_value = nullptr) const;
		void		  set_accepting_payload(bool accepting);
		void		  refresh_title();
		void		  refresh_thumbnail();
		void		  refresh_frame();
		void		  open_popup();
		void		  modify_reference(u64 value);
		void		  show_reference();
		sid_t		  get_thumbnail_guid() const;

		static void on_root_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_root_key(ui::input_router_t& router, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data);
		static void on_show_button_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_popup_asset_pressed(sid_t guid, void* user_data);
		static void on_popup_entity_pressed(entity_guid_t guid, void* user_data);
		static bool on_payload_drop(const editor_payload_t& payload, void* user_data);
		static void on_payload_tick(const editor_payload_t& payload, const vec2i16_t& abs_mouse_pos, void* user_data);
		static void on_payload_end(const editor_payload_t& payload, void* user_data);

	private:
		editor_widget_reference_config_t _config			= {};
		vector_t<u64*>					 _fields			= {};
		ui::ui_context*					 _ui				= nullptr;
		ui::widget_id_t					 _root				= NULL_WIDGET;
		ui::widget_id_t					 _frame				= NULL_WIDGET;
		ui::widget_id_t					 _show_button		= NULL_WIDGET;
		ui::widget_id_t					 _thumbnail			= NULL_WIDGET;
		editor_widget_thumbnail_t*		 _thumbnail_widget	= nullptr;
		ui::widget_id_t					 _label				= NULL_WIDGET;
		bool							 _accepting_payload = false;
		bool							 _mixed				= false;
	};
}
