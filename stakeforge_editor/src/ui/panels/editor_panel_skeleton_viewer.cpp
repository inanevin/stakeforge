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

#include "ui/panels/editor_panel_skeleton_viewer.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_manager.hpp"
#include "editor_surface_controller.hpp"
#include "editor_world_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include "world/editor_world.hpp"

#include <sfg/math/aabb.hpp>
#include <sfg/math/color.hpp>
#include <sfg/math/math.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/resources/skeleton.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/world_debug_draw.hpp>
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define SKELETON_VIEWER_PANE_SPLIT_MIN				0.45f
#define SKELETON_VIEWER_PANE_SPLIT_MAX				0.85f
#define SKELETON_VIEWER_SPLIT_BORDER_THICKNESS_MULT 2.0f
#define SKELETON_VIEWER_JOINT_RADIUS_RATIO			0.02f
#define SKELETON_VIEWER_AXIS_LENGTH_RATIO			0.12f

	editor_panel_skeleton_viewer_t::editor_panel_skeleton_viewer_t()
	{
		set_type(editor_panel_type_e::skeleton_viewer);
		refresh_title();
		set_icon(ICON_ANIMATION);
	}

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
		_asset_deletion_listener = editor_asset_manager_t::get().add_asset_deletion_listener(on_asset_deletion, this);

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = tree.in(_root);
		root_in.flow			 = ui::flow_e::row;
		root_in.child_spacing	 = 0.0f;
		root_in.child_margins	 = {0.0f, 0.0f, theme.margin_vertical, 0.0f};

		_left_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_left_pane, "skeleton_viewer_left_pane");
		tree.attach(_root, _left_pane);

		ui::layout_in_t& left_in = tree.in(_left_pane);
		left_in.flow			 = ui::flow_e::none;
		left_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};
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
		right_in.flow			  = ui::flow_e::column;
		right_in.child_margins	  = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};
		right_in.size_mode_x	  = ui::axis_mode_e::fill;
		right_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		right_in.size_value		  = {1.0f, 1.0f};

		editor_misc_widgets_t::make_section_label(ui, _right_pane, "Skeleton");

		_joint_count_value = append_property_value_row("Joints");
		editor_dividers_t::add_divider_hor(ui, _right_pane, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
		_root_joint_value = append_property_value_row("Root Joint");

		create_preview_world();

		if (_skeleton_guid != 0)
			set_skeleton(_skeleton_guid, _asset_name.c_str());
		else
			refresh_info();

		apply_pane_split();
	}

	void editor_panel_skeleton_viewer_t::uninit()
	{
		editor_asset_manager_t::get().remove_asset_deletion_listener(_asset_deletion_listener);
		_asset_deletion_listener = {};

		_world_view.uninit();
		_split_border.uninit();
		_ui->deallocate_widget(_left_pane);
		_ui->deallocate_widget(_right_pane);
		destroy_preview_world();
		_joint_draw_data.resize(0);
		editor_panel_t::uninit();
	}

	void editor_panel_skeleton_viewer_t::set_skeleton(sid_t skeleton_guid, const char* asset_name)
	{
		_skeleton_guid = skeleton_guid;
		set_sub_item_id(skeleton_guid);
		_asset_name = asset_name;
		_joint_draw_data.resize(0);
		_root_joint_index = UINT32_MAX;

		if (!_world.is_null() && skeleton_guid != 0)
		{
			world_t& world = editor_world_controller_t::get().get_editor_world(_world)->get_world();
			world.add_resource(resource_type_e::skeleton, skeleton_guid);
			world.load_all_used_resources();

			resource_manager_t&		  resource_manager = resource_manager_t::get();
			const skeleton_runtime_t* skeleton		   = resource_manager.find_runtime<skeleton_runtime_t>(skeleton_guid);

			if (skeleton != nullptr)
			{
				const skeleton_joint_runtime_t* joints			 = resource_manager.get_memory().get<skeleton_joint_runtime_t>(skeleton->joints);
				const u32*						evaluation_order = resource_manager.get_memory().get<u32>(skeleton->evaluation_order);
				_joint_draw_data.resize(skeleton->joint_count);
				_root_joint_index = skeleton->root_joint_index;

				for (u32 i = 0; i < skeleton->joint_count; ++i)
				{
					const u32						joint_index = evaluation_order[i];
					const skeleton_joint_runtime_t& joint		= joints[joint_index];
					joint_draw_data_t&				draw_data	= _joint_draw_data[joint_index];
					draw_data.parent_index						= joint.parent_index;
					draw_data.transform							= joint.parent_index == SKELETON_JOINT_NO_PARENT ? joint.local : _joint_draw_data[joint.parent_index].transform * joint.local;
				}

				vec3f_t bounds_min = _joint_draw_data[0].transform.get_translation();
				vec3f_t bounds_max = bounds_min;

				for (const joint_draw_data_t& draw_data : _joint_draw_data)
				{
					const vec3f_t position = draw_data.transform.get_translation();
					bounds_min			   = vec3f_t::min(bounds_min, position);
					bounds_max			   = vec3f_t::max(bounds_max, position);
				}

				const vec3f_t dimensions   = bounds_max - bounds_min;
				const f32	  visual_scale = math::max(math::max(dimensions.x, dimensions.y), math::max(dimensions.z, 1.0f));
				_joint_radius			   = visual_scale * SKELETON_VIEWER_JOINT_RADIUS_RATIO;
				_axis_length			   = visual_scale * SKELETON_VIEWER_AXIS_LENGTH_RATIO;

				const vec3f_t bounds_margin(_axis_length, _axis_length, _axis_length);
				editor_world_controller_t::get().get_editor_world(_world)->fit_camera_to_bounds(aabb_t(bounds_min - bounds_margin, bounds_max + bounds_margin));
			}
		}

		refresh_info();
		refresh_title(_asset_name.c_str(), "S: ");
	}

	void editor_panel_skeleton_viewer_t::create_preview_world()
	{
		const world_init_config_t init_config{
			.render_resolution				= editor_surface_controller_t::get().get_main_surface().swapchain_size,
			.render_entity_max				= 10,
			.render_bone_max				= 128,
			.render_bone_reserve			= 128,
			.animation_graph_memory_reserve = 64 * 1024,
			.component_table_reserve		= 64,
			.free_list_reserve				= 128,
			.used_resource_reserve			= 64,
			.text_allocation_reserve		= 256,
			.physics_enabled				= false,
		};

		editor_world_controller_t& controller = editor_world_controller_t::get();
		_world								  = controller.create_world(init_config, editor_world_edit_type_e::view_with_debug, on_world_tick, this);
		editor_world_t* const editor_world	  = controller.get_editor_world(_world);

		editor_world->install_camera(editor_world_camera_type_e::orbit);
		create_environment();
		_world_view.set_edit_world(_world);
	}

	void editor_panel_skeleton_viewer_t::destroy_preview_world()
	{
		if (_world.is_null())
			return;

		editor_world_controller_t::get().destroy_world(_world);
		_world				= {};
		_environment_entity = NULL_ENTITY_ID;
	}

	void editor_panel_skeleton_viewer_t::create_environment()
	{
		world_t& world			   = editor_world_controller_t::get().get_editor_world(_world)->get_world();
		_environment_entity		   = world.create_entity("skeleton_viewer_environment");
		component_skybox_t& skybox = ecs_helpers_t::table_add_or_get_as<component_skybox_t>(world.get_component_table(type_id_t<component_skybox_t>::value), _environment_entity);
		skybox.skybox_asset		   = DEFAULT_QWANTANI_DUSK_SKYBOX_ASSET_GUID;
		skybox.exposure			   = 0.25f;
		world.scan_for_resources(_environment_entity, true);
	}

	void editor_panel_skeleton_viewer_t::draw_skeleton(world_t& world) const
	{
		resource_manager_t&		  resource_manager = resource_manager_t::get();
		const skeleton_runtime_t* skeleton		   = resource_manager.find_runtime<skeleton_runtime_t>(_skeleton_guid);

		if (skeleton == nullptr || _joint_draw_data.empty())
			return;

		chunk_allocator_t&				resource_memory = resource_manager.get_memory();
		const skeleton_joint_runtime_t* joints			= resource_memory.get<skeleton_joint_runtime_t>(skeleton->joints);
		const editor_theme_t&			theme			= editor_theme_t::get();
		world_debug_draw_t&				debug_draw		= world.get_debug_draw();

		for (u32 joint_index = 0; joint_index < skeleton->joint_count; ++joint_index)
		{
			const joint_draw_data_t& draw_data	= _joint_draw_data[joint_index];
			const vec3f_t			 position	= draw_data.transform.get_translation();
			const char*				 joint_name = resource_memory.get_text(joints[joint_index].name);

			if (draw_data.parent_index != SKELETON_JOINT_NO_PARENT)
			{
				const vec3f_t parent_position = _joint_draw_data[draw_data.parent_index].transform.get_translation();
				debug_draw.draw_line(parent_position, position, color_t::white, 2.0f, debug_draw_depth_e::depth_tested);
			}

			debug_draw.draw_sphere(position, _joint_radius, color_t::purple, 1.5f, debug_draw_depth_e::depth_tested, 10);

			const vec3f_t axis_x = draw_data.transform.get_column(0).normalized();
			const vec3f_t axis_y = draw_data.transform.get_column(1).normalized();
			const vec3f_t axis_z = draw_data.transform.get_column(2).normalized();

			debug_draw.draw_line(position, position + axis_x * _axis_length, color_t::red, 1.5f, debug_draw_depth_e::depth_tested);
			debug_draw.draw_line(position, position + axis_y * _axis_length, color_t::green, 1.5f, debug_draw_depth_e::depth_tested);
			debug_draw.draw_line(position, position + axis_z * _axis_length, color_t::blue, 1.5f, debug_draw_depth_e::depth_tested);
			debug_draw.draw_text_3d(position, joint_name, color_t::white, theme.text_small_px_size, debug_draw_depth_e::always_visible, debug_draw_text_alignment_e::bottom_center, {0.0f, -4.0f});
		}
	}

	void editor_panel_skeleton_viewer_t::refresh_info()
	{
		if (_ui == nullptr)
			return;

		_joint_count_text = "-";
		_root_joint_text  = "-";

		if (_skeleton_guid != 0)
		{
			if (_joint_draw_data.empty())
			{
				_joint_count_text = "Failed";
			}
			else
			{
				_joint_count_text = std::to_string(_joint_draw_data.size());
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

	void editor_panel_skeleton_viewer_t::apply_pane_split()
	{
		if (_ui != nullptr)
			_ui->get_tree().in(_left_pane).size_value.x = _pane_split;
	}

	void editor_panel_skeleton_viewer_t::on_asset_deletion(editor_asset_manager_t&, span_t<const sid_t> asset_ids, void* user_data)
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

		editor_world_controller_t::get().get_editor_world(panel._world)->get_world().unload_all_used_resources();

		panel._skeleton_guid = NULL_SID;
		panel._joint_draw_data.resize(0);
		panel._root_joint_index = UINT32_MAX;
		panel._asset_name.resize(0);
		panel.set_sub_item_id(NULL_SID);

		editor_surface_controller_t::get().request_close_panel(&panel);
	}

	ui::widget_id_t editor_panel_skeleton_viewer_t::append_property_value_row(const char* label)
	{
		const editor_property_row_t row = editor_misc_widgets_t::make_property_row_with_label(*_ui, _right_pane, label);
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

	void editor_panel_skeleton_viewer_t::on_world_tick(world_t& world, f32 delta_time, void* user_data)
	{
		const editor_panel_skeleton_viewer_t& panel = *static_cast<const editor_panel_skeleton_viewer_t*>(user_data);
		panel.draw_skeleton(world);
	}

	void editor_panel_skeleton_viewer_t::on_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		editor_panel_skeleton_viewer_t& panel = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);
		const ui::layout_out_t&			out	  = panel._ui->get_tree().out(panel._root);

		panel._pane_split = math::clamp((pos.x - out.pos.x) / out.size.x, SKELETON_VIEWER_PANE_SPLIT_MIN, SKELETON_VIEWER_PANE_SPLIT_MAX);
		panel.apply_pane_split();
	}
}
