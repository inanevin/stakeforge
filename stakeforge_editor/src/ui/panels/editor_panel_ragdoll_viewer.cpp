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

#include "ui/panels/editor_panel_ragdoll_viewer.hpp"

#include "assets/editor_asset.hpp"
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

		ui::layout_in_t& right_content_in = tree.in(_right_content);
		right_content_in.flow			  = ui::flow_e::column;
		right_content_in.child_margins	  = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};
		right_content_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		right_content_in.size_mode_y	  = ui::axis_mode_e::sum_children;
		right_content_in.size_value		  = {1.0f, 1.0f};

		_right_scrollbar.init(ui, {.target = _right_pane, .axes = editor_scrollbar_axis_y});

		editor_misc_widgets_t::make_section_label(ui, _right_content, "Capsule Ragdoll");

		void* ragdoll_object = &_ragdoll;
		_reflection.init(ui,
						 _right_content,
						 {
							 .fold_states = &_fold_states,
							 .callbacks =
								 {
									 .edit_begin	 = on_edit_begin,
									 .edited		 = on_edited,
									 .edit_submitted = on_edit_submitted,
									 .user_data		 = this,
								 },
							 .objects				   = {.data = &ragdoll_object, .size = 1},
							 .type_id				   = type_id_t<ragdoll_def_t>::value,
							 .dropdown_items		   = resolve_dropdown_items,
							 .dropdown_items_user_data = this,
							 .block_edits			   = _ragdoll_guid == NULL_SID,
							 .elevate_draw_order	   = true,
						 });

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
		_part_count		  = 0;

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
		refresh_reflection();
		_part_count = static_cast<u32>(_ragdoll.parts.size());
		refresh_title(_asset_name.c_str(), "R: ");
	}

	void editor_panel_ragdoll_viewer_t::apply_ragdoll_def(ragdoll_def_t&& ragdoll)
	{
		_ragdoll = std::move(ragdoll);

		// reload_skeleton();
		// rebuild_preview();
		refresh_reflection();
		_part_count = static_cast<u32>(_ragdoll.parts.size());
	}

	void editor_panel_ragdoll_viewer_t::create_preview_world()
	{
		const editor_world_init_config_t init_config = editor_world_init_config_t::make_preview(editor_surface_controller_t::get().get_main_surface().swapchain_size);

		editor_world_controller_t& controller = editor_world_controller_t::get();
		_world								  = controller.create_world(init_config, editor_world_edit_type_e::view_with_debug, on_world_tick, this);
		editor_world_t* const editor_world	  = controller.get_editor_world(_world);

		editor_world->install_camera(editor_world_camera_type_e::orbit);
		editor_world_util_t::install_default_scene(editor_world->get_world());
		_world_view.set_edit_world(_world);
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

			vec3f_t	  capsule_position = vec3f_t::zero;
			quat_t	  capsule_rotation = quat_t::identity;
			vec3f_t	  capsule_scale = vec3f_t::one;
			(_joint_globals[part.joint_index] * mat4x3_t::transform(part.local_position, part.local_rotation, vec3f_t::one)).decompose(capsule_position, capsule_rotation, capsule_scale);
			const f32	  shape_scale = math::max(math::max(math::abs(capsule_scale.x), math::abs(capsule_scale.y)), math::abs(capsule_scale.z));
			const f32	  extent   = (part.radius + part.half_height) * shape_scale;
			const vec3f_t margin(extent, extent, extent);
			bounds_min = vec3f_t::min(bounds_min, capsule_position - margin);
			bounds_max = vec3f_t::max(bounds_max, capsule_position + margin);
		}

		if (!_world.is_null())
			editor_world_controller_t::get().get_editor_world(_world)->fit_camera_to_bounds(aabb_t(bounds_min, bounds_max));
	}

	void editor_panel_ragdoll_viewer_t::refresh_part_dropdown_names()
	{
		_part_dropdown_names.reserve(RAGDOLL_PART_MAX);

		for (u32 part_index = static_cast<u32>(_part_dropdown_names.size()); part_index < _ragdoll.parts.size(); ++part_index)
			_part_dropdown_names.push_back("Part " + std::to_string(part_index));
	}

	void editor_panel_ragdoll_viewer_t::refresh_reflection()
	{
		if (_ui == nullptr)
			return;

		_joint_dropdown_items.resize(0);
		_joint_dropdown_items.reserve(_skeleton.joints.size() + 1);
		_joint_dropdown_items.push_back({.text = "None", .value = UINT32_MAX});

		for (u32 joint_index = 0; joint_index < _skeleton.joints.size(); ++joint_index)
		{
			const skeleton_joint_def_t& joint = _skeleton.joints[joint_index];
			_joint_dropdown_items.push_back({
				.text  = joint.name.empty() ? "Unnamed Joint" : joint.name.c_str(),
				.value = joint_index,
			});
		}

		refresh_part_dropdown_names();

		void* ragdoll_object = &_ragdoll;
		_reflection.save_fold_states();
		_reflection.set_reflection({
			.fold_states = &_fold_states,
			.callbacks =
				{
					.edit_begin		= on_edit_begin,
					.edited			= on_edited,
					.edit_submitted = on_edit_submitted,
					.user_data		= this,
				},
			.objects				  = {.data = &ragdoll_object, .size = 1},
			.type_id				  = type_id_t<ragdoll_def_t>::value,
			.dropdown_items			  = resolve_dropdown_items,
			.dropdown_items_user_data = this,
			.block_edits			  = _ragdoll_guid == NULL_SID,
			.elevate_draw_order		  = true,
		});
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

		const editor_theme_t& theme = editor_theme_t::get();
		const color_t		  joint_color{
			theme.color_accent1.x,
			theme.color_accent1.y,
			theme.color_accent1.z,
			theme.color_accent1.w,
		};
		const color_t joint_name_color{
			theme.color_accent0.x,
			theme.color_accent0.y,
			theme.color_accent0.z,
			theme.color_accent0.w,
		};
		world_debug_draw_t& debug_draw = world.get_debug_draw();

		for (u32 joint_index = 0; joint_index < _skeleton.joints.size(); ++joint_index)
		{
			const skeleton_joint_def_t& joint	 = _skeleton.joints[joint_index];
			const vec3f_t				position = _joint_globals[joint_index].get_translation();

			if (joint.parent_index != SKELETON_JOINT_NO_PARENT)
				debug_draw.draw_line(_joint_globals[joint.parent_index].get_translation(), position, joint_color, 1.0f, debug_draw_depth_e::depth_tested);

			debug_draw.draw_text_3d(position, joint.name.c_str(), joint_name_color, theme.text_small_px_size, debug_draw_depth_e::always_visible, debug_draw_text_alignment_e::bottom_center, {0.0f, -4.0f});
		}

		for (const ragdoll_part_def_t& part : _ragdoll.parts)
		{
			if (part.joint_index >= _joint_globals.size())
				continue;

			const mat4x3_t capsule_transform = _joint_globals[part.joint_index] * mat4x3_t::transform(part.local_position, part.local_rotation, vec3f_t::one);
			vec3f_t		   position			 = vec3f_t::zero;
			quat_t		   rotation			 = quat_t::identity;
			vec3f_t		   scale			 = vec3f_t::one;
			capsule_transform.decompose(position, rotation, scale);
			const f32 shape_scale = math::max(math::max(math::abs(scale.x), math::abs(scale.y)), math::abs(scale.z));

			debug_draw.draw_capsule(position, math::max(part.radius * shape_scale, 0.001f), math::max(part.half_height * shape_scale, 0.001f), rotation.get_up(), color_t::purple, 2.0f, debug_draw_depth_e::depth_tested, 20);
		}
	}

	span_t<const editor_widget_reflection_dropdown_item_t> editor_panel_ragdoll_viewer_t::resolve_dropdown_items(sid_t field_id, sid_t owner_field_id, u32 element_index, void* user_data)
	{
		editor_panel_ragdoll_viewer_t& viewer = *static_cast<editor_panel_ragdoll_viewer_t*>(user_data);

		if (field_id == "joint_index"_hs)
			return {.data = viewer._joint_dropdown_items.data(), .size = viewer._joint_dropdown_items.size()};

		if (field_id == "parent_part_index"_hs)
		{
			SFG_ASSERT(element_index < viewer._part_dropdown_names.size());

			viewer._part_dropdown_items.resize(0);
			viewer._part_dropdown_items.reserve(element_index == 0 ? 1 : element_index);

			if (element_index == 0)
				viewer._part_dropdown_items.push_back({.text = "None", .value = RAGDOLL_PART_NO_PARENT});
			else
			{
				for (u32 part_index = 0; part_index < element_index; ++part_index)
					viewer._part_dropdown_items.push_back({.text = viewer._part_dropdown_names[part_index].c_str(), .value = part_index});
			}

			return {.data = viewer._part_dropdown_items.data(), .size = viewer._part_dropdown_items.size()};
		}

		return {};
	}

	void editor_panel_ragdoll_viewer_t::on_asset_deletion(editor_asset_manager_t&, span_t<const sid_t> asset_ids, void* user_data)
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
				panel._skeleton = {};
				panel.rebuild_preview();
				panel.refresh_reflection();
				return;
			}
		}
	}

	void editor_panel_ragdoll_viewer_t::on_edited(void* user_data)
	{
		editor_panel_ragdoll_viewer_t& panel			= *static_cast<editor_panel_ragdoll_viewer_t*>(user_data);
		const bool					   skeleton_changed = panel._preview_skeleton != panel._ragdoll.target_skeleton;
		const bool					   topology_changed = panel._part_count != panel._ragdoll.parts.size();

		if (skeleton_changed)
		{
			panel.reload_skeleton();
			panel.rebuild_preview();
		}

		if (!skeleton_changed && !topology_changed)
			return;

		for (u32 part_index = 0; part_index < panel._ragdoll.parts.size(); ++part_index)
		{
			ragdoll_part_def_t& part = panel._ragdoll.parts[part_index];

			if (topology_changed)
			{
				if (part_index == 0)
					part.parent_part_index = RAGDOLL_PART_NO_PARENT;
				else if (part.parent_part_index == RAGDOLL_PART_NO_PARENT || part.parent_part_index >= part_index)
					part.parent_part_index = part_index - 1;
			}

			if (!skeleton_changed && part_index < panel._part_count)
				continue;

			bool joint_used = part.joint_index >= panel._skeleton.joints.size();

			for (u32 previous_index = 0; !joint_used && previous_index < part_index; ++previous_index)
				joint_used = panel._ragdoll.parts[previous_index].joint_index == part.joint_index;

			if (!joint_used)
				continue;

			part.joint_index = UINT32_MAX;

			for (const u32 candidate_joint : panel._skeleton.evaluation_order)
			{
				bool candidate_used = false;

				for (u32 previous_index = 0; previous_index < part_index; ++previous_index)
					candidate_used |= panel._ragdoll.parts[previous_index].joint_index == candidate_joint;

				if (candidate_used)
					continue;

				part.joint_index = candidate_joint;
				break;
			}
		}

		panel._part_count = static_cast<u32>(panel._ragdoll.parts.size());
		panel.refresh_part_dropdown_names();
	}

	void editor_panel_ragdoll_viewer_t::on_edit_begin(void* user_data)
	{
		editor_panel_ragdoll_viewer_t& panel = *static_cast<editor_panel_ragdoll_viewer_t*>(user_data);

		if (panel._edit_active)
			return;

		panel._edit_active = editor_command_ragdoll_edit_t::begin(panel);
	}

	void editor_panel_ragdoll_viewer_t::on_edit_submitted(void* user_data)
	{
		editor_panel_ragdoll_viewer_t& panel = *static_cast<editor_panel_ragdoll_viewer_t*>(user_data);

		if (!panel._edit_active)
			return;

		editor_command_ragdoll_edit_t::submit(panel, "Ragdoll Edit Property", false);
		panel._edit_active = false;
	}

	void editor_panel_ragdoll_viewer_t::on_world_tick(world_t& world, f32 delta_time, void* user_data)
	{
		const editor_panel_ragdoll_viewer_t& panel = *static_cast<const editor_panel_ragdoll_viewer_t*>(user_data);
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
