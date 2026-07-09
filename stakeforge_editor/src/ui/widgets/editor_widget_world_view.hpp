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

#include "world/editor_world_handle.hpp"
#include <sfg/math/vec2u16.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg
{
	struct editor_payload_t;
	class world_render_context_t;
	namespace ui
	{
		class ui_context;
	}

	class editor_widget_world_view_t final
	{
	public:
		editor_widget_world_view_t()											 = default;
		~editor_widget_world_view_t()											 = default;
		editor_widget_world_view_t(const editor_widget_world_view_t&)			 = delete;
		editor_widget_world_view_t& operator=(const editor_widget_world_view_t&) = delete;

		void	init(ui::ui_context& ui, ui::widget_id_t parent);
		void	uninit();
		void	set_edit_world(editor_world_handle_t world);
		vec4f_t get_world_view_bounds() const;

		inline ui::widget_id_t get_root() const
		{
			return _root;
		}

	private:
		void clear_world();
		void refresh_world_texture();
		void request_world_resize(bool force);

		static void on_world_view_tick(ui::ui_context& ui, ui::widget_id_t id, f32 dt_seconds, void* user_data);
		static bool on_payload_drop(const editor_payload_t& payload, void* user_data);

	private:
		const world_render_context_t* _world			   = nullptr;
		ui::ui_context*				  _ui				   = nullptr;
		editor_world_handle_t		  _edit_world		   = {};
		vec2u16_t					  _last_resize_request = vec2u16_t::zero;
		ui::widget_id_t				  _root				   = NULL_WIDGET;
		ui::widget_id_t				  _world_view		   = NULL_WIDGET;
		ui::widget_id_t				  _empty_label		   = NULL_WIDGET;
		u8							  _resize_ticks		   = 0;
	};
}
