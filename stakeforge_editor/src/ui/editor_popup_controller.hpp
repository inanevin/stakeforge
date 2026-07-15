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
#include "ui/widgets/editor_widget_color_wheel.hpp"
#include "ui/widgets/editor_widget_input_field.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"
#include "world/editor_world_handle.hpp"
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/vec2f.hpp>
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
	class editor_widget_thumbnail_t;

	using editor_popup_item_pressed_fn	   = void (*)(u16 id, void* user_data);
	using editor_popup_asset_pressed_fn	   = void (*)(sid_t guid, void* user_data);
	using editor_popup_entity_pressed_fn   = void (*)(entity_guid_t guid, void* user_data);
	using editor_popup_input_closed_fn	   = void (*)(const char* value, void* user_data);
	using editor_popup_closed_fn		   = void (*)(void* user_data);
	using editor_custom_popup_install_fn   = void (*)(ui::ui_context& ui, ui::widget_id_t parent, void* user_data);
	using editor_custom_popup_uninstall_fn = void (*)(ui::ui_context& ui, void* user_data);

	struct editor_popup_item_desc_t
	{
		const char* text	 = nullptr;
		u16			id		 = 0;
		bool		selected = false;
	};

	struct editor_popup_desc_t
	{
		const editor_popup_item_desc_t* items			 = nullptr;
		editor_popup_item_pressed_fn	pressed			 = nullptr;
		editor_popup_closed_fn			closed			 = nullptr;
		void*							user_data		 = nullptr;
		vec2f_t							pos				 = {};
		f32								width			 = 0.0f;
		u16								item_count		 = 0;
		bool							close_on_pressed = true;
	};

	struct editor_input_popup_desc_t
	{
		editor_popup_input_closed_fn closed		 = nullptr;
		void*						 user_data	 = nullptr;
		const char*					 text		 = nullptr;
		const char*					 placeholder = nullptr;
		vec2f_t						 pos		 = {};
		f32							 width		 = 0.0f;
	};

	struct editor_asset_popup_desc_t
	{
		editor_popup_asset_pressed_fn pressed		   = nullptr;
		editor_popup_closed_fn		  closed		   = nullptr;
		void*						  user_data		   = nullptr;
		vec2f_t						  pos			   = {};
		f32							  width			   = 0.0f;
		sid_t						  selected		   = NULL_SID;
		editor_asset_type_e			  asset_type	   = editor_asset_type_e::invalid;
		bool						  close_on_pressed = true;
	};

	struct editor_entity_popup_desc_t
	{
		editor_popup_entity_pressed_fn pressed			= nullptr;
		editor_popup_closed_fn		   closed			= nullptr;
		void*						   user_data		= nullptr;
		vec2f_t						   pos				= {};
		editor_world_handle_t		   world			= {};
		entity_guid_t				   selected			= NULL_ENTITY_GUID;
		f32							   width			= 0.0f;
		bool						   close_on_pressed = true;
	};

	struct editor_color_wheel_popup_desc_t
	{
		span_t<color_t*>				   fields		   = {};
		editor_color_wheel_edit_begin_fn   edit_begin	   = nullptr;
		editor_color_wheel_data_changed_fn on_data_changed = nullptr;
		editor_popup_closed_fn			   closed		   = nullptr;
		void*							   user_data	   = nullptr;
		vec2f_t							   pos			   = {};
	};

	struct editor_custom_popup_desc_t
	{
		editor_custom_popup_install_fn	 install   = nullptr;
		editor_custom_popup_uninstall_fn uninstall = nullptr;
		void*							 user_data = nullptr;
		vec2f_t							 pos	   = {};
	};

	class editor_popup_controller_t final
	{
	public:
		static constexpr u32 MAX_CONTROLLERS = 16;
		static constexpr u32 MAX_ITEMS		 = 16;

		editor_popup_controller_t()												   = default;
		~editor_popup_controller_t()											   = default;
		editor_popup_controller_t(const editor_popup_controller_t&)				   = delete;
		editor_popup_controller_t& operator=(const editor_popup_controller_t&)	   = delete;
		editor_popup_controller_t(editor_popup_controller_t&&) noexcept			   = default;
		editor_popup_controller_t& operator=(editor_popup_controller_t&&) noexcept = default;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void request_popup(const editor_popup_desc_t& desc);
		void request_input_popup(const editor_input_popup_desc_t& desc);
		void request_asset_popup(const editor_asset_popup_desc_t& desc);
		void request_entity_popup(const editor_entity_popup_desc_t& desc);
		void request_color_wheel_popup(const editor_color_wheel_popup_desc_t& desc);
		void request_custom_popup(const editor_custom_popup_desc_t& desc);
		void close_popup(bool notify_input = false);

		static editor_popup_controller_t* find(ui::ui_context& ui);

	private:
		enum class popup_mode_e : u8
		{
			none,
			items,
			input,
			assets,
			entities,
			color_wheel,
			custom,
		};

		enum class pending_request_e : u8
		{
			none,
			close,
			items,
			input,
			assets,
			entities,
			color_wheel,
			custom,
			asset_rows,
		};

		struct asset_popup_item_t
		{
			string_t name	   = {};
			sid_t	 guid	   = NULL_SID;
			sid_t	 thumbnail = NULL_SID;
		};

		struct asset_row_t
		{
			ui::widget_id_t			   root			   = NULL_WIDGET;
			ui::widget_id_t			   inner		   = NULL_WIDGET;
			ui::widget_id_t			   marker		   = NULL_WIDGET;
			ui::widget_id_t			   marker_icon	   = NULL_WIDGET;
			ui::widget_id_t			   thumbnail_frame = NULL_WIDGET;
			editor_widget_thumbnail_t* thumbnail	   = nullptr;
			ui::widget_id_t			   label		   = NULL_WIDGET;
			sid_t					   guid			   = NULL_SID;
		};

		void  set_visible(bool visible);
		void  refresh_rows();
		void  refresh_asset_rows();
		void  refresh_layout();
		void  collect_asset_items();
		void  collect_entity_items();
		void  filter_asset_items();
		void  destroy_asset_rows();
		void  begin_asset_scroll_to_selected();
		bool  can_mutate_ui_topology() const;
		bool  defer_request(pending_request_e request);
		void  flush_pending_request();
		sid_t get_search_selected_value() const;
		u32	  find_row(ui::widget_id_t id) const;
		u32	  find_asset_row(ui::widget_id_t id) const;
		void  activate_row(u32 row);
		void  activate_asset_row(u32 row);

		static void on_row_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_row_key(ui::input_router_t& router, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data);
		static void on_asset_row_click(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_asset_row_key(ui::input_router_t& router, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data);
		static void on_popup_outside(ui::input_router_t& router, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data);
		static void on_input_submitted(void* user_data);
		static void on_asset_search_changed(void* user_data);
		static void on_asset_list_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static void on_ui_mutation(ui::ui_context& ui, void* user_data);

	private:
		ui::ui_context*					_ui							  = nullptr;
		ui::widget_id_t					_foreground					  = NULL_WIDGET;
		ui::widget_id_t					_frame						  = NULL_WIDGET;
		ui::widget_id_t					_row_frames[MAX_ITEMS]		  = {};
		ui::widget_id_t					_row_inner_frames[MAX_ITEMS]  = {};
		ui::widget_id_t					_row_markers[MAX_ITEMS]		  = {};
		ui::widget_id_t					_row_marker_labels[MAX_ITEMS] = {};
		ui::widget_id_t					_row_labels[MAX_ITEMS]		  = {};
		ui::widget_id_t					_asset_label_row			  = NULL_WIDGET;
		ui::widget_id_t					_asset_label				  = NULL_WIDGET;
		ui::widget_id_t					_asset_search_row			  = NULL_WIDGET;
		ui::widget_id_t					_assets_frame				  = NULL_WIDGET;
		editor_popup_desc_t				_desc						  = {};
		editor_input_popup_desc_t		_input_desc					  = {};
		editor_asset_popup_desc_t		_asset_desc					  = {};
		editor_entity_popup_desc_t		_entity_desc				  = {};
		editor_color_wheel_popup_desc_t _color_wheel_desc			  = {};
		editor_custom_popup_desc_t		_custom_desc				  = {};
		editor_popup_desc_t				_pending_desc				  = {};
		editor_input_popup_desc_t		_pending_input_desc			  = {};
		editor_asset_popup_desc_t		_pending_asset_desc			  = {};
		editor_entity_popup_desc_t		_pending_entity_desc		  = {};
		editor_color_wheel_popup_desc_t _pending_color_wheel_desc	  = {};
		editor_custom_popup_desc_t		_pending_custom_desc		  = {};
		editor_input_field_t			_input						  = {};
		editor_input_field_t			_asset_search_input			  = {};
		editor_widget_color_wheel_t		_color_wheel				  = {};
		editor_scrollbar_t				_asset_scrollbar			  = {};
		editor_popup_item_desc_t		_items[MAX_ITEMS]			  = {};
		vector_t<asset_popup_item_t>	_asset_items				  = {};
		vector_t<asset_popup_item_t>	_asset_filtered_items		  = {};
		vector_t<asset_row_t>			_asset_rows					  = {};
		string_t						_input_text					  = {};
		string_t						_asset_search_text			  = {};
		color_t							_color_wheel_dummy_color	  = {};
		editor_popup_item_desc_t		_pending_items[MAX_ITEMS]	  = {};
		popup_mode_e					_mode						  = popup_mode_e::none;
		pending_request_e				_pending_request			  = pending_request_e::none;
		vec2f_t							_color_wheel_size			  = {};
		u32								_asset_scroll_target		  = 0;
		u8								_asset_scroll_pending_frames  = 0;
		bool							_pending_close_notify_input	  = false;
		bool							_visible					  = false;
	};
}
