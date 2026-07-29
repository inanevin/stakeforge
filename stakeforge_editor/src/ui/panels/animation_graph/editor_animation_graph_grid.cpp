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

#include "ui/panels/animation_graph/editor_animation_graph_grid.hpp"
#include "commands/editor_command_animation_graph.hpp"
#include "ui/panels/animation_graph/editor_animation_graph_context.hpp"
#include "ui/panels/animation_graph/editor_animation_graph_widget_node.hpp"
#include "ui/editor_action_menu_controller.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"

#include <sfg/input/input_mappings.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/math/easing.hpp>
#include <sfg/math/math.hpp>
#include <sfg/platform/process.hpp>
#include <sfg/runtime/ui/input/input_router.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>
#include <sfg/runtime/ui/vg/vg_path.hpp>

namespace sfg
{
#define ANIMATION_GRAPH_GRID_MIN_ZOOM					 0.25f
#define ANIMATION_GRAPH_GRID_MAX_ZOOM					 2.0f
#define ANIMATION_GRAPH_GRID_WHEEL_ZOOM_STEP			 0.15f
#define ANIMATION_GRAPH_GRID_ZOOM_SMOOTH_TIME			 0.1f
#define ANIMATION_GRAPH_GRID_ZOOM_MAX_SPEED				 10.0f
#define ANIMATION_GRAPH_GRID_ZOOM_SNAP_EPSILON			 0.001f
#define ANIMATION_GRAPH_GRID_CREATE_ASM_COMMAND			 1
#define ANIMATION_GRAPH_GRID_CREATE_BONE_CONTROL_COMMAND 2
#define ANIMATION_GRAPH_GRID_CREATE_IK_COMMAND			 3
#define ANIMATION_GRAPH_GRID_DELETE_NODE_COMMAND		 4
#define ANIMATION_GRAPH_GRID_DUPLICATE_NODE_COMMAND		 5
#define ANIMATION_GRAPH_GRID_MAKE_ENTRY_COMMAND			 6
#define ANIMATION_GRAPH_GRID_MAKE_EXIT_COMMAND			 7
#define ANIMATION_GRAPH_GRID_CREATE_ASM_STATE_COMMAND	 8
#define ANIMATION_GRAPH_GRID_DELETE_ASM_STATE_COMMAND	 9
#define ANIMATION_GRAPH_GRID_DUPLICATE_ASM_STATE_COMMAND 10
#define ANIMATION_GRAPH_GRID_MAKE_START_STATE_COMMAND	 11
#define ANIMATION_GRAPH_GRID_DELETE_TRANSITION_COMMAND	 12
#define ANIMATION_GRAPH_GRID_CONNECTION_SEGMENTS		 24
#define ANIMATION_GRAPH_GRID_CONNECTION_MIN_CONTROL		 48.0f
#define ANIMATION_GRAPH_GRID_CONNECTION_THICKNESS		 2.0f
#define ANIMATION_GRAPH_GRID_CONNECTION_HIT_RADIUS		 6.0f

	namespace
	{
		struct animation_graph_connection_points_t
		{
			vec2f_t control_from = vec2f_t::zero;
			vec2f_t control_to	 = vec2f_t::zero;
		};

		editor_action_menu_row_desc_t ANIMATION_GRAPH_GRID_NODES_ACTION_MENU_ROWS[] = {
			{.text = "Create ASM", .command = ANIMATION_GRAPH_GRID_CREATE_ASM_COMMAND},
			{.text = "Create Bone Control", .command = ANIMATION_GRAPH_GRID_CREATE_BONE_CONTROL_COMMAND},
			{.text = "Create IK", .command = ANIMATION_GRAPH_GRID_CREATE_IK_COMMAND},
			{.text = "Delete", .shortcut = "DEL", .command = ANIMATION_GRAPH_GRID_DELETE_NODE_COMMAND},
			{.text = "Duplicate", .shortcut = "CTRL+D", .command = ANIMATION_GRAPH_GRID_DUPLICATE_NODE_COMMAND},
			{.text = "Make Entry", .command = ANIMATION_GRAPH_GRID_MAKE_ENTRY_COMMAND},
			{.text = "Make Exit", .command = ANIMATION_GRAPH_GRID_MAKE_EXIT_COMMAND},
		};

		editor_action_menu_row_desc_t ANIMATION_GRAPH_GRID_STATE_MACHINE_ACTION_MENU_ROWS[] = {
			{.text = "Create State", .command = ANIMATION_GRAPH_GRID_CREATE_ASM_STATE_COMMAND},
			{.text = "Delete", .shortcut = "DEL", .command = ANIMATION_GRAPH_GRID_DELETE_ASM_STATE_COMMAND},
			{.text = "Duplicate", .shortcut = "CTRL+D", .command = ANIMATION_GRAPH_GRID_DUPLICATE_ASM_STATE_COMMAND},
			{.text = "Make Start", .command = ANIMATION_GRAPH_GRID_MAKE_START_STATE_COMMAND},
			{.text = "Delete Transition", .shortcut = "DEL", .command = ANIMATION_GRAPH_GRID_DELETE_TRANSITION_COMMAND},
		};

		animation_graph_connection_points_t get_animation_graph_connection_points(const vec2f_t& from, const vec2f_t& to, f32 scale)
		{
			const f32 control_distance = math::max(math::abs(to.x - from.x) * 0.5f, ANIMATION_GRAPH_GRID_CONNECTION_MIN_CONTROL * scale);

			return {
				.control_from = from + vec2f_t{control_distance, 0.0f},
				.control_to	  = to - vec2f_t{control_distance, 0.0f},
			};
		}

		void draw_animation_graph_connection(ui::vg_canvas_t& canvas, const vec2f_t& from, const vec2f_t& to, f32 scale, const ui::vg_line_paint_t& line_paint, const ui::ui_render_state_t& state, u32 draw_order)
		{
			const animation_graph_connection_points_t points = get_animation_graph_connection_points(from, to, scale);

			canvas.add_cubic_bezier(from, points.control_from, points.control_to, to, ANIMATION_GRAPH_GRID_CONNECTION_SEGMENTS, line_paint, state, draw_order);
		}
	}

	void editor_animation_graph_grid_t::init(ui::ui_context& ui, ui::widget_id_t parent, const config_t& config)
	{
		SFG_ASSERT(config.context != nullptr);
		SFG_ASSERT(config.grid_size > 0.0f);
		SFG_ASSERT(config.line_thickness > 0.0f);

		const editor_theme_t& theme = editor_theme_t::get();

		_ui		= &ui;
		_config = config;
		_root	= ui.allocate_widget();
		ui.set_widget_debug_name(_root, "animation_graph_grid");
		ui.get_tree().attach(parent, _root);

		ui::layout_in_t& in = ui.get_tree().in(_root);
		in.size_mode_x		= ui::axis_mode_e::parent_relative;
		in.size_mode_y		= ui::axis_mode_e::parent_relative;
		in.size_value		= vec2f_t::one;
		in.flags |= ui::wf_input | ui::wf_focusable;
		in.child_clip_mode = ui::clip_mode_e::scissor_rect;

		ui.get_paint().set_custom(_root, draw, this);
		ui.set_pre_layout_tick(_root, on_tick, this);

		ui::listener_bundle_t listener = {};
		listener.on_press			   = on_press;
		listener.on_click			   = on_click;
		listener.on_double_click	   = on_double_click;
		listener.on_drag_begin		   = on_drag_begin;
		listener.on_drag			   = on_drag;
		listener.on_drag_end		   = on_drag_end;
		listener.on_key				   = on_key;
		listener.on_wheel			   = on_wheel;
		listener.user_data			   = this;
		ui.get_input().set_listener(_root, listener);

		_title = ui.allocate_widget();
		ui.set_widget_debug_name(_title, "animation_graph_grid_title");
		ui.get_tree().attach(_root, _title);

		ui::layout_in_t& title_in = ui.get_tree().in(_title);
		title_in.pos_mode_x		  = ui::pos_mode_e::offset_in_parent;
		title_in.pos_mode_y		  = ui::pos_mode_e::offset_in_parent;
		title_in.pos_value		  = {theme.margin_horizontal * 2, theme.margin_vertical * 2};

		update_text("GRAPH");

		_back_button.init(ui,
						  _root,
						  {
							  .text				  = "Back",
							  .width			  = {.mode = editor_widget_width_e::fixed, .value = theme.item_width},
							  .elevate_draw_order = true,
						  });

		ui::layout_in_t& back_in = ui.get_tree().in(_back_button.get_root());
		back_in.pos_mode_x		 = ui::pos_mode_e::offset_in_parent;
		back_in.pos_mode_y		 = ui::pos_mode_e::relative_in_parent;
		back_in.pos_value		 = {0.0f, 0.9f};
		back_in.anchor_y		 = ui::anchor_e::end;
		back_in.pos_value.x		 = theme.margin_horizontal * 2;

		ui::listener_bundle_t back_listener = {};
		back_listener.on_click				= on_back;
		back_listener.user_data				= this;
		ui.get_input().set_listener(_back_button.get_root(), back_listener);
		ui.get_tree().set_visible(_back_button.get_root(), false, false);

		set_zoom_instant(1.0f);
	}

	void editor_animation_graph_grid_t::uninit()
	{
		destroy_nodes();

		_back_button.uninit();
		_ui->deallocate_widget(_title);
		_ui->deallocate_widget(_root);

		_ui							  = nullptr;
		_config						  = {};
		_offset						  = vec2f_t::zero;
		_zoom_pivot					  = vec2f_t::zero;
		_context_menu_editor_position = vec2f_t::zero;
		_context_menu_node_id		  = ANIMATION_GRAPH_DEF_NULL_ID;
		_context_menu_transition_id	  = ANIMATION_GRAPH_DEF_NULL_ID;
		_drag_node_index			  = UINT32_MAX;
		_drag_pin_node_index		  = UINT32_MAX;
		_last_clicked_node_id		  = UINT32_MAX;
		_double_click_node_id		  = UINT32_MAX;
		_root						  = NULL_WIDGET;
		_title						  = NULL_WIDGET;
		_zoom						  = 1.0f;
		_target_zoom				  = 1.0f;
		_zoom_velocity				  = 0.0f;
	}

	void editor_animation_graph_grid_t::set_mode(editor_animation_graph_display_mode_e mode)
	{
		_ui->get_tree().set_visible(_back_button.get_root(), mode != editor_animation_graph_display_mode_e::display_nodes, true);

		_last_clicked_node_id = UINT32_MAX;
		_double_click_node_id = UINT32_MAX;
		set_zoom_instant(1.0f);
		refresh_nodes();

		if (mode == editor_animation_graph_display_mode_e::display_nodes)
			update_text("GRAPH");
		else if (mode == editor_animation_graph_display_mode_e::display_state_machine)
			update_text("STATE MACHINE");
	}

	void editor_animation_graph_grid_t::refresh_nodes()
	{
		destroy_nodes();

		const animation_graph_def_t& graph = _config.context->get_graph();

		if (_config.context->get_display_mode() == editor_animation_graph_display_mode_e::display_nodes)
		{
			_nodes.reserve(graph.nodes.size());

			for (const animation_graph_node_def_t& node_def : graph.nodes)
			{
				editor_animation_graph_widget_node_t* node = new editor_animation_graph_widget_node_t();
				node->init(*_ui, _root, {.title = node_def.name.c_str(), .id = node_def.id});
				_nodes.push_back(node);
			}

			change_selection(_config.context->get_selected_node_id());
			return;
		}

		const u32  display_node_id = _config.context->get_display_node_id();
		const auto node_it		   = std::find_if(graph.nodes.begin(), graph.nodes.end(), [display_node_id](const animation_graph_node_def_t& node) { return node.id == display_node_id; });
		SFG_ASSERT(node_it != graph.nodes.end());

		_nodes.reserve(node_it->asm_node.states.size());

		for (const animation_graph_asm_state_def_t& state_def : node_it->asm_node.states)
		{
			editor_animation_graph_widget_node_t* node = new editor_animation_graph_widget_node_t();
			node->init(*_ui, _root, {.title = state_def.name.c_str(), .id = state_def.id});
			_nodes.push_back(node);
		}

		change_selection(_config.context->get_selected_sub_node_id());
	}

	void editor_animation_graph_grid_t::refresh_node_titles()
	{
		const animation_graph_def_t& graph = _config.context->get_graph();

		if (_config.context->get_display_mode() == editor_animation_graph_display_mode_e::display_nodes)
		{
			SFG_ASSERT(_nodes.size() == graph.nodes.size());

			for (size_t node_index = 0; node_index < _nodes.size(); ++node_index)
				_nodes[node_index]->update_title(graph.nodes[node_index].name.c_str());

			return;
		}

		const u32  display_node_id = _config.context->get_display_node_id();
		const auto node_it		   = std::find_if(graph.nodes.begin(), graph.nodes.end(), [display_node_id](const animation_graph_node_def_t& node) { return node.id == display_node_id; });

		SFG_ASSERT(node_it != graph.nodes.end());
		SFG_ASSERT(_nodes.size() == node_it->asm_node.states.size());

		for (size_t state_index = 0; state_index < _nodes.size(); ++state_index)
			_nodes[state_index]->update_title(node_it->asm_node.states[state_index].name.c_str());
	}

	void editor_animation_graph_grid_t::change_selection(u32 node_id)
	{
		for (editor_animation_graph_widget_node_t* node : _nodes)
			node->set_selected(node->get_id() == node_id);
	}

	void editor_animation_graph_grid_t::destroy_nodes()
	{
		for (editor_animation_graph_widget_node_t* node : _nodes)
		{
			node->uninit();
			delete node;
		}

		_nodes.resize(0);
		_drag_node_index	 = UINT32_MAX;
		_drag_pin_node_index = UINT32_MAX;
	}

	u32 editor_animation_graph_grid_t::find_pin_index_at(const vec2f_t& pos) const
	{
		for (size_t i = _nodes.size(); i > 0; --i)
		{
			const u32				node_index = static_cast<u32>(i - 1);
			const ui::layout_out_t& out		   = _ui->get_tree().out(_nodes[node_index]->get_pin_frame());
			const vec2f_t			center	   = out.pos + out.size * 0.5f;
			const vec2f_t			delta	   = pos - center;
			const f32				radius	   = out.size.x * 0.5f;

			if (delta.x * delta.x + delta.y * delta.y <= radius * radius)
				return node_index;
		}

		return UINT32_MAX;
	}

	u32 editor_animation_graph_grid_t::find_node_index_at(const vec2f_t& pos) const
	{
		for (size_t i = _nodes.size(); i > 0; --i)
		{
			const u32				node_index = static_cast<u32>(i - 1);
			const ui::layout_out_t& out		   = _ui->get_tree().out(_nodes[node_index]->get_root());

			if (pos.x >= out.pos.x && pos.x <= out.pos.x + out.size.x && pos.y >= out.pos.y && pos.y <= out.pos.y + out.size.y)
				return node_index;
		}

		return UINT32_MAX;
	}

	u32 editor_animation_graph_grid_t::find_node_at(const vec2f_t& pos) const
	{
		const u32 node_index = find_node_index_at(pos);
		return node_index != UINT32_MAX ? _nodes[node_index]->get_id() : ANIMATION_GRAPH_DEF_NULL_ID;
	}

	u32 editor_animation_graph_grid_t::find_transition_at(const vec2f_t& pos) const
	{
		if (_config.context->get_display_mode() != editor_animation_graph_display_mode_e::display_state_machine)
			return ANIMATION_GRAPH_DEF_NULL_ID;

		const animation_graph_def_t& graph			 = _config.context->get_graph();
		const u32					 display_node_id = _config.context->get_display_node_id();
		const auto					 node_it		 = std::find_if(graph.nodes.begin(), graph.nodes.end(), [display_node_id](const animation_graph_node_def_t& node) { return node.id == display_node_id; });

		SFG_ASSERT(node_it != graph.nodes.end());
		SFG_ASSERT(node_it->type == animation_graph_node_type_e::asm_node);
		SFG_ASSERT(_nodes.size() == node_it->asm_node.states.size());

		const f32 scale			 = ui::get_valid_scale(_ui->get_ui_scale());
		const f32 hit_radius	 = ANIMATION_GRAPH_GRID_CONNECTION_HIT_RADIUS * scale;
		const f32 hit_radius_sqr = hit_radius * hit_radius;

		for (size_t transition_index = node_it->asm_node.transitions.size(); transition_index > 0; --transition_index)
		{
			const animation_graph_asm_transition_def_t& transition = node_it->asm_node.transitions[transition_index - 1];
			const auto									source_it  = std::find_if(node_it->asm_node.states.begin(), node_it->asm_node.states.end(), [&transition](const animation_graph_asm_state_def_t& state) { return state.id == transition.from_state_id; });
			const auto									target_it  = std::find_if(node_it->asm_node.states.begin(), node_it->asm_node.states.end(), [&transition](const animation_graph_asm_state_def_t& state) { return state.id == transition.to_state_id; });

			SFG_ASSERT(source_it != node_it->asm_node.states.end());
			SFG_ASSERT(target_it != node_it->asm_node.states.end());

			const size_t							  source_index	 = static_cast<size_t>(source_it - node_it->asm_node.states.begin());
			const size_t							  target_index	 = static_cast<size_t>(target_it - node_it->asm_node.states.begin());
			const ui::layout_out_t&					  source_pin_out = _ui->get_tree().out(_nodes[source_index]->get_pin_frame());
			const ui::layout_out_t&					  target_out	 = _ui->get_tree().out(_nodes[target_index]->get_root());
			const vec2f_t							  from			 = source_pin_out.pos + source_pin_out.size * 0.5f;
			const vec2f_t							  to			 = target_out.pos + vec2f_t{0.0f, target_out.size.y * 0.5f};
			const animation_graph_connection_points_t points		 = get_animation_graph_connection_points(from, to, scale);
			vec2f_t									  previous		 = from;

			for (u32 segment = 1; segment <= ANIMATION_GRAPH_GRID_CONNECTION_SEGMENTS; ++segment)
			{
				const f32	  t		= static_cast<f32>(segment) / static_cast<f32>(ANIMATION_GRAPH_GRID_CONNECTION_SEGMENTS);
				const vec2f_t point = ui::vg_cubic_bezier_point(from, points.control_from, points.control_to, to, t);

				if (vec2f_t::distance_sqr_to_segment(pos, previous, point) <= hit_radius_sqr)
					return transition.id;

				previous = point;
			}
		}

		return ANIMATION_GRAPH_DEF_NULL_ID;
	}

	void editor_animation_graph_grid_t::open_context_menu(const vec2f_t& pos)
	{
		editor_action_menu_controller_t* menu	= editor_action_menu_controller_t::find(*_ui);
		const ui::layout_out_t&			 out	= _ui->get_tree().out(_root);
		const f32						 scale	= ui::get_valid_scale(_ui->get_ui_scale());
		const vec2f_t					 center = out.pos + out.size * 0.5f;

		const bool display_nodes = _config.context->get_display_mode() == editor_animation_graph_display_mode_e::display_nodes;

		_context_menu_editor_position = ((pos - center) / scale - _offset) / _zoom;
		_context_menu_node_id		  = find_node_at(pos);
		_context_menu_transition_id	  = !display_nodes && _context_menu_node_id == ANIMATION_GRAPH_DEF_NULL_ID ? find_transition_at(pos) : ANIMATION_GRAPH_DEF_NULL_ID;

		const bool hit_node		  = _context_menu_node_id != ANIMATION_GRAPH_DEF_NULL_ID;
		const bool hit_transition = _context_menu_transition_id != ANIMATION_GRAPH_DEF_NULL_ID;
		const bool only_node	  = _nodes.size() == 1;

		for (u32 i = 0; i < 3; ++i)
			ANIMATION_GRAPH_GRID_NODES_ACTION_MENU_ROWS[i].disabled = hit_node;

		ANIMATION_GRAPH_GRID_NODES_ACTION_MENU_ROWS[3].disabled = !hit_node || only_node;

		for (u32 i = 4; i < 7; ++i)
			ANIMATION_GRAPH_GRID_NODES_ACTION_MENU_ROWS[i].disabled = !hit_node;

		ANIMATION_GRAPH_GRID_STATE_MACHINE_ACTION_MENU_ROWS[0].disabled = hit_node || hit_transition;
		ANIMATION_GRAPH_GRID_STATE_MACHINE_ACTION_MENU_ROWS[1].disabled = !hit_node || only_node;
		ANIMATION_GRAPH_GRID_STATE_MACHINE_ACTION_MENU_ROWS[2].disabled = !hit_node;
		ANIMATION_GRAPH_GRID_STATE_MACHINE_ACTION_MENU_ROWS[3].disabled = !hit_node;
		ANIMATION_GRAPH_GRID_STATE_MACHINE_ACTION_MENU_ROWS[4].disabled = !hit_transition;

		editor_action_menu_desc_t desc = {};

		if (display_nodes)
		{
			desc.rows	   = ANIMATION_GRAPH_GRID_NODES_ACTION_MENU_ROWS;
			desc.row_count = static_cast<u16>(sizeof(ANIMATION_GRAPH_GRID_NODES_ACTION_MENU_ROWS) / sizeof(ANIMATION_GRAPH_GRID_NODES_ACTION_MENU_ROWS[0]));
		}
		else
		{
			desc.rows	   = ANIMATION_GRAPH_GRID_STATE_MACHINE_ACTION_MENU_ROWS;
			desc.row_count = static_cast<u16>(sizeof(ANIMATION_GRAPH_GRID_STATE_MACHINE_ACTION_MENU_ROWS) / sizeof(ANIMATION_GRAPH_GRID_STATE_MACHINE_ACTION_MENU_ROWS[0]));
		}

		desc.command_fn		   = on_context_menu_command;
		desc.command_user_data = this;
		desc.pos			   = pos;
		desc.style			   = make_default_action_menu_style(editor_theme_t::get());
		menu->request_action_menu(desc);
	}

	void editor_animation_graph_grid_t::update_text(const char* text)
	{
		const editor_theme_t& theme = editor_theme_t::get();

		_ui->set_widget_text(_title, text);
		_ui->get_paint().set_text(_title,
								  _ui->widget_text(_title),
								  _ui->widget_text_len(_title),
								  {
									  .font		   = theme.font_title_bold,
									  .color	   = theme.color_text2,
									  .point_size  = theme.text_big_px_size * 3.0f,
									  .spacing	   = 0,
									  .raster_mode = editor_text_rasterization_t::get_rasterization_type(),
								  });
	}

	void editor_animation_graph_grid_t::set_zoom_instant(f32 zoom)
	{
		const f32 clamped_zoom = math::clamp(zoom, ANIMATION_GRAPH_GRID_MIN_ZOOM, ANIMATION_GRAPH_GRID_MAX_ZOOM);
		const f32 zoom_ratio   = clamped_zoom / _zoom;

		_offset		   = _zoom_pivot + (_offset - _zoom_pivot) * zoom_ratio;
		_zoom		   = clamped_zoom;
		_target_zoom   = _zoom;
		_zoom_velocity = 0.0f;
	}

	void editor_animation_graph_grid_t::on_context_menu_command(u16 command, void* user_data)
	{
		editor_animation_graph_grid_t& grid = *static_cast<editor_animation_graph_grid_t*>(user_data);

		switch (command)
		{
		case ANIMATION_GRAPH_GRID_CREATE_ASM_COMMAND:
			editor_command_animation_graph_edit_t::add_node(*grid._config.context, animation_graph_node_type_e::asm_node, grid._context_menu_editor_position, "ASM");
			break;
		case ANIMATION_GRAPH_GRID_CREATE_BONE_CONTROL_COMMAND:
			editor_command_animation_graph_edit_t::add_node(*grid._config.context, animation_graph_node_type_e::bone_controller, grid._context_menu_editor_position, "Bone Control");
			break;
		case ANIMATION_GRAPH_GRID_CREATE_IK_COMMAND:
			editor_command_animation_graph_edit_t::add_node(*grid._config.context, animation_graph_node_type_e::ik, grid._context_menu_editor_position, "IK");
			break;
		case ANIMATION_GRAPH_GRID_DELETE_NODE_COMMAND:
			editor_command_animation_graph_edit_t::delete_node(*grid._config.context, grid._config.context->get_selected_node_id());
			break;
		case ANIMATION_GRAPH_GRID_DUPLICATE_NODE_COMMAND:
			editor_command_animation_graph_edit_t::duplicate_node(*grid._config.context, grid._config.context->get_selected_node_id());
			break;
		case ANIMATION_GRAPH_GRID_MAKE_ENTRY_COMMAND:
			editor_command_animation_graph_edit_t::make_entry(*grid._config.context, grid._context_menu_node_id);
			break;
		case ANIMATION_GRAPH_GRID_MAKE_EXIT_COMMAND:
			editor_command_animation_graph_edit_t::make_exit(*grid._config.context, grid._context_menu_node_id);
			break;
		case ANIMATION_GRAPH_GRID_CREATE_ASM_STATE_COMMAND:
			editor_command_animation_graph_edit_t::add_asm_state(*grid._config.context, grid._context_menu_editor_position, "State");
			break;
		case ANIMATION_GRAPH_GRID_DELETE_ASM_STATE_COMMAND:
			editor_command_animation_graph_edit_t::delete_asm_state(*grid._config.context, grid._context_menu_node_id);
			break;
		case ANIMATION_GRAPH_GRID_DUPLICATE_ASM_STATE_COMMAND:
			editor_command_animation_graph_edit_t::duplicate_asm_state(*grid._config.context, grid._context_menu_node_id);
			break;
		case ANIMATION_GRAPH_GRID_MAKE_START_STATE_COMMAND:
			editor_command_animation_graph_edit_t::make_start_state(*grid._config.context, grid._context_menu_node_id);
			break;
		case ANIMATION_GRAPH_GRID_DELETE_TRANSITION_COMMAND:
			editor_command_animation_graph_edit_t::delete_asm_transition(*grid._config.context, grid._context_menu_transition_id);
			break;
		default:
			break;
		}
	}

	void editor_animation_graph_grid_t::on_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data)
	{
		editor_animation_graph_grid_t& grid = *static_cast<editor_animation_graph_grid_t*>(user_data);

		const f32 next_zoom = easing_t::smooth_damp(grid._zoom, grid._target_zoom, &grid._zoom_velocity, ANIMATION_GRAPH_GRID_ZOOM_SMOOTH_TIME, ANIMATION_GRAPH_GRID_ZOOM_MAX_SPEED, dt_seconds);

		if (math::abs(next_zoom - grid._target_zoom) <= ANIMATION_GRAPH_GRID_ZOOM_SNAP_EPSILON && math::abs(grid._zoom_velocity) <= ANIMATION_GRAPH_GRID_ZOOM_SNAP_EPSILON)
			grid.set_zoom_instant(grid._target_zoom);
		else
		{
			const f32 zoom_ratio = next_zoom / grid._zoom;

			grid._offset = grid._zoom_pivot + (grid._offset - grid._zoom_pivot) * zoom_ratio;
			grid._zoom	 = next_zoom;
		}

		const ui::layout_out_t&		 out	= ui.get_tree().out(id);
		const f32					 scale	= ui::get_valid_scale(ui.get_ui_scale());
		const vec2f_t				 center = out.pos + out.size * 0.5f;
		const animation_graph_def_t& graph	= grid._config.context->get_graph();

		for (editor_animation_graph_widget_node_t* node : grid._nodes)
			node->set_zoom(grid._zoom);

		if (grid._config.context->get_display_mode() == editor_animation_graph_display_mode_e::display_nodes)
		{
			SFG_ASSERT(grid._nodes.size() == graph.nodes.size());

			for (size_t i = 0; i < grid._nodes.size(); ++i)
			{
				const bool is_entry = graph.nodes[i].id == graph.entry_node_id;
				const bool is_exit	= graph.nodes[i].id == graph.output_node_id;

				if (is_entry && is_exit)
					grid._nodes[i]->make_entry_and_exit();
				else if (is_entry)
					grid._nodes[i]->make_entry();
				else if (is_exit)
					grid._nodes[i]->make_exit();
				else
					grid._nodes[i]->clear_entry_and_exit();

				ui::layout_in_t& node_in = ui.get_tree().in(grid._nodes[i]->get_root());
				node_in.pos_value		 = center + (grid._offset + graph.nodes[i].editor_position * grid._zoom) * scale;
			}
		}
		else
		{
			const u32  display_node_id = grid._config.context->get_display_node_id();
			const auto node_it		   = std::find_if(graph.nodes.begin(), graph.nodes.end(), [display_node_id](const animation_graph_node_def_t& node) { return node.id == display_node_id; });
			SFG_ASSERT(node_it != graph.nodes.end());
			SFG_ASSERT(grid._nodes.size() == node_it->asm_node.states.size());

			for (size_t i = 0; i < grid._nodes.size(); ++i)
			{
				grid._nodes[i]->clear_entry_and_exit();
				grid._nodes[i]->set_start_state(node_it->asm_node.states[i].id == node_it->asm_node.first_state_id);

				ui::layout_in_t& node_in = ui.get_tree().in(grid._nodes[i]->get_root());
				node_in.pos_value		 = center + (grid._offset + node_it->asm_node.states[i].editor_position * grid._zoom) * scale;
			}
		}
	}

	void editor_animation_graph_grid_t::on_press(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		editor_animation_graph_grid_t& grid			= *static_cast<editor_animation_graph_grid_t*>(user_data);
		const bool					   can_drag_pin = btn == ui::mouse_button_e::left;

		grid._drag_pin_node_index = can_drag_pin ? grid.find_pin_index_at(pos) : UINT32_MAX;
		grid._drag_node_index	  = btn == ui::mouse_button_e::left && grid._drag_pin_node_index == UINT32_MAX ? grid.find_node_index_at(pos) : UINT32_MAX;
	}

	void editor_animation_graph_grid_t::on_click(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left && btn != ui::mouse_button_e::right)
			return;

		editor_animation_graph_grid_t& grid	   = *static_cast<editor_animation_graph_grid_t*>(user_data);
		const u32					   node_id = grid.find_node_at(pos);

		if (btn == ui::mouse_button_e::left)
		{
			grid._double_click_node_id = node_id == grid._last_clicked_node_id ? node_id : ANIMATION_GRAPH_DEF_NULL_ID;
			grid._last_clicked_node_id = node_id;
		}

		if (grid._config.context->get_display_mode() == editor_animation_graph_display_mode_e::display_nodes)
			editor_command_animation_graph_select_node_t::select(*grid._config.context, node_id);
		else
		{
			const u32 transition_id = node_id == ANIMATION_GRAPH_DEF_NULL_ID ? grid.find_transition_at(pos) : ANIMATION_GRAPH_DEF_NULL_ID;

			if (transition_id != ANIMATION_GRAPH_DEF_NULL_ID)
				editor_command_animation_graph_select_transition_t::select(*grid._config.context, transition_id);
			else
			{
				editor_command_animation_graph_select_transition_t::select(*grid._config.context, ANIMATION_GRAPH_DEF_NULL_ID);
				editor_command_animation_graph_select_node_t::select_sub_node(*grid._config.context, node_id);
			}
		}

		if (btn == ui::mouse_button_e::right)
			grid.open_context_menu(pos);
	}

	void editor_animation_graph_grid_t::on_double_click(ui::input_router_t&, ui::widget_id_t, const vec2f_t& pos, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_animation_graph_grid_t& grid = *static_cast<editor_animation_graph_grid_t*>(user_data);

		if (grid._config.context->get_display_mode() != editor_animation_graph_display_mode_e::display_nodes)
			return;

		const u32 node_index = grid.find_node_index_at(pos);

		if (node_index == UINT32_MAX)
			return;

		const animation_graph_node_def_t& node = grid._config.context->get_graph().nodes[node_index];

		if (node.id != grid._double_click_node_id)
			return;

		grid._last_clicked_node_id = ANIMATION_GRAPH_DEF_NULL_ID;
		grid._double_click_node_id = ANIMATION_GRAPH_DEF_NULL_ID;

		switch (node.type)
		{
		case animation_graph_node_type_e::asm_node:
			editor_command_animation_graph_set_display_mode_t::set(*grid._config.context, editor_animation_graph_display_mode_e::display_state_machine, node.id);
			break;
		case animation_graph_node_type_e::bone_controller:
		case animation_graph_node_type_e::ik:
			break;
		}
	}

	void editor_animation_graph_grid_t::on_back(ui::input_router_t&, ui::widget_id_t, const vec2f_t&, ui::mouse_button_e btn, void* user_data)
	{
		if (btn != ui::mouse_button_e::left)
			return;

		editor_animation_graph_grid_t& grid = *static_cast<editor_animation_graph_grid_t*>(user_data);

		editor_command_animation_graph_set_display_mode_t::set(*grid._config.context, editor_animation_graph_display_mode_e::display_nodes, ANIMATION_GRAPH_DEF_NULL_ID);
	}

	void editor_animation_graph_grid_t::on_key(ui::input_router_t&, ui::widget_id_t, const ui::key_event_t& ev, void* user_data)
	{
		if (ev.action != ui::key_action_e::press)
			return;

		editor_animation_graph_grid_t& grid					  = *static_cast<editor_animation_graph_grid_t*>(user_data);
		const bool					   display_nodes		  = grid._config.context->get_display_mode() == editor_animation_graph_display_mode_e::display_nodes;
		const u32					   selected_node_id		  = display_nodes ? grid._config.context->get_selected_node_id() : grid._config.context->get_selected_sub_node_id();
		const u32					   selected_transition_id = grid._config.context->get_selected_transition_id();

		if (ev.key == static_cast<u16>(input_code::key_delete) && !display_nodes && selected_transition_id != ANIMATION_GRAPH_DEF_NULL_ID)
		{
			editor_command_animation_graph_edit_t::delete_asm_transition(*grid._config.context, selected_transition_id);
			return;
		}

		if (selected_node_id == ANIMATION_GRAPH_DEF_NULL_ID)
			return;

		const bool ctrl_pressed = process::is_key_down(static_cast<u16>(input_code::key_lctrl)) || process::is_key_down(static_cast<u16>(input_code::key_rctrl));

		if (ev.key == static_cast<u16>(input_code::key_delete))
		{
			if (display_nodes)
				editor_command_animation_graph_edit_t::delete_node(*grid._config.context, selected_node_id);
			else
				editor_command_animation_graph_edit_t::delete_asm_state(*grid._config.context, selected_node_id);
		}
		else if (ev.key == static_cast<u16>(input_code::key_d) && ctrl_pressed)
		{
			if (display_nodes)
				editor_command_animation_graph_edit_t::duplicate_node(*grid._config.context, selected_node_id);
			else
				editor_command_animation_graph_edit_t::duplicate_asm_state(*grid._config.context, selected_node_id);
		}
	}

	void editor_animation_graph_grid_t::on_drag_begin(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		if (router.is_pressed(ui::mouse_button_e::left) != id)
			return;

		editor_animation_graph_grid_t& grid		  = *static_cast<editor_animation_graph_grid_t*>(user_data);
		const u32					   drag_index = grid._drag_pin_node_index != UINT32_MAX ? grid._drag_pin_node_index : grid._drag_node_index;

		if (drag_index == UINT32_MAX)
		{
			if (grid._config.context->get_display_mode() == editor_animation_graph_display_mode_e::display_state_machine)
			{
				editor_command_animation_graph_select_transition_t::select(*grid._config.context, ANIMATION_GRAPH_DEF_NULL_ID);
				editor_command_animation_graph_select_node_t::select_sub_node(*grid._config.context, ANIMATION_GRAPH_DEF_NULL_ID);
			}

			return;
		}

		if (grid._config.context->get_display_mode() == editor_animation_graph_display_mode_e::display_nodes)
			editor_command_animation_graph_select_node_t::select(*grid._config.context, grid._nodes[drag_index]->get_id());
		else
		{
			editor_command_animation_graph_select_transition_t::select(*grid._config.context, ANIMATION_GRAPH_DEF_NULL_ID);
			editor_command_animation_graph_select_node_t::select_sub_node(*grid._config.context, grid._nodes[drag_index]->get_id());
		}

		if (grid._drag_node_index != UINT32_MAX && !editor_command_animation_graph_edit_t::begin(*grid._config.context))
			grid._drag_node_index = UINT32_MAX;
	}

	void editor_animation_graph_grid_t::on_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		if (router.is_pressed(ui::mouse_button_e::left) != id)
			return;

		editor_animation_graph_grid_t& grid	 = *static_cast<editor_animation_graph_grid_t*>(user_data);
		const f32					   scale = ui::get_valid_scale(grid._ui->get_ui_scale());

		if (grid._drag_pin_node_index != UINT32_MAX)
			return;

		if (grid._drag_node_index == UINT32_MAX)
		{
			grid._offset += delta / scale;
			return;
		}

		animation_graph_def_t& graph		= grid._config.context->get_graph();
		const vec2f_t		   editor_delta = delta / (scale * grid._zoom);

		if (grid._config.context->get_display_mode() == editor_animation_graph_display_mode_e::display_nodes)
		{
			SFG_ASSERT(grid._drag_node_index < graph.nodes.size());

			graph.nodes[grid._drag_node_index].editor_position += editor_delta;
			return;
		}

		const u32  display_node_id = grid._config.context->get_display_node_id();
		const auto node_it		   = std::find_if(graph.nodes.begin(), graph.nodes.end(), [display_node_id](const animation_graph_node_def_t& node) { return node.id == display_node_id; });
		SFG_ASSERT(node_it != graph.nodes.end());
		SFG_ASSERT(grid._drag_node_index < node_it->asm_node.states.size());

		node_it->asm_node.states[grid._drag_node_index].editor_position += editor_delta;
	}

	void editor_animation_graph_grid_t::on_drag_end(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		editor_animation_graph_grid_t& grid = *static_cast<editor_animation_graph_grid_t*>(user_data);

		if (grid._drag_pin_node_index != UINT32_MAX)
		{
			const u32 target_node_index = grid.find_node_index_at(pos);

			if (target_node_index != UINT32_MAX && target_node_index != grid._drag_pin_node_index)
			{
				const u32 source_node_id = grid._nodes[grid._drag_pin_node_index]->get_id();
				const u32 target_node_id = grid._nodes[target_node_index]->get_id();

				if (grid._config.context->get_display_mode() == editor_animation_graph_display_mode_e::display_nodes)
					editor_command_animation_graph_edit_t::connect_nodes(*grid._config.context, source_node_id, target_node_id);
				else
					editor_command_animation_graph_edit_t::add_asm_transition(*grid._config.context, source_node_id, target_node_id);
			}
		}
		else if (grid._drag_node_index != UINT32_MAX)
			editor_command_animation_graph_edit_t::submit(*grid._config.context, "Animation Graph Move Node", false);

		grid._drag_node_index	  = UINT32_MAX;
		grid._drag_pin_node_index = UINT32_MAX;
	}

	void editor_animation_graph_grid_t::on_wheel(ui::input_router_t& router, ui::widget_id_t id, f32 delta, void* user_data)
	{
		editor_animation_graph_grid_t& grid	  = *static_cast<editor_animation_graph_grid_t*>(user_data);
		const ui::layout_out_t&		   out	  = grid._ui->get_tree().out(id);
		const f32					   scale  = ui::get_valid_scale(grid._ui->get_ui_scale());
		const vec2f_t				   center = out.pos + out.size * 0.5f;

		grid._zoom_pivot  = (router.get_mouse_position() - center) / scale;
		grid._target_zoom = math::clamp(grid._target_zoom + delta * grid._target_zoom * ANIMATION_GRAPH_GRID_WHEEL_ZOOM_STEP, ANIMATION_GRAPH_GRID_MIN_ZOOM, ANIMATION_GRAPH_GRID_MAX_ZOOM);
	}

	void editor_animation_graph_grid_t::draw(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		const editor_animation_graph_grid_t& grid		= *static_cast<editor_animation_graph_grid_t*>(user_data);
		const ui::layout_out_t&				 out		= grid._ui->get_tree().out(id);
		const editor_theme_t&				 theme		= editor_theme_t::get();
		const f32							 scale		= ui::get_valid_scale(grid._ui->get_ui_scale());
		const f32							 grid_size	= grid._config.grid_size * grid._zoom * scale;
		const vec2f_t						 offset		= grid._offset * scale;
		const vec2f_t						 center		= out.pos + out.size * 0.5f;
		const vec2f_t						 origin		= center + offset;
		const f32							 first_x	= origin.x - math::floor((origin.x - out.pos.x) / grid_size) * grid_size;
		const f32							 first_y	= origin.y - math::floor((origin.y - out.pos.y) / grid_size) * grid_size;
		const f32							 max_x		= out.pos.x + out.size.x;
		const f32							 max_y		= out.pos.y + out.size.y;
		const u32							 draw_order = grid._ui->get_tree().draw_order_const(id);

		ui::ui_render_state_t state = {};
		state.pipeline				= paint.get_pipelines().default_pipeline;

		const ui::vg_rect_paint_t background_paint{
			.fill_color_a = theme.color_frame,
			.fill_color_b = theme.color_frame,
		};
		canvas.add_rect(out.pos, {max_x, max_y}, background_paint, state, draw_order);

		const ui::vg_line_paint_t line_paint{
			.color	   = theme.color_panel_light,
			.thickness = grid._config.line_thickness * scale,
		};

		for (f32 x = first_x; x < max_x; x += grid_size)
			canvas.add_line({x, out.pos.y}, {x, max_y}, line_paint, state, draw_order);

		for (f32 y = first_y; y < max_y; y += grid_size)
			canvas.add_line({out.pos.x, y}, {max_x, y}, line_paint, state, draw_order);

		const animation_graph_def_t& graph = grid._config.context->get_graph();

		const ui::vg_line_paint_t connection_paint{
			.color		  = theme.color_text2,
			.thickness	  = ANIMATION_GRAPH_GRID_CONNECTION_THICKNESS * scale,
			.aa_thickness = theme.aa_thickness * scale,
		};

		if (grid._config.context->get_display_mode() == editor_animation_graph_display_mode_e::display_nodes)
		{
			SFG_ASSERT(grid._nodes.size() == graph.nodes.size());

			for (size_t source_index = 0; source_index < graph.nodes.size(); ++source_index)
			{
				const u32 target_node_id = graph.nodes[source_index].next_node_id;

				if (target_node_id == ANIMATION_GRAPH_DEF_NULL_ID)
					continue;

				const auto target_it = std::find_if(graph.nodes.begin(), graph.nodes.end(), [target_node_id](const animation_graph_node_def_t& node) { return node.id == target_node_id; });

				if (target_it == graph.nodes.end())
					continue;

				const size_t			target_index   = static_cast<size_t>(target_it - graph.nodes.begin());
				const ui::layout_out_t& source_pin_out = grid._ui->get_tree().out(grid._nodes[source_index]->get_pin_frame());
				const ui::layout_out_t& target_out	   = grid._ui->get_tree().out(grid._nodes[target_index]->get_root());
				const vec2f_t			from		   = source_pin_out.pos + source_pin_out.size * 0.5f;
				const vec2f_t			to			   = target_out.pos + vec2f_t{0.0f, target_out.size.y * 0.5f};

				draw_animation_graph_connection(canvas, from, to, scale, connection_paint, state, draw_order);
			}
		}
		else
		{
			const u32  display_node_id = grid._config.context->get_display_node_id();
			const auto node_it		   = std::find_if(graph.nodes.begin(), graph.nodes.end(), [display_node_id](const animation_graph_node_def_t& node) { return node.id == display_node_id; });

			SFG_ASSERT(node_it != graph.nodes.end());
			SFG_ASSERT(node_it->type == animation_graph_node_type_e::asm_node);
			SFG_ASSERT(grid._nodes.size() == node_it->asm_node.states.size());

			const u32 selected_transition_id = grid._config.context->get_selected_transition_id();

			for (const animation_graph_asm_transition_def_t& transition : node_it->asm_node.transitions)
			{
				const auto source_it = std::find_if(node_it->asm_node.states.begin(), node_it->asm_node.states.end(), [&transition](const animation_graph_asm_state_def_t& state_def) { return state_def.id == transition.from_state_id; });
				const auto target_it = std::find_if(node_it->asm_node.states.begin(), node_it->asm_node.states.end(), [&transition](const animation_graph_asm_state_def_t& state_def) { return state_def.id == transition.to_state_id; });

				SFG_ASSERT(source_it != node_it->asm_node.states.end());
				SFG_ASSERT(target_it != node_it->asm_node.states.end());

				const size_t			source_index	 = static_cast<size_t>(source_it - node_it->asm_node.states.begin());
				const size_t			target_index	 = static_cast<size_t>(target_it - node_it->asm_node.states.begin());
				const ui::layout_out_t& source_pin_out	 = grid._ui->get_tree().out(grid._nodes[source_index]->get_pin_frame());
				const ui::layout_out_t& target_out		 = grid._ui->get_tree().out(grid._nodes[target_index]->get_root());
				const vec2f_t			from			 = source_pin_out.pos + source_pin_out.size * 0.5f;
				const vec2f_t			to				 = target_out.pos + vec2f_t{0.0f, target_out.size.y * 0.5f};
				ui::vg_line_paint_t		transition_paint = connection_paint;
				transition_paint.color					 = transition.id == selected_transition_id ? theme.color_accent1 : theme.color_text2;

				draw_animation_graph_connection(canvas, from, to, scale, transition_paint, state, draw_order);
			}
		}

		if (grid._drag_pin_node_index != UINT32_MAX)
		{
			const ui::layout_out_t& source_pin_out = grid._ui->get_tree().out(grid._nodes[grid._drag_pin_node_index]->get_pin_frame());
			const vec2f_t			from		   = source_pin_out.pos + source_pin_out.size * 0.5f;
			const vec2f_t			to			   = grid._ui->get_input().get_mouse_position();

			draw_animation_graph_connection(canvas, from, to, scale, connection_paint, state, draw_order);
		}
	}
}
