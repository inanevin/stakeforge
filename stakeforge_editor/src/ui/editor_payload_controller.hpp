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

#include "ui/editor_payload_type.hpp"
#include <sfg/data/string.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/math/vec2i16.hpp>
#include <sfg/math/vec2u16.hpp>
#include <sfg/runtime/engine/common_engine.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/runtime/ui/ui_common.hpp>

namespace sfg
{
	struct editor_surface_t;
	struct window_runtime_t;
	namespace ui
	{
		class ui_context;
	}

	struct editor_payload_t
	{
		const char*			  text		 = nullptr;
		void*				  user_ptr	 = nullptr;
		vec2i16_t			  pos		 = vec2i16_t::zero;
		vec2u16_t			  size_value = {};
		editor_payload_type_e type		 = editor_payload_type_e::panel;
	};

	struct editor_entity_payload_t
	{
		world_handle_t world  = {};
		entity_id_t	   entity = NULL_ENTITY_ID;
	};

	using editor_payload_listener_fn  = bool (*)(const editor_payload_t& payload, void* user_data);
	using editor_payload_tick_fn	  = void (*)(const editor_payload_t& payload, const vec2i16_t& abs_mouse_pos, void* user_data);
	using editor_payload_end_fn		  = void (*)(const editor_payload_t& payload, void* user_data);
	using editor_payload_unhandled_fn = void (*)(const editor_payload_t& payload, void* user_data);

	class editor_payload_controller_t final
	{
	public:
		editor_payload_controller_t()											   = default;
		~editor_payload_controller_t()											   = default;
		editor_payload_controller_t(const editor_payload_controller_t&)			   = delete;
		editor_payload_controller_t& operator=(const editor_payload_controller_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(editor_surface_t& surface);
		void uninit();
		void tick();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void create_payload(const char* text, editor_payload_type_e type, void* user_ptr, vec2u16_t size_value = {});
		void register_listener(editor_payload_listener_fn fn, editor_payload_tick_fn tick_fn, editor_payload_end_fn end_fn, void* user_data);
		void unregister_listener(void* user_data);
		void set_unhandled_listener(editor_payload_unhandled_fn fn, void* user_data);
		bool drop_payload();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline bool is_payload_active() const
		{
			return _active;
		}

		static editor_payload_controller_t& get();

	private:
		struct listener_t
		{
			editor_payload_listener_fn fn		 = nullptr;
			editor_payload_tick_fn	   tick_fn	 = nullptr;
			editor_payload_end_fn	   end_fn	 = nullptr;
			void*					   user_data = nullptr;
		};

		bool			 is_any_mouse_down() const;
		editor_payload_t make_payload(const vec2i16_t& pos) const;
		void			 tick_listeners(const editor_payload_t& payload, const vec2i16_t& pos);
		void			 end_listeners(const editor_payload_t& payload);
		void			 set_visible(bool visible);
		void			 follow_cursor();

	private:
		window_runtime_t*			_runtime			 = nullptr;
		ui::ui_context*				_ui					 = nullptr;
		vector_t<listener_t>		_listeners			 = {};
		string_t					_text				 = {};
		void*						_user_ptr			 = nullptr;
		void*						_unhandled_user_data = nullptr;
		editor_payload_unhandled_fn _unhandled_fn		 = nullptr;
		vec2u16_t					_size_value			 = {};
		editor_payload_type_e		_type				 = editor_payload_type_e::panel;
		ui::widget_id_t				_frame				 = NULL_WIDGET;
		ui::widget_id_t				_text_widget		 = NULL_WIDGET;
		bool						_active				 = false;
		bool						_mouse_was_down		 = false;
	};
}
