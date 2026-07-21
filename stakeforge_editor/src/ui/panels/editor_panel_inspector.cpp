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
#include "ui/panels/editor_panel_inspector.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_manager.hpp"
#include "editor_surface_controller.hpp"
#include "editor_world_controller.hpp"
#include "ui/panels/assets/editor_panel_assets.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "world/editor_world.hpp"
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	editor_panel_inspector_t::editor_panel_inspector_t()
	{
		set_type(editor_panel_type_e::inspector);
		set_title(editor_panel_type_to_string(editor_panel_type_e::inspector));
		set_icon(ICON_GLASSES);
	}

	void editor_panel_inspector_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);
		_command_listener		 = editor_command_system_t::get().add_listener(on_command_system_event, this);
		ui::layout_in_t& root_in = ui.get_tree().in(_root);
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;

		ui::layout_tree_t& tree = ui.get_tree();
		_scroll_area			= ui.allocate_widget();
		ui.set_widget_debug_name(_scroll_area, "inspector_scroll_area");
		tree.attach(_root, _scroll_area);

		ui::layout_in_t& scroll_area_in = tree.in(_scroll_area);
		scroll_area_in.flags			= ui::wf_visible | ui::wf_input | ui::wf_scroll_y;
		scroll_area_in.child_clip_mode	= ui::clip_mode_e::scissor_rect;
		scroll_area_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		scroll_area_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		scroll_area_in.size_value		= {1.0f, 1.0f};

		_content = ui.allocate_widget();
		ui.set_widget_debug_name(_content, "inspector_content");
		tree.attach(_scroll_area, _content);
		tree.draw_order(_content) = tree.draw_order_const(_scroll_area) + 1;

		ui::layout_in_t& content_in = tree.in(_content);
		content_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		content_in.size_mode_y		= ui::axis_mode_e::sum_children;
		content_in.size_value		= {1.0f, 1.0f};
		content_in.flow				= ui::flow_e::column;
		content_in.child_spacing	= 0.0f;

		editor_scrollbar_config_t scrollbar_config = {};
		scrollbar_config.target					   = _scroll_area;
		scrollbar_config.axes					   = editor_scrollbar_axis_y;
		_scrollbar.init(ui, scrollbar_config);

		_entity_inspector.init(ui,
							   _content,
							   {
								   .allow_prefab_blocks = true,
							   });
		_material_editor.init(ui, _content);
		_texture_sampler_editor.init(ui, _content);
		_physical_material_editor.init(ui, _content);
		_texture_viewer.init(ui, _content);
		set_edit_world(editor_world_controller_t::get().get_main_world_handle());
		refresh_from_available_selection(editor_panel_inspector_source_e::none);
	}

	void editor_panel_inspector_t::uninit()
	{
		save_entity_scroll_state();

		if (!_command_listener.is_null())
			editor_command_system_t::get().remove_listener(_command_listener);
		if (!_selection_listener.is_null())
			editor_world_controller_t::get().get_editor_world(_edit_world)->get_edit_context().remove_selection_listener(_selection_listener);
		_texture_viewer.uninit();
		_physical_material_editor.uninit();
		_texture_sampler_editor.uninit();
		_material_editor.uninit();
		_entity_inspector.uninit();
		_scrollbar.uninit();
		_ui->deallocate_widget(_content);
		_ui->deallocate_widget(_scroll_area);
		_display_entities.resize(0);
		_material_ids.resize(0);
		_texture_sampler_ids.resize(0);
		_physical_material_ids.resize(0);
		_texture_id				= 0;
		_command_listener		= {};
		_selection_listener		= {};
		_edit_world				= {};
		_scroll_area			= NULL_WIDGET;
		_content				= NULL_WIDGET;
		_pending_scroll_y		= 0.0f;
		_display				= editor_panel_inspector_display_e::none;
		_scroll_restore_pending = false;
		editor_panel_t::uninit();
	}

	void editor_panel_inspector_t::set_display_none()
	{
		save_entity_scroll_state();
		_display_entities.resize(0);
		_entity_inspector.set_display_entity(span_t<const entity_id_t>{});
		_texture_viewer.clear_texture();
		_display = editor_panel_inspector_display_e::none;
		apply_display_visibility();
		reset_scroll_state();
	}

	void editor_panel_inspector_t::set_display_entity(entity_id_t entity)
	{
		save_entity_scroll_state();
		_display_entities.assign(&entity, &entity + 1);
		_texture_viewer.clear_texture();
		_entity_inspector.set_display_entity(entity);
		_display	 = editor_panel_inspector_display_e::entity;
		_last_source = editor_panel_inspector_source_e::entity;
		apply_display_visibility();
		restore_entity_scroll_state();
	}

	void editor_panel_inspector_t::set_display_entity(span_t<const entity_id_t> entities)
	{
		save_entity_scroll_state();
		_display_entities.assign(entities.data, entities.data + entities.size);
		_texture_viewer.clear_texture();
		_entity_inspector.set_display_entity(entities);
		_display	 = editor_panel_inspector_display_e::entity;
		_last_source = editor_panel_inspector_source_e::entity;
		apply_display_visibility();
		restore_entity_scroll_state();
	}

	void editor_panel_inspector_t::refresh_display()
	{
		if (_display == editor_panel_inspector_display_e::entity)
		{
			save_entity_scroll_state();
			_entity_inspector.refresh_display();
			restore_entity_scroll_state();
		}
		else if (_display == editor_panel_inspector_display_e::material)
			_material_editor.set_materials({.data = _material_ids.data(), .size = _material_ids.size()});
		else if (_display == editor_panel_inspector_display_e::texture_sampler)
			_texture_sampler_editor.set_texture_samplers({.data = _texture_sampler_ids.data(), .size = _texture_sampler_ids.size()});
		else if (_display == editor_panel_inspector_display_e::physical_material)
			_physical_material_editor.set_physical_materials({.data = _physical_material_ids.data(), .size = _physical_material_ids.size()});
		else if (_display == editor_panel_inspector_display_e::texture)
			_texture_viewer.set_texture(_texture_id);
	}

	void editor_panel_inspector_t::refresh_from_selection()
	{
		refresh_from_available_selection(editor_panel_inspector_source_e::entity);
	}

	void editor_panel_inspector_t::refresh_from_assets()
	{
		refresh_from_available_selection(editor_panel_inspector_source_e::asset);
	}

	void editor_panel_inspector_t::refresh_component_reflection(sid_t component_type)
	{
		_entity_inspector.refresh_component_reflection(component_type);
	}

	void editor_panel_inspector_t::set_edit_world(editor_world_handle_t world)
	{
		if (_edit_world != world && !_selection_listener.is_null())
		{
			editor_world_controller_t::get().get_editor_world(_edit_world)->get_edit_context().remove_selection_listener(_selection_listener);
			_selection_listener = {};
		}

		_edit_world = world;
		_entity_inspector.set_edit_world(world);
		if (!_edit_world.is_null() && _selection_listener.is_null())
			_selection_listener = editor_world_controller_t::get().get_editor_world(_edit_world)->get_edit_context().add_selection_listener(on_entity_selection_changed, this);
	}

	void editor_panel_inspector_t::on_asset_selection_changed()
	{
		refresh_from_available_selection(editor_panel_inspector_source_e::asset);
	}

	void editor_panel_inspector_t::set_display_material(span_t<const sid_t> materials)
	{
		save_entity_scroll_state();
		_display_entities.resize(0);
		_texture_viewer.clear_texture();
		_material_ids.assign(materials.data, materials.data + materials.size);
		_material_editor.set_materials({.data = _material_ids.data(), .size = _material_ids.size()});
		_display	 = editor_panel_inspector_display_e::material;
		_last_source = editor_panel_inspector_source_e::asset;
		apply_display_visibility();
		reset_scroll_state();
	}

	void editor_panel_inspector_t::set_display_texture_sampler(span_t<const sid_t> samplers)
	{
		save_entity_scroll_state();
		_display_entities.resize(0);
		_texture_viewer.clear_texture();
		_texture_sampler_ids.assign(samplers.data, samplers.data + samplers.size);
		_texture_sampler_editor.set_texture_samplers({.data = _texture_sampler_ids.data(), .size = _texture_sampler_ids.size()});
		_display	 = editor_panel_inspector_display_e::texture_sampler;
		_last_source = editor_panel_inspector_source_e::asset;
		apply_display_visibility();
		reset_scroll_state();
	}

	void editor_panel_inspector_t::set_display_physical_material(span_t<const sid_t> physical_materials)
	{
		save_entity_scroll_state();
		_display_entities.resize(0);
		_texture_viewer.clear_texture();
		_physical_material_ids.assign(physical_materials.data, physical_materials.data + physical_materials.size);
		_physical_material_editor.set_physical_materials({.data = _physical_material_ids.data(), .size = _physical_material_ids.size()});
		_display	 = editor_panel_inspector_display_e::physical_material;
		_last_source = editor_panel_inspector_source_e::asset;
		apply_display_visibility();
		reset_scroll_state();
	}

	void editor_panel_inspector_t::set_display_texture(sid_t texture)
	{
		save_entity_scroll_state();
		_display_entities.resize(0);
		_texture_id = texture;
		_texture_viewer.set_texture(texture);
		_display	 = editor_panel_inspector_display_e::texture;
		_last_source = editor_panel_inspector_source_e::asset;
		apply_display_visibility();
		reset_scroll_state();
	}

	void editor_panel_inspector_t::refresh_from_available_selection(editor_panel_inspector_source_e preferred_source)
	{
		const editor_world_edit_context_t* edit_context		   = !_edit_world.is_null() ? &editor_world_controller_t::get().get_editor_world(_edit_world)->get_edit_context() : nullptr;
		const span_t<const entity_id_t>	   selected_entities   = edit_context != nullptr ? edit_context->get_selected_entities() : span_t<const entity_id_t>{};
		const editor_panel_t*			   assets_panel		   = editor_surface_controller_t::get().find_panel(editor_panel_type_e::assets);
		const bool						   has_asset_selection = assets_panel != nullptr && !static_cast<const editor_panel_assets_t*>(assets_panel)->is_asset_selection_empty();

		vector_t<sid_t> selected_materials;
		vector_t<sid_t> selected_samplers;
		vector_t<sid_t> selected_physical_materials;
		sid_t			selected_texture	   = 0;
		const bool		has_materials		   = collect_selected_materials(selected_materials);
		const bool		has_samplers		   = collect_selected_texture_samplers(selected_samplers);
		const bool		has_physical_materials = collect_selected_physical_materials(selected_physical_materials);
		const bool		has_texture			   = collect_selected_texture(selected_texture);
		const bool		has_entities		   = edit_context != nullptr && !edit_context->get_world().is_null() && selected_entities.size != 0;

		if (preferred_source == editor_panel_inspector_source_e::asset)
		{
			if (!has_asset_selection)
				return;

			_last_source = editor_panel_inspector_source_e::asset;
			if (has_materials)
				set_display_material({.data = selected_materials.data(), .size = selected_materials.size()});
			else if (has_samplers)
				set_display_texture_sampler({.data = selected_samplers.data(), .size = selected_samplers.size()});
			else if (has_physical_materials)
				set_display_physical_material({.data = selected_physical_materials.data(), .size = selected_physical_materials.size()});
			else if (has_texture)
				set_display_texture(selected_texture);
			else
				set_display_none();
			return;
		}

		if (preferred_source == editor_panel_inspector_source_e::entity)
		{
			_last_source = editor_panel_inspector_source_e::entity;
			if (has_entities)
				set_display_entity(selected_entities);
			else
				set_display_none();
			return;
		}

		if (_last_source == editor_panel_inspector_source_e::asset)
		{
			if (has_materials)
				set_display_material({.data = selected_materials.data(), .size = selected_materials.size()});
			else if (has_samplers)
				set_display_texture_sampler({.data = selected_samplers.data(), .size = selected_samplers.size()});
			else if (has_physical_materials)
				set_display_physical_material({.data = selected_physical_materials.data(), .size = selected_physical_materials.size()});
			else if (has_texture)
				set_display_texture(selected_texture);
			else
				set_display_none();
			return;
		}

		if (_last_source == editor_panel_inspector_source_e::entity)
		{
			if (has_entities)
				set_display_entity(selected_entities);
			else
				set_display_none();
			return;
		}

		if (has_entities)
		{
			set_display_entity(selected_entities);
			return;
		}
		if (has_materials)
		{
			set_display_material({.data = selected_materials.data(), .size = selected_materials.size()});
			return;
		}
		if (has_samplers)
		{
			set_display_texture_sampler({.data = selected_samplers.data(), .size = selected_samplers.size()});
			return;
		}
		if (has_physical_materials)
		{
			set_display_physical_material({.data = selected_physical_materials.data(), .size = selected_physical_materials.size()});
			return;
		}
		if (has_texture)
		{
			set_display_texture(selected_texture);
			return;
		}
		set_display_none();
	}

	void editor_panel_inspector_t::apply_display_visibility()
	{
		ui::layout_tree_t& tree = _ui->get_tree();
		tree.set_visible(_entity_inspector.get_root(), _display == editor_panel_inspector_display_e::entity, false);
		tree.set_visible(_material_editor.get_root(), _display == editor_panel_inspector_display_e::material, false);
		tree.set_visible(_texture_sampler_editor.get_root(), _display == editor_panel_inspector_display_e::texture_sampler, false);
		tree.set_visible(_physical_material_editor.get_root(), _display == editor_panel_inspector_display_e::physical_material, false);
		tree.set_visible(_texture_viewer.get_root(), _display == editor_panel_inspector_display_e::texture, false);
	}

	void editor_panel_inspector_t::save_entity_scroll_state()
	{
		if (_display != editor_panel_inspector_display_e::entity || _display_entities.size() != 1)
			return;

		entity_scroll_state_t* state = find_entity_scroll_state(_display_entities[0]);
		if (state == nullptr)
		{
			_entity_scroll_states.push_back({.entity = _display_entities[0]});
			state = &_entity_scroll_states.back();
		}
		state->scroll_y = _ui->get_tree().in_const(_scroll_area).scroll_offset.y;
	}

	void editor_panel_inspector_t::restore_entity_scroll_state()
	{
		if (_display != editor_panel_inspector_display_e::entity || _display_entities.size() != 1)
		{
			reset_scroll_state();
			return;
		}

		const entity_scroll_state_t* state = find_entity_scroll_state(_display_entities[0]);
		_pending_scroll_y				   = state != nullptr ? state->scroll_y : 0.0f;
		_scroll_restore_pending			   = true;
		_ui->set_post_layout_tick(_scroll_area, on_scroll_restore_tick, this);
	}

	void editor_panel_inspector_t::reset_scroll_state()
	{
		_scroll_restore_pending = false;
		_pending_scroll_y		= 0.0f;
		_ui->clear_post_layout_tick(_scroll_area);
		_scrollbar.set_scroll_y_immediate(0.0f);
	}

	void editor_panel_inspector_t::apply_pending_scroll_restore()
	{
		if (!_scroll_restore_pending)
		{
			_ui->clear_post_layout_tick(_scroll_area);
			return;
		}

		_scrollbar.set_scroll_y_immediate(_pending_scroll_y);
		_ui->request_post_layout_solve();
		_scroll_restore_pending = false;
		_ui->clear_post_layout_tick(_scroll_area);
	}

	bool editor_panel_inspector_t::collect_selected_materials(vector_t<sid_t>& out_materials) const
	{
		out_materials.resize(0);
		editor_panel_t* panel = editor_surface_controller_t::get().find_panel(editor_panel_type_e::assets);
		if (panel == nullptr)
			return false;

		static_cast<editor_panel_assets_t*>(panel)->collect_selected_asset_guids(out_materials);
		if (out_materials.empty())
			return false;

		const editor_asset_manager_t& assets = editor_asset_manager_t::get();
		for (sid_t guid : out_materials)
		{
			const editor_asset_t* asset = assets.find_asset(guid);
			if (asset == nullptr || asset->asset_type != editor_asset_type_e::material)
			{
				out_materials.resize(0);
				return false;
			}
		}
		return true;
	}

	bool editor_panel_inspector_t::collect_selected_texture_samplers(vector_t<sid_t>& out_samplers) const
	{
		out_samplers.resize(0);
		editor_panel_t* panel = editor_surface_controller_t::get().find_panel(editor_panel_type_e::assets);
		if (panel == nullptr)
			return false;

		static_cast<editor_panel_assets_t*>(panel)->collect_selected_asset_guids(out_samplers);
		if (out_samplers.empty())
			return false;

		const editor_asset_manager_t& assets = editor_asset_manager_t::get();
		for (sid_t guid : out_samplers)
		{
			const editor_asset_t* asset = assets.find_asset(guid);
			if (asset == nullptr || asset->asset_type != editor_asset_type_e::texture_sampler)
			{
				out_samplers.resize(0);
				return false;
			}
		}
		return true;
	}

	bool editor_panel_inspector_t::collect_selected_physical_materials(vector_t<sid_t>& out_physical_materials) const
	{
		out_physical_materials.resize(0);
		editor_panel_t* panel = editor_surface_controller_t::get().find_panel(editor_panel_type_e::assets);
		if (panel == nullptr)
			return false;

		static_cast<editor_panel_assets_t*>(panel)->collect_selected_asset_guids(out_physical_materials);
		if (out_physical_materials.empty())
			return false;

		const editor_asset_manager_t& assets = editor_asset_manager_t::get();
		for (sid_t guid : out_physical_materials)
		{
			const editor_asset_t* asset = assets.find_asset(guid);
			if (asset == nullptr || asset->asset_type != editor_asset_type_e::physical_material)
			{
				out_physical_materials.resize(0);
				return false;
			}
		}
		return true;
	}

	bool editor_panel_inspector_t::collect_selected_texture(sid_t& out_texture) const
	{
		out_texture			  = 0;
		editor_panel_t* panel = editor_surface_controller_t::get().find_panel(editor_panel_type_e::assets);
		if (panel == nullptr)
			return false;

		vector_t<sid_t> selected_textures;
		static_cast<editor_panel_assets_t*>(panel)->collect_selected_asset_guids(selected_textures);
		if (selected_textures.size() != 1)
			return false;

		const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(selected_textures[0]);
		if (asset == nullptr || asset->asset_type != editor_asset_type_e::texture)
			return false;

		out_texture = selected_textures[0];
		return true;
	}

	editor_panel_inspector_t::entity_scroll_state_t* editor_panel_inspector_t::find_entity_scroll_state(entity_id_t entity)
	{
		for (entity_scroll_state_t& state : _entity_scroll_states)
		{
			if (state.entity == entity)
				return &state;
		}
		return nullptr;
	}

	void editor_panel_inspector_t::on_entity_selection_changed(editor_world_edit_context_t&, void* user_data)
	{
		static_cast<editor_panel_inspector_t*>(user_data)->refresh_from_available_selection(editor_panel_inspector_source_e::entity);
	}

	void editor_panel_inspector_t::on_scroll_restore_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		static_cast<editor_panel_inspector_t*>(user_data)->apply_pending_scroll_restore();
	}

	void editor_panel_inspector_t::on_command_system_event(editor_command_system_t&, const editor_command_t& command, void* user_data)
	{
		editor_panel_inspector_t& panel = *static_cast<editor_panel_inspector_t*>(user_data);
		switch (command.type)
		{
		case editor_command_type_e::material_edit:
		case editor_command_type_e::shader_edit:
			if (panel._display == editor_panel_inspector_display_e::material)
				panel.refresh_display();
			break;
		case editor_command_type_e::texture_sampler_edit:
			if (panel._display == editor_panel_inspector_display_e::texture_sampler)
				panel.refresh_display();
			break;
		case editor_command_type_e::physical_material_edit:
			if (panel._display == editor_panel_inspector_display_e::physical_material)
				panel.refresh_display();
			break;
		default:
			break;
		}
	}
}
