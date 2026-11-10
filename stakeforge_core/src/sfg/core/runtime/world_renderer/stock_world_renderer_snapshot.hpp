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

#include "stock_world_renderer_draw.hpp"
#include "core/runtime/world/world_debug_render_snapshot.hpp"
#include "core/runtime/world/world_canvas_render_snapshot.hpp"
#include "stock_world_renderer_bone.hpp"
#include "stock_world_renderer_entity.hpp"
#include "stock_world_renderer_light.hpp"
#include "stock_world_renderer_reflection_probe.hpp"
#include "stock_world_renderer_shadow.hpp"
#include "stock_world_renderer_material.hpp"
#include "world_renderer_camera.hpp"
#include "core/data/frame_vector.hpp"
#include "core/data/inplace_vector.hpp"
#include "core/data/span.hpp"
#include "core/data/fixed_vector.hpp"
#include "core/math/frustum.hpp"
#include "core/math/mat4x4.hpp"
#include "core/math/vec2f.hpp"
#include "core/math/vec4f.hpp"
#include "core/runtime/render/render_resource_handle.hpp"
#include "core/gfx/common/gfx_constants.hpp"
#include "core/runtime/project/project_settings.hpp"

namespace sfg
{
	struct world_render_context_config_t;
	struct world_debug_draw_config_t;

	enum class stock_world_renderer_fog_type_e : u32
	{
		linear,
		exponential,
		exponential_squared,
		exponential_height,
	};

	struct stock_world_renderer_fog_t
	{
		vec4f_t							color		   = vec4f_t::zero;
		f32								intensity	   = 0.0f;
		f32								density		   = 0.01f;
		f32								start_distance = 0.0f;
		f32								end_distance   = 100.0f;
		f32								height		   = 0.0f;
		f32								height_falloff = 0.1f;
		f32								max_opacity	   = 1.0f;
		stock_world_renderer_fog_type_e type		   = stock_world_renderer_fog_type_e::exponential;
	};

	struct stock_world_renderer_environment_t
	{
		vec4f_t ambient_color		  = vec4f_t::zero;
		u32		material_index		  = UINT32_MAX;
		f32		intensity			  = 1.0f;
		u8		debug_cluster_heatmap = 0;
	};

	struct stock_world_renderer_ssao_t
	{
		f32 radius_world			 = 0.75f;
		f32 bias					 = 0.04f;
		f32 intensity				 = 1.25f;
		f32 power					 = 1.25f;
		f32 random_rotation_strength = 1.5f;
		u32 direction_count			 = 8;
		u32 step_count				 = 6;
		u8	enabled					 = 0;
	};

	struct stock_world_renderer_bloom_t
	{
		f32 strength	  = 0.04f;
		f32 filter_radius = 0.01f;
		u8	enabled		  = 0;
	};

	struct stock_world_renderer_fxaa_t
	{
		f32 fixed_threshold	   = 0.0833f;
		f32 relative_threshold = 0.166f;
		f32 subpixel_blending  = 0.75f;
		u8	enabled			   = 0;
	};

	struct stock_world_renderer_post_process_t
	{
		stock_world_renderer_ssao_t	 ssao				  = {};
		stock_world_renderer_fxaa_t	 fxaa				  = {};
		stock_world_renderer_bloom_t bloom				  = {};
		f32							 exposure_ev		  = 0.0f;
		f32							 saturation			  = 1.0f;
		f32							 temperature		  = 0.0f;
		f32							 tint				  = 0.0f;
		f32							 reinhard_white_point = 6.0f;
		u32							 tonemap_mode		  = 1;
	};

	struct stock_world_renderer_queue_range_t
	{
		u32 offset = 0;
		u32 count  = 0;
	};

	struct stock_world_renderer_prep_view_t
	{
		mat4x4_t						   view								   = mat4x4_t::identity;
		mat4x4_t						   view_proj						   = mat4x4_t::identity;
		mat4x4_t						   inv_view							   = mat4x4_t::identity;
		mat4x4_t						   inv_view_proj					   = mat4x4_t::identity;
		frustum_t						   frustum							   = {};
		vec4f_t							   camera_pos						   = vec4f_t::zero;
		vec4f_t							   cluster_depth					   = vec4f_t::zero;
		u32								   cluster_dims[4]					   = {};
		vec2f_t							   viewport_size					   = vec2f_t::zero;
		vec2f_t							   inv_viewport_size				   = vec2f_t::zero;
		f32								   near_plane						   = 0.0f;
		f32								   far_plane						   = 0.0f;
		gpu_index_t						   depth_texture_index				   = NULL_GPU_INDEX;
		u32								   cluster_buffer_offset			   = 0;
		u32								   cluster_light_indices_buffer_offset = 0;
		u32								   cluster_light_capacity			   = 0;
		u32								   queue_flags						   = 0;
		stock_world_renderer_queue_range_t depth_range						   = {};
		stock_world_renderer_queue_range_t opaque_range						   = {};
		stock_world_renderer_queue_range_t transparent_range				   = {};
		stock_world_renderer_queue_range_t shadow_range						   = {};
		stock_world_renderer_queue_range_t visible_range					   = {};
	};

	enum stock_world_renderer_queue_flags_e : u32
	{
		stock_world_renderer_queue_flag_depth		= 1 << 0,
		stock_world_renderer_queue_flag_opaque		= 1 << 1,
		stock_world_renderer_queue_flag_transparent = 1 << 2,
		stock_world_renderer_queue_flag_shadow		= 1 << 3,
		stock_world_renderer_queue_flag_visible		= 1 << 4,
	};

	struct stock_world_renderer_queue_item_t
	{
		f32 depth			 = 0.0f;
		u32 renderable_index = UINT32_MAX;
		u32 instance_index	 = UINT32_MAX;
	};

	struct stock_world_renderer_prep_data_t
	{
		frame_vector_t<stock_world_renderer_prep_view_t>   views				  = {};
		frame_vector_t<stock_world_renderer_queue_item_t>  depth_queue			  = {};
		frame_vector_t<stock_world_renderer_queue_item_t>  opaque_queue			  = {};
		frame_vector_t<stock_world_renderer_queue_item_t>  transparent_queue	  = {};
		frame_vector_t<stock_world_renderer_queue_item_t>  shadow_queue			  = {};
		frame_vector_t<stock_world_renderer_queue_item_t>  visible_queue		  = {};
		frame_vector_t<stock_world_renderer_queue_item_t>  view_model_depth_queue = {};
		frame_vector_t<stock_world_renderer_queue_item_t>  view_model_queue		  = {};
		frame_vector_t<stock_world_renderer_shadow_view_t> shadow_views			  = {};
		u32												   reflection_probe_count = 0;

		u32 visible_renderables_count = 0;

		inline void reserve()
		{
			views.reserve(16);
			depth_queue.reserve(16);
			opaque_queue.reserve(16);
			transparent_queue.reserve(16);
			shadow_queue.reserve(16);
			visible_queue.reserve(16);
			view_model_depth_queue.reserve(16);
			view_model_queue.reserve(16);
			shadow_views.reserve(16);
		}

		inline u16 add_view(const stock_world_renderer_prep_view_t& view)
		{
			const u16 index = static_cast<u16>(views.size());
			views.push_back(view);
			return index;
		}

		inline void begin_view_queues(stock_world_renderer_prep_view_t& view) const
		{
			view.depth_range	   = {.offset = static_cast<u32>(depth_queue.size())};
			view.opaque_range	   = {.offset = static_cast<u32>(opaque_queue.size())};
			view.transparent_range = {.offset = static_cast<u32>(transparent_queue.size())};
			view.shadow_range	   = {.offset = static_cast<u32>(shadow_queue.size())};
			view.visible_range	   = {.offset = static_cast<u32>(visible_queue.size())};
		}

		inline void end_view_queues(stock_world_renderer_prep_view_t& view) const
		{
			view.depth_range.count		 = static_cast<u32>(depth_queue.size()) - view.depth_range.offset;
			view.opaque_range.count		 = static_cast<u32>(opaque_queue.size()) - view.opaque_range.offset;
			view.transparent_range.count = static_cast<u32>(transparent_queue.size()) - view.transparent_range.offset;
			view.shadow_range.count		 = static_cast<u32>(shadow_queue.size()) - view.shadow_range.offset;
			view.visible_range.count	 = static_cast<u32>(visible_queue.size()) - view.visible_range.offset;
		}

		inline span_t<const stock_world_renderer_queue_item_t> get_depth_queue(const stock_world_renderer_prep_view_t& view) const
		{
			return {.data = depth_queue.data() + view.depth_range.offset, .size = view.depth_range.count};
		}

		inline span_t<const stock_world_renderer_queue_item_t> get_opaque_queue(const stock_world_renderer_prep_view_t& view) const
		{
			return {.data = opaque_queue.data() + view.opaque_range.offset, .size = view.opaque_range.count};
		}

		inline span_t<const stock_world_renderer_queue_item_t> get_transparent_queue(const stock_world_renderer_prep_view_t& view) const
		{
			return {.data = transparent_queue.data() + view.transparent_range.offset, .size = view.transparent_range.count};
		}

		inline span_t<const stock_world_renderer_queue_item_t> get_shadow_queue(const stock_world_renderer_prep_view_t& view) const
		{
			return {.data = shadow_queue.data() + view.shadow_range.offset, .size = view.shadow_range.count};
		}

		inline span_t<const stock_world_renderer_queue_item_t> get_visible_queue(const stock_world_renderer_prep_view_t& view) const
		{
			return {.data = visible_queue.data() + view.visible_range.offset, .size = view.visible_range.count};
		}

		inline span_t<const stock_world_renderer_queue_item_t> get_view_model_depth_queue() const
		{
			return {.data = view_model_depth_queue.data(), .size = view_model_depth_queue.size()};
		}

		inline span_t<const stock_world_renderer_queue_item_t> get_view_model_queue() const
		{
			return {.data = view_model_queue.data(), .size = view_model_queue.size()};
		}

		inline span_t<stock_world_renderer_queue_item_t> get_mutable_depth_queue(const stock_world_renderer_prep_view_t& view)
		{
			return {.data = depth_queue.data() + view.depth_range.offset, .size = view.depth_range.count};
		}

		inline span_t<stock_world_renderer_queue_item_t> get_mutable_opaque_queue(const stock_world_renderer_prep_view_t& view)
		{
			return {.data = opaque_queue.data() + view.opaque_range.offset, .size = view.opaque_range.count};
		}

		inline span_t<stock_world_renderer_queue_item_t> get_mutable_transparent_queue(const stock_world_renderer_prep_view_t& view)
		{
			return {.data = transparent_queue.data() + view.transparent_range.offset, .size = view.transparent_range.count};
		}

		inline span_t<stock_world_renderer_queue_item_t> get_mutable_shadow_queue(const stock_world_renderer_prep_view_t& view)
		{
			return {.data = shadow_queue.data() + view.shadow_range.offset, .size = view.shadow_range.count};
		}

		inline span_t<stock_world_renderer_queue_item_t> get_mutable_visible_queue(const stock_world_renderer_prep_view_t& view)
		{
			return {.data = visible_queue.data() + view.visible_range.offset, .size = view.visible_range.count};
		}
	};

	struct stock_world_renderer_snapshot_t
	{
		engine_shadow_settings_t								shadows			  = {};
		world_renderer_camera_t									main_view		  = {};
		stock_world_renderer_environment_t						environment		  = {};
		stock_world_renderer_fog_t								fog				  = {};
		stock_world_renderer_post_process_t						post_process	  = {};
		world_debug_render_snapshot_t							debug_draw		  = {};
		world_canvas_render_snapshot_t							canvas			  = {};
		fixed_vector_t<stock_world_renderer_material_t>			materials		  = {};
		fixed_vector_t<stock_world_renderer_entity_t>			entities		  = {};
		fixed_vector_t<stock_world_renderer_bone_t>				bones			  = {};
		fixed_vector_t<stock_world_renderer_light_t>			lights			  = {};
		fixed_vector_t<stock_world_renderer_reflection_probe_t> reflection_probes = {};
		fixed_vector_t<stock_world_renderer_renderable_t>		renderables		  = {};
		fixed_vector_t<stock_world_renderer_mesh_draw_t>		mesh_draws		  = {};
		fixed_vector_t<stock_world_renderer_sprite_draw_t>		sprite_draws	  = {};
		fixed_vector_t<stock_world_renderer_particle_draw_t>	particle_draws	  = {};
		fixed_vector_t<stock_world_renderer_particle_t>			particles		  = {};
		engine_quality_level_e									quality_level	  = engine_quality_level_e::high;

		void init(const world_render_context_config_t& config, const world_debug_draw_config_t& debug_draw_config);
		void uninit();
	};
}
