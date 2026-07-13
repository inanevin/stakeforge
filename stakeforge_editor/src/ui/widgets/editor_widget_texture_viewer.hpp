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

#include "ui/widgets/editor_widgets_dropdown.hpp"
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg
{
	namespace ui
	{
		class ui_context;
	}

	class editor_widget_texture_viewer_t final
	{
	public:
		editor_widget_texture_viewer_t()												 = default;
		~editor_widget_texture_viewer_t()												 = default;
		editor_widget_texture_viewer_t(const editor_widget_texture_viewer_t&)			 = delete;
		editor_widget_texture_viewer_t& operator=(const editor_widget_texture_viewer_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_texture(sid_t texture_guid);
		void clear_texture();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		void			load_texture();
		void			unload_texture();
		void			rebuild_mip_dropdown();
		void			refresh_info();
		void			refresh_texture_state();
		void			append_property_row(ui::widget_id_t row);
		ui::widget_id_t append_property_value_row(const char* label);
		ui::widget_id_t append_property_control_row(const char* label);
		ui::widget_id_t make_section_label(const char* text);
		ui::widget_id_t make_value_label(ui::widget_id_t parent);
		bool			can_mutate_ui_topology() const;
		void			request_texture_refresh(sid_t texture_guid);
		void			flush_pending_ui_mutations();

		static void on_texture_display_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static void on_ui_mutation(ui::ui_context& ui, void* user_data);
		static u16	get_selected_mip(void* user_data);
		static void on_mip_selected(u16 value, void* user_data);

	private:
		editor_dropdown_t				 _mip_dropdown			  = {};
		vector_t<editor_dropdown_item_t> _mip_dropdown_items	  = {};
		vector_t<ui::widget_id_t>		 _rows					  = {};
		vector_t<ui::widget_id_t>		 _dividers				  = {};
		vector_t<ui::widget_id_t>		 _labels				  = {};
		string_t						 _asset_name			  = {};
		string_t						 _texture_size_text		  = {};
		string_t						 _is_linear_text		  = {};
		string_t						 _payload_format_text	  = {};
		string_t						 _runtime_format_text	  = {};
		string_t						 _mip_count_text		  = {};
		string_t						 _mip_label_storage[16]	  = {};
		ui::ui_context*					 _ui					  = nullptr;
		sid_t							 _texture_guid			  = 0;
		sid_t							 _pending_texture_guid	  = 0;
		ui::widget_id_t					 _root					  = NULL_WIDGET;
		ui::widget_id_t					 _top_pane				  = NULL_WIDGET;
		ui::widget_id_t					 _bottom_pane			  = NULL_WIDGET;
		ui::widget_id_t					 _texture_display		  = NULL_WIDGET;
		ui::widget_id_t					 _asset_name_value		  = NULL_WIDGET;
		ui::widget_id_t					 _texture_size_value	  = NULL_WIDGET;
		ui::widget_id_t					 _is_linear_value		  = NULL_WIDGET;
		ui::widget_id_t					 _mip_dropdown_row		  = NULL_WIDGET;
		ui::widget_id_t					 _payload_format_value	  = NULL_WIDGET;
		ui::widget_id_t					 _runtime_format_value	  = NULL_WIDGET;
		ui::widget_id_t					 _mip_count_value		  = NULL_WIDGET;
		u16								 _selected_mip			  = 0;
		u8								 _loaded_mip_count		  = 0;
		bool							 _texture_loaded		  = false;
		bool							 _texture_failed		  = false;
		bool							 _mip_dropdown_inited	  = false;
		bool							 _refresh_texture_pending = false;
	};
}
