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

#include "editor_widget_animation_library_states.hpp"
#include "editor_panel_animation_library.hpp"

#include "commands/editor_command_animation_library.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widget_checkbox.hpp"
#include "ui/widgets/editor_widget_fold.hpp"
#include "ui/widgets/editor_widget_input_field.hpp"
#include "ui/widgets/editor_widget_reference.hpp"
#include "ui/widgets/editor_widgets_dividers.hpp"
#include "ui/widgets/editor_widgets_dropdown.hpp"
#include "ui/widgets/editor_widgets_misc.hpp"

#include <sfg/math/math.hpp>
#include <sfg/runtime/ui/ui_context.hpp>
#include <sfg/runtime/ui/vg/vg_canvas.hpp>

namespace sfg
{
#define ANIMATION_BLEND_PREVIEW_HEIGHT 10.0f

	struct editor_widget_animation_library_states_t::clip_controls_t
	{
		editor_widget_reference_t animation		  = {};
		editor_input_field_t	  start_time	  = {};
		editor_input_field_t	  duration		  = {};
		editor_input_field_t	  playback_speed  = {};
		editor_input_field_t	  blend_x		  = {};
		editor_input_field_t	  blend_y		  = {};
		editor_widget_fold_t	  fold			  = {};
		state_controls_t*		  state			  = nullptr;
		resource_handle_t		  animation_value = NULL_RESOURCE_HANDLE;
		vec2f_t					  drag_offset	  = vec2f_t::zero;
		u32						  index			  = 0;
		ui::widget_id_t			  diamond		  = NULL_WIDGET;
		ui::widget_id_t			  coordinates	  = NULL_WIDGET;
		ui::widget_id_t			  blend_x_row	  = NULL_WIDGET;
		ui::widget_id_t			  blend_y_row	  = NULL_WIDGET;
		bool					  dragging		  = false;
	};

	struct editor_widget_animation_library_states_t::state_controls_t
	{
		clip_controls_t							  clips[MAX_ANIMATION_LIBRARY_STATE_CLIPS] = {};
		editor_input_field_t					  name									   = {};
		editor_input_field_t					  speed									   = {};
		editor_dropdown_t						  blend_type							   = {};
		editor_checkbox_t						  loop									   = {};
		editor_widget_fold_t					  fold									   = {};
		editor_widget_button_t					  add_clip								   = {};
		editor_widget_button_t					  clear_clips							   = {};
		editor_widget_animation_library_states_t* owner									   = nullptr;
		u32										  index									   = 0;
		u32										  clip_count							   = 0;
		ui::widget_id_t							  preview								   = NULL_WIDGET;
		ui::widget_id_t							  blend_frame							   = NULL_WIDGET;
		ui::widget_id_t							  clip_list								   = NULL_WIDGET;
	};

	editor_widget_animation_library_states_t::editor_widget_animation_library_states_t()  = default;
	editor_widget_animation_library_states_t::~editor_widget_animation_library_states_t() = default;

	void editor_widget_animation_library_states_t::init(ui::ui_context& ui, ui::widget_id_t parent, editor_panel_animation_library_t& panel)
	{
		_ui	   = &ui;
		_panel = &panel;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		_root = ui.allocate_widget();
		tree.attach(parent, _root);
		ui.set_widget_debug_name(_root, "animation_library_states");

		ui::layout_in_t& root_in = tree.in(_root);

		root_in.flow		  = ui::flow_e::column;
		root_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		root_in.size_mode_y	  = ui::axis_mode_e::sum_children;
		root_in.size_value.x  = 1.0f;
		root_in.child_spacing = theme.item_spacing;

		const editor_property_row_t actions = editor_misc_widgets_t::make_property_row(ui, _root);

		_add_state.init(ui, actions.left, {.text = "Add State"});
		_clear_states.init(ui, actions.right, {.text = "Clear All"});
		ui.get_input().set_listener(_add_state.get_root(), {.on_click = on_states_pressed, .user_data = this});
		ui.get_input().set_listener(_clear_states.get_root(), {.on_click = on_states_pressed, .user_data = this});

		_list = ui.allocate_widget();
		tree.attach(_root, _list);

		ui::layout_in_t& list_in = tree.in(_list);

		list_in.flow		  = ui::flow_e::column;
		list_in.size_mode_x	  = ui::axis_mode_e::parent_relative;
		list_in.size_mode_y	  = ui::axis_mode_e::sum_children;
		list_in.size_value.x  = 1.0f;
		list_in.child_spacing = theme.item_spacing;
	}

	void editor_widget_animation_library_states_t::uninit()
	{
		clear_controls();
		_add_state.uninit();
		_clear_states.uninit();
		_ui->deallocate_widget(_root);
		reset();
		_root  = NULL_WIDGET;
		_list  = NULL_WIDGET;
		_ui	   = nullptr;
		_panel = nullptr;
	}

	void editor_widget_animation_library_states_t::reset()
	{
		for (auto& states : _state_ui)
			states.resize(0);
	}

	void editor_widget_animation_library_states_t::clear_controls()
	{
		for (state_controls_t* controls : _controls)
		{
			for (u32 i = 0; i < controls->clip_count; ++i)
			{
				clip_controls_t& clip = controls->clips[i];

				clip.animation.uninit();
				clip.start_time.uninit();
				clip.duration.uninit();
				clip.playback_speed.uninit();
				clip.blend_x.uninit();
				clip.blend_y.uninit();
				clip.fold.uninit();
			}

			controls->name.uninit();
			controls->speed.uninit();
			controls->blend_type.uninit();
			controls->loop.uninit();
			controls->add_clip.uninit();
			controls->clear_clips.uninit();
			controls->fold.uninit();

			delete controls;
		}

		_controls.resize(0);
		_layer = UINT32_MAX;
	}

	void editor_widget_animation_library_states_t::refresh()
	{
		clear_controls();
		_layer = _panel->_selected_layer;

		ui::layout_tree_t& tree = _ui->get_tree();

		if (_layer == UINT32_MAX)
		{
			tree.in(_root).flags |= ui::wf_disabled;
			return;
		}

		tree.in(_root).flags &= ~ui::wf_disabled;

		const size_t count = _panel->_library.layers[_layer].states.size();

		_state_ui[_layer].resize(count);
		_controls.reserve(count);

		for (u32 i = 0; i < count; ++i)
			create_state(i);

		if (count == 0)
			tree.in(_clear_states.get_root()).flags |= ui::wf_disabled;
		else
			tree.in(_clear_states.get_root()).flags &= ~ui::wf_disabled;

		refresh_values();
	}

	ui::widget_id_t editor_widget_animation_library_states_t::init_number(ui::widget_id_t parent, editor_input_field_t& field, const char* label, f32& value, f32 minimum, f32 maximum)
	{
		const editor_theme_t&		theme = editor_theme_t::get();
		const editor_property_row_t row	  = editor_misc_widgets_t::make_property_row_with_label(*_ui, parent, label);
		u8*							data  = reinterpret_cast<u8*>(&value);

		field.init(*_ui,
				   row.right,
				   {
					   .field	  = {.fields = {.data = &data, .size = 1}, .field_size = sizeof(value), .type = editor_input_field_field_type_e::pod_number, .is_slider = minimum == -1.0f && maximum == 1.0f},
					   .callbacks = {.edit_begin = on_edit_begin, .edited = on_edited, .edit_submitted = on_edit_submitted, .user_data = this},
					   .increment = 0.01f,
					   .min_value = minimum,
					   .max_value = maximum,
				   });

		ui::layout_tree_t& tree = _ui->get_tree();
		ui::layout_in_t&   in	= tree.in(field.get_root());

		in.size_mode_x = ui::axis_mode_e::fill;
		in.pos_mode_y  = ui::pos_mode_e::relative_in_parent;
		in.pos_value.y = 0.5f;
		in.anchor_y	   = ui::anchor_e::center;
		tree.in(row.label).flags &= ~ui::wf_input;

		const ui::widget_id_t divider	 = editor_dividers_t::add_divider_hor(*_ui, row.row, theme.divider_thickness, theme.color_outline, theme.color_outline, ui::vg_gradient_e::none);
		ui::layout_in_t&	  divider_in = tree.in(divider);

		divider_in.flags |= ui::wf_overlay;
		divider_in.pos_mode_y  = ui::pos_mode_e::relative_in_parent;
		divider_in.pos_value.y = 1.0f;
		divider_in.anchor_y	   = ui::anchor_e::end;

		return row.row;
	}

	void editor_widget_animation_library_states_t::create_state(u32 index)
	{
		_controls.push_back(new state_controls_t{});

		state_controls_t&				controls = *_controls.back();
		animation_library_state_def_t&	state	 = _panel->_library.layers[_layer].states[index];
		ui::layout_tree_t&				tree	 = _ui->get_tree();
		const editor_theme_t&			theme	 = editor_theme_t::get();
		const editor_widget_callbacks_t callbacks{.edit_begin = on_edit_begin, .edited = on_edited, .edit_submitted = on_edit_submitted, .user_data = this};
		char							label[sizeof(state.name) + sizeof("State: ")] = {};

		controls.owner = this;
		controls.index = index;
		std::snprintf(label, sizeof(label), "State: %s", state.name);
		controls.fold.init(*_ui,
						   _list,
						   {
							   .label			= label,
							   .on_fold_changed = on_state_fold_changed,
							   .user_data		= &controls,
							   .folded			= !_state_ui[_layer][index].expanded,
							   .header_frame	= false,
						   });

		const ui::widget_id_t body = controls.fold.get_body();

		tree.in(body).child_spacing = 0.0f;

		const editor_property_row_t name_row = editor_misc_widgets_t::make_property_row_with_label(*_ui, body, "Name");
		u8*							name	 = reinterpret_cast<u8*>(state.name);

		controls.name.init(*_ui,
						   name_row.right,
						   {
							   .field	  = {.fields = {.data = &name, .size = 1}, .field_size = sizeof(state.name), .type = editor_input_field_field_type_e::char_array},
							   .callbacks = callbacks,
						   });
		tree.in(controls.name.get_root()).size_mode_x = ui::axis_mode_e::fill;
		editor_dividers_t::add_divider_hor(*_ui, body, theme.divider_thickness, theme.color_outline, theme.color_outline, ui::vg_gradient_e::none);
		init_number(body, controls.speed, "Speed", state.speed, -FLT_MAX, FLT_MAX);

		const editor_property_row_t loop_row = editor_misc_widgets_t::make_property_row_with_label(*_ui, body, "Loop");
		u8*							loop	 = reinterpret_cast<u8*>(&state.loop);

		controls.loop.init(*_ui, loop_row.right, {.field = {.fields = {.data = &loop, .size = 1}, .field_size = sizeof(state.loop)}, .callbacks = callbacks});
		editor_dividers_t::add_divider_hor(*_ui, body, theme.divider_thickness, theme.color_outline, theme.color_outline, ui::vg_gradient_e::none);

		static const editor_dropdown_item_t blend_items[] = {
			{.text = "No Blend", .value = static_cast<u64>(animation_library_blend_type_e::no_blend)},
			{.text = "1D", .value = static_cast<u64>(animation_library_blend_type_e::blend_1d)},
			{.text = "2D", .value = static_cast<u64>(animation_library_blend_type_e::blend_2d)},
		};
		const editor_property_row_t blend_row = editor_misc_widgets_t::make_property_row_with_label(*_ui, body, "Blend Type");
		u8*							blend	  = reinterpret_cast<u8*>(&state.blend_type);

		controls.blend_type.init(*_ui,
								 blend_row.right,
								 {
									 .items		 = blend_items,
									 .field		 = {.fields = {.data = &blend, .size = 1}, .field_size = sizeof(state.blend_type)},
									 .callbacks	 = {.edit_begin = on_edit_begin, .edited = on_blend_edited, .edit_submitted = on_edit_submitted, .user_data = this},
									 .item_count = 3,
									 .width		 = editor_dropdown_width_e::parent_relative,
									 .pos_y		 = editor_dropdown_pos_y_e::center,
								 });
		editor_dividers_t::add_divider_hor(*_ui, body, theme.divider_thickness, theme.color_outline, theme.color_outline, ui::vg_gradient_e::none);
		init_preview(controls);
		editor_misc_widgets_t::make_section_label(*_ui, body, "Clips");

		const editor_property_row_t actions = editor_misc_widgets_t::make_property_row(*_ui, body);

		controls.add_clip.init(*_ui, actions.left, {.text = "Add New"});
		controls.clear_clips.init(*_ui, actions.right, {.text = "Clear All"});
		_ui->get_input().set_listener(controls.add_clip.get_root(), {.on_click = on_clips_pressed, .user_data = &controls});
		_ui->get_input().set_listener(controls.clear_clips.get_root(), {.on_click = on_clips_pressed, .user_data = &controls});
		controls.clip_list = _ui->allocate_widget();
		tree.attach(body, controls.clip_list);

		ui::layout_in_t& clips_in = tree.in(controls.clip_list);

		clips_in.flow		   = ui::flow_e::column;
		clips_in.size_mode_x   = ui::axis_mode_e::parent_relative;
		clips_in.size_mode_y   = ui::axis_mode_e::sum_children;
		clips_in.size_value.x  = 1.0f;
		clips_in.child_spacing = theme.item_spacing;

		for (u32 i = 0; i < state.clip_count; ++i)
			create_clip(controls, i);
	}

	void editor_widget_animation_library_states_t::init_preview(state_controls_t& controls)
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		controls.preview = _ui->allocate_widget();
		tree.attach(controls.fold.get_body(), controls.preview);

		ui::layout_in_t& preview_in = tree.in(controls.preview);

		preview_in.size_mode_x	 = ui::axis_mode_e::parent_relative;
		preview_in.size_mode_y	 = ui::axis_mode_e::fixed;
		preview_in.size_value	 = {1.0f, theme.item_height * ANIMATION_BLEND_PREVIEW_HEIGHT};
		preview_in.child_margins = {theme.item_height, theme.item_height, theme.item_height, theme.item_height};
		_ui->get_paint().set_rect(controls.preview, {.fill_color_a = theme.color_frame, .fill_color_b = theme.color_frame});
		controls.blend_frame = _ui->allocate_widget();
		tree.attach(controls.preview, controls.blend_frame);
		tree.draw_order(controls.blend_frame) = tree.draw_order_const(controls.preview) + 1;

		ui::layout_in_t& inner_in = tree.in(controls.blend_frame);

		inner_in.size_mode_x = ui::axis_mode_e::copy_other;
		inner_in.size_mode_y = ui::axis_mode_e::parent_relative;
		inner_in.size_value	 = {1.0f, 1.0f};
		inner_in.pos_mode_x	 = ui::pos_mode_e::relative_in_parent;
		inner_in.pos_mode_y	 = ui::pos_mode_e::relative_in_parent;
		inner_in.pos_value	 = {0.5f, 0.5f};
		inner_in.anchor_x	 = ui::anchor_e::center;
		inner_in.anchor_y	 = ui::anchor_e::center;

		const vec2f_t positions[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {0.5f, 0.5f}};
		const char*	  labels[]	  = {"-1, 1", "1, 1", "-1, -1", "1, -1", "0, 0"};

		for (u32 i = 0; i < 5; ++i)
		{
			const ui::widget_id_t label = _ui->allocate_widget();

			tree.attach(controls.blend_frame, label);

			ui::layout_in_t& in = tree.in(label);

			in.pos_mode_x = ui::pos_mode_e::relative_in_parent;
			in.pos_mode_y = ui::pos_mode_e::relative_in_parent;
			in.pos_value  = positions[i];
			in.anchor_x	  = ui::anchor_e::center;
			in.anchor_y	  = ui::anchor_e::center;
			_ui->set_widget_text(label, labels[i]);
			_ui->get_paint().set_text(label,
									  _ui->widget_text(label),
									  _ui->widget_text_len(label),
									  {
										  .font		   = theme.font_default,
										  .color	   = theme.color_text1,
										  .point_size  = theme.text_default_px_size,
										  .raster_mode = editor_text_rasterization_t::get_rasterization_type(),
									  });
		}
	}

	void editor_widget_animation_library_states_t::create_clip(state_controls_t& controls, u32 index)
	{
		clip_controls_t&			  clip		 = controls.clips[index];
		animation_library_clip_def_t& definition = _panel->_library.layers[_layer].states[controls.index].clips[index];
		ui::layout_tree_t&			  tree		 = _ui->get_tree();
		const editor_theme_t&		  theme		 = editor_theme_t::get();
		char						  label[32]	 = {};

		clip.state = &controls;
		clip.index = index;
		std::snprintf(label, sizeof(label), "Clip: %u", index + 1);
		clip.fold.init(*_ui,
					   controls.clip_list,
					   {
						   .background		 = {.fill_color_a = theme.color_frame, .fill_color_b = theme.color_frame, .rounding = theme.item_rounding, .rounding_segs = 4},
						   .label			 = label,
						   .on_pressed		 = on_clip_pressed,
						   .on_fold_changed	 = on_clip_fold_changed,
						   .user_data		 = &clip,
						   .folded			 = !_state_ui[_layer][controls.index].clip_expanded[index],
						   .background_frame = true,
					   });

		const ui::widget_id_t body = clip.fold.get_body();

		tree.in(body).child_spacing = 0.0f;

		const editor_property_row_t animation_row = editor_misc_widgets_t::make_property_row_with_label(*_ui, body, "Animation");
		u64*						animation	  = &clip.animation_value;

		clip.animation_value = definition.animation_clip;
		clip.animation.init(*_ui,
							animation_row.right,
							{
								.callbacks	= {.edited = on_clip_animation_edited, .user_data = &clip},
								.fields		= {.data = &animation, .size = 1},
								.asset_type = editor_asset_type_e::animation,
							});
		tree.in(animation_row.label).flags &= ~ui::wf_input;
		tree.in(clip.animation.get_root()).size_mode_x = ui::axis_mode_e::fill;
		init_number(body, clip.start_time, "Start Time", definition.start_time, 0.0f, FLT_MAX);
		init_number(body, clip.duration, "Duration", definition.duration, 0.0f, FLT_MAX);
		init_number(body, clip.playback_speed, "Playback Speed", definition.playback_speed, -FLT_MAX, FLT_MAX);
		clip.blend_x_row = init_number(body, clip.blend_x, "Blend X", definition.blend_position.x, -1.0f, 1.0f);
		clip.blend_y_row = init_number(body, clip.blend_y, "Blend Y", definition.blend_position.y, -1.0f, 1.0f);
		clip.diamond	 = _ui->allocate_widget();
		tree.attach(controls.blend_frame, clip.diamond);
		tree.draw_order(clip.diamond) = tree.draw_order_const(controls.blend_frame) + 1;

		ui::layout_in_t& diamond_in = tree.in(clip.diamond);

		diamond_in.flags |= ui::wf_input | ui::wf_focusable;
		diamond_in.size_mode_x = ui::axis_mode_e::fixed;
		diamond_in.size_mode_y = ui::axis_mode_e::fixed;
		diamond_in.size_value  = {theme.item_height * 0.5f, theme.item_height * 0.5f};
		diamond_in.pos_mode_x  = ui::pos_mode_e::relative_in_parent;
		diamond_in.pos_mode_y  = ui::pos_mode_e::relative_in_parent;
		diamond_in.anchor_x	   = ui::anchor_e::center;
		diamond_in.anchor_y	   = ui::anchor_e::center;
		_ui->get_paint().set_custom(clip.diamond, draw_diamond, &clip);
		_ui->get_input().set_listener(clip.diamond,
									  {
										  .on_press	   = on_diamond_pressed,
										  .on_drag	   = on_diamond_drag,
										  .on_drag_end = on_diamond_drag_end,
										  .on_focus_lose =
											  [](ui::input_router_t& router, ui::widget_id_t id, bool from_nav, void* user_data) {
												  auto& clip = *static_cast<clip_controls_t*>(user_data);

												  clip.dragging = false;
												  clip.state->owner->_panel->finish_edit();
											  },
										  .user_data = &clip,
									  });
		clip.coordinates = _ui->allocate_widget();
		tree.attach(clip.diamond, clip.coordinates);

		ui::layout_in_t& coordinate_in = tree.in(clip.coordinates);

		coordinate_in.pos_mode_x  = ui::pos_mode_e::offset_in_parent;
		coordinate_in.pos_value.x = diamond_in.size_value.x + theme.item_spacing;
		coordinate_in.pos_mode_y  = ui::pos_mode_e::relative_in_parent;
		coordinate_in.pos_value.y = 0.5f;
		coordinate_in.anchor_y	  = ui::anchor_e::center;
		++controls.clip_count;
	}

	void editor_widget_animation_library_states_t::refresh_values()
	{
		if (_layer == UINT32_MAX)
			return;

		ui::layout_tree_t&	  tree	= _ui->get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		for (const auto& controls : _controls)
		{
			const animation_library_state_def_t& state										   = _panel->_library.layers[_layer].states[controls->index];
			const bool							 blended									   = state.blend_type != animation_library_blend_type_e::no_blend;
			const bool							 blend_2d									   = state.blend_type == animation_library_blend_type_e::blend_2d;
			char								 title[sizeof(state.name) + sizeof("State: ")] = {};

			std::snprintf(title, sizeof(title), "State: %s", state.name);
			controls->fold.set_text(title);
			tree.set_visible(controls->preview, blended);

			if (state.clip_count == MAX_ANIMATION_LIBRARY_STATE_CLIPS)
				tree.in(controls->add_clip.get_root()).flags |= ui::wf_disabled;
			else
				tree.in(controls->add_clip.get_root()).flags &= ~ui::wf_disabled;

			if (state.clip_count == 0)
				tree.in(controls->clear_clips.get_root()).flags |= ui::wf_disabled;
			else
				tree.in(controls->clear_clips.get_root()).flags &= ~ui::wf_disabled;

			for (u32 i = 0; i < controls->clip_count; ++i)
			{
				clip_controls_t& clip = controls->clips[i];
				const vec2f_t	 position{math::clamp(state.clips[i].blend_position.x, -1.0f, 1.0f), blend_2d ? math::clamp(state.clips[i].blend_position.y, -1.0f, 1.0f) : 0.0f};
				char			 coordinates[64] = {};

				tree.in(clip.diamond).pos_value = {(position.x + 1.0f) * 0.5f, (1.0f - position.y) * 0.5f};
				tree.set_visible(clip.blend_x_row, blended);
				tree.set_visible(clip.blend_y_row, blend_2d);

				if (blend_2d)
					std::snprintf(coordinates, sizeof(coordinates), "%.2f, %.2f", position.x, position.y);
				else
					std::snprintf(coordinates, sizeof(coordinates), "%.2f", position.x);

				_ui->set_widget_text(clip.coordinates, coordinates);
				_ui->get_paint().set_text(clip.coordinates,
										  _ui->widget_text(clip.coordinates),
										  _ui->widget_text_len(clip.coordinates),
										  {
											  .font		   = theme.font_default,
											  .color	   = theme.color_text0,
											  .point_size  = theme.text_default_px_size,
											  .raster_mode = editor_text_rasterization_t::get_rasterization_type(),
										  });
			}
		}

		refresh_selection();
	}

	void editor_widget_animation_library_states_t::refresh_selection()
	{
		ui::layout_tree_t&	  tree	= _ui->get_tree();
		const editor_theme_t& theme = editor_theme_t::get();

		for (const auto& controls : _controls)
		{
			for (u32 i = 0; i < controls->clip_count; ++i)
			{
				const clip_controls_t& clip		= controls->clips[i];
				const bool			   selected = _panel->_selected_layer == _layer && _panel->_selected_state == controls->index && _panel->_selected_clip == i;

				_ui->get_paint().set_rect(clip.fold.get_root(),
										  {
											  .fill_color_a		 = theme.color_frame,
											  .fill_color_b		 = theme.color_frame,
											  .outline_color	 = selected ? theme.color_accent1 : theme.color_frame,
											  .rounding			 = theme.item_rounding,
											  .outline_thickness = theme.border_thickness,
											  .rounding_segs	 = 4,
										  });
				tree.set_visible(clip.coordinates, selected);
				tree.draw_order(clip.diamond)	  = tree.draw_order_const(controls->blend_frame) + (selected ? 2 : 1);
				tree.draw_order(clip.coordinates) = tree.draw_order_const(clip.diamond);
			}
		}
	}

	void editor_widget_animation_library_states_t::on_edit_begin(void* user_data)
	{
		auto& widget = *static_cast<editor_widget_animation_library_states_t*>(user_data);

		editor_command_animation_library_edit_t::begin(*widget._panel);
	}

	void editor_widget_animation_library_states_t::on_edited(void* user_data)
	{
		auto& widget = *static_cast<editor_widget_animation_library_states_t*>(user_data);

		widget.refresh_values();
	}

	void editor_widget_animation_library_states_t::on_edit_submitted(void* user_data)
	{
		auto& widget = *static_cast<editor_widget_animation_library_states_t*>(user_data);

		widget._panel->finish_edit();
		widget.refresh_values();
	}

	void editor_widget_animation_library_states_t::on_blend_edited(void* user_data)
	{
		auto& widget = *static_cast<editor_widget_animation_library_states_t*>(user_data);

		for (const auto& controls : widget._controls)
		{
			animation_library_state_def_t& state = widget._panel->_library.layers[widget._layer].states[controls->index];

			if (state.blend_type != animation_library_blend_type_e::blend_1d)
				continue;

			for (u32 i = 0; i < state.clip_count; ++i)
			{
				state.clips[i].blend_position.y = 0.0f;
				controls->clips[i].blend_y.refresh_field_data();
			}
		}

		widget.refresh_values();
	}

	void editor_widget_animation_library_states_t::on_state_fold_changed(bool folded, void* user_data)
	{
		const auto& controls = *static_cast<state_controls_t*>(user_data);

		controls.owner->_state_ui[controls.owner->_layer][controls.index].expanded = !folded;
	}

	void editor_widget_animation_library_states_t::on_clip_fold_changed(bool folded, void* user_data)
	{
		const auto& clip	 = *static_cast<clip_controls_t*>(user_data);
		const auto& controls = *clip.state;

		controls.owner->_state_ui[controls.owner->_layer][controls.index].clip_expanded[clip.index] = !folded;
	}

	void editor_widget_animation_library_states_t::on_clip_pressed(void* user_data)
	{
		const auto& clip   = *static_cast<clip_controls_t*>(user_data);
		auto&		widget = *clip.state->owner;

		widget._panel->finish_edit();
		editor_command_animation_library_edit_t::select(*widget._panel, widget._layer, clip.state->index, clip.index);
	}

	void editor_widget_animation_library_states_t::on_clip_animation_edited(void* user_data)
	{
		const auto& clip   = *static_cast<clip_controls_t*>(user_data);
		auto&		widget = *clip.state->owner;
		auto&		panel  = *widget._panel;

		panel.finish_edit();

		if (!editor_command_animation_library_edit_t::begin(panel))
			return;

		panel._library.layers[widget._layer].states[clip.state->index].clips[clip.index].animation_clip = clip.animation_value;
		editor_command_animation_library_edit_t::submit(panel, "Set Animation Clip Reference");
	}

	void editor_widget_animation_library_states_t::on_states_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left)
			return;

		auto& widget = *static_cast<editor_widget_animation_library_states_t*>(user_data);
		auto& panel	 = *widget._panel;

		panel.finish_edit();

		if (!editor_command_animation_library_edit_t::begin(panel))
			return;

		animation_library_layer_def_t& layer  = panel._library.layers[widget._layer];
		const bool					   adding = id == widget._add_state.get_root();

		if (adding)
			layer.states.emplace_back();
		else
		{
			layer.states.resize(0);
			widget._state_ui[widget._layer].resize(0);
			layer.default_active_state = UINT32_MAX;
			panel._selected_state	   = UINT32_MAX;
			panel._selected_clip	   = UINT32_MAX;
		}

		editor_command_animation_library_edit_t::submit(panel, adding ? "Add Animation State" : "Clear Animation States");
		panel._refresh_states = true;
	}

	void editor_widget_animation_library_states_t::on_clips_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left)
			return;

		auto&						   controls = *static_cast<state_controls_t*>(user_data);
		auto&						   widget	= *controls.owner;
		auto&						   panel	= *widget._panel;
		animation_library_state_def_t& state	= panel._library.layers[widget._layer].states[controls.index];
		const bool					   adding	= id == controls.add_clip.get_root();

		if (adding && state.clip_count == MAX_ANIMATION_LIBRARY_STATE_CLIPS)
			return;

		panel.finish_edit();

		if (!editor_command_animation_library_edit_t::begin(panel))
			return;

		if (adding)
		{
			state.clips[state.clip_count]													= {};
			widget._state_ui[widget._layer][controls.index].clip_expanded[state.clip_count] = false;
			++state.clip_count;
		}
		else
		{
			for (u32 i = 0; i < state.clip_count; ++i)
				state.clips[i] = {};

			state.clip_count = 0;
			std::fill_n(widget._state_ui[widget._layer][controls.index].clip_expanded, MAX_ANIMATION_LIBRARY_STATE_CLIPS, false);

			if (panel._selected_state == controls.index)
				panel._selected_clip = UINT32_MAX;
		}

		editor_command_animation_library_edit_t::submit(panel, adding ? "Add Animation Clip" : "Clear Animation Clips");
		panel._refresh_states = true;
	}

	void editor_widget_animation_library_states_t::on_diamond_pressed(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, ui::mouse_button_e button, void* user_data)
	{
		if (button != ui::mouse_button_e::left)
			return;

		auto&					clip = *static_cast<clip_controls_t*>(user_data);
		const ui::layout_out_t& out	 = clip.state->owner->_ui->get_tree().out(id);

		clip.drag_offset = pos - (out.pos + out.size * 0.5f);
		on_clip_pressed(user_data);
	}

	void editor_widget_animation_library_states_t::on_diamond_drag(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		if (router.is_pressed(ui::mouse_button_e::left) != id)
			return;

		auto&					clip   = *static_cast<clip_controls_t*>(user_data);
		auto&					widget = *clip.state->owner;
		const ui::layout_out_t& out	   = widget._ui->get_tree().out(clip.state->blend_frame);

		if (out.size.x <= 0.0f || out.size.y <= 0.0f)
			return;

		if (!clip.dragging)
		{
			if (!editor_command_animation_library_edit_t::begin(*widget._panel))
				return;

			clip.dragging = true;
		}

		animation_library_state_def_t& state	= widget._panel->_library.layers[widget._layer].states[clip.state->index];
		const vec2f_t				   position = pos - clip.drag_offset - out.pos;

		state.clips[clip.index].blend_position = {
			math::clamp(position.x / out.size.x * 2.0f - 1.0f, -1.0f, 1.0f),
			state.blend_type == animation_library_blend_type_e::blend_2d ? math::clamp(1.0f - position.y / out.size.y * 2.0f, -1.0f, 1.0f) : 0.0f,
		};
		clip.blend_x.refresh_field_data();
		clip.blend_y.refresh_field_data();
		widget.refresh_values();
	}

	void editor_widget_animation_library_states_t::on_diamond_drag_end(ui::input_router_t& router, ui::widget_id_t id, const vec2f_t& pos, const vec2f_t& delta, void* user_data)
	{
		auto& clip = *static_cast<clip_controls_t*>(user_data);

		if (!clip.dragging)
			return;

		clip.dragging = false;
		editor_command_animation_library_edit_t::submit(*clip.state->owner->_panel, "Move Animation Blend Clip");
	}

	void editor_widget_animation_library_states_t::draw_diamond(ui::paint_layer_t& paint, ui::widget_id_t id, ui::vg_canvas_t& canvas, void* user_data)
	{
		const auto&					clip	 = *static_cast<clip_controls_t*>(user_data);
		const auto&					widget	 = *clip.state->owner;
		const auto&					panel	 = *widget._panel;
		const ui::layout_tree_t&	tree	 = widget._ui->get_tree();
		const ui::layout_out_t&		out		 = tree.out(id);
		const editor_theme_t&		theme	 = editor_theme_t::get();
		const bool					selected = panel._selected_layer == widget._layer && panel._selected_state == clip.state->index && panel._selected_clip == clip.index;
		const vec4f_t&				color	 = selected ? theme.color_accent1 : theme.color_text0;
		const vec2f_t				center	 = out.pos + out.size * 0.5f;
		const vec2f_t				points[] = {{center.x, out.pos.y}, {out.pos.x + out.size.x, center.y}, {center.x, out.pos.y + out.size.y}, {out.pos.x, center.y}};
		const ui::ui_render_state_t render_state{.pipeline = paint.get_pipelines().default_pipeline};
		const f32					scale = ui::get_valid_scale(widget._ui->get_ui_scale());

		canvas.push_clip(out.clip, ui::clip_mode_e::scissor_rect);
		canvas.add_convex({points, 4}, {.fill_color_a = color, .fill_color_b = color, .aa_thickness = theme.aa_thickness * scale}, render_state, tree.draw_order_const(id));
		canvas.pop_clip(ui::clip_mode_e::scissor_rect);
	}
}
