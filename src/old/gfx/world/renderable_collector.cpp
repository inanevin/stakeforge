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

#include "renderable_collector.hpp"
#include "gfx/proxy/render_proxy_entity.hpp"
#include "gfx/proxy/render_proxy_components.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/world/view.hpp"
#include "gfx/draw_stream.hpp"
#include "math/frustum.hpp"
#include "resources/vertex.hpp"
#include "resources/shader_direct.hpp"

#include "io/log.hpp"
namespace SFG
{
	void renderable_collector::collect_mesh_instances(proxy_manager& pm, const view& target_view, vector<renderable_object>& out_objects)
	{
		const u32		   mesh_instances_peak = pm.get_peak_mesh_instances();
		auto&			   mesh_instances	   = *pm.get_mesh_instances();
		auto&			   entities			   = *pm.get_entities();
		chunk_allocator32& aux				   = pm.get_aux();

		u32 obj_counter = 0;

		for (u32 i = 0; i < mesh_instances_peak; i++)
		{
			const render_proxy_mesh_instance& mesh_instance = mesh_instances.get(i);
			if (mesh_instance.mesh == NULL_RESOURCE_ID)
				continue;

			const render_proxy_entity& proxy_entity = entities.get(mesh_instance.entity);
			SFG_ASSERT(proxy_entity.status == render_proxy_status::rps_active);

			if (proxy_entity._assigned_index == UINT32_MAX)
				continue;

			const render_proxy_mesh& proxy_mesh = pm.get_mesh(mesh_instance.mesh);
			if (proxy_mesh.primitives.size == 0)
				continue;

			const vector3		 pos = proxy_entity.model.get_translation();
			const frustum_result res = frustum::test(target_view.view_frustum, proxy_mesh.local_aabb, proxy_entity.model.to_linear3x3(), pos);
			if (res == frustum_result::outside)
				continue;

			const u32					  buffer_index = proxy_entity._assigned_index;
			const render_proxy_primitive* primitives   = aux.get<render_proxy_primitive>(proxy_mesh.primitives);

			SFG_ASSERT(buffer_index != UINT32_MAX && (mesh_instance.skin == NULL_RESOURCE_ID || mesh_instance._assigned_bone_index != UINT32_MAX));

			resource_id* materials	  = pm.get_aux().get<resource_id>(mesh_instance.materials);
			const u16	 mi_mat_count = mesh_instance.materials_count;

			for (u32 j = 0; j < proxy_mesh.primitive_count; j++)
			{
				const render_proxy_primitive& prim		= primitives[j];
				const u16					  mat_index = prim.material_index;
				SFG_ASSERT(mat_index < mi_mat_count);
				const resource_id mat = materials[mat_index];

				out_objects.emplace_back(renderable_object{
					.vertex_buffer	   = const_cast<buffer_cpu_gpu*>(&proxy_mesh.vertex_buffer),
					.index_buffer	   = const_cast<buffer_cpu_gpu*>(&proxy_mesh.index_buffer),
					.vertex_start	   = prim.vertex_start,
					.index_start	   = prim.index_start,
					.index_count	   = prim.index_count,
					.gpu_entity		   = buffer_index,
					.bones_start_index = mesh_instance._assigned_bone_index,
					.world_entity	   = mesh_instance.entity,
					.distance		   = vector3::distance_sqr(pos, target_view.position),
					.material		   = mat,
					.is_skinned		   = proxy_mesh.is_skinned,
				});
			}
		}
	}

	void renderable_collector::populate_draw_stream(proxy_manager& pm, const vector<renderable_object>& renderables, draw_stream& stream, u32 required_material_flags, u32 base_flags, u8 frame_index, gfx_id override_shader)
	{
		for (const renderable_object& obj : renderables)
		{
			const render_proxy_material_runtime& proxy_material = pm.get_material_runtime(obj.material);
			if (!proxy_material.flags.is_all_set(required_material_flags))
				continue;

			const gpu_index gpu_index_mat		   = proxy_material.gpu_index_buffers[frame_index];
			const gpu_index gpu_index_mat_textures = proxy_material.gpu_index_texture_buffers[frame_index];
			const gpu_index gpu_index_mat_sampler  = proxy_material.gpu_index_sampler;
			const gfx_id	base_shader			   = override_shader != NULL_GFX_ID ? override_shader : proxy_material.shader_handle;
			const bitmask32 mat_flags			   = proxy_material.flags;
			const bool		is_alpha_cutoff		   = mat_flags.is_set(material_flags::material_flags_is_alpha_cutoff);
			const bool		is_double_sided		   = mat_flags.is_set(material_flags::material_flags_is_double_sided);

			bitmask<u32> variant_flags = base_flags;
			variant_flags.set(shader_variant_flags::variant_flag_alpha_cutoff, is_alpha_cutoff);
			variant_flags.set(shader_variant_flags::variant_flag_skinned, obj.is_skinned);
			variant_flags.set(shader_variant_flags::variant_flag_double_sided, is_double_sided);

			const gfx_id target_shader = pm.get_shader_variant(base_shader, variant_flags.value());
			SFG_ASSERT(target_shader != NULL_GFX_ID);

			stream.add_command({
				.start_index			 = obj.index_start,
				.index_count			 = obj.index_count,
				.base_vertex			 = obj.vertex_start,
				.material_constant_index = gpu_index_mat,
				.texture_constant_index	 = gpu_index_mat_textures,
				.sampler_constant_index	 = gpu_index_mat_sampler,
				.entity_constant_index	 = obj.gpu_entity,
				.bone_constant_index	 = obj.bones_start_index,
#ifdef SFG_TOOLMODE
				.entity_world_id = obj.world_entity,
#endif
				.vb_hw		 = obj.vertex_buffer->get_gpu(),
				.ib_hw		 = obj.index_buffer->get_gpu(),
				.pipeline_hw = target_shader,
				.vertex_size = obj.is_skinned ? static_cast<u16>(sizeof(vertex_skinned)) : static_cast<u16>(sizeof(vertex_static)),
				.priority	 = proxy_material.draw_priority,
			});
		}
	}

	void renderable_collector::populate_draw_stream(proxy_manager& pm, const vector<renderable_object>& renderables, draw_stream_distance& stream, u32 required_material_flags, u32 base_variant_flags, u8 frame_index, gfx_id override_shader)
	{
		for (const renderable_object& obj : renderables)
		{
			const render_proxy_material_runtime& proxy_material = pm.get_material_runtime(obj.material);
			if (!proxy_material.flags.is_all_set(required_material_flags))
				continue;

			const gpu_index gpu_index_mat		   = proxy_material.gpu_index_buffers[frame_index];
			const gpu_index gpu_index_mat_textures = proxy_material.gpu_index_texture_buffers[frame_index];
			const gpu_index gpu_index_mat_sampler  = proxy_material.gpu_index_sampler;
			const gfx_id	base_shader			   = override_shader != NULL_GFX_ID ? override_shader : proxy_material.shader_handle;
			const bitmask32 mat_flags			   = proxy_material.flags;
			const bool		is_alpha_cutoff		   = mat_flags.is_set(material_flags::material_flags_is_alpha_cutoff);
			const bool		is_double_sided		   = mat_flags.is_set(material_flags::material_flags_is_double_sided);

			bitmask<u32> variant_flags = base_variant_flags;
			variant_flags.set(shader_variant_flags::variant_flag_alpha_cutoff, is_alpha_cutoff);
			variant_flags.set(shader_variant_flags::variant_flag_skinned, obj.is_skinned);
			variant_flags.set(shader_variant_flags::variant_flag_double_sided, is_double_sided);

			const gfx_id target_shader = pm.get_shader_variant(base_shader, variant_flags.value());
			SFG_ASSERT(target_shader != NULL_GFX_ID);

			stream.add_command({
				.start_index			 = obj.index_start,
				.index_count			 = obj.index_count,
				.base_vertex			 = obj.vertex_start,
				.material_constant_index = gpu_index_mat,
				.texture_constant_index	 = gpu_index_mat_textures,
				.sampler_constant_index	 = gpu_index_mat_sampler,
				.entity_constant_index	 = obj.gpu_entity,
				.bone_constant_index	 = obj.bones_start_index,
#ifdef SFG_TOOLMODE
				.entity_world_id = obj.world_entity,
#endif
				.vb_hw		 = obj.vertex_buffer->get_gpu(),
				.ib_hw		 = obj.index_buffer->get_gpu(),
				.pipeline_hw = target_shader,
				.vertex_size = obj.is_skinned ? static_cast<u16>(sizeof(vertex_skinned)) : static_cast<u16>(sizeof(vertex_static)),
				.priority	 = proxy_material.draw_priority,
				.distance	 = obj.distance,
			});
		}
	}

	void renderable_collector::populate_draw_stream_outline_filtered(proxy_manager& pm, const vector<renderable_object>& renderables, draw_stream& stream, u32 base_variant_flags, u8 frame_index, const shader_direct& direct, u32 target_world_id)
	{
		for (const renderable_object& obj : renderables)
		{
			if (obj.world_entity != target_world_id)
				continue;

			const render_proxy_material_runtime& proxy_material			= pm.get_material_runtime(obj.material);
			const gpu_index						 gpu_index_mat			= proxy_material.gpu_index_buffers[frame_index];
			const gpu_index						 gpu_index_mat_textures = proxy_material.gpu_index_texture_buffers[frame_index];
			const gpu_index						 gpu_index_mat_sampler	= proxy_material.gpu_index_sampler;

			bitmask<u32> variant_flags = base_variant_flags;
			variant_flags.set(shader_variant_flags::variant_flag_skinned, obj.is_skinned);
			variant_flags.set(shader_variant_flags::variant_flag_double_sided, proxy_material.flags.is_set(material_flags::material_flags_is_double_sided));

			const gfx_id target_shader = direct.get_hw(variant_flags.value());
			SFG_ASSERT(target_shader != NULL_GFX_ID);

			stream.add_command({
				.start_index			 = obj.index_start,
				.index_count			 = obj.index_count,
				.base_vertex			 = obj.vertex_start,
				.material_constant_index = gpu_index_mat,
				.texture_constant_index	 = gpu_index_mat_textures,
				.sampler_constant_index	 = gpu_index_mat_sampler,
				.entity_constant_index	 = obj.gpu_entity,
				.bone_constant_index	 = obj.bones_start_index,
#ifdef SFG_TOOLMODE
				.entity_world_id = obj.world_entity,
#endif
				.vb_hw		 = obj.vertex_buffer->get_gpu(),
				.ib_hw		 = obj.index_buffer->get_gpu(),
				.pipeline_hw = target_shader,
				.vertex_size = obj.is_skinned ? static_cast<u16>(sizeof(vertex_skinned)) : static_cast<u16>(sizeof(vertex_static)),
				.priority	 = proxy_material.draw_priority,
			});
		}
	}
}
