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
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_color_field_t::init(ui::ui_context& ui, ui::widget_id_t parent, const editor_color_field_config_t& config)
	{
		_ui		= &ui;
		_config = config;
		_color	= {0.0f, 0.0f, 0.0f, 1.0f};

		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "color_field");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flags			 = ui::wf_visible;
		apply_editor_widget_width(root_in, config.width);
		root_in.size_mode_y	  = ui::axis_mode_e::fixed;
		root_in.size_value.y  = theme.item_height;
		root_in.flow		  = ui::flow_e::row;
		root_in.child_spacing = theme.item_spacing;

		_swatch = ui.allocate_widget();
		ui.set_widget_debug_name(_swatch, "color_field_swatch");
		tree.attach(_root, _swatch);

		ui::layout_in_t& swatch_in = tree.in(_swatch);
		swatch_in.flags			   = ui::wf_visible;
		swatch_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		swatch_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		swatch_in.size_value	   = {1.0f, 1.0f};
		refresh_color();
	}

	void editor_color_field_t::uninit()
	{
		_ui->deallocate_widget(_root);

		_ui		= nullptr;
		_root	= NULL_WIDGET;
		_swatch = NULL_WIDGET;
		_config = {};
		_color	= {1.0f, 1.0f, 1.0f, 1.0f};
	}

	void editor_color_field_t::set_color(const vec4f_t& color)
	{
		_color = color;
		refresh_color();
	}

	void editor_color_field_t::refresh_color()
	{
		const editor_theme_t& theme = editor_theme_t::get();
		ui::vg_rect_paint_t	  rect	= {};
		rect.fill_color_a			= {0.0f, 0.0f, 0.0f, 1.0f};
		rect.fill_color_b			= {0.0f, 0.0f, 0.0f, 1.0f};
		rect.outline_color			= theme.color_outline_light;
		rect.outline_thickness		= theme.outline_thickness;
		rect.rounding				= theme.item_rounding;
		rect.rounding_segs			= 4;
		rect.aa_thickness			= theme.aa_thickness;
		_ui->get_paint().set_rect(_swatch, rect);
	}
}
