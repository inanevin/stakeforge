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

#include "editor_panel_skeleton_viewer.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"
#include "assets/editor_asset_util.hpp"
#include "commands/editor_command_skeleton.hpp"
#include "editor_command_system.hpp"
#include "editor_surface_controller.hpp"
#include "editor_world_controller.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_popup_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/editor_tooltip_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widget_toggle_button.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include "world/editor_world.hpp"
#include "world/editor_world_util.hpp"

#include <sfg/data/frame_vector.hpp>
#include <sfg/data/char_util.hpp>
#include <sfg/input/input_mappings.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/rectf.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/mesh.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/skeleton.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/world_debug_draw.hpp>
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define SKELETON_VIEWER_ADD_SLOT	   1
#define SKELETON_VIEWER_DUPLICATE_SLOT 2
#define SKELETON_VIEWER_DELETE_SLOT	   3
#define SKELETON_VIEWER_RENAME_SLOT	   4

#define SKELETON_VIEWER_PANE_SPLIT_MIN				0.45f
#define SKELETON_VIEWER_PANE_SPLIT_MAX				0.85f
#define SKELETON_VIEWER_SPLIT_BORDER_THICKNESS_MULT 2.0f

	struct editor_panel_skeleton_viewer_t::mask_item_t
	{
		editor_input_field_t			name_field		= {};
		editor_widget_toggle_button_t	activate_button = {};
		editor_widget_button_t			edit_button		= {};
		editor_widget_button_t			remove_button	= {};
		editor_panel_skeleton_viewer_t* viewer			= nullptr;
		ui::widget_id_t					root			= NULL_WIDGET;
	};

	editor_panel_skeleton_viewer_t::editor_panel_skeleton_viewer_t()
	{
		set_type(editor_panel_type_e::skeleton_viewer);
		refresh_title();
		set_icon(ICON_ANIMATION);
	}

	editor_panel_skeleton_viewer_t::~editor_panel_skeleton_viewer_t() = default;

	void editor_panel_skeleton_viewer_t::serialize(nlohmann::json& j) const
	{
		j				   = nlohmann::json::object();
		j["skeleton_guid"] = _skeleton_guid;
		j["asset_name"]	   = _asset_name;
		j["pane_split"]	   = _pane_split;
	}

	void editor_panel_skeleton_viewer_t::deserialize(const nlohmann::json& j)
	{
		_skeleton_guid = j.value<sid_t>("skeleton_guid", 0);
		set_sub_item_id(_skeleton_guid);
		_asset_name = j.value<string_t>("asset_name", {});
		_pane_split = math::clamp(j.value<f32>("pane_split", _pane_split), SKELETON_VIEWER_PANE_SPLIT_MIN, SKELETON_VIEWER_PANE_SPLIT_MAX);
		refresh_title(_asset_name.c_str(), "S: ");
	}

	void editor_panel_skeleton_viewer_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);

		_commands = make_unique<editor_command_system_t>();
		_commands->init({.listener_initial_capacity = 1, .global_instance = false});

		_asset_deletion_listener = editor_asset_manager_t::get().add_asset_deletion_listener(on_asset_deletion, this);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::row;
		root_in.child_spacing	 = 0.0f;

		_left_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_left_pane, "skeleton_viewer_left_pane");
		tree.attach(_root, _left_pane);

		ui::layout_in_t& left_in = tree.in(_left_pane);
		left_in.flow			 = ui::flow_e::none;
		left_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		left_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		left_in.size_value		 = {_pane_split, 1.0f};

		ui::vg_rect_paint_t left_rect = {};
		left_rect.fill_color_a		  = theme.color_frame;
		left_rect.fill_color_b		  = theme.color_frame;
		paint.set_rect(_left_pane, left_rect);

		_world_view.init(ui, _left_pane);

		editor_split_border_t::config_t split_config = {};
		split_config.direction						 = editor_split_border_direction_e::horizontal;
		split_config.on_drag						 = on_split_border_drag;
		split_config.user_data						 = this;
		_split_border.init(ui, _root, split_config);

		ui::layout_in_t& border_in = tree.in(_split_border.get_root());
		border_in.size_mode_x	   = ui::axis_mode_e::fixed;
		border_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		border_in.size_value	   = {theme.border_thickness * SKELETON_VIEWER_SPLIT_BORDER_THICKNESS_MULT, 1.0f};

		_right_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_right_pane, "skeleton_viewer_right_pane");
		tree.attach(_root, _right_pane);

		ui::layout_in_t& right_in = tree.in(_right_pane);
		right_in.flags |= ui::wf_input | ui::wf_scroll_y;
		right_in.child_clip_mode = ui::clip_mode_e::scissor_rect;
		right_in.size_mode_x	 = ui::axis_mode_e::fill;
		right_in.size_mode_y	 = ui::axis_mode_e::parent_relative;
		right_in.size_value		 = {1.0f, 1.0f};

		_right_content = ui.allocate_widget();
		ui.set_widget_debug_name(_right_content, "skeleton_viewer_right_content");
		tree.attach(_right_pane, _right_content);
		tree.draw_order(_right_content) = tree.draw_order_const(_right_pane) + 1;

		ui::layout_in_t& right_content_in = tree.in(_right_content);
		right_content_in.flow			  = ui::flow_e::column;
		right_content_in.child_margins	  = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};
		right_content_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		right_content_in.size_mode_y	  = ui::axis_mode_e::sum_children;
		right_content_in.size_value		  = {1.0f, 1.0f};

		_right_scrollbar.init(ui, {.target = _right_pane, .axes = editor_scrollbar_axis_y});

		editor_misc_widgets_t::make_section_label(ui, _right_content, "Skeleton");

		_joint_count_value = append_property_value_row("Joints");
		editor_dividers_t::add_divider_hor(ui, _right_content, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);

		_root_joint_value = append_property_value_row("Root Joint");
		editor_dividers_t::add_divider_hor(ui, _right_content, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);

		const editor_property_row_t preview_mesh_row   = editor_misc_widgets_t::make_property_row_with_label(ui, _right_content, "Preview Mesh");
		u64*						preview_mesh_field = &_preview_mesh;

		_preview_mesh_reference.init(ui,
									 preview_mesh_row.right,
									 {
										 .callbacks =
											 {
												 .edited	= on_preview_mesh_edited,
												 .user_data = this,
											 },
										 .fields	 = {.data = &preview_mesh_field, .size = 1},
										 .asset_type = editor_asset_type_e::mesh,
									 });

		ui::layout_in_t& preview_mesh_in = tree.in(_preview_mesh_reference.get_root());
		preview_mesh_in.size_mode_x		 = ui::axis_mode_e::fill;
		preview_mesh_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		preview_mesh_in.pos_value.y		 = 0.5f;
		preview_mesh_in.anchor_y		 = ui::anchor_e::center;

		editor_dividers_t::add_divider_hor(ui, _right_content, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);

		const editor_property_row_t preview_animation_row	= editor_misc_widgets_t::make_property_row_with_label(ui, _right_content, "Preview Animation");
		u64*						preview_animation_field = &_preview_animation;

		_preview_animation_reference.init(ui,
										  preview_animation_row.right,
										  {
											  .callbacks  = {.edited = on_preview_animation_edited, .user_data = this},
											  .fields	  = {.data = &preview_animation_field, .size = 1},
											  .asset_type = editor_asset_type_e::animation,
										  });

		ui::layout_in_t& preview_animation_in = tree.in(_preview_animation_reference.get_root());
		preview_animation_in.size_mode_x	  = ui::axis_mode_e::fill;
		preview_animation_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		preview_animation_in.pos_value.y	  = 0.5f;
		preview_animation_in.anchor_y		  = ui::anchor_e::center;

		editor_dividers_t::add_divider_hor(ui, _right_content, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);

		init_animation_controls();

		editor_misc_widgets_t::add_spacer(ui, _right_content, {0.0f, theme.item_spacing});
		_save_changes_button.init(ui, _right_content, {.text = "Save Changes"});
		tree.in(_save_changes_button.get_root()).flags |= ui::wf_disabled;
		ui.get_input().set_listener(_save_changes_button.get_root(), {.on_click = on_save_changes_pressed, .user_data = this});

		editor_dividers_t::add_divider_hor(ui, _right_content, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);

		editor_misc_widgets_t::make_section_label(ui, _right_content, "Masks");
		_make_mask_button.init(ui, _right_content, {.text = "Make Mask"});
		tree.in(_make_mask_button.get_root()).flags |= ui::wf_disabled;
		ui.get_input().set_listener(_make_mask_button.get_root(), {.on_click = on_make_mask_pressed, .user_data = this});
		editor_tooltip_controller_t::find(ui)->set_tooltip(_make_mask_button.get_root(), {.text = "Create a mask from the selected joints and all their descendants."});

		_mask_list = ui.allocate_widget();
		ui.set_widget_debug_name(_mask_list, "skeleton_masks");
		tree.attach(_right_content, _mask_list);

		ui::layout_in_t& masks_in = tree.in(_mask_list);
		masks_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		masks_in.size_mode_y	  = ui::axis_mode_e::sum_children;
		masks_in.size_value		  = {1.0f, 1.0f};
		masks_in.flow			  = ui::flow_e::column;
		masks_in.child_spacing	  = theme.item_spacing;
		masks_in.child_margins	  = {theme.margin_vertical, 0.0f, 0.0f, 0.0f};

		editor_misc_widgets_t::make_section_label(ui, _right_content, "Selected Slot");
		init_slot_fields();

		editor_misc_widgets_t::make_section_label(ui, _right_content, "Hierarchy");
		init_joint_hierarchy();

		create_preview_world();

		if (_skeleton_guid != 0)
			set_skeleton(_skeleton_guid, _asset_name.c_str());
		else
		{
			update_preview_environment();
			refresh_info();
			refresh_joint_hierarchy();
		}

		apply_pane_split();
	}

	void editor_panel_skeleton_viewer_t::uninit()
	{
		if (_rename_slot_index != UINT32_MAX)
		{
			editor_popup_controller_t::find(*_ui)->close_popup();
			_rename_slot_index = UINT32_MAX;
		}

		if (_row_menu_open)
			editor_action_menu_controller_t::find(*_ui)->close_action_menu();

		editor_world_controller_t::get().get_editor_world(_world)->set_gizmo_callbacks({});
		editor_command_skeleton_edit_t::cancel(*this);
		_commands->clear();
		_slot_fields_edit_active = false;
		_mask_name_edit_active	 = false;
		_editing_mask			 = UINT32_MAX;
		_active_mask			 = UINT32_MAX;

		_slot_preview_mesh_reference.uninit();
		_slot_position_field.uninit();
		_slot_preview_scale_field.uninit();
		_slot_rotation_field.uninit();

		editor_asset_manager_t::get().remove_asset_deletion_listener(_asset_deletion_listener);

		_asset_deletion_listener = {};
		_right_scrollbar.uninit();
		_preview_mesh_reference.uninit();
		_preview_animation_reference.uninit();
		_animation_play_button.uninit();
		_animation_reset_button.uninit();
		_save_changes_button.uninit();
		editor_tooltip_controller_t::find(*_ui)->clear_tooltip(_make_mask_button.get_root());
		_make_mask_button.uninit();
		clear_mask_items();
		_selected_joints.resize(0);
		_world_view.uninit();
		_split_border.uninit();
		_ui->deallocate_widget(_left_pane);
		_ui->deallocate_widget(_right_pane);
		destroy_preview_world();

		_joint_rows.resize(0);
		_selected_joint_index = SKELETON_JOINT_NO_PARENT;
		_selected_slot_index  = UINT32_MAX;
		_joint_list_area	  = NULL_WIDGET;
		_skeleton			  = {};

		_commands->uninit();
		_commands.reset();

		editor_panel_t::uninit();
	}

	void editor_panel_skeleton_viewer_t::set_skeleton(sid_t skeleton_guid, const char* asset_name)
	{
		if (!_world.is_null())
			editor_world_controller_t::get().get_editor_world(_world)->cancel_gizmo_action();

		if (_rename_slot_index != UINT32_MAX)
		{
			editor_popup_controller_t::find(*_ui)->close_popup();
			_rename_slot_index = UINT32_MAX;
		}

		if (_row_menu_open)
			editor_action_menu_controller_t::find(*_ui)->close_action_menu();

		clear_mask_items();
		_selected_joints.resize(0);

		_selected_joint_index = SKELETON_JOINT_NO_PARENT;
		_selected_slot_index  = UINT32_MAX;
		++_slot_generation;

		editor_command_skeleton_edit_t::cancel(*this);
		_commands->clear();
		_slot_fields_edit_active = false;
		_mask_name_edit_active	 = false;
		_editing_mask			 = UINT32_MAX;
		_active_mask			 = UINT32_MAX;

		_skeleton_guid = skeleton_guid;

		if (skeleton_guid != NULL_SID)
			_ui->get_tree().in(_save_changes_button.get_root()).flags &= ~ui::wf_disabled;
		else
			_ui->get_tree().in(_save_changes_button.get_root()).flags |= ui::wf_disabled;

		set_sub_item_id(skeleton_guid);
		_asset_name		  = asset_name;
		_skeleton		  = {};
		_root_joint_index = UINT32_MAX;

		if (skeleton_guid != NULL_SID)
		{
			const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(skeleton_guid);

			if (asset != nullptr && asset->asset_type == editor_asset_type_e::skeleton && !asset->embedded_source.empty())
			{
				const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(*asset);

				if (!reflection_registry_t::get().type_from_json(type_id_t<skeleton_def_t>::value, &_skeleton, nullptr, embedded_source))
					_skeleton = {};
			}
		}

		_selected_joints.reserve(_skeleton.joints.size());

		_preview_mesh		  = _skeleton.preview_mesh;
		_preview_animation	  = _skeleton.preview_animation;
		_is_animation_playing = false;
		_root_joint_index	  = _skeleton.is_evaluation_order_valid() ? _skeleton.root_joint_index : UINT32_MAX;

		refresh_preview_mesh_reference();
		refresh_preview_animation_reference();
		update_preview_environment();
		create_display_entity();

		if (const mesh_internals_t* internals = resource_manager_t::get().find_internals<mesh_internals_t>(_preview_mesh))
			editor_world_controller_t::get().get_editor_world(_world)->fit_camera_to_bounds(internals->local_bounds);

		refresh_info();
		refresh_joint_hierarchy();
		refresh_mask_items();
		refresh_title(_asset_name.c_str(), "S: ");
	}

	bool editor_panel_skeleton_viewer_t::on_command_event(const window_event_t& ev)
	{
		if (ev.type != window_event_type_e::key || (ev.sub_type != window_event_sub_type_e::press && ev.sub_type != window_event_sub_type_e::repeat))
			return false;

		const bool ctrl = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));

		if (!ctrl || (ev.button != static_cast<u16>(input_code::key_z) && ev.button != static_cast<u16>(input_code::key_r)))
			return false;

		editor_world_controller_t::get().get_editor_world(_world)->end_gizmo_action();

		if (_slot_position_field.is_editing() || _slot_rotation_field.is_editing() || _slot_preview_scale_field.is_editing() || _mask_name_edit_active)
			_ui->get_input().set_focus(_joint_list_area, false);

		on_slot_fields_edit_submitted(this);
		finish_mask_edit();

		return _commands->on_window_event(ev);
	}

	void editor_panel_skeleton_viewer_t::apply_edits(vector_t<skeleton_slot_def_t>&& slots, vector_t<skeleton_mask_def_t>&& masks, u32 selected_joint, u32 selected_slot)
	{
		if (_rename_slot_index != UINT32_MAX)
		{
			editor_popup_controller_t::find(*_ui)->close_popup();
			_rename_slot_index = UINT32_MAX;
		}

		if (_row_menu_open)
			editor_action_menu_controller_t::find(*_ui)->close_action_menu();

		editor_world_controller_t::get().get_editor_world(_world)->cancel_gizmo_action();
		_selected_joints.resize(0);
		_selected_joint_index = selected_joint;
		_selected_slot_index  = selected_slot;
		++_slot_generation;

		clear_mask_items();
		_editing_mask		   = UINT32_MAX;
		_mask_name_edit_active = false;
		_skeleton.slots		   = std::move(slots);
		if (_active_mask != UINT32_MAX && masks.size() != _skeleton.masks.size())
		{
			const sid_t active_name = TO_SID(static_cast<const char*>(_skeleton.masks[_active_mask].name));
			const auto	active		= std::find_if(masks.begin(), masks.end(), [active_name](const skeleton_mask_def_t& mask) { return TO_SID(static_cast<const char*>(mask.name)) == active_name; });

			_active_mask = active == masks.end() ? UINT32_MAX : static_cast<u32>(active - masks.begin());
		}

		_skeleton.masks = std::move(masks);
		refresh_mask_items();

		refresh_info();
		refresh_joint_hierarchy();
	}

	void editor_panel_skeleton_viewer_t::create_preview_world()
	{
		const editor_world_init_config_t init_config = editor_world_init_config_t::make_preview(editor_surface_controller_t::get().get_main_surface().swapchain_size);

		editor_world_controller_t& controller = editor_world_controller_t::get();
		_world								  = controller.create_world(init_config, editor_world_edit_type_e::view_with_debug, on_world_tick, this);
		editor_world_t* const editor_world	  = controller.get_editor_world(_world);

		editor_world->install_camera(editor_world_camera_type_e::fly);

		editor_world->set_gizmo_callbacks({
			.get_target	 = [](void* user_data, editor_gizmo_target_t& target) { return static_cast<editor_panel_skeleton_viewer_t*>(user_data)->get_slot_gizmo_target(target); },
			.begin		 = [](void* user_data) { return static_cast<editor_panel_skeleton_viewer_t*>(user_data)->begin_slot_gizmo(); },
			.update		 = [](void* user_data, const mat4x3_t& delta) { static_cast<editor_panel_skeleton_viewer_t*>(user_data)->update_slot_gizmo(delta); },
			.commit		 = [](void* user_data) { static_cast<editor_panel_skeleton_viewer_t*>(user_data)->commit_slot_gizmo(); },
			.cancel		 = [](void* user_data) { static_cast<editor_panel_skeleton_viewer_t*>(user_data)->cancel_slot_gizmo(); },
			.user_data	 = this,
			.allow_scale = true,
		});
		_world_view.set_edit_world(_world);
	}

	void editor_panel_skeleton_viewer_t::update_preview_environment()
	{
		if (_world.is_null())
			return;

		world_t&  world		  = editor_world_controller_t::get().get_editor_world(_world)->get_world();
		const f32 spotlight_y = _skeleton.joints.empty() ? 5.0f : _skeleton.local_bounds.bounds_max.y * 2.0f;

		if (_environment_entity == NULL_ENTITY_ID)
		{
			_environment_entity = editor_world_util_t::install_default_scene_dark(world, spotlight_y);
			return;
		}

		world.set_entity_pos_local(_environment_entity, {0.0f, spotlight_y, 0.0f});

		component_light_t& light = ecs_helpers_t::table_get_as<component_light_t>(world.get_component_table(type_id_t<component_light_t>::value), _environment_entity);

		light.range = math::max(10.0f, spotlight_y * 2.0f);
	}

	void editor_panel_skeleton_viewer_t::destroy_preview_world()
	{
		if (_world.is_null())
			return;

		editor_world_controller_t::get().destroy_world(_world);

		_world				= {};
		_display_entity		= NULL_ENTITY_ID;
		_environment_entity = NULL_ENTITY_ID;
		_slot_previews.resize(0);
	}

	void editor_panel_skeleton_viewer_t::create_display_entity()
	{
		if (_world.is_null())
			return;

		editor_world_controller_t::get().get_editor_world(_world)->cancel_gizmo_action();
		++_slot_generation;
		clear_display_entity();

		world_t& world = editor_world_controller_t::get().get_editor_world(_world)->get_world();

		_display_entity = world.create_entity("skeleton_viewer_mesh");

		component_skinned_mesh_renderer_t& skinned_renderer = ecs_helpers_t::table_add_or_get_as<component_skinned_mesh_renderer_t>(world.get_component_table(type_id_t<component_skinned_mesh_renderer_t>::value), _display_entity);

		skinned_renderer.mesh	  = _preview_mesh;
		skinned_renderer.skeleton = _skeleton_guid;

		const editor_asset_t* mesh_asset = editor_asset_manager_t::get().find_asset(_preview_mesh);
		mesh_def_t			  mesh_def	 = {};

		if (mesh_asset != nullptr && mesh_asset->asset_type == editor_asset_type_e::mesh && editor_asset_util_t::load_mesh_def(*mesh_asset, mesh_def) && !mesh_def.preview_materials.empty())
		{
			for (const resource_handle_t material : mesh_def.preview_materials)
				skinned_renderer.materials.push_back(material);
		}
		else
		{
			for (size_t i = 0; i < decltype(skinned_renderer.materials)::capacity; ++i)
				skinned_renderer.materials.push_back(DEFAULT_OPAQUE_MATERIAL_ASSET_GUID);
		}

		update_animation_player(true);
		world.scan_for_resources(_display_entity, true);
	}

	void editor_panel_skeleton_viewer_t::clear_display_entity()
	{
		if (_display_entity == NULL_ENTITY_ID)
			return;

		world_t& world = editor_world_controller_t::get().get_editor_world(_world)->get_world();

		world.destroy_entity(_display_entity);
		_display_entity = NULL_ENTITY_ID;
	}

	void editor_panel_skeleton_viewer_t::draw_skeleton(world_t& world) const
	{
		const ecs_component_table_t&					system_skinned_table = world.get_component_table(type_id_t<component_system_skinned_mesh_renderer_t>::value);
		const component_system_skinned_mesh_renderer_t* system_skinned		 = ecs_helpers_t::table_find_as_const<component_system_skinned_mesh_renderer_t>(system_skinned_table, _display_entity);

		if (system_skinned == nullptr || !system_skinned->final_bones_calculated)
			return;

		resource_manager_t&		  resource_manager = resource_manager_t::get();
		const skeleton_runtime_t* skeleton		   = resource_manager.find_runtime<skeleton_runtime_t>(system_skinned->skeleton);

		if (skeleton == nullptr)
			return;

		const skeleton_joint_runtime_t*		 joints			= resource_manager.get_memory().get<skeleton_joint_runtime_t>(skeleton->joints);
		const span_t<const animation_bone_t> bones			= world.get_animation_controller().get_bones(system_skinned->bones_handle);
		const mat4x3_t						 mesh_transform = world.calculate_transform_direct(_display_entity);
		world_debug_draw_t&					 debug_draw		= world.get_debug_draw();

		frame_vector_t<u8> selected = {};

		selected.resize(skeleton->joint_count, 0);

		for (const u32 joint_index : _selected_joints)
			selected[joint_index] = 1;

		for (u32 joint_index = 0; joint_index < skeleton->joint_count; ++joint_index)
		{
			const skeleton_joint_runtime_t& joint = joints[joint_index];

			if (joint.parent_index == SKELETON_JOINT_NO_PARENT)
				continue;

			const vec3f_t position		  = mesh_transform * (bones.data[joint_index].bone_transform * joint.bind_global.get_translation());
			const vec3f_t parent_position = mesh_transform * (bones.data[joint.parent_index].bone_transform * joints[joint.parent_index].bind_global.get_translation());

			debug_draw.draw_line(parent_position, position, selected[joint_index] != 0 ? color_t::red : color_t::white, 2.0f, debug_draw_depth_e::always_visible);
		}

		if (_selected_joint_index != SKELETON_JOINT_NO_PARENT)
		{
			const mat4x3_t transform = mesh_transform * bones.data[_selected_joint_index].bone_transform * joints[_selected_joint_index].bind_global;
			const f32	   length	 = math::max(0.05f, (_skeleton.local_bounds.bounds_max - _skeleton.local_bounds.bounds_min).magnitude() * 0.08f) * 0.2f;

			editor_world_util_t::draw_transform_axes(debug_draw, transform, length, 2.0f, debug_draw_depth_e::always_visible);
		}
	}

	void editor_panel_skeleton_viewer_t::init_animation_controls()
	{
		const editor_theme_t& theme	   = editor_theme_t::get();
		const ui::widget_id_t controls = _ui->allocate_widget();

		_ui->set_widget_debug_name(controls, "skeleton_animation_controls");
		_ui->get_tree().attach(_right_content, controls);

		ui::layout_in_t& in = _ui->get_tree().in(controls);

		in.flow			 = ui::flow_e::row;
		in.child_spacing = theme.item_spacing;
		in.size_mode_x	 = ui::axis_mode_e::sum_children;
		in.size_mode_y	 = ui::axis_mode_e::fixed;
		in.size_value.y	 = theme.item_area_height;
		in.pos_mode_x	 = ui::pos_mode_e::relative_in_parent;
		in.pos_value.x	 = 0.5f;
		in.anchor_x		 = ui::anchor_e::center;

		_animation_play_button.init(*_ui,
									controls,
									{
										.toggled_frame_color = theme.color_accent2_dim,
										.hover_color		 = theme.color_panel_light1,
										.toggled_hover_color = theme.color_accent2_dim,
										.press_color		 = theme.color_frame_light,
										.icon_color			 = theme.color_accent2,
										.disabled_color		 = theme.color_text_disabled,
										.toggled_icon_color	 = theme.color_text0,
										.icon				 = ICON_PLAY,
										.toggled_icon		 = ICON_PAUSE,
										.tooltip			 = "Play",
										.on_clicked			 = on_animation_play_pressed,
										.user_data			 = this,
										.size				 = theme.item_area_height,
										.icon_size			 = theme.text_big_px_size,
										.rounding			 = theme.item_rounding,
										.toggle_enabled		 = true,
									});

		_animation_reset_button.init(*_ui,
									 controls,
									 {
										 .hover_color	 = theme.color_panel_light1,
										 .press_color	 = theme.color_frame_light,
										 .icon_color	 = theme.color_text0,
										 .disabled_color = theme.color_text_disabled,
										 .icon			 = ICON_RESET,
										 .tooltip		 = "Reset",
										 .on_clicked	 = on_animation_reset_pressed,
										 .user_data		 = this,
										 .size			 = theme.item_area_height,
										 .icon_size		 = theme.text_big_px_size,
										 .rounding		 = theme.item_rounding,
									 });

		refresh_animation_controls();
	}

	void editor_panel_skeleton_viewer_t::refresh_animation_controls()
	{
		const bool disabled = _preview_animation == NULL_RESOURCE_HANDLE || _display_entity == NULL_ENTITY_ID;

		_animation_play_button.set_toggled(_is_animation_playing);
		_animation_play_button.set_disabled(disabled);
		_animation_reset_button.set_disabled(disabled);
	}

	void editor_panel_skeleton_viewer_t::update_animation_player(bool reset)
	{
		if (_display_entity == NULL_ENTITY_ID)
		{
			refresh_animation_controls();
			return;
		}

		world_t&			   world   = editor_world_controller_t::get().get_editor_world(_world)->get_world();
		ecs_component_table_t& players = world.get_component_table(type_id_t<component_animation_player_t>::value);

		if (_preview_animation == NULL_RESOURCE_HANDLE)
		{
			if (ecs_t::table_has(players, _display_entity))
				ecs_t::table_remove(players, _display_entity);
		}
		else
		{
			component_animation_player_t& player = ecs_helpers_t::table_add_or_get_as<component_animation_player_t>(players, _display_entity);

			player.animation		= _preview_animation;
			player.mask				= _active_mask == UINT32_MAX ? NULL_SID : TO_SID(static_cast<const char*>(_skeleton.masks[_active_mask].name));
			player.speed_multiplier = _is_animation_playing ? 1.0f : 0.0f;
			player.is_looping		= true;
			player.is_scrub			= false;

			if (reset)
			{
				component_system_animation_player_t* system_player = ecs_helpers_t::table_find_as<component_system_animation_player_t>(world.get_component_table(type_id_t<component_system_animation_player_t>::value), _display_entity);

				if (system_player != nullptr)
					system_player->sample_time = 0.0f;
			}
		}

		refresh_animation_controls();
	}

	void editor_panel_skeleton_viewer_t::refresh_preview_animation_reference()
	{
		u64* field = &_preview_animation;

		_preview_animation_reference.set_reference({
			.callbacks	= {.edited = on_preview_animation_edited, .user_data = this},
			.fields		= {.data = &field, .size = 1},
			.asset_type = editor_asset_type_e::animation,
		});
	}

	void editor_panel_skeleton_viewer_t::on_preview_animation_edited(void* user_data)
	{
		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);
		editor_world_t&					world  = *editor_world_controller_t::get().get_editor_world(viewer._world);

		world.cancel_gizmo_action();
		++viewer._slot_generation;
		viewer._skeleton.preview_animation = viewer._preview_animation;
		viewer._is_animation_playing	   = false;
		viewer.update_animation_player(true);

		if (viewer._display_entity != NULL_ENTITY_ID)
			world.get_world().scan_for_resources(viewer._display_entity, true);
	}

	void editor_panel_skeleton_viewer_t::on_animation_play_pressed(bool toggled, void* user_data)
	{
		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);

		editor_world_controller_t::get().get_editor_world(viewer._world)->cancel_gizmo_action();
		++viewer._slot_generation;
		viewer._is_animation_playing = toggled;
		viewer.update_animation_player(false);
	}

	void editor_panel_skeleton_viewer_t::on_animation_reset_pressed(bool toggled, void* user_data)
	{
		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);

		editor_world_controller_t::get().get_editor_world(viewer._world)->cancel_gizmo_action();
		++viewer._slot_generation;
		viewer._is_animation_playing = false;
		viewer.update_animation_player(true);
	}

	void editor_panel_skeleton_viewer_t::on_save_changes_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left)
			return;

		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);

		editor_world_controller_t::get().get_editor_world(viewer._world)->end_gizmo_action();
		router.set_focus(id, false);
		on_slot_fields_edit_submitted(&viewer);
		viewer.finish_mask_edit();

		nlohmann::json embedded_source = nlohmann::json::object();

		if (!reflection_registry_t::get().type_to_json(type_id_t<skeleton_def_t>::value, &viewer._skeleton, nullptr, embedded_source))
		{
			SFG_ERR("failed to serialize skeleton definition for asset {0}", viewer._skeleton_guid);
			return;
		}

		if (!editor_asset_manager_t::get().save_and_cook_embedded_asset_async(viewer._skeleton_guid, embedded_source))
			SFG_ERR("failed to save and queue cooking for skeleton asset {0}", viewer._skeleton_guid);
	}

	void editor_panel_skeleton_viewer_t::refresh_preview_mesh_reference()
	{
		if (_ui == nullptr)
			return;

		u64* preview_mesh_field = &_preview_mesh;

		_preview_mesh_reference.set_reference({
			.callbacks =
				{
					.edited	   = on_preview_mesh_edited,
					.user_data = this,
				},
			.fields		= {.data = &preview_mesh_field, .size = 1},
			.asset_type = editor_asset_type_e::mesh,
		});
	}

	void editor_panel_skeleton_viewer_t::refresh_info()
	{
		if (_ui == nullptr)
			return;

		_joint_count_text = "-";
		_root_joint_text  = "-";

		if (_skeleton_guid != 0)
		{
			if (_root_joint_index == UINT32_MAX)
			{
				_joint_count_text = "Failed";
			}
			else
			{
				_joint_count_text = std::to_string(_skeleton.joints.size());
				_root_joint_text  = std::to_string(_root_joint_index);
			}
		}

		_ui->set_widget_text(_joint_count_value, _joint_count_text.c_str());
		_ui->set_widget_text(_root_joint_value, _root_joint_text.c_str());

		ui::paint_layer_t&		  paint		  = _ui->get_paint();
		const editor_theme_t&	  theme		  = editor_theme_t::get();
		const ui::vg_text_style_t value_paint = {
			.font		 = theme.font_default,
			.color		 = _joint_count_text == "Failed" ? theme.color_accent_warn : theme.color_text0,
			.point_size	 = theme.text_default_px_size,
			.spacing	 = 0,
			.raster_mode = editor_text_rasterization_t::get_rasterization_type(),
		};
		paint.set_text(_joint_count_value, _ui->widget_text(_joint_count_value), _ui->widget_text_len(_joint_count_value), value_paint);
		paint.set_text(_root_joint_value, _ui->widget_text(_root_joint_value), _ui->widget_text_len(_root_joint_value), value_paint);
	}

	void editor_panel_skeleton_viewer_t::refresh_slot_entities()
	{
		world_t&	 world		= editor_world_controller_t::get().get_editor_world(_world)->get_world();
		const size_t slot_count = _skeleton.slots.size();

		for (size_t i = slot_count; i < _slot_previews.size(); ++i)
		{
			if (_slot_previews[i].entity != NULL_ENTITY_ID)
				world.destroy_entity(_slot_previews[i].entity);
		}

		_slot_previews.resize(slot_count);

		for (size_t i = 0; i < slot_count; ++i)
		{
			const skeleton_slot_def_t& slot	   = _skeleton.slots[i];
			slot_preview_t&			   preview = _slot_previews[i];

			if (preview.mesh == slot.preview_mesh)
				continue;

			if (preview.entity != NULL_ENTITY_ID)
				world.destroy_entity(preview.entity);

			preview = {.mesh = slot.preview_mesh};

			if (slot.preview_mesh == NULL_RESOURCE_HANDLE)
				continue;

			preview.entity = world.create_entity("skeleton_viewer_slot");

			component_mesh_renderer_t& renderer = ecs_helpers_t::table_add_or_get_as<component_mesh_renderer_t>(world.get_component_table(type_id_t<component_mesh_renderer_t>::value), preview.entity);
			const editor_asset_t*	   asset	= editor_asset_manager_t::get().find_asset(slot.preview_mesh);
			mesh_def_t				   mesh_def = {};

			renderer.mesh = slot.preview_mesh;

			if (asset != nullptr && asset->asset_type == editor_asset_type_e::mesh && editor_asset_util_t::load_mesh_def(*asset, mesh_def) && !mesh_def.preview_materials.empty())
			{
				for (const resource_handle_t material : mesh_def.preview_materials)
					renderer.materials.push_back(material);
			}
			else
			{
				for (size_t material_index = 0; material_index < decltype(renderer.materials)::capacity; ++material_index)
					renderer.materials.push_back(DEFAULT_OPAQUE_MATERIAL_ASSET_GUID);
			}

			world.scan_for_resources(preview.entity, true);
		}

		update_slot_entity_transforms(world);
	}

	void editor_panel_skeleton_viewer_t::update_slot_entity_transforms(world_t& world)
	{
		if (_slot_previews.empty())
			return;

		const component_system_skinned_mesh_renderer_t* skinned = ecs_helpers_t::table_find_as_const<component_system_skinned_mesh_renderer_t>(world.get_component_table(type_id_t<component_system_skinned_mesh_renderer_t>::value), _display_entity);

		if (skinned == nullptr || !skinned->final_bones_calculated)
			return;

		resource_manager_t&		  resources = resource_manager_t::get();
		const skeleton_runtime_t* skeleton	= resources.find_runtime<skeleton_runtime_t>(skinned->skeleton);

		if (skeleton == nullptr)
			return;

		const skeleton_joint_runtime_t*		 joints			= resources.get_memory().get<skeleton_joint_runtime_t>(skeleton->joints);
		const span_t<const animation_bone_t> bones			= world.get_animation_controller().get_bones(skinned->bones_handle);
		const mat4x3_t						 mesh_transform = world.calculate_transform_direct(_display_entity);
		bool								 updated		= false;

		for (size_t i = 0; i < _slot_previews.size(); ++i)
		{
			const skeleton_slot_def_t& slot	   = _skeleton.slots[i];
			const slot_preview_t&	   preview = _slot_previews[i];

			if (preview.entity == NULL_ENTITY_ID || slot.slot_joint_index == SKELETON_JOINT_NO_PARENT)
				continue;

			const mat4x3_t parent	= mesh_transform * bones.data[slot.slot_joint_index].bone_transform * joints[slot.slot_joint_index].bind_global;
			vec3f_t		   position = vec3f_t::zero;
			quat_t		   rotation = quat_t::identity;
			vec3f_t		   scale	= vec3f_t::one;

			parent.decompose(position, rotation, scale);
			world.teleport_entity(preview.entity, parent * slot.local_position, (rotation * slot.local_rotation).normalized(), slot.preview_scale);
			updated = true;
		}

		if (updated)
			world.update_world_transforms(false);
	}

	void editor_panel_skeleton_viewer_t::on_slot_preview_mesh_edited(void* user_data)
	{
		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);

		if (viewer._selected_slot_index == UINT32_MAX)
		{
			viewer.refresh_slot_fields();

			return;
		}

		viewer._skeleton.slots[viewer._selected_slot_index].preview_mesh = viewer._slot_preview_mesh;
		viewer.refresh_slot_entities();
	}

	void editor_panel_skeleton_viewer_t::init_slot_fields()
	{
		const editor_theme_t&		theme			 = editor_theme_t::get();
		const editor_property_row_t preview_mesh_row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _right_content, "Preview Mesh");
		u64*						mesh_field		 = &_slot_preview_mesh;

		_slot_preview_mesh_reference.init(*_ui,
										  preview_mesh_row.right,
										  {
											  .callbacks  = {.edit_begin = on_slot_fields_edit_begin, .edited = on_slot_preview_mesh_edited, .edit_submitted = on_slot_fields_edit_submitted, .user_data = this},
											  .fields	  = {.data = &mesh_field, .size = 1},
											  .asset_type = editor_asset_type_e::mesh,
										  });

		editor_dividers_t::add_divider_hor(*_ui, _right_content, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);

		const editor_widget_callbacks_t callbacks{
			.edit_begin		= on_slot_fields_edit_begin,
			.edited			= on_slot_fields_edited,
			.edit_submitted = on_slot_fields_edit_submitted,
			.user_data		= this,
		};

		const editor_property_row_t preview_scale_row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _right_content, "Preview Scale");

		_slot_preview_scale_field.init(*_ui, preview_scale_row.right, {.callbacks = callbacks});
		editor_dividers_t::add_divider_hor(*_ui, _right_content, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);

		const editor_property_row_t position_row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _right_content, "Local Position");

		_slot_position_field.init(*_ui, position_row.right, {.callbacks = callbacks});
		editor_dividers_t::add_divider_hor(*_ui, _right_content, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);

		const editor_property_row_t rotation_row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _right_content, "Local Rotation");

		_slot_rotation_field.init(*_ui, rotation_row.right, {.callbacks = callbacks});

		const ui::widget_id_t fields[] = {_slot_preview_mesh_reference.get_root(), _slot_preview_scale_field.get_root(), _slot_position_field.get_root(), _slot_rotation_field.get_root()};

		for (const ui::widget_id_t field : fields)
		{
			ui::layout_in_t& in = _ui->get_tree().in(field);

			in.size_mode_x = ui::axis_mode_e::fill;
			in.pos_mode_y  = ui::pos_mode_e::relative_in_parent;
			in.pos_value.y = 0.5f;
			in.anchor_y	   = ui::anchor_e::center;
		}

		refresh_slot_fields();
	}

	void editor_panel_skeleton_viewer_t::refresh_slot_fields()
	{
		const bool				enabled = _selected_slot_index != UINT32_MAX;
		const resource_handle_t mesh	= enabled ? _skeleton.slots[_selected_slot_index].preview_mesh : NULL_RESOURCE_HANDLE;

		if (_slot_preview_mesh != mesh)
		{
			_slot_preview_mesh = mesh;

			u64* mesh_field = &_slot_preview_mesh;

			_slot_preview_mesh_reference.set_reference({
				.callbacks	= {.edit_begin = on_slot_fields_edit_begin, .edited = on_slot_preview_mesh_edited, .edit_submitted = on_slot_fields_edit_submitted, .user_data = this},
				.fields		= {.data = &mesh_field, .size = 1},
				.asset_type = editor_asset_type_e::mesh,
			});
		}

		ui::layout_tree_t&	  tree	   = _ui->get_tree();
		const ui::widget_id_t fields[] = {_slot_preview_mesh_reference.get_root(), _slot_preview_scale_field.get_root(), _slot_position_field.get_root(), _slot_rotation_field.get_root()};

		for (const ui::widget_id_t field : fields)
		{
			if (enabled)
				tree.in(field).flags &= ~ui::wf_disabled;
			else
				tree.in(field).flags |= ui::wf_disabled;
		}

		_slot_position_field.set_value(enabled ? _skeleton.slots[_selected_slot_index].local_position : vec3f_t::zero);
		_slot_preview_scale_field.set_value(enabled ? _skeleton.slots[_selected_slot_index].preview_scale : vec3f_t::one);
		_slot_rotation_field.set_value(enabled ? _skeleton.slots[_selected_slot_index].local_rotation : quat_t::identity);
	}

	void editor_panel_skeleton_viewer_t::refresh_mask_button()
	{
		ui::layout_in_t& in = _ui->get_tree().in(_make_mask_button.get_root());

		if (_selected_joints.empty())
			in.flags |= ui::wf_disabled;
		else
			in.flags &= ~ui::wf_disabled;
	}

	void editor_panel_skeleton_viewer_t::refresh_joint_selection()
	{
		for (u32 i = 0; i < _joint_rows.size(); ++i)
			update_joint_row_background(i);

		refresh_slot_fields();
		refresh_mask_button();
	}

	void editor_panel_skeleton_viewer_t::refresh_mask_backgrounds()
	{
		const editor_theme_t& theme = editor_theme_t::get();

		for (u32 index = 0; index < _mask_items.size(); ++index)
		{
			const vec4f_t& color = index == _editing_mask ? theme.color_accent0_dim : theme.color_frame;

			_ui->get_paint().set_rect(_mask_items[index]->root, {.fill_color_a = color, .fill_color_b = color});
			_mask_items[index]->activate_button.set_toggled(index == _active_mask);
		}
	}

	void editor_panel_skeleton_viewer_t::finish_mask_edit()
	{
		if (_editing_mask == UINT32_MAX)
			return;

		const u32 index = _editing_mask;

		if (!editor_command_skeleton_edit_t::begin(*this))
			return;

		_skeleton.masks[index].joint_indices = _selected_joints;
		std::sort(_skeleton.masks[index].joint_indices.begin(), _skeleton.masks[index].joint_indices.end());
		_editing_mask = UINT32_MAX;
		editor_command_skeleton_edit_t::submit(*this, "Skeleton Edit Mask Joints", false);
		refresh_mask_backgrounds();
	}

	void editor_panel_skeleton_viewer_t::on_mask_edit_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left)
			return;

		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);
		const auto						item   = std::find_if(viewer._mask_items.begin(), viewer._mask_items.end(), [id](const unique_t<mask_item_t>& value) { return value->edit_button.get_root() == id; });

		SFG_ASSERT(item != viewer._mask_items.end());

		const u32  index	   = static_cast<u32>(item - viewer._mask_items.begin());
		const bool was_editing = viewer._editing_mask == index;

		router.set_focus(id, false);
		viewer.finish_mask_edit();

		if (was_editing)
			return;

		editor_world_controller_t::get().get_editor_world(viewer._world)->cancel_gizmo_action();
		on_slot_fields_edit_submitted(&viewer);
		viewer._editing_mask		 = index;
		viewer._selected_joints		 = viewer._skeleton.masks[index].joint_indices;
		viewer._selected_joint_index = viewer._selected_joints.empty() ? SKELETON_JOINT_NO_PARENT : viewer._selected_joints.back();
		viewer._selected_slot_index	 = UINT32_MAX;
		++viewer._slot_generation;
		viewer.refresh_joint_selection();
		viewer.refresh_mask_backgrounds();
	}

	void editor_panel_skeleton_viewer_t::on_mask_remove_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left)
			return;

		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);
		const auto						item   = std::find_if(viewer._mask_items.begin(), viewer._mask_items.end(), [id](const unique_t<mask_item_t>& value) { return value->remove_button.get_root() == id; });

		SFG_ASSERT(item != viewer._mask_items.end());

		const u32 index = static_cast<u32>(item - viewer._mask_items.begin());

		router.set_focus(viewer._right_content, false);
		viewer.finish_mask_edit();
		on_slot_fields_edit_submitted(&viewer);

		if (!editor_command_skeleton_edit_t::begin(viewer))
			return;

		viewer.clear_mask_items();
		viewer._skeleton.masks.erase(viewer._skeleton.masks.begin() + index);

		if (viewer._active_mask == index)
			viewer._active_mask = UINT32_MAX;
		else if (viewer._active_mask != UINT32_MAX && viewer._active_mask > index)
			--viewer._active_mask;

		editor_command_skeleton_edit_t::submit(viewer, "Skeleton Remove Mask", false);
		viewer.refresh_mask_items();
	}

	void editor_panel_skeleton_viewer_t::on_mask_activate_toggled(bool is_toggled, void* user_data)
	{
		mask_item_t&					item   = *static_cast<mask_item_t*>(user_data);
		editor_panel_skeleton_viewer_t& viewer = *item.viewer;
		const auto						active = std::find_if(viewer._mask_items.begin(), viewer._mask_items.end(), [&item](const unique_t<mask_item_t>& value) { return value.get() == &item; });

		viewer._ui->get_input().set_focus(viewer._right_content, false);
		viewer._active_mask = is_toggled ? static_cast<u32>(active - viewer._mask_items.begin()) : UINT32_MAX;
		viewer.refresh_mask_backgrounds();
		viewer.update_animation_player(false);
	}

	void editor_panel_skeleton_viewer_t::on_mask_name_edit_begin(void* user_data)
	{
		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);

		viewer._mask_name_edit_active = editor_command_skeleton_edit_t::begin(viewer);
	}

	void editor_panel_skeleton_viewer_t::on_mask_name_edit_submitted(void* user_data)
	{
		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);

		if (!viewer._mask_name_edit_active)
			return;

		viewer._mask_name_edit_active = false;
		editor_command_skeleton_edit_t::submit(viewer, "Skeleton Rename Mask", false);

		if (viewer._display_entity != NULL_ENTITY_ID)
			viewer.update_animation_player(false);
	}

	void editor_panel_skeleton_viewer_t::clear_mask_items()
	{
		for (const unique_t<mask_item_t>& item : _mask_items)
		{
			item->name_field.uninit();
			item->activate_button.uninit();
			item->edit_button.uninit();
			item->remove_button.uninit();
			_ui->deallocate_widget(item->root);
		}

		_mask_items.resize(0);
	}

	void editor_panel_skeleton_viewer_t::refresh_mask_items()
	{
		clear_mask_items();

		ui::layout_tree_t&	  tree	= _ui->get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		_mask_items.reserve(_skeleton.masks.size());

		for (skeleton_mask_def_t& mask : _skeleton.masks)
		{
			unique_t<mask_item_t> item = make_unique<mask_item_t>();

			item->root = _ui->allocate_widget();
			_ui->set_widget_debug_name(item->root, "skeleton_mask");
			tree.attach(_mask_list, item->root);

			ui::layout_in_t& in = tree.in(item->root);
			in.size_mode_x		= ui::axis_mode_e::parent_relative;
			in.size_mode_y		= ui::axis_mode_e::fixed;
			in.size_value		= {1.0f, theme.item_area_height * 2.0f};
			in.flow				= ui::flow_e::column;
			_ui->get_paint().set_rect(item->root, {.fill_color_a = theme.color_frame, .fill_color_b = theme.color_frame});

			const editor_property_row_t name_row = editor_misc_widgets_t::make_property_row_with_label(*_ui, item->root, "Name");
			u8*							name	 = reinterpret_cast<u8*>(mask.name);

			tree.draw_order(name_row.label) = tree.draw_order_const(item->root) + 1;

			item->name_field.init(*_ui,
								  name_row.right,
								  {
									  .field	 = {.fields = {.data = &name, .size = 1}, .field_size = sizeof(mask.name), .type = editor_input_field_field_type_e::char_array},
									  .callbacks = {.edit_begin = on_mask_name_edit_begin, .edit_submitted = on_mask_name_edit_submitted, .user_data = this},
								  });

			const editor_property_row_t edit_row = editor_misc_widgets_t::make_property_row(*_ui, item->root);

			item->viewer = this;
			item->activate_button.init(*_ui,
									   edit_row.right,
									   {
										   .frame_color			= theme.color_panel_light,
										   .toggled_frame_color = theme.color_accent0_dim,
										   .hover_color			= theme.color_frame_light,
										   .toggled_hover_color = theme.color_accent0,
										   .pressed_color		= theme.color_frame,
										   .text_color			= theme.color_text0,
										   .toggled_text_color	= theme.color_text0,
										   .text				= "Activate",
										   .toggled_text		= "Activate",
										   .on_toggle			= on_mask_activate_toggled,
										   .user_data			= item.get(),
									   });

			item->edit_button.init(*_ui, edit_row.right, {.text = "Edit"});
			_ui->get_input().set_listener(item->edit_button.get_root(), {.on_click = on_mask_edit_pressed, .user_data = this});

			item->remove_button.init(*_ui, edit_row.right, {.text = "Remove"});
			_ui->get_input().set_listener(item->remove_button.get_root(), {.on_click = on_mask_remove_pressed, .user_data = this});

			const ui::widget_id_t controls[] = {item->name_field.get_root(), item->activate_button.get_root(), item->edit_button.get_root(), item->remove_button.get_root()};

			for (const ui::widget_id_t control : controls)
			{
				ui::layout_in_t& control_in = tree.in(control);
				control_in.size_mode_x		= ui::axis_mode_e::fill;
				control_in.pos_mode_y		= ui::pos_mode_e::relative_in_parent;
				control_in.pos_value.y		= 0.5f;
				control_in.anchor_y			= ui::anchor_e::center;
			}

			_mask_items.push_back(std::move(item));
		}

		refresh_mask_backgrounds();

		if (_display_entity != NULL_ENTITY_ID)
			update_animation_player(false);
	}

	void editor_panel_skeleton_viewer_t::on_make_mask_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left)
			return;

		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);

		SFG_ASSERT(!viewer._selected_joints.empty());

		router.set_focus(id, false);
		viewer.finish_mask_edit();
		on_slot_fields_edit_submitted(&viewer);

		if (!editor_command_skeleton_edit_t::begin(viewer))
			return;

		viewer.clear_mask_items();

		skeleton_mask_def_t mask{.name = "Mask"};
		frame_vector_t<u8>	included = {};

		included.resize(viewer._skeleton.joints.size(), 0);

		for (const u32 joint : viewer._selected_joints)
			included[joint] = 1;

		mask.joint_indices.reserve(viewer._skeleton.joints.size());

		for (const u32 joint : viewer._skeleton.evaluation_order)
		{
			const u32 parent = viewer._skeleton.joints[joint].parent_index;

			if (parent != SKELETON_JOINT_NO_PARENT && included[parent] != 0)
				included[joint] = 1;

			if (included[joint] != 0)
				mask.joint_indices.push_back(joint);
		}

		std::sort(mask.joint_indices.begin(), mask.joint_indices.end());
		viewer._skeleton.masks.push_back(std::move(mask));
		editor_command_skeleton_edit_t::submit(viewer, "Skeleton Make Mask", false);
		viewer.refresh_mask_items();
	}

	void editor_panel_skeleton_viewer_t::select_row(u32 row_index, bool additive)
	{
		editor_world_controller_t::get().get_editor_world(_world)->cancel_gizmo_action();

		on_slot_fields_edit_submitted(this);

		const joint_row_t& row = _joint_rows[row_index];

		_ui->get_input().set_focus(row.root, false);

		if (row.slot_index != UINT32_MAX)
			finish_mask_edit();

		if (!additive || row.slot_index != UINT32_MAX)
			_selected_joints.resize(0);

		if (row.slot_index == UINT32_MAX)
		{
			const auto selected = std::find(_selected_joints.begin(), _selected_joints.end(), row.joint_index);

			if (selected == _selected_joints.end())
				_selected_joints.push_back(row.joint_index);
			else
				_selected_joints.erase(selected);
		}

		_selected_joint_index = _selected_joints.empty() ? SKELETON_JOINT_NO_PARENT : _selected_joints.back();
		_selected_slot_index  = row.slot_index;
		++_slot_generation;

		refresh_joint_selection();
	}

	void editor_panel_skeleton_viewer_t::add_slot()
	{
		SFG_ASSERT(_selected_joint_index != SKELETON_JOINT_NO_PARENT);

		on_slot_fields_edit_submitted(this);
		finish_mask_edit();

		if (!editor_command_skeleton_edit_t::begin(*this))
			return;

		const skeleton_slot_def_t slot{
			.slot_name		  = "Slot",
			.slot_joint_index = _selected_joint_index,
		};

		_joint_rows[_selected_joint_index].expanded = true;
		_skeleton.slots.push_back(slot);
		_selected_slot_index  = static_cast<u32>(_skeleton.slots.size() - 1);
		_selected_joint_index = SKELETON_JOINT_NO_PARENT;
		++_slot_generation;

		refresh_joint_hierarchy();
		editor_command_skeleton_edit_t::submit(*this, "Skeleton Add Slot", false);
	}

	void editor_panel_skeleton_viewer_t::duplicate_slot()
	{
		SFG_ASSERT(_selected_slot_index != UINT32_MAX);

		editor_world_controller_t::get().get_editor_world(_world)->cancel_gizmo_action();
		on_slot_fields_edit_submitted(this);

		if (!editor_command_skeleton_edit_t::begin(*this))
			return;

		skeleton_slot_def_t slot = _skeleton.slots[_selected_slot_index];
		char*				name = slot.slot_name + std::strlen(slot.slot_name);
		char* const			end	 = slot.slot_name + sizeof(slot.slot_name);

		char_util::append(name, end, " Copy");
		*name = '\0';
		_skeleton.slots.push_back(slot);
		_selected_slot_index = static_cast<u32>(_skeleton.slots.size() - 1);
		++_slot_generation;

		refresh_joint_hierarchy();
		editor_command_skeleton_edit_t::submit(*this, "Skeleton Duplicate Slot", false);
	}

	void editor_panel_skeleton_viewer_t::delete_slot()
	{
		SFG_ASSERT(_selected_slot_index != UINT32_MAX);

		editor_world_controller_t::get().get_editor_world(_world)->cancel_gizmo_action();
		on_slot_fields_edit_submitted(this);

		if (!editor_command_skeleton_edit_t::begin(*this))
			return;

		const slot_preview_t& preview = _slot_previews[_selected_slot_index];

		if (preview.entity != NULL_ENTITY_ID)
			editor_world_controller_t::get().get_editor_world(_world)->get_world().destroy_entity(preview.entity);

		_slot_previews.erase(_slot_previews.begin() + _selected_slot_index);
		_selected_joint_index = _skeleton.slots[_selected_slot_index].slot_joint_index;
		_skeleton.slots.erase(_skeleton.slots.begin() + _selected_slot_index);
		_selected_slot_index = UINT32_MAX;
		++_slot_generation;

		refresh_joint_hierarchy();
		editor_command_skeleton_edit_t::submit(*this, "Skeleton Delete Slot", false);
	}

	void editor_panel_skeleton_viewer_t::rename_slot()
	{
		SFG_ASSERT(_selected_slot_index != UINT32_MAX);

		editor_action_menu_controller_t::find(*_ui)->close_action_menu();

		const joint_row_t&		row		  = _joint_rows[_skeleton.joints.size() + _selected_slot_index];
		const ui::layout_out_t& row_out	  = _ui->get_tree().out(row.root);
		const ui::layout_out_t& label_out = _ui->get_tree().out(row.label);

		_rename_slot_index = _selected_slot_index;
		editor_popup_controller_t::find(*_ui)->request_input_popup({
			.submitted	 = on_slot_rename_submitted,
			.cancelled	 = [](void* user_data) { static_cast<editor_panel_skeleton_viewer_t*>(user_data)->_rename_slot_index = UINT32_MAX; },
			.user_data	 = this,
			.text		 = _skeleton.slots[_rename_slot_index].slot_name,
			.placeholder = "Slot Name",
			.pos		 = {label_out.pos.x, row_out.pos.y},
			.width		 = row_out.pos.x + row_out.size.x - label_out.pos.x,
		});
	}

	void editor_panel_skeleton_viewer_t::on_slot_rename_submitted(const char* value, void* user_data)
	{
		editor_panel_skeleton_viewer_t& viewer	   = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);
		const u32						slot_index = viewer._rename_slot_index;

		viewer._rename_slot_index = UINT32_MAX;

		if (value[0] == '\0')
			return;

		on_slot_fields_edit_submitted(&viewer);

		if (!editor_command_skeleton_edit_t::begin(viewer))
			return;

		skeleton_slot_def_t& slot	= viewer._skeleton.slots[slot_index];
		char*				 name	= slot.slot_name;
		const size_t		 length = math::min(std::strlen(value), sizeof(slot.slot_name) - 1);

		char_util::append(name, slot.slot_name + sizeof(slot.slot_name), value, length);
		viewer._ui->set_widget_text(viewer._joint_rows[viewer._skeleton.joints.size() + slot_index].label, slot.slot_name);
		editor_command_skeleton_edit_t::submit(viewer, "Skeleton Rename Slot", false);
	}

	void editor_panel_skeleton_viewer_t::open_row_menu(const vec2f_t& pos)
	{
		static const editor_action_menu_row_desc_t joint_actions[] = {
			{
				.text	 = "Add Slot",
				.command = SKELETON_VIEWER_ADD_SLOT,
			},
		};
		static const editor_action_menu_row_desc_t slot_actions[] = {
			{
				.text	 = "Rename",
				.command = SKELETON_VIEWER_RENAME_SLOT,
			},
			{
				.text	  = "Duplicate",
				.shortcut = "CTRL+D",
				.command  = SKELETON_VIEWER_DUPLICATE_SLOT,
			},
			{
				.text	  = "Delete",
				.shortcut = "DEL",
				.command  = SKELETON_VIEWER_DELETE_SLOT,
			},
		};
		const bool is_slot = _selected_slot_index != UINT32_MAX;

		editor_action_menu_controller_t::find(*_ui)->request_action_menu({
			.style			   = make_default_action_menu_style(editor_theme_t::get()),
			.rows			   = is_slot ? slot_actions : joint_actions,
			.command_fn		   = on_row_menu_action,
			.command_user_data = this,
			.closed_fn		   = [](void* user_data) { static_cast<editor_panel_skeleton_viewer_t*>(user_data)->_row_menu_open = false; },
			.closed_user_data  = this,
			.pos			   = pos,
			.row_count		   = static_cast<u16>(is_slot ? 3 : 1),
		});
		_row_menu_open = true;
	}

	void editor_panel_skeleton_viewer_t::on_row_menu_action(u16 action, void* user_data)
	{
		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);

		switch (action)
		{
		case SKELETON_VIEWER_ADD_SLOT:
			viewer.add_slot();
			break;
		case SKELETON_VIEWER_RENAME_SLOT:
			viewer.rename_slot();
			break;
		case SKELETON_VIEWER_DUPLICATE_SLOT:
			viewer.duplicate_slot();
			break;
		case SKELETON_VIEWER_DELETE_SLOT:
			viewer.delete_slot();
			break;
		}
	}

	void editor_panel_skeleton_viewer_t::on_hierarchy_key(ui::input_router_t& router, ui::widget_id_t id, const ui::key_event_t& ev, void* user_data)
	{
		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);

		if (ev.action != ui::key_action_e::press || viewer._selected_slot_index == UINT32_MAX || router.is_popup_scope_active())
			return;

		const bool ctrl = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));

		if (ev.key == static_cast<u16>(input_code::key_delete))
			viewer.delete_slot();
		else if (ev.key == static_cast<u16>(input_code::key_d) && ctrl)
			viewer.duplicate_slot();
	}

	void editor_panel_skeleton_viewer_t::refresh_joint_visibility()
	{
		for (const u32 index : _skeleton.evaluation_order)
		{
			joint_row_t& row		  = _joint_rows[index];
			const u32	 parent_index = _skeleton.joints[index].parent_index;

			row.visible = parent_index == SKELETON_JOINT_NO_PARENT || (_joint_rows[parent_index].visible && _joint_rows[parent_index].expanded);
			_ui->get_tree().set_visible(row.root, row.visible);
		}

		for (u32 i = 0; i < _skeleton.slots.size(); ++i)
		{
			joint_row_t& row = _joint_rows[_skeleton.joints.size() + i];

			row.visible = row.joint_index == SKELETON_JOINT_NO_PARENT || (_joint_rows[row.joint_index].visible && _joint_rows[row.joint_index].expanded);
			_ui->get_tree().set_visible(row.root, row.visible);
		}
	}

	bool editor_panel_skeleton_viewer_t::get_joint_world_transform(u32 joint_index, mat4x3_t& transform) const
	{
		if (joint_index == SKELETON_JOINT_NO_PARENT)
			return false;

		world_t&										world	= editor_world_controller_t::get().get_editor_world(_world)->get_world();
		const component_system_skinned_mesh_renderer_t* skinned = ecs_helpers_t::table_find_as_const<component_system_skinned_mesh_renderer_t>(world.get_component_table(type_id_t<component_system_skinned_mesh_renderer_t>::value), _display_entity);

		if (skinned == nullptr || !skinned->final_bones_calculated)
			return false;

		resource_manager_t&		  resources = resource_manager_t::get();
		const skeleton_runtime_t* skeleton	= resources.find_runtime<skeleton_runtime_t>(skinned->skeleton);

		if (skeleton == nullptr)
			return false;

		const skeleton_joint_runtime_t*		 joints = resources.get_memory().get<skeleton_joint_runtime_t>(skeleton->joints);
		const span_t<const animation_bone_t> bones	= world.get_animation_controller().get_bones(skinned->bones_handle);

		transform = world.calculate_transform_direct(_display_entity) * bones.data[joint_index].bone_transform * joints[joint_index].bind_global;

		return true;
	}

	bool editor_panel_skeleton_viewer_t::get_slot_gizmo_target(editor_gizmo_target_t& target)
	{
		if (_selected_slot_index == UINT32_MAX)
			return false;

		const skeleton_slot_def_t& slot	  = _skeleton.slots[_selected_slot_index];
		mat4x3_t				   parent = mat4x3_t::identity;

		if (!get_joint_world_transform(slot.slot_joint_index, parent))
			return false;

		vec3f_t position = vec3f_t::zero;
		quat_t	rotation = quat_t::identity;
		vec3f_t scale	 = vec3f_t::one;

		parent.decompose(position, rotation, scale);
		target.position		 = parent * slot.local_position;
		target.rotation		 = (rotation * slot.local_rotation).normalized();
		target.prev_position = target.position;
		target.prev_rotation = target.rotation;
		target.generation	 = _slot_generation;

		return true;
	}

	bool editor_panel_skeleton_viewer_t::begin_slot_gizmo()
	{
		if (_slot_position_field.is_editing() || _slot_rotation_field.is_editing() || _slot_preview_scale_field.is_editing())
			return false;

		const skeleton_slot_def_t& slot = _skeleton.slots[_selected_slot_index];

		if (!get_joint_world_transform(slot.slot_joint_index, _slot_parent_transform))
			return false;

		vec3f_t position = vec3f_t::zero;
		vec3f_t scale	 = vec3f_t::one;

		_slot_parent_transform.decompose(position, _slot_parent_rotation, scale);
		_slot_initial_position		= slot.local_position;
		_slot_initial_rotation		= slot.local_rotation;
		_slot_initial_preview_scale = slot.preview_scale;
		_slot_initial_absolute		= mat4x3_t::transform(_slot_parent_transform * slot.local_position, (_slot_parent_rotation * slot.local_rotation).normalized(), vec3f_t::one);
		on_slot_fields_edit_submitted(this);
		_slot_gizmo_active = editor_command_skeleton_edit_t::begin(*this);

		return _slot_gizmo_active;
	}

	void editor_panel_skeleton_viewer_t::update_slot_gizmo(const mat4x3_t& delta)
	{
		SFG_ASSERT(_slot_gizmo_active);

		skeleton_slot_def_t&				  slot	   = _skeleton.slots[_selected_slot_index];
		const mat4x3_t						  absolute = delta * _slot_initial_absolute;
		const editor_transform_control_type_e control  = editor_world_controller_t::get().get_editor_world(_world)->get_edit_context().get_transform_control_type();

		bool identity_delta = true;

		for (u32 i = 0; i < 12; ++i)
			identity_delta &= delta[i] == mat4x3_t::identity[i];

		if (identity_delta)
		{
			slot.local_position = _slot_initial_position;
			slot.local_rotation = _slot_initial_rotation;
			slot.preview_scale	= _slot_initial_preview_scale;
		}
		else if (control == editor_transform_control_type_e::move)
			slot.local_position = _slot_parent_transform.to_linear3x3().inversed() * (absolute.get_translation() - _slot_parent_transform.get_translation());
		else
		{
			vec3f_t position = vec3f_t::zero;
			quat_t	rotation = quat_t::identity;
			vec3f_t scale	 = vec3f_t::one;

			absolute.decompose(position, rotation, scale);

			if (control == editor_transform_control_type_e::scale)
				slot.preview_scale = _slot_initial_preview_scale * scale;
			else
				slot.local_rotation = (_slot_parent_rotation.inverse() * rotation).normalized();
		}

		refresh_slot_fields();
	}

	void editor_panel_skeleton_viewer_t::commit_slot_gizmo()
	{
		_slot_gizmo_active = false;
		editor_command_skeleton_edit_t::submit(*this, "Skeleton Transform Slot", false);
		refresh_slot_fields();
	}

	void editor_panel_skeleton_viewer_t::cancel_slot_gizmo()
	{
		SFG_ASSERT(_slot_gizmo_active);

		skeleton_slot_def_t& slot = _skeleton.slots[_selected_slot_index];

		slot.local_position = _slot_initial_position;
		slot.local_rotation = _slot_initial_rotation;
		slot.preview_scale	= _slot_initial_preview_scale;
		_slot_gizmo_active	= false;
		editor_command_skeleton_edit_t::cancel(*this);
		refresh_slot_fields();
	}

	void editor_panel_skeleton_viewer_t::on_slot_fields_edit_begin(void* user_data)
	{
		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);

		editor_world_controller_t::get().get_editor_world(viewer._world)->cancel_gizmo_action();
		++viewer._slot_generation;

		if (viewer._selected_slot_index != UINT32_MAX && !viewer._slot_fields_edit_active)
			viewer._slot_fields_edit_active = editor_command_skeleton_edit_t::begin(viewer);
	}

	void editor_panel_skeleton_viewer_t::on_slot_fields_edit_submitted(void* user_data)
	{
		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);

		if (!viewer._slot_fields_edit_active)
			return;

		viewer._slot_fields_edit_active = false;
		editor_command_skeleton_edit_t::submit(viewer, "Skeleton Edit Slot Property", false);
	}

	void editor_panel_skeleton_viewer_t::on_slot_fields_edited(void* user_data)
	{
		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);
		skeleton_slot_def_t&			slot   = viewer._skeleton.slots[viewer._selected_slot_index];

		slot.local_position = viewer._slot_position_field.get_value();
		slot.local_rotation = viewer._slot_rotation_field.get_value();
		slot.preview_scale	= viewer._slot_preview_scale_field.get_value();
	}

	void editor_panel_skeleton_viewer_t::init_joint_hierarchy()
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		_joint_list_area = _ui->allocate_widget();
		_ui->set_widget_debug_name(_joint_list_area, "skeleton_joint_hierarchy");
		tree.attach(_right_content, _joint_list_area);

		ui::layout_in_t& list_in = tree.in(_joint_list_area);

		list_in.child_clip_mode = ui::clip_mode_e::scissor_rect;
		list_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		list_in.size_mode_y		= ui::axis_mode_e::sum_children;
		list_in.size_value		= {1.0f, 1.0f};
		list_in.flow			= ui::flow_e::column;
		list_in.child_margins	= {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, 0.0f};

		const ui::vg_rect_paint_t list_rect{
			.fill_color_a = theme.color_frame,
			.fill_color_b = theme.color_frame,
		};

		_ui->get_paint().set_rect(_joint_list_area, list_rect);
		_ui->get_input().set_listener(_right_content, {.on_key = on_hierarchy_key, .user_data = this});
	}

	void editor_panel_skeleton_viewer_t::refresh_joint_hierarchy()
	{
		if (_ui == nullptr)
			return;

		if (_selected_slot_index != UINT32_MAX)
			_selected_joints.resize(0);
		else if (_selected_joint_index != SKELETON_JOINT_NO_PARENT && _selected_joints.empty())
			_selected_joints.push_back(_selected_joint_index);

		refresh_mask_button();
		refresh_slot_entities();

		frame_vector_t<u8> expanded = {};

		expanded.reserve(_joint_rows.size());

		for (const joint_row_t& row : _joint_rows)
		{
			expanded.push_back(row.expanded);
			_ui->deallocate_widget(row.root);
		}

		_joint_rows.resize(0);
		refresh_slot_fields();

		if (_root_joint_index == SKELETON_JOINT_NO_PARENT)
			return;

		const u32 joint_count = static_cast<u32>(_skeleton.joints.size());
		const u32 row_count	  = joint_count + static_cast<u32>(_skeleton.slots.size());

		_joint_rows.resize(row_count);

		for (u32 row_index = 0; row_index < row_count; ++row_index)
		{
			joint_row_t& row		  = _joint_rows[row_index];
			const bool	 is_slot	  = row_index >= joint_count;
			const u32	 parent_index = is_slot ? _skeleton.slots[row_index - joint_count].slot_joint_index : _skeleton.joints[row_index].parent_index;

			row.joint_index = is_slot ? parent_index : row_index;
			row.slot_index	= is_slot ? row_index - joint_count : UINT32_MAX;
			row.expanded	= is_slot || row_index >= expanded.size() || expanded[row_index] != 0;

			if (parent_index == SKELETON_JOINT_NO_PARENT)
				continue;

			row.next_sibling					  = _joint_rows[parent_index].first_child;
			_joint_rows[parent_index].first_child = row_index;
		}

		frame_vector_t<u32> pending = {};

		pending.reserve(row_count);

		for (u32 row_index = row_count; row_index != 0; --row_index)
		{
			const joint_row_t& row			= _joint_rows[row_index - 1];
			const u32		   parent_index = row.slot_index == UINT32_MAX ? _skeleton.joints[row.joint_index].parent_index : row.joint_index;

			if (parent_index == SKELETON_JOINT_NO_PARENT)
				pending.push_back(row_index - 1);
		}

		while (!pending.empty())
		{
			const u32 row_index = pending.back();

			pending.pop_back();
			create_joint_row(row_index);

			for (u32 child_index = _joint_rows[row_index].first_child; child_index != SKELETON_JOINT_NO_PARENT; child_index = _joint_rows[child_index].next_sibling)
			{
				_joint_rows[child_index].depth = _joint_rows[row_index].depth + 1;
				pending.push_back(child_index);
			}
		}

		refresh_joint_visibility();

		if (_selected_slot_index != UINT32_MAX)
			_ui->get_input().set_focus(_joint_rows[joint_count + _selected_slot_index].root, false);
		else if (_selected_joint_index != SKELETON_JOINT_NO_PARENT)
			_ui->get_input().set_focus(_joint_rows[_selected_joint_index].root, false);
	}

	void editor_panel_skeleton_viewer_t::create_joint_row(u32 joint_index)
	{
		ui::layout_tree_t&	  tree	  = _ui->get_tree();
		ui::paint_layer_t&	  paint	  = _ui->get_paint();
		const editor_theme_t& theme	  = editor_theme_t::get();
		joint_row_t&		  row	  = _joint_rows[joint_index];
		const bool			  is_slot = row.slot_index != UINT32_MAX;

		row.root = _ui->allocate_widget();
		_ui->set_widget_debug_name(row.root, "skeleton_joint_row");
		tree.attach(_joint_list_area, row.root);
		tree.draw_order(row.root) = tree.draw_order_const(_joint_list_area) + 1;

		ui::layout_in_t& row_in = tree.in(row.root);

		row_in.flags |= ui::wf_input | ui::wf_focusable;
		row_in.size_mode_x	 = ui::axis_mode_e::parent_relative;
		row_in.size_mode_y	 = ui::axis_mode_e::fixed;
		row_in.size_value	 = {1.0f, theme.item_height};
		row_in.flow			 = ui::flow_e::row;
		row_in.child_spacing = theme.item_spacing * 0.5f;
		row_in.child_margins = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal + static_cast<f32>(row.depth) * theme.indent_horizontal * 2.0f};

		const ui::listener_bundle_t row_listener{
			.on_click		 = on_joint_row_clicked,
			.on_double_click = on_joint_row_double_clicked,
			.user_data		 = this,
		};

		_ui->get_input().set_listener(row.root, row_listener);
		update_joint_row_background(joint_index);

		row.fold_icon = _ui->allocate_widget();
		_ui->set_widget_debug_name(row.fold_icon, "skeleton_joint_fold");
		tree.attach(row.root, row.fold_icon);

		ui::layout_in_t& icon_in = tree.in(row.fold_icon);

		icon_in.pos_mode_y	= ui::pos_mode_e::relative_in_parent;
		icon_in.pos_value.y = 0.5f;
		icon_in.anchor_y	= ui::anchor_e::center;
		icon_in.size_mode_x = ui::axis_mode_e::fixed;
		icon_in.size_mode_y = ui::axis_mode_e::fixed;
		icon_in.size_value	= {theme.item_height, theme.item_height};

		row.fold_icon_text = _ui->allocate_widget();
		_ui->set_widget_debug_name(row.fold_icon_text, "skeleton_joint_fold_icon");
		tree.attach(row.fold_icon, row.fold_icon_text);

		ui::layout_in_t& icon_text_in = tree.in(row.fold_icon_text);

		icon_text_in.pos_mode_x	 = ui::pos_mode_e::relative_in_parent;
		icon_text_in.pos_mode_y	 = ui::pos_mode_e::relative_in_parent;
		icon_text_in.pos_value	 = {0.5f, 0.5f};
		icon_text_in.anchor_x	 = ui::anchor_e::center;
		icon_text_in.anchor_y	 = ui::anchor_e::center;
		icon_text_in.size_mode_x = ui::axis_mode_e::fixed;
		icon_text_in.size_mode_y = ui::axis_mode_e::fixed;

		_ui->set_widget_text(row.fold_icon_text, is_slot ? ICON_CUBE : row.first_child == SKELETON_JOINT_NO_PARENT ? "" : row.expanded ? ICON_DD_DOWN : ICON_DD_RIGHT);
		paint.set_text(row.fold_icon_text,
					   _ui->widget_text(row.fold_icon_text),
					   _ui->widget_text_len(row.fold_icon_text),
					   {
						   .font		= theme.font_icons,
						   .color		= is_slot ? theme.color_accent_green : theme.color_text0,
						   .point_size	= theme.icon_default_px_size,
						   .raster_mode = editor_text_rasterization_t::get_rasterization_type(),
					   });

		row.label = _ui->allocate_widget();
		_ui->set_widget_debug_name(row.label, "skeleton_joint_name");
		tree.attach(row.root, row.label);

		ui::layout_in_t& label_in = tree.in(row.label);

		label_in.pos_mode_y	 = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value.y = 0.5f;
		label_in.anchor_y	 = ui::anchor_e::center;
		label_in.size_mode_x = ui::axis_mode_e::fill;
		label_in.size_mode_y = ui::axis_mode_e::fixed;

		const char* label = is_slot ? _skeleton.slots[row.slot_index].slot_name : _skeleton.joints[row.joint_index].name.c_str();

		_ui->set_widget_text(row.label, label[0] == '\0' ? (is_slot ? "Slot" : "Unnamed Joint") : label);
		paint.set_text(row.label,
					   _ui->widget_text(row.label),
					   _ui->widget_text_len(row.label),
					   {
						   .font		= theme.font_default,
						   .color		= theme.color_text0,
						   .point_size	= theme.text_default_px_size,
						   .raster_mode = editor_text_rasterization_t::get_rasterization_type(),
					   });
	}

	void editor_panel_skeleton_viewer_t::update_joint_row_background(u32 joint_index)
	{
		const editor_theme_t&	  theme	   = editor_theme_t::get();
		const joint_row_t&		  row	   = _joint_rows[joint_index];
		const bool				  selected = row.slot_index == UINT32_MAX ? std::find(_selected_joints.begin(), _selected_joints.end(), row.joint_index) != _selected_joints.end() : row.slot_index == _selected_slot_index;
		const ui::vg_rect_paint_t row_rect{
			.fill_color_a  = selected ? theme.color_accent0 : vec4f_t::zero,
			.fill_color_b  = selected ? theme.color_accent0_dim : vec4f_t::zero,
			.rounding	   = theme.item_rounding,
			.rounding_segs = 4,
			.gradient	   = ui::vg_gradient_e::horizontal,
		};

		_ui->get_paint().set_rect(row.root, row_rect);
		_ui->get_paint().set_hover_color(row.root, selected ? theme.color_accent0 : theme.color_panel_light);
		_ui->get_paint().set_press_color(row.root, selected ? theme.color_accent0 : theme.color_light);
	}

	void editor_panel_skeleton_viewer_t::toggle_joint_fold(u32 joint_index)
	{
		joint_row_t& row = _joint_rows[joint_index];

		if (row.first_child == SKELETON_JOINT_NO_PARENT)
			return;

		const editor_theme_t& theme = editor_theme_t::get();

		row.expanded = !row.expanded;
		_ui->set_widget_text(row.fold_icon_text, row.expanded ? ICON_DD_DOWN : ICON_DD_RIGHT);
		_ui->get_paint().set_text(row.fold_icon_text,
								  _ui->widget_text(row.fold_icon_text),
								  _ui->widget_text_len(row.fold_icon_text),
								  {
									  .font		   = theme.font_icons,
									  .color	   = theme.color_text0,
									  .point_size  = theme.icon_default_px_size,
									  .raster_mode = editor_text_rasterization_t::get_rasterization_type(),
								  });

		refresh_joint_visibility();
	}

	void editor_panel_skeleton_viewer_t::on_joint_row_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left && button != ui::mouse_button_e::right)
			return;

		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);
		const auto						row	   = std::find_if(viewer._joint_rows.begin(), viewer._joint_rows.end(), [id](const joint_row_t& value) { return value.root == id; });

		SFG_ASSERT(row != viewer._joint_rows.end());

		const ui::layout_out_t& fold = viewer._ui->get_tree().out(row->fold_icon);
		const rectf_t			fold_bounds{fold.pos.x, fold.pos.y, fold.size.x, fold.size.y};

		if (button == ui::mouse_button_e::left && row->first_child != SKELETON_JOINT_NO_PARENT && fold_bounds.contains(pos))
		{
			viewer.toggle_joint_fold(static_cast<u32>(row - viewer._joint_rows.begin()));

			return;
		}

		const bool ctrl = button == ui::mouse_button_e::left && (process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl)));

		viewer.select_row(static_cast<u32>(row - viewer._joint_rows.begin()), ctrl);

		if (button == ui::mouse_button_e::right)
			viewer.open_row_menu(pos);
	}

	void editor_panel_skeleton_viewer_t::on_joint_row_double_clicked(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left)
			return;

		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);
		const auto						row	   = std::find_if(viewer._joint_rows.begin(), viewer._joint_rows.end(), [id](const joint_row_t& value) { return value.root == id; });

		SFG_ASSERT(row != viewer._joint_rows.end());

		viewer.toggle_joint_fold(static_cast<u32>(row - viewer._joint_rows.begin()));
	}

	void editor_panel_skeleton_viewer_t::apply_pane_split()
	{
		if (_ui != nullptr)
			_ui->get_tree().in(_left_pane).size_value.x = _pane_split;
	}

	void editor_panel_skeleton_viewer_t::on_asset_deletion(editor_asset_manager_t& asset_manager, span_t<const sid_t> asset_ids, void* user_data)
	{
		editor_panel_skeleton_viewer_t& panel			 = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);
		bool							skeleton_deleted = false;

		for (size_t i = 0; i < asset_ids.size; ++i)
		{
			if (asset_ids.data[i] == panel._skeleton_guid)
			{
				skeleton_deleted = true;
				break;
			}
		}

		if (!skeleton_deleted)
			return;

		editor_world_t& editor_world = *editor_world_controller_t::get().get_editor_world(panel._world);

		if (panel._rename_slot_index != UINT32_MAX)
		{
			editor_popup_controller_t::find(*panel._ui)->close_popup();
			panel._rename_slot_index = UINT32_MAX;
		}

		if (panel._row_menu_open)
			editor_action_menu_controller_t::find(*panel._ui)->close_action_menu();

		editor_world.cancel_gizmo_action();
		editor_command_skeleton_edit_t::cancel(panel);
		panel._commands->clear();
		panel._slot_fields_edit_active = false;

		editor_world.get_world().unload_all_used_resources();
		panel._selected_joint_index = SKELETON_JOINT_NO_PARENT;
		panel._selected_slot_index	= UINT32_MAX;
		++panel._slot_generation;

		panel._skeleton_guid	= NULL_SID;
		panel._skeleton			= {};
		panel._root_joint_index = UINT32_MAX;
		panel._asset_name.resize(0);
		panel.set_sub_item_id(NULL_SID);
		panel.refresh_slot_fields();

		editor_surface_controller_t::get().request_close_panel(&panel);
	}

	ui::widget_id_t editor_panel_skeleton_viewer_t::append_property_value_row(const char* label)
	{
		const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _right_content, label);

		return append_value_label(row.right);
	}

	ui::widget_id_t editor_panel_skeleton_viewer_t::append_value_label(ui::widget_id_t parent)
	{
		const editor_theme_t& theme = editor_theme_t::get();

		ui::widget_id_t label = _ui->allocate_widget();
		_ui->set_widget_debug_name(label, "skeleton_viewer_property_value");
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
		_ui->get_paint().set_text(label,
								  _ui->widget_text(label),
								  _ui->widget_text_len(label),
								  {
									  .font		   = theme.font_default,
									  .color	   = theme.color_text0,
									  .point_size  = theme.text_default_px_size,
									  .spacing	   = 0,
									  .raster_mode = editor_text_rasterization_t::get_rasterization_type(),
								  });
		return label;
	}

	void editor_panel_skeleton_viewer_t::on_preview_mesh_edited(void* user_data)
	{
		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);

		viewer._skeleton.preview_mesh = viewer._preview_mesh;
		viewer.create_display_entity();
	}

	void editor_panel_skeleton_viewer_t::on_world_tick(world_t& world, f32 delta_time, void* user_data)
	{
		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);

		viewer.update_slot_entity_transforms(world);
		viewer.draw_skeleton(world);
	}

	void editor_panel_skeleton_viewer_t::on_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		editor_panel_skeleton_viewer_t& panel = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);
		const ui::layout_out_t&			out	  = panel._ui->get_tree().out(panel._root);

		panel._pane_split = math::clamp((pos.x - out.pos.x) / out.size.x, SKELETON_VIEWER_PANE_SPLIT_MIN, SKELETON_VIEWER_PANE_SPLIT_MAX);
		panel.apply_pane_split();
	}
}
