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
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"
#include "assets/editor_asset_util.hpp"
#include "commands/editor_command_skeleton.hpp"
#include "editor_command_system.hpp"
#include "editor_surface_controller.hpp"
#include "editor_world_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include "world/editor_world.hpp"
#include "world/editor_world_util.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/math/math.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/mesh.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
#define SKELETON_VIEWER_PANE_SPLIT_MIN				0.45f
#define SKELETON_VIEWER_PANE_SPLIT_MAX				0.85f
#define SKELETON_VIEWER_SPLIT_BORDER_THICKNESS_MULT 2.0f

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

		const editor_property_row_t preview_mesh_row   = editor_misc_widgets_t::make_property_row_with_label(ui, _right_pane, "Preview Mesh");
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

		editor_dividers_t::add_divider_hor(ui, _right_pane, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
		_root_joint_value = append_property_value_row("Root Joint");

		void* skeleton_object = &_skeleton;
		_skeleton_reflection.init(ui,
								  _right_pane,
								  {
									  .fold_states = &_fold_states,
									  .callbacks =
										  {
											  .edit_begin	  = on_edit_begin,
											  .edit_submitted = on_edit_submitted,
											  .user_data	  = this,
										  },
									  .objects					= {.data = &skeleton_object, .size = 1},
									  .type_id					= type_id_t<skeleton_def_t>::value,
									  .dropdown_items			= resolve_dropdown_items,
									  .dropdown_items_user_data = this,
									  .block_edits				= _skeleton_guid == NULL_SID,
								  });

		create_preview_world();

		if (_skeleton_guid != 0)
			set_skeleton(_skeleton_guid, _asset_name.c_str());
		else
		{
			refresh_info();
			refresh_reflection();
		}

		apply_pane_split();
	}

	void editor_panel_skeleton_viewer_t::uninit()
	{
		editor_command_skeleton_edit_t::cancel(*this);
		editor_command_system_t::get().clear_user_data(this);
		_edit_active = false;

		editor_asset_manager_t::get().remove_asset_deletion_listener(_asset_deletion_listener);

		_asset_deletion_listener = {};
		_skeleton_reflection.uninit();
		_preview_mesh_reference.uninit();
		_world_view.uninit();
		_split_border.uninit();
		_ui->deallocate_widget(_left_pane);
		_ui->deallocate_widget(_right_pane);
		destroy_preview_world();

		_joint_dropdown_items.resize(0);
		_fold_states.resize(0);
		_skeleton = {};

		editor_panel_t::uninit();
	}

	void editor_panel_skeleton_viewer_t::set_skeleton(sid_t skeleton_guid, const char* asset_name)
	{
		if (_skeleton_guid != skeleton_guid)
		{
			editor_command_skeleton_edit_t::cancel(*this);
			editor_command_system_t::get().clear_user_data(this);
			_edit_active = false;
		}

		_skeleton_guid = skeleton_guid;
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

		_preview_mesh	  = _skeleton.preview_mesh;
		_root_joint_index = _skeleton.is_evaluation_order_valid() ? _skeleton.root_joint_index : UINT32_MAX;

		refresh_preview_mesh_reference();
		create_display_entity();
		refresh_info();
		refresh_reflection();
		refresh_title(_asset_name.c_str(), "S: ");
	}

	void editor_panel_skeleton_viewer_t::apply_skeleton_def(skeleton_def_t&& skeleton)
	{
		_skeleton		  = std::move(skeleton);
		_root_joint_index = _skeleton.is_evaluation_order_valid() ? _skeleton.root_joint_index : UINT32_MAX;

		refresh_info();
		refresh_reflection();
	}

	void editor_panel_skeleton_viewer_t::create_preview_world()
	{
		const editor_world_init_config_t init_config = editor_world_init_config_t::make_preview(editor_surface_controller_t::get().get_main_surface().swapchain_size);

		editor_world_controller_t& controller = editor_world_controller_t::get();
		_world								  = controller.create_world(init_config, editor_world_edit_type_e::view_with_debug);
		editor_world_t* const editor_world	  = controller.get_editor_world(_world);

		editor_world->install_camera(editor_world_camera_type_e::orbit);

		editor_world_util_t::install_default_scene(editor_world->get_world());

		_world_view.set_edit_world(_world);
	}

	void editor_panel_skeleton_viewer_t::destroy_preview_world()
	{
		if (_world.is_null())
			return;

		editor_world_controller_t::get().destroy_world(_world);

		_world			= {};
		_display_entity = NULL_ENTITY_ID;
	}

	void editor_panel_skeleton_viewer_t::create_display_entity()
	{
		if (_world.is_null())
			return;

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

		world.scan_for_resources(_display_entity, true);

		if (const mesh_internals_t* internals = resource_manager_t::get().find_internals<mesh_internals_t>(_preview_mesh))
			editor_world_controller_t::get().get_editor_world(_world)->fit_camera_to_bounds(internals->local_bounds);
	}

	void editor_panel_skeleton_viewer_t::clear_display_entity()
	{
		if (_display_entity == NULL_ENTITY_ID)
			return;

		world_t& world = editor_world_controller_t::get().get_editor_world(_world)->get_world();

		world.destroy_entity(_display_entity);
		_display_entity = NULL_ENTITY_ID;
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

	void editor_panel_skeleton_viewer_t::refresh_reflection()
	{
		if (_ui == nullptr)
			return;

		_joint_dropdown_items.resize(0);
		_joint_dropdown_items.reserve(_skeleton.joints.size() + 1);
		_joint_dropdown_items.push_back({.text = "None", .value = SKELETON_JOINT_NO_PARENT});

		for (u32 joint_index = 0; joint_index < _skeleton.joints.size(); ++joint_index)
		{
			const skeleton_joint_def_t& joint = _skeleton.joints[joint_index];

			_joint_dropdown_items.push_back({
				.text  = joint.name.empty() ? "Unnamed Joint" : joint.name.c_str(),
				.value = joint_index,
			});
		}

		void* skeleton_object = &_skeleton;
		_skeleton_reflection.save_fold_states();
		_skeleton_reflection.set_reflection({
			.fold_states = &_fold_states,
			.callbacks =
				{
					.edit_begin		= on_edit_begin,
					.edit_submitted = on_edit_submitted,
					.user_data		= this,
				},
			.objects				  = {.data = &skeleton_object, .size = 1},
			.type_id				  = type_id_t<skeleton_def_t>::value,
			.dropdown_items			  = resolve_dropdown_items,
			.dropdown_items_user_data = this,
			.block_edits			  = _skeleton_guid == NULL_SID,
		});
	}

	void editor_panel_skeleton_viewer_t::apply_pane_split()
	{
		if (_ui != nullptr)
			_ui->get_tree().in(_left_pane).size_value.x = _pane_split;
	}

	void editor_panel_skeleton_viewer_t::on_edit_begin()
	{
		if (_edit_active)
			return;

		_edit_active = editor_command_skeleton_edit_t::begin(*this);
	}

	void editor_panel_skeleton_viewer_t::on_edit_submitted()
	{
		if (!_edit_active)
			return;

		editor_command_skeleton_edit_t::submit(*this, "Skeleton Edit Property", false);
		_edit_active = false;
	}

	span_t<const editor_widget_reflection_dropdown_item_t> editor_panel_skeleton_viewer_t::resolve_dropdown_items(sid_t field_id, sid_t owner_field_id, u32 element_index, void* user_data)
	{
		editor_panel_skeleton_viewer_t& viewer = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);

		if (field_id == "slot_joint_index"_hs)
			return {.data = viewer._joint_dropdown_items.data(), .size = viewer._joint_dropdown_items.size()};

		return {};
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
		panel._skeleton		 = {};
		panel._joint_dropdown_items.resize(0);
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

	void editor_panel_skeleton_viewer_t::on_edit_begin(void* user_data)
	{
		static_cast<editor_panel_skeleton_viewer_t*>(user_data)->on_edit_begin();
	}

	void editor_panel_skeleton_viewer_t::on_edit_submitted(void* user_data)
	{
		static_cast<editor_panel_skeleton_viewer_t*>(user_data)->on_edit_submitted();
	}

	void editor_panel_skeleton_viewer_t::on_preview_mesh_edited(void* user_data)
	{
		static_cast<editor_panel_skeleton_viewer_t*>(user_data)->create_display_entity();
	}

	void editor_panel_skeleton_viewer_t::on_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		editor_panel_skeleton_viewer_t& panel = *static_cast<editor_panel_skeleton_viewer_t*>(user_data);
		const ui::layout_out_t&			out	  = panel._ui->get_tree().out(panel._root);

		panel._pane_split = math::clamp((pos.x - out.pos.x) / out.size.x, SKELETON_VIEWER_PANE_SPLIT_MIN, SKELETON_VIEWER_PANE_SPLIT_MAX);
		panel.apply_pane_split();
	}
}
