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

#pragma once

#include <sfg/memory/dynamic_gen_pool.hpp>
#include <sfg/memory/pool_handle.hpp>
#include <sfg/memory/chunk_allocator.hpp>
#include <sfg/common/size_definitions.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/runtime/resources/common_resources.hpp>

namespace sfg
{
	class world_t;

	// tags
	struct animator_state_tag_t
	{
	};
	struct animator_pose_tag_t
	{
	};
	struct animator_library_tag_t
	{
	};
	struct animator_layer_tag_t
	{
	};

	typedef pool_handle_t<u32, animator_state_tag_t>   animator_state_handle_t;
	typedef pool_handle_t<u32, animator_pose_tag_t>	   animator_pose_handle_t;
	typedef pool_handle_t<u32, animator_layer_tag_t>   animator_layer_handle_t;
	typedef pool_handle_t<u32, animator_library_tag_t> animator_library_handle_t;

	struct animator_state_switch_t
	{
		animator_state_handle_t target_state = {};
		float					duration	 = 0.0f;
		bool					active		 = false;
	};

	struct animator_library_t
	{
		resource_handle_t		skeleton_handle = NULL_RESOURCE_HANDLE;
		animator_state_switch_t current_switch	= {};
		animator_layer_handle_t first_layer		= {};
		u32						layer_count		= 0;
	};

	struct animator_state_t
	{
		animator_layer_handle_t layer	   = {};
		animator_state_handle_t next_state = {};
	};

	struct animator_pose_t
	{
	};

	struct animator_layer_t
	{
		animator_layer_handle_t next_layer	 = {};
		skeleton_mask_t			mask		 = {};
		animator_state_handle_t active_state = {};
		animator_state_handle_t first_state	 = {};
		u32						state_count	 = 0;
	};

	class animation_processor_t final
	{
	public:
		animation_processor_t()												 = default;
		~animation_processor_t()											 = default;
		animation_processor_t(const animation_processor_t& other)			 = delete;
		animation_processor_t& operator=(const animation_processor_t& other) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(world_t& world, size_t aux_size, size_t max_library_support);
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void tick(f32 dt);

	private:
		void alloc_for_entity(entity_id_t id);
		void dealloc_for_entity(entity_id_t id);

	private:
		world_t*														  _world = nullptr;
		chunk_allocator_t												  _aux	 = {};
		dynamic_gen_pool_t<animator_state_t, u32, animator_state_tag_t>	  _states;
		dynamic_gen_pool_t<animator_state_t, u32, animator_library_tag_t> _libraries;
		dynamic_gen_pool_t<animator_pose_t, u32, animator_pose_tag_t>	  _poses;
	};
}
