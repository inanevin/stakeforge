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

#include "ui/editor_modal_component_add.hpp"
#include "ui/editor_modal_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include "commands/editor_commands_component.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"

#include <sfg/data/string_util.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/reflection/reflection_icons.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/ecs.hpp>

namespace sfg
{
#define COMPONENT_PICKER_COLUMNS 5u

	void editor_modal_component_add_t::open(ui::ui_context& ui, editor_world_handle_t handle, span_t<const entity_id_t> entities)
	{
		close();

		_world = handle;
		_entities.assign(entities.data, entities.data + entities.size);
		_items.resize(0);
		_search_text.resize(0);
		_search_lower.resize(0);
		_selected = 0;

		world_t&							   world  = editor_world_controller_t::get().get_editor_world(handle)->get_world();
		const vector_t<ecs_component_table_t>& tables = world.get_component_tables();
		_items.reserve(tables.size());

		for (const ecs_component_table_t& table : tables)
		{
			const reflected_type_t* type = reflection_registry_t::get().find_type(table.get_type_desc().type_id);

			if (type == nullptr || type->flags.is_set(reflected_type_flag_no_ui))
				continue;

			const bool all_have_component = std::all_of(_entities.begin(), _entities.end(), [&](entity_id_t entity) { return table.has(entity); });

			if (all_have_component)
				continue;

			const char* name	 = type->display_name != nullptr ? type->display_name : type->name;
			const char* category = type->category != nullptr ? type->category : "Component";
			item_t		item{.name = name, .category = category, .type = table.get_type_desc().type_id};

			SFG_MEMCPY(item.icon, type->icon, sizeof(item.icon));
			item.search_text = category;
			item.search_text += " ";
			item.search_text += name;
			string_util::to_lower(item.search_text);
			_items.push_back(std::move(item));
		}

		std::sort(_items.begin(), _items.end(), [](const item_t& a, const item_t& b) {
			const int category = std::strcmp(a.category, b.category);

			return category == 0 ? std::strcmp(a.name, b.name) < 0 : category < 0;
		});

		const editor_modal_content_desc_t content{.init = init_content, .uninit = uninit_content, .user_data = this};
		editor_modal_controller_t::find(ui)->request_modal("Pick Component", "Choose a component to add to the selected entities.", false, nullptr, 0, &content);
		ui.get_input().set_focus(_search.get_root(), false);
	}

	void editor_modal_component_add_t::close()
	{
		if (_ui != nullptr)
			editor_modal_controller_t::find(*_ui)->close_modal();
	}

	void editor_modal_component_add_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui = &ui;

		ui::layout_tree_t&	  tree		 = ui.get_tree();
		const editor_theme_t& theme		 = editor_theme_t::get();
		const f32			  tile_width = theme.item_height * 6.0f;
		_root							 = ui.allocate_widget();
		ui.set_widget_debug_name(_root, "component_picker");
		tree.attach(parent, _root);

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::fixed;
		root_in.size_mode_y		 = ui::axis_mode_e::fixed;
		root_in.size_value		 = {tile_width * COMPONENT_PICKER_COLUMNS + theme.item_spacing * (COMPONENT_PICKER_COLUMNS - 1) + theme.margin_horizontal * 3.0f, theme.item_height * 21.0f};
		root_in.flow			 = ui::flow_e::column;
		root_in.child_spacing	 = theme.item_spacing;

		const ui::widget_id_t search_row = ui.allocate_widget();
		tree.attach(_root, search_row);

		ui::layout_in_t& search_row_in = tree.in(search_row);
		search_row_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		search_row_in.size_mode_y	   = ui::axis_mode_e::fixed;
		search_row_in.size_value	   = {1.0f, theme.item_area_height};

		u8* search_data = reinterpret_cast<u8*>(&_search_text);
		_search.init(ui,
					 search_row,
					 {
						 .field			 = {.fields = {.data = &search_data, .size = 1}, .type = editor_input_field_field_type_e::string},
						 .callbacks		 = {.edited = on_search_changed, .user_data = this},
						 .placeholder	 = "Search components...",
						 .return_pressed = on_add,
						 .escape_pressed = on_cancel,
					 });

		ui::layout_in_t& search_in = tree.in(_search.get_root());
		search_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		search_in.pos_value.y	   = 0.5f;
		search_in.anchor_y		   = ui::anchor_e::center;
		search_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		search_in.size_mode_y	   = ui::axis_mode_e::fixed;
		search_in.size_value	   = {1.0f, theme.item_height};

		_area = ui.allocate_widget();
		ui.set_widget_debug_name(_area, "component_picker_area");
		tree.attach(_root, _area);

		ui::layout_in_t& area_in = tree.in(_area);
		area_in.flags			 = ui::wf_visible | ui::wf_input | ui::wf_scroll_y;
		area_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		area_in.size_mode_y		 = ui::axis_mode_e::fill;
		area_in.size_value		 = vec2f_t::one;
		area_in.flow			 = ui::flow_e::column;
		area_in.child_clip_mode	 = ui::clip_mode_e::scissor_rect;
		area_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal * 2.0f, theme.margin_vertical, theme.margin_horizontal};

		ui::vg_rect_paint_t area_rect = {};
		area_rect.fill_color_a		  = theme.color_panel;
		area_rect.fill_color_b		  = theme.color_panel;
		area_rect.rounding			  = theme.item_rounding;
		area_rect.rounding_segs		  = 4;
		ui.get_paint().set_rect(_area, area_rect);
		_scrollbar.init(ui, {.target = _area, .axes = editor_scrollbar_axis_y});

		const ui::widget_id_t buttons = ui.allocate_widget();
		tree.attach(_root, buttons);

		ui::layout_in_t& buttons_in = tree.in(buttons);
		buttons_in.size_mode_x		= ui::axis_mode_e::sum_children;
		buttons_in.size_mode_y		= ui::axis_mode_e::fixed;
		buttons_in.size_value.y		= theme.item_height;
		buttons_in.flow				= ui::flow_e::row;
		buttons_in.child_spacing	= theme.margin_horizontal;

		_cancel.init(ui, buttons, {.text = "Cancel", .width = {.mode = editor_widget_width_e::fixed, .value = theme.item_width}});
		_add.init(ui, buttons, {.text = "Add", .width = {.mode = editor_widget_width_e::fixed, .value = theme.item_width}});
		ui.get_input().set_listener(_cancel.get_root(), {.on_click = on_button_clicked, .user_data = this});
		ui.get_input().set_listener(_add.get_root(), {.on_click = on_button_clicked, .user_data = this});
		ui.set_pre_layout_tick(_root, on_tick, this);
		rebuild();
	}

	void editor_modal_component_add_t::uninit()
	{
		_search.uninit();
		_scrollbar.uninit();
		_cancel.uninit();
		_add.uninit();
		_ui->deallocate_widget(_root);
		_ui				 = nullptr;
		_root			 = NULL_WIDGET;
		_area			 = NULL_WIDGET;
		_column			 = NULL_WIDGET;
		_refresh_pending = false;
	}

	void editor_modal_component_add_t::rebuild()
	{
		ui::ui_context&		  ui	= *_ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		if (_column != NULL_WIDGET)
			ui.deallocate_widget(_column);

		_column = ui.allocate_widget();
		tree.attach(_area, _column);
		tree.draw_order(_column) = tree.draw_order_const(_area) + 1;

		ui::layout_in_t& column_in = tree.in(_column);
		column_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		column_in.size_mode_y	   = ui::axis_mode_e::sum_children;
		column_in.size_value.x	   = 1.0f;
		column_in.flow			   = ui::flow_e::column;
		column_in.child_spacing	   = theme.item_spacing;

		const f32		icon_size		 = theme.item_height * 2.0f;
		const f32		tile_width		 = theme.item_height * 6.0f;
		const char*		category		 = nullptr;
		ui::widget_id_t row				 = NULL_WIDGET;
		u32				column			 = 0;
		sid_t			first_match		 = 0;
		bool			selected_visible = false;

		for (item_t& item : _items)
		{
			item.tile = NULL_WIDGET;

			if (!_search_lower.empty() && item.search_text.find(_search_lower) == string_t::npos)
				continue;

			if (first_match == 0)
				first_match = item.type;

			selected_visible |= item.type == _selected;

			if (category == nullptr || std::strcmp(category, item.category) != 0)
			{
				category = item.category;

				const ui::widget_id_t category_row	= editor_misc_widgets_t::make_section_label(ui, _column, category);
				const ui::widget_id_t category_text = tree.node(category_row).first_child;

				tree.in(category_text).size_value.y = theme.text_med_title_px_size;
				paint.set_text(category_text,
							   ui.widget_text(category_text),
							   ui.widget_text_len(category_text),
							   {.font = theme.font_title, .color = theme.color_accent1, .point_size = theme.text_med_title_px_size, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
				column = 0;
			}

			if (column == 0)
			{
				row = ui.allocate_widget();
				tree.attach(_column, row);

				ui::layout_in_t& row_in = tree.in(row);
				row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
				row_in.size_mode_y		= ui::axis_mode_e::fixed;
				row_in.size_value		= {1.0f, icon_size + theme.item_height + theme.item_spacing};
				row_in.flow				= ui::flow_e::row;
				row_in.child_spacing	= theme.item_spacing;
			}

			const ui::widget_id_t cell = ui.allocate_widget();
			tree.attach(row, cell);

			ui::layout_in_t& cell_in = tree.in(cell);
			cell_in.size_mode_x		 = ui::axis_mode_e::fixed;
			cell_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
			cell_in.size_value		 = {tile_width, 1.0f};
			cell_in.child_clip_mode	 = ui::clip_mode_e::scissor_rect;

			item.tile = ui.allocate_widget();
			ui.set_widget_debug_name(item.tile, "component_picker_tile");
			tree.attach(cell, item.tile);

			ui::layout_in_t& tile_in = tree.in(item.tile);
			tile_in.flags			 = ui::wf_visible | ui::wf_input;
			tile_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
			tile_in.pos_value.x		 = 0.5f;
			tile_in.anchor_x		 = ui::anchor_e::center;
			tile_in.size_mode_x		 = ui::axis_mode_e::fixed;
			tile_in.size_mode_y		 = ui::axis_mode_e::copy_other;
			tile_in.size_value		 = {icon_size, 1.0f};

			ui::vg_rect_paint_t tile_rect = {};
			tile_rect.outline_color		  = theme.color_outline;
			tile_rect.outline_thickness	  = theme.outline_thickness;
			tile_rect.rounding			  = theme.item_rounding;
			tile_rect.rounding_segs		  = 4;
			paint.set_rect(item.tile, tile_rect);
			paint.set_press_color(item.tile, theme.color_accent1_dim);
			ui.get_input().set_listener(item.tile, {.on_click = on_item_clicked, .user_data = this});

			const ui::widget_id_t icon = ui.allocate_widget();
			tree.attach(item.tile, icon);

			ui::layout_in_t& icon_in = tree.in(icon);
			icon_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
			icon_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
			icon_in.pos_value		 = {0.5f, 0.5f};
			icon_in.anchor_x		 = ui::anchor_e::center;
			icon_in.anchor_y		 = ui::anchor_e::center;
			ui.set_widget_text(icon, item.icon[0] != '\0' ? item.icon : ICON_COMPONENT_GENERIC);
			paint.set_text(icon, ui.widget_text(icon), ui.widget_text_len(icon), {.font = theme.font_icons, .color = theme.color_text0, .point_size = theme.item_height, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

			const ui::widget_id_t label = ui.allocate_widget();
			tree.attach(cell, label);

			ui::layout_in_t& label_in = tree.in(label);
			label_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
			label_in.pos_mode_y		  = ui::pos_mode_e::offset_in_parent;
			label_in.pos_value		  = {0.5f, icon_size + theme.item_spacing};
			label_in.anchor_x		  = ui::anchor_e::center;
			ui.set_widget_text(label, item.name);
			paint.set_text(label, ui.widget_text(label), ui.widget_text_len(label), {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
			column = (column + 1) % COMPONENT_PICKER_COLUMNS;
		}

		if (!selected_visible)
			_selected = first_match;

		if (first_match == 0)
			editor_misc_widgets_t::make_section_label(ui, _column, "No matching components");

		refresh_selection();
		_scrollbar.set_scroll_y_immediate(0.0f);
		_refresh_pending = false;
	}

	void editor_modal_component_add_t::refresh_selection()
	{
		const editor_theme_t& theme = editor_theme_t::get();
		ui::paint_layer_t&	  paint = _ui->get_paint();

		for (const item_t& item : _items)
		{
			if (item.tile == NULL_WIDGET)
				continue;

			const bool		 selected = item.type == _selected;
			const vec4f_t	 color	  = selected ? theme.color_accent1_dim : theme.color_frame;
			ui::paint_def_t& def	  = paint.def(item.tile);
			def.rect.fill_color_a	  = color;
			def.rect.fill_color_b	  = color;
			paint.set_hover_color(item.tile, selected ? theme.color_accent1_dim : theme.color_frame_light);
		}

		ui::layout_in_t& add_in = _ui->get_tree().in(_add.get_root());

		if (_selected == 0)
			add_in.flags |= ui::wf_disabled;
		else
			add_in.flags &= ~ui::wf_disabled;
	}

	void editor_modal_component_add_t::init_content(ui::ui_context& ui, ui::widget_id_t parent, void* user_data)
	{
		static_cast<editor_modal_component_add_t*>(user_data)->init(ui, parent);
	}

	void editor_modal_component_add_t::uninit_content(void* user_data)
	{
		static_cast<editor_modal_component_add_t*>(user_data)->uninit();
	}

	void editor_modal_component_add_t::on_search_changed(void* user_data)
	{
		editor_modal_component_add_t& modal = *static_cast<editor_modal_component_add_t*>(user_data);
		modal._search_lower					= modal._search_text;
		string_util::to_lower(modal._search_lower);
		modal._refresh_pending = true;
	}

	void editor_modal_component_add_t::on_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data)
	{
		editor_modal_component_add_t& modal = *static_cast<editor_modal_component_add_t*>(user_data);

		if (modal._refresh_pending)
			modal.rebuild();
	}

	void editor_modal_component_add_t::on_item_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left)
			return;

		editor_modal_component_add_t& modal = *static_cast<editor_modal_component_add_t*>(user_data);
		const auto					  item	= std::find_if(modal._items.begin(), modal._items.end(), [id](const item_t& candidate) { return candidate.tile == id; });

		modal._selected = item->type;
		modal.refresh_selection();
	}

	void editor_modal_component_add_t::on_button_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left)
			return;

		editor_modal_component_add_t& modal = *static_cast<editor_modal_component_add_t*>(user_data);

		if (id == modal._add.get_root())
			on_add(user_data);
		else
			on_cancel(user_data);
	}

	void editor_modal_component_add_t::on_add(void* user_data)
	{
		editor_modal_component_add_t& modal = *static_cast<editor_modal_component_add_t*>(user_data);

		if (modal._refresh_pending)
			modal.rebuild();

		if (modal._selected == 0)
			return;

		modal.close();

		frame_vector_t<entity_id_t> entities = {};
		entities.assign(modal._entities.begin(), modal._entities.end());
		editor_commands_component_t::add(modal._world, entities, modal._selected);
	}

	void editor_modal_component_add_t::on_cancel(void* user_data)
	{
		static_cast<editor_modal_component_add_t*>(user_data)->close();
	}
}
