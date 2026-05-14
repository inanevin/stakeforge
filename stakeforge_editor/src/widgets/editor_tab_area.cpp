// Copyright (c) 2025 Inan Evin

#include "widgets/editor_tab_area.hpp"
#include "editor_text_rasterization.hpp"
#include "panels/editor_theme.hpp"
#include "widgets/editor_widgets_icons.hpp"
#include "widgets/editor_widgets_misc.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/runtime/ui/layout/layout_tree.hpp>
#include <sfg/runtime/ui/paint/paint.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	void editor_tab_area_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		SFG_ASSERT(_ui == nullptr);

		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "editor_tab_area");
		tree.attach(parent, _root);

		ui::layout_in_t& in = tree.in(_root);
		in.size_mode_x		= ui::axis_mode_e::parent_relative;
		in.size_mode_y		= ui::axis_mode_e::fixed;
		in.size_value		= {1.0f, theme.item_height};
		in.flow				= ui::flow_e::row;
		in.child_spacing	= 0.0f;
		in.child_margins	= {0.0f, 0.0f, 0.0f, 0.0f};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = theme.color_bg0;
		rect.fill_color_b		 = theme.color_bg0;
		paint.set_rect(_root, rect);
	}

	void editor_tab_area_t::uninit()
	{
		_ui->deallocate_widget(_root);

		_ui	  = nullptr;
		_root = NULL_WIDGET;
		_tabs.clear();
	}

	void editor_tab_area_t::add_tab(const char* title)
	{
		SFG_ASSERT(title != nullptr);

		const sid_t identifier = TO_SID(title);
		for (const editor_tab_t& tab : _tabs)
			SFG_ASSERT(tab.identifier != identifier);

		ui::ui_context&		  ui	= *_ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		const ui::widget_id_t tab = ui.allocate_widget();
		ui.set_widget_debug_name(tab, "editor_tab");
		tree.attach(_root, tab);

		ui::layout_in_t& tab_in = tree.in(tab);
		tab_in.flags			= ui::wf_visible | ui::wf_input;
		tab_in.size_mode_x		= ui::axis_mode_e::sum_children;
		tab_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		tab_in.size_value		= {0.0f, 1.0f};
		tab_in.flow				= ui::flow_e::row;
		tab_in.child_spacing	= theme.item_spacing;
		tab_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		ui::vg_rect_paint_t tab_rect = {};
		tab_rect.fill_color_a		 = theme.color_bg4;
		tab_rect.fill_color_b		 = theme.color_bg4;
		paint.set_rect(tab, tab_rect);

		const ui::widget_id_t label = ui.allocate_widget();
		ui.set_widget_debug_name(label, "editor_tab_label");
		tree.attach(tab, label);

		ui::layout_in_t& label_in = tree.in(label);
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y	  = 0.5f;
		label_in.anchor_y		  = ui::anchor_e::center;

		ui.set_widget_text(label, title);
		paint.set_text(
			label, ui.widget_text(label), ui.widget_text_len(label), {.font = theme.font_default, .color = theme.color_fg2, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		editor_misc_widgets_t::add_spacer(ui, tab, {theme.item_spacing, theme.item_spacing});
		editor_icon_widgets_t::add_naked_icon_button(ui, tab, ICON_CROSS, theme.icon_default_px_size, theme.color_fg1);

		_tabs.push_back({.identifier = identifier, .widget = tab});
	}

	void editor_tab_area_t::remove_tab(sid_t identifier)
	{
		for (auto it = _tabs.begin(); it != _tabs.end(); ++it)
		{
			if (it->identifier == identifier)
			{
				_ui->deallocate_widget(it->widget);
				_tabs.erase(it);
				return;
			}
		}

		SFG_ASSERT(false);
	}
}
