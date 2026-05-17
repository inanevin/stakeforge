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
#include "ui/widgets/editor_widgets_color_field.hpp"
#include "ui/panels/editor_theme.hpp"
#include <sfg/math/color.hpp>
#include <sfg/math/color_utils.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <cstring>

namespace sfg
{
	namespace
	{
		void color_to_hex(const vec4f_t& color, char* out, size_t out_size)
		{
			const color_t  srgb = color_t{color.x, color.y, color.z, color.w}.linear_to_srgb();
			const string_t hex	= color_utils_t::to_hex(srgb);
			const size_t   len	= math::min(out_size - 1, hex.size());
			std::memcpy(out, hex.c_str(), len);
			out[len] = '\0';
		}

		bool hex_to_color(const char* text, vec4f_t& out)
		{
			if (text == nullptr || std::strlen(text) != 7 || text[0] != '#')
				return false;

			color_t srgb = color_utils_t::from_hex(string_t{text});
			out			 = srgb.srgb_to_linear().to_vector();
			return true;
		}

		void set_color_rect(ui::paint_layer_t& paint, ui::widget_id_t id, const vec4f_t& color)
		{
			const editor_theme_t& theme = editor_theme_t::get();
			ui::vg_rect_paint_t	  rect	= {};
			rect.fill_color_a			= color;
			rect.fill_color_b			= color;
			rect.outline_color			= theme.color_outline_light;
			rect.outline_thickness		= theme.outline_thickness;
			rect.rounding				= theme.item_rounding;
			rect.rounding_segs			= 4;
			rect.aa_thickness			= theme.aa_thickness;
			paint.set_rect(id, rect);
		}
	}

	void editor_color_field_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_color_field_config_t& config)
	{
		_ui		= &ui;
		_config = config;
		_color	= config.color;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "color_field");
		tree.attach(parent, _root);
		tree.draw_order(_root) = tree.draw_order_const(parent) + 1;

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible;
		apply_editor_widget_width(root_in, config.width);
		root_in.size_mode_y	  = ui::axis_mode_e::fixed;
		root_in.size_value.y  = theme.item_height;
		root_in.flow		  = ui::flow_e::row;
		root_in.child_spacing = theme.item_spacing;

		char text[16] = {};
		color_to_hex(_color, text, sizeof(text));

		editor_input_field_config_t input_config = {};
		input_config.placeholder				 = "#ffffff";
		input_config.text_value					 = text;
		input_config.type						 = editor_input_field_type_e::text;
		input_config.on_text_changed			 = on_text_changed;
		input_config.user_data					 = this;
		_input.init(ui, _root, input_config);

		ui::layout_in_t& input_in = tree.in(_input.get_root());
		input_in.size_mode_x	  = ui::axis_mode_e::fill;
		input_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		input_in.size_value		  = {1.0f, 1.0f};

		_swatch = ui.allocate_widget();
		ui.set_widget_debug_name(_swatch, "color_field_swatch");
		tree.attach(_root, _swatch);
		tree.draw_order(_swatch) = tree.draw_order_const(_root) + 1;

		ui::layout_in_t& swatch_in = tree.in(_swatch);
		swatch_in.flags			   = ui::wf_visible;
		swatch_in.size_mode_x	   = ui::axis_mode_e::fixed;
		swatch_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		swatch_in.size_value	   = {theme.item_height, 1.0f};
		refresh_color();
	}

	void editor_color_field_t::uninit()
	{
		_input.uninit();
		_ui->deallocate_widget(_root);

		_ui			= nullptr;
		_root		= NULL_WIDGET;
		_swatch		= NULL_WIDGET;
		_config		= {};
		_color		= {1.0f, 1.0f, 1.0f, 1.0f};
		_refreshing = false;
	}

	void editor_color_field_t::set_color(const vec4f_t& color)
	{
		_color = color;
		refresh_color();
		refresh_text();
	}

	void editor_color_field_t::refresh_color()
	{
		set_color_rect(_ui->get_paint(), _swatch, _color);
	}

	void editor_color_field_t::refresh_text()
	{
		char text[16] = {};
		color_to_hex(_color, text, sizeof(text));
		_refreshing = true;
		_input.set_text(text);
		_refreshing = false;
	}

	void editor_color_field_t::on_text_changed(const char* value, void* user_data)
	{
		editor_color_field_t& field = *static_cast<editor_color_field_t*>(user_data);
		if (field._refreshing)
			return;

		vec4f_t color = {};
		if (!hex_to_color(value, color))
			return;

		field._color = color;
		field.refresh_color();
		if (field._config.on_changed != nullptr)
			field._config.on_changed(field._color, field._config.user_data);
	}
}
