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

#include "editor_panel_animation_library.hpp"
#include "editor_theme.hpp"

#include "assets/editor_asset.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"
#include "commands/editor_command_animation_library.hpp"
#include "editor_command_system.hpp"
#include "editor_surface_controller.hpp"
#include "editor_world_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include "world/editor_world.hpp"
#include "world/editor_world_util.hpp"

#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define ANIMATION_LIBRARY_SIDE_MIN 0.1f
#define ANIMATION_LIBRARY_MID_MIN  0.2f

	editor_panel_animation_library_t::editor_panel_animation_library_t()
	{
		set_type(editor_panel_type_e::animation_library);
		refresh_title();
		set_icon(ICON_ANIMATION);
	}

	editor_panel_animation_library_t::~editor_panel_animation_library_t() = default;

	void editor_panel_animation_library_t::serialize(nlohmann::json& j) const
	{
		j["library_guid"] = _library_guid;
		j["asset_name"]	  = _asset_name;
		j["left_split"]	  = _left_split;
		j["right_split"]  = _right_split;
	}

	void editor_panel_animation_library_t::deserialize(const nlohmann::json& j)
	{
		_library_guid = j.value<sid_t>("library_guid", NULL_SID);
		_asset_name	  = j.value<string_t>("asset_name", {});
		_left_split	  = math::clamp(j.value<f32>("left_split", _left_split), ANIMATION_LIBRARY_SIDE_MIN, 1.0f - ANIMATION_LIBRARY_SIDE_MIN - ANIMATION_LIBRARY_MID_MIN);
		_right_split  = math::clamp(j.value<f32>("right_split", _right_split), _left_split + ANIMATION_LIBRARY_MID_MIN, 1.0f - ANIMATION_LIBRARY_SIDE_MIN);

		set_sub_item_id(_library_guid);
		refresh_title(_asset_name.c_str(), "AL: ");
	}

	void editor_panel_animation_library_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		_commands = make_unique<editor_command_system_t>();
		_commands->init({.listener_initial_capacity = 1, .global_instance = false});
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

		const ui::widget_id_t right_content = ui.allocate_widget();

		tree.attach(_right_pane, right_content);
		tree.draw_order(right_content) = tree.draw_order_const(_right_pane) + 1;

		ui::layout_in_t& content_in = tree.in(right_content);

		content_in.flow			 = ui::flow_e::column;
		content_in.child_margins = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};
		content_in.size_mode_x	 = ui::axis_mode_e::parent_relative;
		content_in.size_mode_y	 = ui::axis_mode_e::sum_children;
		content_in.size_value	 = {1.0f, 1.0f};

		_right_scrollbar.init(ui, {.target = _right_pane, .axes = editor_scrollbar_axis_y});
		editor_misc_widgets_t::make_section_label(ui, right_content, "Animation library");

		const editor_property_row_t skeleton_row   = editor_misc_widgets_t::make_property_row_with_label(ui, right_content, "Skeleton");
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

		editor_misc_widgets_t::add_spacer(ui, right_content, {0.0f, theme.item_spacing});
		_save_changes_button.init(ui, right_content, {.text = "Save Changes"});
		tree.in(_save_changes_button.get_root()).flags |= ui::wf_disabled;
		tree.in(_skeleton_reference.get_root()).flags |= ui::wf_disabled;
		ui.get_input().set_listener(_save_changes_button.get_root(), {.on_click = on_save_changes_pressed, .user_data = this});

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
		editor_asset_manager_t::get().remove_asset_deletion_listener(_asset_deletion_listener);
		_commands->uninit();
		_commands.reset();

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

		_commands->clear();
		_library_guid = library_guid;
		_asset_name	  = asset_name;

		set_sub_item_id(_library_guid);
		refresh_title(_asset_name.c_str(), "AL: ");
		_ui->get_tree().in(_save_changes_button.get_root()).flags &= ~ui::wf_disabled;
		_ui->get_tree().in(_skeleton_reference.get_root()).flags &= ~ui::wf_disabled;
		apply_edits(definition);
	}

	void editor_panel_animation_library_t::apply_edits(const animation_library_def_t& definition)
	{
		_library = definition;

		refresh_skeleton_reference();
		refresh_skeleton_overlay();
		refresh_scene();
	}

	bool editor_panel_animation_library_t::on_command_event(const window_event_t& ev)
	{
		return _commands->on_window_event(ev);
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
		const editor_asset_t* skeleton	   = editor_asset_manager_t::get().find_asset(_library.skeleton);
		const bool			  has_skeleton = skeleton != nullptr && skeleton->asset_type == editor_asset_type_e::skeleton;

		_ui->get_tree().set_visible(_missing_skeleton_frame, !has_skeleton);
	}

	void editor_panel_animation_library_t::apply_pane_splits()
	{
		_ui->get_tree().in(_left_pane).size_value.x	 = _left_split;
		_ui->get_tree().in(_right_pane).size_value.x = 1.0f - _right_split;
	}

	void editor_panel_animation_library_t::on_skeleton_edited(void* user_data)
	{
		editor_panel_animation_library_t& panel		 = *static_cast<editor_panel_animation_library_t*>(user_data);
		animation_library_def_t			  definition = panel._library;

		definition.skeleton = panel._skeleton_reference_value;

		if (!editor_command_animation_library_edit_t::submit(*panel._commands, panel, definition, "Assign Skeleton"))
			panel.refresh_skeleton_reference();
	}

	void editor_panel_animation_library_t::on_save_changes_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left)
			return;

		editor_panel_animation_library_t& panel	 = *static_cast<editor_panel_animation_library_t*>(user_data);
		nlohmann::json					  source = nlohmann::json::object();

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
			panel._commands->clear();
			panel._library_guid = NULL_SID;
			panel._asset_name.resize(0);

			panel.set_sub_item_id(NULL_SID);
			panel.refresh_title();
			panel._ui->get_tree().in(panel._save_changes_button.get_root()).flags |= ui::wf_disabled;
			panel._ui->get_tree().in(panel._skeleton_reference.get_root()).flags |= ui::wf_disabled;
			panel.apply_edits({});
		}
		else if (std::find(asset_ids.begin(), asset_ids.end(), panel._library.skeleton) != asset_ids.end())
		{
			panel._ui->get_tree().set_visible(panel._missing_skeleton_frame, true);
			panel.refresh_skeleton_reference();
			panel.refresh_scene();
		}
	}
}
