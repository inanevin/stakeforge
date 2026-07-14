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

#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg::ui
{
	class ui_context;
}

namespace sfg
{
	class editor_widget_thumbnail_t;

	enum class editor_tooltip_mode_e : u8
	{
		invalid,
		normal,
		asset,
	};

	struct editor_tooltip_desc_t
	{
		const char* text	  = nullptr;
		f32			delay	  = 0.25f;
		f32			max_width = 260.0f;
	};

	struct editor_asset_tooltip_desc_t
	{
		const char* name	  = nullptr;
		const char* guid	  = nullptr;
		const char* type	  = nullptr;
		sid_t		thumbnail = NULL_SID;
		f32			delay	  = 0.25f;
		f32			max_width = 260.0f;
	};

	class editor_tooltip_controller_t final
	{
	public:
		static constexpr u32 MAX_CONTROLLERS = 16;

		editor_tooltip_controller_t()												   = default;
		~editor_tooltip_controller_t()												   = default;
		editor_tooltip_controller_t(const editor_tooltip_controller_t&)				   = delete;
		editor_tooltip_controller_t& operator=(const editor_tooltip_controller_t&)	   = delete;
		editor_tooltip_controller_t(editor_tooltip_controller_t&&) noexcept			   = default;
		editor_tooltip_controller_t& operator=(editor_tooltip_controller_t&&) noexcept = default;

		void init(ui::ui_context& ui);
		void uninit();
		void set_tooltip(ui::widget_id_t owner, const editor_tooltip_desc_t& desc);
		void set_asset_tooltip(ui::widget_id_t owner, const editor_asset_tooltip_desc_t& desc);
		void clear_tooltip(ui::widget_id_t owner);

		static editor_tooltip_controller_t* find(ui::ui_context& ui);

	private:
		struct tooltip_entry_t
		{
			ui::widget_id_t		  owner		  = NULL_WIDGET;
			editor_tooltip_mode_e mode		  = editor_tooltip_mode_e::normal;
			editor_tooltip_desc_t desc		  = {};
			string_t			  asset_name  = {};
			string_t			  asset_guid  = {};
			string_t			  asset_type  = {};
			sid_t				  asset_thumb = NULL_SID;
			f32					  asset_delay = 0.25f;
			f32					  asset_width = 260.0f;
		};

		void				   tick(f32 dt_seconds);
		void				   set_mode(editor_tooltip_mode_e mode);
		void				   set_visible(bool visible);
		tooltip_entry_t*	   find_entry(ui::widget_id_t owner);
		const tooltip_entry_t* find_entry(ui::widget_id_t owner) const;

		static void on_pre_layout_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);

	private:
		vector_t<tooltip_entry_t>  _entries;
		ui::ui_context*			   _ui					 = nullptr;
		ui::widget_id_t			   _foreground			 = NULL_WIDGET;
		ui::widget_id_t			   _frame				 = NULL_WIDGET;
		ui::widget_id_t			   _default_row			 = NULL_WIDGET;
		ui::widget_id_t			   _label				 = NULL_WIDGET;
		ui::widget_id_t			   _asset_name_row		 = NULL_WIDGET;
		ui::widget_id_t			   _asset_guid_row		 = NULL_WIDGET;
		ui::widget_id_t			   _asset_type_row		 = NULL_WIDGET;
		ui::widget_id_t			   _asset_thumb_row		 = NULL_WIDGET;
		ui::widget_id_t			   _asset_name_title	 = NULL_WIDGET;
		ui::widget_id_t			   _asset_name_value	 = NULL_WIDGET;
		ui::widget_id_t			   _asset_guid_title	 = NULL_WIDGET;
		ui::widget_id_t			   _asset_guid_value	 = NULL_WIDGET;
		ui::widget_id_t			   _asset_type_title	 = NULL_WIDGET;
		ui::widget_id_t			   _asset_type_value	 = NULL_WIDGET;
		editor_widget_thumbnail_t* _asset_thumbnail		 = nullptr;
		sid_t					   _asset_thumbnail_guid = NULL_SID;
		ui::widget_id_t			   _hovered_owner		 = NULL_WIDGET;
		f32						   _hover_seconds		 = 0.0f;
		editor_tooltip_mode_e	   _mode				 = editor_tooltip_mode_e::invalid;
		bool					   _visible				 = false;
	};
}
