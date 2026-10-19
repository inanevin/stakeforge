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

#include "editor_panel_ragdoll_viewer.hpp"

#include "assets/editor_asset.hpp"
#include "assets/editor_asset_util.hpp"
#include "assets/editor_ragdoll_util.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"
#include "commands/editor_command_ragdoll.hpp"
#include "editor_command_system.hpp"
#include "editor_surface_controller.hpp"
#include "editor_world_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include "world/editor_world.hpp"
#include "world/editor_world_util.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/aabb.hpp>
#include <sfg/math/color.hpp>
#include <sfg/math/math.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>
#include <sfg/runtime/physics/physics_world.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/mesh_def.hpp>
#include <sfg/runtime/resources/physical_material.hpp>
#include <sfg/runtime/world/world_debug_draw.hpp>
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define RAGDOLL_VIEWER_PANE_SPLIT_MIN			   0.45f
#define RAGDOLL_VIEWER_PANE_SPLIT_MAX			   0.85f
#define RAGDOLL_VIEWER_SPLIT_BORDER_THICKNESS_MULT 2.0f

	editor_panel_ragdoll_viewer_t::editor_panel_ragdoll_viewer_t()
	{
		set_type(editor_panel_type_e::ragdoll_viewer);
		refresh_title();
		set_icon(ICON_ANIMATION);
	}

	void editor_panel_ragdoll_viewer_t::serialize(nlohmann::json& j) const
	{
		j				  = nlohmann::json::object();
		j["ragdoll_guid"] = _ragdoll_guid;
		j["asset_name"]	  = _asset_name;
		j["pane_split"]	  = _pane_split;
	}

	void editor_panel_ragdoll_viewer_t::deserialize(const nlohmann::json& j)
	{
		_ragdoll_guid = j.value<sid_t>("ragdoll_guid", NULL_SID);
		set_sub_item_id(_ragdoll_guid);
		_asset_name = j.value<string_t>("asset_name", {});
		_pane_split = math::clamp(j.value<f32>("pane_split", _pane_split), RAGDOLL_VIEWER_PANE_SPLIT_MIN, RAGDOLL_VIEWER_PANE_SPLIT_MAX);
		refresh_title(_asset_name.c_str(), "R: ");
	}

	void editor_panel_ragdoll_viewer_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		editor_panel_t::init(ui, parent);
		_asset_deletion_listener = editor_asset_manager_t::get().add_asset_deletion_listener(on_asset_deletion, this);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::row;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, 0.0f, theme.margin_vertical, 0.0f};

		_left_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_left_pane, "ragdoll_viewer_left_pane");
		tree.attach(_root, _left_pane);

		ui::layout_in_t& left_in = tree.in(_left_pane);
		left_in.flow			 = ui::flow_e::column;
		left_in.child_spacing	 = theme.item_spacing;
		left_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};
		left_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		left_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		left_in.size_value		 = {_pane_split, 1.0f};

		ui::vg_rect_paint_t left_rect = {};
		left_rect.fill_color_a		  = theme.color_frame;
		left_rect.fill_color_b		  = theme.color_frame;
		paint.set_rect(_left_pane, left_rect);

		const ui::widget_id_t toolbar = ui.allocate_widget();
		tree.attach(_left_pane, toolbar);

		ui::layout_in_t& toolbar_in = tree.in(toolbar);
		toolbar_in.flow				= ui::flow_e::row;
		toolbar_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		toolbar_in.size_mode_y		= ui::axis_mode_e::fixed;
		toolbar_in.size_value		= {1.0f, theme.item_height};
		toolbar_in.child_spacing	= theme.item_spacing;

		_simulate_button.init(ui, toolbar, {.text = "Simulate"});
		_reset_button.init(ui, toolbar, {.text = "Reset"});
		_mesh_button.init(ui, toolbar, {.text = "Mesh"});
		_skeleton_button.init(ui, toolbar, {.text = "Skeleton"});
		_capsules_button.init(ui, toolbar, {.text = "Capsules"});
		_limits_button.init(ui, toolbar, {.text = "Limits"});

		for (editor_widget_button_t* button : {&_simulate_button, &_reset_button, &_mesh_button, &_skeleton_button, &_capsules_button, &_limits_button})
		{
			tree.in(button->get_root()).size_mode_x = ui::axis_mode_e::fill;
			ui.get_input().set_listener(button->get_root(), {.on_click = on_action, .user_data = this});
		}

		const ui::widget_id_t viewport = ui.allocate_widget();
		tree.attach(_left_pane, viewport);

		ui::layout_in_t& viewport_in = tree.in(viewport);
		viewport_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		viewport_in.size_mode_y		 = ui::axis_mode_e::fill;
		viewport_in.size_value.x	 = 1.0f;
		viewport_in.child_clip_mode	 = ui::clip_mode_e::scissor_rect;

		_world_view.init(ui, viewport);

		const editor_split_border_t::config_t split_config{
			.on_drag   = on_split_border_drag,
			.user_data = this,
			.direction = editor_split_border_direction_e::horizontal,
		};
		_split_border.init(ui, _root, split_config);

		ui::layout_in_t& border_in = tree.in(_split_border.get_root());
		border_in.size_mode_x	   = ui::axis_mode_e::fixed;
		border_in.size_mode_y	   = ui::axis_mode_e::parent_relative;
		border_in.size_value	   = {theme.border_thickness * RAGDOLL_VIEWER_SPLIT_BORDER_THICKNESS_MULT, 1.0f};

		_right_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_right_pane, "ragdoll_viewer_right_pane");
		tree.attach(_root, _right_pane);

		ui::layout_in_t& right_in = tree.in(_right_pane);
		right_in.flags			  = ui::wf_visible | ui::wf_input | ui::wf_scroll_y;
		right_in.child_clip_mode  = ui::clip_mode_e::scissor_rect;
		right_in.size_mode_x	  = ui::axis_mode_e::fill;
		right_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		right_in.size_value		  = {1.0f, 1.0f};

		_right_content = ui.allocate_widget();
		ui.set_widget_debug_name(_right_content, "ragdoll_viewer_right_content");
		tree.attach(_right_pane, _right_content);
		tree.draw_order(_right_content) = tree.draw_order_const(_right_pane) + 1;
		paint.set_rect(_right_pane, {.fill_color_a = theme.color_frame, .fill_color_b = theme.color_frame});

		ui::layout_in_t& right_content_in = tree.in(_right_content);
		right_content_in.flow			  = ui::flow_e::column;
		right_content_in.child_spacing	  = theme.item_spacing;
		right_content_in.child_margins	  = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};
		right_content_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		right_content_in.size_mode_y	  = ui::axis_mode_e::sum_children;
		right_content_in.size_value		  = {1.0f, 1.0f};

		_right_scrollbar.init(ui, {.target = _right_pane, .axes = editor_scrollbar_axis_y});

		editor_misc_widgets_t::make_section_label(ui, _right_content, "Ragdoll");
		_save_button.init(ui, _right_content, {.text = "Save Changes"});
		_status.init(ui, _right_content, {.text = "Reference Pose"});
		tree.in(_status.get_root()).flags &= ~ui::wf_input;
		_reflection.init(ui, _right_content, {});
		editor_misc_widgets_t::make_section_label(ui, _right_content, "Parts");
		_generate_button.init(ui, _right_content, {.text = "Regenerate From Skeleton"});
		_add_button.init(ui, _right_content, {.text = "Add Part"});
		_remove_button.init(ui, _right_content, {.text = "Remove Selected Part"});

		for (editor_widget_button_t* button : {&_save_button, &_generate_button, &_add_button, &_remove_button, &_status})
		{
			tree.in(button->get_root()).size_mode_x	 = ui::axis_mode_e::parent_relative;
			tree.in(button->get_root()).size_value.x = 1.0f;

			if (button != &_status)
				ui.get_input().set_listener(button->get_root(), {.on_click = on_action, .user_data = this});
		}

		_part_list = ui.allocate_widget();
		tree.attach(_right_content, _part_list);

		ui::layout_in_t& list_in = tree.in(_part_list);
		list_in.flow			 = ui::flow_e::column;
		list_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		list_in.size_mode_y		 = ui::axis_mode_e::fixed;
		list_in.flags |= ui::wf_input | ui::wf_scroll_y;
		list_in.child_clip_mode = ui::clip_mode_e::scissor_rect;
		list_in.size_value.x	= 1.0f;
		list_in.child_spacing	= theme.item_spacing;
		_parts_scrollbar.init(ui, {.target = _part_list, .axes = editor_scrollbar_axis_y});

		editor_misc_widgets_t::make_section_label(ui, _right_content, "Selected Part");
		_part_reflection.init(ui, _right_content, {});

		create_preview_world();

		if (_ragdoll_guid != NULL_SID)
			set_ragdoll(_ragdoll_guid, _asset_name.c_str());
		else
			refresh_reflection();

		apply_pane_split();
	}

	void editor_panel_ragdoll_viewer_t::uninit()
	{
		editor_command_ragdoll_edit_t::cancel(*this);
		editor_command_system_t::get().clear_user_data(this);
		_edit_active = false;

		editor_asset_manager_t::get().remove_asset_deletion_listener(_asset_deletion_listener);

		_asset_deletion_listener = {};
		_parts_scrollbar.uninit();
		_part_reflection.uninit();

		for (u32 index = 0; index < _part_button_count; ++index)
			_part_buttons[index].uninit();

		_part_button_count = 0;

		for (editor_widget_button_t* button : {&_save_button, &_simulate_button, &_reset_button, &_generate_button, &_add_button, &_remove_button, &_mesh_button, &_skeleton_button, &_capsules_button, &_limits_button, &_status})
			button->uninit();

		_reflection.uninit();
		_right_scrollbar.uninit();
		_world_view.uninit();
		_split_border.uninit();
		_ui->deallocate_widget(_left_pane);
		_ui->deallocate_widget(_right_content);
		_ui->deallocate_widget(_right_pane);
		destroy_preview_world();

		_joint_globals.resize(0);
		_joint_dropdown_items.resize(0);
		_part_dropdown_items.resize(0);
		_part_dropdown_names.resize(0);
		_fold_states.resize(0);
		_ragdoll		  = {};
		_skeleton		  = {};
		_preview_skeleton = NULL_RESOURCE_HANDLE;

		editor_panel_t::uninit();
	}

	void editor_panel_ragdoll_viewer_t::set_ragdoll(sid_t ragdoll_guid, const char* asset_name)
	{
		if (_ragdoll_guid != ragdoll_guid)
		{
			editor_command_ragdoll_edit_t::cancel(*this);
			editor_command_system_t::get().clear_user_data(this);
			_edit_active = false;
		}

		reset_simulation();
		_part_reflection.set_reflection({});
		_ragdoll_guid = ragdoll_guid;
		set_sub_item_id(ragdoll_guid);
		_asset_name = asset_name;
		_ragdoll	= {};

		const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(ragdoll_guid);

		if (asset != nullptr && asset->asset_type == editor_asset_type_e::ragdoll && !asset->embedded_source.empty())
		{
			const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(*asset);

			if (!reflection_registry_t::get().type_from_json(type_id_t<ragdoll_def_t>::value, &_ragdoll, nullptr, embedded_source))
				_ragdoll = {};
		}

		reload_skeleton();
		rebuild_preview();
		refresh_scene();
		_dirty		   = false;
		_selected_part = _ragdoll.parts.empty() ? UINT32_MAX : 0;
		refresh_reflection();
		refresh_title(_asset_name.c_str(), "R: ");
	}

	void editor_panel_ragdoll_viewer_t::apply_ragdoll_def(ragdoll_def_t&& ragdoll)
	{
		reset_simulation();
		_part_reflection.set_reflection({});
		_ragdoll = std::move(ragdoll);

		// reload_skeleton();
		// rebuild_preview();
		reload_skeleton();
		rebuild_preview();
		refresh_scene();
		mark_dirty();
		_refresh_pending = true;
	}
	void editor_panel_ragdoll_viewer_t::create_preview_world()
	{
		editor_world_init_config_t init_config		  = editor_world_init_config_t::make_preview(editor_surface_controller_t::get().get_main_surface().swapchain_size);
		init_config.physics.physics_enabled			  = true;
		init_config.physics.max_bodies				  = 128;
		init_config.physics.body_reserve			  = 128;
		init_config.physics.max_body_pairs			  = 1024;
		init_config.physics.max_contact_constraints	  = 1024;
		init_config.physics.character_reserve		  = 0;
		init_config.physics.contact_event_reserve	  = 256;
		init_config.physics.temp_allocator_bytes	  = 2 * 1024 * 1024;
		init_config.physics.ragdoll_pose_budget_bytes = 128 * 1024;

		editor_world_controller_t& controller = editor_world_controller_t::get();
		_world								  = controller.create_world(init_config, editor_world_edit_type_e::view_with_debug, on_world_tick, this);
		editor_world_t& editor_world		  = *controller.get_editor_world(_world);
		world_t&		world				  = editor_world.get_world();

		editor_world.install_camera(editor_world_camera_type_e::orbit);
		editor_world.get_input_controller().set_pointer_drag_callbacks({
			.begin	   = on_preview_drag_begin,
			.update	   = on_preview_drag_update,
			.end	   = on_preview_drag_end,
			.user_data = this,
		});
		editor_world_util_t::install_default_scene_dark(world, 5.0f);
		_world_view.set_edit_world(_world);

		const entity_id_t	  floor	   = world.create_entity("ragdoll_collision_floor");
		component_physical_t& physical = world.get_component_table<component_physical_t>().add_or_get_as<component_physical_t>(floor);
		physical.half_extent		   = {25.0f, 0.1f, 25.0f};
		world.set_entity_pos_local(floor, {0.0f, -0.1f, 0.0f});
	}
	void editor_panel_ragdoll_viewer_t::refresh_scene()
	{
		world_t& world = editor_world_controller_t::get().get_editor_world(_world)->get_world();

		if (_display_entity != NULL_ENTITY_ID)
		{
			const component_skinned_mesh_renderer_t& renderer = world.get_component_table<component_skinned_mesh_renderer_t>().get_as_const<component_skinned_mesh_renderer_t>(_display_entity);

			if (renderer.skeleton != _ragdoll.target_skeleton || _skeleton.joints.empty())
			{
				world.destroy_entity(_display_entity);
				_display_entity = NULL_ENTITY_ID;
			}
		}

		if (_skeleton.joints.empty())
			return;

		if (_display_entity == NULL_ENTITY_ID)
			_display_entity = world.create_entity("ragdoll_preview");

		component_skinned_mesh_renderer_t& renderer = world.get_component_table<component_skinned_mesh_renderer_t>().add_or_get_as<component_skinned_mesh_renderer_t>(_display_entity);
		renderer.skeleton							= _ragdoll.target_skeleton;
		renderer.mesh								= _skeleton.preview_mesh;
		renderer.materials.resize(0);

		if (_show_mesh)
		{
			const editor_asset_t* mesh_asset = editor_asset_manager_t::get().find_asset(_skeleton.preview_mesh);
			mesh_def_t			  mesh		 = {};

			if (mesh_asset != nullptr && editor_asset_util_t::load_mesh_def(*mesh_asset, mesh) && !mesh.preview_materials.empty())
			{
				for (resource_handle_t material : mesh.preview_materials)
					renderer.materials.push_back(material);
			}
			else
			{
				for (u32 index = 0; index < decltype(renderer.materials)::capacity; ++index)
					renderer.materials.push_back(DEFAULT_OPAQUE_MATERIAL_ASSET_GUID);
			}
		}

		world.scan_for_resources(_display_entity, true);
	}

	bool editor_panel_ragdoll_viewer_t::start_simulation()
	{
		if (_simulating)
			return true;

		if (_display_entity == NULL_ENTITY_ID)
			return false;

		if (!_ragdoll.is_valid(static_cast<u32>(_skeleton.joints.size())))
		{
			_status.set_text("Invalid parts or limits; see log", editor_theme_t::get().color_accent_err);
			return false;
		}

		world_t& world = editor_world_controller_t::get().get_editor_world(_world)->get_world();

		if (_ragdoll.physical_material != NULL_RESOURCE_HANDLE)
		{
			world.add_resource(resource_type_e::physical_material, _ragdoll.physical_material);
			world.load_all_used_resources();

			if (resource_manager_t::get().find_runtime<physical_material_runtime_t>(_ragdoll.physical_material) == nullptr)
			{
				_status.set_text("Material could not be loaded", editor_theme_t::get().color_accent_err);
				return false;
			}
		}

		world.tick_animation_logic(0.0f);
		world.get_component_table<component_ragdoll_t>().add_or_get_as<component_ragdoll_t>(_display_entity);
		_simulating = world.get_physics().create_ragdoll(_display_entity, _ragdoll);

		if (!_simulating)
			world.get_component_table<component_ragdoll_t>().remove(_display_entity);

		_status.set_text(_simulating ? "Simulating unsaved preview" : "Simulation failed; see log");
		refresh_actions();

		return _simulating;
	}

	bool editor_panel_ragdoll_viewer_t::on_preview_drag_begin(const vec2f_t& relative_position, void* user_data)
	{
		editor_panel_ragdoll_viewer_t& panel = *static_cast<editor_panel_ragdoll_viewer_t*>(user_data);
		panel.finish_edit();

		if (!panel.start_simulation())
			return false;

		editor_world_t&	  editor_world = *editor_world_controller_t::get().get_editor_world(panel._world);
		physics_world_t&  physics	   = editor_world.get_world().get_physics();
		physics_raycast_t ray		   = {};
		physics_raycast_t center_ray   = {};
		physics_hit_t	  hit		   = {};

		if (!editor_world.get_camera_ray(relative_position, ray) || !editor_world.get_camera_ray({0.5f, 0.5f}, center_ray) || !physics.raycast_closest(ray, hit) || hit.entity != panel._display_entity)
			return false;

		if (!physics.begin_ragdoll_drag(hit.entity, hit.sub_shape_id, hit.position))
			return false;

		panel._drag_plane_point	 = hit.position;
		panel._drag_plane_normal = center_ray.direction;
		panel._selected_part	 = hit.sub_shape_id;
		panel._refresh_pending	 = true;

		return true;
	}

	void editor_panel_ragdoll_viewer_t::on_preview_drag_update(const vec2f_t& relative_position, void* user_data)
	{
		editor_panel_ragdoll_viewer_t& panel		= *static_cast<editor_panel_ragdoll_viewer_t*>(user_data);
		editor_world_t&				   editor_world = *editor_world_controller_t::get().get_editor_world(panel._world);
		physics_raycast_t			   ray			= {};

		if (!editor_world.get_camera_ray(relative_position, ray))
			return;

		const f32 denominator = vec3f_t::dot(ray.direction, panel._drag_plane_normal);

		if (math::abs(denominator) < MATH_EPS)
			return;

		const f32 distance = vec3f_t::dot(panel._drag_plane_point - ray.origin, panel._drag_plane_normal) / denominator;

		if (distance <= 0.0f)
			return;

		if (!editor_world.get_world().get_physics().update_ragdoll_drag(ray.origin + ray.direction * distance))
			editor_world.get_input_controller().cancel_pointer_drag();
	}

	void editor_panel_ragdoll_viewer_t::on_preview_drag_end(void* user_data)
	{
		editor_panel_ragdoll_viewer_t& panel = *static_cast<editor_panel_ragdoll_viewer_t*>(user_data);
		editor_world_controller_t::get().get_editor_world(panel._world)->get_world().get_physics().end_ragdoll_drag();
		panel._drag_plane_point	 = vec3f_t::zero;
		panel._drag_plane_normal = vec3f_t::zero;
	}

	void editor_panel_ragdoll_viewer_t::reset_simulation()
	{
		if (_display_entity == NULL_ENTITY_ID)
			return;

		editor_world_controller_t::get().get_editor_world(_world)->get_input_controller().cancel_pointer_drag();

		world_t&			   world	= editor_world_controller_t::get().get_editor_world(_world)->get_world();
		ecs_component_table_t& ragdolls = world.get_component_table<component_ragdoll_t>();

		if (world.get_component_table<component_system_ragdoll_t>().has(_display_entity))
			world.get_physics().destroy_ragdoll(_display_entity);

		if (ragdolls.has(_display_entity))
			ragdolls.remove(_display_entity);

		world.set_entity_pos_local(_display_entity, vec3f_t::zero);
		world.set_entity_rot_local(_display_entity, quat_t::identity);
		_simulating = false;
		_status.set_text("Reference Pose");
		refresh_actions();
	}

	void editor_panel_ragdoll_viewer_t::refresh_actions()
	{
		ui::layout_tree_t&	  tree		   = _ui->get_tree();
		const editor_theme_t& theme		   = editor_theme_t::get();
		const bool			  has_skeleton = !_skeleton.joints.empty();

		for (editor_widget_button_t* button : {&_save_button, &_simulate_button, &_reset_button, &_generate_button, &_add_button, &_remove_button})
			tree.in(button->get_root()).flags &= ~ui::wf_disabled;

		if (!_dirty || _ragdoll_guid == NULL_SID)
			tree.in(_save_button.get_root()).flags |= ui::wf_disabled;

		if (!has_skeleton || _ragdoll.parts.empty() || _simulating)
			tree.in(_simulate_button.get_root()).flags |= ui::wf_disabled;

		if (!_simulating)
			tree.in(_reset_button.get_root()).flags |= ui::wf_disabled;

		if (!has_skeleton)
			tree.in(_generate_button.get_root()).flags |= ui::wf_disabled;

		if (!has_skeleton || _ragdoll.parts.size() >= RAGDOLL_PART_MAX || _ragdoll.parts.size() >= _skeleton.joints.size())
			tree.in(_add_button.get_root()).flags |= ui::wf_disabled;

		if (_selected_part == UINT32_MAX)
			tree.in(_remove_button.get_root()).flags |= ui::wf_disabled;

		_mesh_button.set_text("Mesh", _show_mesh ? theme.color_accent1 : theme.color_text1);
		_skeleton_button.set_text("Skeleton", _show_skeleton ? theme.color_accent1 : theme.color_text1);
		_capsules_button.set_text("Capsules", _show_capsules ? theme.color_accent1 : theme.color_text1);
		_limits_button.set_text("Limits", _show_limits ? theme.color_accent1 : theme.color_text1);
		refresh_title(_asset_name.c_str(), "R: ", _dirty);
	}

	void editor_panel_ragdoll_viewer_t::mark_dirty()
	{
		_dirty = true;
		refresh_actions();
	}

	void editor_panel_ragdoll_viewer_t::finish_edit()
	{
		if (!_edit_active)
			return;

		editor_command_ragdoll_edit_t::submit(*this, "Ragdoll Edit", false);
		_edit_active	 = false;
		_refresh_pending = true;
	}

	void editor_panel_ragdoll_viewer_t::on_part_selected(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left)
			return;

		editor_panel_ragdoll_viewer_t& panel = *static_cast<editor_panel_ragdoll_viewer_t*>(user_data);
		router.set_focus(panel._root, false);
		panel.finish_edit();

		for (u32 index = 0; index < panel._part_button_count; ++index)
		{
			if (panel._part_buttons[index].get_root() == id)
				panel._selected_part = index;
		}

		panel._refresh_pending = true;
	}

	void editor_panel_ragdoll_viewer_t::on_action(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left)
			return;

		editor_panel_ragdoll_viewer_t& panel = *static_cast<editor_panel_ragdoll_viewer_t*>(user_data);
		router.set_focus(panel._root, false);
		panel.finish_edit();

		if (id == panel._save_button.get_root())
		{
			if (!panel._ragdoll.is_valid(static_cast<u32>(panel._skeleton.joints.size())))
			{
				panel._status.set_text("Invalid parts or limits; see log", editor_theme_t::get().color_accent_err);
				return;
			}

			nlohmann::json source = nlohmann::json::object();

			if (!reflection_registry_t::get().type_to_json(type_id_t<ragdoll_def_t>::value, &panel._ragdoll, nullptr, source))
				return;

			source["schema"] = "sfg.schema.ragdoll";

			if (editor_asset_manager_t::get().save_and_cook_embedded_asset_async(panel._ragdoll_guid, source))
			{
				panel._dirty = false;
				panel._status.set_text("Saved; cooking queued");
				panel.refresh_actions();
			}
			else
				panel._status.set_text("Save failed; see log", editor_theme_t::get().color_accent_err);

			return;
		}

		if (id == panel._reset_button.get_root())
		{
			panel.reset_simulation();
			return;
		}

		if (id == panel._simulate_button.get_root())
		{
			panel.start_simulation();
			return;
		}

		if (id == panel._mesh_button.get_root() || id == panel._skeleton_button.get_root() || id == panel._capsules_button.get_root() || id == panel._limits_button.get_root())
		{
			if (id == panel._mesh_button.get_root())
			{
				panel._show_mesh = !panel._show_mesh;
				panel.refresh_scene();
			}
			else if (id == panel._skeleton_button.get_root())
				panel._show_skeleton = !panel._show_skeleton;
			else if (id == panel._capsules_button.get_root())
				panel._show_capsules = !panel._show_capsules;
			else
				panel._show_limits = !panel._show_limits;

			panel.refresh_actions();
			return;
		}

		panel.reset_simulation();
		on_edit_begin(&panel);

		if (!panel._edit_active)
			return;

		panel._part_reflection.set_reflection({});

		if (id == panel._generate_button.get_root())
		{
			if (!editor_ragdoll_util_t::generate(panel._skeleton, panel._ragdoll))
				panel._status.set_text("Generation failed; see log", editor_theme_t::get().color_accent_err);

			panel._selected_part = panel._ragdoll.parts.empty() ? UINT32_MAX : 0;
		}
		else if (id == panel._add_button.get_root())
		{
			for (const u32 joint : panel._skeleton.evaluation_order)
			{
				const auto used = std::find_if(panel._ragdoll.parts.begin(), panel._ragdoll.parts.end(), [joint](const ragdoll_part_def_t& part) { return part.joint_index == joint; });

				if (used != panel._ragdoll.parts.end())
					continue;

				panel._ragdoll.parts.push_back({.joint_index = joint, .parent_part_index = panel._ragdoll.parts.empty() ? RAGDOLL_PART_NO_PARENT : 0});
				panel._selected_part = static_cast<u32>(panel._ragdoll.parts.size() - 1);
				break;
			}
		}
		else if (id == panel._remove_button.get_root())
		{
			const u32 removed = panel._selected_part;
			const u32 parent  = panel._ragdoll.parts[removed].parent_part_index;
			panel._ragdoll.parts.erase(panel._ragdoll.parts.begin() + removed);

			for (u32 index = 0; index < panel._ragdoll.parts.size(); ++index)
			{
				ragdoll_part_def_t& part = panel._ragdoll.parts[index];

				if (index == 0)
					part.parent_part_index = RAGDOLL_PART_NO_PARENT;
				else if (part.parent_part_index == removed)
					part.parent_part_index = removed == 0 ? 0 : parent;
				else if (part.parent_part_index > removed)
					--part.parent_part_index;
			}

			panel._selected_part = panel._ragdoll.parts.empty() ? UINT32_MAX : math::min(removed, static_cast<u32>(panel._ragdoll.parts.size() - 1));
		}

		panel.mark_dirty();
		panel.finish_edit();
	}

	void editor_panel_ragdoll_viewer_t::destroy_preview_world()
	{
		if (_world.is_null())
			return;

		editor_world_controller_t::get().destroy_world(_world);
		_world = {};
	}

	void editor_panel_ragdoll_viewer_t::reload_skeleton()
	{
		_preview_skeleton = _ragdoll.target_skeleton;
		_skeleton		  = {};

		if (_ragdoll.target_skeleton == NULL_RESOURCE_HANDLE)
			return;

		const editor_asset_t* skeleton_asset = editor_asset_manager_t::get().find_asset(_ragdoll.target_skeleton);

		if (skeleton_asset == nullptr || skeleton_asset->asset_type != editor_asset_type_e::skeleton || skeleton_asset->embedded_source.empty())
			return;

		const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(*skeleton_asset);

		if (!reflection_registry_t::get().type_from_json(type_id_t<skeleton_def_t>::value, &_skeleton, nullptr, embedded_source))
			_skeleton = {};
	}

	void editor_panel_ragdoll_viewer_t::rebuild_preview()
	{
		_joint_globals.resize(0);

		if (!_skeleton.is_evaluation_order_valid())
			return;

		_joint_globals.resize(_skeleton.joints.size(), mat4x3_t::identity);

		for (const u32 joint_index : _skeleton.evaluation_order)
		{
			const skeleton_joint_def_t& joint = _skeleton.joints[joint_index];
			_joint_globals[joint_index]		  = joint.parent_index == SKELETON_JOINT_NO_PARENT ? joint.local : _joint_globals[joint.parent_index] * joint.local;
		}

		for (mat4x3_t& joint_global : _joint_globals)
			joint_global = _skeleton.skinning_transform * joint_global;

		vec3f_t bounds_min = _joint_globals[0].get_translation();
		vec3f_t bounds_max = bounds_min;

		for (const mat4x3_t& joint_global : _joint_globals)
		{
			const vec3f_t position = joint_global.get_translation();
			bounds_min			   = vec3f_t::min(bounds_min, position);
			bounds_max			   = vec3f_t::max(bounds_max, position);
		}

		for (const ragdoll_part_def_t& part : _ragdoll.parts)
		{
			if (part.joint_index >= _joint_globals.size())
				continue;

			vec3f_t capsule_position = vec3f_t::zero;
			quat_t	capsule_rotation = quat_t::identity;
			vec3f_t capsule_scale	 = vec3f_t::one;
			_joint_globals[part.joint_index].decompose(capsule_position, capsule_rotation, capsule_scale);

			capsule_position += capsule_rotation * (part.local_position * capsule_scale);

			const f32	  shape_scale = math::max(math::max(math::abs(capsule_scale.x), math::abs(capsule_scale.y)), math::abs(capsule_scale.z));
			const f32	  extent	  = (part.radius + part.half_height) * shape_scale;
			const vec3f_t margin(extent, extent, extent);
			bounds_min = vec3f_t::min(bounds_min, capsule_position - margin);
			bounds_max = vec3f_t::max(bounds_max, capsule_position + margin);
		}

		if (!_world.is_null())
			editor_world_controller_t::get().get_editor_world(_world)->fit_camera_to_bounds(aabb_t(bounds_min, bounds_max));
	}

	void editor_panel_ragdoll_viewer_t::refresh_reflection()
	{
		_joint_dropdown_items.resize(0);
		_joint_dropdown_items.reserve(_skeleton.joints.size());

		for (u32 joint = 0; joint < _skeleton.joints.size(); ++joint)
			_joint_dropdown_items.push_back({.text = _skeleton.joints[joint].name.c_str(), .value = joint});

		if (_selected_part >= _ragdoll.parts.size())
			_selected_part = _ragdoll.parts.empty() ? UINT32_MAX : 0;

		for (u32 index = 0; index < _part_button_count; ++index)
			_part_buttons[index].uninit();

		_part_button_count							= static_cast<u32>(_ragdoll.parts.size());
		const editor_theme_t& theme					= editor_theme_t::get();
		_ui->get_tree().in(_part_list).size_value.y = static_cast<f32>(math::max(1u, math::min(_part_button_count, 7u))) * (theme.item_height + theme.item_spacing);
		_part_dropdown_names.resize(0);
		_part_dropdown_names.reserve(_part_button_count);

		for (u32 index = 0; index < _part_button_count; ++index)
		{
			const ragdoll_part_def_t& part = _ragdoll.parts[index];
			_part_dropdown_names.push_back(part.joint_index < _skeleton.joints.size() ? _skeleton.joints[part.joint_index].name : "Unassigned Joint");
			_part_buttons[index].init(*_ui, _part_list, {.text = _part_dropdown_names.back().c_str(), .elevate_draw_order = true});

			const ui::widget_id_t button = _part_buttons[index].get_root();
			ui::layout_in_t&	  layout = _ui->get_tree().in(button);
			layout.size_mode_x			 = ui::axis_mode_e::parent_relative;
			layout.size_value.x			 = 1.0f;

			const vec4f_t color = index == _selected_part ? theme.color_accent1 : theme.color_panel;
			_ui->get_paint().set_rect(button, {.fill_color_a = color, .fill_color_b = color, .rounding = theme.item_rounding, .rounding_segs = 4});
			_ui->get_input().set_listener(button, {.on_click = on_part_selected, .user_data = this});
		}

		void*							object	   = &_ragdoll;
		const sid_t						excluded[] = {"parts"_hs};
		const editor_widget_callbacks_t callbacks{.edit_begin = on_edit_begin, .edited = on_edited, .edit_submitted = on_edit_submitted, .user_data = this};
		_reflection.save_fold_states();
		_reflection.set_reflection({
			.fold_states	 = &_fold_states,
			.callbacks		 = callbacks,
			.excluded_fields = {.data = excluded, .size = std::size(excluded)},
			.objects		 = {.data = &object, .size = 1},
			.type_id		 = type_id_t<ragdoll_def_t>::value,
			.block_edits	 = _ragdoll_guid == NULL_SID,
		});

		void* part = _selected_part == UINT32_MAX ? nullptr : &_ragdoll.parts[_selected_part];
		_part_reflection.set_reflection({
			.callbacks				  = callbacks,
			.objects				  = {.data = &part, .size = part == nullptr ? 0u : 1u},
			.type_id				  = type_id_t<ragdoll_part_def_t>::value,
			.dropdown_items			  = resolve_dropdown_items,
			.dropdown_items_user_data = this,
			.block_edits			  = _ragdoll_guid == NULL_SID,
		});
		refresh_actions();
	}

	void editor_panel_ragdoll_viewer_t::apply_pane_split()
	{
		if (_ui != nullptr)
			_ui->get_tree().in(_left_pane).size_value.x = _pane_split;
	}

	void editor_panel_ragdoll_viewer_t::draw_ragdoll(world_t& world) const
	{
		if (_joint_globals.empty())
			return;

		const editor_theme_t&  theme = editor_theme_t::get();
		const color_t		   selected{theme.color_accent1.x, theme.color_accent1.y, theme.color_accent1.z, 1.0f};
		const color_t		   neutral{theme.color_text1.x, theme.color_text1.y, theme.color_text1.z, 1.0f};
		world_debug_draw_t&	   draw = world.get_debug_draw();
		span_t<const mat4x3_t> pose{.data = _joint_globals.data(), .size = _joint_globals.size()};
		mat4x3_t			   entity_transform = mat4x3_t::identity;

		if (_simulating && world.get_component_table<component_system_ragdoll_t>().has(_display_entity))
		{
			pose			 = world.get_physics().get_ragdoll_joint_globals(_display_entity);
			entity_transform = world.get_component_table<component_system_transform_t>().get_as_const<component_system_transform_t>(_display_entity).abs_mat;
		}

		if (_show_skeleton)
		{
			for (u32 joint_index = 0; joint_index < _skeleton.joints.size(); ++joint_index)
			{
				const skeleton_joint_def_t& joint	 = _skeleton.joints[joint_index];
				const vec3f_t				position = entity_transform * pose[joint_index].get_translation();

				if (joint.parent_index != SKELETON_JOINT_NO_PARENT)
					draw.draw_line(entity_transform * pose[joint.parent_index].get_translation(), position, neutral, 1.0f, debug_draw_depth_e::always_visible);
			}
		}

		for (u32 index = 0; index < _ragdoll.parts.size(); ++index)
		{
			const ragdoll_part_def_t& part = _ragdoll.parts[index];

			if (part.joint_index >= pose.size)
				continue;

			const mat4x3_t joint_transform = entity_transform * pose[part.joint_index];
			vec3f_t		   position		   = vec3f_t::zero;
			quat_t		   rotation		   = quat_t::identity;
			vec3f_t		   scale		   = vec3f_t::one;
			joint_transform.decompose(position, rotation, scale);

			position += rotation * (part.local_position * scale);
			rotation = rotation * part.local_rotation;

			const f32  shape_scale = math::max(math::abs(scale.x), math::max(math::abs(scale.y), math::abs(scale.z)));
			const bool active	   = index == _selected_part;

			if (_show_capsules)
				draw.draw_capsule(position, math::max(part.radius * shape_scale, 0.001f), math::max(part.half_height * shape_scale, 0.001f), rotation.get_up(), active ? selected : neutral, active ? 2.0f : 1.0f, debug_draw_depth_e::always_visible, 20);

			if (!active || !_show_limits || part.parent_part_index == RAGDOLL_PART_NO_PARENT || part.twist_axis.is_zero() || part.plane_axis.is_zero())
				continue;

			vec3f_t joint_position = vec3f_t::zero;
			quat_t	joint_rotation = quat_t::identity;
			vec3f_t joint_scale	   = vec3f_t::one;
			joint_transform.decompose(joint_position, joint_rotation, joint_scale);

			if (_simulating && part.parent_part_index < _ragdoll.parts.size())
			{
				const u32	   parent_joint		  = _ragdoll.parts[part.parent_part_index].joint_index;
				vec3f_t		   reference_position = vec3f_t::zero;
				quat_t		   reference_rotation = quat_t::identity;
				vec3f_t		   reference_scale	  = vec3f_t::one;
				const mat4x3_t frame			  = entity_transform * pose[parent_joint] * _joint_globals[parent_joint].inverse() * _joint_globals[part.joint_index];
				frame.decompose(reference_position, reference_rotation, reference_scale);
				joint_rotation = reference_rotation;
			}

			const vec3f_t twist	 = joint_rotation * part.twist_axis.normalized();
			const vec3f_t plane	 = joint_rotation * part.plane_axis.normalized();
			const vec3f_t normal = vec3f_t::cross(twist, plane);

			if (normal.is_zero())
				continue;

			const f32 length = math::max(part.radius + part.half_height, 0.01f) * shape_scale;
			draw.draw_line(joint_position, joint_position + twist * length, selected, 2.0f, debug_draw_depth_e::always_visible);
			draw.draw_line(joint_position, joint_position + plane * length, color_t::green, 1.0f, debug_draw_depth_e::always_visible);
			draw.draw_arc(joint_position,
						  normal.normalized(),
						  quat_t::angle_axis(-part.plane_half_cone_angle_degrees, normal.normalized()) * twist,
						  length,
						  math::degrees_to_radians(2.0f * part.plane_half_cone_angle_degrees),
						  selected,
						  1.0f,
						  debug_draw_depth_e::always_visible);
			draw.draw_arc(joint_position, plane, quat_t::angle_axis(-part.normal_half_cone_angle_degrees, plane) * twist, length, math::degrees_to_radians(2.0f * part.normal_half_cone_angle_degrees), selected, 1.0f, debug_draw_depth_e::always_visible);
			draw.draw_arc(joint_position,
						  twist,
						  quat_t::angle_axis(part.twist_min_angle_degrees, twist) * plane,
						  length * 0.65f,
						  math::degrees_to_radians(part.twist_max_angle_degrees - part.twist_min_angle_degrees),
						  color_t::green,
						  1.0f,
						  debug_draw_depth_e::always_visible);
		}
	}

	span_t<const editor_widget_reflection_dropdown_item_t> editor_panel_ragdoll_viewer_t::resolve_dropdown_items(sid_t field_id, sid_t owner_field_id, u32 element_index, void* user_data)
	{
		editor_panel_ragdoll_viewer_t& viewer = *static_cast<editor_panel_ragdoll_viewer_t*>(user_data);

		if (field_id == "joint_index"_hs)
			return {.data = viewer._joint_dropdown_items.data(), .size = viewer._joint_dropdown_items.size()};

		if (field_id == "parent_part_index"_hs)
		{
			viewer._part_dropdown_items.resize(0);

			if (viewer._selected_part == 0)
				viewer._part_dropdown_items.push_back({.text = "None", .value = RAGDOLL_PART_NO_PARENT});
			else
			{
				for (u32 index = 0; index < viewer._selected_part; ++index)
					viewer._part_dropdown_items.push_back({.text = viewer._part_dropdown_names[index].c_str(), .value = index});
			}

			return {.data = viewer._part_dropdown_items.data(), .size = viewer._part_dropdown_items.size()};
		}

		return {};
	}

	void editor_panel_ragdoll_viewer_t::on_asset_deletion(editor_asset_manager_t& asset_manager, span_t<const sid_t> asset_ids, void* user_data)
	{
		editor_panel_ragdoll_viewer_t& panel = *static_cast<editor_panel_ragdoll_viewer_t*>(user_data);

		for (size_t i = 0; i < asset_ids.size; ++i)
		{
			if (asset_ids[i] == panel._ragdoll_guid)
			{
				editor_surface_controller_t::get().request_close_panel(&panel);
				return;
			}

			if (asset_ids[i] == panel._ragdoll.target_skeleton)
			{
				panel.reset_simulation();
				panel._skeleton = {};
				panel.refresh_scene();
				panel.rebuild_preview();
				panel.refresh_reflection();
				return;
			}
		}
	}

	void editor_panel_ragdoll_viewer_t::on_edited(void* user_data)
	{
		editor_panel_ragdoll_viewer_t& panel = *static_cast<editor_panel_ragdoll_viewer_t*>(user_data);

		if (panel._preview_skeleton != panel._ragdoll.target_skeleton)
		{
			panel._part_reflection.set_reflection({});
			panel.reload_skeleton();
			panel._ragdoll.parts.resize(0);

			if (!panel._skeleton.joints.empty())
				editor_ragdoll_util_t::generate(panel._skeleton, panel._ragdoll);

			panel.rebuild_preview();
			panel.refresh_scene();
		}

		panel.mark_dirty();
	}
	void editor_panel_ragdoll_viewer_t::on_edit_begin(void* user_data)
	{
		editor_panel_ragdoll_viewer_t& panel = *static_cast<editor_panel_ragdoll_viewer_t*>(user_data);

		if (panel._edit_active)
			return;

		panel.reset_simulation();
		panel._edit_active = editor_command_ragdoll_edit_t::begin(panel);
	}

	void editor_panel_ragdoll_viewer_t::on_edit_submitted(void* user_data)
	{
		editor_panel_ragdoll_viewer_t& panel = *static_cast<editor_panel_ragdoll_viewer_t*>(user_data);
		panel.finish_edit();
	}
	void editor_panel_ragdoll_viewer_t::on_world_tick(world_t& world, f32 delta_time, void* user_data)
	{
		editor_panel_ragdoll_viewer_t& panel = *static_cast<editor_panel_ragdoll_viewer_t*>(user_data);

		if (panel._refresh_pending && !panel._edit_active)
		{
			panel._refresh_pending = false;
			panel.refresh_reflection();
		}

		if (panel._simulating && !world.get_component_table<component_system_ragdoll_t>().has(panel._display_entity))
			panel.reset_simulation();

		panel.draw_ragdoll(world);
	}
	void editor_panel_ragdoll_viewer_t::on_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		editor_panel_ragdoll_viewer_t& panel = *static_cast<editor_panel_ragdoll_viewer_t*>(user_data);
		const ui::layout_out_t&		   out	 = panel._ui->get_tree().out(panel._root);

		panel._pane_split = math::clamp((pos.x - out.pos.x) / out.size.x, RAGDOLL_VIEWER_PANE_SPLIT_MIN, RAGDOLL_VIEWER_PANE_SPLIT_MAX);
		panel.apply_pane_split();
	}
}
