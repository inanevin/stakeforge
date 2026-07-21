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

#pragma once

#include "ui/panels/editor_panel.hpp"
#include "ui/widgets/editor_split_border.hpp"
#include "ui/widgets/editor_widget_world_view.hpp"
#include "world/editor_world_handle.hpp"

#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/mat4x3.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class world_t;

	class editor_panel_skeleton_viewer_t final : public editor_panel_t
	{
	public:
		editor_panel_skeleton_viewer_t();
		~editor_panel_skeleton_viewer_t() override										 = default;
		editor_panel_skeleton_viewer_t(const editor_panel_skeleton_viewer_t&)			 = delete;
		editor_panel_skeleton_viewer_t& operator=(const editor_panel_skeleton_viewer_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void serialize(nlohmann::json& j) const override;
		void deserialize(const nlohmann::json& j) override;
		void init(ui::ui_context& ui, ui::widget_id_t parent) override;
		void uninit() override;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void set_skeleton(sid_t skeleton_guid, const char* asset_name);

	private:
		struct joint_draw_data_t
		{
			mat4x3_t transform	  = mat4x3_t::identity;
			u32		 parent_index = UINT32_MAX;
		};

		void			create_preview_world();
		void			destroy_preview_world();
		void			create_environment();
		void			draw_skeleton(world_t& world) const;
		void			refresh_info();
		void			apply_pane_split();
		ui::widget_id_t append_property_value_row(const char* label);
		ui::widget_id_t append_value_label(ui::widget_id_t parent);

		static void on_world_tick(world_t& world, f32 delta_time, void* user_data);
		static void on_split_border_drag(editor_split_border_t& border, const vec2f_t& pos, const vec2f_t& delta, void* user_data);

	private:
		editor_widget_world_view_t	_world_view			= {};
		editor_split_border_t		_split_border		= {};
		vector_t<joint_draw_data_t> _joint_draw_data	= {};
		string_t					_asset_name			= {};
		string_t					_joint_count_text	= {};
		string_t					_root_joint_text	= {};
		editor_world_handle_t		_world				= {};
		sid_t						_skeleton_guid		= 0;
		u32							_root_joint_index	= UINT32_MAX;
		entity_id_t					_environment_entity = NULL_ENTITY_ID;
		ui::widget_id_t				_left_pane			= NULL_WIDGET;
		ui::widget_id_t				_right_pane			= NULL_WIDGET;
		ui::widget_id_t				_joint_count_value	= NULL_WIDGET;
		ui::widget_id_t				_root_joint_value	= NULL_WIDGET;
		f32							_joint_radius		= 0.025f;
		f32							_axis_length		= 0.15f;
		f32							_pane_split			= 0.72f;
	};
}
