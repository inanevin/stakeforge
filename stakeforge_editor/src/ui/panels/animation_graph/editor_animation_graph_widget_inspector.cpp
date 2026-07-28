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

#include "ui/panels/animation_graph/editor_animation_graph_widget_inspector.hpp"
#include "ui/panels/animation_graph/editor_animation_graph_context.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"
#include "commands/editor_command_animation_graph.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void editor_animation_graph_widget_inspector_t::init(ui::ui_context& ui, ui::widget_id_t parent, const config_t& config)
	{
		SFG_ASSERT(config.context != nullptr);

		_ui		= &ui;
		_config = config;
		_root	= ui.allocate_widget();

		ui.set_widget_debug_name(_root, "animation_graph_inspector");
		ui.get_tree().attach(parent, _root);
		const editor_theme_t& theme = editor_theme_t::get();

		ui::layout_in_t& root_in = ui.get_tree().in(_root);
		root_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y		 = ui::axis_mode_e::sum_children;
		root_in.size_value		 = vec2f_t::one;
		root_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};

		root_in.flow = ui::flow_e::column;

		_graph_title = editor_misc_widgets_t::make_section_label(ui, _root, "Animation Graph");

		const editor_property_row_t asset_row = editor_misc_widgets_t::make_property_row_with_label(ui, _root, "Asset", false, false, editor_theme_t::get().margin_horizontal);

		_asset_name_label = ui.allocate_widget();
		ui.set_widget_debug_name(_asset_name_label, "animation_graph_inspector_asset_name");
		ui.get_tree().attach(asset_row.right, _asset_name_label);

		ui::layout_in_t& asset_name_in = ui.get_tree().in(_asset_name_label);
		asset_name_in.flags			   = ui::wf_visible;
		asset_name_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		asset_name_in.pos_value.y	   = 0.5f;
		asset_name_in.anchor_y		   = ui::anchor_e::center;
		asset_name_in.size_mode_x	   = ui::axis_mode_e::fill;
		asset_name_in.size_mode_y	   = ui::axis_mode_e::fixed;
		asset_name_in.size_value	   = {1.0f, theme.text_default_px_size};

		set_asset_name("");
		editor_dividers_t::add_divider_hor(ui, _root, editor_theme_t::get().divider_thickness * 2.0f, editor_theme_t::get().color_frame, editor_theme_t::get().color_frame, ui::vg_gradient_e::none);

		_graph_reflection.init(ui,
							   _root,
							   {
								   .fold_states		   = &_fold_states,
								   .elevate_draw_order = true,
							   });

		_invalid_skeleton_frame = ui.allocate_widget();
		ui.set_widget_debug_name(_invalid_skeleton_frame, "animation_graph_inspector_invalid_skeleton_frame");
		ui.get_tree().attach(_root, _invalid_skeleton_frame);

		ui::layout_in_t& invalid_skeleton_frame_in = ui.get_tree().in(_invalid_skeleton_frame);
		invalid_skeleton_frame_in.size_mode_x	   = ui::axis_mode_e::parent_relative;
		invalid_skeleton_frame_in.size_mode_y	   = ui::axis_mode_e::fixed;
		invalid_skeleton_frame_in.size_value	   = {1.0f, theme.item_area_height};
		invalid_skeleton_frame_in.child_clip_mode  = ui::clip_mode_e::cpu_rect;

		ui.get_paint().set_rect(_invalid_skeleton_frame,
								{
									.fill_color_a	   = theme.color_frame,
									.fill_color_b	   = theme.color_frame,
									.outline_color	   = theme.color_outline_light,
									.rounding		   = theme.item_rounding,
									.outline_thickness = theme.outline_thickness,
								});

		_invalid_skeleton_label = ui.allocate_widget();
		ui.set_widget_debug_name(_invalid_skeleton_label, "animation_graph_inspector_invalid_skeleton_label");
		ui.get_tree().attach(_invalid_skeleton_frame, _invalid_skeleton_label);
		ui.get_tree().draw_order(_invalid_skeleton_label) = ui.get_tree().draw_order_const(_invalid_skeleton_frame) + 1;

		ui::layout_in_t& invalid_skeleton_label_in = ui.get_tree().in(_invalid_skeleton_label);
		invalid_skeleton_label_in.pos_mode_x	   = ui::pos_mode_e::relative_in_parent;
		invalid_skeleton_label_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
		invalid_skeleton_label_in.pos_value		   = {0.5f, 0.5f};
		invalid_skeleton_label_in.anchor_x		   = ui::anchor_e::center;
		invalid_skeleton_label_in.anchor_y		   = ui::anchor_e::center;

		ui.set_widget_text(_invalid_skeleton_label, "Graph does not have a valid skeleton, can't do further edits");
		ui.get_paint().set_text(_invalid_skeleton_label,
								ui.widget_text(_invalid_skeleton_label),
								ui.widget_text_len(_invalid_skeleton_label),
								{
									.font		 = theme.font_default,
									.color		 = theme.color_accent_err,
									.point_size	 = theme.text_default_px_size,
									.spacing	 = 0,
									.raster_mode = editor_text_rasterization_t::get_rasterization_type(),
								});
		ui.get_tree().set_visible(_invalid_skeleton_frame, false, false);

		_asm_node_title = editor_misc_widgets_t::make_section_label(ui, _root, "ASM Node");
		_asm_node_reflection.init(ui,
								  _root,
								  {
									  .fold_states		  = &_asm_node_fold_states,
									  .elevate_draw_order = true,
								  });

		_asm_state_title = editor_misc_widgets_t::make_section_label(ui, _root, "State");
		_asm_state_reflection.init(ui,
								   _root,
								   {
									   .fold_states		   = &_asm_state_fold_states,
									   .elevate_draw_order = true,
								   });

		_asm_transition_title = editor_misc_widgets_t::make_section_label(ui, _root, "Transition");
		_asm_transition_reflection.init(ui,
										_root,
										{
											.fold_states		= &_asm_transition_fold_states,
											.elevate_draw_order = true,
										});

		_bone_control_title = editor_misc_widgets_t::make_section_label(ui, _root, "Bone Control Node");
		_bone_control_reflection.init(ui,
									  _root,
									  {
										  .fold_states		  = &_bone_control_fold_states,
										  .elevate_draw_order = true,
									  });

		_ik_title = editor_misc_widgets_t::make_section_label(ui, _root, "IK Node");
		_ik_reflection.init(ui,
							_root,
							{
								.fold_states		= &_ik_fold_states,
								.elevate_draw_order = true,
							});

		ui.get_tree().set_visible(_asm_node_title, false, false);
		ui.get_tree().set_visible(_asm_state_title, false, false);
		ui.get_tree().set_visible(_asm_transition_title, false, false);
		ui.get_tree().set_visible(_bone_control_title, false, false);
		ui.get_tree().set_visible(_ik_title, false, false);
		ui.get_tree().set_visible(_asm_node_reflection.get_root(), false, false);
		ui.get_tree().set_visible(_asm_state_reflection.get_root(), false, false);
		ui.get_tree().set_visible(_asm_transition_reflection.get_root(), false, false);
		ui.get_tree().set_visible(_bone_control_reflection.get_root(), false, false);
		ui.get_tree().set_visible(_ik_reflection.get_root(), false, false);
	}

	void editor_animation_graph_widget_inspector_t::uninit()
	{
		_ui->cancel_mutations(this);

		if (_edit_active)
			editor_command_animation_graph_edit_t::cancel(*_config.context);

		_ik_reflection.uninit();
		_bone_control_reflection.uninit();
		_asm_transition_reflection.uninit();
		_asm_state_reflection.uninit();
		_asm_node_reflection.uninit();
		_graph_reflection.uninit();
		_ui->deallocate_widget(_root);

		_fold_states.resize(0);
		_asm_node_fold_states.resize(0);
		_asm_state_fold_states.resize(0);
		_asm_transition_fold_states.resize(0);
		_bone_control_fold_states.resize(0);
		_ik_fold_states.resize(0);
		_bone_dropdown_items.resize(0);
		_parameter_dropdown_items.resize(0);
		_skeleton				= {};
		_ui						= nullptr;
		_config					= {};
		_asset_name_label		= NULL_WIDGET;
		_graph_title			= NULL_WIDGET;
		_asm_node_title			= NULL_WIDGET;
		_asm_state_title		= NULL_WIDGET;
		_asm_transition_title	= NULL_WIDGET;
		_bone_control_title		= NULL_WIDGET;
		_ik_title				= NULL_WIDGET;
		_invalid_skeleton_frame = NULL_WIDGET;
		_invalid_skeleton_label = NULL_WIDGET;
		_root					= NULL_WIDGET;
		_edit_active			= false;
	}

	void editor_animation_graph_widget_inspector_t::refresh_inspector()
	{
		_ui->request_unique_mutation(on_refresh_mutation, this);
	}

	void editor_animation_graph_widget_inspector_t::set_asset_name(const char* asset_name)
	{
		const editor_theme_t& theme = editor_theme_t::get();

		_ui->set_widget_text(_asset_name_label, asset_name != nullptr ? asset_name : "");
		_ui->get_paint().set_text(_asset_name_label,
								  _ui->widget_text(_asset_name_label),
								  _ui->widget_text_len(_asset_name_label),
								  {
									  .font		   = theme.font_default,
									  .color	   = theme.color_text0,
									  .point_size  = theme.text_default_px_size,
									  .spacing	   = 0,
									  .raster_mode = editor_text_rasterization_t::get_rasterization_type(),
								  });
	}

	void editor_animation_graph_widget_inspector_t::refresh_dropdown_items()
	{
		const animation_graph_def_t& graph = _config.context->get_graph();

		_bone_dropdown_items.resize(0);
		_bone_dropdown_items.reserve(_skeleton.joints.size() + 1);
		_bone_dropdown_items.push_back({.text = "None", .value = UINT32_MAX});

		for (u32 joint_index = 0; joint_index < _skeleton.joints.size(); ++joint_index)
		{
			const skeleton_joint_def_t& joint = _skeleton.joints[joint_index];

			_bone_dropdown_items.push_back({
				.text  = joint.name.empty() ? "Unnamed Bone" : joint.name.c_str(),
				.value = joint_index,
			});
		}

		_parameter_dropdown_items.resize(0);
		_parameter_dropdown_items.reserve(graph.parameters.size() + 1);
		_parameter_dropdown_items.push_back({.text = "None", .value = NULL_SID});

		for (const animation_graph_param_def_t& parameter : graph.parameters)
		{
			_parameter_dropdown_items.push_back({
				.text  = parameter.name.empty() ? "Unnamed Parameter" : parameter.name.c_str(),
				.value = TO_SID(parameter.name.c_str()),
			});
		}
	}

	void editor_animation_graph_widget_inspector_t::refresh_inspector_immediate()
	{
		animation_graph_def_t&						graph		 = _config.context->get_graph();
		ui::layout_tree_t&							tree		 = _ui->get_tree();
		const editor_animation_graph_display_mode_e display_mode = _config.context->get_display_mode();
		const editor_widget_callbacks_t				callbacks{
			.edit_begin		= on_edit_begin,
			.edit_submitted = on_edit_submitted,
			.user_data		= this,
		};

		tree.set_visible(_asm_node_title, false, false);
		tree.set_visible(_asm_state_title, false, false);
		tree.set_visible(_asm_transition_title, false, false);
		tree.set_visible(_bone_control_title, false, false);
		tree.set_visible(_ik_title, false, false);
		tree.set_visible(_asm_node_reflection.get_root(), false, false);
		tree.set_visible(_asm_state_reflection.get_root(), false, false);
		tree.set_visible(_asm_transition_reflection.get_root(), false, false);
		tree.set_visible(_bone_control_reflection.get_root(), false, false);
		tree.set_visible(_ik_reflection.get_root(), false, false);
		tree.set_visible(_invalid_skeleton_frame, false, false);

		void* graph_object = &graph;

		tree.set_visible(_graph_reflection.get_root(), true, false);
		_graph_reflection.save_fold_states();
		_graph_reflection.set_reflection({
			.fold_states = &_fold_states,
			.callbacks	 = callbacks,
			.objects	 = {.data = &graph_object, .size = 1},
			.type_id	 = type_id_t<animation_graph_def_t>::value,
		});

		const u32  selected_node_id = display_mode == editor_animation_graph_display_mode_e::display_nodes ? _config.context->get_selected_node_id() : _config.context->get_display_node_id();
		const auto selected_node_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [selected_node_id](const animation_graph_node_def_t& node) { return node.id == selected_node_id; });

		if (display_mode == editor_animation_graph_display_mode_e::display_nodes && (selected_node_id == ANIMATION_GRAPH_DEF_NULL_ID || selected_node_it == graph.nodes.end()))
			return;

		SFG_ASSERT(selected_node_it != graph.nodes.end());

		if (graph.target_skeleton == NULL_RESOURCE_HANDLE)
		{
			tree.set_visible(_invalid_skeleton_frame, true, false);
			return;
		}

		const editor_asset_t* skeleton_asset = editor_asset_manager_t::get().find_asset(graph.target_skeleton);

		if (skeleton_asset == nullptr || skeleton_asset->asset_type != editor_asset_type_e::skeleton || skeleton_asset->embedded_source.empty())
		{
			tree.set_visible(_invalid_skeleton_frame, true, false);
			return;
		}

		const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(*skeleton_asset);

		_skeleton = {};

		if (!reflection_registry_t::get().type_from_json(type_id_t<skeleton_def_t>::value, &_skeleton, nullptr, embedded_source))
		{
			tree.set_visible(_invalid_skeleton_frame, true, false);
			return;
		}

		refresh_dropdown_items();

		if (display_mode == editor_animation_graph_display_mode_e::display_state_machine)
			SFG_ASSERT(selected_node_it->type == animation_graph_node_type_e::asm_node);

		switch (selected_node_it->type)
		{
		case animation_graph_node_type_e::asm_node: {
			void* asm_node = &selected_node_it->asm_node;

			tree.set_visible(_asm_node_title, true, false);
			tree.set_visible(_asm_node_reflection.get_root(), true, false);
			_asm_node_reflection.save_fold_states();
			_asm_node_reflection.set_reflection({
				.fold_states			  = &_asm_node_fold_states,
				.callbacks				  = callbacks,
				.objects				  = {.data = &asm_node, .size = 1},
				.type_id				  = type_id_t<animation_graph_node_asm_def_t>::value,
				.dropdown_items			  = resolve_dropdown_items,
				.dropdown_items_user_data = this,
			});

			if (display_mode != editor_animation_graph_display_mode_e::display_state_machine)
				break;

			const u32 selected_transition_id = _config.context->get_selected_transition_id();

			if (selected_transition_id != ANIMATION_GRAPH_DEF_NULL_ID)
			{
				const auto selected_transition_it = std::find_if(
					selected_node_it->asm_node.transitions.begin(), selected_node_it->asm_node.transitions.end(), [selected_transition_id](const animation_graph_asm_transition_def_t& transition) { return transition.id == selected_transition_id; });

				SFG_ASSERT(selected_transition_it != selected_node_it->asm_node.transitions.end());

				void* asm_transition = &*selected_transition_it;

				tree.set_visible(_asm_transition_title, true, false);
				tree.set_visible(_asm_transition_reflection.get_root(), true, false);
				_asm_transition_reflection.save_fold_states();
				_asm_transition_reflection.set_reflection({
					.fold_states			  = &_asm_transition_fold_states,
					.callbacks				  = callbacks,
					.objects				  = {.data = &asm_transition, .size = 1},
					.type_id				  = type_id_t<animation_graph_asm_transition_def_t>::value,
					.dropdown_items			  = resolve_dropdown_items,
					.dropdown_items_user_data = this,
				});
				break;
			}

			const u32 selected_state_id = _config.context->get_selected_sub_node_id();

			if (selected_state_id != ANIMATION_GRAPH_DEF_NULL_ID)
			{
				const auto selected_state_it = std::find_if(selected_node_it->asm_node.states.begin(), selected_node_it->asm_node.states.end(), [selected_state_id](const animation_graph_asm_state_def_t& state) { return state.id == selected_state_id; });

				SFG_ASSERT(selected_state_it != selected_node_it->asm_node.states.end());

				void* asm_state = &*selected_state_it;

				tree.set_visible(_asm_state_title, true, false);
				tree.set_visible(_asm_state_reflection.get_root(), true, false);
				_asm_state_reflection.save_fold_states();
				_asm_state_reflection.set_reflection({
					.fold_states			  = &_asm_state_fold_states,
					.callbacks				  = callbacks,
					.objects				  = {.data = &asm_state, .size = 1},
					.type_id				  = type_id_t<animation_graph_asm_state_def_t>::value,
					.dropdown_items			  = resolve_dropdown_items,
					.dropdown_items_user_data = this,
				});
			}
			break;
		}
		case animation_graph_node_type_e::bone_controller: {
			void* bone_control = &selected_node_it->bone_control_node;

			tree.set_visible(_bone_control_title, true, false);
			tree.set_visible(_bone_control_reflection.get_root(), true, false);
			_bone_control_reflection.save_fold_states();
			_bone_control_reflection.set_reflection({
				.fold_states			  = &_bone_control_fold_states,
				.callbacks				  = callbacks,
				.objects				  = {.data = &bone_control, .size = 1},
				.type_id				  = type_id_t<animation_graph_node_bone_control_def_t>::value,
				.dropdown_items			  = resolve_dropdown_items,
				.dropdown_items_user_data = this,
			});
			break;
		}
		case animation_graph_node_type_e::ik: {
			void* ik = &selected_node_it->ik_node;

			tree.set_visible(_ik_title, true, false);
			tree.set_visible(_ik_reflection.get_root(), true, false);
			_ik_reflection.save_fold_states();
			_ik_reflection.set_reflection({
				.fold_states = &_ik_fold_states,
				.callbacks	 = callbacks,
				.objects	 = {.data = &ik, .size = 1},
				.type_id	 = type_id_t<animation_graph_node_ik_def_t>::value,
			});
			break;
		}
		}
	}

	void editor_animation_graph_widget_inspector_t::on_edit_begin()
	{
		if (_edit_active)
			return;

		_edit_active = editor_command_animation_graph_edit_t::begin(*_config.context);
	}

	void editor_animation_graph_widget_inspector_t::on_edit_submitted()
	{
		if (!_edit_active)
			return;

		editor_command_animation_graph_edit_t::submit(*_config.context, "Animation Graph Edit Property", false);
		_edit_active = false;
	}

	span_t<const editor_widget_reflection_dropdown_item_t> editor_animation_graph_widget_inspector_t::resolve_dropdown_items(sid_t field_id, sid_t owner_field_id, void* user_data)
	{
		editor_animation_graph_widget_inspector_t& inspector = *static_cast<editor_animation_graph_widget_inspector_t*>(user_data);

		if (field_id == "bone_index"_hs)
			return {.data = inspector._bone_dropdown_items.data(), .size = inspector._bone_dropdown_items.size()};

		if (owner_field_id == "masked_bones"_hs)
			return {.data = inspector._bone_dropdown_items.data(), .size = inspector._bone_dropdown_items.size()};

		if (field_id == "parameter_id"_hs || field_id == "blend_parameter_id"_hs)
			return {.data = inspector._parameter_dropdown_items.data(), .size = inspector._parameter_dropdown_items.size()};

		return {};
	}

	void editor_animation_graph_widget_inspector_t::on_refresh_mutation(ui::ui_context& ui, void* user_data)
	{
		static_cast<editor_animation_graph_widget_inspector_t*>(user_data)->refresh_inspector_immediate();
	}

	void editor_animation_graph_widget_inspector_t::on_edit_begin(void* user_data)
	{
		static_cast<editor_animation_graph_widget_inspector_t*>(user_data)->on_edit_begin();
	}

	void editor_animation_graph_widget_inspector_t::on_edit_submitted(void* user_data)
	{
		static_cast<editor_animation_graph_widget_inspector_t*>(user_data)->on_edit_submitted();
	}

}
