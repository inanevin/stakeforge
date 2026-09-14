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

#include <sfg/math/mat4x3.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/memory/dynamic_gen_pool.hpp>
#include <sfg/memory/pool_handle.hpp>
#include <sfg/memory/chunk_allocator.hpp>
#include <sfg/common/size_definitions.hpp>
#include <sfg/runtime/world/ecs_defs.hpp>
#include <sfg/runtime/resources/common_resources.hpp>
#include <sfg/runtime/animation/common_animation.hpp>

namespace sfg
{
	class world_t;

	// tags
	struct animator_state_tag_t
	{
	};

	struct animator_library_tag_t
	{
	};

	typedef pool_handle_t<u32, animator_state_tag_t>   animator_state_handle_t;
	typedef pool_handle_t<u32, animator_library_tag_t> animator_library_handle_t;

	struct animator_clip_t
	{
		resource_handle_t clip_handle	 = NULL_RESOURCE_HANDLE;
		vec2f_t			  blend_position = vec2f_t::zero;
		f32				  speed			 = 1.0f;
		f32				  start_time	 = 0.0f;
	};

	struct animator_state_switch_t
	{
		animator_state_handle_t target_state = {};
		float					duration	 = 0.0f;
		float					current_time = 0.0f;
		bool					active		 = false;
	};

	struct animator_state_t
	{
		animator_clip_t				   clips[MAX_ANIMATION_LIBRARY_STATE_CLIPS];
		chunk_handle32_t			   delaunay_triangles	= {};
		vec2f_t						   blend_position_value = vec2f_t::zero;
		sid_t						   name_hash			= NULL_SID;
		u32							   triangle_count		= 0;
		u32							   layer_index			= 0;
		u32							   clip_count			= 0;
		f32							   speed				= 0.0f;
		f32							   current_phase		= 0.0f;
		animation_library_blend_type_e blend_type			= animation_library_blend_type_e::no_blend;
		bool						   loop					= false;
	};

	struct animator_pose_t
	{
		mat4x3_t		bone[MAX_SKELETON_BONES];
		skeleton_mask_t write_mask = {};
	};

	struct animator_layer_t
	{
		animator_state_switch_t current_switch = {};
		sid_t					name_hash	   = NULL_SID;
		skeleton_mask_t			mask		   = {};
		chunk_handle32_t		state_handles  = {};
		animator_state_handle_t active_state   = {};
		u32						state_count	   = 0;
		f32						weight		   = 0.0f;
	};

	struct animator_library_t
	{
		resource_handle_t skeleton_handle = NULL_RESOURCE_HANDLE;
		animator_layer_t  layers[6]		  = {};
		u32				  layer_count	  = 0;
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
		void calculate_skinning_matrices(f32 dt);
		void destroy_entity(entity_id_t id);

		void					switch_layer_state(animator_library_handle_t library, u32 layer_index, animator_state_handle_t state, f32 transition_duration);
		animator_state_handle_t find_state_handle(animator_library_handle_t library, sid_t name_hash, u32 layer = UINT32_MAX);

	private:
		struct process_state_params_t
		{
			animator_state_t&	   state;
			decomposed_bone_t*	   decomposed;
			decomposed_bone_t*	   scratch;
			const skeleton_mask_t& mask;
			skeleton_mask_t&	   out_position_mask;
			skeleton_mask_t&	   out_rotation_mask;
			skeleton_mask_t&	   out_scale_mask;
			u32					   joint_count;
			f32					   dt;
			bool				   sample_animation;
		};

		struct blend_decomposed_params_t
		{
			decomposed_bone_t*		 store;
			const decomposed_bone_t* target;
			const skeleton_mask_t&	 target_position_writes;
			const skeleton_mask_t&	 target_rotation_writes;
			const skeleton_mask_t&	 target_scale_writes;
			u32						 joint_count;
			f32						 blend;
		};

		void alloc_for_entity(entity_id_t id);
		void dealloc_for_entity(entity_id_t id);
		void process_state(const process_state_params_t& params);
		u32	 get_count_for_lib_alloc(u32 skeleton_joint_count);
		void blend_decomposed(const blend_decomposed_params_t& params);

	private:
		world_t*															_world			   = nullptr;
		chunk_allocator_t													_aux			   = {};
		chunk_allocator_t													_bone_aux		   = {};
		chunk_allocator_t													_decomposition_aux = {};
		dynamic_gen_pool_t<animator_state_t, u32, animator_state_tag_t>		_states;
		dynamic_gen_pool_t<animator_library_t, u32, animator_library_tag_t> _libraries;
		u32																	_frame_counter = 0;
	};
}
