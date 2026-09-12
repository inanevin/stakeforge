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

#include "editor_command_system.hpp"
#include "ui/panels/editor_panel.hpp"
#include "ui/widgets/editor_widget_material_editor.hpp"
#include "ui/widgets/editor_widget_audio_viewer.hpp"
#include "ui/widgets/editor_widget_curve_editor.hpp"
#include "ui/widgets/editor_widget_physical_material_editor.hpp"
#include "ui/widgets/editor_widget_texture_sampler_editor.hpp"
#include "ui/widgets/editor_widget_texture_viewer.hpp"
#include "ui/widgets/editor_widgets_scrollbar.hpp"
#include "ui/widgets/inspector/editor_widget_inspector.hpp"
#include "world/editor_world_edit_context.hpp"
#include <sfg/data/frame_vector.hpp>

namespace sfg
{
	class editor_asset_manager_t;
	struct editor_asset_deletion_listener_tag_t;

	enum class editor_panel_inspector_display_e : u8
	{
		none,
		entity,
		material,
		texture_sampler,
		curve,
		physical_material,
		audio,
		texture,
		sprite,
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
		void				   set_display_curve(span_t<const sid_t> curves);
		void				   set_display_physical_material(span_t<const sid_t> physical_materials);
		void				   set_display_audio(sid_t audio);
		void				   set_display_texture(sid_t texture);
		void				   set_display_sprite(sid_t sprite);
		void				   refresh_from_available_selection(editor_panel_inspector_source_e preferred_source);
		void				   apply_display_visibility();
		void				   save_entity_scroll_state();
		void				   restore_entity_scroll_state();
		void				   reset_scroll_state();
		bool				   collect_selected_materials(frame_vector_t<sid_t>& out_materials) const;
		bool				   collect_selected_texture_samplers(frame_vector_t<sid_t>& out_samplers) const;
		bool				   collect_selected_curves(frame_vector_t<sid_t>& out_curves) const;
		bool				   collect_selected_physical_materials(frame_vector_t<sid_t>& out_physical_materials) const;
		bool				   collect_selected_audio(sid_t& out_audio) const;
		bool				   collect_selected_texture(sid_t& out_texture) const;
		bool				   collect_selected_sprite(sid_t& out_sprite) const;
		entity_scroll_state_t* find_entity_scroll_state(entity_id_t entity);

		static void on_entity_selection_changed(editor_world_edit_context_t& context, void* user_data);
		static void on_asset_deletion(editor_asset_manager_t& asset_manager, span_t<const sid_t> asset_ids, void* user_data);
		static void on_command_system_event(editor_command_system_t& system, const editor_command_t& command, void* user_data);

	private:
		editor_widget_inspector_t								 _entity_inspector		   = {};
		editor_widget_material_editor_t							 _material_editor		   = {};
		editor_widget_texture_sampler_editor_t					 _texture_sampler_editor   = {};
		editor_widget_curve_editor_t							 _curve_editor			   = {};
		editor_widget_physical_material_editor_t				 _physical_material_editor = {};
		editor_widget_audio_viewer_t							 _audio_viewer			   = {};
		editor_widget_texture_viewer_t							 _texture_viewer		   = {};
		editor_widget_texture_viewer_t							 _sprite_viewer			   = {};
		editor_scrollbar_t										 _scrollbar				   = {};
		vector_t<entity_scroll_state_t>							 _entity_scroll_states	   = {};
		vector_t<entity_id_t>									 _display_entities		   = {};
		vector_t<sid_t>											 _material_ids			   = {};
		vector_t<sid_t>											 _texture_sampler_ids	   = {};
		vector_t<sid_t>											 _curve_ids				   = {};
		vector_t<sid_t>											 _physical_material_ids	   = {};
		sid_t													 _texture_id			   = 0;
		sid_t													 _sprite_id				   = 0;
		sid_t													 _audio_id				   = 0;
		editor_command_listener_handle_t						 _command_listener		   = {};
		pool_handle_t<u32, editor_asset_deletion_listener_tag_t> _asset_deletion_listener  = {};
		editor_selection_listener_handle_t						 _selection_listener	   = {};
		editor_world_handle_t									 _edit_world			   = {};
		ui::widget_id_t											 _scroll_area			   = NULL_WIDGET;
		ui::widget_id_t											 _content				   = NULL_WIDGET;
		editor_panel_inspector_display_e						 _display				   = editor_panel_inspector_display_e::none;
		editor_panel_inspector_source_e							 _last_source			   = editor_panel_inspector_source_e::none;
	};
}
