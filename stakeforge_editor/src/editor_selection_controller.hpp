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

#include <sfg/data/span.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/memory/dynamic_gen_pool.hpp>
#include <sfg/runtime/engine/common_engine.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>

namespace sfg
{
	class editor_selection_controller_t;
	struct editor_selection_listener_tag_t;

	using editor_selection_listener_handle_t = pool_handle_t<u32, editor_selection_listener_tag_t>;
	using editor_selection_listener_fn		 = void (*)(editor_selection_controller_t& controller, void* user_data);

	struct editor_selection_listener_t
	{
		editor_selection_listener_fn fn		   = nullptr;
		void*						 user_data = nullptr;
	};

	class editor_selection_controller_t final
	{
	public:
		editor_selection_controller_t()												   = default;
		~editor_selection_controller_t()											   = default;
		editor_selection_controller_t(const editor_selection_controller_t&)			   = delete;
		editor_selection_controller_t& operator=(const editor_selection_controller_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init();
		void uninit();
		void clear();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void							   set_world(world_handle_t world);
		void							   issue_entity_selection(span_t<const entity_id_t> entities, entity_id_t anchor);
		void							   apply_entity_selection(span_t<const entity_id_t> entities, entity_id_t anchor);
		void							   clear_entity_selection();
		editor_selection_listener_handle_t add_listener(editor_selection_listener_fn fn, void* user_data);
		void							   remove_listener(editor_selection_listener_handle_t handle);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		span_t<const entity_id_t> get_selected_entities() const;
		world_handle_t			  get_world() const;
		entity_id_t				  get_entity_anchor() const;
		u32						  get_generation() const;

	private:
		void notify_listeners();

	private:
		dynamic_gen_pool_t<editor_selection_listener_t, u32, editor_selection_listener_tag_t> _listeners;
		vector_t<entity_id_t>																  _selected_entities = {};
		world_handle_t																		  _world			 = {};
		entity_id_t																			  _entity_anchor	 = NULL_ENTITY_ID;
		u32																					  _generation		 = 0;
		bool																				  _inited			 = false;
	};
}
