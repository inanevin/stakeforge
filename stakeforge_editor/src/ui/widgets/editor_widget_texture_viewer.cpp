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

#include "ui/widgets/editor_widget_texture_viewer.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include <sfg/common/type_id.hpp>
#include <sfg/math/math.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/sprite.hpp>
#include <sfg/runtime/resources/texture.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
#define TEXTURE_VIEWER_TEXTURE_SHADER "editor/resource_pack/shaders/editor_ui_texture_mip.hlsl"_hs

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

	void editor_widget_texture_viewer_t::init(ui::ui_context& ui, ui::widget_id_t parent, editor_widget_texture_viewer_resource_e resource_type)
	{
		_ui			   = &ui;
		_resource_type = resource_type;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "texture_viewer");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		root_in.pos_mode_y		 = ui::pos_mode_e::flow;
		root_in.pos_value		 = {0.0f, 0.0f};
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value		 = {1.0f, 1.0f};
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = 0.0f;

		_top_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_top_pane, "texture_viewer_top_pane");
		tree.attach(_root, _top_pane);

		ui::layout_in_t& top_in = tree.in(_top_pane);
		top_in.flow				= ui::flow_e::column;
		top_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		top_in.size_mode_y		= ui::axis_mode_e::sum_children;
		top_in.size_value		= {1.0f, 1.0f};
		top_in.child_spacing	= 0.0f;

		_labels.push_back(make_section_label(_resource_type == editor_widget_texture_viewer_resource_e::sprite ? "Sprite" : "Texture"));

		_texture_size_value = append_property_value_row(_resource_type == editor_widget_texture_viewer_resource_e::sprite ? "Sprite Size" : "Texture Size");
		_is_linear_value	= append_property_value_row("Is Linear");

		if (_resource_type == editor_widget_texture_viewer_resource_e::texture)
			_mip_dropdown_row = append_property_control_row("Mipmaps");

		_mip_count_value	  = append_property_value_row("Mip Count");
		_payload_format_value = append_property_value_row("Payload Format");
		_runtime_format_value = append_property_value_row("Runtime Format");

		_bottom_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_bottom_pane, "texture_viewer_bottom_pane");
		tree.attach(_root, _bottom_pane);

		ui::layout_in_t& bottom_in = tree.in(_bottom_pane);
		bottom_in.flow			   = ui::flow_e::none;
		bottom_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		bottom_in.size_mode_y	   = ui::axis_mode_e::fixed;
		bottom_in.size_value	   = {1.0f, theme.item_height * 20.0f};
		bottom_in.child_margins	   = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};

		ui::vg_rect_paint_t bottom_rect = {};
		bottom_rect.fill_color_a		= theme.color_frame;
		bottom_rect.fill_color_b		= theme.color_frame;
		paint.set_rect(_bottom_pane, bottom_rect);

		_texture_display = ui.allocate_widget();
		ui.set_widget_debug_name(_texture_display, "texture_viewer_texture_display");
		tree.attach(_bottom_pane, _texture_display);
		tree.draw_order(_texture_display) = tree.draw_order_const(_bottom_pane) + 1;

		ui::layout_in_t& texture_in = tree.in(_texture_display);
		texture_in.pos_mode_x		= ui::pos_mode_e::relative_in_parent;
		texture_in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
		texture_in.pos_value		= {0.5f, 0.5f};
		texture_in.anchor_x			= ui::anchor_e::center;
		texture_in.anchor_y			= ui::anchor_e::center;
		texture_in.size_mode_x		= ui::axis_mode_e::fixed;
		texture_in.size_mode_y		= ui::axis_mode_e::fixed;
		texture_in.size_value		= {1.0f, 1.0f};

		ui.set_pre_layout_tick(_bottom_pane, on_texture_display_tick, this);
		if (_resource_type == editor_widget_texture_viewer_resource_e::texture)
			rebuild_mip_dropdown();

		refresh_info();
		refresh_texture_state();
	}

	void editor_widget_texture_viewer_t::uninit()
	{
		_ui->cancel_mutations(this);

		if (_mip_dropdown_inited)
		{
			_mip_dropdown.uninit();
			_mip_dropdown_inited = false;
		}

		unload_texture();
		_ui->deallocate_widget(_root);

		_mip_dropdown_items.resize(0);
		_rows.resize(0);
		_dividers.resize(0);
		_labels.resize(0);
		_texture_size_text.resize(0);
		_is_linear_text.resize(0);
		_payload_format_text.resize(0);
		_runtime_format_text.resize(0);
		_mip_count_text.resize(0);
		_ui						 = nullptr;
		_texture_guid			 = 0;
		_pending_texture_guid	 = 0;
		_root					 = NULL_WIDGET;
		_top_pane				 = NULL_WIDGET;
		_bottom_pane			 = NULL_WIDGET;
		_texture_display		 = NULL_WIDGET;
		_texture_size_value		 = NULL_WIDGET;
		_is_linear_value		 = NULL_WIDGET;
		_mip_dropdown_row		 = NULL_WIDGET;
		_payload_format_value	 = NULL_WIDGET;
		_runtime_format_value	 = NULL_WIDGET;
		_mip_count_value		 = NULL_WIDGET;
		_selected_mip			 = 0;
		_loaded_mip_count		 = 0;
		_texture_loaded			 = false;
		_texture_failed			 = false;
		_refresh_texture_pending = false;
		_resource_type			 = editor_widget_texture_viewer_resource_e::texture;
	}

	void editor_widget_texture_viewer_t::set_texture(sid_t texture_guid)
	{
		set_resource(texture_guid);
	}

	void editor_widget_texture_viewer_t::clear_texture()
	{
		clear_resource();
	}

	void editor_widget_texture_viewer_t::set_resource(sid_t resource_guid)
	{
		if (!can_mutate_ui_topology())
		{
			if (_texture_guid != resource_guid)
			{
				unload_texture();
				_texture_guid = resource_guid;
			}

			request_texture_refresh(resource_guid);
			return;
		}

		if (_texture_guid != resource_guid)
			unload_texture();

		_texture_guid = resource_guid;
		load_texture();

		if (_resource_type == editor_widget_texture_viewer_resource_e::texture)
			rebuild_mip_dropdown();

		refresh_info();
		refresh_texture_state();
	}

	void editor_widget_texture_viewer_t::clear_resource()
	{
		set_resource(0);
	}

	void editor_widget_texture_viewer_t::load_texture()
	{
		if (_texture_guid == 0 || _texture_loaded || _ui == nullptr)
			return;

		const resource_type_e resource_type = _resource_type == editor_widget_texture_viewer_resource_e::sprite ? resource_type_e::sprite : resource_type_e::texture;
		_texture_failed						= resource_manager_t::get().load_resource(_texture_guid, resource_type) == resource_state_e::failed;
		_texture_loaded						= !_texture_failed;
	}

	void editor_widget_texture_viewer_t::unload_texture()
	{
		if (_texture_loaded)
			resource_manager_t::get().unload_resource(_texture_guid);

		_texture_loaded	  = false;
		_texture_failed	  = false;
		_loaded_mip_count = 0;
		_selected_mip	  = 0;
	}

	void editor_widget_texture_viewer_t::rebuild_mip_dropdown()
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

		if (_texture_loaded && _resource_type == editor_widget_texture_viewer_resource_e::texture)
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
		mip_config.pos_y					= editor_dropdown_pos_y_e::center;
		_mip_dropdown.init(*_ui, _mip_dropdown_row, mip_config);
		_mip_dropdown_inited = true;

		ui::layout_in_t& dropdown_in = _ui->get_tree().in(_mip_dropdown.get_root());
		dropdown_in.size_value.x	 = 1.0f;
	}

	void editor_widget_texture_viewer_t::refresh_info()
	{
		if (_ui == nullptr)
			return;

		_texture_size_text	 = _texture_failed ? "Failed" : "-";
		_is_linear_text		 = "-";
		_payload_format_text = "-";
		_runtime_format_text = "-";
		_mip_count_text		 = "-";

		if (_texture_loaded && _resource_type == editor_widget_texture_viewer_resource_e::texture)
		{
			if (const texture_runtime_t* runtime = resource_manager_t::get().find_runtime<texture_runtime_t>(_texture_guid))
			{
				const texture_header_t& header = runtime->header;

				_texture_size_text = std::to_string(header.size.x);
				_texture_size_text += " x ";
				_texture_size_text += std::to_string(header.size.y);
				_is_linear_text		 = header.is_linear ? "true" : "false";
				_payload_format_text = reflected_enum_display_name(type_id_t<texture_payload_type_e>::value, static_cast<u32>(header.payload_type));
				_runtime_format_text = reflected_enum_display_name(type_id_t<format_e>::value, static_cast<u32>(header.texture_format));
				_mip_count_text		 = std::to_string(header.mip_count);
			}
		}
		else if (_texture_loaded)
		{
			if (const sprite_runtime_t* runtime = resource_manager_t::get().find_runtime<sprite_runtime_t>(_texture_guid))
			{
				const sprite_header_t& header		  = runtime->header;
				const format_e		   runtime_format = header.payload_type == sprite_payload_type_e::png ? format_e::r8g8b8a8_srgb : format_e::bc7_block_srgb;

				_texture_size_text = std::to_string(header.size.x);
				_texture_size_text += " x ";
				_texture_size_text += std::to_string(header.size.y);
				_is_linear_text		 = "false";
				_payload_format_text = reflected_enum_display_name(type_id_t<sprite_payload_type_e>::value, static_cast<u32>(header.payload_type));
				_runtime_format_text = reflected_enum_display_name(type_id_t<format_e>::value, static_cast<u32>(runtime_format));
				_mip_count_text		 = "1";
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

	void editor_widget_texture_viewer_t::refresh_texture_state()
	{
		if (_ui == nullptr)
			return;

		ui::ui_render_state_t state = {};

		if (_texture_loaded)
		{
			state.pipeline					  = TEXTURE_VIEWER_TEXTURE_SHADER;
			state.constants[0].handle		  = _texture_guid;
			state.constants[0].type			  = _resource_type == editor_widget_texture_viewer_resource_e::sprite ? ui::ui_resource_type_e::sprite : ui::ui_resource_type_e::texture;
			state.constants[1].type			  = ui::ui_resource_type_e::test;
			state.constants[1].gpu_indices[0] = static_cast<gpu_index_t>(_selected_mip);
		}

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = {1.0f, 1.0f, 1.0f, _texture_loaded ? 1.0f : 0.0f};
		rect.fill_color_b		 = rect.fill_color_a;
		_ui->get_paint().set_rect(_texture_display, rect, state);

		_ui->get_tree().in(_texture_display).flags = _texture_loaded ? ui::wf_visible : 0;
	}

	void editor_widget_texture_viewer_t::append_property_row(ui::widget_id_t row)
	{
		_rows.push_back(row);
		_dividers.push_back(editor_dividers_t::add_divider_hor(*_ui, _top_pane, editor_theme_t::get().divider_thickness * 2.0f, editor_theme_t::get().color_frame, editor_theme_t::get().color_frame, ui::vg_gradient_e::none));
	}

	ui::widget_id_t editor_widget_texture_viewer_t::append_property_value_row(const char* label)
	{
		const editor_property_row_t row	  = editor_misc_widgets_t::make_property_row_with_label(*_ui, _top_pane, label);
		const ui::widget_id_t		value = make_value_label(row.right);
		append_property_row(row.row);
		return value;
	}

	ui::widget_id_t editor_widget_texture_viewer_t::append_property_control_row(const char* label)
	{
		const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _top_pane, label);
		append_property_row(row.row);
		return row.right;
	}

	ui::widget_id_t editor_widget_texture_viewer_t::make_section_label(const char* text)
	{
		const editor_theme_t& theme = editor_theme_t::get();

		const ui::widget_id_t label = _ui->allocate_widget();
		_ui->set_widget_debug_name(label, "texture_viewer_section_label");
		_ui->get_tree().attach(_top_pane, label);

		ui::layout_in_t& label_in = _ui->get_tree().in(label);
		label_in.flags			  = ui::wf_visible;
		label_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		label_in.size_mode_y	  = ui::axis_mode_e::fixed;
		label_in.size_value		  = {1.0f, theme.item_area_height};
		label_in.child_margins	  = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};
		label_in.flow			  = ui::flow_e::row;

		ui::widget_id_t text_widget = _ui->allocate_widget();
		_ui->set_widget_debug_name(text_widget, "texture_viewer_section_text");
		_ui->get_tree().attach(label, text_widget);

		ui::layout_in_t& text_in = _ui->get_tree().in(text_widget);
		text_in.flags			 = ui::wf_visible;
		text_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		text_in.pos_value.y		 = 0.5f;
		text_in.anchor_y		 = ui::anchor_e::center;
		text_in.size_mode_x		 = ui::axis_mode_e::fill;
		text_in.size_mode_y		 = ui::axis_mode_e::fixed;
		text_in.size_value		 = {1.0f, theme.text_default_px_size};

		_ui->set_widget_text(text_widget, text);
		_ui->get_paint().set_text(text_widget,
								  _ui->widget_text(text_widget),
								  _ui->widget_text_len(text_widget),
								  {.font = theme.font_title_bold, .color = theme.color_accent1, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		return label;
	}

	ui::widget_id_t editor_widget_texture_viewer_t::make_value_label(ui::widget_id_t parent)
	{
		const editor_theme_t& theme = editor_theme_t::get();

		const ui::widget_id_t label = _ui->allocate_widget();
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

	bool editor_widget_texture_viewer_t::can_mutate_ui_topology() const
	{
		const ui::ui_phase_e phase = _ui->get_phase();
		return phase == ui::ui_phase_e::idle || phase == ui::ui_phase_e::mutation || phase == ui::ui_phase_e::pre_layout;
	}

	void editor_widget_texture_viewer_t::request_texture_refresh(sid_t texture_guid)
	{
		_pending_texture_guid	 = texture_guid;
		_refresh_texture_pending = true;
		_ui->request_unique_mutation(on_ui_mutation, this);
	}

	void editor_widget_texture_viewer_t::flush_pending_ui_mutations()
	{
		if (!_refresh_texture_pending)
			return;

		const sid_t texture_guid = _pending_texture_guid;
		_pending_texture_guid	 = 0;
		_refresh_texture_pending = false;
		set_resource(texture_guid);
	}

	void editor_widget_texture_viewer_t::on_texture_display_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		editor_widget_texture_viewer_t& viewer = *static_cast<editor_widget_texture_viewer_t*>(user_data);

		if (!viewer._texture_loaded)
			return;

		const editor_theme_t&	theme		 = editor_theme_t::get();
		const ui::layout_out_t& out			 = viewer._ui->get_tree().out(viewer._bottom_pane);
		const f32				scale		 = ui::get_valid_scale(viewer._ui->get_ui_scale());
		const f32				max_width	 = math::max(1.0f, out.size.x / scale - theme.margin_horizontal * 2.0f);
		const f32				max_height	 = math::max(1.0f, out.size.y / scale - theme.margin_vertical * 2.0f);
		vec2u16_t				texture_size = vec2u16_t::zero;

		if (viewer._resource_type == editor_widget_texture_viewer_resource_e::sprite)
		{
			const sprite_runtime_t* runtime = resource_manager_t::get().find_runtime<sprite_runtime_t>(viewer._texture_guid);

			if (runtime == nullptr)
				return;

			texture_size = runtime->header.size;
		}
		else
		{
			const texture_runtime_t* runtime = resource_manager_t::get().find_runtime<texture_runtime_t>(viewer._texture_guid);

			if (runtime == nullptr)
				return;

			texture_size = runtime->header.mips[viewer._selected_mip].size;
		}

		const f32 texture_width	 = math::max(1.0f, static_cast<f32>(texture_size.x));
		const f32 texture_height = math::max(1.0f, static_cast<f32>(texture_size.y));
		const f32 aspect		 = texture_width / texture_height;
		vec2f_t	  display_size	 = {};

		if (max_width / max_height > aspect)
			display_size = {max_height * aspect, max_height};
		else
			display_size = {max_width, max_width / aspect};

		ui::layout_in_t& texture_display_in = viewer._ui->get_tree().in(viewer._texture_display);
		texture_display_in.size_value		= {display_size.x, display_size.y};
	}

	void editor_widget_texture_viewer_t::on_ui_mutation(ui::ui_context&, void* user_data)
	{
		static_cast<editor_widget_texture_viewer_t*>(user_data)->flush_pending_ui_mutations();
	}

	u16 editor_widget_texture_viewer_t::get_selected_mip(void* user_data)
	{
		return static_cast<editor_widget_texture_viewer_t*>(user_data)->_selected_mip;
	}

	void editor_widget_texture_viewer_t::on_mip_selected(u16 value, void* user_data)
	{
		editor_widget_texture_viewer_t& viewer = *static_cast<editor_widget_texture_viewer_t*>(user_data);
		viewer._selected_mip				   = value;
		viewer.refresh_texture_state();
	}
}
