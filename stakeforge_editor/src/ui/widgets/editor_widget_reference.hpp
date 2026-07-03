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

#include "assets/editor_asset_type.hpp"
#include "ui/widgets/editor_widgets_common.hpp"
#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/vec2i16.hpp>
#include <sfg/runtime/engine/common_engine.hpp>
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

	enum class editor_widget_reference_type_e : u8
	{
		asset,
		entity,
	};

	struct editor_widget_reference_config_t
	{
		span_t<u64*>				   fields		   = {};
		editor_widget_callbacks_t	   callbacks	   = {};
		sid_t						   selected_asset  = NULL_SID;
		entity_guid_t				   selected_entity = NULL_ENTITY_GUID;
		world_handle_t				   world		   = {};
		editor_asset_type_e			   asset_type	   = editor_asset_type_e::invalid;
		editor_widget_reference_type_e type			   = editor_widget_reference_type_e::asset;
	};

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
		void		  refresh_frame();
		void		  open_popup();
		void		  modify_reference(u64 value);

		static void on_root_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_root_key(ui::input_router_t& router, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data);
		static void on_popup_asset_pressed(sid_t guid, void* user_data);
		static void on_popup_entity_pressed(entity_guid_t guid, void* user_data);
		static bool on_payload_drop(const editor_payload_t& payload, void* user_data);
		static void on_payload_tick(const editor_payload_t& payload, const vec2i16_t& abs_mouse_pos, void* user_data);
		static void on_payload_end(const editor_payload_t& payload, void* user_data);

	private:
		ui::ui_context*					 _ui				= nullptr;
		ui::widget_id_t					 _root				= NULL_WIDGET;
		ui::widget_id_t					 _thumbnail			= NULL_WIDGET;
		ui::widget_id_t					 _label				= NULL_WIDGET;
		vector_t<u64*>					 _fields			= {};
		editor_widget_reference_config_t _config			= {};
		bool							 _accepting_payload = false;
		bool							 _mixed				= false;
	};
}
