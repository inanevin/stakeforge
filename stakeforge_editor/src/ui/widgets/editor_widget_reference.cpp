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

#include "ui/widgets/editor_widget_reference.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_util.hpp"
#include "assets/editor_asset_manager.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "ui/editor_payload_controller.hpp"
#include "ui/editor_popup_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/assets/editor_panel_assets.hpp"
#include "ui/panels/editor_panel_entities.hpp"
#include "editor_surface_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widget_thumbnail.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/rectf.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/world.hpp>

namespace sfg
{
	namespace
	{
		const char* find_builtin_mesh_display_name(sid_t guid)
		{
			switch (guid)
			{
			case DEFAULT_MESH_CUBE_GUID:
				return "Cube";
			case DEFAULT_MESH_SPHERE_GUID:
				return "Sphere";
			case DEFAULT_MESH_CYLINDER_GUID:
				return "Cylinder";
			case DEFAULT_MESH_CAPSULE_GUID:
				return "Capsule";
			default:
				return nullptr;
			}
		}
	}

	void editor_widget_reference_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_widget_reference_config_t& config)
	{
		_ui = &ui;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "reference");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;
		root_in.size_value		 = {1.0f, theme.item_height};
		root_in.flow			 = ui::flow_e::row;
		root_in.child_spacing	 = theme.item_spacing;

		_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_frame, "reference_frame");
		tree.attach(_root, _frame);

		ui::layout_in_t& frame_in = tree.in(_frame);
		frame_in.flags			  = ui::wf_visible | ui::wf_input | ui::wf_focusable;
		frame_in.size_mode_x	  = ui::axis_mode_e::fill;
		frame_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		frame_in.size_value		  = {1.0f, 1.0f};
		frame_in.flow			  = ui::flow_e::row;
		frame_in.child_spacing	  = theme.item_spacing;
		frame_in.child_margins	  = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};
		frame_in.child_clip_mode  = ui::clip_mode_e::cpu_rect;

		refresh_frame();
		paint.set_hover_color(_frame, theme.color_panel);
		paint.set_press_color(_frame, theme.color_frame_light);
		paint.set_focus_color(_frame, theme.color_accent0);

		ui::listener_bundle_t root_listener = {};
		root_listener.user_data				= this;
		root_listener.on_click				= on_root_click;
		root_listener.on_key				= on_root_key;
		ui.get_input().set_listener(_frame, root_listener);
		editor_payload_controller_t::get().register_listener(on_payload_drop, on_payload_tick, on_payload_end, this);

		_thumbnail = ui.allocate_widget();
		ui.set_widget_debug_name(_thumbnail, "reference_thumbnail");
		tree.attach(_frame, _thumbnail);

		ui::layout_in_t& thumbnail_in = tree.in(_thumbnail);
		thumbnail_in.flags			  = 0;
		thumbnail_in.size_mode_x	  = ui::axis_mode_e::copy_other;
		thumbnail_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		thumbnail_in.size_value		  = {1.0f, 0.75f};
		thumbnail_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		thumbnail_in.pos_value.y	  = 0.5f;
		thumbnail_in.anchor_y		  = ui::anchor_e::center;

		ui::vg_rect_paint_t thumbnail_rect = {};
		thumbnail_rect.fill_color_a		   = theme.color_frame;
		thumbnail_rect.fill_color_b		   = theme.color_frame;
		thumbnail_rect.rounding			   = theme.item_rounding;
		paint.set_rect(_thumbnail, thumbnail_rect);

		_thumbnail_widget = new editor_widget_thumbnail_t();
		_thumbnail_widget->init(ui, _thumbnail, {});

		_label = ui.allocate_widget();
		ui.set_widget_debug_name(_label, "reference_label");
		tree.attach(_frame, _label);

		ui::layout_in_t& label_in = tree.in(_label);
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;
		tree.draw_order(_label)	  = tree.draw_order_const(_frame) + 1;

		paint.set_text(_label, nullptr, 0, {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_show_button = editor_icon_widgets_t::add_naked_icon_button(ui, _root, ICON_EYE, theme.item_height * 0.75f, theme.color_text1, theme.color_accent1, theme.color_accent1_dim, theme.color_text_disabled);
		ui.set_widget_debug_name(_show_button, "reference_show_button");
		ui::listener_bundle_t show_listener = {};
		show_listener.user_data				= this;
		show_listener.on_click				= on_show_button_click;
		ui.get_input().set_listener(_show_button, show_listener);

		set_reference(config);
	}

	void editor_widget_reference_t::uninit()
	{
		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		SFG_ASSERT(popup != nullptr);
		popup->close_popup();
		editor_payload_controller_t::get().unregister_listener(this);

		_thumbnail_widget->uninit();
		delete _thumbnail_widget;
		_ui->deallocate_widget(_root);
		_ui				  = nullptr;
		_root			  = NULL_WIDGET;
		_frame			  = NULL_WIDGET;
		_show_button	  = NULL_WIDGET;
		_thumbnail		  = NULL_WIDGET;
		_thumbnail_widget = nullptr;
		_label			  = NULL_WIDGET;
		_fields.resize(0);
		_config			   = {};
		_accepting_payload = false;
		_mixed			   = false;
	}

	void editor_widget_reference_t::set_reference(const editor_widget_reference_config_t& config)
	{
		SFG_ASSERT(config.fields.size > 0);
		SFG_ASSERT(config.fields.data != nullptr);
		for (size_t i = 0; i < config.fields.size; ++i)
			SFG_ASSERT(config.fields.data[i] != nullptr);
		SFG_ASSERT(config.type != editor_widget_reference_type_e::asset || config.asset_type != editor_asset_type_e::invalid);
		SFG_ASSERT(config.type != editor_widget_reference_type_e::entity || !config.world.is_null());

		if (config.fields.data != _fields.data())
			_fields.assign(config.fields.data, config.fields.data + config.fields.size);
		_config		   = config;
		_config.fields = {.data = _fields.data(), .size = _fields.size()};
		_mixed		   = false;

		const u64 value = *_config.fields.data[0];
		for (size_t i = 1; i < _config.fields.size; ++i)
		{
			if (*_config.fields.data[i] != value)
			{
				_mixed = true;
				break;
			}
		}

		if (!_mixed)
		{
			if (_config.type == editor_widget_reference_type_e::asset)
				_config.selected_asset = value;
			else
				_config.selected_entity = value;
		}

		const bool thumbnail_visible		 = _config.type == editor_widget_reference_type_e::asset;
		_ui->get_tree().in(_thumbnail).flags = thumbnail_visible ? ui::wf_visible : 0;
		_thumbnail_widget->set_visible(thumbnail_visible);

		refresh_thumbnail();
		refresh_title();
	}

	u64 editor_widget_reference_t::get_selected_value() const
	{
		SFG_ASSERT(_config.fields.size > 0);
		SFG_ASSERT(_config.fields.data != nullptr);
		if (_mixed)
			return _config.type == editor_widget_reference_type_e::asset ? _config.selected_asset : _config.selected_entity;
		return *_config.fields.data[0];
	}

	sid_t editor_widget_reference_t::get_payload_asset_guid(const editor_payload_t& payload) const
	{
		if (payload.type != editor_payload_type_e::asset)
			return NULL_SID;
		SFG_ASSERT(payload.user_ptr != nullptr);

		const editor_asset_node_handle_t payload_node = *static_cast<editor_asset_node_handle_t*>(payload.user_ptr);
		const editor_asset_tree_t&		 tree		  = editor_asset_manager_t::get().get_asset_tree();
		if (payload_node.is_null() || !tree.is_valid(payload_node))
			return NULL_SID;

		const editor_asset_node_t& node = tree.value(payload_node);
		if (node.type != editor_asset_node_type_e::asset)
			return NULL_SID;

		const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(node.asset_id);
		if (asset == nullptr || asset->asset_type != _config.asset_type)
			return NULL_SID;

		return asset->guid;
	}

	entity_guid_t editor_widget_reference_t::get_payload_entity_guid(const editor_payload_t& payload) const
	{
		if (payload.type != editor_payload_type_e::entity)
			return NULL_ENTITY_GUID;
		SFG_ASSERT(payload.user_ptr != nullptr);

		const editor_entity_payload_t& entity_payload = *static_cast<const editor_entity_payload_t*>(payload.user_ptr);
		if (!(entity_payload.world == _config.world))
			return NULL_ENTITY_GUID;

		const world_t& world = editor_world_controller_t::get().get_editor_world(_config.world)->get_world();
		if (entity_payload.entity == NULL_ENTITY_ID || !world.is_alive(entity_payload.entity))
			return NULL_ENTITY_GUID;

		return world.get_entity_guid(entity_payload.entity);
	}

	bool editor_widget_reference_t::can_accept_payload(const editor_payload_t& payload, u64* out_value) const
	{
		if (_config.type == editor_widget_reference_type_e::asset)
		{
			const sid_t guid = get_payload_asset_guid(payload);
			if (out_value != nullptr)
				*out_value = guid;
			return guid != NULL_SID;
		}

		const entity_guid_t guid = get_payload_entity_guid(payload);
		if (out_value != nullptr)
			*out_value = guid;
		return guid != NULL_ENTITY_GUID;
	}

	void editor_widget_reference_t::set_accepting_payload(bool accepting)
	{
		if (_accepting_payload == accepting)
			return;

		_accepting_payload = accepting;
		refresh_frame();
	}

	void editor_widget_reference_t::refresh_title()
	{
		const editor_theme_t& theme		 = editor_theme_t::get();
		const char*			  label		 = "None";
		vec4f_t				  text_color = theme.color_text0;

		if (_mixed)
		{
			label	   = "Mixed";
			text_color = theme.color_accent_warn;
		}
		else if (_config.type == editor_widget_reference_type_e::asset)
		{
			const sid_t selected = get_selected_value();
			if (selected != NULL_SID && selected != 0)
			{
				label = editor_asset_util_t::find_asset_display_name(selected);
				if (label == nullptr && _config.asset_type == editor_asset_type_e::mesh)
					label = find_builtin_mesh_display_name(selected);
				if (label == nullptr)
				{
					label	   = "Missing";
					text_color = theme.color_accent_err;
				}
			}
		}
		else
		{
			const entity_guid_t selected = get_selected_value();
			if (selected != NULL_ENTITY_GUID)
			{
				const world_t&	  world	 = editor_world_controller_t::get().get_editor_world(_config.world)->get_world();
				const entity_id_t entity = world.get_entity_from_guid(selected);
				if (entity != NULL_ENTITY_ID && world.is_alive(entity))
				{
					const char* name = world.get_entity_name(entity);
					label			 = name != nullptr ? name : "Entity";
				}
				else
				{
					label	   = "Missing";
					text_color = theme.color_accent_err;
				}
			}
		}

		_ui->set_widget_text(_label, label);
		_ui->get_paint().set_text(
			_label, _ui->widget_text(_label), _ui->widget_text_len(_label), {.font = theme.font_default, .color = text_color, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
	}

	void editor_widget_reference_t::refresh_thumbnail()
	{
		_thumbnail_widget->set_thumbnail(get_thumbnail_guid());
	}

	void editor_widget_reference_t::refresh_frame()
	{
		const editor_theme_t& theme = editor_theme_t::get();
		ui::vg_rect_paint_t	  rect	= {};
		rect.fill_color_a			= theme.color_frame;
		rect.fill_color_b			= theme.color_frame;
		rect.rounding				= theme.item_rounding;
		rect.outline_color			= _accepting_payload ? theme.color_accent1 : theme.color_panel_light;
		rect.outline_thickness		= theme.outline_thickness;
		_ui->get_paint().set_rect(_frame, rect);
	}

	void editor_widget_reference_t::open_popup()
	{
		editor_popup_controller_t* popup = editor_popup_controller_t::find(*_ui);
		SFG_ASSERT(popup != nullptr);

		const editor_theme_t&	theme	 = editor_theme_t::get();
		const ui::layout_out_t& root_out = _ui->get_tree().out(_frame);
		if (_config.type == editor_widget_reference_type_e::asset)
		{
			editor_asset_popup_desc_t desc = {};
			desc.asset_type				   = _config.asset_type;
			desc.pos					   = {root_out.pos.x, root_out.pos.y + root_out.size.y + theme.item_spacing};
			desc.width					   = root_out.size.x;
			desc.pressed				   = on_popup_asset_pressed;
			desc.user_data				   = this;
			desc.selected				   = get_selected_value();
			popup->request_asset_popup(desc);
			return;
		}

		editor_entity_popup_desc_t desc = {};
		desc.world						= _config.world;
		desc.pos						= {root_out.pos.x, root_out.pos.y + root_out.size.y + theme.item_spacing};
		desc.width						= root_out.size.x;
		desc.pressed					= on_popup_entity_pressed;
		desc.user_data					= this;
		desc.selected					= get_selected_value();
		popup->request_entity_popup(desc);
	}

	void editor_widget_reference_t::modify_reference(u64 value)
	{
		SFG_ASSERT(_config.fields.size > 0);
		SFG_ASSERT(_config.fields.data != nullptr);
		for (size_t i = 0; i < _config.fields.size; ++i)
			*_config.fields.data[i] = value;

		_mixed = false;
		if (_config.type == editor_widget_reference_type_e::asset)
			_config.selected_asset = value;
		else
			_config.selected_entity = value;
		refresh_thumbnail();
		refresh_title();
		if (_config.callbacks.edited != nullptr)
			_config.callbacks.edited(_config.callbacks.user_data);
	}

	void editor_widget_reference_t::show_reference()
	{
		if (_mixed)
			return;

		if (_config.type == editor_widget_reference_type_e::asset)
		{
			const sid_t guid = get_selected_value();
			if (guid == NULL_SID)
				return;

			editor_panel_t* panel = editor_surface_controller_t::get().show_panel(editor_panel_type_e::assets);
			if (panel != nullptr)
				static_cast<editor_panel_assets_t*>(panel)->show_asset(guid);
			return;
		}

		const entity_guid_t guid = get_selected_value();
		if (guid == NULL_ENTITY_GUID)
			return;

		editor_panel_t* panel = editor_surface_controller_t::get().show_panel(editor_panel_type_e::entities);
		if (panel != nullptr)
		{
			editor_panel_entities_t* entities_panel = static_cast<editor_panel_entities_t*>(panel);
			entities_panel->set_edit_world(_config.world);
			entities_panel->show_entity(guid);
		}
	}

	sid_t editor_widget_reference_t::get_thumbnail_guid() const
	{
		if (_mixed || _config.type != editor_widget_reference_type_e::asset)
			return NULL_SID;

		const sid_t selected = get_selected_value();
		if (selected == NULL_SID || selected == 0)
			return NULL_SID;

		const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(selected);
		return asset != nullptr ? asset->thumbnail_guid : NULL_SID;
	}

	void editor_widget_reference_t::on_root_click(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		static_cast<editor_widget_reference_t*>(user_data)->open_popup();
	}

	void editor_widget_reference_t::on_root_key(ui::input_router_t&, ui::widget_id_t, const ui::key_event_t& ev, void* user_data)
	{
		if (ev.action != ui::key_action_e::press || ev.key != static_cast<u16>(input_code::key_return))
			return;

		static_cast<editor_widget_reference_t*>(user_data)->open_popup();
	}

	void editor_widget_reference_t::on_show_button_click(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		static_cast<editor_widget_reference_t*>(user_data)->show_reference();
	}

	void editor_widget_reference_t::on_popup_asset_pressed(sid_t guid, void* user_data)
	{
		editor_widget_reference_t& reference = *static_cast<editor_widget_reference_t*>(user_data);
		if (reference._config.callbacks.edit_begin != nullptr)
			reference._config.callbacks.edit_begin(reference._config.callbacks.user_data);
		reference.modify_reference(guid);
		if (reference._config.callbacks.edit_submitted != nullptr)
			reference._config.callbacks.edit_submitted(reference._config.callbacks.user_data);
	}

	void editor_widget_reference_t::on_popup_entity_pressed(entity_guid_t guid, void* user_data)
	{
		editor_widget_reference_t& reference = *static_cast<editor_widget_reference_t*>(user_data);
		if (reference._config.callbacks.edit_begin != nullptr)
			reference._config.callbacks.edit_begin(reference._config.callbacks.user_data);
		reference.modify_reference(guid);
		if (reference._config.callbacks.edit_submitted != nullptr)
			reference._config.callbacks.edit_submitted(reference._config.callbacks.user_data);
	}

	bool editor_widget_reference_t::on_payload_drop(const editor_payload_t& payload, void* user_data)
	{
		editor_widget_reference_t& reference = *static_cast<editor_widget_reference_t*>(user_data);
		const ui::layout_out_t&	   out		 = reference._ui->get_tree().out(reference._frame);
		if (!rectf_t{out.pos.x, out.pos.y, out.size.x, out.size.y}.contains(reference._ui->get_input().get_mouse_position()))
			return false;

		u64 value = 0;
		if (!reference.can_accept_payload(payload, &value))
			return false;

		if (reference._config.callbacks.edit_begin != nullptr)
			reference._config.callbacks.edit_begin(reference._config.callbacks.user_data);
		reference.modify_reference(value);
		if (reference._config.callbacks.edit_submitted != nullptr)
			reference._config.callbacks.edit_submitted(reference._config.callbacks.user_data);
		return true;
	}

	void editor_widget_reference_t::on_payload_tick(const editor_payload_t& payload, const vec2i16_t&, void* user_data)
	{
		editor_widget_reference_t& reference = *static_cast<editor_widget_reference_t*>(user_data);
		reference.set_accepting_payload(reference.can_accept_payload(payload));
	}

	void editor_widget_reference_t::on_payload_end(const editor_payload_t&, void* user_data)
	{
		static_cast<editor_widget_reference_t*>(user_data)->set_accepting_payload(false);
	}
}
