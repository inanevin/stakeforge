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

#include "editor_panel_animation_library.hpp"
#include "editor_widget_animation_library_states.hpp"

#include "assets/editor_asset.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"
#include "commands/editor_command_animation_library.hpp"
#include "editor_command_system.hpp"
#include "editor_surface_controller.hpp"
#include "editor_world_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widget_fold.hpp"
#include "ui/widgets/editor_widget_input_field.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_dropdown.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include "world/editor_world.hpp"
#include "world/editor_world_util.hpp"

#include <sfg/io/log.hpp>
#include <sfg/common/hashing.hpp>
#include <sfg/math/math.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/skeleton_def.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define ANIMATION_LIBRARY_SIDE_MIN 0.1f
#define ANIMATION_LIBRARY_MID_MIN  0.2f

	struct editor_panel_animation_library_t::layer_controls_t
	{
		editor_input_field_t			  name	 = {};
		editor_input_field_t			  weight = {};
		editor_dropdown_t				  mask	 = {};
		editor_widget_button_t			  remove = {};
		editor_widget_fold_t			  fold	 = {};
		editor_panel_animation_library_t* panel	 = nullptr;
		u32								  index	 = 0;
	};

	editor_panel_animation_library_t::editor_panel_animation_library_t()
	{
		set_type(editor_panel_type_e::animation_library);
		refresh_title();
		set_icon(ICON_ANIMATION);
	}

	editor_panel_animation_library_t::~editor_panel_animation_library_t() = default;

	void editor_panel_animation_library_t::serialize(nlohmann::json& j) const
	{
		j["library_guid"]	= _library_guid;
		j["asset_name"]		= _asset_name;
		j["left_split"]		= _left_split;
		j["right_split"]	= _right_split;
		j["selected_layer"] = _selected_layer;
	}

	void editor_panel_animation_library_t::deserialize(const nlohmann::json& j)
	{
		_library_guid	= j.value<sid_t>("library_guid", NULL_SID);
		_asset_name		= j.value<string_t>("asset_name", {});
		_left_split		= math::clamp(j.value<f32>("left_split", _left_split), ANIMATION_LIBRARY_SIDE_MIN, 1.0f - ANIMATION_LIBRARY_SIDE_MIN - ANIMATION_LIBRARY_MID_MIN);
		_right_split	= math::clamp(j.value<f32>("right_split", _right_split), _left_split + ANIMATION_LIBRARY_MID_MIN, 1.0f - ANIMATION_LIBRARY_SIDE_MIN);
		_selected_layer = j.value<u32>("selected_layer", UINT32_MAX);

		set_sub_item_id(_library_guid);
		refresh_title(_asset_name.c_str(), "AL: ");
	}

	void editor_panel_animation_library_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		_layer_controls = new layer_controls_t[MAX_ANIMATION_LIBRARY_LAYERS]{};
		_states_widget	= new editor_widget_animation_library_states_t{};

		_asset_deletion_listener = editor_asset_manager_t::get().add_asset_deletion_listener(on_asset_deletion, this);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		tree.in(_root).flow			 = ui::flow_e::row;
		tree.in(_root).child_spacing = 0.0f;

		_left_pane = ui.allocate_widget();
		tree.attach(_root, _left_pane);
		ui.set_widget_debug_name(_left_pane, "animation_library_left_pane");

		ui::layout_in_t& left_in = tree.in(_left_pane);

		left_in.size_mode_x = ui::axis_mode_e::parent_relative;
		left_in.size_mode_y = ui::axis_mode_e::parent_relative;
		left_in.size_value	= {_left_split, 1.0f};
		left_in.flags |= ui::wf_input | ui::wf_scroll_y;
		left_in.child_clip_mode = ui::clip_mode_e::scissor_rect;

		_left_split_border.init(ui, _root, {.on_drag = on_split_border_drag, .user_data = this, .direction = editor_split_border_direction_e::horizontal});

		_mid_pane = ui.allocate_widget();
		tree.attach(_root, _mid_pane);
		ui.set_widget_debug_name(_mid_pane, "animation_library_mid_pane");

		ui::layout_in_t& mid_in = tree.in(_mid_pane);

		mid_in.flow			   = ui::flow_e::none;
		mid_in.size_mode_x	   = ui::axis_mode_e::fill;
		mid_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		mid_in.size_value	   = {1.0f, 1.0f};
		mid_in.child_clip_mode = ui::clip_mode_e::scissor_rect;

		_world_view.init(ui, _mid_pane);
		_right_split_border.init(ui, _root, {.on_drag = on_split_border_drag, .user_data = this, .direction = editor_split_border_direction_e::horizontal});

		for (const ui::widget_id_t border : {_left_split_border.get_root(), _right_split_border.get_root()})
		{
			ui::layout_in_t& border_in = tree.in(border);

			border_in.size_mode_x = ui::axis_mode_e::fixed;
			border_in.size_mode_y = ui::axis_mode_e::parent_relative;
			border_in.size_value  = {theme.border_thickness * 2.0f, 1.0f};
		}

		_right_pane = ui.allocate_widget();
		tree.attach(_root, _right_pane);
		ui.set_widget_debug_name(_right_pane, "animation_library_right_pane");

		ui::layout_in_t& right_in = tree.in(_right_pane);

		right_in.flags |= ui::wf_input | ui::wf_scroll_y;
		right_in.child_clip_mode = ui::clip_mode_e::scissor_rect;
		right_in.size_mode_x	 = ui::axis_mode_e::parent_relative;
		right_in.size_mode_y	 = ui::axis_mode_e::parent_relative;
		right_in.size_value		 = {1.0f - _right_split, 1.0f};

		for (const ui::widget_id_t pane : {_left_pane, _right_pane})
			ui.get_paint().set_rect(pane, {.fill_color_a = theme.color_panel, .fill_color_b = theme.color_panel});

		const ui::widget_id_t left_content = ui.allocate_widget();

		tree.attach(_left_pane, left_content);
		tree.draw_order(left_content) = tree.draw_order_const(_left_pane) + 1;

		ui::layout_in_t& content_in = tree.in(left_content);

		content_in.flow			 = ui::flow_e::column;
		content_in.child_margins = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};
		content_in.size_mode_x	 = ui::axis_mode_e::parent_relative;
		content_in.size_mode_y	 = ui::axis_mode_e::sum_children;
		content_in.size_value	 = {1.0f, 1.0f};

		_left_scrollbar.init(ui, {.target = _left_pane, .axes = editor_scrollbar_axis_y});
		editor_misc_widgets_t::make_section_label(ui, left_content, "Animation Library");

		editor_dividers_t::add_divider_hor(ui, left_content, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);

		const editor_property_row_t skeleton_row   = editor_misc_widgets_t::make_property_row_with_label(ui, left_content, "Skeleton");
		u64*						skeleton_field = &_skeleton_reference_value;

		_skeleton_reference.init(ui,
								 skeleton_row.right,
								 {
									 .callbacks	 = {.edited = on_skeleton_edited, .user_data = this},
									 .fields	 = {.data = &skeleton_field, .size = 1},
									 .asset_type = editor_asset_type_e::skeleton,
								 });

		ui::layout_in_t& reference_in = tree.in(_skeleton_reference.get_root());

		reference_in.size_mode_x = ui::axis_mode_e::fill;
		reference_in.pos_mode_y	 = ui::pos_mode_e::relative_in_parent;
		reference_in.pos_value.y = 0.5f;
		reference_in.anchor_y	 = ui::anchor_e::center;

		editor_dividers_t::add_divider_hor(ui, left_content, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);

		editor_misc_widgets_t::add_spacer(ui, left_content, {0.0f, theme.item_spacing});
		_save_changes_button.init(ui, left_content, {.text = "Save Changes"});
		tree.in(_save_changes_button.get_root()).flags |= ui::wf_disabled;
		tree.in(_skeleton_reference.get_root()).flags |= ui::wf_disabled;
		ui.get_input().set_listener(_save_changes_button.get_root(), {.on_click = on_save_changes_pressed, .user_data = this});

		editor_misc_widgets_t::make_section_label(ui, left_content, "Layers");

		_layer_list = ui.allocate_widget();
		tree.attach(left_content, _layer_list);
		ui.set_widget_debug_name(_layer_list, "animation_library_layers");

		ui::layout_in_t& layers_in = tree.in(_layer_list);

		layers_in.flow			= ui::flow_e::column;
		layers_in.size_mode_x	= ui::axis_mode_e::parent_relative;
		layers_in.size_mode_y	= ui::axis_mode_e::sum_children;
		layers_in.size_value	= {1.0f, 1.0f};
		layers_in.child_spacing = theme.item_spacing;
		layers_in.flags |= ui::wf_disabled;

		editor_misc_widgets_t::add_spacer(ui, left_content, {0.0f, theme.item_spacing});
		_add_layer_button.init(ui, left_content, {.text = "Add Layer"});
		tree.in(_add_layer_button.get_root()).flags |= ui::wf_disabled;
		ui.get_input().set_listener(_add_layer_button.get_root(), {.on_click = on_layer_add_pressed, .user_data = this});

		_right_content = ui.allocate_widget();
		tree.attach(_right_pane, _right_content);
		tree.draw_order(_right_content) = tree.draw_order_const(_right_pane) + 1;

		ui::layout_in_t& states_in = tree.in(_right_content);

		states_in.flow			= ui::flow_e::column;
		states_in.child_margins = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};
		states_in.size_mode_x	= ui::axis_mode_e::parent_relative;
		states_in.size_mode_y	= ui::axis_mode_e::sum_children;
		states_in.size_value	= {1.0f, 1.0f};

		editor_misc_widgets_t::make_section_label(ui, _right_content, "States");
		_states_widget->init(ui, _right_content, *this);
		_right_scrollbar.init(ui, {.target = _right_pane, .axes = editor_scrollbar_axis_y});
		ui.set_pre_layout_tick(_root, on_refresh_tick, this);

		_missing_skeleton_frame = ui.allocate_widget();
		tree.attach(_mid_pane, _missing_skeleton_frame);
		ui.set_widget_debug_name(_missing_skeleton_frame, "animation_library_missing_skeleton");
		tree.draw_order(_missing_skeleton_frame) = tree.draw_order_const(_world_view.get_view_widget()) + 3;

		ui::layout_in_t& frame_in = tree.in(_missing_skeleton_frame);

		frame_in.flags |= ui::wf_overlay | ui::wf_input;
		frame_in.size_mode_x = ui::axis_mode_e::parent_relative;
		frame_in.size_mode_y = ui::axis_mode_e::parent_relative;
		frame_in.size_value	 = {1.0f, 1.0f};

		ui.get_paint().set_rect(_missing_skeleton_frame, {.fill_color_a = {0.0f, 0.0f, 0.0f, 0.6f}, .fill_color_b = {0.0f, 0.0f, 0.0f, 0.6f}});

		const ui::widget_id_t label = ui.allocate_widget();

		tree.attach(_missing_skeleton_frame, label);

		ui::layout_in_t& label_in = tree.in(label);

		label_in.pos_mode_x = ui::pos_mode_e::relative_in_parent;
		label_in.pos_mode_y = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value	= {0.5f, 0.5f};
		label_in.anchor_x	= ui::anchor_e::center;
		label_in.anchor_y	= ui::anchor_e::center;

		ui.set_widget_text(label, "Please assign a skeleton");
		ui.get_paint().set_text(label,
								ui.widget_text(label),
								ui.widget_text_len(label),
								{
									.font		 = theme.font_title_bold,
									.color		 = theme.color_text0,
									.point_size	 = theme.text_med_title_px_size,
									.raster_mode = editor_text_rasterization_t::get_rasterization_type(),
								});

		create_preview_world();

		if (_library_guid != NULL_SID)
			set_library(_library_guid, _asset_name.c_str());
		else
			refresh_skeleton_overlay();

		apply_pane_splits();
	}

	void editor_panel_animation_library_t::uninit()
	{
		_ui->clear_pre_layout_tick(_root);
		_ui->get_input().set_focus(_root, false);
		finish_edit();
		clear_layer_controls();
		_states_widget->uninit();
		delete _states_widget;
		_states_widget = nullptr;

		delete[] _layer_controls;
		_layer_controls = nullptr;

		_add_layer_button.uninit();
		_left_scrollbar.uninit();

		editor_command_system_t::get().clear_user_data(this);
		editor_asset_manager_t::get().remove_asset_deletion_listener(_asset_deletion_listener);

		_skeleton_reference.uninit();
		_save_changes_button.uninit();
		_right_scrollbar.uninit();
		_world_view.uninit();
		_left_split_border.uninit();
		_right_split_border.uninit();
		editor_world_controller_t::get().destroy_world(_world);

		_world					 = {};
		_library				 = {};
		_asset_deletion_listener = {};
		_has_skeleton			 = false;
		_refresh_layers			 = false;
		_refresh_states			 = false;
		_refresh_skeleton		 = false;
		std::fill_n(_layer_expanded, MAX_ANIMATION_LIBRARY_LAYERS, false);

		editor_panel_t::uninit();
	}

	void editor_panel_animation_library_t::set_library(sid_t library_guid, const char* asset_name)
	{
		const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(library_guid);

		if (asset == nullptr || asset->asset_type != editor_asset_type_e::animation_library)
		{
			SFG_ERR("animation library asset is missing: {0}", library_guid);
			return;
		}

		animation_library_def_t definition = {};
		const nlohmann::json	source	   = editor_asset_io_t::get_embedded_source_json(*asset);

		if (!reflection_registry_t::get().type_from_json(type_id_t<animation_library_def_t>::value, &definition, nullptr, source))
		{
			SFG_ERR("failed to deserialize animation library asset: {0}", library_guid);
			return;
		}

		_ui->get_input().set_focus(_root, false);
		finish_edit();
		editor_command_system_t::get().clear_user_data(this);

		const u32 selected_layer = _library_guid == library_guid ? _selected_layer : UINT32_MAX;

		if (_library_guid != library_guid)
		{
			std::fill_n(_layer_expanded, MAX_ANIMATION_LIBRARY_LAYERS, false);
			_states_widget->reset();
		}

		if (definition.layer_count == 0)
		{
			definition.layers[0]   = {};
			definition.layer_count = 1;
		}

		for (u32 i = 0; i < definition.layer_count; ++i)
		{
			animation_library_layer_def_t& layer = definition.layers[i];

			if (layer.name[0] == '\0')
				std::snprintf(layer.name, sizeof(layer.name), "Layer %u", i + 1);
		}

		_library_guid = library_guid;
		_asset_name	  = asset_name;

		set_sub_item_id(_library_guid);
		refresh_title(_asset_name.c_str(), "AL: ");
		_ui->get_tree().in(_save_changes_button.get_root()).flags &= ~ui::wf_disabled;
		_ui->get_tree().in(_skeleton_reference.get_root()).flags &= ~ui::wf_disabled;
		apply_edits(definition, selected_layer);
	}

	void editor_panel_animation_library_t::apply_edits(const animation_library_def_t& definition, u32 selected_layer, const bool* layer_expanded, u32 selected_state, u32 selected_clip)
	{
		if (layer_expanded != nullptr && definition.layer_count != _library.layer_count)
			SFG_MEMCPY(_layer_expanded, layer_expanded, sizeof(_layer_expanded));

		_library		= definition;
		_selected_layer = selected_layer < _library.layer_count ? selected_layer : UINT32_MAX;
		_selected_state = _selected_layer != UINT32_MAX && selected_state < _library.layers[_selected_layer].states.size() ? selected_state : UINT32_MAX;
		_selected_clip	= _selected_state != UINT32_MAX && selected_clip < _library.layers[_selected_layer].states[_selected_state].clip_count ? selected_clip : UINT32_MAX;
		_refresh_layers = true;
		_refresh_states = true;

		refresh_skeleton_masks();
		validate_layer_masks();
		refresh_skeleton_reference();
		refresh_skeleton_overlay();
		refresh_scene();
	}

	void editor_panel_animation_library_t::apply_selection(u32 selected_layer, u32 selected_state, u32 selected_clip)
	{
		SFG_ASSERT(selected_layer == UINT32_MAX || selected_layer < _library.layer_count);
		SFG_ASSERT(selected_state == UINT32_MAX || (selected_layer != UINT32_MAX && selected_state < _library.layers[selected_layer].states.size()));
		SFG_ASSERT(selected_clip == UINT32_MAX || (selected_state != UINT32_MAX && selected_clip < _library.layers[selected_layer].states[selected_state].clip_count));

		const bool layer_changed = _selected_layer != selected_layer;

		_selected_layer = selected_layer;
		_selected_state = selected_state;
		_selected_clip	= selected_clip;
		_refresh_states |= layer_changed;
		refresh_layer_selection();
		_states_widget->refresh_selection();
	}

	void editor_panel_animation_library_t::create_preview_world()
	{
		const editor_world_init_config_t config		= editor_world_init_config_t::make_preview(editor_surface_controller_t::get().get_main_surface().swapchain_size);
		editor_world_controller_t&		 controller = editor_world_controller_t::get();

		_world = controller.create_world(config, editor_world_edit_type_e::view_with_debug);

		editor_world_t& editor_world = *controller.get_editor_world(_world);

		editor_world.install_camera(editor_world_camera_type_e::fly);
		editor_world_util_t::install_default_scene_dark(editor_world.get_world(), 5.0f);
		_world_view.set_edit_world(_world);
	}

	void editor_panel_animation_library_t::refresh_scene()
	{
	}

	void editor_panel_animation_library_t::refresh_skeleton_reference()
	{
		_skeleton_reference_value = _library.skeleton;

		u64* skeleton_field = &_skeleton_reference_value;

		_skeleton_reference.set_reference({
			.callbacks	= {.edited = on_skeleton_edited, .user_data = this},
			.fields		= {.data = &skeleton_field, .size = 1},
			.asset_type = editor_asset_type_e::skeleton,
		});
	}

	void editor_panel_animation_library_t::refresh_skeleton_overlay()
	{
		ui::layout_tree_t& tree = _ui->get_tree();

		tree.set_visible(_missing_skeleton_frame, !_has_skeleton);

		for (const ui::widget_id_t widget : {_layer_list, _right_content, _save_changes_button.get_root()})
		{
			if (_has_skeleton)
				tree.in(widget).flags &= ~ui::wf_disabled;
			else
				tree.in(widget).flags |= ui::wf_disabled;
		}

		if (_has_skeleton && _library.layer_count < MAX_ANIMATION_LIBRARY_LAYERS)
			tree.in(_add_layer_button.get_root()).flags &= ~ui::wf_disabled;
		else
			tree.in(_add_layer_button.get_root()).flags |= ui::wf_disabled;
	}

	void editor_panel_animation_library_t::refresh_skeleton_masks()
	{
		for (u32 i = 0; i < _layer_control_count; ++i)
			_layer_controls[i].mask.close();

		_mask_names.resize(0);
		_mask_items.resize(0);
		_mask_items.push_back({.text = "None", .value = NULL_SID});
		_has_skeleton = false;

		const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(_library.skeleton);

		if (asset != nullptr && asset->asset_type == editor_asset_type_e::skeleton)
		{
			skeleton_def_t		 skeleton = {};
			const nlohmann::json source	  = editor_asset_io_t::get_embedded_source_json(*asset);

			if (reflection_registry_t::get().type_from_json(type_id_t<skeleton_def_t>::value, &skeleton, nullptr, source))
			{
				_has_skeleton = true;
				_mask_names.reserve(skeleton.masks.size());
				_mask_items.reserve(skeleton.masks.size() + 1);

				for (const skeleton_mask_def_t& mask : skeleton.masks)
					_mask_names.emplace_back(mask.name);

				for (const string_t& name : _mask_names)
					_mask_items.push_back({.text = name.c_str(), .value = TO_SID(name.c_str())});
			}
			else
				SFG_ERR("failed to read skeleton masks: {0}", _library.skeleton);
		}

		for (u32 i = 0; i < _layer_control_count; ++i)
			_layer_controls[i].mask.set_items(_mask_items.data(), static_cast<u16>(_mask_items.size()));
	}

	void editor_panel_animation_library_t::validate_layer_masks()
	{
		for (size_t i = 0; i < _library.layer_count; ++i)
		{
			animation_library_layer_def_t& layer = _library.layers[i];
			const auto					   mask	 = std::find_if(_mask_items.begin(), _mask_items.end(), [&layer](const editor_dropdown_item_t& item) { return item.value == layer.mask; });

			if (mask == _mask_items.end())
				layer.mask = NULL_SID;
		}
	}

	void editor_panel_animation_library_t::clear_layer_controls()
	{
		for (u32 i = 0; i < _layer_control_count; ++i)
		{
			layer_controls_t& controls = _layer_controls[i];

			controls.name.uninit();
			controls.weight.uninit();
			controls.mask.uninit();
			controls.remove.uninit();
			controls.fold.uninit();
		}

		_layer_control_count = 0;
	}

	void editor_panel_animation_library_t::refresh_layer_controls()
	{
		clear_layer_controls();

		ui::layout_tree_t&				tree  = _ui->get_tree();
		const editor_theme_t&			theme = editor_theme_t::get();
		const editor_widget_callbacks_t callbacks{.edit_begin = on_edit_begin, .edit_submitted = on_edit_submitted, .user_data = this};

		for (u32 i = 0; i < _library.layer_count; ++i)
		{
			animation_library_layer_def_t& layer	= _library.layers[i];
			layer_controls_t&			   controls = _layer_controls[i];

			controls.panel = this;
			controls.index = i;

			char label[sizeof(layer.name) + sizeof("Layer: ")] = {};

			std::snprintf(label, sizeof(label), "Layer: %s", layer.name);
			controls.fold.init(*_ui,
							   _layer_list,
							   {
								   .background =
									   {
										   .fill_color_a  = theme.color_frame,
										   .fill_color_b  = theme.color_frame,
										   .rounding	  = theme.item_rounding,
										   .rounding_segs = 4,
									   },
								   .label			 = label,
								   .on_pressed		 = on_layer_pressed,
								   .on_fold_changed	 = on_layer_fold_changed,
								   .user_data		 = &controls,
								   .folded			 = !_layer_expanded[i],
								   .background_frame = true,
							   });
			_ui->set_widget_debug_name(controls.fold.get_root(), "animation_library_layer");

			const ui::widget_id_t body = controls.fold.get_body();
			ui::layout_in_t&	  in   = tree.in(body);

			in.child_spacing = 0.0f;
			in.child_margins = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};

			const editor_property_row_t name_row = editor_misc_widgets_t::make_property_row_with_label(*_ui, body, "Name");
			u8*							name	 = reinterpret_cast<u8*>(layer.name);

			controls.name.init(*_ui,
							   name_row.right,
							   {
								   .field	  = {.fields = {.data = &name, .size = 1}, .field_size = sizeof(layer.name), .type = editor_input_field_field_type_e::char_array},
								   .callbacks = callbacks,
							   });

			const editor_property_row_t weight_row = editor_misc_widgets_t::make_property_row_with_label(*_ui, body, "Weight");
			u8*							weight	   = reinterpret_cast<u8*>(&layer.weight);

			controls.weight.init(*_ui,
								 weight_row.right,
								 {
									 .field		= {.fields = {.data = &weight, .size = 1}, .field_size = sizeof(layer.weight), .type = editor_input_field_field_type_e::pod_number, .is_slider = true},
									 .callbacks = callbacks,
									 .increment = 0.01f,
									 .min_value = 0.0f,
									 .max_value = 1.0f,
								 });

			const editor_property_row_t mask_row = editor_misc_widgets_t::make_property_row_with_label(*_ui, body, "Mask");
			u8*							mask	 = reinterpret_cast<u8*>(&layer.mask);

			controls.mask.init(*_ui,
							   mask_row.right,
							   {
								   .items	   = _mask_items.data(),
								   .field	   = {.fields = {.data = &mask, .size = 1}, .field_size = sizeof(layer.mask)},
								   .callbacks  = callbacks,
								   .item_count = static_cast<u16>(_mask_items.size()),
								   .width	   = editor_dropdown_width_e::parent_relative,
								   .pos_y	   = editor_dropdown_pos_y_e::center,
							   });

			const editor_property_row_t remove_row = editor_misc_widgets_t::make_property_row(*_ui, body);

			controls.remove.init(*_ui, remove_row.right, {.text = "Remove"});
			_ui->get_input().set_listener(controls.remove.get_root(), {.on_click = on_layer_remove_pressed, .user_data = &controls});

			if (_library.layer_count == 1)
				tree.in(controls.remove.get_root()).flags |= ui::wf_disabled;

			for (const ui::widget_id_t label : {name_row.label, weight_row.label, mask_row.label})
				tree.in(label).flags &= ~ui::wf_input;

			for (const ui::widget_id_t widget : {controls.name.get_root(), controls.weight.get_root(), controls.remove.get_root()})
			{
				ui::layout_in_t& control_in = tree.in(widget);

				control_in.size_mode_x = ui::axis_mode_e::fill;
				control_in.pos_mode_y  = ui::pos_mode_e::relative_in_parent;
				control_in.pos_value.y = 0.5f;
				control_in.anchor_y	   = ui::anchor_e::center;
			}

			++_layer_control_count;
		}

		refresh_layer_selection();
		refresh_skeleton_overlay();
	}

	void editor_panel_animation_library_t::refresh_layer_selection()
	{
		const editor_theme_t& theme = editor_theme_t::get();

		for (u32 i = 0; i < _layer_control_count; ++i)
		{
			_ui->get_paint().set_rect(_layer_controls[i].fold.get_root(),
									  {
										  .fill_color_a		 = theme.color_frame,
										  .fill_color_b		 = theme.color_frame,
										  .outline_color	 = i == _selected_layer ? theme.color_accent1 : theme.color_frame,
										  .rounding			 = theme.item_rounding,
										  .outline_thickness = theme.border_thickness,
										  .rounding_segs	 = 4,
									  });
		}
	}

	void editor_panel_animation_library_t::finish_edit()
	{
		if (_edit_previous_stream.size != 0)
			editor_command_animation_library_edit_t::submit(*this, "Edit Animation Library");
	}

	void editor_panel_animation_library_t::on_edit_begin(void* user_data)
	{
		editor_panel_animation_library_t& panel = *static_cast<editor_panel_animation_library_t*>(user_data);

		editor_command_animation_library_edit_t::begin(panel);
	}

	void editor_panel_animation_library_t::on_edit_submitted(void* user_data)
	{
		editor_panel_animation_library_t& panel = *static_cast<editor_panel_animation_library_t*>(user_data);

		panel.finish_edit();

		for (u32 i = 0; i < panel._layer_control_count; ++i)
		{
			const animation_library_layer_def_t& layer										   = panel._library.layers[i];
			char								 label[sizeof(layer.name) + sizeof("Layer: ")] = {};

			std::snprintf(label, sizeof(label), "Layer: %s", layer.name);
			panel._layer_controls[i].fold.set_text(label);
		}

		panel.refresh_scene();
	}

	void editor_panel_animation_library_t::on_layer_pressed(void* user_data)
	{
		layer_controls_t&				  controls = *static_cast<layer_controls_t*>(user_data);
		editor_panel_animation_library_t& panel	   = *controls.panel;

		panel.finish_edit();
		editor_command_animation_library_edit_t::select(panel, controls.index);
	}

	void editor_panel_animation_library_t::on_layer_fold_changed(bool folded, void* user_data)
	{
		layer_controls_t& controls = *static_cast<layer_controls_t*>(user_data);

		controls.panel->_layer_expanded[controls.index] = !folded;
	}

	void editor_panel_animation_library_t::on_layer_add_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left)
			return;

		editor_panel_animation_library_t& panel = *static_cast<editor_panel_animation_library_t*>(user_data);

		panel.finish_edit();

		if (panel._library.layer_count == MAX_ANIMATION_LIBRARY_LAYERS || !editor_command_animation_library_edit_t::begin(panel))
			return;

		const u32 index = static_cast<u32>(panel._library.layer_count++);

		panel._library.layers[index] = {};
		panel._layer_expanded[index] = false;
		panel._selected_layer		 = index;
		panel._selected_state		 = UINT32_MAX;
		panel._selected_clip		 = UINT32_MAX;
		editor_command_animation_library_edit_t::submit(panel, "Add Animation Layer");
		panel._refresh_layers = true;
		panel._refresh_states = true;
		panel.refresh_skeleton_overlay();
	}

	void editor_panel_animation_library_t::on_layer_remove_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left)
			return;

		layer_controls_t&				  controls = *static_cast<layer_controls_t*>(user_data);
		editor_panel_animation_library_t& panel	   = *controls.panel;
		const u32						  index	   = controls.index;

		if (panel._library.layer_count == 1)
			return;

		panel.finish_edit();

		if (!editor_command_animation_library_edit_t::begin(panel))
			return;

		for (size_t i = index + 1; i < panel._library.layer_count; ++i)
		{
			panel._library.layers[i - 1] = std::move(panel._library.layers[i]);
			panel._layer_expanded[i - 1] = panel._layer_expanded[i];
		}

		--panel._library.layer_count;
		panel._library.layers[panel._library.layer_count] = {};
		panel._layer_expanded[panel._library.layer_count] = false;

		if (panel._selected_layer == index)
		{
			panel._selected_layer = UINT32_MAX;
			panel._selected_state = UINT32_MAX;
			panel._selected_clip  = UINT32_MAX;
		}
		else if (panel._selected_layer != UINT32_MAX && panel._selected_layer > index)
			--panel._selected_layer;

		editor_command_animation_library_edit_t::submit(panel, "Remove Animation Layer");
		panel._refresh_layers = true;
		panel._refresh_states = true;
		panel.refresh_skeleton_overlay();
	}

	void editor_panel_animation_library_t::on_refresh_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data)
	{
		editor_panel_animation_library_t& panel = *static_cast<editor_panel_animation_library_t*>(user_data);

		if (panel._refresh_skeleton)
		{
			panel._refresh_skeleton = false;
			panel.refresh_skeleton_masks();
			panel.validate_layer_masks();
			panel.refresh_skeleton_reference();
			panel.refresh_skeleton_overlay();
		}

		if (panel._refresh_layers)
		{
			panel._refresh_layers = false;
			panel.refresh_layer_controls();
		}

		if (panel._refresh_states)
		{
			panel._refresh_states = false;
			panel._states_widget->refresh();
		}
	}

	void editor_panel_animation_library_t::apply_pane_splits()
	{
		_ui->get_tree().in(_left_pane).size_value.x	 = _left_split;
		_ui->get_tree().in(_right_pane).size_value.x = 1.0f - _right_split;
	}

	void editor_panel_animation_library_t::on_skeleton_edited(void* user_data)
	{
		editor_panel_animation_library_t& panel = *static_cast<editor_panel_animation_library_t*>(user_data);

		panel.finish_edit();

		if (!editor_command_animation_library_edit_t::begin(panel))
		{
			panel.refresh_skeleton_reference();
			return;
		}

		panel._library.skeleton = panel._skeleton_reference_value;
		panel.refresh_skeleton_masks();
		panel.validate_layer_masks();
		editor_command_animation_library_edit_t::submit(panel, "Assign Skeleton");
		panel.refresh_skeleton_overlay();
		panel.refresh_scene();
		panel._refresh_layers = true;
		panel._refresh_states = true;
	}

	void editor_panel_animation_library_t::on_save_changes_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left)
			return;

		editor_panel_animation_library_t& panel	 = *static_cast<editor_panel_animation_library_t*>(user_data);
		nlohmann::json					  source = nlohmann::json::object();

		router.set_focus(panel._root, false);
		panel.finish_edit();

		if (!reflection_registry_t::get().type_to_json(type_id_t<animation_library_def_t>::value, &panel._library, nullptr, source))
		{
			SFG_ERR("failed to serialize animation library asset: {0}", panel._library_guid);
			return;
		}

		source["schema"] = "sfg.schema.animation_library";

		if (!editor_asset_manager_t::get().save_and_cook_embedded_asset_async(panel._library_guid, source))
			SFG_ERR("failed to save and queue cooking for animation library asset: {0}", panel._library_guid);
	}

	void editor_panel_animation_library_t::on_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		editor_panel_animation_library_t& panel = *static_cast<editor_panel_animation_library_t*>(user_data);
		const ui::layout_out_t&			  out	= panel._ui->get_tree().out(panel._root);
		const f32						  split = (pos.x - out.pos.x) / out.size.x;

		if (&border == &panel._left_split_border)
			panel._left_split = math::clamp(split, ANIMATION_LIBRARY_SIDE_MIN, panel._right_split - ANIMATION_LIBRARY_MID_MIN);
		else
			panel._right_split = math::clamp(split, panel._left_split + ANIMATION_LIBRARY_MID_MIN, 1.0f - ANIMATION_LIBRARY_SIDE_MIN);

		panel.apply_pane_splits();
	}

	void editor_panel_animation_library_t::on_asset_deletion(editor_asset_manager_t& manager, span_t<const sid_t> asset_ids, void* user_data)
	{
		editor_panel_animation_library_t& panel = *static_cast<editor_panel_animation_library_t*>(user_data);

		if (std::find(asset_ids.begin(), asset_ids.end(), panel._library_guid) != asset_ids.end())
		{
			panel._ui->get_input().set_focus(panel._root, false);
			panel.finish_edit();
			editor_command_system_t::get().clear_user_data(&panel);
			panel._library_guid = NULL_SID;
			panel._asset_name.resize(0);

			panel.set_sub_item_id(NULL_SID);
			panel.refresh_title();
			panel._ui->get_tree().in(panel._save_changes_button.get_root()).flags |= ui::wf_disabled;
			panel._ui->get_tree().in(panel._skeleton_reference.get_root()).flags |= ui::wf_disabled;
			panel.apply_edits({}, UINT32_MAX);
		}
		else if (std::find(asset_ids.begin(), asset_ids.end(), panel._library.skeleton) != asset_ids.end())
		{
			panel._ui->get_input().set_focus(panel._root, false);
			panel.finish_edit();
			panel._has_skeleton = false;
			panel.refresh_skeleton_overlay();
			panel.refresh_scene();
			panel._refresh_skeleton = true;
			panel._refresh_layers	= true;
			panel._refresh_states	= true;
		}
	}
}
