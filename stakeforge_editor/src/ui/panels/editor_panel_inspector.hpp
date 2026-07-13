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

#include "editor_command_system.hpp"
#include "ui/panels/editor_panel.hpp"
#include "ui/widgets/editor_widget_material_editor.hpp"
#include "ui/widgets/editor_widget_texture_sampler_editor.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"
#include "ui/widgets/inspector/editor_widget_inspector.hpp"
#include "world/editor_world_edit_context.hpp"

namespace sfg
{
	enum class editor_panel_inspector_display_e : u8
	{
		none,
		entity,
		material,
		texture_sampler,
	};

	enum class editor_panel_inspector_source_e : u8
	{
		none,
		entity,
		asset,
	};

	class editor_panel_inspector_t final : public editor_panel_t
	{
	public:
		editor_panel_inspector_t();
		~editor_panel_inspector_t() override								 = default;
		editor_panel_inspector_t(const editor_panel_inspector_t&)			 = delete;
		editor_panel_inspector_t& operator=(const editor_panel_inspector_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_display_none();
		void set_display_entity(entity_id_t entity);
		void set_display_entity(span_t<const entity_id_t> entities);
		void refresh_display();
		void refresh_from_selection();
		void refresh_from_assets();
		void refresh_component_reflection(sid_t component_type);
		void set_edit_world(editor_world_handle_t world);
		void on_asset_selection_changed();

	private:
		struct entity_scroll_state_t
		{
			entity_id_t entity	 = {};
			f32			scroll_y = 0.0f;
		};

		void				   set_display_material(span_t<const sid_t> materials);
		void				   set_display_texture_sampler(span_t<const sid_t> samplers);
		void				   refresh_from_available_selection(editor_panel_inspector_source_e preferred_source);
		void				   apply_display_visibility();
		void				   save_entity_scroll_state();
		void				   restore_entity_scroll_state();
		void				   reset_scroll_state();
		void				   apply_pending_scroll_restore();
		bool				   collect_selected_materials(vector_t<sid_t>& out_materials) const;
		bool				   collect_selected_texture_samplers(vector_t<sid_t>& out_samplers) const;
		entity_scroll_state_t* find_entity_scroll_state(entity_id_t entity);

		static void on_entity_selection_changed(editor_world_edit_context_t& context, void* user_data);
		static void on_scroll_restore_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static void on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data);

	private:
		editor_widget_inspector_t			   _entity_inspector	   = {};
		editor_widget_material_editor_t		   _material_editor		   = {};
		editor_widget_texture_sampler_editor_t _texture_sampler_editor = {};
		editor_scrollbar_t					   _scrollbar			   = {};
		vector_t<entity_scroll_state_t>		   _entity_scroll_states   = {};
		vector_t<entity_id_t>				   _display_entities	   = {};
		vector_t<sid_t>						   _material_ids		   = {};
		vector_t<sid_t>						   _texture_sampler_ids	   = {};
		editor_command_listener_handle_t	   _command_listener	   = {};
		editor_selection_listener_handle_t	   _selection_listener	   = {};
		editor_world_handle_t				   _edit_world			   = {};
		ui::widget_id_t						   _scroll_area			   = NULL_WIDGET;
		ui::widget_id_t						   _content				   = NULL_WIDGET;
		f32									   _pending_scroll_y	   = 0.0f;
		editor_panel_inspector_display_e	   _display				   = editor_panel_inspector_display_e::none;
		editor_panel_inspector_source_e		   _last_source			   = editor_panel_inspector_source_e::none;
		bool								   _scroll_restore_pending = false;
	};
}
