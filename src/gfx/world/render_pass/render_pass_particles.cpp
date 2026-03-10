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

#include "render_pass_particles.hpp"
#include "math/math.hpp"
#include "math/random.hpp"
#include "common/packed_size.hpp"
#include "app/engine_resources.hpp"

// gfx
#include "gfx/backend/backend.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/commands.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/proxy/proxy_manager.hpp"
#include "gfx/common/barrier_description.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "gfx/world/view.hpp"
#include "gfx/texture_queue.hpp"

#include "common/system_info.hpp"

#include <tracy/Tracy.hpp>

namespace SFG
{

	void render_pass_particles::init(gfx_id bind_layout, gfx_id bind_layout_compute)
	{
		gfx_backend* backend = gfx_backend::get();

		_indirect_sig_dispatch = backend->create_dispatch_indirect_signature(NULL_GFX_ID, sizeof(u32) * 3);
		_indirect_sig_draw	   = backend->create_draw_indirect_signature(NULL_GFX_ID, sizeof(particle_indirect_args));
		_alloc.init(PASS_ALLOC_SIZE_PARTICLES, 8);

		_sim_state.emit_counts.create(
			{
				.size		= sizeof(u32) * MAX_WORLD_PARTICLES,
				.flags		= resource_flags::rf_cpu_visible,
				.debug_name = "emit_counts",
			},
			{
				.size			 = sizeof(u32) * MAX_WORLD_PARTICLES,
				.structure_size	 = sizeof(u32),
				.structure_count = MAX_WORLD_PARTICLES,
				.flags			 = resource_flags::rf_storage_buffer | resource_flags::rf_gpu_only,
				.debug_name		 = "emit_counts_gpu",
			});

		_sim_state.emit_arguments.create(
			{
				.size		= sizeof(particle_emit_args) * MAX_WORLD_COMP_PARTICLE_EMITTERS,
				.flags		= resource_flags::rf_cpu_visible,
				.debug_name = "emit_args",
			},
			{
				.size			 = sizeof(particle_emit_args) * MAX_WORLD_COMP_PARTICLE_EMITTERS,
				.structure_size	 = sizeof(particle_emit_args),
				.structure_count = MAX_WORLD_COMP_PARTICLE_EMITTERS,
				.flags			 = resource_flags::rf_storage_buffer | resource_flags::rf_gpu_only,
				.debug_name		 = "emit_args_gpu",
			});

		_sim_state.system_data.create(
			{
				.size		= sizeof(particle_system_data) * MAX_WORLD_COMP_PARTICLE_EMITTERS,
				.flags		= resource_flags::rf_cpu_visible,
				.debug_name = "particle_system_data",
			},
			{
				.size			 = sizeof(particle_system_data) * MAX_WORLD_COMP_PARTICLE_EMITTERS,
				.structure_size	 = sizeof(particle_system_data),
				.structure_count = MAX_WORLD_COMP_PARTICLE_EMITTERS,
				.flags			 = resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
				.debug_name		 = "particle_system_data_gpu",
			});

		_sim_state.states.create(
			{
				.size		= sizeof(particle_state) * MAX_WORLD_PARTICLES,
				.flags		= resource_flags::rf_cpu_visible,
				.debug_name = "particle_states",
			},
			{
				.size			 = sizeof(particle_state) * MAX_WORLD_PARTICLES,
				.structure_size	 = sizeof(particle_state),
				.structure_count = MAX_WORLD_PARTICLES,
				.flags			 = resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
				.debug_name		 = "particle_states_gpu",
			});

		_sim_state.sim_count_indirect_arguments.create(
			{
				.size		= sizeof(particle_sim_count_args),
				.flags		= resource_flags::rf_cpu_visible,
				.debug_name = "particle_sim_count_args",
			},
			{
				.size			 = sizeof(particle_sim_count_args),
				.structure_size	 = sizeof(particle_sim_count_args),
				.structure_count = 1,
				.flags			 = resource_flags::rf_indirect_buffer | resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
				.debug_name		 = "particle_sim_count_args_gpu",
			});

		_sim_state.alive_list_a.create(
			{
				.size		= sizeof(u32) * MAX_WORLD_PARTICLES,
				.flags		= resource_flags::rf_cpu_visible,
				.debug_name = "particle_alive_a",
			},
			{
				.size			 = sizeof(u32) * MAX_WORLD_PARTICLES,
				.structure_size	 = sizeof(u32),
				.structure_count = MAX_WORLD_PARTICLES,
				.flags			 = resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
				.debug_name		 = "particle_alive_a_gpu",
			});

		_sim_state.alive_list_b.create(
			{
				.size		= sizeof(u32) * MAX_WORLD_PARTICLES,
				.flags		= resource_flags::rf_cpu_visible,
				.debug_name = "particle_alive_b",
			},
			{
				.size			 = sizeof(u32) * MAX_WORLD_PARTICLES,
				.structure_size	 = sizeof(u32),
				.structure_count = MAX_WORLD_PARTICLES,
				.flags			 = resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
				.debug_name		 = "particle_alive_b_gpu",
			});

		_sim_state.dead_indices.create(
			{
				.size		= sizeof(u32) * MAX_WORLD_PARTICLES,
				.flags		= resource_flags::rf_cpu_visible,
				.debug_name = "particle_dead_indices",
			},
			{
				.size			 = sizeof(u32) * MAX_WORLD_PARTICLES,
				.structure_size	 = sizeof(u32),
				.structure_count = MAX_WORLD_PARTICLES,
				.flags			 = resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
				.debug_name		 = "particle_dead_indices_gpu",
			});

		_sim_state.counters.create(
			{
				.size		= sizeof(particle_counters),
				.flags		= resource_flags::rf_cpu_visible,
				.debug_name = "particle_counters",
			},
			{
				.size			 = sizeof(particle_counters),
				.structure_size	 = sizeof(particle_counters),
				.structure_count = 1,
				.flags			 = resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
				.debug_name		 = "particle_counters_gpu",
			});

		const particle_sim_count_args dummy_sim_count_args = {
			.group_sim_x   = 1,
			.group_sim_y   = 1,
			.group_sim_z   = 1,
			.group_count_x = 1,
			.group_count_y = 1,
			.group_count_z = 1,
		};

		_sim_state.sim_count_indirect_arguments.buffer_data(0, &dummy_sim_count_args, sizeof(particle_sim_count_args));

		for (u32 i = 0; i < MAX_WORLD_COMP_PARTICLE_EMITTERS; i++)
		{
			for (u32 j = 0; j < MAX_WORLD_PARTICLES_PER_EMITTER; j++)
			{
				const u32 local_j = MAX_WORLD_PARTICLES_PER_EMITTER - 1 - j;
				const u32 data	  = MAX_WORLD_PARTICLES_PER_EMITTER - j - 1;
				_sim_state.dead_indices.buffer_data((i * MAX_WORLD_PARTICLES_PER_EMITTER + j) * sizeof(u32), &data, sizeof(u32));
			}

			const particle_system_data sd = {
				.alive_count = 0,
				.dead_count	 = MAX_WORLD_PARTICLES_PER_EMITTER,
			};

			_sim_state.system_data.buffer_data(sizeof(particle_system_data) * i, &sd, sizeof(particle_system_data));

			const particle_counters counters = {};
			_sim_state.counters.buffer_data(0, &counters, sizeof(particle_counters));
		}

		// ofd
		string name = "";
		name.reserve(256);

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			pfd.cmd_buffer		   = backend->create_command_buffer({.type = command_type::graphics, .debug_name = "particles_gfx"});
			pfd.cmd_buffer_compute = backend->create_command_buffer({.type = command_type::compute, .debug_name = "particles_cmp"});
			pfd.ubo.create({.size = sizeof(ubo), .flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible, .debug_name = "particles_ubo"});

			name = "particle_render_args_gpu " + std::to_string(i);
			pfd.indirect_arguments.create(
				{
					.size		= sizeof(particle_indirect_args) * MAX_WORLD_COMP_PARTICLE_EMITTERS,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "particle_render_args",
				},
				{
					.size			 = sizeof(particle_indirect_args) * MAX_WORLD_COMP_PARTICLE_EMITTERS,
					.structure_size	 = sizeof(particle_indirect_args),
					.structure_count = MAX_WORLD_COMP_PARTICLE_EMITTERS,
					.flags			 = resource_flags::rf_indirect_buffer | resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
					.debug_name		 = name.c_str(),
				});

			name = "particle_instances_gpu " + std::to_string(i);
			pfd.instance_data.create(
				{
					.size		= sizeof(particle_instance_data) * MAX_WORLD_PARTICLES,
					.flags		= resource_flags::rf_cpu_visible,
					.debug_name = "particle_instances",
				},
				{
					.size			 = sizeof(particle_instance_data) * MAX_WORLD_PARTICLES,
					.structure_size	 = sizeof(particle_instance_data),
					.structure_count = MAX_WORLD_PARTICLES,
					.flags			 = resource_flags::rf_storage_buffer | resource_flags::rf_gpu_write | resource_flags::rf_gpu_only,
					.debug_name		 = name.c_str(),
				});
		}
		_shader_clear		= engine_resources::get().get_shader_direct(engine_resource_ident::shader_particle_clear).get_hw();
		_shader_simulate	= engine_resources::get().get_shader_direct(engine_resource_ident::shader_particle_sim).get_hw();
		_shader_emit		= engine_resources::get().get_shader_direct(engine_resource_ident::shader_particle_emit).get_hw();
		_shader_write_count = engine_resources::get().get_shader_direct(engine_resource_ident::shader_particle_write_count).get_hw();
		_shader_count		= engine_resources::get().get_shader_direct(engine_resource_ident::shader_particle_count).get_hw();
		_shader_swap		= engine_resources::get().get_shader_direct(engine_resource_ident::shader_particle_swap).get_hw();

#ifdef SFG_TOOLMODE
		engine_resources::get().add_shader_reload_listener([this](engine_resource_ident type, shader_direct& sh) {
			if (type == engine_resource_ident::shader_particle_clear)
			{
				_shader_clear = sh.get_hw();
				return;
			}
			if (type == engine_resource_ident::shader_particle_sim)
			{
				_shader_simulate = sh.get_hw();
				return;
			}

			if (type == engine_resource_ident::shader_particle_emit)
			{
				_shader_emit = sh.get_hw();
				return;
			}

			if (type == engine_resource_ident::shader_particle_write_count)
			{
				_shader_write_count = sh.get_hw();
				return;
			}

			if (type == engine_resource_ident::shader_particle_count)
			{
				_shader_count = sh.get_hw();
				return;
			}

			if (type == engine_resource_ident::shader_particle_swap)
			{
				_shader_swap = sh.get_hw();
				return;
			}
		});
#endif
	}

	void render_pass_particles::uninit()
	{
		gfx_backend* backend = gfx_backend::get();

		backend->destroy_indirect_signature(_indirect_sig_dispatch);
		backend->destroy_indirect_signature(_indirect_sig_draw);
		_alloc.uninit();

		for (u32 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			per_frame_data& pfd = _pfd[i];

			backend->destroy_command_buffer(pfd.cmd_buffer);
			backend->destroy_command_buffer(pfd.cmd_buffer_compute);
			pfd.ubo.destroy();
			pfd.indirect_arguments.destroy();
			pfd.instance_data.destroy();
		}

		_sim_state.emit_arguments.destroy();
		_sim_state.emit_counts.destroy();
		_sim_state.system_data.destroy();
		_sim_state.states.destroy();
		_sim_state.sim_count_indirect_arguments.destroy();
		_sim_state.alive_list_a.destroy();
		_sim_state.alive_list_b.destroy();
		_sim_state.dead_indices.destroy();
		_sim_state.counters.destroy();
	}

	void render_pass_particles::prepare(u8 frame_index, proxy_manager& pm, const view& main_camera_view)
	{
		ZoneScoped;
		per_frame_data& pfd = _pfd[frame_index];
		_alloc.reset();
		_draw_stream.prepare(_alloc, MAX_WORLD_COMP_PARTICLE_EMITTERS);
		_num_systems = 0;

		auto&	  particles		 = *pm.get_particle_emitters();
		const u32 peak_particles = pm.get_peak_particle_emitters();

		u32		  num_emitters	  = 0;
		const f32 particles_delta = static_cast<f32>(frame_info::get_render_thread_time_milli()) * 0.001f;

		u32				   emit_count = 0;
		bool			   emit_dead  = false;
		particle_emit_args args		  = {};

		for (u32 i = 0; i < peak_particles; i++)
		{
			render_proxy_particle_emitter& p = particles.get(i);
			if (p.status != render_proxy_status::rps_active)
				continue;

			const resource_id particle_res = p.particle_res_id;
			const resource_id material_res = p.material;

			if (particle_res == NULL_RESOURCE_ID || material_res == NULL_RESOURCE_ID)
				continue;

			const render_proxy_particle_resource& res = pm.get_particle(particle_res);
			SFG_ASSERT(res.status == render_proxy_status::rps_active);

			emit_count = 0;
			emit_dead  = false;

			const render_proxy_material_runtime& mat = pm.get_material_runtime(material_res);
			if (mat.shader_handle == NULL_RESOURCE_ID)
				continue;

			const render_proxy_shader& shader = pm.get_shader(mat.shader_handle);
			SFG_ASSERT(shader.status == render_proxy_status::rps_active);

			const gfx_id target_shader = pm.get_shader_variant(shader.handle, 0);
			SFG_ASSERT(target_shader != NULL_GFX_ID);

			_draw_stream.add_command({
				.system_index		  = num_emitters,
				.material_index		  = mat.gpu_index_buffers[frame_index],
				.texture_buffer_index = mat.gpu_index_texture_buffers[frame_index],
				.pipeline_hw		  = target_shader,
			});

			const particle_emit_properties& ep = res.emit_props;

			if (!math::almost_equal(ep.spawn.emitter_lifetime, 0.0f))
			{
				if (p.current_life > ep.spawn.emitter_lifetime)
					emit_dead = true;
			}

			if (!emit_dead)
			{
				if (math::almost_equal(ep.spawn.wait_between_emits, 0.0f))
				{
					// emits constantly.
					emit_count = static_cast<u32>(random::random_int(static_cast<int>(ep.spawn.min_particle_count), static_cast<int>(ep.spawn.max_particle_count)));
				}
				else
				{
					// emit at start.
					if (math::almost_equal(p.last_emitted, 0.0f))
					{
						emit_count = static_cast<u32>(random::random_int(static_cast<int>(ep.spawn.min_particle_count), static_cast<int>(ep.spawn.max_particle_count)));
						p.last_emitted += particles_delta;
					}
					else if (p.current_life - p.last_emitted > ep.spawn.wait_between_emits)
					{
						// emit in bursts
						p.last_emitted = p.current_life;
						emit_count	   = static_cast<u32>(random::random_int(static_cast<int>(ep.spawn.min_particle_count), static_cast<int>(ep.spawn.max_particle_count)));
					}
				}
				p.current_life += particles_delta;
			}

			_sim_state.emit_counts.buffer_data(sizeof(u32) * num_emitters, &emit_count, sizeof(u32));

			const render_proxy_entity& e = pm.get_entity(p.entity);
			SFG_ASSERT(e.status == render_proxy_status::rps_active);
			const vector3 base_pos = e.model.get_translation();

			const u8 local_vel = ep.velocity.is_local;

			const vector3 min_start_vel = local_vel ? e.rotation * ep.velocity.min_start : ep.velocity.min_start;
			const vector3 max_start_vel = local_vel ? e.rotation * ep.velocity.max_start : ep.velocity.max_start;
			const vector3 min_mid_vel	= local_vel ? e.rotation * ep.velocity.min_mid : ep.velocity.min_mid;
			const vector3 max_mid_vel	= local_vel ? e.rotation * ep.velocity.max_mid : ep.velocity.max_mid;
			const vector3 min_end_vel	= local_vel ? e.rotation * ep.velocity.min_end : ep.velocity.min_end;
			const vector3 max_end_vel	= local_vel ? e.rotation * ep.velocity.max_end : ep.velocity.max_end;

			args = {
				.integrate_points = vector4(ep.velocity.integrate_point, ep.color_settings.integrate_point_opacity, ep.rotation.integrate_point_angular_velocity, ep.size.integrate_point),
				.opacity_points	  = vector4(ep.color_settings.min_start_opacity, ep.color_settings.max_start_opacity, ep.color_settings.mid_opacity, ep.color_settings.end_opacity),
				.size_points	  = vector4(ep.size.min_start, ep.size.max_start, ep.size.mid, ep.size.end),
				.min_lifetime	  = ep.spawn.min_lifetime,
				.max_lifetime	  = ep.spawn.max_lifetime,

				.min_pos_x	 = base_pos.x + ep.pos.min_start.x,
				.min_pos_y	 = base_pos.y + ep.pos.min_start.y,
				.min_pos_z	 = base_pos.z + ep.pos.min_start.z,
				.max_pos_x	 = base_pos.x + ep.pos.max_start.x,
				.max_pos_y	 = base_pos.y + ep.pos.max_start.y,
				.max_pos_z	 = base_pos.z + ep.pos.max_start.z,
				.cone_radius = ep.pos.cone_radius,

				.min_start_vel_x = min_start_vel.x,
				.min_start_vel_y = min_start_vel.y,
				.min_start_vel_z = min_start_vel.z,
				.max_start_vel_x = max_start_vel.x,
				.max_start_vel_y = max_start_vel.y,
				.max_start_vel_z = max_start_vel.z,

				.min_mid_vel_x = min_mid_vel.x,
				.min_mid_vel_y = min_mid_vel.y,
				.min_mid_vel_z = min_mid_vel.z,
				.max_mid_vel_x = max_mid_vel.x,
				.max_mid_vel_y = max_mid_vel.y,
				.max_mid_vel_z = max_mid_vel.z,

				.min_end_vel_x = min_end_vel.x,
				.min_end_vel_y = min_end_vel.y,
				.min_end_vel_z = min_end_vel.z,
				.max_end_vel_x = max_end_vel.x,
				.max_end_vel_y = max_end_vel.y,
				.max_end_vel_z = max_end_vel.z,

				.min_col_x			 = ep.color_settings.min_start.x,
				.min_col_y			 = ep.color_settings.min_start.y,
				.min_col_z			 = ep.color_settings.min_start.z,
				.max_col_x			 = ep.color_settings.max_start.x,
				.max_col_y			 = ep.color_settings.max_start.y,
				.max_col_z			 = ep.color_settings.max_start.z,
				.mid_col_x			 = ep.color_settings.mid_color.x,
				.mid_col_y			 = ep.color_settings.mid_color.y,
				.mid_col_z			 = ep.color_settings.mid_color.z,
				.end_col_x			 = ep.color_settings.end_color.x,
				.end_col_y			 = ep.color_settings.end_color.y,
				.end_col_z			 = ep.color_settings.end_color.z,
				.col_integrate_point = ep.color_settings.integrate_point_color,

				.min_start_rotation			= ep.rotation.min_start_rotation * DEG_2_RAD,
				.max_start_rotation			= ep.rotation.max_start_rotation * DEG_2_RAD,
				.min_start_angular_velocity = ep.rotation.min_start_angular_velocity * DEG_2_RAD,
				.max_start_angular_velocity = ep.rotation.max_start_angular_velocity * DEG_2_RAD,
				.min_end_angular_velocity	= ep.rotation.min_end_angular_velocity * DEG_2_RAD,
				.max_end_angular_velocity	= ep.rotation.max_end_angular_velocity * DEG_2_RAD,
			};

			_sim_state.emit_arguments.buffer_data(sizeof(particle_emit_args) * num_emitters, &args, sizeof(particle_emit_args));

			num_emitters++;
		}

		_draw_stream.build();

		gfx_backend* backend = gfx_backend::get();

		const gfx_id cmd_buffer = pfd.cmd_buffer_compute;

		backend->reset_command_buffer(cmd_buffer);

		static_vector<barrier, 8> barriers;

		if (_sim_state.buffers_init == 0)
		{
			_sim_state.sim_count_indirect_arguments.copy(cmd_buffer);
			_sim_state.dead_indices.copy(cmd_buffer);
			_sim_state.system_data.copy(cmd_buffer);
			_sim_state.counters.copy(cmd_buffer);
		}

		if (num_emitters != 0)
		{
			_sim_state.emit_arguments.copy_region(cmd_buffer, 0, num_emitters * sizeof(particle_emit_args));
			_sim_state.emit_counts.copy_region(cmd_buffer, 0, num_emitters * sizeof(u32));
		}

		if (!barriers.empty())
			backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});

		barriers.resize(0);

		if (_sim_state.buffers_init == 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_indirect_arg,
				.resource	 = _sim_state.sim_count_indirect_arguments.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});
			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_common,
				.resource	 = _sim_state.dead_indices.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});
			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_common,
				.resource	 = _sim_state.system_data.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});
			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_common,
				.resource	 = _sim_state.counters.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});
		}

		_sim_state.buffers_init = 1;

		if (num_emitters != 0)
		{
			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_non_ps_resource,
				.resource	 = _sim_state.emit_arguments.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});
			barriers.push_back({
				.from_states = resource_state::resource_state_copy_dest,
				.to_states	 = resource_state::resource_state_non_ps_resource,
				.resource	 = _sim_state.emit_counts.get_gpu(),
				.flags		 = barrier_flags::baf_is_resource,
			});
		}

		if (!barriers.empty())
			backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});

		const world_id main_cam	   = pm.get_main_camera();
		vector3		   cam_forward = vector3::zero;

		if (main_cam != NULL_WORLD_ID)
		{
			const render_proxy_camera& cam		  = pm.get_camera(main_cam);
			const render_proxy_entity& cam_entity = pm.get_entity(cam.entity);
			cam_forward							  = cam_entity.rotation.get_forward();
		}

		const ubo ubo_data = {
			.view_proj				  = main_camera_view.view_proj_matrix,
			.cam_pos_and_delta		  = vector4(main_camera_view.position.x, main_camera_view.position.y, main_camera_view.position.z, particles_delta),
			.cam_dir				  = vector4(cam_forward.x, cam_forward.y, cam_forward.z, 0.0f),
			.max_particles_per_system = MAX_WORLD_PARTICLES_PER_EMITTER,
			.frame_index			  = static_cast<u32>(frame_info::get_frame() % UINT32_MAX),
			.max_systems			  = MAX_WORLD_COMP_PARTICLE_EMITTERS,
			.num_systems			  = num_emitters,
		};

		pfd.ubo.buffer_data(0, &ubo_data, sizeof(ubo));
		_num_systems = num_emitters;
	}

	void render_pass_particles::compute(const compute_params& p)
	{
		ZoneScoped;
		gfx_backend*	backend	   = gfx_backend::get();
		per_frame_data& pfd		   = _pfd[p.frame_index];
		const gfx_id	cmd_buffer = pfd.cmd_buffer_compute;
		if (_num_systems == 0)
		{
			backend->close_command_buffer(cmd_buffer);
			return;
		}

		backend->cmd_bind_layout_compute(cmd_buffer, {.layout = p.global_layout_compute});
		backend->cmd_bind_group_compute(cmd_buffer, {.group = p.global_group});

		const u8 frame_switch = _sim_state.frame_switch;
		_sim_state.frame_switch ^= 1;

		const gpu_index gpu_index_ubo					  = pfd.ubo.get_index();
		const gpu_index gpu_index_indirect_args			  = pfd.indirect_arguments.get_index();
		const gpu_index gpu_index_alive_list_a_uav		  = frame_switch == 0 ? _sim_state.alive_list_a.get_index() : _sim_state.alive_list_b.get_index();
		const gpu_index gpu_index_alive_list_b_uav		  = frame_switch == 0 ? _sim_state.alive_list_b.get_index() : _sim_state.alive_list_a.get_index();
		const gpu_index gpu_index_counters_uav			  = _sim_state.counters.get_index();
		const gpu_index gpu_index_states				  = _sim_state.states.get_index();
		const gpu_index gpu_index_system_data_uav		  = _sim_state.system_data.get_index();
		const gpu_index gpu_index_dead_list_uav			  = _sim_state.dead_indices.get_index();
		const gpu_index gpu_index_emit_counts			  = _sim_state.emit_counts.get_index();
		const gpu_index gpu_index_emit_args				  = _sim_state.emit_arguments.get_index();
		const gpu_index gpu_index_sim_count_indirect_args = _sim_state.sim_count_indirect_arguments.get_index();
		const gpu_index gpu_index_instances_srv			  = pfd.instance_data.get_index();
		const gpu_index gpu_index_instances_uav			  = pfd.instance_data.get_index_secondary();
		const gfx_id	hw_sim_count_indirect			  = _sim_state.sim_count_indirect_arguments.get_gpu();
		const gfx_id	hw_render_indirect				  = pfd.indirect_arguments.get_gpu();
		const gfx_id	hw_counters						  = _sim_state.counters.get_gpu();
		const gfx_id	hw_system_data					  = _sim_state.system_data.get_gpu();
		const gfx_id	hw_states						  = _sim_state.states.get_gpu();
		const gfx_id	hw_dead_indices					  = _sim_state.dead_indices.get_gpu();
		const gfx_id	hw_alive_list_a					  = frame_switch ? _sim_state.alive_list_a.get_gpu() : _sim_state.alive_list_b.get_gpu();
		const gfx_id	hw_alive_list_b					  = frame_switch ? _sim_state.alive_list_b.get_gpu() : _sim_state.alive_list_a.get_gpu();
		const gfx_id	hw_instances					  = pfd.instance_data.get_gpu();

		static_vector<barrier, 16> barriers;
		barriers.push_back({
			.from_states = resource_state::resource_state_indirect_arg,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_render_indirect,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_counters,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_common,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_system_data,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_states,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_dead_indices,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_alive_list_a,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_alive_list_b,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_common,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_instances,
			.flags		 = barrier_flags::baf_is_resource,
		});
		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "particle_clear");

		// pass 0 - clear
		{
			const gpu_index constants[2] = {gpu_index_ubo, gpu_index_indirect_args};
			backend->cmd_bind_constants_compute(cmd_buffer, {.data = (u8*)&constants, .offset = constant_index_rp_constant0, .count = 2, .param_index = rpi_constants});
		}
		backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = _shader_clear});
		backend->cmd_dispatch(cmd_buffer,
							  {
								  .group_size_x = (_num_systems / 64) + 1,
								  .group_size_y = 1,
								  .group_size_z = 1,
							  });

		END_DEBUG_EVENT(backend, cmd_buffer);

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "particle_sim");

		// pass 1 - simulate
		{
			const gpu_index constants[7] = {
				gpu_index_ubo,
				gpu_index_alive_list_a_uav,
				gpu_index_alive_list_b_uav,
				gpu_index_counters_uav,
				gpu_index_states,
				gpu_index_system_data_uav,
				gpu_index_dead_list_uav,
			};
			backend->cmd_bind_constants_compute(cmd_buffer, {.data = (u8*)&constants, .offset = constant_index_rp_constant0, .count = 7, .param_index = rpi_constants});
		}

		backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = _shader_simulate});
		backend->cmd_execute_indirect(cmd_buffer,
									  {
										  .indirect_buffer		  = hw_sim_count_indirect,
										  .indirect_buffer_offset = 0,
										  .count				  = 1,
										  .indirect_signature	  = _indirect_sig_dispatch,
									  });

		END_DEBUG_EVENT(backend, cmd_buffer);

		// pass 1 simulate might write to below, so uav-uav
		barriers.resize(0);
		barriers.push_back({
			.resource = hw_counters,
			.flags	  = barrier_flags::baf_is_resource | barrier_flags::baf_is_uav,
		});
		barriers.push_back({
			.resource = hw_states,
			.flags	  = barrier_flags::baf_is_resource | barrier_flags::baf_is_uav,
		});
		barriers.push_back({
			.resource = hw_system_data,
			.flags	  = barrier_flags::baf_is_resource | barrier_flags::baf_is_uav,
		});
		barriers.push_back({
			.resource = hw_dead_indices,
			.flags	  = barrier_flags::baf_is_resource | barrier_flags::baf_is_uav,
		});
		barriers.push_back({
			.resource = hw_alive_list_b,
			.flags	  = barrier_flags::baf_is_resource | barrier_flags::baf_is_uav,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_indirect_arg,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_sim_count_indirect,
			.flags		 = barrier_flags::baf_is_resource,
		});

		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "particle_emit");

		// pass 2 - emit
		{
			const gpu_index constants[8] = {
				gpu_index_ubo,
				gpu_index_emit_counts,
				gpu_index_states,
				gpu_index_dead_list_uav,
				gpu_index_emit_args,
				gpu_index_system_data_uav,
				gpu_index_alive_list_b_uav,
				gpu_index_counters_uav,
			};
			backend->cmd_bind_constants_compute(cmd_buffer, {.data = (u8*)&constants, .offset = constant_index_rp_constant0, .count = 8, .param_index = rpi_constants});
		}

		backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = _shader_emit});
		backend->cmd_dispatch(cmd_buffer,
							  {
								  .group_size_x = 1,
								  .group_size_y = _num_systems == 0 ? 1 : _num_systems,
								  .group_size_z = 1,
							  });

		END_DEBUG_EVENT(backend, cmd_buffer);

		// we are done writing to counters for now, will be read as srv in in count passes.
		barriers.resize(0);
		barriers.push_back({
			.resource = hw_counters,
			.flags	  = barrier_flags::baf_is_resource | barrier_flags::baf_is_uav,
		});

		barriers.push_back({
			.resource = hw_alive_list_b,
			.flags	  = barrier_flags::baf_is_resource | barrier_flags::baf_is_uav,
		});
		barriers.push_back({
			.resource = hw_states,
			.flags	  = barrier_flags::baf_is_resource | barrier_flags::baf_is_uav,
		});
		barriers.push_back({
			.resource = hw_system_data,
			.flags	  = barrier_flags::baf_is_resource | barrier_flags::baf_is_uav,
		});

		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "particle_write_count");

		// pass 3 - write counts
		{
			const gpu_index constants[2] = {
				gpu_index_counters_uav,
				gpu_index_sim_count_indirect_args,
			};
			backend->cmd_bind_constants_compute(cmd_buffer, {.data = (u8*)&constants, .offset = constant_index_rp_constant0, .count = 2, .param_index = rpi_constants});
		}

		backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = _shader_write_count});
		backend->cmd_dispatch(cmd_buffer,
							  {
								  .group_size_x = 1,
								  .group_size_y = 1,
								  .group_size_z = 1,
							  });

		END_DEBUG_EVENT(backend, cmd_buffer);

		// wrote to sim and counts indirect arguments.
		barriers.resize(0);
		barriers.push_back({
			.resource = hw_counters,
			.flags	  = barrier_flags::baf_is_resource | barrier_flags::baf_is_uav,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_indirect_arg,
			.resource	 = hw_sim_count_indirect,
			.flags		 = barrier_flags::baf_is_resource,
		});

		// was written by pass 0.
		barriers.push_back({
			.resource = hw_render_indirect,
			.flags	  = barrier_flags::baf_is_resource | barrier_flags::baf_is_uav,
		});

		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "particle_count");

		// pass 4 - counts
		{
			const gpu_index constants[6] = {
				gpu_index_ubo,
				gpu_index_counters_uav,
				gpu_index_alive_list_b_uav,
				gpu_index_states,
				gpu_index_indirect_args,
				gpu_index_instances_uav,
			};
			backend->cmd_bind_constants_compute(cmd_buffer, {.data = (u8*)&constants, .offset = constant_index_rp_constant0, .count = 6, .param_index = rpi_constants});
		}
		backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = _shader_count});
		backend->cmd_execute_indirect(cmd_buffer,
									  {
										  .indirect_buffer		  = hw_sim_count_indirect,
										  .indirect_buffer_offset = sizeof(u32) * 3,
										  .count				  = 1,
										  .indirect_signature	  = _indirect_sig_dispatch,
									  });

		END_DEBUG_EVENT(backend, cmd_buffer);

		barriers.resize(0);
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_indirect_arg,
			.resource	 = hw_render_indirect,
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.from_states = resource_state::resource_state_indirect_arg,
			.to_states	 = resource_state::resource_state_uav,
			.resource	 = hw_sim_count_indirect,
			.flags		 = barrier_flags::baf_is_resource,
		});

		barriers.push_back({
			.resource = hw_counters,
			.flags	  = barrier_flags::baf_is_resource | barrier_flags::baf_is_uav,
		});

		// for graphics queue.
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_common,
			.resource	 = hw_instances,
			.flags		 = barrier_flags::baf_is_resource,
		});

		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "particle_swap");

		// pass 5 - swap
		{
			const gpu_index constants[2] = {
				gpu_index_counters_uav,
				gpu_index_sim_count_indirect_args,
			};
			backend->cmd_bind_constants_compute(cmd_buffer, {.data = (u8*)&constants, .offset = constant_index_rp_constant0, .count = 2, .param_index = rpi_constants});
		}
		backend->cmd_bind_pipeline_compute(cmd_buffer, {.pipeline = _shader_swap});
		backend->cmd_dispatch(cmd_buffer,
							  {
								  .group_size_x = 1,
								  .group_size_y = 1,
								  .group_size_z = 1,
							  });

		END_DEBUG_EVENT(backend, cmd_buffer);

		barriers.resize(0);
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_indirect_arg,
			.resource	 = hw_sim_count_indirect,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_non_ps_resource,
			.resource	 = hw_counters,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_common,
			.resource	 = hw_instances,
			.flags		 = barrier_flags::baf_is_resource,
		});
		barriers.push_back({
			.from_states = resource_state::resource_state_uav,
			.to_states	 = resource_state::resource_state_common,
			.resource	 = hw_system_data,
			.flags		 = barrier_flags::baf_is_resource,
		});
		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});

		backend->close_command_buffer(cmd_buffer);
	}

	void render_pass_particles::render(const render_params& p)
	{
		ZoneScoped;
		gfx_backend*	backend	   = gfx_backend::get();
		per_frame_data& pfd		   = _pfd[p.frame_index];
		const gfx_id	cmd_buffer = pfd.cmd_buffer;

		backend->reset_command_buffer(cmd_buffer);

		if (_num_systems == 0)
		{
			backend->close_command_buffer(cmd_buffer);
			return;
		}

		const render_pass_color_attachment att = {
			.clear_color = vector4(0, 0, 0, 1.0f),
			.texture	 = p.hw_lighting,
			.load_op	 = load_op::load,
			.store_op	 = store_op::store,
		};

		BEGIN_DEBUG_EVENT(backend, cmd_buffer, "particle_render");

		backend->cmd_begin_render_pass_depth_read_only(cmd_buffer,
													   {
														   .color_attachments = &att,
														   .depth_stencil_attachment =
															   {
																   .texture		   = p.hw_depth,
																   .depth_load_op  = load_op::load,
																   .depth_store_op = store_op::store,
																   .view_index	   = 1,
															   },
														   .color_attachment_count = 1,
													   });

		backend->cmd_set_scissors(cmd_buffer, {.width = static_cast<u16>(p.size.x), .height = static_cast<u16>(p.size.y)});
		backend->cmd_set_viewport(cmd_buffer,
								  {
									  .min_depth = 0.0f,
									  .max_depth = 1.0f,
									  .width	 = static_cast<u16>(p.size.x),
									  .height	 = static_cast<u16>(p.size.y),

								  });

		backend->cmd_bind_layout(cmd_buffer, {.layout = p.global_layout});
		backend->cmd_bind_group(cmd_buffer, {.group = p.global_group});

		const gpu_index gpu_index_ubo		= pfd.ubo.get_index();
		const gpu_index gpu_index_instances = pfd.instance_data.get_index();
		const gfx_id	hw_instances		= pfd.instance_data.get_gpu();
		const gfx_id	hw_render_indirect	= pfd.indirect_arguments.get_gpu();

		static_vector<barrier, 4> barriers;
		barriers.push_back({
			.from_states = resource_state::resource_state_common,
			.to_states	 = resource_state::resource_state_non_ps_resource,
			.resource	 = hw_instances,
			.flags		 = barrier_flags::baf_is_resource,
		});

		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});

		const u32 constants[3] = {gpu_index_ubo, gpu_index_instances, 0};
		backend->cmd_bind_constants(cmd_buffer, {.data = (u8*)&constants, .offset = constant_index_rp_constant0, .count = 3, .param_index = rpi_constants});

		if (_num_systems != 0)
		{
			_draw_stream.draw(cmd_buffer, hw_render_indirect, _indirect_sig_draw, sizeof(indirect_render), MAX_WORLD_PARTICLES_PER_EMITTER);
		}

		backend->cmd_end_render_pass(cmd_buffer, {});

		END_DEBUG_EVENT(backend, cmd_buffer);

		barriers.resize(0);
		barriers.push_back({
			.from_states = resource_state::resource_state_non_ps_resource,
			.to_states	 = resource_state::resource_state_common,
			.resource	 = hw_instances,
			.flags		 = barrier_flags::baf_is_resource,
		});

		backend->cmd_barrier(cmd_buffer, {.barriers = barriers.data(), .barrier_count = static_cast<u16>(barriers.size())});

		backend->close_command_buffer(cmd_buffer);
	}

}
