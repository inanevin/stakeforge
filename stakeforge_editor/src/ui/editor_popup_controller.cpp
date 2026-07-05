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
#include "ui/editor_popup_controller.hpp"
#include "assets/editor_asset_manager.hpp"
#include "editor_app.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"

#include <sfg/data/frame_string.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>

namespace sfg
{
	namespace
	{
		constexpr u32 POPUP_FG_DRAW_ORDER	   = 61000u;
		constexpr u32 POPUP_DRAW_ORDER		   = 61001u;
		constexpr u32 ASSET_POPUP_VISIBLE_ROWS = 8u;
#define ASSET_POPUP_MAX_WIDTH		520.0f
#define COLOR_WHEEL_POPUP_MIN_WIDTH 280.0f

		editor_popup_controller_t* s_controllers[editor_popup_controller_t::MAX_CONTROLLERS] = {};
		u32						   s_controller_count										 = 0;

		void set_focusable_widget_visible(ui::layout_tree_t& tree, ui::widget_id_t id, bool visible, bool input)
		{
			ui::layout_in_t& in = tree.in(id);
			in.flags			= visible ? static_cast<u16>(ui::wf_visible | (input ? ui::wf_input : 0) | ui::wf_focusable) : 0;
		}

		void set_rect(ui::paint_layer_t& paint, ui::widget_id_t id, const vec4f_t& fill, f32 rounding = 0.0f)
		{
			ui::vg_rect_paint_t rect = {};
			rect.fill_color_a		 = fill;
			rect.fill_color_b		 = fill;
			rect.rounding			 = rounding;
			paint.set_rect(id, rect);
		}
	}

	void editor_popup_controller_t::init(ui::ui_context& ui)
	{
		SFG_ASSERT(_ui == nullptr);
		SFG_ASSERT(s_controller_count < MAX_CONTROLLERS);

		_ui							= &ui;
		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_foreground = ui.allocate_widget();
		ui.set_widget_debug_name(_foreground, "popup_foreground");
		tree.attach(ui.get_root(), _foreground);
		tree.draw_order(_foreground) = POPUP_FG_DRAW_ORDER;

		ui::layout_in_t& foreground_in = tree.in(_foreground);
		foreground_in.flags			   = 0;
		foreground_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		foreground_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		foreground_in.size_value	   = {1.0f, 1.0f};

		_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_frame, "popup_frame");
		tree.attach(_foreground, _frame);
		tree.draw_order(_frame) = POPUP_DRAW_ORDER;

		ui::layout_in_t& frame_in = tree.in(_frame);
		frame_in.flags			  = 0;
		frame_in.pos_mode_x		  = ui::pos_mode_e::absolute_screen;
		frame_in.pos_mode_y		  = ui::pos_mode_e::absolute_screen;
		frame_in.size_mode_x	  = ui::axis_mode_e::fixed;
		frame_in.size_mode_y	  = ui::axis_mode_e::sum_children;
		frame_in.flow			  = ui::flow_e::column;
		frame_in.child_spacing	  = 0.0f;
		frame_in.child_margins	  = {theme.margin_vertical, 0.0f, theme.margin_vertical, 0.0f};

		ui::vg_rect_paint_t frame_rect = {};
		frame_rect.fill_color_a		   = theme.color_frame_light;
		frame_rect.fill_color_b		   = theme.color_frame_light;
		frame_rect.outline_color	   = theme.color_outline_light;
		frame_rect.outline_thickness   = theme.outline_thickness;
		paint.set_rect(_frame, frame_rect);

		ui::listener_bundle_t row_listener = {};
		row_listener.user_data			   = this;
		row_listener.on_click			   = on_row_click;
		row_listener.on_key				   = on_row_key;

		for (u32 i = 0; i < MAX_ITEMS; ++i)
		{
			const f32 row_margin = theme.outline_thickness;

			_row_frames[i] = ui.allocate_widget();
			ui.set_widget_debug_name(_row_frames[i], "popup_item");
			tree.attach(_frame, _row_frames[i]);

			ui::layout_in_t& row_in = tree.in(_row_frames[i]);
			row_in.flags			= 0;
			row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
			row_in.size_mode_y		= ui::axis_mode_e::fixed;
			row_in.size_value		= {1.0f, theme.item_height + row_margin * 2.0f};
			row_in.flow				= ui::flow_e::none;
			row_in.child_spacing	= 0.0f;
			row_in.child_margins	= {row_margin, row_margin, row_margin, row_margin};

			ui::vg_rect_paint_t row_rect = {};
			row_rect.fill_color_a		 = {0.0f, 0.0f, 0.0f, 0.0f};
			row_rect.fill_color_b		 = {0.0f, 0.0f, 0.0f, 0.0f};
			paint.set_rect(_row_frames[i], row_rect);
			ui.get_input().set_listener(_row_frames[i], row_listener);

			_row_inner_frames[i] = ui.allocate_widget();
			ui.set_widget_debug_name(_row_inner_frames[i], "popup_item_inner");
			tree.attach(_row_frames[i], _row_inner_frames[i]);

			ui::layout_in_t& inner_in = tree.in(_row_inner_frames[i]);
			inner_in.flags			  = 0;
			inner_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
			inner_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
			inner_in.size_value		  = {1.0f, 1.0f};
			inner_in.flow			  = ui::flow_e::row;
			inner_in.child_spacing	  = 0.0f;
			inner_in.child_margins	  = {0.0f, theme.margin_horizontal, 0.0f, 0.0f};

			paint.set_rect(_row_inner_frames[i], row_rect);
			paint.set_hover_color(_row_inner_frames[i], theme.color_panel);
			paint.set_press_color(_row_inner_frames[i], theme.color_frame_light);
			paint.set_focus_color(_row_inner_frames[i], theme.color_accent0);
			paint.set_state_source(_row_inner_frames[i], _row_frames[i]);

			_row_markers[i] = ui.allocate_widget();
			ui.set_widget_debug_name(_row_markers[i], "popup_selected_marker");
			tree.attach(_row_inner_frames[i], _row_markers[i]);

			ui::layout_in_t& marker_in = tree.in(_row_markers[i]);
			marker_in.flags			   = 0;
			marker_in.size_mode_x	   = ui::axis_mode_e::fixed;
			marker_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
			marker_in.size_value	   = {theme.item_height, 1.0f};

			_row_marker_labels[i] = ui.allocate_widget();
			ui.set_widget_debug_name(_row_marker_labels[i], "popup_selected_marker_icon");
			tree.attach(_row_markers[i], _row_marker_labels[i]);

			ui::layout_in_t& marker_label_in = tree.in(_row_marker_labels[i]);
			marker_label_in.flags			 = 0;
			marker_label_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
			marker_label_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
			marker_label_in.pos_value		 = {0.5f, 0.5f};
			marker_label_in.anchor_x		 = ui::anchor_e::center;
			marker_label_in.anchor_y		 = ui::anchor_e::center;
			ui.set_widget_text(_row_marker_labels[i], ICON_FILLED_CIRCLE);
			paint.set_text(_row_marker_labels[i],
						   ui.widget_text(_row_marker_labels[i]),
						   ui.widget_text_len(_row_marker_labels[i]),
						   {.font = theme.font_icons, .color = theme.color_accent0, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

			_row_labels[i] = ui.allocate_widget();
			ui.set_widget_debug_name(_row_labels[i], "popup_item_label");
			tree.attach(_row_inner_frames[i], _row_labels[i]);

			ui::layout_in_t& label_in = tree.in(_row_labels[i]);
			label_in.flags			  = 0;
			label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
			label_in.pos_value.y	  = 0.5f;
			label_in.anchor_y		  = ui::anchor_e::center;
			paint.set_text(_row_labels[i], nullptr, 0, {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		}

		_asset_label_row = ui.allocate_widget();
		ui.set_widget_debug_name(_asset_label_row, "asset_popup_label_row");
		tree.attach(_frame, _asset_label_row);

		ui::layout_in_t& asset_label_row_in = tree.in(_asset_label_row);
		asset_label_row_in.flags			= 0;
		asset_label_row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		asset_label_row_in.size_mode_y		= ui::axis_mode_e::fixed;
		asset_label_row_in.size_value		= {1.0f, theme.item_area_height};
		asset_label_row_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		_asset_label = ui.allocate_widget();
		ui.set_widget_debug_name(_asset_label, "asset_popup_label");
		tree.attach(_asset_label_row, _asset_label);

		ui::layout_in_t& asset_label_in = tree.in(_asset_label);
		asset_label_in.flags			= 0;
		asset_label_in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
		asset_label_in.pos_value.y		= 0.5f;
		asset_label_in.anchor_y			= ui::anchor_e::center;
		ui.set_widget_text(_asset_label, "Select a resource");
		paint.set_text(_asset_label,
					   ui.widget_text(_asset_label),
					   ui.widget_text_len(_asset_label),
					   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		_asset_search_row = ui.allocate_widget();
		ui.set_widget_debug_name(_asset_search_row, "asset_popup_search_row");
		tree.attach(_frame, _asset_search_row);

		ui::layout_in_t& asset_search_row_in = tree.in(_asset_search_row);
		asset_search_row_in.flags			 = 0;
		asset_search_row_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		asset_search_row_in.size_mode_y		 = ui::axis_mode_e::fixed;
		asset_search_row_in.size_value		 = {1.0f, theme.item_area_height};
		asset_search_row_in.child_margins	 = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		u8*							asset_search_text_field = reinterpret_cast<u8*>(&_asset_search_text);
		editor_input_field_config_t asset_search_config		= {};
		asset_search_config.placeholder						= "Search";
		asset_search_config.field							= {
									  .fields = {.data = &asset_search_text_field, .size = 1},
									  .type	  = editor_input_field_field_type_e::string,
		  };
		asset_search_config.callbacks.edited	= on_asset_search_changed;
		asset_search_config.callbacks.user_data = this;
		_asset_search_input.init(ui, _asset_search_row, asset_search_config);

		ui::layout_in_t& asset_search_in = tree.in(_asset_search_input.get_root());
		asset_search_in.pos_mode_x		 = ui::pos_mode_e::relative_in_parent;
		asset_search_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		asset_search_in.pos_value		 = {0.5f, 0.5f};
		asset_search_in.anchor_x		 = ui::anchor_e::center;
		asset_search_in.anchor_y		 = ui::anchor_e::center;
		asset_search_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		asset_search_in.size_mode_y		 = ui::axis_mode_e::fixed;
		asset_search_in.size_value		 = {1.0f, theme.item_height};

		_assets_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_assets_frame, "asset_popup_assets_frame");
		tree.attach(_frame, _assets_frame);

		ui::layout_in_t& assets_frame_in = tree.in(_assets_frame);
		assets_frame_in.flags			 = 0;
		assets_frame_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		assets_frame_in.size_mode_y		 = ui::axis_mode_e::fixed;
		assets_frame_in.size_value		 = {1.0f, theme.item_height};
		assets_frame_in.flow			 = ui::flow_e::column;
		assets_frame_in.child_spacing	 = 0.0f;
		assets_frame_in.child_clip_mode	 = ui::clip_mode_e::scissor_rect;
		ui.set_pre_layout_tick(_assets_frame, on_asset_list_tick, this);

		editor_scrollbar_config_t scrollbar_config = {};
		scrollbar_config.target					   = _assets_frame;
		scrollbar_config.axes					   = editor_scrollbar_axis_y;
		_asset_scrollbar.init(ui, scrollbar_config);

		u8*							input_text_field = reinterpret_cast<u8*>(&_input_text);
		editor_input_field_config_t input_config	 = {};
		input_config.field							 = {
									  .fields = {.data = &input_text_field, .size = 1},
									  .type	  = editor_input_field_field_type_e::string,
		  };
		input_config.callbacks.edit_submitted = on_input_submitted;
		input_config.callbacks.user_data	  = this;
		_input.init(ui, _foreground, input_config);
		color_t* color_wheel_color = &_color_wheel_dummy_color;
		_color_wheel.init(ui, _frame, {.field = {.fields = {.data = &color_wheel_color, .size = 1}}});

		s_controllers[s_controller_count++] = this;
		set_visible(false);
	}

	void editor_popup_controller_t::uninit()
	{
		_ui->cancel_mutations(this);
		close_popup(false);
		destroy_asset_rows();
		_asset_scrollbar.uninit();
		_asset_search_input.uninit();
		_color_wheel.uninit();
		_input.uninit();
		_ui->deallocate_widget(_foreground);

		for (u32 i = 0; i < s_controller_count; ++i)
		{
			if (s_controllers[i] == this)
			{
				s_controllers[i]					  = s_controllers[s_controller_count - 1];
				s_controllers[s_controller_count - 1] = nullptr;
				s_controller_count--;
				break;
			}
		}

		_ui				  = nullptr;
		_foreground		  = NULL_WIDGET;
		_frame			  = NULL_WIDGET;
		_asset_label_row  = NULL_WIDGET;
		_asset_label	  = NULL_WIDGET;
		_asset_search_row = NULL_WIDGET;
		_assets_frame	  = NULL_WIDGET;
		_desc			  = {};
		_input_desc		  = {};
		_asset_desc		  = {};
		_entity_desc	  = {};
		_color_wheel_desc = {};
		_mode			  = popup_mode_e::none;
		_visible		  = false;
		for (u32 i = 0; i < MAX_ITEMS; ++i)
		{
			_row_frames[i]		  = NULL_WIDGET;
			_row_inner_frames[i]  = NULL_WIDGET;
			_row_markers[i]		  = NULL_WIDGET;
			_row_marker_labels[i] = NULL_WIDGET;
			_row_labels[i]		  = NULL_WIDGET;
			_items[i]			  = {};
		}
		_asset_items.resize(0);
		_asset_filtered_items.resize(0);
		_asset_rows.resize(0);
		_input_text.resize(0);
		_asset_search_text.resize(0);
		_color_wheel_dummy_color = {};
		_color_wheel_size		 = {};
	}

	void editor_popup_controller_t::request_popup(const editor_popup_desc_t& desc)
	{
		SFG_ASSERT(_ui != nullptr);
		SFG_ASSERT(desc.items != nullptr);
		SFG_ASSERT(desc.item_count <= MAX_ITEMS);

		if (!can_mutate_ui_topology())
		{
			_pending_desc = desc;
			for (u32 i = 0; i < MAX_ITEMS; ++i)
				_pending_items[i] = i < desc.item_count ? desc.items[i] : editor_popup_item_desc_t{};
			_pending_desc.items = _pending_items;
			defer_request(pending_request_e::items);
			return;
		}

		close_popup(false);
		_desc = desc;
		_mode = popup_mode_e::items;
		for (u32 i = 0; i < MAX_ITEMS; ++i)
			_items[i] = i < desc.item_count ? desc.items[i] : editor_popup_item_desc_t{};

		refresh_rows();
		refresh_layout();
		set_visible(true);

		ui::widget_id_t popup_roots[] = {_frame};
		_ui->get_input().set_popup_scope(_frame, popup_roots, 1, on_popup_outside, this);
	}

	void editor_popup_controller_t::request_input_popup(const editor_input_popup_desc_t& desc)
	{
		SFG_ASSERT(_ui != nullptr);

		if (!can_mutate_ui_topology())
		{
			_pending_input_desc = desc;
			defer_request(pending_request_e::input);
			return;
		}

		close_popup(false);
		_input_desc = desc;
		_mode		= popup_mode_e::input;
		_input_text = desc.text != nullptr ? desc.text : "";
		_input.refresh_field_data();

		refresh_layout();
		set_visible(true);
		_ui->get_input().set_focus(_input.get_root(), false);
		_input.select_all();

		ui::widget_id_t popup_roots[] = {_input.get_root()};
		_ui->get_input().set_popup_scope(_input.get_root(), popup_roots, 1, on_popup_outside, this);
	}

	void editor_popup_controller_t::request_asset_popup(const editor_asset_popup_desc_t& desc)
	{
		SFG_ASSERT(_ui != nullptr);
		SFG_ASSERT(desc.asset_type != editor_asset_type_e::invalid);

		if (!can_mutate_ui_topology())
		{
			_pending_asset_desc = desc;
			defer_request(pending_request_e::assets);
			return;
		}

		close_popup(false);
		_asset_desc = desc;
		_mode		= popup_mode_e::assets;
		_ui->set_widget_text(_asset_label, "Select a resource");
		const editor_theme_t& theme = editor_theme_t::get();
		_ui->get_paint().set_text(_asset_label,
								  _ui->widget_text(_asset_label),
								  _ui->widget_text_len(_asset_label),
								  {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		_asset_search_text.resize(0);
		_asset_search_input.refresh_field_data();
		collect_asset_items();
		filter_asset_items();
		refresh_asset_rows();
		refresh_layout();
		begin_asset_scroll_to_selected();
		set_visible(true);

		ui::widget_id_t popup_roots[] = {_frame, _asset_scrollbar.get_root()};
		_ui->get_input().set_popup_scope(_frame, popup_roots, 2, on_popup_outside, this);
	}

	void editor_popup_controller_t::request_entity_popup(const editor_entity_popup_desc_t& desc)
	{
		SFG_ASSERT(_ui != nullptr);
		SFG_ASSERT(!desc.world.is_null());

		if (!can_mutate_ui_topology())
		{
			_pending_entity_desc = desc;
			defer_request(pending_request_e::entities);
			return;
		}

		close_popup(false);
		_entity_desc = desc;
		_mode		 = popup_mode_e::entities;
		_ui->set_widget_text(_asset_label, "Select an entity");
		const editor_theme_t& theme = editor_theme_t::get();
		_ui->get_paint().set_text(_asset_label,
								  _ui->widget_text(_asset_label),
								  _ui->widget_text_len(_asset_label),
								  {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		_asset_search_text.resize(0);
		_asset_search_input.refresh_field_data();
		collect_entity_items();
		filter_asset_items();
		refresh_asset_rows();
		refresh_layout();
		begin_asset_scroll_to_selected();
		set_visible(true);

		ui::widget_id_t popup_roots[] = {_frame, _asset_scrollbar.get_root()};
		_ui->get_input().set_popup_scope(_frame, popup_roots, 2, on_popup_outside, this);
	}

	void editor_popup_controller_t::request_color_wheel_popup(const editor_color_wheel_popup_desc_t& desc)
	{
		SFG_ASSERT(_ui != nullptr);

		if (!can_mutate_ui_topology())
		{
			_pending_color_wheel_desc = desc;
			defer_request(pending_request_e::color_wheel);
			return;
		}

		close_popup(false);
		_color_wheel_desc = desc;
		_mode			  = popup_mode_e::color_wheel;
		if (desc.fields.size > 0)
		{
			_color_wheel.update_config({
				.field			 = {.fields = desc.fields},
				.edit_begin		 = desc.edit_begin,
				.on_data_changed = desc.on_data_changed,
				.user_data		 = desc.user_data,
			});
		}
		else
		{
			color_t* color_wheel_color = &_color_wheel_dummy_color;
			_color_wheel.update_config({
				.field			 = {.fields = {.data = &color_wheel_color, .size = 1}},
				.edit_begin		 = desc.edit_begin,
				.on_data_changed = desc.on_data_changed,
				.user_data		 = desc.user_data,
			});
		}

		const ui::layout_tree_t& tree	  = _ui->get_tree();
		const ui::layout_out_t&	 screen	  = tree.out(tree.get_root());
		const editor_theme_t&	 theme	  = editor_theme_t::get();
		const f32				 scale	  = _ui->get_ui_scale() > 0.0f ? _ui->get_ui_scale() : 1.0f;
		const f32				 screen_w = screen.clip.z / scale;
		const f32				 max_w	  = math::max(theme.item_width, screen_w - theme.margin_horizontal * 2.0f);
		_color_wheel_size				  = {math::min(math::max(screen_w * 0.2f, COLOR_WHEEL_POPUP_MIN_WIDTH), max_w), editor_widget_color_wheel_t::calculate_min_height()};

		refresh_layout();
		set_visible(true);

		ui::widget_id_t popup_roots[] = {_frame};
		_ui->get_input().set_popup_scope(_frame, popup_roots, 1, on_popup_outside, this);
	}

	void editor_popup_controller_t::close_popup(bool notify_input)
	{
		if (_ui == nullptr || !_visible)
			return;

		if (!can_mutate_ui_topology())
		{
			_pending_close_notify_input = notify_input;
			defer_request(pending_request_e::close);
			return;
		}

		const bool						   notify_input_closed	  = notify_input && _mode == popup_mode_e::input && _input_desc.closed != nullptr;
		const editor_popup_input_closed_fn input_closed			  = _input_desc.closed;
		void*							   input_closed_user_data = _input_desc.user_data;
		const frame_string_t<char>		   input_value			  = _input_text.c_str();
		editor_popup_closed_fn			   popup_closed			  = nullptr;
		void*							   popup_closed_user_data = nullptr;
		switch (_mode)
		{
		case popup_mode_e::items:
			popup_closed		   = _desc.closed;
			popup_closed_user_data = _desc.user_data;
			break;
		case popup_mode_e::assets:
			popup_closed		   = _asset_desc.closed;
			popup_closed_user_data = _asset_desc.user_data;
			break;
		case popup_mode_e::entities:
			popup_closed		   = _entity_desc.closed;
			popup_closed_user_data = _entity_desc.user_data;
			break;
		case popup_mode_e::color_wheel:
			popup_closed		   = _color_wheel_desc.closed;
			popup_closed_user_data = _color_wheel_desc.user_data;
			break;
		default:
			break;
		}

		set_visible(false);
		_ui->get_input().clear_popup_scope();
		_desc						 = {};
		_input_desc					 = {};
		_asset_desc					 = {};
		_entity_desc				 = {};
		_color_wheel_desc			 = {};
		_mode						 = popup_mode_e::none;
		_asset_scroll_pending_frames = 0;
		_color_wheel_size			 = {};
		for (editor_popup_item_desc_t& item : _items)
			item = {};
		_asset_items.resize(0);
		_asset_filtered_items.resize(0);

		if (notify_input_closed)
			input_closed(input_value.c_str(), input_closed_user_data);
		if (popup_closed != nullptr)
			popup_closed(popup_closed_user_data);
	}

	editor_popup_controller_t* editor_popup_controller_t::find(ui::ui_context& ui)
	{
		for (u32 i = 0; i < s_controller_count; ++i)
		{
			if (s_controllers[i]->_ui == &ui)
				return s_controllers[i];
		}
		return nullptr;
	}

	void editor_popup_controller_t::set_visible(bool visible)
	{
		_visible				= visible;
		ui::layout_tree_t& tree = _ui->get_tree();
		tree.set_visible(_foreground, visible, false);
		tree.set_visible(_frame, visible && (_mode == popup_mode_e::items || _mode == popup_mode_e::assets || _mode == popup_mode_e::entities || _mode == popup_mode_e::color_wheel), false);
		for (u32 i = 0; i < MAX_ITEMS; ++i)
		{
			const bool item_visible	  = visible && _mode == popup_mode_e::items && i < _desc.item_count;
			const bool marker_visible = item_visible && _items[i].selected;
			set_focusable_widget_visible(tree, _row_frames[i], item_visible, item_visible);
			tree.set_visible(_row_inner_frames[i], item_visible, false);
			tree.set_visible(_row_markers[i], item_visible, false);
			tree.set_visible(_row_marker_labels[i], marker_visible, false);
			tree.set_visible(_row_labels[i], item_visible, false);
		}
		const bool search_popup_visible = visible && (_mode == popup_mode_e::assets || _mode == popup_mode_e::entities);
		tree.set_visible(_asset_label_row, search_popup_visible, false);
		tree.set_visible(_asset_label, search_popup_visible, false);
		tree.set_visible(_asset_search_row, search_popup_visible, false);
		_asset_search_input.set_visible(search_popup_visible);
		ui::layout_in_t& assets_frame_in = tree.in(_assets_frame);
		assets_frame_in.flags			 = search_popup_visible ? static_cast<u16>(ui::wf_visible | ui::wf_input | ui::wf_scroll_y) : 0;
		for (size_t i = 0; i < _asset_rows.size(); ++i)
		{
			const asset_row_t& row		   = _asset_rows[i];
			const bool		   row_visible = search_popup_visible && i < _asset_filtered_items.size();
			set_focusable_widget_visible(tree, row.root, row_visible, row_visible);
			tree.set_visible(row.inner, row_visible, false);
			tree.set_visible(row.marker, row_visible, false);
			tree.set_visible(row.marker_icon, row_visible, false);
			tree.set_visible(row.thumbnail, row_visible && _mode == popup_mode_e::assets, false);
			tree.set_visible(row.label, row_visible, false);
		}
		_input.set_visible(visible && _mode == popup_mode_e::input);
		_color_wheel.set_visible(visible && _mode == popup_mode_e::color_wheel);
	}

	void editor_popup_controller_t::refresh_rows()
	{
		ui::paint_layer_t&	  paint = _ui->get_paint();
		const editor_theme_t& theme = editor_theme_t::get();
		for (u32 i = 0; i < _desc.item_count; ++i)
		{
			_ui->set_widget_text(_row_labels[i], _items[i].text);
			paint.set_text(_row_labels[i],
						   _ui->widget_text(_row_labels[i]),
						   _ui->widget_text_len(_row_labels[i]),
						   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
			paint.def(_row_marker_labels[i]).text.color = theme.color_accent0;
			_ui->get_tree().set_visible(_row_marker_labels[i], _visible && _items[i].selected, false);
		}
	}

	void editor_popup_controller_t::refresh_asset_rows()
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		ui::paint_layer_t&	  paint = _ui->get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::listener_bundle_t row_listener = {};
		row_listener.user_data			   = this;
		row_listener.on_click			   = on_asset_row_click;
		row_listener.on_key				   = on_asset_row_key;

		_asset_rows.reserve(_asset_filtered_items.size());
		for (size_t i = 0; i < _asset_filtered_items.size(); ++i)
		{
			const asset_popup_item_t& item = _asset_filtered_items[i];
			if (i >= _asset_rows.size())
			{
				const f32 row_margin = theme.outline_thickness;

				asset_row_t row = {};

				row.root = _ui->allocate_widget();
				_ui->set_widget_debug_name(row.root, "asset_popup_item");
				tree.attach(_assets_frame, row.root);
				tree.draw_order(row.root) = tree.draw_order_const(_assets_frame) + 1;

				ui::layout_in_t& row_in = tree.in(row.root);
				row_in.flags			= 0;
				row_in.size_mode_x		= ui::axis_mode_e::parent_relative;
				row_in.size_mode_y		= ui::axis_mode_e::fixed;
				row_in.size_value		= {1.0f, theme.item_height + row_margin * 2.0f};
				row_in.flow				= ui::flow_e::none;
				row_in.child_spacing	= 0.0f;
				row_in.child_margins	= {row_margin, row_margin, row_margin, row_margin};

				set_rect(paint, row.root, {0.0f, 0.0f, 0.0f, 0.0f});
				_ui->get_input().set_listener(row.root, row_listener);

				row.inner = _ui->allocate_widget();
				_ui->set_widget_debug_name(row.inner, "asset_popup_item_inner");
				tree.attach(row.root, row.inner);

				ui::layout_in_t& inner_in = tree.in(row.inner);
				inner_in.flags			  = 0;
				inner_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
				inner_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
				inner_in.size_value		  = {1.0f, 1.0f};
				inner_in.flow			  = ui::flow_e::row;
				inner_in.child_spacing	  = theme.item_spacing;
				inner_in.child_margins	  = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

				set_rect(paint, row.inner, {0.0f, 0.0f, 0.0f, 0.0f});
				paint.set_hover_color(row.inner, theme.color_panel);
				paint.set_press_color(row.inner, theme.color_frame_light);
				paint.set_focus_color(row.inner, theme.color_accent0);
				paint.set_state_source(row.inner, row.root);

				row.marker = _ui->allocate_widget();
				_ui->set_widget_debug_name(row.marker, "asset_popup_item_selected_marker");
				tree.attach(row.inner, row.marker);

				ui::layout_in_t& marker_in = tree.in(row.marker);
				marker_in.flags			   = 0;
				marker_in.size_mode_x	   = ui::axis_mode_e::fixed;
				marker_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
				marker_in.size_value	   = {theme.item_height, 1.0f};

				row.marker_icon = _ui->allocate_widget();
				_ui->set_widget_debug_name(row.marker_icon, "asset_popup_item_selected_marker_icon");
				tree.attach(row.marker, row.marker_icon);

				ui::layout_in_t& marker_icon_in = tree.in(row.marker_icon);
				marker_icon_in.flags			= 0;
				marker_icon_in.pos_mode_x		= ui::pos_mode_e::relative_in_parent;
				marker_icon_in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
				marker_icon_in.pos_value		= {0.5f, 0.5f};
				marker_icon_in.anchor_x			= ui::anchor_e::center;
				marker_icon_in.anchor_y			= ui::anchor_e::center;
				_ui->set_widget_text(row.marker_icon, ICON_FILLED_CIRCLE);
				paint.set_text(row.marker_icon,
							   _ui->widget_text(row.marker_icon),
							   _ui->widget_text_len(row.marker_icon),
							   {.font = theme.font_icons, .color = {}, .point_size = theme.icon_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

				row.thumbnail = _ui->allocate_widget();
				_ui->set_widget_debug_name(row.thumbnail, "asset_popup_item_thumbnail");
				tree.attach(row.inner, row.thumbnail);

				ui::layout_in_t& thumbnail_in = tree.in(row.thumbnail);
				thumbnail_in.flags			  = 0;
				thumbnail_in.size_mode_x	  = ui::axis_mode_e::copy_other;
				thumbnail_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
				thumbnail_in.size_value		  = {1.0f, 0.75f};
				thumbnail_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
				thumbnail_in.pos_value.y	  = 0.5f;
				thumbnail_in.anchor_y		  = ui::anchor_e::center;
				set_rect(paint, row.thumbnail, {1.0f, 1.0f, 1.0f, 1.0f}, theme.item_rounding);

				row.label = _ui->allocate_widget();
				_ui->set_widget_debug_name(row.label, "asset_popup_item_label");
				tree.attach(row.inner, row.label);

				ui::layout_in_t& label_in = tree.in(row.label);
				label_in.flags			  = 0;
				label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
				label_in.pos_value.y	  = 0.5f;
				label_in.anchor_y		  = ui::anchor_e::center;

				_asset_rows.push_back(row);
			}

			asset_row_t& row					  = _asset_rows[i];
			row.guid							  = item.guid;
			paint.def(row.marker_icon).text.color = item.guid == get_search_selected_value() ? theme.color_accent0 : vec4f_t{};

			_ui->set_widget_text(row.label, item.name.c_str());
			paint.set_text(row.label,
						   _ui->widget_text(row.label),
						   _ui->widget_text_len(row.label),
						   {.font = theme.font_default, .color = theme.color_text0, .point_size = theme.text_default_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});
		}

		for (size_t i = _asset_filtered_items.size(); i < _asset_rows.size(); ++i)
			_asset_rows[i].guid = NULL_SID;
	}

	void editor_popup_controller_t::refresh_layout()
	{
		ui::layout_tree_t&		tree	   = _ui->get_tree();
		const editor_theme_t&	theme	   = editor_theme_t::get();
		const ui::layout_out_t& screen	   = tree.out(tree.get_root());
		const f32				row_margin = theme.outline_thickness;
		const f32				row_height = theme.item_height + row_margin * 2.0f;

		f32 width  = _desc.width;
		f32 height = static_cast<f32>(_desc.item_count) * row_height + theme.margin_vertical * 2.0f;
		if (_mode == popup_mode_e::items)
		{
			for (u32 i = 0; i < _desc.item_count; ++i)
				width = math::max(width, theme.item_height + static_cast<f32>(_ui->widget_text_len(_row_labels[i])) * theme.text_default_px_size * 0.7f + theme.margin_horizontal * 2.0f + row_margin * 2.0f);
		}
		else if (_mode == popup_mode_e::input)
		{
			width  = math::max(_input_desc.width, theme.item_width);
			height = theme.item_height + theme.margin_vertical * 2.0f;
		}
		else if (_mode == popup_mode_e::assets || _mode == popup_mode_e::entities)
		{
			width				   = theme.item_width * 2.0f;
			const u32 visible_rows = math::min(ASSET_POPUP_VISIBLE_ROWS, static_cast<u32>(math::max<size_t>(1, _asset_filtered_items.size())));
			height				   = theme.item_area_height * 2.0f + static_cast<f32>(visible_rows) * row_height + theme.margin_vertical * 2.0f;
			width				   = math::max(width, static_cast<f32>(_ui->widget_text_len(_asset_label)) * theme.text_default_px_size * 0.7f + theme.margin_horizontal * 2.0f);
			for (size_t i = 0; i < _asset_filtered_items.size(); ++i)
			{
				const asset_row_t& row			   = _asset_rows[i];
				const f32		   thumbnail_width = _mode == popup_mode_e::assets ? theme.item_height + theme.item_spacing : 0.0f;
				width = math::max(width, theme.item_height + theme.item_spacing + thumbnail_width + static_cast<f32>(_ui->widget_text_len(row.label)) * theme.text_default_px_size * 0.7f + theme.margin_horizontal * 2.0f + row_margin * 2.0f);
			}

			const f32 scale				= _ui->get_ui_scale() > 0.0f ? _ui->get_ui_scale() : 1.0f;
			const f32 screen_width		= screen.clip.z / scale;
			const f32 max_allowed_width = math::max(theme.item_width, math::min(ASSET_POPUP_MAX_WIDTH, screen_width - theme.margin_horizontal * 2.0f));
			width						= math::min(width, max_allowed_width);
		}
		else if (_mode == popup_mode_e::color_wheel)
		{
			width  = _color_wheel_size.x;
			height = _color_wheel_size.y;
		}

		const f32 scale		= _ui->get_ui_scale() > 0.0f ? _ui->get_ui_scale() : 1.0f;
		const f32 width_px	= width * scale;
		const f32 height_px = height * scale;
		f32		  x = _mode == popup_mode_e::input ? _input_desc.pos.x : _mode == popup_mode_e::assets ? _asset_desc.pos.x : _mode == popup_mode_e::entities ? _entity_desc.pos.x : _mode == popup_mode_e::color_wheel ? _color_wheel_desc.pos.x : _desc.pos.x;
		f32		  y = _mode == popup_mode_e::input ? _input_desc.pos.y : _mode == popup_mode_e::assets ? _asset_desc.pos.y : _mode == popup_mode_e::entities ? _entity_desc.pos.y : _mode == popup_mode_e::color_wheel ? _color_wheel_desc.pos.y : _desc.pos.y;
		if (x + width_px > screen.clip.x + screen.clip.z)
			x = screen.clip.x + screen.clip.z - width_px;
		if (y + height_px > screen.clip.y + screen.clip.w)
			y = (_mode == popup_mode_e::input		  ? _input_desc.pos.y
				 : _mode == popup_mode_e::assets	  ? _asset_desc.pos.y
				 : _mode == popup_mode_e::entities	  ? _entity_desc.pos.y
				 : _mode == popup_mode_e::color_wheel ? _color_wheel_desc.pos.y
													  : _desc.pos.y) -
				height_px;
		x = math::clamp(x, screen.clip.x, math::max(screen.clip.x, screen.clip.x + screen.clip.z - width_px));
		y = math::clamp(y, screen.clip.y, math::max(screen.clip.y, screen.clip.y + screen.clip.w - height_px));

		ui::layout_in_t& frame_in = tree.in(_frame);
		if (_mode == popup_mode_e::input)
		{
			ui::layout_in_t& input_in = tree.in(_input.get_root());
			input_in.pos_mode_x		  = ui::pos_mode_e::absolute_screen;
			input_in.pos_mode_y		  = ui::pos_mode_e::absolute_screen;
			input_in.pos_value		  = {x, y};
			input_in.size_mode_x	  = ui::axis_mode_e::fixed;
			input_in.size_mode_y	  = ui::axis_mode_e::fixed;
			input_in.size_value		  = {width, theme.item_height};
			return;
		}

		frame_in.pos_value	   = {x, y};
		frame_in.size_mode_y   = _mode == popup_mode_e::color_wheel ? ui::axis_mode_e::fixed : ui::axis_mode_e::sum_children;
		frame_in.size_value	   = {width, _mode == popup_mode_e::color_wheel ? height : 0.0f};
		frame_in.child_margins = _mode == popup_mode_e::color_wheel ? vec4f_t{theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal} : vec4f_t{theme.margin_vertical, 0.0f, theme.margin_vertical, 0.0f};
		if (_mode == popup_mode_e::assets || _mode == popup_mode_e::entities)
			tree.in(_assets_frame).size_value.y = height - theme.margin_vertical * 2.0f - theme.item_area_height * 2.0f;
	}

	void editor_popup_controller_t::collect_asset_items()
	{
		_asset_items.resize(0);

		const editor_asset_manager_t& asset_manager = editor_asset_manager_t::get();
		const editor_asset_tree_t&	  tree			= asset_manager.get_asset_tree();
		_asset_items.reserve(asset_manager.get_assets().size() + 1);
		_asset_items.push_back({.name = "None", .guid = NULL_SID});

		for (auto it = tree.begin_handle(); it != tree.end_handle(); ++it)
		{
			const editor_asset_node_t& node = tree.value(*it);
			if (node.type != editor_asset_node_type_e::asset)
				continue;

			const editor_asset_t* asset = asset_manager.find_asset(node.asset_id);
			if (asset == nullptr || asset->asset_type != _asset_desc.asset_type)
				continue;

			_asset_items.push_back({.name = node.name, .guid = asset->guid});
		}
	}

	void editor_popup_controller_t::collect_entity_items()
	{
		_asset_items.resize(0);
		_asset_items.push_back({.name = "None", .guid = static_cast<sid_t>(NULL_ENTITY_GUID)});

		const world_t&				   world	   = editor_app_t::get().get_world_controller().get_world(_entity_desc.world);
		const world_component_table_t* alive_table = world.find_component_table(type_id_t<component_alive_t>::value);
		const world_component_table_t* guid_table  = world.find_component_table(type_id_t<component_guid_t>::value);
		const world_component_table_t* name_table  = world.find_component_table(type_id_t<component_name_t>::value);
		SFG_ASSERT(alive_table != nullptr);
		SFG_ASSERT(guid_table != nullptr);
		SFG_ASSERT(name_table != nullptr);

		const ecs_component_table_ref_t table_refs[] = {
			alive_table->table.ref(),
			guid_table->table.ref(),
			name_table->table.ref(),
		};

		for (const ecs_query_row_t& row : ecs_t::inner_join({.data = table_refs, .size = std::size(table_refs)}))
		{
			const component_guid_t& guid = ecs_helpers_t::row_get<component_guid_t>(row, 1);
			const component_name_t& name = ecs_helpers_t::row_get<component_name_t>(row, 2);
			_asset_items.push_back({.name = name.text, .guid = static_cast<sid_t>(guid.guid)});
		}
	}

	void editor_popup_controller_t::filter_asset_items()
	{
		_asset_filtered_items.resize(0);
		_asset_filtered_items.reserve(_asset_items.size());

		const char* query = _asset_search_input.get_text();
		if (query == nullptr || query[0] == '\0')
		{
			for (const asset_popup_item_t& item : _asset_items)
				_asset_filtered_items.push_back(item);
			return;
		}

		frame_string_t<char> query_lower;
		query_lower.assign(query, std::strlen(query));
		string_util::to_lower(query_lower);
		for (const asset_popup_item_t& item : _asset_items)
		{
			frame_string_t<char> name_lower;
			name_lower.assign(item.name.c_str(), item.name.size());
			string_util::to_lower(name_lower);
			if (name_lower.find(query_lower.c_str()) != frame_string_t<char>::npos)
				_asset_filtered_items.push_back(item);
		}
	}

	void editor_popup_controller_t::destroy_asset_rows()
	{
		for (size_t i = _asset_rows.size(); i > 0; --i)
			_ui->deallocate_widget(_asset_rows[i - 1].root);
		_asset_rows.resize(0);
		if (_ui != nullptr && _assets_frame != NULL_WIDGET)
			_ui->get_tree().in(_assets_frame).scroll_offset = {};
	}

	void editor_popup_controller_t::begin_asset_scroll_to_selected()
	{
		_asset_scroll_pending_frames = 0;
		_asset_scroll_target		 = 0;
		_asset_scrollbar.set_scroll_y_immediate(0.0f);
		const sid_t selected = get_search_selected_value();
		for (u32 i = 0; i < _asset_filtered_items.size(); ++i)
		{
			if (_asset_filtered_items[i].guid == selected)
			{
				_asset_scroll_target		 = i;
				_asset_scroll_pending_frames = 2;
				return;
			}
		}
	}

	bool editor_popup_controller_t::can_mutate_ui_topology() const
	{
		const ui::ui_phase_e phase = _ui->get_phase();
		return phase == ui::ui_phase_e::idle || phase == ui::ui_phase_e::mutation || phase == ui::ui_phase_e::pre_layout;
	}

	bool editor_popup_controller_t::defer_request(pending_request_e request)
	{
		_pending_request = request;
		_ui->request_unique_mutation(on_ui_mutation, this);
		return true;
	}

	void editor_popup_controller_t::flush_pending_request()
	{
		const pending_request_e request = _pending_request;
		_pending_request				= pending_request_e::none;

		switch (request)
		{
		case pending_request_e::close:
			close_popup(_pending_close_notify_input);
			_pending_close_notify_input = false;
			return;
		case pending_request_e::items:
			request_popup(_pending_desc);
			return;
		case pending_request_e::input:
			request_input_popup(_pending_input_desc);
			return;
		case pending_request_e::assets:
			request_asset_popup(_pending_asset_desc);
			return;
		case pending_request_e::entities:
			request_entity_popup(_pending_entity_desc);
			return;
		case pending_request_e::color_wheel:
			request_color_wheel_popup(_pending_color_wheel_desc);
			return;
		case pending_request_e::asset_rows:
			refresh_asset_rows();
			refresh_layout();
			_asset_scrollbar.set_scroll_y_immediate(0.0f);
			set_visible(true);
			return;
		default:
			return;
		}
	}

	sid_t editor_popup_controller_t::get_search_selected_value() const
	{
		return _mode == popup_mode_e::entities ? static_cast<sid_t>(_entity_desc.selected) : _asset_desc.selected;
	}

	u32 editor_popup_controller_t::find_row(ui::widget_id_t id) const
	{
		for (u32 i = 0; i < _desc.item_count; ++i)
		{
			if (_row_frames[i] == id)
				return i;
		}
		SFG_ASSERT(false);
		return 0;
	}

	u32 editor_popup_controller_t::find_asset_row(ui::widget_id_t id) const
	{
		for (u32 i = 0; i < _asset_rows.size(); ++i)
		{
			if (_asset_rows[i].root == id)
				return i;
		}
		SFG_ASSERT(false);
		return 0;
	}

	void editor_popup_controller_t::activate_row(u32 row)
	{
		const u16						   value			 = _items[row].id;
		const editor_popup_item_pressed_fn pressed			 = _desc.pressed;
		void*							   pressed_user_data = _desc.user_data;
		const bool						   close_on_pressed	 = _desc.close_on_pressed;
		if (close_on_pressed)
			close_popup();
		if (pressed != nullptr)
			pressed(value, pressed_user_data);
	}

	void editor_popup_controller_t::activate_asset_row(u32 row)
	{
		const sid_t							 value			   = _asset_rows[row].guid;
		const bool							 entity_popup	   = _mode == popup_mode_e::entities;
		const editor_popup_asset_pressed_fn	 asset_pressed	   = _asset_desc.pressed;
		const editor_popup_entity_pressed_fn entity_pressed	   = _entity_desc.pressed;
		void*								 pressed_user_data = entity_popup ? _entity_desc.user_data : _asset_desc.user_data;
		const bool							 close_on_pressed  = entity_popup ? _entity_desc.close_on_pressed : _asset_desc.close_on_pressed;
		if (close_on_pressed)
			close_popup();
		if (entity_popup)
		{
			if (entity_pressed != nullptr)
				entity_pressed(static_cast<entity_guid_t>(value), pressed_user_data);
		}
		else if (asset_pressed != nullptr)
			asset_pressed(value, pressed_user_data);
	}

	void editor_popup_controller_t::on_row_click(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_popup_controller_t& popup = *static_cast<editor_popup_controller_t*>(user_data);
		popup.activate_row(popup.find_row(id));
	}

	void editor_popup_controller_t::on_row_key(ui::input_router_t&, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data)
	{
		if (ev.action != ui::key_action_e::press || ev.key != static_cast<u16>(input_code::key_return))
			return;

		editor_popup_controller_t& popup = *static_cast<editor_popup_controller_t*>(user_data);
		popup.activate_row(popup.find_row(id));
	}

	void editor_popup_controller_t::on_asset_row_click(ui::input_router_t&, ui::widget_id_t id, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_popup_controller_t& popup = *static_cast<editor_popup_controller_t*>(user_data);
		popup.activate_asset_row(popup.find_asset_row(id));
	}

	void editor_popup_controller_t::on_asset_row_key(ui::input_router_t&, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data)
	{
		if (ev.action != ui::key_action_e::press || ev.key != static_cast<u16>(input_code::key_return))
			return;

		editor_popup_controller_t& popup = *static_cast<editor_popup_controller_t*>(user_data);
		popup.activate_asset_row(popup.find_asset_row(id));
	}

	void editor_popup_controller_t::on_popup_outside(ui::input_router_t&, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		editor_popup_controller_t& popup = *static_cast<editor_popup_controller_t*>(user_data);
		popup.close_popup(true);
	}

	void editor_popup_controller_t::on_input_submitted(void* user_data)
	{
		static_cast<editor_popup_controller_t*>(user_data)->close_popup(true);
	}

	void editor_popup_controller_t::on_asset_search_changed(void* user_data)
	{
		editor_popup_controller_t& popup = *static_cast<editor_popup_controller_t*>(user_data);
		popup.filter_asset_items();
		if (!popup.can_mutate_ui_topology())
		{
			popup.defer_request(pending_request_e::asset_rows);
			return;
		}
		popup.refresh_asset_rows();
		popup._asset_scrollbar.set_scroll_y_immediate(0.0f);
		popup.set_visible(true);
	}

	void editor_popup_controller_t::on_asset_list_tick(ui::ui_context& ui, ui::widget_id_t, f32, void* user_data)
	{
		editor_popup_controller_t& popup = *static_cast<editor_popup_controller_t*>(user_data);
		if (popup._asset_scroll_pending_frames == 0)
			return;
		if (!popup._visible || (popup._mode != popup_mode_e::assets && popup._mode != popup_mode_e::entities))
		{
			popup._asset_scroll_pending_frames = 0;
			return;
		}
		if (popup._asset_scroll_pending_frames > 1)
		{
			popup._asset_scroll_pending_frames--;
			return;
		}

		const editor_theme_t&	 theme		= editor_theme_t::get();
		const ui::layout_tree_t& tree		= ui.get_tree();
		const ui::layout_out_t&	 list_out	= tree.out(popup._assets_frame);
		const f32				 ui_scale	= ui.get_ui_scale() > 0.0f ? ui.get_ui_scale() : 1.0f;
		const f32				 viewport	= list_out.size.y / ui_scale;
		const f32				 row_height = theme.item_height + theme.outline_thickness * 2.0f;
		const f32				 row_center = (static_cast<f32>(popup._asset_scroll_target) + 0.5f) * row_height;
		const f32				 target		= math::clamp(row_center - viewport * 0.5f, 0.0f, list_out.max_scroll.y);
		popup._asset_scrollbar.set_scroll_y_immediate(-target);
		popup._asset_scroll_pending_frames = 0;
	}

	void editor_popup_controller_t::on_ui_mutation(ui::ui_context&, void* user_data)
	{
		static_cast<editor_popup_controller_t*>(user_data)->flush_pending_request();
	}
}
