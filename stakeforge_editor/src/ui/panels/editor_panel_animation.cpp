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
#include "ui/panels/editor_panel_animation.hpp"
#include "assets/editor_asset_manager.hpp"
#include "editor_surface_controller.hpp"
#include "editor_world_controller.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include "world/editor_world.hpp"
#include "world/editor_world_util.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/animation.hpp>
#include <sfg/runtime/resources/mesh.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/world.hpp>
#include <sfg/runtime/world/world_init_config.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

#include <cstddef>

namespace sfg
{
#define ANIMATION_VIEWER_PANE_SPLIT_MIN				 0.45f
#define ANIMATION_VIEWER_PANE_SPLIT_MAX				 0.85f
#define ANIMATION_VIEWER_LEFT_PANE_SPLIT_MIN		 0.35f
#define ANIMATION_VIEWER_LEFT_PANE_SPLIT_MAX		 0.85f
#define ANIMATION_VIEWER_SPLIT_BORDER_THICKNESS_MULT 2.0f

	editor_panel_animation_t::editor_panel_animation_t()
	{
		set_type(editor_panel_type_e::animation);
		refresh_title();
		set_icon(ICON_ANIMATION);
	}

	void editor_panel_animation_t::serialize(nlohmann::json& j) const
	{
		j					 = nlohmann::json::object();
		j["animation_guid"]	 = _animation_guid;
		j["asset_name"]		 = _asset_name;
		j["pane_split"]		 = _pane_split;
		j["left_pane_split"] = _left_pane_split;
	}

	void editor_panel_animation_t::deserialize(const nlohmann::json& j)
	{
		_animation_guid = j.value<sid_t>("animation_guid", NULL_SID);
		set_sub_item_id(_animation_guid);
		_asset_name		 = j.value<string_t>("asset_name", {});
		_pane_split		 = math::clamp(j.value<f32>("pane_split", _pane_split), ANIMATION_VIEWER_PANE_SPLIT_MIN, ANIMATION_VIEWER_PANE_SPLIT_MAX);
		_left_pane_split = math::clamp(j.value<f32>("left_pane_split", _left_pane_split), ANIMATION_VIEWER_LEFT_PANE_SPLIT_MIN, ANIMATION_VIEWER_LEFT_PANE_SPLIT_MAX);
		refresh_title(_asset_name.c_str(), "A: ");
	}

	void editor_panel_animation_t::init(ui::ui_context& ui, ui::widget_id_t parent)
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
		ui.set_widget_debug_name(_left_pane, "animation_viewer_left_pane");
		tree.attach(_root, _left_pane);

		ui::layout_in_t& left_in = tree.in(_left_pane);
		left_in.flow			 = ui::flow_e::column;
		left_in.child_spacing	 = 0.0f;
		left_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		left_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		left_in.size_value		 = {_pane_split, 1.0f};

		_left_pane_top = ui.allocate_widget();
		ui.set_widget_debug_name(_left_pane_top, "animation_viewer_left_pane_top");
		tree.attach(_left_pane, _left_pane_top);

		ui::layout_in_t& left_top_in = tree.in(_left_pane_top);
		left_top_in.flow			 = ui::flow_e::none;
		left_top_in.child_margins	 = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};
		left_top_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		left_top_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		left_top_in.size_value		 = {1.0f, _left_pane_split};

		ui::vg_rect_paint_t left_top_rect = {};
		left_top_rect.fill_color_a		  = theme.color_frame;
		left_top_rect.fill_color_b		  = theme.color_frame;
		paint.set_rect(_left_pane_top, left_top_rect);

		_world_view.init(ui, _left_pane_top);

		const editor_split_border_t::config_t left_pane_split_config{
			.on_drag   = on_left_pane_split_border_drag,
			.user_data = this,
			.direction = editor_split_border_direction_e::vertical,
		};
		_left_pane_split_border.init(ui, _left_pane, left_pane_split_config);

		ui::layout_in_t& left_pane_border_in = tree.in(_left_pane_split_border.get_root());
		left_pane_border_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
		left_pane_border_in.size_mode_y		 = ui::axis_mode_e::fixed;
		left_pane_border_in.size_value		 = {1.0f, theme.border_thickness * ANIMATION_VIEWER_SPLIT_BORDER_THICKNESS_MULT};

		_left_pane_bottom = ui.allocate_widget();
		ui.set_widget_debug_name(_left_pane_bottom, "animation_viewer_left_pane_bottom");
		tree.attach(_left_pane, _left_pane_bottom);

		ui::layout_in_t& left_bottom_in = tree.in(_left_pane_bottom);
		left_bottom_in.flow				= ui::flow_e::none;
		left_bottom_in.child_margins	= {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};
		left_bottom_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		left_bottom_in.size_mode_y		= ui::axis_mode_e::fill;
		left_bottom_in.size_value		= {1.0f, 1.0f};

		ui::vg_rect_paint_t left_bottom_rect = {};
		left_bottom_rect.fill_color_a		 = theme.color_frame;
		left_bottom_rect.fill_color_b		 = theme.color_frame;
		paint.set_rect(_left_pane_bottom, left_bottom_rect);

		const editor_split_border_t::config_t pane_split_config{
			.on_drag   = on_pane_split_border_drag,
			.user_data = this,
			.direction = editor_split_border_direction_e::horizontal,
		};
		_pane_split_border.init(ui, _root, pane_split_config);

		ui::layout_in_t& pane_border_in = tree.in(_pane_split_border.get_root());
		pane_border_in.size_mode_x		= ui::axis_mode_e::fixed;
		pane_border_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		pane_border_in.size_value		= {theme.border_thickness * ANIMATION_VIEWER_SPLIT_BORDER_THICKNESS_MULT, 1.0f};

		_right_pane = ui.allocate_widget();
		ui.set_widget_debug_name(_right_pane, "animation_viewer_right_pane");
		tree.attach(_root, _right_pane);

		ui::layout_in_t& right_in = tree.in(_right_pane);
		right_in.flow			  = ui::flow_e::column;
		right_in.child_margins	  = {theme.margin_vertical, theme.margin_horizontal, theme.margin_vertical, theme.margin_horizontal};
		right_in.size_mode_x	  = ui::axis_mode_e::fill;
		right_in.size_mode_y	  = ui::axis_mode_e::parent_relative;
		right_in.size_value		  = {1.0f, 1.0f};

		editor_misc_widgets_t::make_section_label(ui, _right_pane, "Animation");

		void* data_object = &_data;
		_data_reflection.init(ui,
							  _right_pane,
							  {
								  .callbacks =
									  {
										  .edit_submitted = on_data_edit_submitted,
										  .user_data	  = this,
									  },
								  .objects	   = {.data = &data_object, .size = 1},
								  .type_id	   = type_id_t<panel_animation_data_t>::value,
								  .world	   = _world,
								  .block_edits = _animation_guid == NULL_SID,
							  });

		if (_animation_guid != NULL_SID)
			set_animation(_animation_guid, _asset_name.c_str());

		apply_pane_splits();
	}

	void editor_panel_animation_t::uninit()
	{
		editor_asset_manager_t::get().remove_asset_deletion_listener(_asset_deletion_listener);
		_asset_deletion_listener = {};

		_data_reflection.uninit();
		_world_view.uninit();
		_left_pane_split_border.uninit();
		_pane_split_border.uninit();
		_ui->deallocate_widget(_left_pane_top);
		_ui->deallocate_widget(_left_pane_bottom);
		_ui->deallocate_widget(_left_pane);
		_ui->deallocate_widget(_right_pane);

		if (!_world.is_null())
			destroy_preview_world();

		_preview_materials = {};
		_asset_name.resize(0);
		_data = {};

		editor_panel_t::uninit();
	}

	void editor_panel_animation_t::set_animation(sid_t animation_guid, const char* asset_name)
	{
		if (!_world.is_null())
		{
			_world_view.set_edit_world({});
			destroy_preview_world();
		}

		_animation_guid = animation_guid;
		set_sub_item_id(animation_guid);
		_asset_name		   = asset_name;
		_preview_materials = {};
		_data			   = {};

		if (_ui != nullptr && _animation_guid != NULL_SID)
			create_preview_world();

		if (_ui != nullptr)
			refresh_data_reflection();

		refresh_title(_asset_name.c_str(), "A: ");
	}

	void editor_panel_animation_t::create_preview_world()
	{
		SFG_ASSERT(_world.is_null());
		SFG_ASSERT(_animation_guid != NULL_SID);

		const editor_world_init_config_t init_config = editor_world_init_config_t::make_preview(editor_surface_controller_t::get().get_main_surface().swapchain_size);

		editor_world_controller_t& controller = editor_world_controller_t::get();
		_world								  = controller.create_world(init_config, editor_world_edit_type_e::view_with_debug);
		editor_world_t* const editor_world	  = controller.get_editor_world(_world);
		world_t&			  world			  = editor_world->get_world();

		editor_world->install_camera(editor_world_camera_type_e::orbit);

		editor_world_util_t::install_default_scene(world);

		_world_view.set_edit_world(_world);

		world.add_resource(resource_type_e::animation, _animation_guid);
		world.load_all_used_resources();

		const animation_runtime_t* animation = resource_manager_t::get().find_runtime<animation_runtime_t>(_animation_guid);

		if (animation != nullptr)
		{
			_data.target_mesh	  = animation->preview_mesh;
			_data.target_skeleton = animation->preview_skeleton;
			_preview_materials	  = animation->preview_materials;
		}
		else
		{
			SFG_ERR("failed to load animation preview data: {0}", _animation_guid);
		}

		create_display_entity();
	}

	void editor_panel_animation_t::destroy_preview_world()
	{
		SFG_ASSERT(!_world.is_null());

		editor_world_controller_t::get().destroy_world(_world);

		_world			= {};
		_display_entity = NULL_ENTITY_ID;
	}

	void editor_panel_animation_t::create_display_entity()
	{
		clear_display_entity();

		world_t& world	= editor_world_controller_t::get().get_editor_world(_world)->get_world();
		_display_entity = world.create_entity("animation_viewer_mesh");

		component_skinned_mesh_renderer_t& skinned_renderer = ecs_helpers_t::table_add_or_get_as<component_skinned_mesh_renderer_t>(world.get_component_table(type_id_t<component_skinned_mesh_renderer_t>::value), _display_entity);

		skinned_renderer.mesh	  = _data.target_mesh;
		skinned_renderer.skeleton = _data.target_skeleton;

		for (const resource_handle_t material : _preview_materials)
			skinned_renderer.materials.push_back(material);

		world.scan_for_resources(_display_entity, true);

		if (const mesh_internals_t* internals = resource_manager_t::get().find_internals<mesh_internals_t>(_data.target_mesh))
			editor_world_controller_t::get().get_editor_world(_world)->fit_camera_to_bounds(internals->local_bounds);
	}

	void editor_panel_animation_t::clear_display_entity()
	{
		if (_display_entity == NULL_ENTITY_ID)
			return;

		world_t& world = editor_world_controller_t::get().get_editor_world(_world)->get_world();
		world.destroy_entity(_display_entity);
		_display_entity = NULL_ENTITY_ID;
	}

	void editor_panel_animation_t::refresh_data_reflection()
	{
		void* data_object = &_data;

		_data_reflection.set_reflection({
			.callbacks =
				{
					.edit_submitted = on_data_edit_submitted,
					.user_data		= this,
				},
			.objects	 = {.data = &data_object, .size = 1},
			.type_id	 = type_id_t<panel_animation_data_t>::value,
			.world		 = _world,
			.block_edits = _animation_guid == NULL_SID,
		});
	}

	void editor_panel_animation_t::apply_pane_splits()
	{
		ui::layout_tree_t& tree = _ui->get_tree();

		tree.in(_left_pane).size_value.x	 = _pane_split;
		tree.in(_left_pane_top).size_value.y = _left_pane_split;
	}

	void editor_panel_animation_t::on_asset_deletion(editor_asset_manager_t& asset_manager, span_t<const sid_t> asset_ids, void* user_data)
	{
		editor_panel_animation_t& panel				= *static_cast<editor_panel_animation_t*>(user_data);
		bool					  animation_deleted = false;

		for (size_t i = 0; i < asset_ids.size; ++i)
		{
			if (asset_ids.data[i] == panel._animation_guid)
			{
				animation_deleted = true;
				break;
			}
		}

		if (!animation_deleted)
			return;

		editor_world_controller_t::get().get_editor_world(panel._world)->get_world().unload_all_used_resources();

		panel._animation_guid = NULL_SID;
		panel._asset_name.resize(0);
		panel.set_sub_item_id(NULL_SID);

		editor_surface_controller_t::get().request_close_panel(&panel);
	}

	void editor_panel_animation_t::on_data_edit_submitted(void* user_data)
	{
		editor_panel_animation_t& panel = *static_cast<editor_panel_animation_t*>(user_data);
		panel.create_display_entity();
	}

	void editor_panel_animation_t::on_pane_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		editor_panel_animation_t& panel = *static_cast<editor_panel_animation_t*>(user_data);
		const ui::layout_out_t&	  out	= panel._ui->get_tree().out(panel._root);

		panel._pane_split = math::clamp((pos.x - out.pos.x) / out.size.x, ANIMATION_VIEWER_PANE_SPLIT_MIN, ANIMATION_VIEWER_PANE_SPLIT_MAX);
		panel.apply_pane_splits();
	}

	void editor_panel_animation_t::on_left_pane_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		editor_panel_animation_t& panel = *static_cast<editor_panel_animation_t*>(user_data);
		const ui::layout_out_t&	  out	= panel._ui->get_tree().out(panel._left_pane);

		panel._left_pane_split = math::clamp((pos.y - out.pos.y) / out.size.y, ANIMATION_VIEWER_LEFT_PANE_SPLIT_MIN, ANIMATION_VIEWER_LEFT_PANE_SPLIT_MAX);
		panel.apply_pane_splits();
	}

	panel_animation_data_reflection_t::panel_animation_data_reflection_t()
	{
		reflection_registry_t::get().register_type({
			.name		  = "panel_animation_data_t",
			.display_name = "Animation",
			.fields =
				{
					{.name		   = "target_mesh",
					 .display_name = "Target Mesh",
					 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_MESH,
					 .offset	   = offsetof(panel_animation_data_t, target_mesh),
					 .size		   = sizeof(resource_handle_t),
					 .type		   = reflected_value_type_e::u64},
					{.name		   = "target_skeleton",
					 .display_name = "Target Skeleton",
					 .sub_type_id  = SFG_REFLECTION_RESOURCE_SUB_TYPE_ID_SKELETON,
					 .offset	   = offsetof(panel_animation_data_t, target_skeleton),
					 .size		   = sizeof(resource_handle_t),
					 .type		   = reflected_value_type_e::u64},
				},
			.type_id   = type_id_t<panel_animation_data_t>::value,
			.size	   = sizeof(panel_animation_data_t),
			.alignment = alignof(panel_animation_data_t),
		});
	}
}
