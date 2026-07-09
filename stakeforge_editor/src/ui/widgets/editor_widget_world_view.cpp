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
#include "ui/widgets/editor_widget_world_view.hpp"
#include "ui/editor_payload_controller.hpp"
#include "editor_world_controller.hpp"
#include "world/editor_world.hpp"
#include "ui/editor_global_toolbar.hpp"
#include "ui/editor_text_rasterization.hpp"
#include "ui/panels/editor_theme.hpp"
#include "ui/widgets/editor_widgets_icons.hpp"
#include "assets/editor_asset_spawn.hpp"
#include <sfg/math/math.hpp>
#include <sfg/math/rectf.hpp>
#include <sfg/runtime/ui/ui_context.hpp>

namespace sfg
{
	namespace
	{
		ui::ui_resource_ref_t make_null_world_texture_ref()
		{
			ui::ui_resource_ref_t ref = {
				.handle = NULL_RESOURCE_HANDLE,
				.type	= ui::ui_resource_type_e::gpu_index_fof,
			};
			for (u8 i = 0; i < BACK_BUFFER_COUNT; ++i)
				ref.gpu_indices[i] = NULL_GPU_INDEX;
			return ref;
		}

		gpu_index_t get_world_view_gpu_index(const world_render_context_t& world, editor_main_toolbar_world_view_e view, u8 frame_index)
		{
			switch (view)
			{
			case editor_main_toolbar_world_view_e::gbuffer_albedo:
				return world.get_gbuffer_albedo_index(frame_index);
			case editor_main_toolbar_world_view_e::gbuffer_orm:
				return world.get_gbuffer_orm_index(frame_index);
			case editor_main_toolbar_world_view_e::gbuffer_normal:
				return world.get_gbuffer_normal_index(frame_index);
			case editor_main_toolbar_world_view_e::gbuffer_emissive:
				return world.get_gbuffer_emissive_index(frame_index);
			case editor_main_toolbar_world_view_e::lighting:
				return world.get_lighting_texture_index(frame_index);
			case editor_main_toolbar_world_view_e::post_process:
				return world.get_post_process_texture_index(frame_index);
			case editor_main_toolbar_world_view_e::final:
			default:
				return world.get_world_texture_index(frame_index);
			}
		}
	}

	void editor_widget_world_view_t::init(ui::ui_context& ui, ui::widget_id_t parent)
	{
		_ui	  = &ui;
		_root = parent;

		ui::layout_tree_t&	  tree	= ui.get_tree();
		ui::paint_layer_t&	  paint = ui.get_paint();
		const editor_theme_t& theme = editor_theme_t::get();

		_world_view = ui.allocate_widget();
		ui.set_widget_debug_name(_world_view, "world_view");
		tree.attach(_root, _world_view);

		ui::layout_in_t& in = tree.in(_world_view);
		in.size_mode_x		= ui::axis_mode_e::parent_relative;
		in.size_mode_y		= ui::axis_mode_e::parent_relative;
		in.size_value		= {1.0f, 1.0f};

		ui::vg_rect_paint_t rect = {};
		rect.fill_color_a		 = {1.0f, 1.0f, 1.0f, 1.0f};
		rect.fill_color_b		 = rect.fill_color_a;
		rect.filled				 = true;

		ui::ui_render_state_t state = {};
		state.pipeline				= "editor/resource_pack/shaders/editor_ui_texture.hlsl"_hs;
		state.constants[0]			= make_null_world_texture_ref();

		paint.set_rect(_world_view, rect, state);
		ui.set_pre_layout_tick(_world_view, on_world_view_tick, this);

		_empty_label = ui.allocate_widget();
		ui.set_widget_debug_name(_empty_label, "empty_label");
		tree.attach(_root, _empty_label);

		ui::layout_in_t& label_in = tree.in(_empty_label);
		label_in.flags			  = ui::wf_visible;
		label_in.pos_mode_x		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_mode_y		  = ui::pos_mode_e::relative_in_parent;
		label_in.pos_value		  = {0.5f, 0.5f};
		label_in.anchor_x		  = ui::anchor_e::center;
		label_in.anchor_y		  = ui::anchor_e::center;

		ui.set_widget_text(_empty_label, "NO WORLD AVAILABLE");
		paint.set_text(_empty_label,
					   ui.widget_text(_empty_label),
					   ui.widget_text_len(_empty_label),
					   {.font = theme.font_title_bold, .color = theme.color_text2, .point_size = theme.text_big_px_size, .spacing = 0, .raster_mode = editor_text_rasterization_t::get_rasterization_type()});

		editor_payload_controller_t::get().register_listener(on_payload_drop, nullptr, nullptr, this);
		clear_world();
	}

	void editor_widget_world_view_t::uninit()
	{
		editor_payload_controller_t::get().unregister_listener(this);
		_ui->deallocate_widget(_empty_label);
		_ui->deallocate_widget(_world_view);
		_world				 = nullptr;
		_edit_world			 = {};
		_last_resize_request = vec2u16_t::zero;
		_empty_label		 = NULL_WIDGET;
		_world_view			 = NULL_WIDGET;
		_root				 = NULL_WIDGET;
		_resize_ticks		 = 0;
		_ui					 = nullptr;
	}

	void editor_widget_world_view_t::set_edit_world(editor_world_handle_t world)
	{
		_edit_world = world;
		SFG_ASSERT(_ui != nullptr);
		SFG_ASSERT(_world_view != NULL_WIDGET);

		if (_edit_world.is_null())
		{
			clear_world();
			return;
		}

		editor_world_controller_t& controller = editor_world_controller_t::get();
		_world								  = &controller.get_editor_world(_edit_world)->get_render_context();
		request_world_resize(true);
		refresh_world_texture();

		ui::layout_tree_t& tree = _ui->get_tree();
		tree.set_visible(_world_view, true);
		tree.set_visible(_empty_label, false);
	}

	vec4f_t editor_widget_world_view_t::get_world_view_bounds() const
	{
		SFG_ASSERT(_ui != nullptr);
		SFG_ASSERT(_world_view != NULL_WIDGET);
		return _ui->get_tree().bounds(_world_view);
	}

	void editor_widget_world_view_t::clear_world()
	{
		SFG_ASSERT(_ui != nullptr);
		SFG_ASSERT(_world_view != NULL_WIDGET);

		ui::paint_def_t& def		  = _ui->get_paint().def(_world_view);
		def.render_state.pipeline	  = "editor/resource_pack/shaders/editor_ui_texture.hlsl"_hs;
		def.render_state.constants[0] = make_null_world_texture_ref();
		_world						  = nullptr;

		ui::layout_tree_t& tree = _ui->get_tree();
		tree.set_visible(_world_view, false);
		tree.set_visible(_empty_label, true);
	}

	void editor_widget_world_view_t::request_world_resize(bool force)
	{
		if (_edit_world.is_null())
			return;

		const ui::layout_out_t& out = _ui->get_tree().out(_world_view);
		const vec2u16_t			resolution{
			.x = static_cast<u16>(math::clamp(out.size.x, 1.0f, 65535.0f) + 0.5f),
			.y = static_cast<u16>(math::clamp(out.size.y, 1.0f, 65535.0f) + 0.5f),
		};
		if (!force && _last_resize_request == resolution)
			return;

		_last_resize_request = resolution;
		editor_world_controller_t::get().resize_world(_edit_world, resolution);
	}

	void editor_widget_world_view_t::refresh_world_texture()
	{
		SFG_ASSERT(_ui != nullptr);
		SFG_ASSERT(_world_view != NULL_WIDGET);
		SFG_ASSERT(_world != nullptr);

		ui::ui_resource_ref_t texture_ref = {
			.handle = NULL_RESOURCE_HANDLE,
			.type	= ui::ui_resource_type_e::gpu_index_fof,
		};
		const editor_main_toolbar_world_view_e world_view = editor_global_toolbar_t::get().get_world_view();
		for (u8 i = 0; i < BACK_BUFFER_COUNT; ++i)
			texture_ref.gpu_indices[i] = get_world_view_gpu_index(*_world, world_view, i);

		ui::paint_def_t& def		  = _ui->get_paint().def(_world_view);
		def.render_state.pipeline	  = "editor/resource_pack/shaders/editor_ui_texture.hlsl"_hs;
		def.render_state.constants[0] = texture_ref;
	}

	void editor_widget_world_view_t::on_world_view_tick(ui::ui_context&, ui::widget_id_t, f32, void* user_data)
	{
		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);
		if (widget._world != nullptr)
		{
			++widget._resize_ticks;
			if (widget._resize_ticks >= 5)
			{
				widget._resize_ticks = 0;
				widget.request_world_resize(false);
			}
			widget.refresh_world_texture();
		}
	}

	bool editor_widget_world_view_t::on_payload_drop(const editor_payload_t& payload, void* user_data)
	{
		if (payload.type != editor_payload_type_e::asset && payload.type != editor_payload_type_e::asset_multi)
			return false;
		SFG_ASSERT(payload.user_ptr != nullptr);

		editor_widget_world_view_t& widget = *static_cast<editor_widget_world_view_t*>(user_data);
		if (widget._edit_world.is_null())
			return false;

		const ui::layout_out_t& out	  = widget._ui->get_tree().out(widget._world_view);
		const vec2f_t&			mouse = widget._ui->get_input().get_mouse_position();
		if (!rectf_t{out.pos.x, out.pos.y, out.size.x, out.size.y}.contains(mouse))
			return false;

		return editor_asset_spawn_t::spawn_from_payload({
			.payload	= &payload,
			.screen_pos = mouse,
			.world		= widget._edit_world,
			.parent		= NULL_ENTITY_ID,
		});
	}
}
