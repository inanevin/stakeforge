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

#include "game_world_renderer.hpp"
#include "math/math.hpp"

// gfx
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/world/gpu_entity.hpp"
#include "gfx/world/gpu_bone.hpp"
#include "gfx/world/gpu_light.hpp"
#include "gfx/world/gpu_shadow_data.hpp"
#include "gfx/world/renderable_collector.hpp"
#include "gfx/util/shadow_util.hpp"

#include "world/world.hpp"

#include <tracy/Tracy.hpp>
#include <functional>
#include <algorithm>
#include <execution>

namespace SFG
{
	game_world_renderer::game_world_renderer(proxy_manager& pm, world& w) : _proxy_manager(pm), _world(w){};

	void game_world_renderer::init(const vector2ui16& size, texture_queue* tq, buffer_queue* bq, gfx_id bind_layout, gfx_id bind_layout_compute)
	{
		gfx_backend* backend = gfx_backend::get();

		_renderables.reserve(MAX_RENDERABLES_ALL);

		const bitmask16 color_flags = texture_flags::tf_is_2d | texture_flags::tf_render_target | texture_flags::tf_sampled;

		_base_size = size;

		// pfd
		for (u8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd			= _pfd[i];
			pfd.cmd_upload				= backend->create_command_buffer({.type = command_type::graphics, .debug_name = "wr_upload"});
			pfd.semp_frame.semaphore	= backend->create_semaphore();
			pfd.semp_ssao.semaphore		= backend->create_semaphore();
			pfd.semp_lighting.semaphore = backend->create_semaphore();

			pfd.shadow_data_buffer.create(
				{
					.size		= sizeof(gpu_shadow_data) * MAX_GPU_SHADOW_DATA,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "shadow_data_cpu",
				},
				{
					.size			 = sizeof(gpu_shadow_data) * MAX_GPU_SHADOW_DATA,
					.structure_size	 = sizeof(gpu_shadow_data),
					.structure_count = MAX_GPU_SHADOW_DATA,
					.flags			 = resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name		 = "shadow_data_gpu",
				});

			pfd.bones_buffer.create(
				{
					.size		= sizeof(gpu_bone) * MAX_GPU_BONES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "opaque_bones_cpu",
				},
				{
					.size			 = sizeof(gpu_bone) * MAX_GPU_BONES,
					.structure_size	 = sizeof(gpu_bone),
					.structure_count = MAX_GPU_BONES,
					.flags			 = resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name		 = "opaque_bones_gpu",
				});

			pfd.entity_buffer.create(
				{
					.size		= sizeof(gpu_entity) * MAX_GPU_ENTITIES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "opaque_entities_cpu",
				},
				{
					.size			 = sizeof(gpu_entity) * MAX_GPU_ENTITIES,
					.structure_size	 = sizeof(gpu_entity),
					.structure_count = MAX_GPU_ENTITIES,
					.flags			 = resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name		 = "opaque_entities_gpu",
				});

			pfd.dir_lights_buffer.create(
				{
					.size		= sizeof(gpu_dir_light) * MAX_WORLD_COMP_DIR_LIGHTS,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "lighting_dir_lights_cpu",
				},
				{
					.size			 = sizeof(gpu_dir_light) * MAX_WORLD_COMP_DIR_LIGHTS,
					.structure_size	 = sizeof(gpu_dir_light),
					.structure_count = MAX_WORLD_COMP_DIR_LIGHTS,
					.flags			 = resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name		 = "lighting_dir_lights_gpu",
				});

			pfd.point_lights_buffer.create(
				{
					.size		= sizeof(gpu_point_light) * MAX_WORLD_COMP_POINT_LIGHTS,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "lighting_point_lights_cpu",
				},
				{
					.size			 = sizeof(gpu_point_light) * MAX_WORLD_COMP_POINT_LIGHTS,
					.structure_size	 = sizeof(gpu_point_light),
					.structure_count = MAX_WORLD_COMP_POINT_LIGHTS,
					.flags			 = resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name		 = "lighting_point_lights_gpu",
				});

			pfd.spot_lights_buffer.create(
				{
					.size		= sizeof(gpu_spot_light) * MAX_WORLD_COMP_SPOT_LIGHTS,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "lighting_spot_lights_cpu",
				},
				{
					.size			 = sizeof(gpu_spot_light) * MAX_WORLD_COMP_SPOT_LIGHTS,
					.structure_size	 = sizeof(gpu_spot_light),
					.structure_count = MAX_WORLD_COMP_SPOT_LIGHTS,
					.flags			 = resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name		 = "lighting_spot_lights_gpu",
				});

			pfd.float_buffer.create(
				{
					.size		= sizeof(f32) * 128,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "float_buffer",
				},
				{
					.size			 = sizeof(f32) * 128,
					.structure_size	 = sizeof(f32),
					.structure_count = 128,
					.flags			 = resource_flags::rf_gpu_only | resource_flags::rf_storage_buffer,
					.debug_name		 = "float_buffer_gpu",
				});
		}

		_pass_pre_depth.init(size);
		_pass_opaque.init(size);
		_pass_lighting.init(size);
		_pass_shadows.init();
		_pass_ssao.init(size, *tq);
		_pass_bloom.init(size);
		_pass_post.init(size);
		_pass_forward.init(size);
		_pass_canvas_2d.init(size);
		_pass_particles.init(bind_layout, bind_layout_compute);
		_pass_sprite.init(size);

#ifdef SFG_TOOLMODE
		_pass_debug_rendering.init(size, _world);
		_pass_object_id.init(size);
		_pass_selection_outline.init(size);
#endif
	}

	void game_world_renderer::uninit()
	{
		_pass_pre_depth.uninit();
		_pass_opaque.uninit();
		_pass_lighting.uninit();
		_pass_shadows.uninit();
		_pass_ssao.uninit();
		_pass_bloom.uninit();
		_pass_post.uninit();
		_pass_forward.uninit();
		_pass_canvas_2d.uninit();
		_pass_particles.uninit();
		_pass_sprite.uninit();

#ifdef SFG_TOOLMODE
		_pass_debug_rendering.uninit();
		_pass_object_id.uninit();
		_pass_selection_outline.uninit();
#endif

		gfx_backend* backend = gfx_backend::get();

		for (u8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];
			backend->destroy_command_buffer(pfd.cmd_upload);
			backend->destroy_semaphore(pfd.semp_frame.semaphore);
			backend->destroy_semaphore(pfd.semp_lighting.semaphore);
			backend->destroy_semaphore(pfd.semp_ssao.semaphore);
			pfd.shadow_data_buffer.destroy();
			pfd.entity_buffer.destroy();
			pfd.bones_buffer.destroy();
			pfd.dir_lights_buffer.destroy();
			pfd.spot_lights_buffer.destroy();
			pfd.point_lights_buffer.destroy();
			pfd.float_buffer.destroy();
		}
	}

	void game_world_renderer::tick()
	{
		ZoneScoped;
#ifdef SFG_TOOLMODE
		_pass_debug_rendering.tick();
#endif
	}

	void game_world_renderer::prepare(u8 frame_index)
	{
		ZoneScoped;

		per_frame_data& pfd = _pfd[frame_index];
		pfd.reset();

		chunk_allocator32& proxy_aux = _proxy_manager.get_aux();

		// Handle main camera.
		const world_id main_cam_trait = _proxy_manager.get_main_camera();
		if (main_cam_trait != NULL_WORLD_ID)
		{
			const render_proxy_camera& cam_proxy  = _proxy_manager.get_camera(main_cam_trait);
			const render_proxy_entity& cam_entity = _proxy_manager.get_entity(cam_proxy.entity);

			const vector3 pos = cam_entity.model.get_translation();

			const matrix4x4 view	  = matrix4x4::view(cam_entity.rotation, pos);
			const matrix4x4 proj	  = matrix4x4::perspective_reverse_z(cam_proxy.fov_degrees, static_cast<f32>(_base_size.x) / static_cast<f32>(_base_size.y), cam_proxy.near_plane, cam_proxy.far_plane);
			const matrix4x4 view_proj = proj * view;
			_main_camera_view		  = {
						.view_frustum		  = frustum::extract(view_proj),
						.view_matrix		  = view,
						.proj_matrix		  = proj,
						.inv_proj_matrix	  = proj.inverse(),
						.view_proj_matrix	  = view_proj,
						.inv_view_proj_matrix = view_proj.inverse(),
						.position			  = pos,
						.near_plane			  = cam_proxy.near_plane,
						.far_plane			  = cam_proxy.far_plane,
						.fov_degrees		  = cam_proxy.fov_degrees,
			};
			_world.get_screen().set_camera_data(view_proj, _main_camera_view.inv_view_proj_matrix, pos);

			if (cam_proxy.cascades.size != 0)
			{
				f32* cascades_ptr = proxy_aux.get<f32>(cam_proxy.cascades);
				for (u8 i = 0; i < cam_proxy.cascade_count; i++)
					_main_camera_view.cascades.push_back(cascades_ptr[i]);
			}
		}

		// entities, lights, bones etc.
		// prepare shadow pass before, collect_and_upload will add passes to it.
		_pass_shadows.prepare(_proxy_manager, _main_camera_view, _base_size, frame_index);
		collect_and_upload(frame_index);

		_renderables.resize(0);
		renderable_collector::collect_mesh_instances(_proxy_manager, _main_camera_view, _renderables);
		_pass_pre_depth.prepare(_proxy_manager, _renderables, _main_camera_view, frame_index);
		_pass_opaque.prepare(_proxy_manager, _renderables, _main_camera_view, frame_index);
		_pass_forward.prepare(_proxy_manager, _renderables, _main_camera_view, _base_size, frame_index);
		_pass_canvas_2d.prepare(_proxy_manager, _renderables, _main_camera_view, _base_size, frame_index);

#ifdef SFG_TOOLMODE
		_pass_object_id.prepare(_proxy_manager, _renderables, _main_camera_view, frame_index);
		_pass_selection_outline.prepare(_proxy_manager, _renderables, _main_camera_view, frame_index);
		_pass_debug_rendering.prepare(_proxy_manager, _main_camera_view, _base_size, frame_index);
#endif

		_pass_lighting.prepare(_proxy_manager, _main_camera_view, frame_index);
		_pass_ssao.prepare(_proxy_manager, _main_camera_view, _base_size, frame_index);
		_pass_bloom.prepare(_proxy_manager, frame_index);
		_pass_post.prepare(_proxy_manager, frame_index, _base_size);
		_pass_particles.prepare(frame_index, _proxy_manager, _main_camera_view);
		_pass_sprite.prepare(frame_index, _proxy_manager, _main_camera_view);
	}

	void game_world_renderer::run_pre_depth(const void* context)
	{
		const game_world_renderer::task_common* cmn = static_cast<const game_world_renderer::task_common*>(context);
		cmn->rend->_pass_pre_depth.render({
			.frame_index		= cmn->frame_index,
			.size				= cmn->resolution,
			.gpu_index_entities = cmn->gpu_index_entities,
			.gpu_index_bones	= cmn->gpu_index_bones,
			.global_layout		= cmn->layout_global,
			.global_group		= cmn->bind_group_global,
		});
	}

	void game_world_renderer::run_shadows(const void* context)
	{
		const game_world_renderer::task_common* cmn = static_cast<const game_world_renderer::task_common*>(context);
		cmn->rend->_pass_shadows.render({
			.frame_index		= cmn->frame_index,
			.size				= cmn->resolution,
			.gpu_index_entities = cmn->gpu_index_entities,
			.gpu_index_bones	= cmn->gpu_index_bones,
			.global_layout		= cmn->layout_global,
			.global_group		= cmn->bind_group_global,
		});
	}

	void game_world_renderer::run_ssao(const void* context)
	{
		const game_world_renderer::task_common* cmn = static_cast<const game_world_renderer::task_common*>(context);

		cmn->rend->_pass_ssao.render({
			.frame_index		   = cmn->frame_index,
			.size				   = cmn->resolution,
			.gpu_index_depth	   = cmn->gpu_index_depth_texture,
			.gpu_index_normals	   = cmn->gpu_index_gbuffer_textures[1],
			.global_layout_compute = cmn->layout_global_compute,
			.global_group		   = cmn->bind_group_global,
		});
	}

	void game_world_renderer::run_opaque(const void* context)
	{
		const game_world_renderer::task_common* cmn = static_cast<const game_world_renderer::task_common*>(context);
		cmn->rend->_pass_opaque.render({
			.frame_index		= cmn->frame_index,
			.size				= cmn->resolution,
			.gpu_index_entities = cmn->gpu_index_entities,
			.gpu_index_bones	= cmn->gpu_index_bones,
			.depth_texture		= cmn->depth_texture,
			.global_layout		= cmn->layout_global,
			.global_group		= cmn->bind_group_global,
		});
	}

	void game_world_renderer::run_particles_compute(const void* context)
	{
		const game_world_renderer::task_common* cmn = static_cast<const game_world_renderer::task_common*>(context);
		cmn->rend->_pass_particles.compute({
			.frame_index		   = cmn->frame_index,
			.global_layout_compute = cmn->layout_global_compute,
			.global_group		   = cmn->bind_group_global,
		});
	}

#ifdef SFG_TOOLMODE
	void game_world_renderer::run_obj_id(const void* context)
	{
		const game_world_renderer::task_common* cmn = static_cast<const game_world_renderer::task_common*>(context);
		cmn->rend->_pass_object_id.render({
			.frame_index		= cmn->frame_index,
			.size				= cmn->resolution,
			.gpu_index_entities = cmn->gpu_index_entities,
			.gpu_index_bones	= cmn->gpu_index_bones,
			.depth_texture		= cmn->depth_texture,
			.global_layout		= cmn->layout_global,
			.global_group		= cmn->bind_group_global,
		});
	}
	void game_world_renderer::run_selection_outline(const void* context)
	{
		const game_world_renderer::task_common* cmn = static_cast<const game_world_renderer::task_common*>(context);
		cmn->rend->_pass_selection_outline.render({
			.frame_index		= cmn->frame_index,
			.size				= cmn->resolution,
			.gpu_index_entities = cmn->gpu_index_entities,
			.gpu_index_bones	= cmn->gpu_index_bones,
			.global_layout		= cmn->layout_global,
			.global_group		= cmn->bind_group_global,
		});
	}
	void game_world_renderer::run_debug_rendering(const void* ctx)
	{
		const game_world_renderer::task_common* cmn = static_cast<const game_world_renderer::task_common*>(ctx);
		cmn->rend->_pass_debug_rendering.render({
			.frame_index   = cmn->frame_index,
			.size		   = cmn->resolution,
			.depth_texture = cmn->depth_texture,
			.input_texture = cmn->lighting_texture,
			.global_layout = cmn->layout_global,
			.global_group  = cmn->bind_group_global,
		});
	}
#endif

	void game_world_renderer::run_lighting(const void* ctx)
	{
		const game_world_renderer::task_common* cmn = static_cast<const game_world_renderer::task_common*>(ctx);
		cmn->rend->_pass_lighting.render({
			.frame_index				  = cmn->frame_index,
			.size						  = cmn->resolution,
			.gpu_index_gbuffer_textures	  = cmn->gpu_index_gbuffer_textures,
			.gpu_index_depth_texture	  = cmn->gpu_index_depth_texture,
			.gpu_index_point_lights		  = cmn->gpu_index_point_lights,
			.gpu_index_spot_lights		  = cmn->gpu_index_spot_lights,
			.gpu_index_dir_lights		  = cmn->gpu_index_dir_lights,
			.gpu_index_entities			  = cmn->gpu_index_entities,
			.gpu_index_shadow_data_buffer = cmn->gpu_index_shadow_data_buffer,
			.gpu_index_float_buffer		  = cmn->gpu_index_float_buffer,
			.gpu_index_ao_out			  = cmn->gpu_index_ao_out,
			.global_layout				  = cmn->layout_global,
			.global_group				  = cmn->bind_group_global,
		});
	}
	void game_world_renderer::run_forward(const void* ctx)
	{
		const game_world_renderer::task_common* cmn = static_cast<const game_world_renderer::task_common*>(ctx);

#ifdef SFG_TOOLMODE
		const bool transition_resources = false;
#else
		const bool transition_resources = true;
#endif
		cmn->rend->_pass_forward.render({
			.frame_index		= cmn->frame_index,
			.size				= cmn->resolution,
			.gpu_index_entities = cmn->gpu_index_entities,
			.gpu_index_bones	= cmn->gpu_index_bones,
			.depth_texture		= cmn->depth_texture,
			.input_texture		= cmn->lighting_texture,
			.global_layout		= cmn->layout_global,
			.global_group		= cmn->bind_group_global,
			.transition			= transition_resources,
		});
	}
	void game_world_renderer::run_particles_render(const void* ctx)
	{
		const game_world_renderer::task_common* cmn = static_cast<const game_world_renderer::task_common*>(ctx);
		cmn->rend->_pass_particles.render({
			.frame_index   = cmn->frame_index,
			.size		   = cmn->resolution,
			.global_layout = cmn->layout_global,
			.global_group  = cmn->bind_group_global,
			.hw_lighting   = cmn->lighting_texture,
			.hw_depth	   = cmn->depth_texture,

		});
	}
	void game_world_renderer::run_sprite(const void* ctx)
	{
		const game_world_renderer::task_common* cmn = static_cast<const game_world_renderer::task_common*>(ctx);
		cmn->rend->_pass_sprite.render({
			.frame_index		= cmn->frame_index,
			.size				= cmn->resolution,
			.global_layout		= cmn->layout_global,
			.global_group		= cmn->bind_group_global,
			.input_texture		= cmn->lighting_texture,
			.depth_texture		= cmn->depth_texture,
			.gpu_index_entities = cmn->gpu_index_entities,
		});
	}
	void game_world_renderer::run_post(const void* ctx)
	{
		const game_world_renderer::task_common* cmn = static_cast<const game_world_renderer::task_common*>(ctx);
		cmn->rend->_pass_post.render({
			.frame_index				 = cmn->frame_index,
			.size						 = cmn->resolution,
			.gpu_index_lighting			 = cmn->gpu_index_lighting,
			.gpu_index_bloom			 = cmn->gpu_index_bloom,
			.gpu_index_selection_outline = cmn->gpu_index_selection_outline,
			.global_layout				 = cmn->layout_global,
			.global_group				 = cmn->bind_group_global,
		});
	}
	void game_world_renderer::run_bloom(const void* ctx)
	{
		const game_world_renderer::task_common* cmn = static_cast<const game_world_renderer::task_common*>(ctx);
		cmn->rend->_pass_bloom.render({
			.frame_index		   = cmn->frame_index,
			.size				   = cmn->resolution,
			.lighting			   = cmn->lighting_texture,
			.gpu_index_lighting	   = cmn->gpu_index_lighting,
			.global_layout_compute = cmn->layout_global_compute,
			.global_group		   = cmn->bind_group_global,
		});
	}
	void game_world_renderer::run_canvas_2d(const void* ctx)
	{
		const game_world_renderer::task_common* cmn = static_cast<const game_world_renderer::task_common*>(ctx);
		cmn->rend->_pass_canvas_2d.render({
			.frame_index   = cmn->frame_index,
			.size		   = cmn->resolution,
			.input_texture = cmn->post_combiner_texture,
			.global_layout = cmn->layout_global,
			.global_group  = cmn->bind_group_global,
		});
	}

	void game_world_renderer::render(u8 frame_index, gfx_id layout_global, gfx_id layout_global_compute, gfx_id bind_group_global, u64 prev_copy, u64 next_copy, gfx_id sem_copy)
	{
		ZoneScoped;

		gfx_backend* backend	   = gfx_backend::get();
		const gfx_id queue_gfx	   = backend->get_queue_gfx();
		const gfx_id queue_compute = backend->get_queue_compute();

		per_frame_data&	  pfd		 = _pfd[frame_index];
		const vector2ui16 resolution = _base_size;

		const gfx_id cmd_canvas_2d		   = _pass_canvas_2d.get_cmd_buffer(frame_index);
		const gfx_id cmd_ssao			   = _pass_ssao.get_cmd_buffer(frame_index);
		const gfx_id cmd_depth			   = _pass_pre_depth.get_cmd_buffer(frame_index);
		const gfx_id cmd_opaque			   = _pass_opaque.get_cmd_buffer(frame_index);
		const gfx_id cmd_lighting		   = _pass_lighting.get_cmd_buffer(frame_index);
		const gfx_id cmd_post			   = _pass_post.get_cmd_buffer(frame_index);
		const gfx_id cmd_shadows		   = _pass_shadows.get_cmd_buffer(frame_index);
		const gfx_id cmd_bloom			   = _pass_bloom.get_cmd_buffer(frame_index);
		const gfx_id cmd_forward		   = _pass_forward.get_cmd_buffer(frame_index);
		const gfx_id cmd_particles		   = _pass_particles.get_cmd_buffer(frame_index);
		const gfx_id cmd_particles_compute = _pass_particles.get_cmd_buffer_compute(frame_index);
		const gfx_id cmd_sprite			   = _pass_sprite.get_cmd_buffer(frame_index);

#ifdef SFG_TOOLMODE
		const gfx_id cmd_debug	   = _pass_debug_rendering.get_cmd_buffer(frame_index);
		const gfx_id cmd_object_id = _pass_object_id.get_cmd_buffer(frame_index);
		const gfx_id cmd_outline   = _pass_selection_outline.get_cmd_buffer(frame_index);
#endif
		const gfx_id sem_frame		   = pfd.semp_frame.semaphore;
		const gfx_id sem_lighting	   = pfd.semp_lighting.semaphore;
		const gfx_id sem_ssao		   = pfd.semp_ssao.semaphore;
		const u64	 sem_lighting_val0 = ++pfd.semp_lighting.value;
		const u64	 sem_lighting_val1 = ++pfd.semp_lighting.value;
		const u64	 sem_ssao_val0	   = ++pfd.semp_ssao.value;
		const u64	 sem_ssao_val1	   = ++pfd.semp_ssao.value;
		const u64	 sem_frame_val	   = ++pfd.semp_frame.value;
		const u8	 ssao_enabled	   = _proxy_manager.get_ssao_exists();
		const u8	 bloom_enabled	   = _proxy_manager.get_bloom_exists();
		const u8	 post_enabled	   = _proxy_manager.get_post_process_exists();

#ifdef SFG_TOOLMODE
		const gpu_index gpu_index_selection_outline = _pass_selection_outline.get_gpu_index_output(frame_index);
#else
		const gpu_index gpu_index_selection_outline = 0;
#endif

		const static_vector<gpu_index, GBUFFER_COLOR_TEXTURES>& gpu_index_gbuffer_textures = _pass_opaque.get_output_gpu_index(frame_index);

		const task_common common_data = {
			.rend						  = this,
			.resolution					  = resolution,
			.gpu_index_entities			  = pfd.entity_buffer.get_index(),
			.gpu_index_bones			  = pfd.bones_buffer.get_index(),
			.gpu_index_gbuffer_textures	  = gpu_index_gbuffer_textures.data(),
			.gpu_index_point_lights		  = pfd.point_lights_buffer.get_index(),
			.gpu_index_spot_lights		  = pfd.spot_lights_buffer.get_index(),
			.gpu_index_dir_lights		  = pfd.dir_lights_buffer.get_index(),
			.gpu_index_shadow_data_buffer = pfd.shadow_data_buffer.get_index(),
			.gpu_index_float_buffer		  = pfd.float_buffer.get_index(),
			.gpu_index_depth_texture	  = _pass_pre_depth.get_output_gpu_index(frame_index),
			.gpu_index_ao_out			  = _pass_ssao.get_output_gpu_index(frame_index),
			.gpu_index_lighting			  = _pass_lighting.get_output_gpu_index(frame_index),
			.gpu_index_bloom			  = _pass_bloom.get_output_gpu_index(frame_index),
			.gpu_index_selection_outline  = gpu_index_selection_outline,
			.layout_global				  = layout_global,
			.layout_global_compute		  = layout_global_compute,
			.bind_group_global			  = bind_group_global,
			.depth_texture				  = _pass_pre_depth.get_output_hw(frame_index),
			.lighting_texture			  = _pass_lighting.get_output_hw(frame_index),
			.post_combiner_texture		  = _pass_post.get_output_hw(frame_index),
			.frame_index				  = frame_index,
		};

		static_vector<task, 12> tt;

		tt.push_back({run_pre_depth, (void*)&common_data});
		tt.push_back({run_shadows, (void*)&common_data});
		tt.push_back({run_opaque, (void*)&common_data});
		tt.push_back({run_ssao, (void*)&common_data});
		tt.push_back({run_particles_compute, (void*)&common_data});

#ifdef SFG_TOOLMODE
		tt.push_back({run_obj_id, (void*)&common_data});
		tt.push_back({run_selection_outline, (void*)&common_data});

#endif
		std::for_each(std::execution::par, tt.begin(), tt.end(), [](auto&& task) { task(); });

		if (prev_copy != next_copy)
			backend->queue_wait(queue_gfx, &sem_copy, &next_copy, 1);

		gfx_id first_batch[3] = {cmd_depth, cmd_shadows, cmd_opaque};
		backend->submit_commands(queue_gfx, first_batch, 3);
		backend->queue_signal(queue_gfx, &sem_ssao, &sem_ssao_val0, 1);

		// kick off particle compute immediately.
		backend->submit_commands(queue_compute, &cmd_particles_compute, 1);

#ifdef SFG_TOOLMODE
		// object-id pass & outline (no dependencies besides depth)
		backend->submit_commands(queue_gfx, &cmd_object_id, 1);
		backend->submit_commands(queue_gfx, &cmd_outline, 1);
#endif

		tt.resize(0);

#ifdef SFG_TOOLMODE
		tt.push_back({run_debug_rendering, (void*)&common_data});
#endif
		tt.push_back({run_lighting, (void*)&common_data});
		tt.push_back({run_forward, (void*)&common_data});
		tt.push_back({run_particles_render, (void*)&common_data});
		tt.push_back({run_sprite, (void*)&common_data});

		std::for_each(std::execution::par, tt.begin(), tt.end(), [](auto&& task) { task(); });

		// SSAO waits for opaque, signals after done
		backend->queue_wait(queue_compute, &sem_ssao, &sem_ssao_val0, 1);
		backend->submit_commands(queue_compute, &cmd_ssao, 1);
		backend->queue_signal(queue_compute, &sem_ssao, &sem_ssao_val1, 1);

		// submit lighting + forward + particles + debugs, waits for ssao (and particles compute)
		backend->queue_wait(queue_gfx, &sem_ssao, &sem_ssao_val1, 1);

#ifdef SFG_TOOLMODE
		const gfx_id fw_commands[5] = {cmd_lighting, cmd_particles, cmd_sprite, cmd_forward, cmd_debug};
		backend->submit_commands(queue_gfx, fw_commands, 5);
#else
		const gfx_id fw_commands[4] = {cmd_lighting, cmd_particles, cmd_sprite, cmd_forward};
		backend->submit_commands(queue_gfx, fw_commands, 4);
#endif
		backend->queue_signal(queue_gfx, &sem_lighting, &sem_lighting_val0, 1);

		tt.resize(0);
		tt.push_back({run_post, (void*)&common_data});
		tt.push_back({run_bloom, (void*)&common_data});
		tt.push_back({run_canvas_2d, (void*)&common_data});

		std::for_each(std::execution::par, tt.begin(), tt.end(), [](auto&& task) { task(); });

		// bloom waits for all lighting results.
		backend->queue_wait(queue_compute, &sem_lighting, &sem_lighting_val0, 1);
		backend->submit_commands(queue_compute, &cmd_bloom, 1);
		backend->queue_signal(queue_compute, &sem_lighting, &sem_lighting_val1, 1);

		// post combine waits for bloom
		backend->queue_wait(queue_gfx, &sem_lighting, &sem_lighting_val1, 1);
		backend->submit_commands(queue_gfx, &cmd_post, 1);

		// canvas2d writes after post combiner..
		backend->submit_commands(queue_gfx, &cmd_canvas_2d, 1);

		backend->queue_signal(queue_compute, &sem_frame, &sem_frame_val, 1);
	}

	void game_world_renderer::resize(const vector2ui16& size)
	{
		_base_size			 = size;
		gfx_backend* backend = gfx_backend::get();

		// depth prepass
		_pass_pre_depth.resize(size);
		_pass_opaque.resize(size);
		_pass_lighting.resize(size);
		_pass_ssao.resize(size);
		_pass_bloom.resize(size);
		_pass_post.resize(size);
#ifdef SFG_TOOLMODE
		_pass_object_id.resize(size);
		_pass_selection_outline.resize(size);
#endif
	}

	u32 game_world_renderer::add_to_float_buffer(u8 frame_index, f32 f)
	{
		per_frame_data& pfd = _pfd[frame_index];
		pfd.float_buffer.buffer_data(pfd._float_buffer_count * sizeof(f32), &f, sizeof(f32));
		pfd._float_buffer_count++;
		return pfd._float_buffer_count - 1;
	}

	void game_world_renderer::collect_and_upload(u8 frame_index)
	{
		gfx_backend* backend   = gfx_backend::get();
		const gfx_id queue_gfx = backend->get_queue_gfx();

		per_frame_data& pfd = _pfd[frame_index];
		const gfx_id	cmd = pfd.cmd_upload;

		backend->reset_command_buffer(cmd);

		static_vector<barrier, 7> barriers;

		barriers.push_back({
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_copy_dest,
			.resource	 = pfd.bones_buffer.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_copy_dest,
			.resource	 = pfd.entity_buffer.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_ps_resource,
			.to_states	 = resource_state::resource_state_copy_dest,
			.resource	 = pfd.dir_lights_buffer.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_ps_resource,
			.to_states	 = resource_state::resource_state_copy_dest,
			.resource	 = pfd.spot_lights_buffer.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_ps_resource,
			.to_states	 = resource_state::resource_state_copy_dest,
			.resource	 = pfd.point_lights_buffer.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_ps_resource,
			.to_states	 = resource_state::resource_state_copy_dest,
			.resource	 = pfd.shadow_data_buffer.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_ps_resource,
			.to_states	 = resource_state::resource_state_copy_dest,
			.resource	 = pfd.float_buffer.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});

		backend->cmd_barrier(cmd, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});

		const u8 cascades_count = static_cast<u8>(_main_camera_view.cascades.size());
		for (u8 i = 0; i < cascades_count; i++)
		{
			const u32 idx = add_to_float_buffer(frame_index, _main_camera_view.cascades[i] * _main_camera_view.far_plane);
			if (i == 0)
				_main_camera_view.cascsades_gpu_index = idx;
		}

		if (pfd._float_buffer_count != 0)
			pfd.float_buffer.copy_region(cmd, 0, sizeof(f32) * pfd._float_buffer_count);

		collect_and_upload_entities(cmd, frame_index);
		collect_and_upload_bones(cmd, frame_index);
		collect_and_upload_lights(cmd, frame_index);

		barriers.resize(0);
		barriers.push_back({
			.from_states = resource_state::resource_state_copy_dest,
			.to_states	 = resource_state::resource_state_non_ps_resource,
			.resource	 = pfd.bones_buffer.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_copy_dest,
			.to_states	 = resource_state::resource_state_non_ps_resource,
			.resource	 = pfd.entity_buffer.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_copy_dest,
			.to_states	 = resource_state::resource_state_ps_resource,
			.resource	 = pfd.dir_lights_buffer.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_copy_dest,
			.to_states	 = resource_state::resource_state_ps_resource,
			.resource	 = pfd.spot_lights_buffer.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_copy_dest,
			.to_states	 = resource_state::resource_state_ps_resource,
			.resource	 = pfd.point_lights_buffer.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_copy_dest,
			.to_states	 = resource_state::resource_state_ps_resource,
			.resource	 = pfd.shadow_data_buffer.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_copy_dest,
			.to_states	 = resource_state::resource_state_ps_resource,
			.resource	 = pfd.float_buffer.get_gpu(),
			.flags		 = barrier_flags::baf_is_resource,
		});

		backend->cmd_barrier(cmd, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});
		backend->close_command_buffer(cmd);
		backend->submit_commands(queue_gfx, &cmd, 1);
	}

	void game_world_renderer::collect_and_upload_entities(gfx_id cmd_buffer, u8 frame_index)
	{
		ZoneScoped;

		per_frame_data& pfd			  = _pfd[frame_index];
		auto&			entities	  = *_proxy_manager.get_entities();
		const u32		entities_peak = _proxy_manager.get_peak_entities();

		size_t offset		  = 0;
		u32	   assigned_index = 0;

		gpu_entity* gpu_entities = reinterpret_cast<gpu_entity*>(pfd.entity_buffer.get_mapped());

		for (u32 i = 0; i < entities_peak; i++)
		{
			render_proxy_entity& e = entities.get(i);
			if (e.status != render_proxy_status::rps_active)
				continue;

			if (e.flags.is_set(render_proxy_entity_flags::render_proxy_entity_invisible))
			{
				e._assigned_index = UINT32_MAX;
				continue;
			}

			e._assigned_index = assigned_index;

			const vector3 forward = e.rotation.get_forward();
			const vector3 pos	  = e.model.get_translation();

			gpu_entities[assigned_index] = {
				.model	  = e.model.to_matrix4x4(),
				.normal	  = e.normal.to_matrix4x4(),
				.position = vector4(pos.x, pos.y, pos.z, 0),
				.forward  = vector4(forward.x, forward.y, forward.z, 0.0f),
			};

			assigned_index++;
		}

		if (assigned_index != 0)
			pfd.entity_buffer.copy_region(cmd_buffer, 0, assigned_index * sizeof(gpu_entity));
	}

	void game_world_renderer::collect_and_upload_bones(gfx_id cmd_buffer, u8 frame_index)
	{
		ZoneScoped;

		per_frame_data& pfd = _pfd[frame_index];

		auto&			   meshes	   = *_proxy_manager.get_mesh_instances();
		const u32		   meshes_peak = _proxy_manager.get_peak_mesh_instances();
		auto&			   entities	   = *_proxy_manager.get_entities();
		auto&			   skins	   = *_proxy_manager.get_skins();
		chunk_allocator32& aux		   = _proxy_manager.get_aux();

		u32 assigned_index = 0;

		gpu_bone* bones = reinterpret_cast<gpu_bone*>(pfd.bones_buffer.get_mapped());

		for (u32 i = 0; i < meshes_peak; i++)
		{
			render_proxy_mesh_instance& mi = meshes.get(i);
			if (mi.mesh == NULL_RESOURCE_ID)
				continue;

			// early out if entity or skin is invalid.
			const render_proxy_entity& e = entities.get(mi.entity);
			if (e._assigned_index == UINT32_MAX)
			{
				mi._assigned_bone_index = UINT32_MAX;
				continue;
			}

			if (mi.skin == NULL_RESOURCE_ID)
				continue;

			mi._assigned_bone_index = assigned_index;

			const render_proxy_skin& skin = skins.get(mi.skin);
			SFG_ASSERT(skin.status == render_proxy_status::rps_active);

			const u16*		 node_indices	   = aux.get<u16>(skin.nodes);
			const world_id*	 skin_entities_ptr = aux.get<world_id>(mi.skin_entities);
			const matrix4x3* matrices_ptr	   = aux.get<matrix4x3>(skin.matrices);

			matrix4x3 root_global = e.model;
			if (skin.root_node != -1)
			{
				const world_id			   root_entity_id = skin_entities_ptr[node_indices[skin.root_node]];
				const render_proxy_entity& root_entity	  = entities.get(root_entity_id);
				SFG_ASSERT(root_entity.status == render_proxy_status::rps_active);
				root_global = root_entity.model;
			}
			root_global = root_global.inverse();

			for (u16 j = 0; j < skin.node_count; j++)
			{
				const u16				   node_index	  = node_indices[j];
				const world_id			   skin_entity_id = skin_entities_ptr[node_index];
				const render_proxy_entity& skin_entity	  = entities.get(skin_entity_id);
				SFG_ASSERT(skin_entity.status == render_proxy_status::rps_active);

				bones[assigned_index].mat = (root_global * skin_entity.model * matrices_ptr[j]).to_matrix4x4();

				assigned_index++;
			}
		}

		if (assigned_index != 0)
			pfd.bones_buffer.copy_region(cmd_buffer, 0, assigned_index * sizeof(gpu_bone));
	}

	void game_world_renderer::collect_and_upload_lights(gfx_id cmd_buffer, u8 frame_index)
	{
		ZoneScoped;

		per_frame_data& pfd = _pfd[frame_index];

		const u32 points_peak  = _proxy_manager.get_peak_point_lights();
		const u32 spots_peak   = _proxy_manager.get_peak_spot_lights();
		const u32 dirs_peak	   = _proxy_manager.get_peak_dir_lights();
		auto&	  spots		   = *_proxy_manager.get_spot_lights();
		auto&	  points	   = *_proxy_manager.get_point_lights();
		auto&	  dirs		   = *_proxy_manager.get_dir_lights();
		auto&	  entities	   = *_proxy_manager.get_entities();
		u32		  points_count = 0, spots_count = 0, dirs_count = 0;
		u32		  shadow_data_count = 0;

		const matrix4x4 main_view_matrix = _main_camera_view.view_matrix;
		const f32		main_cam_near	 = _main_camera_view.near_plane;
		const f32		main_cam_far	 = _main_camera_view.far_plane;
		const f32		main_cam_fov	 = _main_camera_view.fov_degrees;
		const f32		aspect_ratio	 = static_cast<f32>(_base_size.x) / static_cast<f32>(_base_size.y);

		gpu_point_light* plights = reinterpret_cast<gpu_point_light*>(pfd.point_lights_buffer.get_mapped());
		gpu_shadow_data* sdata	 = reinterpret_cast<gpu_shadow_data*>(pfd.shadow_data_buffer.get_mapped());

		{
			ZoneScopedN("plightbase");
			for (u32 i = 0; i < points_peak; i++)
			{
				render_proxy_point_light& light = points.get(i);
				if (light.status != render_proxy_status::rps_active)
					continue;

				const render_proxy_entity& proxy_entity = entities.get(light.entity);
				SFG_ASSERT(proxy_entity.status == render_proxy_status::rps_active);

				if (proxy_entity.flags.is_set(render_proxy_entity_flags::render_proxy_entity_invisible))
					continue;

				const vector3 pos = proxy_entity.model.get_translation();

				constexpr f32 energy_thresh = LIGHT_CULLING_ENERGY_THRESHOLD;
				const f32	  radius		= math::sqrt(light.intensity / (4.0f * MATH_PI * energy_thresh));

				const frustum_result res = frustum::test(_main_camera_view.view_frustum, pos, radius);
				if (res == frustum_result::outside)
					continue;

				i32		  first_shadow_index = -1;
				const f32 far_plane			 = math::almost_equal(light.range, 0.0f) ? main_cam_far : light.range;
				const f32 near_plane		 = light.near_plane;

				if (light.cast_shadows)
				{
					const f32 light_aspect = static_cast<f32>(light.shadow_res.x) / static_cast<f32>(light.shadow_res.y);
					first_shadow_index	   = static_cast<i32>(shadow_data_count);

					static_vector<vector3, 6> dirs;
					static_vector<vector3, 6> fws;

					dirs.push_back(vector3::right * 0.1);
					dirs.push_back(-vector3::right * 0.1);
					dirs.push_back(vector3::up * 0.1);
					dirs.push_back(-vector3::up * 0.1);
					dirs.push_back(vector3::forward * 0.1);
					dirs.push_back(-vector3::forward * 0.1);

					fws.push_back(vector3::up);
					fws.push_back(vector3::up);
					fws.push_back(vector3::forward);
					fws.push_back(vector3::forward);
					fws.push_back(vector3::up);
					fws.push_back(vector3::up);

					for (u8 j = 0; j < 6; j++)
					{
						const matrix4x4 light_view		 = matrix4x4::look_at(pos, pos + dirs[j], fws[j]);
						const matrix4x4 light_projection = matrix4x4::perspective(90 + near_plane, light_aspect, near_plane, far_plane);

						const gpu_shadow_data sh = {
							.light_space_matrix = light_projection * light_view,
						};

						sdata[shadow_data_count].light_space_matrix = light_projection * light_view;
						shadow_data_count++;

						_pass_shadows.add_pass({
							.pm				  = _proxy_manager,
							.frame_index	  = frame_index,
							.res			  = light.shadow_res,
							.texture		  = light.shadow_texture_hw[frame_index],
							.transition_owner = j == 0,
							.view_index		  = j,
							.proj			  = light_projection,
							.view			  = light_view,
							.position		  = pos,
							.cascade_near	  = main_cam_near,
							.cascade_far	  = far_plane,
							.fov			  = main_cam_fov,
						});
					}
				}

				plights[points_count] = {
					.color_entity_index			   = vector4(light.base_color.x, light.base_color.y, light.base_color.z, proxy_entity._assigned_index),
					.intensity_range			   = vector4(light.intensity, light.range, 0.0f, 0.0f),
					.shadow_res_map_and_data_index = vector4(light.shadow_res.x, light.shadow_res.y, static_cast<f32>(light.shadow_texture_gpu_index[frame_index]), static_cast<f32>(first_shadow_index)),
					.near_plane					   = near_plane,
					.far_plane					   = far_plane,
				};

				points_count++;
			}
		}

		gpu_spot_light* slights = reinterpret_cast<gpu_spot_light*>(pfd.spot_lights_buffer.get_mapped());

		for (u32 i = 0; i < spots_peak; i++)
		{
			render_proxy_spot_light& light = spots.get(i);
			if (light.status != render_proxy_status::rps_active)
				continue;

			const render_proxy_entity& proxy_entity = entities.get(light.entity);
			SFG_ASSERT(proxy_entity.status == render_proxy_status::rps_active);
			if (proxy_entity.flags.is_set(render_proxy_entity_flags::render_proxy_entity_invisible))
				continue;

			const vector3 pos			= proxy_entity.model.get_translation();
			constexpr f32 energy_thresh = LIGHT_CULLING_ENERGY_THRESHOLD;
			const f32	  radius		= math::sqrt(light.intensity / (4.0f * MATH_PI * energy_thresh));

			const frustum_result res = frustum::test(_main_camera_view.view_frustum, pos, radius);
			if (res == frustum_result::outside)
				continue;

			const f32 cos_outer	 = math::cos(light.outer_cone * DEG_2_RAD);
			const f32 cos_inner	 = math::cos(light.inner_cone * DEG_2_RAD);
			const f32 near_plane = light.near_plane;

			i32 shadow_data_index = -1;
			if (light.cast_shadows)
			{
				const f32 far_plane	   = math::almost_equal(light.range, 0.0f) ? main_cam_far : light.range;
				const f32 fov_deg	   = RAD_2_DEG * 2.0f * math::acos(cos_outer);
				const f32 light_aspect = static_cast<f32>(light.shadow_res.x) / static_cast<f32>(light.shadow_res.y);

				const vector3	entity_fw		 = proxy_entity.rotation.get_forward();
				const vector3	up				 = math::abs(vector3::dot(entity_fw, vector3::up)) > 0.98f ? vector3::forward : vector3::up;
				const vector3	pos				 = proxy_entity.model.get_translation();
				const matrix4x4 light_view		 = matrix4x4::look_at(pos, pos + proxy_entity.rotation.get_forward() * 0.1, up);
				const matrix4x4 light_projection = matrix4x4::perspective(fov_deg, light_aspect, near_plane, far_plane);

				sdata[shadow_data_count].light_space_matrix = light_projection * light_view;

				_pass_shadows.add_pass({
					.pm				  = _proxy_manager,
					.frame_index	  = frame_index,
					.res			  = light.shadow_res,
					.texture		  = light.shadow_texture_hw[frame_index],
					.transition_owner = true,
					.view_index		  = 0,
					.proj			  = light_projection,
					.view			  = light_view,
					.position		  = pos,
					.cascade_near	  = main_cam_near,
					.cascade_far	  = far_plane,
					.fov			  = main_cam_fov,
				});

				shadow_data_index = static_cast<i32>(shadow_data_count);
				shadow_data_count++;
			}

			slights[spots_count] = {
				.color_entity_index			   = vector4(light.base_color.x, light.base_color.y, light.base_color.z, proxy_entity._assigned_index),
				.intensity_range_inner_outer   = vector4(light.intensity, light.range, cos_inner, cos_outer),
				.shadow_res_map_and_data_index = vector4(light.shadow_res.x, light.shadow_res.y, static_cast<f32>(light.shadow_texture_gpu_index[frame_index]), static_cast<f32>(shadow_data_index)),
			};
			spots_count++;
		}

		gpu_dir_light* dlights = reinterpret_cast<gpu_dir_light*>(pfd.dir_lights_buffer.get_mapped());

		const static_vector<f32, MAX_SHADOW_CASCADES>& cascade_levels = _main_camera_view.cascades;

		for (u32 i = 0; i < dirs_peak; i++)
		{
			render_proxy_dir_light& light = dirs.get(i);
			if (light.status != render_proxy_status::rps_active)
				continue;

			const render_proxy_entity& proxy_entity = entities.get(light.entity);
			SFG_ASSERT(proxy_entity.status == render_proxy_status::rps_active);

			if (proxy_entity.flags.is_set(render_proxy_entity_flags::render_proxy_entity_invisible))
				continue;

			i32 first_shadow_index = -1;

			if (light.cast_shadows)
			{
				first_shadow_index = static_cast<i32>(shadow_data_count);

				const u8 cascades = math::min(static_cast<u8>(cascade_levels.size()), light.cascade_levels);
				for (u8 j = 0; j < cascades; j++)
				{
					const f32 far_multiplier = cascade_levels[j];
					const f32 cascade_far	 = main_cam_far * far_multiplier;
					const f32 cascade_near	 = j == 0 ? main_cam_near : cascade_levels[static_cast<i32>(j) - 1] * main_cam_far;

					const matrix4x4 proj	  = matrix4x4::perspective(main_cam_fov, aspect_ratio, cascade_near, cascade_far);
					const matrix4x4 view_proj = proj * main_view_matrix;

					static_vector<vector4, 8> frustum_world_space;
					vector3					  frustum_world_center;
					shadow_util::get_world_space_ndc(view_proj.inverse(), frustum_world_space, frustum_world_center);

					const matrix4x4 light_view = matrix4x4::look_at(frustum_world_center, frustum_world_center + proxy_entity.rotation.get_forward(), vector3::up);

					matrix4x4 light_projection = matrix4x4::identity;
					vector2	  texel_world	   = vector2::zero;
					shadow_util::get_lightspace_projection(light_projection, light_view, frustum_world_space, light.shadow_res, texel_world);

					sdata[shadow_data_count] = {
						.light_space_matrix = light_projection * light_view,
						.texel_world		= math::max(texel_world.x, texel_world.y),
					};

					_pass_shadows.add_pass({
						.pm				  = _proxy_manager,
						.frame_index	  = frame_index,
						.res			  = light.shadow_res,
						.texture		  = light.shadow_texture_hw[frame_index],
						.transition_owner = j == 0,
						.view_index		  = j,
						.proj			  = light_projection,
						.view			  = light_view,
						.position		  = frustum_world_center,
						.cascade_near	  = cascade_near,
						.cascade_far	  = cascade_far,
						.fov			  = main_cam_fov,
					});
					shadow_data_count++;
				}
			}

			dlights[dirs_count] = {
				.color_entity_index			   = vector4(light.base_color.x, light.base_color.y, light.base_color.z, proxy_entity._assigned_index),
				.intensity					   = vector4(light.intensity, 0.0f, 0.0f, 0.0f),
				.shadow_res_map_and_data_index = vector4(light.shadow_res.x, light.shadow_res.y, static_cast<f32>(light.shadow_texture_gpu_index[frame_index]), static_cast<f32>(first_shadow_index)),
			};
			dirs_count++;
		}

		if (dirs_count != 0)
			pfd.dir_lights_buffer.copy_region(cmd_buffer, 0, dirs_count * sizeof(gpu_dir_light));
		if (spots_count != 0)
			pfd.spot_lights_buffer.copy_region(cmd_buffer, 0, spots_count * sizeof(gpu_spot_light));
		if (points_count != 0)
			pfd.point_lights_buffer.copy_region(cmd_buffer, 0, points_count * sizeof(gpu_point_light));
		if (shadow_data_count != 0)
			pfd.shadow_data_buffer.copy_region(cmd_buffer, 0, shadow_data_count * sizeof(gpu_shadow_data));

		_pass_lighting.set_light_counts_for_frame(points_count, spots_count, dirs_count);
	}

}
