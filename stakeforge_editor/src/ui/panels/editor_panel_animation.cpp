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
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_io.hpp"
#include "assets/editor_asset_manager.hpp"
#include "editor_surface_controller.hpp"
#include "editor_world_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"
#include "world/editor_world.hpp"
#include "world/editor_world_util.hpp"

#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/animation.hpp>
#include <sfg/runtime/resources/font.hpp>
#include <sfg/runtime/resources/mesh.hpp>
#include <sfg/runtime/resources/resource_manager.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>
#include <sfg/runtime/world/ecs_helpers.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/runtime/world/system_components.hpp>
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
#define ANIMATION_VIEWER_LEFT_PANE_BOTTOM_SPLIT_MIN	 0.1f
#define ANIMATION_VIEWER_LEFT_PANE_BOTTOM_SPLIT_MAX	 0.9f
#define ANIMATION_VIEWER_SPLIT_BORDER_THICKNESS_MULT 2.0f
#define ANIMATION_VIEWER_TIMELINE_FRAME_RATE		 30.0f
#define ANIMATION_VIEWER_TIMELINE_PIXELS_PER_FRAME	 12.0f
#define ANIMATION_VIEWER_TIMELINE_PADDING			 16.0f
#define ANIMATION_VIEWER_TIMELINE_LABEL_INTERVAL	 5
#define ANIMATION_VIEWER_TIMELINE_TICK_HEIGHT		 5.0f
#define ANIMATION_VIEWER_TIMELINE_KEY_SIZE			 4.0f
#define ANIMATION_VIEWER_TIMELINE_CURSOR_WIDTH		 1.0f
#define ANIMATION_VIEWER_TIMELINE_CURSOR_HEAD_WIDTH	 22.0f
#define ANIMATION_VIEWER_TIMELINE_CURSOR_HEAD_HEIGHT 14.0f
#define ANIMATION_VIEWER_TIMELINE_CURSOR_HEAD_MARGIN 2.0f

	editor_panel_animation_t::editor_panel_animation_t()
	{
		set_type(editor_panel_type_e::animation);
		refresh_title();
		set_icon(ICON_ANIMATION);
	}

	void editor_panel_animation_t::serialize(nlohmann::json& j) const
	{
		j							= nlohmann::json::object();
		j["animation_guid"]			= _animation_guid;
		j["asset_name"]				= _asset_name;
		j["pane_split"]				= _pane_split;
		j["left_pane_split"]		= _left_pane_split;
		j["left_pane_bottom_split"] = _left_pane_bottom_split;
	}

	void editor_panel_animation_t::deserialize(const nlohmann::json& j)
	{
		_animation_guid = j.value<sid_t>("animation_guid", NULL_SID);
		set_sub_item_id(_animation_guid);
		_asset_name				= j.value<string_t>("asset_name", {});
		_pane_split				= math::clamp(j.value<f32>("pane_split", _pane_split), ANIMATION_VIEWER_PANE_SPLIT_MIN, ANIMATION_VIEWER_PANE_SPLIT_MAX);
		_left_pane_split		= math::clamp(j.value<f32>("left_pane_split", _left_pane_split), ANIMATION_VIEWER_LEFT_PANE_SPLIT_MIN, ANIMATION_VIEWER_LEFT_PANE_SPLIT_MAX);
		_left_pane_bottom_split = math::clamp(j.value<f32>("left_pane_bottom_split", _left_pane_bottom_split), ANIMATION_VIEWER_LEFT_PANE_BOTTOM_SPLIT_MIN, ANIMATION_VIEWER_LEFT_PANE_BOTTOM_SPLIT_MAX);
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
		left_bottom_in.flow				= ui::flow_e::row;
		left_bottom_in.child_spacing	= 0.0f;
		left_bottom_in.child_margins	= {theme.margin_vertical, 0.0f, theme.margin_vertical, 0.0f};
		left_bottom_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		left_bottom_in.size_mode_y		= ui::axis_mode_e::fill;
		left_bottom_in.size_value		= {1.0f, 1.0f};

		ui::vg_rect_paint_t left_bottom_rect = {};
		left_bottom_rect.fill_color_a		 = theme.color_panel;
		left_bottom_rect.fill_color_b		 = theme.color_panel;
		paint.set_rect(_left_pane_bottom, left_bottom_rect);

		_left_pane_bottom_left = ui.allocate_widget();
		ui.set_widget_debug_name(_left_pane_bottom_left, "left_pane_bottom_left");
		tree.attach(_left_pane_bottom, _left_pane_bottom_left);

		ui::layout_in_t& bottom_left_in = tree.in(_left_pane_bottom_left);
		bottom_left_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		bottom_left_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		bottom_left_in.size_value		= {_left_pane_bottom_split, 1.0f};
		bottom_left_in.flow				= ui::flow_e::column;
		bottom_left_in.child_spacing	= 0.0f;
		bottom_left_in.child_clip_mode	= ui::clip_mode_e::cpu_rect;

		_left_pane_bottom_left_toolbar = ui.allocate_widget();
		ui.set_widget_debug_name(_left_pane_bottom_left_toolbar, "left_pane_bottom_left_toolbar");
		tree.attach(_left_pane_bottom_left, _left_pane_bottom_left_toolbar);

		ui::layout_in_t& bottom_left_toolbar_in = tree.in(_left_pane_bottom_left_toolbar);
		bottom_left_toolbar_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		bottom_left_toolbar_in.size_mode_y		= ui::axis_mode_e::fixed;
		bottom_left_toolbar_in.size_value		= {1.0f, theme.item_area_height};
		bottom_left_toolbar_in.flow				= ui::flow_e::row;
		bottom_left_toolbar_in.child_margins	= {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};

		_play_button.init(ui,
						  _left_pane_bottom_left_toolbar,
						  {
							  .frame_color		   = {},
							  .toggled_frame_color = theme.color_accent2_dim,
							  .hover_color		   = theme.color_panel_light1,
							  .toggled_hover_color = theme.color_accent2_dim,
							  .press_color		   = theme.color_frame_light,
							  .icon_color		   = theme.color_accent2,
							  .disabled_color	   = theme.color_text_disabled,
							  .icon				   = ICON_PLAY,
							  .toggled_icon		   = ICON_PLAY,
							  .tooltip			   = "Play",
							  .on_clicked		   = on_play_pressed,
							  .user_data		   = this,
							  .size				   = theme.item_area_height,
							  .icon_size		   = theme.text_big_px_size,
							  .rounding			   = theme.item_rounding,
							  .toggle_enabled	   = true,
						  });

		_left_pane_bottom_left_divider = editor_dividers_t::add_divider_hor(ui, _left_pane_bottom_left, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
		ui.set_widget_debug_name(_left_pane_bottom_left_divider, "left_pane_bottom_left_divider");

		_left_pane_bottom_left_body = ui.allocate_widget();
		ui.set_widget_debug_name(_left_pane_bottom_left_body, "left_pane_bottom_left_body");
		tree.attach(_left_pane_bottom_left, _left_pane_bottom_left_body);

		ui::layout_in_t& bottom_left_body_in = tree.in(_left_pane_bottom_left_body);
		bottom_left_body_in.flags |= ui::wf_scroll_y;
		bottom_left_body_in.child_clip_mode = ui::clip_mode_e::scissor_rect;
		bottom_left_body_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		bottom_left_body_in.size_mode_y		= ui::axis_mode_e::fill;
		bottom_left_body_in.size_value		= {1.0f, 1.0f};
		bottom_left_body_in.flow			= ui::flow_e::column;
		bottom_left_body_in.child_spacing	= 0.0f;

		const editor_split_border_t::config_t left_pane_bottom_split_config{
			.on_drag   = on_left_pane_bottom_split_border_drag,
			.user_data = this,
			.direction = editor_split_border_direction_e::horizontal,
		};
		_left_pane_bottom_split_border.init(ui, _left_pane_bottom, left_pane_bottom_split_config);

		ui::layout_in_t& bottom_split_border_in = tree.in(_left_pane_bottom_split_border.get_root());
		bottom_split_border_in.size_mode_x		= ui::axis_mode_e::fixed;
		bottom_split_border_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		bottom_split_border_in.size_value		= {theme.border_thickness * ANIMATION_VIEWER_SPLIT_BORDER_THICKNESS_MULT, 1.0f};

		_left_pane_bottom_right = ui.allocate_widget();
		ui.set_widget_debug_name(_left_pane_bottom_right, "left_pane_bottom_right");
		tree.attach(_left_pane_bottom, _left_pane_bottom_right);

		ui::layout_in_t& bottom_right_in = tree.in(_left_pane_bottom_right);
		bottom_right_in.size_mode_x		 = ui::axis_mode_e::fill;
		bottom_right_in.size_mode_y		 = ui::axis_mode_e::parent_relative;
		bottom_right_in.size_value		 = {1.0f, 1.0f};
		bottom_right_in.flow			 = ui::flow_e::column;
		bottom_right_in.child_spacing	 = 0.0f;

		_left_pane_bottom_right_toolbar = ui.allocate_widget();
		ui.set_widget_debug_name(_left_pane_bottom_right_toolbar, "left_pane_bottom_right_toolbar");
		tree.attach(_left_pane_bottom_right, _left_pane_bottom_right_toolbar);

		ui::layout_in_t& bottom_right_toolbar_in = tree.in(_left_pane_bottom_right_toolbar);
		bottom_right_toolbar_in.flags |= ui::wf_input | ui::wf_scroll_x;
		bottom_right_toolbar_in.child_clip_mode = ui::clip_mode_e::scissor_rect;
		bottom_right_toolbar_in.size_mode_x		= ui::axis_mode_e::parent_relative;
		bottom_right_toolbar_in.size_mode_y		= ui::axis_mode_e::fixed;
		bottom_right_toolbar_in.size_value		= {1.0f, theme.item_area_height};
		paint.set_custom(_left_pane_bottom_right_toolbar, draw_timeline_toolbar, this);

		_timeline_toolbar_content = ui.allocate_widget();
		ui.set_widget_debug_name(_timeline_toolbar_content, "animation_viewer_timeline_toolbar_content");
		tree.attach(_left_pane_bottom_right_toolbar, _timeline_toolbar_content);

		ui::layout_in_t& toolbar_content_in = tree.in(_timeline_toolbar_content);
		toolbar_content_in.size_mode_x		= ui::axis_mode_e::fixed;
		toolbar_content_in.size_mode_y		= ui::axis_mode_e::parent_relative;
		toolbar_content_in.size_value		= {ANIMATION_VIEWER_TIMELINE_PADDING * 2.0f, 1.0f};

		_left_pane_bottom_right_divider = editor_dividers_t::add_divider_hor(ui, _left_pane_bottom_right, theme.border_thickness, theme.color_divider_dark, theme.color_divider_dark, ui::vg_gradient_e::none);
		ui.set_widget_debug_name(_left_pane_bottom_right_divider, "left_pane_bottom_right_divider");

		_left_pane_bottom_right_body = ui.allocate_widget();
		ui.set_widget_debug_name(_left_pane_bottom_right_body, "left_pane_bottom_right_body");
		tree.attach(_left_pane_bottom_right, _left_pane_bottom_right_body);

		ui::layout_in_t& bottom_right_body_in = tree.in(_left_pane_bottom_right_body);
		bottom_right_body_in.flags |= ui::wf_input | ui::wf_scroll_x | ui::wf_scroll_y;
		bottom_right_body_in.child_clip_mode = ui::clip_mode_e::scissor_rect;
		bottom_right_body_in.size_mode_x	 = ui::axis_mode_e::parent_relative;
		bottom_right_body_in.size_mode_y	 = ui::axis_mode_e::fill;
		bottom_right_body_in.size_value		 = {1.0f, 1.0f};
		bottom_right_body_in.flow			 = ui::flow_e::column;
		bottom_right_body_in.child_spacing	 = 0.0f;

		_timeline_body_content = ui.allocate_widget();
		ui.set_widget_debug_name(_timeline_body_content, "animation_viewer_timeline_body_content");
		tree.attach(_left_pane_bottom_right_body, _timeline_body_content);

		ui::layout_in_t& body_content_in = tree.in(_timeline_body_content);
		body_content_in.size_mode_x		 = ui::axis_mode_e::fixed;
		body_content_in.size_mode_y		 = ui::axis_mode_e::fixed;
		body_content_in.size_value		 = {ANIMATION_VIEWER_TIMELINE_PADDING * 2.0f, 0.0f};

		_timeline_cursor_body = ui.allocate_widget();
		ui.set_widget_debug_name(_timeline_cursor_body, "animation_viewer_timeline_cursor_body");
		tree.attach(_left_pane_bottom_right_body, _timeline_cursor_body);
		tree.draw_order(_timeline_cursor_body) = tree.draw_order_const(_left_pane_bottom_right_body) + 1;

		ui::layout_in_t& cursor_body_in = tree.in(_timeline_cursor_body);
		cursor_body_in.flags |= ui::wf_overlay;
		cursor_body_in.pos_mode_x  = ui::pos_mode_e::relative_in_parent;
		cursor_body_in.pos_mode_y  = ui::pos_mode_e::relative_in_parent;
		cursor_body_in.size_mode_x = ui::axis_mode_e::parent_relative;
		cursor_body_in.size_mode_y = ui::axis_mode_e::parent_relative;
		cursor_body_in.size_value  = {1.0f, 1.0f};
		paint.set_custom(_timeline_cursor_body, draw_timeline_cursor_body, this);

		_left_pane_bottom_scrollbar.init(ui, {.target = _left_pane_bottom_right_body, .axes = editor_scrollbar_axis_xy});

		ui::listener_bundle_t timeline_listener = {};
		timeline_listener.user_data				= this;
		timeline_listener.on_press				= on_timeline_press;
		timeline_listener.on_drag_begin			= on_timeline_drag;
		timeline_listener.on_drag				= on_timeline_drag;
		timeline_listener.on_wheel				= on_timeline_wheel;
		ui.get_input().set_listener(_left_pane_bottom_right_body, timeline_listener);

		ui::listener_bundle_t timeline_toolbar_listener = {};
		timeline_toolbar_listener.user_data				= this;
		timeline_toolbar_listener.on_press				= on_timeline_press;
		timeline_toolbar_listener.on_drag_begin			= on_timeline_drag;
		timeline_toolbar_listener.on_drag				= on_timeline_drag;
		ui.get_input().set_listener(_left_pane_bottom_right_toolbar, timeline_toolbar_listener);

		ui.set_pre_layout_tick(_left_pane_bottom, on_left_pane_bottom_scroll_sync, this);

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

		rebuild_timeline(nullptr);

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
		_ui->clear_pre_layout_tick(_left_pane_bottom);
		_play_button.uninit();
		_left_pane_bottom_scrollbar.uninit();
		clear_joint_rows();
		_left_pane_bottom_split_border.uninit();
		_left_pane_split_border.uninit();
		_pane_split_border.uninit();
		_ui->deallocate_widget(_left_pane_top);
		_ui->deallocate_widget(_left_pane_bottom);
		_ui->deallocate_widget(_left_pane);
		_ui->deallocate_widget(_right_pane);

		if (!_world.is_null())
			destroy_preview_world();

		_preview_materials = {};
		_preview_skeleton  = {};
		_joint_rows.resize(0);
		_timeline_labels.resize(0);
		_timeline_keyframes.resize(0);
		_asset_name.resize(0);
		_data		= {};
		_is_playing = false;

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
		_preview_skeleton  = {};
		_data			   = {};
		_is_playing		   = false;

		if (_ui != nullptr)
			_play_button.set_toggled(false);

		if (_ui != nullptr && _animation_guid != NULL_SID)
			create_preview_world();
		else if (_ui != nullptr)
			refresh_joint_rows();

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

		refresh_preview_skeleton();
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
		component_animation_player_t&	   animation_player = ecs_helpers_t::table_add_or_get_as<component_animation_player_t>(world.get_component_table(type_id_t<component_animation_player_t>::value), _display_entity);
		const animation_runtime_t*		   animation		= resource_manager_t::get().find_runtime<animation_runtime_t>(_animation_guid);

		skinned_renderer.mesh			  = _data.target_mesh;
		skinned_renderer.skeleton		  = _data.target_skeleton;
		animation_player.animation		  = _animation_guid;
		animation_player.scrub_ratio	  = animation != nullptr && animation->duration > 0.0f ? math::min(static_cast<f32>(_timeline_cursor_frame) / ANIMATION_VIEWER_TIMELINE_FRAME_RATE / animation->duration, 1.0f) : 0.0f;
		animation_player.speed_multiplier = _data.speed_multiplier;
		animation_player.is_looping		  = _is_playing;
		animation_player.is_scrub		  = !_is_playing;

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

	void editor_panel_animation_t::refresh_preview_skeleton()
	{
		_preview_skeleton = {};

		if (_data.target_skeleton != NULL_RESOURCE_HANDLE)
		{
			const editor_asset_t* asset = editor_asset_manager_t::get().find_asset(_data.target_skeleton);

			if (asset != nullptr && asset->asset_type == editor_asset_type_e::skeleton && !asset->embedded_source.empty())
			{
				const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(*asset);

				if (!reflection_registry_t::get().type_from_json(type_id_t<skeleton_def_t>::value, &_preview_skeleton, nullptr, embedded_source))
					_preview_skeleton = {};
			}
		}

		refresh_joint_rows();
	}

	void editor_panel_animation_t::refresh_joint_rows()
	{
		clear_joint_rows();

		const animation_runtime_t* animation = _animation_guid == NULL_SID ? nullptr : resource_manager_t::get().find_runtime<animation_runtime_t>(_animation_guid);

		rebuild_timeline(animation == nullptr ? nullptr : &animation->def);

		ui::layout_tree_t&	  tree	= _ui->get_tree();
		ui::paint_layer_t&	  paint = _ui->get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_joint_rows.reserve(_preview_skeleton.joints.size());
		const u32 joint_count = static_cast<u32>(_preview_skeleton.joints.size());

		for (u32 joint_index = 0; joint_index < joint_count; ++joint_index)
		{
			const skeleton_joint_def_t& joint = _preview_skeleton.joints[joint_index];
			joint_row_t&				row	  = _joint_rows.emplace_back();
			row.owner						  = this;
			row.left_root					  = _ui->allocate_widget();

			if (animation != nullptr)
				append_joint_keyframes(animation->def, joint_index, row);

			_ui->set_widget_debug_name(row.left_root, "joint_row_left");
			tree.attach(_left_pane_bottom_left_body, row.left_root);

			ui::layout_in_t& left_row_in = tree.in(row.left_root);
			left_row_in.size_mode_x		 = ui::axis_mode_e::parent_relative;
			left_row_in.size_mode_y		 = ui::axis_mode_e::fixed;
			left_row_in.size_value		 = {1.0f, theme.item_height};
			left_row_in.child_clip_mode	 = ui::clip_mode_e::cpu_rect;
			left_row_in.child_margins	 = {0.0f, theme.margin_horizontal, 0.0f, theme.margin_horizontal};
			left_row_in.flow			 = ui::flow_e::row;

			const ui::widget_id_t joint_name = _ui->allocate_widget();
			_ui->set_widget_debug_name(joint_name, "joint_name");
			tree.attach(row.left_root, joint_name);

			ui::layout_in_t& joint_name_in = tree.in(joint_name);
			joint_name_in.pos_mode_y	   = ui::pos_mode_e::relative_in_parent;
			joint_name_in.pos_value.y	   = 0.5f;
			joint_name_in.anchor_y		   = ui::anchor_e::center;

			_ui->set_widget_text(joint_name, joint.name.c_str());
			paint.set_text(joint_name,
						   _ui->widget_text(joint_name),
						   _ui->widget_text_len(joint_name),
						   {
							   .font		= theme.font_default,
							   .color		= theme.color_text0,
							   .point_size	= theme.text_default_px_size,
							   .spacing		= 0,
							   .raster_mode = editor_text_rasterization_t::get_rasterization_type(),
						   });

			row.left_divider = editor_dividers_t::add_divider_hor(*_ui, _left_pane_bottom_left_body, theme.divider_thickness * 2.0f, theme.color_frame, theme.color_frame, ui::vg_gradient_e::none);
			_ui->set_widget_debug_name(row.left_divider, "joint_row_left_divider");

			row.right_root = _ui->allocate_widget();
			_ui->set_widget_debug_name(row.right_root, "joint_row_right");
			tree.attach(_left_pane_bottom_right_body, row.right_root);

			ui::layout_in_t& right_row_in = tree.in(row.right_root);
			right_row_in.size_mode_x	  = ui::axis_mode_e::fixed;
			right_row_in.size_mode_y	  = ui::axis_mode_e::fixed;
			right_row_in.size_value		  = {_timeline_content_width, theme.item_height};

			paint.set_custom(row.right_root, draw_timeline_joint_row, &row);

			row.right_divider = editor_dividers_t::add_divider_hor(*_ui, _left_pane_bottom_right_body, theme.divider_thickness * 2.0f, theme.color_frame, theme.color_frame, ui::vg_gradient_e::none);
			_ui->set_widget_debug_name(row.right_divider, "joint_row_right_divider");

			ui::layout_in_t& right_divider_in = tree.in(row.right_divider);
			right_divider_in.size_mode_x	  = ui::axis_mode_e::fixed;
			right_divider_in.size_value.x	  = _timeline_content_width;
		}
	}

	void editor_panel_animation_t::clear_joint_rows()
	{
		for (const joint_row_t& row : _joint_rows)
		{
			_ui->deallocate_widget(row.left_root);
			_ui->deallocate_widget(row.left_divider);
			_ui->deallocate_widget(row.right_root);
			_ui->deallocate_widget(row.right_divider);
		}

		_joint_rows.resize(0);
		_timeline_keyframes.resize(0);
	}

	void editor_panel_animation_t::rebuild_timeline(const animation_def_t* animation)
	{
		_timeline_labels.resize(0);
		_timeline_keyframes.resize(0);

		const f32 duration		= animation == nullptr ? 0.0f : animation->duration;
		_timeline_frame_count	= math::max(1u, static_cast<u32>(math::ceil(duration * ANIMATION_VIEWER_TIMELINE_FRAME_RATE)) + 1u);
		_timeline_content_width = ANIMATION_VIEWER_TIMELINE_PADDING * 2.0f + static_cast<f32>(_timeline_frame_count - 1u) * ANIMATION_VIEWER_TIMELINE_PIXELS_PER_FRAME;

		const u32 label_count = (_timeline_frame_count + ANIMATION_VIEWER_TIMELINE_LABEL_INTERVAL - 1u) / ANIMATION_VIEWER_TIMELINE_LABEL_INTERVAL + 1u;
		_timeline_labels.reserve(label_count);

		for (u32 frame = 0; frame < _timeline_frame_count; frame += ANIMATION_VIEWER_TIMELINE_LABEL_INTERVAL)
		{
			timeline_label_t& label = _timeline_labels.emplace_back();
			label.frame				= frame;
			const int written		= std::snprintf(label.text, sizeof(label.text), "%u", frame);
			label.text_length		= written > 0 ? static_cast<u8>(written) : 0;
		}

		const u32 last_frame = _timeline_frame_count - 1u;

		if (_timeline_labels.back().frame != last_frame)
		{
			timeline_label_t& label = _timeline_labels.emplace_back();
			label.frame				= last_frame;
			const int written		= std::snprintf(label.text, sizeof(label.text), "%u", last_frame);
			label.text_length		= written > 0 ? static_cast<u8>(written) : 0;
		}

		if (animation != nullptr)
		{
			size_t keyframe_count = 0;

			for (const animation_channel_v3_def_t& channel : animation->position_channels)
				keyframe_count += channel.keyframes.size() + channel.keyframes_spline.size();

			for (const animation_channel_q_def_t& channel : animation->rotation_channels)
				keyframe_count += channel.keyframes.size() + channel.keyframes_spline.size();

			for (const animation_channel_v3_def_t& channel : animation->scale_channels)
				keyframe_count += channel.keyframes.size() + channel.keyframes_spline.size();

			_timeline_keyframes.reserve(keyframe_count);
		}

		ui::layout_tree_t& tree							= _ui->get_tree();
		tree.in(_timeline_toolbar_content).size_value.x = _timeline_content_width;
		tree.in(_timeline_body_content).size_value.x	= _timeline_content_width;
		set_timeline_cursor_frame(math::min(_timeline_cursor_frame, last_frame));
	}

	void editor_panel_animation_t::append_joint_keyframes(const animation_def_t& animation, u32 joint_index, joint_row_t& row)
	{
		row.keyframe_start = static_cast<u32>(_timeline_keyframes.size());

		const auto append_time = [this](f32 time) {
			const f32 frame = math::round(time * ANIMATION_VIEWER_TIMELINE_FRAME_RATE);
			_timeline_keyframes.push_back(static_cast<u32>(math::clamp(frame, 0.0f, static_cast<f32>(_timeline_frame_count - 1u))));
		};

		for (const animation_channel_v3_def_t& channel : animation.position_channels)
		{
			if (channel.node_index != static_cast<i32>(joint_index))
				continue;

			for (const animation_keyframe_v3_t& keyframe : channel.keyframes)
				append_time(keyframe.time);

			for (const animation_keyframe_v3_spline_t& keyframe : channel.keyframes_spline)
				append_time(keyframe.time);
		}

		for (const animation_channel_q_def_t& channel : animation.rotation_channels)
		{
			if (channel.node_index != static_cast<i32>(joint_index))
				continue;

			for (const animation_keyframe_q_t& keyframe : channel.keyframes)
				append_time(keyframe.time);

			for (const animation_keyframe_q_spline_t& keyframe : channel.keyframes_spline)
				append_time(keyframe.time);
		}

		for (const animation_channel_v3_def_t& channel : animation.scale_channels)
		{
			if (channel.node_index != static_cast<i32>(joint_index))
				continue;

			for (const animation_keyframe_v3_t& keyframe : channel.keyframes)
				append_time(keyframe.time);

			for (const animation_keyframe_v3_spline_t& keyframe : channel.keyframes_spline)
				append_time(keyframe.time);
		}

		auto keyframe_begin = _timeline_keyframes.begin() + row.keyframe_start;
		std::sort(keyframe_begin, _timeline_keyframes.end());
		const auto unique_end = std::unique(keyframe_begin, _timeline_keyframes.end());
		_timeline_keyframes.erase(unique_end, _timeline_keyframes.end());
		row.keyframe_count = static_cast<u32>(_timeline_keyframes.size()) - row.keyframe_start;
	}

	void editor_panel_animation_t::set_timeline_cursor_from_position(const vec2f_t& position)
	{
		const ui::layout_tree_t& tree	   = _ui->get_tree();
		const ui::layout_out_t&	 out	   = tree.out(_left_pane_bottom_right_body);
		const ui::layout_in_t&	 in		   = tree.in_const(_left_pane_bottom_right_body);
		const f32				 scale	   = ui::get_valid_scale(_ui->get_ui_scale());
		const f32				 content_x = (position.x - out.pos.x) / scale - in.scroll_offset.x - ANIMATION_VIEWER_TIMELINE_PADDING;
		const f32				 frame	   = math::round(content_x / ANIMATION_VIEWER_TIMELINE_PIXELS_PER_FRAME);

		set_timeline_cursor_frame(static_cast<u32>(math::clamp(frame, 0.0f, static_cast<f32>(_timeline_frame_count - 1u))));
	}

	void editor_panel_animation_t::set_timeline_cursor_frame(u32 frame)
	{
		_timeline_cursor_frame = frame;

		const int written			  = std::snprintf(_timeline_cursor_label, sizeof(_timeline_cursor_label), "%u", frame);
		_timeline_cursor_label_length = written > 0 ? static_cast<u8>(written) : 0;

		update_animation_player();
	}

	void editor_panel_animation_t::set_playing(bool is_playing)
	{
		_is_playing = is_playing;
		_play_button.set_toggled(is_playing);
		update_animation_player();
	}

	void editor_panel_animation_t::update_animation_player()
	{
		if (_display_entity == NULL_ENTITY_ID)
			return;

		world_t&					  world			   = editor_world_controller_t::get().get_editor_world(_world)->get_world();
		component_animation_player_t& animation_player = ecs_helpers_t::table_get_as<component_animation_player_t>(world.get_component_table(type_id_t<component_animation_player_t>::value), _display_entity);
		const animation_runtime_t*	  animation		   = resource_manager_t::get().find_runtime<animation_runtime_t>(_animation_guid);

		animation_player.animation		  = _animation_guid;
		animation_player.scrub_ratio	  = animation != nullptr && animation->duration > 0.0f ? math::min(static_cast<f32>(_timeline_cursor_frame) / ANIMATION_VIEWER_TIMELINE_FRAME_RATE / animation->duration, 1.0f) : 0.0f;
		animation_player.speed_multiplier = _data.speed_multiplier;
		animation_player.is_looping		  = _is_playing;
		animation_player.is_scrub		  = !_is_playing;
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

		tree.in(_left_pane).size_value.x			 = _pane_split;
		tree.in(_left_pane_top).size_value.y		 = _left_pane_split;
		tree.in(_left_pane_bottom_left).size_value.x = _left_pane_bottom_split;
	}

	void editor_panel_animation_t::draw_timeline_toolbar(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		const editor_panel_animation_t& panel	   = *static_cast<editor_panel_animation_t*>(user_data);
		const ui::layout_tree_t&		tree	   = panel._ui->get_tree();
		const ui::layout_out_t&			out		   = tree.out(id);
		const ui::layout_in_t&			in		   = tree.in_const(id);
		const editor_theme_t&			theme	   = editor_theme_t::get();
		const f32						scale	   = ui::get_valid_scale(panel._ui->get_ui_scale());
		const u32						draw_order = tree.draw_order_const(id);
		ui::ui_render_state_t			state	   = {};
		state.pipeline							   = paint.get_pipelines().default_pipeline;

		canvas.push_clip(out.clip, ui::clip_mode_e::scissor_rect);

		const ui::vg_line_paint_t tick_paint{
			.color		  = theme.color_text2,
			.thickness	  = scale,
			.aa_thickness = theme.aa_thickness * scale,
		};
		const f32  visible_min_frame = ((out.clip.x - out.pos.x) / scale - in.scroll_offset.x - ANIMATION_VIEWER_TIMELINE_PADDING) / ANIMATION_VIEWER_TIMELINE_PIXELS_PER_FRAME;
		const f32  visible_max_frame = ((out.clip.x + out.clip.z - out.pos.x) / scale - in.scroll_offset.x - ANIMATION_VIEWER_TIMELINE_PADDING) / ANIMATION_VIEWER_TIMELINE_PIXELS_PER_FRAME;
		const u32  first_frame		 = visible_min_frame > ANIMATION_VIEWER_TIMELINE_LABEL_INTERVAL ? static_cast<u32>(visible_min_frame) - ANIMATION_VIEWER_TIMELINE_LABEL_INTERVAL : 0u;
		const u32  last_frame		 = math::min(panel._timeline_frame_count - 1u, visible_max_frame > 0.0f ? static_cast<u32>(visible_max_frame) + ANIMATION_VIEWER_TIMELINE_LABEL_INTERVAL : 0u);
		const auto label_begin		 = std::lower_bound(panel._timeline_labels.begin(), panel._timeline_labels.end(), first_frame, [](const timeline_label_t& label, u32 frame) { return label.frame < frame; });
		const auto label_end		 = std::upper_bound(label_begin, panel._timeline_labels.end(), last_frame, [](u32 frame, const timeline_label_t& label) { return frame < label.frame; });

		for (auto label = label_begin; label != label_end; ++label)
		{
			const f32 x = out.pos.x + (ANIMATION_VIEWER_TIMELINE_PADDING + static_cast<f32>(label->frame) * ANIMATION_VIEWER_TIMELINE_PIXELS_PER_FRAME + in.scroll_offset.x) * scale;
			canvas.add_line({x, out.pos.y + out.size.y - ANIMATION_VIEWER_TIMELINE_TICK_HEIGHT * scale}, {x, out.pos.y + out.size.y}, tick_paint, state, draw_order);
		}

		const f32 cursor_x	  = out.pos.x + (ANIMATION_VIEWER_TIMELINE_PADDING + static_cast<f32>(panel._timeline_cursor_frame) * ANIMATION_VIEWER_TIMELINE_PIXELS_PER_FRAME + in.scroll_offset.x) * scale;
		const f32 head_half	  = ANIMATION_VIEWER_TIMELINE_CURSOR_HEAD_WIDTH * 0.5f * scale;
		const f32 head_bottom = out.pos.y + out.size.y;
		const f32 head_top	  = head_bottom - ANIMATION_VIEWER_TIMELINE_CURSOR_HEAD_HEIGHT * scale;
		vec4f_t	  head_color  = theme.color_accent_green;
		head_color.w		  = 0.5f;

		const ui::vg_rect_paint_t head_paint{
			.fill_color_a = head_color,
			.fill_color_b = head_color,
			.aa_thickness = theme.aa_thickness * scale,
		};
		canvas.add_rect({cursor_x - head_half, head_top}, {cursor_x + head_half, head_bottom}, head_paint, state, draw_order + 1);

		const font_runtime_t* font = resource_manager_t::get().find_runtime<font_runtime_t>(theme.font_default);

		if (font != nullptr && font->face != nullptr)
		{
			const ui::glyph_raster_mode_e raster_mode = editor_text_rasterization_t::get_rasterization_type();
			ui::ui_render_state_t		  text_state  = {};

			switch (raster_mode)
			{
			case ui::glyph_raster_mode_e::lcd:
				text_state.pipeline = paint.get_pipelines().text_pipeline;
				break;
			case ui::glyph_raster_mode_e::grayscale:
				text_state.pipeline = paint.get_pipelines().grayscale_text_pipeline;
				break;
			case ui::glyph_raster_mode_e::sdf:
				text_state.pipeline = paint.get_pipelines().sdf_pipeline;
				break;
			}

			const ui::vg_text_paint_t text_paint{
				.font		 = font,
				.color		 = theme.color_text1,
				.size_px	 = theme.text_small_px_size * scale,
				.raster_px	 = ui::get_text_raster_px(theme.text_small_px_size * scale),
				.raster_mode = raster_mode,
			};

			for (auto label = label_begin; label != label_end; ++label)
			{
				const f32	  x			 = out.pos.x + (ANIMATION_VIEWER_TIMELINE_PADDING + static_cast<f32>(label->frame) * ANIMATION_VIEWER_TIMELINE_PIXELS_PER_FRAME + in.scroll_offset.x) * scale;
				const vec2f_t label_size = ui::vg_canvas_t::measure_text(label->text, label->text_length, text_paint);
				const vec2f_t label_pos	 = {x - label_size.x * 0.5f, out.pos.y + ANIMATION_VIEWER_TIMELINE_CURSOR_HEAD_MARGIN * scale};
				canvas.add_text(label->text, label->text_length, label_pos, text_paint, text_state, draw_order);
			}

			const ui::vg_text_paint_t cursor_text_paint{
				.font		 = font,
				.color		 = theme.color_text0,
				.size_px	 = theme.text_small_px_size * scale,
				.raster_px	 = ui::get_text_raster_px(theme.text_small_px_size * scale),
				.raster_mode = raster_mode,
			};
			const vec2f_t cursor_label_size = ui::vg_canvas_t::measure_text(panel._timeline_cursor_label, panel._timeline_cursor_label_length, cursor_text_paint);
			const vec2f_t cursor_label_pos	= {
				cursor_x - cursor_label_size.x * 0.5f,
				head_top + (head_bottom - head_top - cursor_label_size.y) * 0.5f,
			};
			canvas.add_text(panel._timeline_cursor_label, panel._timeline_cursor_label_length, cursor_label_pos, cursor_text_paint, text_state, draw_order + 2);
		}

		canvas.pop_clip(ui::clip_mode_e::scissor_rect);
	}

	void editor_panel_animation_t::draw_timeline_joint_row(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		const joint_row_t&				row		   = *static_cast<joint_row_t*>(user_data);
		const editor_panel_animation_t& panel	   = *row.owner;
		const ui::layout_tree_t&		tree	   = panel._ui->get_tree();
		const ui::layout_out_t&			out		   = tree.out(id);
		const editor_theme_t&			theme	   = editor_theme_t::get();
		const f32						scale	   = ui::get_valid_scale(panel._ui->get_ui_scale());
		const u32						draw_order = tree.draw_order_const(id);
		ui::ui_render_state_t			state	   = {};
		state.pipeline							   = paint.get_pipelines().default_pipeline;

		canvas.add_rect(out.pos, out.pos + out.size, {.fill_color_a = theme.color_frame, .fill_color_b = theme.color_frame}, state, draw_order);

		const f32					key_size = ANIMATION_VIEWER_TIMELINE_KEY_SIZE * scale;
		const f32					center_y = out.pos.y + out.size.y * 0.5f;
		const ui::vg_convex_paint_t key_paint{
			.fill_color_a = theme.color_accent1,
			.fill_color_b = theme.color_accent1,
			.aa_thickness = theme.aa_thickness * scale,
		};
		const f32  visible_min_frame = ((out.clip.x - out.pos.x) / scale - ANIMATION_VIEWER_TIMELINE_PADDING - ANIMATION_VIEWER_TIMELINE_KEY_SIZE) / ANIMATION_VIEWER_TIMELINE_PIXELS_PER_FRAME;
		const f32  visible_max_frame = ((out.clip.x + out.clip.z - out.pos.x) / scale - ANIMATION_VIEWER_TIMELINE_PADDING + ANIMATION_VIEWER_TIMELINE_KEY_SIZE) / ANIMATION_VIEWER_TIMELINE_PIXELS_PER_FRAME;
		const u32  first_frame		 = visible_min_frame > 0.0f ? static_cast<u32>(visible_min_frame) : 0u;
		const u32  last_frame		 = math::min(panel._timeline_frame_count - 1u, visible_max_frame > 0.0f ? static_cast<u32>(visible_max_frame) + 1u : 0u);
		const auto keyframe_begin	 = panel._timeline_keyframes.begin() + row.keyframe_start;
		const auto keyframe_end		 = keyframe_begin + row.keyframe_count;
		auto	   keyframe			 = std::lower_bound(keyframe_begin, keyframe_end, first_frame);

		for (; keyframe != keyframe_end && *keyframe <= last_frame; ++keyframe)
		{
			const u32 frame = *keyframe;
			const f32 x		= out.pos.x + (ANIMATION_VIEWER_TIMELINE_PADDING + static_cast<f32>(frame) * ANIMATION_VIEWER_TIMELINE_PIXELS_PER_FRAME) * scale;

			const vec2f_t path[4] = {
				{x, center_y - key_size},
				{x + key_size, center_y},
				{x, center_y + key_size},
				{x - key_size, center_y},
			};
			canvas.add_convex({path, 4}, key_paint, state, draw_order);
		}
	}

	void editor_panel_animation_t::draw_timeline_cursor_body(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		const editor_panel_animation_t& panel	= *static_cast<editor_panel_animation_t*>(user_data);
		const ui::layout_tree_t&		tree	= panel._ui->get_tree();
		const ui::layout_out_t&			out		= tree.out(id);
		const ui::layout_in_t&			body_in = tree.in_const(panel._left_pane_bottom_right_body);
		const editor_theme_t&			theme	= editor_theme_t::get();
		const f32						scale	= ui::get_valid_scale(panel._ui->get_ui_scale());
		const f32						x		= out.pos.x + (ANIMATION_VIEWER_TIMELINE_PADDING + static_cast<f32>(panel._timeline_cursor_frame) * ANIMATION_VIEWER_TIMELINE_PIXELS_PER_FRAME + body_in.scroll_offset.x) * scale;
		const ui::vg_line_paint_t		line_paint{
			.color		  = theme.color_accent_green,
			.thickness	  = ANIMATION_VIEWER_TIMELINE_CURSOR_WIDTH * scale,
			.aa_thickness = theme.aa_thickness * scale,
		};
		ui::ui_render_state_t state = {};
		state.pipeline				= paint.get_pipelines().default_pipeline;

		canvas.add_line({x, out.pos.y}, {x, out.pos.y + out.size.y}, line_paint, state, tree.draw_order_const(id));
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
		editor_panel_animation_t&				 panel			  = *static_cast<editor_panel_animation_t*>(user_data);
		world_t&								 world			  = editor_world_controller_t::get().get_editor_world(panel._world)->get_world();
		const component_skinned_mesh_renderer_t& skinned_renderer = ecs_helpers_t::table_get_as_const<component_skinned_mesh_renderer_t>(world.get_component_table(type_id_t<component_skinned_mesh_renderer_t>::value), panel._display_entity);

		if (skinned_renderer.mesh != panel._data.target_mesh || skinned_renderer.skeleton != panel._data.target_skeleton)
		{
			panel.refresh_preview_skeleton();
			panel.create_display_entity();
			return;
		}

		panel.update_animation_player();
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

	void editor_panel_animation_t::on_left_pane_bottom_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		editor_panel_animation_t& panel = *static_cast<editor_panel_animation_t*>(user_data);
		const ui::layout_out_t&	  out	= panel._ui->get_tree().out(panel._left_pane_bottom);

		panel._left_pane_bottom_split = math::clamp((pos.x - out.pos.x) / out.size.x, ANIMATION_VIEWER_LEFT_PANE_BOTTOM_SPLIT_MIN, ANIMATION_VIEWER_LEFT_PANE_BOTTOM_SPLIT_MAX);
		panel.apply_pane_splits();
	}

	void editor_panel_animation_t::on_left_pane_bottom_scroll_sync(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data)
	{
		editor_panel_animation_t& panel			= *static_cast<editor_panel_animation_t*>(user_data);
		ui::layout_tree_t&		  tree			= ui.get_tree();
		const ui::layout_in_t&	  right_body_in = tree.in_const(panel._left_pane_bottom_right_body);

		if (tree.in_const(panel._left_pane_bottom_left_body).scroll_offset.y != right_body_in.scroll_offset.y)
			tree.in(panel._left_pane_bottom_left_body).scroll_offset.y = right_body_in.scroll_offset.y;

		if (tree.in_const(panel._left_pane_bottom_right_toolbar).scroll_offset.x != right_body_in.scroll_offset.x)
			tree.in(panel._left_pane_bottom_right_toolbar).scroll_offset.x = right_body_in.scroll_offset.x;

		if (!panel._is_playing || panel._display_entity == NULL_ENTITY_ID)
			return;

		const world_t&							   world				   = editor_world_controller_t::get().get_editor_world(panel._world)->get_world();
		const component_system_animation_player_t* system_animation_player = ecs_helpers_t::table_find_as_const<component_system_animation_player_t>(world.get_component_table(type_id_t<component_system_animation_player_t>::value), panel._display_entity);
		const animation_runtime_t*				   animation			   = resource_manager_t::get().find_runtime<animation_runtime_t>(panel._animation_guid);

		if (system_animation_player == nullptr || animation == nullptr || animation->duration <= 0.0f)
			return;

		const f32 frame = math::round(system_animation_player->sample_time * ANIMATION_VIEWER_TIMELINE_FRAME_RATE);

		panel.set_timeline_cursor_frame(static_cast<u32>(math::clamp(frame, 0.0f, static_cast<f32>(panel._timeline_frame_count - 1u))));
	}

	void editor_panel_animation_t::on_play_pressed(bool toggled, void* user_data)
	{
		static_cast<editor_panel_animation_t*>(user_data)->set_playing(true);
	}

	void editor_panel_animation_t::on_timeline_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_panel_animation_t& panel = *static_cast<editor_panel_animation_t*>(user_data);
		panel.set_playing(false);
		panel.set_timeline_cursor_from_position(pos);
	}

	void editor_panel_animation_t::on_timeline_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		if (router.is_pressed(ui::mouse_button_e::left) != id)
			return;

		editor_panel_animation_t& panel = *static_cast<editor_panel_animation_t*>(user_data);
		panel.set_playing(false);
		panel.set_timeline_cursor_from_position(pos);
	}

	void editor_panel_animation_t::on_timeline_wheel(ui::input_router_t& router, ui::widget_id_t id, f32 delta, void* user_data)
	{
		editor_panel_animation_t& panel = *static_cast<editor_panel_animation_t*>(user_data);
		panel._left_pane_bottom_scrollbar.scroll_y(delta);
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
					{
						.name			   = "speed_multiplier",
						.display_name	   = "Speed Multiplier",
						.offset			   = offsetof(panel_animation_data_t, speed_multiplier),
						.size			   = sizeof(f32),
						.flags			   = reflected_field_flag_clamped,
						.min_clamp		   = 0.0f,
						.max_clamp		   = 10.0f,
						.clamp_granularity = 0.1f,
						.type			   = reflected_value_type_e::f32,
					},
				},
			.type_id   = type_id_t<panel_animation_data_t>::value,
			.size	   = sizeof(panel_animation_data_t),
			.alignment = alignof(panel_animation_data_t),
		});
	}
}
