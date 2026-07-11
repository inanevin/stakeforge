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
#include "ui/panels/editor_panel_texture_viewer.hpp"
#include "editor_surface_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include <sfg/math/math.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/texture.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define TEXTURE_VIEWER_PANE_SPLIT_MIN			   0.45f
#define TEXTURE_VIEWER_PANE_SPLIT_MAX			   0.85f
#define TEXTURE_VIEWER_SPLIT_BORDER_THICKNESS_MULT 2.0f
#define TEXTURE_VIEWER_TEXTURE_SHADER			   "editor/resource_pack/shaders/editor_ui_texture_mip.hlsl"_hs
#define TEXTURE_VIEWER_NOT_FOUND_TEXT			   "TEXTURE NOT FOUND"

	namespace
	{
		const char* reflected_enum_display_name(sid_t type_id, u32 value)
		{
			const reflected_type_t* type = reflection_registry_t::get().find_type(type_id);
			if (type == nullptr || value >= type->fields.end - type->fields.start)
				return "Unknown";

			const reflected_field_t* field = reflection_registry_t::get().get_field(type->fields.start + value);
			return field->display_name != nullptr ? field->display_name : field->name;
		}
	}

	editor_panel_texture_viewer_t::editor_panel_texture_viewer_t()
	{
		set_type(editor_panel_type_e::texture_viewer);
		set_title(editor_panel_type_to_string(editor_panel_type_e::texture_viewer));
		set_icon(ICON_EYE);
	}

	void editor_panel_texture_viewer_t::serialize(nlohmann::json& j) const
	{
		j				  = nlohmann::json::object();
		j["texture_guid"] = _texture_guid;
		j["asset_name"]	  = _asset_name;
		j["pane_split"]	  = _pane_split;
		j["selected_mip"] = _selected_mip;
	}

	void editor_panel_texture_viewer_t::deserialize(const nlohmann::json& j)
	{
		_texture_guid = j.value<sid_t>("texture_guid", 0);
		_asset_name	  = j.value<string_t>("asset_name", {});
		_pane_split	  = math::clamp(j.value<f32>("pane_split", _pane_split), TEXTURE_VIEWER_PANE_SPLIT_MIN, TEXTURE_VIEWER_PANE_SPLIT_MAX);
		_selected_mip = j.value<u16>("selected_mip", 0);
		refresh_title();
	}

	void editor_panel_texture_viewer_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::row;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, 0.0f, theme.margin_vertical, 0.0f};

		_left_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_left_pane, "texture_viewer_left_pane");
		tree.attach(_root, _left_pane);

		ui::layout_in_t& left_in = tree.in(_left_pane);
		left_in.flags			 = ui::wf_visible;
		left_in.flow			 = ui::flow_e::none;
		left_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};
		left_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		left_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		left_in.size_value		 = {_pane_split, 1.0f};

		ui::vg_rect_paint_t left_rect = {};
		left_rect.fill_color_a		  = theme.color_panel;
		left_rect.fill_color_b		  = theme.color_panel;
		paint.set_rect(_left_pane, left_rect);

		_texture_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_texture_frame, "texture_viewer_texture_frame");
		tree.attach(_left_pane, _texture_frame);
		tree.draw_order(_texture_frame) = tree.draw_order_const(_left_pane) + 1;

		ui::layout_in_t& frame_in = tree.in(_texture_frame);
		frame_in.flags			  = ui::wf_visible;
		frame_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		frame_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		frame_in.pos_value		  = {0.5f, 0.5f};
		frame_in.anchor_x		  = ui::anchor_e::center;
		frame_in.anchor_y		  = ui::anchor_e::center;
		frame_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		frame_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		frame_in.size_value		  = {1.0f, 1.0f};
		frame_in.child_margins	  = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};

		ui::vg_rect_paint_t frame_rect = {};
		frame_rect.fill_color_a		   = theme.color_frame;
		frame_rect.fill_color_b		   = theme.color_frame;
		paint.set_rect(_texture_frame, frame_rect);

		_texture_display = ui.allocate_widget();
		ui.set_widget_debug_name(_texture_display, "texture_viewer_texture_display");
		tree.attach(_texture_frame, _texture_display);
		tree.draw_order(_texture_display) = tree.draw_order_const(_texture_frame) + 1;

		ui::layout_in_t& texture_in = tree.in(_texture_display);
		texture_in.flags			= ui::wf_visible;
		texture_in.pos_mode_x		= ui::pos_mode_e::relative_in_parent;
		texture_in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
		texture_in.pos_value		= {0.5f, 0.5f};
		texture_in.anchor_x			= ui::anchor_e::center;
		texture_in.anchor_y			= ui::anchor_e::center;
		texture_in.size_mode_x		= ui::axis_mode_e::fixed;
		texture_in.size_mode_y		= ui::axis_mode_e::fixed;
		texture_in.size_value		= {1.0f, 1.0f};

		_texture_not_found_label = ui.allocate_widget();
		ui.set_widget_debug_name(_texture_not_found_label, "texture_viewer_texture_not_found");
		tree.attach(_texture_frame, _texture_not_found_label);
		tree.draw_order(_texture_not_found_label) = tree.draw_order_const(_texture_frame) + 1;

		ui::layout_in_t& not_found_in = tree.in(_texture_not_found_label);
		not_found_in.flags			  = 0;
		not_found_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		not_found_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		not_found_in.pos_value		  = {0.5f, 0.5f};
		not_found_in.anchor_x		  = ui::anchor_e::center;
		not_found_in.anchor_y		  = ui::anchor_e::center;
		not_found_in.size_mode_x	  = ui::axis_mode_e::fixed;
		not_found_in.size_mode_y	  = ui::axis_mode_e::fixed;
		not_found_in.size_value		  = {static_cast<f32>(sizeof(TEXTURE_VIEWER_NOT_FOUND_TEXT) - 1) * theme.text_med_title_px_size * 0.7f, theme.text_med_title_px_size};

		ui.set_widget_text(_texture_not_found_label, TEXTURE_VIEWER_NOT_FOUND_TEXT);
		paint.set_text(_texture_not_found_label,
					   ui.widget_text(_texture_not_found_label),
					   ui.widget_text_len(_texture_not_found_label),
					   {.font = theme.font_title, .color = theme.color_accent_err, .point_size = theme.text_med_title_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		ui.set_pre_layout_tick(_left_pane, on_texture_display_tick, this);

		editor_split_border_t::config_t split_config = {};
		split_config.direction						 = editor_split_border_direction_e::horizontal;
		split_config.on_drag						 = on_split_border_drag;
		split_config.user_data						 = this;
		_split_border.init(ui, _root, split_config);
		tree.draw_order(_split_border.get_root()) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& border_in = tree.in(_split_border.get_root());
		border_in.size_mode_x	   = ui::axis_mode_e::fixed;
		border_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		border_in.size_value	   = {theme.border_thickness * TEXTURE_VIEWER_SPLIT_BORDER_THICKNESS_MULT, 1.0f};

		_right_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_right_pane, "texture_viewer_right_pane");
		tree.attach(_root, _right_pane);
		tree.draw_order(_right_pane) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& right_in = tree.in(_right_pane);
		right_in.flags			  = ui::wf_visible;
		right_in.flow			  = ui::flow_e::column;
		right_in.child_margins	  = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};
		right_in.size_mode_x	  = ui::axis_mode_e::fill;
		right_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		right_in.size_value		  = {1.0f, 1.0f};

		_texture_size_value = append_property_value_row("Texture Size");
		editor_dividers_t::add_divider_hor(ui, _right_pane, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
		_is_linear_value = append_property_value_row("Is Linear");
		editor_dividers_t::add_divider_hor(ui, _right_pane, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
		_mip_dropdown_row = append_property_control_row("Mipmaps");
		editor_dividers_t::add_divider_hor(ui, _right_pane, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
		_mip_count_value = append_property_value_row("Mip Count");
		editor_dividers_t::add_divider_hor(ui, _right_pane, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
		_payload_format_value = append_property_value_row("Payload Format");
		editor_dividers_t::add_divider_hor(ui, _right_pane, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
		_runtime_format_value = append_property_value_row("Runtime Format");

		load_texture();
		rebuild_mip_dropdown();
		refresh_info();
		refresh_texture_state();
		apply_pane_split();
	}

	void editor_panel_texture_viewer_t::uninit()
	{
		if (_mip_dropdown_inited)
		{
			_mip_dropdown.uninit();
			_mip_dropdown_inited = false;
		}
		_split_border.uninit();
		_ui->deallocate_widget(_left_pane);
		_ui->deallocate_widget(_right_pane);
		unload_texture();
		editor_panel_t::uninit();
	}

	void editor_panel_texture_viewer_t::set_texture(sid_t texture_guid, const char* asset_name)
	{
		SFG_ASSERT(asset_name != nullptr);
		if (_texture_guid != texture_guid)
			unload_texture();
		_texture_guid = texture_guid;
		_asset_name	  = asset_name;
		load_texture();
		rebuild_mip_dropdown();
		refresh_info();
		refresh_texture_state();
		refresh_title();
	}

	void editor_panel_texture_viewer_t::refresh_title()
	{
		_title_text = editor_panel_type_to_string(editor_panel_type_e::texture_viewer);
		if (!_asset_name.empty())
		{
			_title_text = "T: ";
			_title_text += _asset_name;
		}
		set_title(_title_text.c_str());
		if (_ui != nullptr)
			editor_surface_controller_t::get().refresh_panel_title(this);
	}

	void editor_panel_texture_viewer_t::load_texture()
	{
		if (_texture_guid == 0 || _texture_loaded || _ui == nullptr)
			return;

		_texture_failed = resource_manager_t::get().load_resource(_texture_guid, resource_type_e::texture) == resource_state_e::failed;
		_texture_loaded = !_texture_failed;
	}

	void editor_panel_texture_viewer_t::unload_texture()
	{
		if (!_texture_loaded)
			return;

		resource_manager_t::get().unload_resource(_texture_guid);
		_texture_loaded	  = false;
		_texture_failed	  = false;
		_loaded_mip_count = 0;
		_selected_mip	  = 0;
	}

	void editor_panel_texture_viewer_t::rebuild_mip_dropdown()
	{
		if (_ui == nullptr)
			return;

		if (_mip_dropdown_inited)
		{
			_mip_dropdown.uninit();
			_mip_dropdown_inited = false;
		}

		_mip_dropdown_items.resize(0);
		_loaded_mip_count = 1;
		if (_texture_loaded)
		{
			if (const texture_runtime_t* runtime = resource_manager_t::get().find_runtime<texture_runtime_t>(_texture_guid))
				_loaded_mip_count = math::max<u8>(runtime->header.mip_count, 1);
		}

		_selected_mip = math::min<u16>(_selected_mip, static_cast<u16>(_loaded_mip_count - 1));
		_mip_dropdown_items.reserve(_loaded_mip_count);
		for (u16 i = 0; i < _loaded_mip_count; ++i)
		{
			_mip_label_storage[i] = "Mip ";
			_mip_label_storage[i] += std::to_string(i);
			_mip_dropdown_items.push_back({.text = _mip_label_storage[i].c_str(), .value = i});
		}

		editor_dropdown_config_t mip_config = {};
		mip_config.items					= _mip_dropdown_items.data();
		mip_config.item_count				= static_cast<u16>(_mip_dropdown_items.size());
		mip_config.selected					= get_selected_mip;
		mip_config.pressed					= on_mip_selected;
		mip_config.user_data				= this;
		mip_config.width					= editor_dropdown_width_e::parent_relative;
		_mip_dropdown.init(*_ui, _mip_dropdown_row, mip_config);
		_mip_dropdown_inited = true;

		ui::layout_in_t& dropdown_in = _ui->get_tree().in(_mip_dropdown.get_root());
		dropdown_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		dropdown_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		dropdown_in.pos_value		 = {1.0f, 0.5f};
		dropdown_in.anchor_x		 = ui::anchor_e::end;
		dropdown_in.anchor_y		 = ui::anchor_e::center;
		dropdown_in.size_value.x	 = 1.0f;
	}

	void editor_panel_texture_viewer_t::refresh_info()
	{
		if (_ui == nullptr)
			return;

		_texture_size_text	 = _texture_failed ? "Failed" : "-";
		_is_linear_text		 = "-";
		_payload_format_text = "-";
		_runtime_format_text = "-";
		_mip_count_text		 = "-";

		if (_texture_loaded)
		{
			if (const texture_runtime_t* runtime = resource_manager_t::get().find_runtime<texture_runtime_t>(_texture_guid))
			{
				const texture_header_t& header = runtime->header;
				_texture_size_text			   = std::to_string(header.size.x);
				_texture_size_text += " x ";
				_texture_size_text += std::to_string(header.size.y);
				_is_linear_text		 = header.is_linear ? "true" : "false";
				_payload_format_text = reflected_enum_display_name(type_id_t<texture_payload_type_e>::value, static_cast<u32>(header.payload_type));
				_runtime_format_text = reflected_enum_display_name(type_id_t<format_e>::value, static_cast<u32>(header.texture_format));
				_mip_count_text		 = std::to_string(header.mip_count);
			}
		}

		_ui->set_widget_text(_texture_size_value, _texture_size_text.c_str());
		_ui->set_widget_text(_is_linear_value, _is_linear_text.c_str());
		_ui->set_widget_text(_payload_format_value, _payload_format_text.c_str());
		_ui->set_widget_text(_runtime_format_value, _runtime_format_text.c_str());
		_ui->set_widget_text(_mip_count_value, _mip_count_text.c_str());

		ui::paint_layer_t&		  paint		  = _ui->get_paint();
		const editor_theme_t&	  theme		  = editor_theme_t::get();
		const ui::vg_text_style_t value_paint = {
			.font = theme.font_default, .color = _texture_failed ? theme.color_accent_warn : theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()};
		paint.set_text(_texture_size_value, _ui->widget_text(_texture_size_value), _ui->widget_text_len(_texture_size_value), value_paint);
		paint.set_text(_is_linear_value, _ui->widget_text(_is_linear_value), _ui->widget_text_len(_is_linear_value), value_paint);
		paint.set_text(_payload_format_value, _ui->widget_text(_payload_format_value), _ui->widget_text_len(_payload_format_value), value_paint);
		paint.set_text(_runtime_format_value, _ui->widget_text(_runtime_format_value), _ui->widget_text_len(_runtime_format_value), value_paint);
		paint.set_text(_mip_count_value, _ui->widget_text(_mip_count_value), _ui->widget_text_len(_mip_count_value), value_paint);
	}

	void editor_panel_texture_viewer_t::refresh_texture_state()
	{
		if (_ui == nullptr)
			return;

		ui::ui_render_state_t state = {};
		if (_texture_loaded)
		{
			state.pipeline					  = TEXTURE_VIEWER_TEXTURE_SHADER;
			state.constants[0].handle		  = _texture_guid;
			state.constants[0].type			  = ui::ui_resource_type_e::texture;
			state.constants[1].type			  = ui::ui_resource_type_e::test;
			state.constants[1].gpu_indices[0] = static_cast<gpu_index_t>(_selected_mip);
		}

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = {1.0f, 1.0f, 1.0f, _texture_loaded ? 1.0f : 0.0f};
		rect.fill_color_b		 = rect.fill_color_a;
		_ui->get_paint().set_rect(_texture_display, rect, state);

		_ui->get_tree().in(_texture_display).flags		   = _texture_loaded ? ui::wf_visible : 0;
		_ui->get_tree().in(_texture_not_found_label).flags = _texture_loaded ? 0 : ui::wf_visible;
	}

	void editor_panel_texture_viewer_t::apply_pane_split()
	{
		if (_ui != nullptr)
			_ui->get_tree().in(_left_pane).size_value.x = _pane_split;
	}

	ui::widget_id_t editor_panel_texture_viewer_t::append_property_value_row(const char* label)
	{
		const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _right_pane, label);
		return append_value_label(row.right);
	}

	ui::widget_id_t editor_panel_texture_viewer_t::append_property_control_row(const char* label)
	{
		const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _right_pane, label);
		return row.right;
	}

	ui::widget_id_t editor_panel_texture_viewer_t::append_value_label(ui::widget_id_t parent)
	{
		const editor_theme_t& theme = editor_theme_t::get();

		ui::widget_id_t label = _ui->allocate_widget();
		_ui->set_widget_debug_name(label, "texture_viewer_property_value");
		_ui->get_tree().attach(parent, label);

		ui::layout_in_t& label_in = _ui->get_tree().in(label);
		label_in.flags			  = ui::wf_visible;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;
		label_in.size_mode_x	  = ui::axis_mode_e::fill;
		label_in.size_mode_y	  = ui::axis_mode_e::fixed;
		label_in.size_value		  = {1.0f, theme.text_default_px_size};

		_ui->set_widget_text(label, "");
		_ui->get_paint().set_text(
			label, _ui->widget_text(label), _ui->widget_text_len(label), {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		return label;
	}

	void editor_panel_texture_viewer_t::on_split_border_drag(editor_split_border_t&, const vec2f_t& pos, const vec2f_t&, void* user_data)
	{
		editor_panel_texture_viewer_t& panel = *static_cast<editor_panel_texture_viewer_t*>(user_data);
		const ui::layout_out_t&		   out	 = panel._ui->get_tree().out(panel._root);
		SFG_ASSERT(out.size.x > 0.0f);

		panel._pane_split = math::clamp((pos.x - out.pos.x) / out.size.x, TEXTURE_VIEWER_PANE_SPLIT_MIN, TEXTURE_VIEWER_PANE_SPLIT_MAX);
		panel.apply_pane_split();
	}

	void editor_panel_texture_viewer_t::on_texture_display_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		editor_panel_texture_viewer_t& panel = *static_cast<editor_panel_texture_viewer_t*>(user_data);
		if (!panel._texture_loaded)
			return;

		const texture_runtime_t* runtime = resource_manager_t::get().find_runtime<texture_runtime_t>(panel._texture_guid);
		if (runtime == nullptr)
			return;

		const editor_theme_t&		theme		   = editor_theme_t::get();
		const ui::layout_out_t&		out			   = panel._ui->get_tree().out(panel._texture_frame);
		const texture_mip_header_t& mip			   = runtime->header.mips[panel._selected_mip];
		const f32					max_width	   = math::max(1.0f, out.size.x - theme.margin_horizontal * 2.0f);
		const f32					max_height	   = math::max(1.0f, out.size.y - theme.margin_vertical * 2.0f);
		const f32					texture_width  = math::max(1.0f, static_cast<f32>(mip.size.x));
		const f32					texture_height = math::max(1.0f, static_cast<f32>(mip.size.y));
		const f32					aspect		   = texture_width / texture_height;
		vec2f_t						display_size   = {};

		if (max_width / max_height > aspect)
			display_size = {max_height * aspect, max_height};
		else
			display_size = {max_width, max_width / aspect};

		ui::layout_in_t& texture_display_in = panel._ui->get_tree().in(panel._texture_display);
		texture_display_in.size_value		= {display_size.x, display_size.y};
	}

	u16 editor_panel_texture_viewer_t::get_selected_mip(void* user_data)
	{
		return static_cast<editor_panel_texture_viewer_t*>(user_data)->_selected_mip;
	}

	void editor_panel_texture_viewer_t::on_mip_selected(u16 value, void* user_data)
	{
		editor_panel_texture_viewer_t& panel = *static_cast<editor_panel_texture_viewer_t*>(user_data);
		panel._selected_mip					 = value;
		panel.refresh_texture_state();
	}
}
