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

#include "proxy_manager.hpp"
#include "resources/common_resources.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "gfx/event_stream/render_events_entity.hpp"
#include "gfx/event_stream/render_events_trait.hpp"
#include "gfx/texture_queue.hpp"
#include "gfx/buffer_queue.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/backend/backend.hpp"
#include "gfx/util/gfx_util.hpp"
#include "gfx/common/render_target_definitions.hpp"
#include "common/system_info.hpp"
#include "data/istream.hpp"
#include "math/math.hpp"
#include "gui/vekt.hpp"
#include "platform/time.hpp"
#include <tracy/Tracy.hpp>

namespace SFG
{

	proxy_manager::proxy_manager(buffer_queue& b, texture_queue& t) : _buffer_queue(b), _texture_queue(t)
	{
		_textures			= new textures_type();
		_samplers			= new samplers_type();
		_materials			= new materials_type();
		_material_runtimes	= new material_runtimes_type();
		_shaders			= new shaders_type();
		_meshes				= new meshes_type();
		_skins				= new skins_type();
		_entities			= new entity_type();
		_mesh_instances		= new mesh_instances_type();
		_cameras			= new cameras_type();
		_ambients			= new ambients_type();
		_blooms				= new blooms_type();
		_ssaos				= new ssaos_type();
		_post_processes		= new post_processes_type();
		_skyboxes			= new skyboxes_type();
		_point_lights		= new point_lights_type();
		_spot_lights		= new spot_lights_type();
		_dir_lights			= new dir_lights_type();
		_canvases			= new canvas_type();
		_emitters			= new emitters_type();
		_sprites			= new sprites_type();
		_material_updates	= new material_updates_type();
		_particle_resources = new particle_res_type();
	}

	proxy_manager::~proxy_manager()
	{
		delete _textures;
		delete _samplers;
		delete _materials;
		delete _material_runtimes;
		delete _shaders;
		delete _meshes;
		delete _skins;
		delete _entities;
		delete _mesh_instances;
		delete _cameras;
		delete _ambients;
		delete _blooms;
		delete _ssaos;
		delete _post_processes;
		delete _skyboxes;
		delete _point_lights;
		delete _spot_lights;
		delete _dir_lights;
		delete _canvases;
		delete _emitters;
		delete _sprites;
		delete _material_updates;
		delete _particle_resources;
	}

	void proxy_manager::reset()
	{
		_textures->reset();
		_samplers->reset();
		_materials->reset();
		_material_runtimes->reset();
		_shaders->reset();
		_meshes->reset();
		_skins->reset();
		_entities->reset();
		_mesh_instances->reset();
		_cameras->reset();
		_ambients->reset();
		_blooms->reset();
		_ssaos->reset();
		_post_processes->reset();
		_skyboxes->reset();
		_point_lights->reset();
		_spot_lights->reset();
		_dir_lights->reset();
		_canvases->reset();
		_emitters->reset();
		_sprites->reset();
		_particle_resources->reset();
		_material_updates->resize(0);
		_ambient_exists		 = 0;
		_bloom_exists		 = 0;
		_ssao_exists		 = 0;
		_post_process_exists = 0;
		_skybox_exists		 = 0;
		_peak_sprites		 = 0;
	}

	void proxy_manager::init()
	{
		_destroy_list.reserve(1024);

		_aux_memory.init(1024 * 1024 * 2);
	}

	void proxy_manager::uninit()
	{
		auto& textures	= *_textures;
		auto& samplers	= *_samplers;
		auto& materials = *_materials;
		auto& shaders	= *_shaders;
		auto& meshes	= *_meshes;

		for (auto& res : *_textures)
		{
			if (res.status == 0)
				continue;

			destroy_texture(res);
		}

		for (auto& res : samplers)
		{
			if (res.status == 0)
				continue;

			destroy_sampler(res);
		}

		for (auto& res : shaders)
		{
			if (res.status == 0)
				continue;

			destroy_shader(res);
		}

		for (auto& res : materials)
		{
			if (res.status == 0)
				continue;

			destroy_material(res);
		}

		for (auto& res : meshes)
		{
			if (res.status == 0)
				continue;

			destroy_mesh(res);
		}

		flush_destroys(true);

		reset();

		_aux_memory.uninit();
	}

	void proxy_manager::fetch_render_events(render_event_stream& stream, u8 frame_index)
	{
		ZoneScoped;

		u32		peak = 0;
		istream in;
		stream.read(&_entities->get(0), peak, in);
		_peak_entities = math::max(_peak_entities, peak);

		if (in.get_size() == 0)
			return;

		render_event_header header = {};

		while (!in.is_eof())
		{
			header.deserialize(in);
			process_event(header, in, frame_index);
		}

		u8 all_clear = 1;
		for (material_update& u : *_material_updates)
		{
			if (u.count >= BACK_BUFFER_COUNT)
				continue;
			all_clear = 0;

			if (frame_index != u.count)
				continue;

			render_proxy_material& mat		 = get_material(u.material_id);
			const size_t		   padding	 = static_cast<size_t>(u.padding);
			const u8*			   data		 = _aux_memory.get<u8>(u.data);
			const size_t		   data_size = static_cast<size_t>(u.data.size);

			if (u.is_texture)
				mat.texture_buffers[frame_index].buffer_data(padding, data, data_size);
			else
				mat.buffers[frame_index].buffer_data(padding, data, data_size);

			u.count++;
			if (u.count == BACK_BUFFER_COUNT)
				_aux_memory.free(u.data);
		}

		if (all_clear)
			_material_updates->resize(0);
	}

	void proxy_manager::flush_destroys(bool force)
	{
		gfx_backend* backend = gfx_backend::get();

		const u64 frame = frame_info::get_render_frame();
		if (!force && frame < BACK_BUFFER_COUNT + 1)
			return;

		bool waiting = false;

		for (destroy_data& dd : _destroy_list)
		{
			if (dd.destroyed)
				continue;

			if (!force && dd.target_frame > frame)
			{
				waiting = true;
				continue;
			}

			dd.destroyed = true;

			if (dd.type == destroy_data_type::texture)
				backend->destroy_texture(dd.id);
			else if (dd.type == destroy_data_type::sampler)
				backend->destroy_sampler(dd.id);
			else if (dd.type == destroy_data_type::bind_group)
				backend->destroy_bind_group(dd.id);
			else if (dd.type == destroy_data_type::resource)
				backend->destroy_resource(dd.id);
			else if (dd.type == destroy_data_type::shader)
				backend->destroy_shader(dd.id);
		}

		if (!waiting)
			_destroy_list.resize(0);
	}

	gfx_id proxy_manager::get_shader_variant(resource_id shader_handle, u32 flags)
	{
		const render_proxy_shader&		   proxy_shader	  = get_shader(shader_handle);
		const u32						   variants_count = proxy_shader.variant_count;
		const render_proxy_shader_variant* var			  = _aux_memory.get<const render_proxy_shader_variant>(proxy_shader.variants);

		for (u32 i = 0; i < variants_count; i++)
		{
			const render_proxy_shader_variant& v = var[i];
			if (v.variant_flags.is_all_set(flags))
			{
				return v.hw;
			}
		}

		return NULL_GFX_ID;
	}

	void proxy_manager::process_event(const render_event_header& header, istream& stream, u8 frame_index)
	{
		gfx_backend* backend = gfx_backend::get();

		const world_id			index = header.index;
		const render_event_type type  = header.event_type;

		if (type == render_event_type::create_canvas)
		{
			render_event_canvas ev = {};
			ev.deserialize(stream);

			render_proxy_canvas& proxy = get_canvas(index);
			proxy.entity			   = ev.entity_index;
			proxy.status			   = render_proxy_status::rps_active;
			proxy.is_3d				   = ev.is_3d;

			for (u8 i = 0; i < BACK_BUFFER_COUNT; i++)
			{
				proxy.vertex_buffers[i].create(
					{
						.size		= ev.vertex_size,
						.flags		= resource_flags::rf_cpu_visible,
						.debug_name = "canvas_vertex",
					},
					{
						.size		= ev.vertex_size,
						.flags		= resource_flags::rf_vertex_buffer | resource_flags::rf_gpu_only,
						.debug_name = "canvas_vertex_gpu",
					});

				proxy.index_buffers[i].create(
					{
						.size		= ev.index_size,
						.flags		= resource_flags::rf_cpu_visible,
						.debug_name = "canvas_index",
					},
					{
						.size		= ev.index_size,
						.flags		= resource_flags::rf_index_buffer | resource_flags::rf_gpu_only,
						.debug_name = "canvas_index_index",
					});
			}

			proxy._draw_calls.reserve(1024);
		}

		else if (type == render_event_type::destroy_canvas)
		{
			render_proxy_canvas& proxy = get_canvas(index);
			destroy_canvas(proxy);
		}
		else if (type == render_event_type::canvas_reset_draws)
		{
			render_proxy_canvas& proxy = get_canvas(index);
			proxy._draw_calls.resize(0);
			proxy._max_vertex_offset = 0;
			proxy._max_index_offset	 = 0;
		}
		else if (type == render_event_type::canvas_update)
		{
			render_event_canvas ev = {};
			ev.deserialize(stream);

			render_proxy_canvas& proxy = get_canvas(index);
			proxy.is_3d				   = ev.is_3d;
		}
		else if (type == render_event_type::canvas_add_draw)
		{
			render_event_canvas_add_draw ev = {};
			ev.deserialize(stream);

			render_proxy_canvas& proxy = get_canvas(index);
			proxy._draw_calls.push_back({
				.clip		  = ev.clip,
				.start_index  = ev.start_index,
				.start_vertex = ev.start_vertex,
				.index_count  = ev.index_count,
				.mat_id		  = ev.material_handle,
				.atlas_id	  = ev.atlas_handle,
				.atlas_exists = ev.atlas_exists,
			});

			proxy.vertex_buffers[frame_index].buffer_data(proxy._max_vertex_offset, ev.vertex_data, ev.vertex_data_size);
			proxy.index_buffers[frame_index].buffer_data(proxy._max_index_offset, ev.index_data, ev.index_data_size);
			proxy._max_vertex_offset += ev.vertex_data_size;
			proxy._max_index_offset += ev.index_data_size;

			_peak_canvases = math::max(_peak_canvases, index);
		}
		else if (type == render_event_type::create_entity)
		{
			render_proxy_entity& proxy = get_entity(index);
			proxy.status			   = render_proxy_status::rps_active;
		}
		else if (type == render_event_type::remove_entity)
		{
			render_proxy_entity& proxy = get_entity(index);
			proxy.status			   = render_proxy_status::rps_inactive;
		}
		else if (type == render_event_type::update_entity_flags)
		{
			render_event_entity_flags ev = {};
			ev.deserialize(stream);
			render_proxy_entity& proxy = get_entity(index);
			proxy.status			   = render_proxy_status::rps_active;
			proxy.flags.set(render_proxy_entity_flags::render_proxy_entity_invisible, !ev.is_visible);
			proxy.flags.set(render_proxy_entity_flags::render_proxy_entity_is_template, ev.is_template);
		}
		else if (type == render_event_type::update_ambient)
		{
			render_event_ambient ev = {};
			ev.deserialize(stream);
			_ambient_exists = 1;

			render_proxy_ambient& proxy = get_ambient();
			proxy.status				= render_proxy_status::rps_active;
			proxy.entity				= ev.entity_index;
			proxy.base_color			= ev.base_color;
		}
		else if (type == render_event_type::update_bloom)
		{
			render_event_bloom ev = {};
			ev.deserialize(stream);
			_bloom_exists = 1;

			render_proxy_bloom& proxy = get_bloom();
			proxy.status			  = render_proxy_status::rps_active;
			proxy.entity			  = ev.entity_index;
			proxy.filter_radius		  = ev.filter_radius;
		}
		else if (type == render_event_type::update_ssao)
		{
			render_event_ssao ev = {};
			ev.deserialize(stream);
			_ssao_exists = 1;

			render_proxy_ssao& proxy  = get_ssao();
			proxy.status			  = render_proxy_status::rps_active;
			proxy.entity			  = ev.entity_index;
			proxy.radius_world		  = ev.radius_world;
			proxy.bias				  = ev.bias;
			proxy.intensity			  = ev.intensity;
			proxy.power				  = ev.power;
			proxy.num_dirs			  = ev.num_dirs;
			proxy.num_steps			  = ev.num_steps;
			proxy.random_rot_strength = ev.random_rot_strength;
		}
		else if (type == render_event_type::update_post_process)
		{
			render_event_post_process ev = {};
			ev.deserialize(stream);
			_post_process_exists = 1;

			render_proxy_post_process& proxy = get_post_process();
			proxy.status					 = render_proxy_status::rps_active;
			proxy.entity					 = ev.entity_index;
			proxy.bloom_strength			 = ev.bloom_strength;
			proxy.exposure					 = ev.exposure;
			proxy.tonemap_mode				 = ev.tonemap_mode;
			proxy.saturation				 = ev.saturation;
			proxy.wb_temp					 = ev.wb_temp;
			proxy.wb_tint					 = ev.wb_tint;
			proxy.reinhard_white_point		 = ev.reinhard_white_point;
		}
		else if (type == render_event_type::update_skybox)
		{
			render_event_skybox ev = {};
			ev.deserialize(stream);
			_skybox_exists = 1;

			render_proxy_skybox& proxy = get_skybox();
			proxy.status			   = render_proxy_status::rps_active;
			proxy.entity			   = ev.entity_index;
			proxy.start_color		   = ev.start_color;
			proxy.mid_color			   = ev.mid_color;
			proxy.end_color			   = ev.end_color;
			proxy.fog_color			   = ev.fog_color;
			proxy.fog_start			   = ev.fog_start;
			proxy.fog_end			   = ev.fog_end;
		}
		else if (type == render_event_type::update_spot_light)
		{
			render_event_spot_light ev = {};
			ev.deserialize(stream);
			_peak_spot_lights = math::max(_peak_spot_lights, index);

			render_proxy_spot_light& proxy = _spot_lights->get(index);

			if (proxy.status != render_proxy_status::rps_active)
				_count_spot_lights++;

			const vector2ui16 pre_res = proxy.shadow_res;
			proxy.status			  = render_proxy_status::rps_active;
			proxy.entity			  = ev.entity_index;
			proxy.base_color		  = ev.base_color;
			proxy.intensity			  = ev.intensity;
			proxy.near_plane		  = ev.near_plane;
			proxy.inner_cone		  = ev.inner_cone;
			proxy.outer_cone		  = ev.outer_cone;
			proxy.cast_shadows		  = ev.cast_shadows;
			proxy.shadow_res		  = ev.shadow_resolution;

			if (proxy.shadow_res.x < 16)
				proxy.shadow_res.x = 16;
			if (proxy.shadow_res.y < 16)
				proxy.shadow_res.y = 16;

			if (proxy.near_plane < 0.001f)
				proxy.near_plane = 0.001f;

			auto create_shadow_texture = [&]() {
				// create textures
				vector<view_desc> views;

				views.push_back({
					.type			= view_type::depth_stencil,
					.base_arr_level = 0,
					.level_count	= 1,
				});

				views.push_back({
					.type			= view_type::sampled,
					.base_arr_level = 0,
					.level_count	= 1,
				});

				for (u8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					proxy.shadow_texture_hw[i] = backend->create_texture({
						.texture_format		  = render_target_definitions::get_format_depth_default_read(),
						.depth_stencil_format = render_target_definitions::get_format_shadows(),
						.size				  = proxy.shadow_res,
						.flags				  = texture_flags::tf_is_2d | texture_flags::tf_sampled | texture_flags::tf_depth_texture | texture_flags::tf_typeless,
						.views				  = views,
						.array_length		  = 1,
						.clear_values		  = {1.0f, 0.0f, 0.0f, 0.0f},
						.debug_name			  = "spot_light_shadow",
					});

					proxy.shadow_texture_gpu_index[i] = backend->get_texture_gpu_index(proxy.shadow_texture_hw[i], views.size() - 1);
				}
			};

			auto kill_shadow_texture = [&]() {
				for (u8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					add_to_destroy_list(proxy.shadow_texture_hw[i], destroy_data_type::texture);
					proxy.shadow_texture_hw[i] = NULL_GFX_ID;
				}
			};

			if (proxy.cast_shadows == 0)
			{
				if (proxy.shadow_texture_hw[0] != NULL_GFX_ID)
					kill_shadow_texture();
			}
			else
			{
				if (proxy.shadow_texture_hw[0] == NULL_GFX_ID)
				{
					create_shadow_texture();
				}
				else if (proxy.shadow_res != pre_res)
				{
					kill_shadow_texture();
					create_shadow_texture();
				}
			}
		}
		else if (type == render_event_type::update_point_light)
		{
			render_event_point_light ev = {};
			ev.deserialize(stream);
			_peak_point_lights = math::max(_peak_point_lights, index);

			render_proxy_point_light& proxy = _point_lights->get(index);

			if (proxy.status != render_proxy_status::rps_active)
				_count_point_lights++;

			const vector2ui16 pre_res = proxy.shadow_res;
			proxy.status			  = render_proxy_status::rps_active;
			proxy.entity			  = ev.entity_index;
			proxy.base_color		  = ev.base_color;
			proxy.range				  = ev.range;
			proxy.intensity			  = ev.intensity;
			proxy.near_plane		  = ev.near_plane;
			proxy.cast_shadows		  = ev.cast_shadows;
			proxy.shadow_res		  = ev.shadow_resolution;

			if (proxy.near_plane < 0.001f)
				proxy.near_plane = 0.001f;

			auto create_shadow_texture = [&]() {
				// create textures
				vector<view_desc> views;

				for (u8 i = 0; i < 6; i++)
				{
					views.push_back({
						.type			= view_type::depth_stencil,
						.base_arr_level = i,
						.level_count	= 1,
					});
				}
				views.push_back({
					.type			= view_type::sampled,
					.base_arr_level = 0,
					.level_count	= 1,
					.is_cubemap		= 1,
				});

				if (proxy.shadow_res.x < 16)
					proxy.shadow_res.x = 16;
				if (proxy.shadow_res.y < 16)
					proxy.shadow_res.y = 16;

				for (u8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					proxy.shadow_texture_hw[i] = backend->create_texture({
						.texture_format		  = render_target_definitions::get_format_depth_default_read(),
						.depth_stencil_format = render_target_definitions::get_format_shadows(),
						.size				  = proxy.shadow_res,
						.flags				  = texture_flags::tf_is_2d | texture_flags::tf_sampled | texture_flags::tf_depth_texture | texture_flags::tf_typeless,
						.views				  = views,
						.array_length		  = 6,
						.clear_values		  = {1.0f, 0.0f, 0.0f, 0.0f},
						.debug_name			  = "point_light_shadow",
					});

					proxy.shadow_texture_gpu_index[i] = backend->get_texture_gpu_index(proxy.shadow_texture_hw[i], views.size() - 1);
				}
			};

			auto kill_shadow_texture = [&]() {
				for (u8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					add_to_destroy_list(proxy.shadow_texture_hw[i], destroy_data_type::texture);
					proxy.shadow_texture_hw[i] = NULL_GFX_ID;
				}
			};

			if (proxy.cast_shadows == 0)
			{
				if (proxy.shadow_texture_hw[0] != NULL_GFX_ID)
					kill_shadow_texture();
			}
			else
			{
				if (proxy.shadow_texture_hw[0] == NULL_GFX_ID)
				{
					create_shadow_texture();
				}
				else if (proxy.shadow_res != pre_res)
				{
					kill_shadow_texture();
					create_shadow_texture();
				}
			}
		}
		else if (type == render_event_type::update_dir_light)
		{
			render_event_dir_light ev = {};
			ev.deserialize(stream);
			_peak_dir_lights = math::max(_peak_dir_lights, index);

			render_proxy_dir_light& proxy = _dir_lights->get(index);

			if (proxy.status != render_proxy_status::rps_active)
				_count_dir_lights++;

			const vector2ui16 pre_res	  = proxy.shadow_res;
			const u8		  pre_cascade = proxy.cascade_levels;

			proxy.status		 = render_proxy_status::rps_active;
			proxy.entity		 = ev.entity_index;
			proxy.base_color	 = ev.base_color;
			proxy.intensity		 = ev.intensity;
			proxy.cast_shadows	 = ev.cast_shadows;
			proxy.shadow_res	 = ev.shadow_resolution;
			proxy.cascade_levels = ev.max_cascades;

			if (proxy.shadow_res.x < 16)
				proxy.shadow_res.x = 16;
			if (proxy.shadow_res.y < 16)
				proxy.shadow_res.y = 16;

			auto create_shadow_texture = [&]() {
				// create textures
				vector<view_desc> views;

				const u8 cascade_size = ev.max_cascades;

				for (u8 i = 0; i < cascade_size; i++)
				{
					views.push_back({
						.type			= view_type::depth_stencil,
						.base_arr_level = i,
						.level_count	= 1,
					});
				}

				views.push_back({
					.type			= view_type::sampled,
					.base_arr_level = 0,
					.level_count	= cascade_size,
				});

				for (u8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					proxy.shadow_texture_hw[i] = backend->create_texture({
						.texture_format		  = render_target_definitions::get_format_depth_default_read(),
						.depth_stencil_format = render_target_definitions::get_format_shadows(),
						.size				  = proxy.shadow_res,
						.flags				  = texture_flags::tf_is_2d | texture_flags::tf_sampled | texture_flags::tf_depth_texture | texture_flags::tf_typeless,
						.views				  = views,
						.array_length		  = cascade_size,
						.clear_values		  = {1.0f, 0.0f, 0.0f, 0.0f},
						.debug_name			  = "dir_light_shadow",
					});

					proxy.shadow_texture_gpu_index[i] = backend->get_texture_gpu_index(proxy.shadow_texture_hw[i], views.size() - 1);
				}
			};

			auto kill_shadow_texture = [&]() {
				for (u8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					add_to_destroy_list(proxy.shadow_texture_hw[i], destroy_data_type::texture);
					proxy.shadow_texture_hw[i] = NULL_GFX_ID;
				}
			};

			if (proxy.cast_shadows == 0)
			{
				if (proxy.shadow_texture_hw[0] != NULL_GFX_ID)
					kill_shadow_texture();
			}
			else
			{
				if (proxy.shadow_texture_hw[0] == NULL_GFX_ID)
				{
					create_shadow_texture();
				}
				else if (proxy.shadow_res != pre_res || proxy.cascade_levels != pre_cascade)
				{
					kill_shadow_texture();
					create_shadow_texture();
				}
			}
		}
		else if (type == render_event_type::remove_ambient)
		{
			render_proxy_ambient& proxy = get_ambient();
			proxy.status				= render_proxy_status::rps_active;
			proxy						= {};
			_ambient_exists				= 0;
		}
		else if (type == render_event_type::remove_bloom)
		{
			render_proxy_bloom& proxy = get_bloom();
			proxy.status			  = render_proxy_status::rps_active;
			proxy					  = {};
			_bloom_exists			  = 0;
		}
		else if (type == render_event_type::remove_ssao)
		{
			render_proxy_ssao& proxy = get_ssao();
			proxy.status			 = render_proxy_status::rps_active;
			proxy					 = {};
			_ssao_exists			 = 0;
		}
		else if (type == render_event_type::remove_post_process)
		{
			render_proxy_post_process& proxy = get_post_process();
			proxy.status					 = render_proxy_status::rps_active;
			proxy							 = {};
			_post_process_exists			 = 0;
		}
		else if (type == render_event_type::remove_skybox)
		{
			render_proxy_skybox& proxy = get_skybox();
			proxy.status			   = render_proxy_status::rps_active;
			proxy					   = {};
			_skybox_exists			   = 0;
		}
		else if (type == render_event_type::remove_point_light)
		{
			render_proxy_point_light& proxy = _point_lights->get(index);

			if (proxy.status != render_proxy_status::rps_inactive)
				_count_point_lights--;

			if (proxy.shadow_texture_hw[0] != NULL_GFX_ID)
			{
				for (u8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					add_to_destroy_list(proxy.shadow_texture_hw[i], destroy_data_type::texture);
					proxy.shadow_texture_hw[i] = NULL_GFX_ID;
				}
			}

			proxy.status = render_proxy_status::rps_active;
			proxy		 = {};
		}
		else if (type == render_event_type::remove_spot_light)
		{
			render_proxy_spot_light& proxy = _spot_lights->get(index);

			if (proxy.status != render_proxy_status::rps_inactive)
				_count_spot_lights--;

			if (proxy.shadow_texture_hw[0] != NULL_GFX_ID)
			{
				for (u8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					add_to_destroy_list(proxy.shadow_texture_hw[i], destroy_data_type::texture);
					proxy.shadow_texture_hw[i] = NULL_GFX_ID;
				}
			}

			proxy.status = render_proxy_status::rps_active;
			proxy		 = {};
		}
		else if (type == render_event_type::remove_dir_light)
		{
			render_proxy_dir_light& proxy = _dir_lights->get(index);

			if (proxy.status != render_proxy_status::rps_inactive)
				_count_dir_lights--;

			if (proxy.shadow_texture_hw[0] != NULL_GFX_ID)
			{
				for (u8 i = 0; i < BACK_BUFFER_COUNT; i++)
				{
					add_to_destroy_list(proxy.shadow_texture_hw[i], destroy_data_type::texture);
					proxy.shadow_texture_hw[i] = NULL_GFX_ID;
				}
			}

			proxy.status = render_proxy_status::rps_active;
			proxy		 = {};
		}
		else if (type == render_event_type::update_mesh_instance_material)
		{
			render_event_mesh_instance_material ev = {};
			ev.deserialize(stream);
			_peak_mesh_instances			  = math::max(_peak_mesh_instances, index);
			render_proxy_mesh_instance& proxy = get_mesh_instance(index);
			if (proxy.materials_count > ev.index)
			{
				resource_id* ptr = _aux_memory.get<resource_id>(proxy.materials);
				ptr[index]		 = ev.material;
			}
		}
		else if (type == render_event_type::update_mesh_instance)
		{
			render_event_mesh_instance ev = {};
			ev.deserialize(stream);
			_peak_mesh_instances = math::max(_peak_mesh_instances, index);

			render_proxy_mesh_instance& proxy = get_mesh_instance(index);

			proxy.entity			  = ev.entity_index;
			proxy.mesh				  = ev.mesh;
			proxy.skin				  = ev.skin;
			proxy.materials_count	  = static_cast<u16>(ev.materials.size());
			proxy.skin_entities_count = static_cast<u16>(ev.skin_node_entities.size());

			if (proxy.materials_count != 0)
			{
				resource_id* out = nullptr;
				proxy.materials	 = _aux_memory.allocate<resource_id>(proxy.materials_count, out);
				for (u16 i = 0; i < proxy.materials_count; i++)
					out[i] = ev.materials[i];
			}

			if (proxy.skin_entities_count != 0)
			{
				world_id* out		= nullptr;
				proxy.skin_entities = _aux_memory.allocate<world_id>(proxy.skin_entities_count, out);
				for (u16 i = 0; i < proxy.skin_entities_count; i++)
				{
					out[i] = ev.skin_node_entities[i];
				}
			}
		}
		else if (type == render_event_type::remove_mesh_instance)
		{
			render_proxy_mesh_instance& proxy = get_mesh_instance(index);
			if (proxy.materials.size != 0)
				_aux_memory.free(proxy.materials);
			if (proxy.skin_entities.size != 0)
				_aux_memory.free(proxy.skin_entities);
			proxy = {};
		}
		else if (type == render_event_type::update_camera)
		{
			render_event_camera ev = {};
			ev.deserialize(stream);

			render_proxy_camera& proxy = get_camera(index);

			proxy = {
				.entity		 = ev.entity_index,
				.near_plane	 = ev.near_plane,
				.far_plane	 = ev.far_plane,
				.fov_degrees = ev.fov_degrees,
			};

			const u8 cascade_levels = static_cast<u8>(ev.cascades.size());

			if (cascade_levels != 0)
			{
				chunk_handle32 cascades		= _aux_memory.allocate<f32>(cascade_levels);
				f32*		   cascades_ptr = _aux_memory.get<f32>(cascades);
				for (u8 i = 0; i < cascade_levels; i++)
					cascades_ptr[i] = ev.cascades[i];
				proxy.cascades		= cascades;
				proxy.cascade_count = cascade_levels;
			}
		}
		else if (type == render_event_type::set_main_camera)
		{
			render_proxy_camera& proxy = get_camera(index);
			_main_camera_trait		   = index;
		}
		else if (type == render_event_type::remove_camera)
		{
			render_proxy_camera& proxy = get_camera(index);
			if (index == _main_camera_trait)
				_main_camera_trait = NULL_WORLD_ID;

			if (proxy.cascades.size != 0)
				_aux_memory.free(proxy.cascades);

			proxy = {};
		}
		else if (type == render_event_type::create_texture)
		{
			render_event_texture ev = {};
			ev.deserialize(stream);

			render_proxy_texture& proxy = get_texture(index);
			proxy.status				= render_proxy_status::rps_active;
			proxy.handle				= index;

			proxy.hw = backend->create_texture({
				.texture_format = static_cast<format>(ev.format),
				.size			= ev.size,
				.flags			= texture_flags::tf_is_2d | texture_flags::tf_sampled,
				.views			= {{
							 .type			 = view_type::sampled,
							 .base_arr_level = 0,
							 .level_count	 = 1,
							 .base_mip_level = 0,
							 .mip_count		 = static_cast<u8>(ev.buffers.size()),
				 }},
				.mip_levels		= static_cast<u8>(ev.buffers.size()),
				.array_length	= 1,
				.samples		= 1,
#ifndef SFG_STRIP_DEBUG_NAMES
				.debug_name = ev.name.c_str(),
#endif
			});

			proxy.heap_index = backend->get_texture_gpu_index(proxy.hw, 0);

			proxy.intermediate = backend->create_resource({
				.size		= ev.intermediate_size,
				.flags		= resource_flags::rf_cpu_visible,
				.debug_name = "texture_intermediate",
			});

			_texture_queue.add_request(ev.buffers, proxy.hw, proxy.intermediate, 1, resource_state::resource_state_ps_resource, ev.buffers_persistent);
		}
		else if (type == render_event_type::destroy_texture)
		{
			render_proxy_texture& proxy = get_texture(index);
			destroy_texture(proxy);
		}
		else if (type == render_event_type::create_sampler)
		{
			render_event_sampler ev = {};
			ev.deserialize(stream);
			render_proxy_sampler& proxy = get_sampler(index);
			proxy.status				= render_proxy_status::rps_active;
			proxy.handle				= index;
			proxy.hw					= backend->create_sampler(ev.desc);
			proxy.heap_index			= backend->get_sampler_gpu_index(proxy.hw);
		}
		else if (type == render_event_type::destroy_sampler)
		{
			render_proxy_sampler& proxy = get_sampler(index);
			destroy_sampler(proxy);
		}
		else if (type == render_event_type::create_shader)
		{
			render_event_shader ev = {};
			ev.deserialize(stream);
			render_proxy_shader& proxy = get_shader(index);
			proxy.status			   = render_proxy_status::rps_active;
			proxy.handle			   = index;

			const u32 pso_count = static_cast<u32>(ev.pso_variants.size());
			proxy.variants		= _aux_memory.allocate<render_proxy_shader_variant>(pso_count);
			proxy.variant_count = pso_count;

			render_proxy_shader_variant* proxy_vars = _aux_memory.get<render_proxy_shader_variant>(proxy.variants);

			for (u32 i = 0; i < pso_count; i++)
			{
				render_proxy_shader_variant& variant = proxy_vars[i];
				pso_variant&				 pso	 = ev.pso_variants[i];

				variant.variant_flags = pso.variant_flags;
				variant.hw			  = backend->create_shader(pso.desc, ev.compile_variants.at(pso.compile_variant).blobs, ev.layout);
			}

			if (!ev.persistent_blobs)
			{
				for (compile_variant& cv : ev.compile_variants)
					cv.destroy();
				ev.compile_variants.clear();
			}
		}
		else if (type == render_event_type::destroy_shader)
		{
			render_proxy_shader& proxy = get_shader(index);
			const chunk_handle32 vars  = proxy.variants;
			destroy_shader(proxy);
			_aux_memory.free(vars);
		}
		else if (type == render_event_type::reload_shader)
		{
			render_event_resource_reloaded ev = {};
			ev.deserialize(stream);
			for (u32 i = 0; i < MAX_WORLD_MATERIALS; i++)
			{
				render_proxy_material&		   mat = _materials->get(i);
				render_proxy_material_runtime& rt  = get_material_runtime(i);
				if (mat.status != render_proxy_status::rps_active)
					continue;

				if (rt.shader_handle == ev.prev_id)
					rt.shader_handle = ev.new_id;
			}
		}
		else if (type == render_event_type::create_material)
		{
			render_event_material ev = {};
			ev.deserialize(stream);

			render_proxy_material&		   proxy   = get_material(index);
			render_proxy_material_runtime& runtime = get_material_runtime(index);
			proxy.status						   = render_proxy_status::rps_active;
			runtime.flags						   = ev.flags;
			runtime.shader_handle				   = ev.shader_index;
			runtime.draw_priority				   = ev.priority;

			if ((ev.sampler == NULL_RESOURCE_ID && !ev.textures.empty()))
			{
				SFG_ASSERT(false);
			}

			for (u8 i = 0; i < BACK_BUFFER_COUNT; i++)
			{
				proxy.buffer_size = static_cast<u32>(ev.data.size);

				if (ev.data.size != 0)
				{
					proxy.buffers[i].create({
						.size  = static_cast<u32>(ev.data.size),
						.flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible,
#ifndef SFG_STRIP_DEBUG_NAMES
						.debug_name = ev.name.c_str(),
#endif
					});

					proxy.buffers[i].buffer_data(0, ev.data.data, ev.data.size);
					runtime.gpu_index_buffers[i] = backend->get_resource_gpu_index(proxy.buffers[i].get_gpu());
				}

				const u32 texture_buffer_size = static_cast<u32>(sizeof(gpu_index) * ev.textures.size());

				if (ev.sampler != NULL_RESOURCE_ID)
				{
					runtime.gpu_index_sampler = backend->get_sampler_gpu_index(get_sampler(ev.sampler).hw);
				}

				proxy.texture_count = static_cast<u32>(ev.textures.size());

				if (texture_buffer_size != 0)
				{
					proxy.texture_buffers[i].create({
						.size  = texture_buffer_size,
						.flags = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible,
#ifndef SFG_STRIP_DEBUG_NAMES
						.debug_name = ev.name.c_str(),
#endif
					});

					runtime.gpu_index_texture_buffers[i] = backend->get_resource_gpu_index(proxy.texture_buffers[i].get_gpu());

					size_t offset = 0;

					for (resource_id txt_id : ev.textures)
					{
						const render_proxy_texture& txt = get_texture(txt_id);
						SFG_ASSERT(txt.status == render_proxy_status::rps_active);
						proxy.texture_buffers[i].buffer_data(offset, &txt.heap_index, sizeof(gpu_index));
						offset += sizeof(gpu_index);
					}
				}
			}
		}
		else if (type == render_event_type::update_material_sampler)
		{
			render_event_update_material_sampler ev = {};
			ev.deserialize(stream);

			render_proxy_sampler& smp = get_sampler(ev.sampler);
			SFG_ASSERT(smp.status == render_proxy_status::rps_active);

			render_proxy_material& proxy = get_material(index);
			SFG_ASSERT(proxy.status == render_proxy_status::rps_active);

			render_proxy_material_runtime& runtime = get_material_runtime(index);
			runtime.gpu_index_sampler			   = backend->get_sampler_gpu_index(smp.hw);
		}
		else if (type == render_event_type::update_material_textures)
		{
			render_event_update_material_textures ev = {};
			ev.deserialize(stream);

			render_proxy_material& proxy = get_material(index);

			SFG_ASSERT(proxy.texture_buffers[0].get_gpu() != NULL_GFX_ID);
			SFG_ASSERT(proxy.texture_count != 0 && proxy.texture_count >= ev.start + static_cast<u8>(ev.textures.size()));

			const material_update update = {
				.data		 = _aux_memory.allocate<u8>(ev.textures.size() * sizeof(gpu_index)),
				.padding	 = static_cast<u32>(ev.start) * sizeof(gpu_index),
				.material_id = index,
				.is_texture	 = 0,
				.count		 = 0,
			};

			u8*	   ptr	  = _aux_memory.get<u8>(update.data);
			size_t offset = 0;
			for (resource_id id : ev.textures)
			{
				const render_proxy_texture& txt = get_texture(id);
				SFG_ASSERT(txt.status == render_proxy_status::rps_active);
				SFG_MEMCPY(ptr, &txt.heap_index, sizeof(gpu_index));
				offset += sizeof(gpu_index);
			}

			_material_updates->push_back(update);
		}
		else if (type == render_event_type::update_material_data)
		{
			render_event_update_material_data ev = {};
			ev.deserialize(stream);

			render_proxy_material& proxy = get_material(index);
			SFG_ASSERT(proxy.buffers[0].get_gpu() != NULL_GFX_ID);
			SFG_ASSERT(proxy.buffer_size != 0 && proxy.buffer_size >= ev.padding + ev.size);

			const material_update update = {
				.data		 = _aux_memory.allocate<u8>(ev.size),
				.padding	 = ev.padding,
				.material_id = index,
				.is_texture	 = 0,
				.count		 = 0,
			};
			u8* ptr = _aux_memory.get<u8>(update.data);
			SFG_MEMCPY(ptr, ev.data, static_cast<size_t>(ev.size));

			_material_updates->push_back(update);
		}
		else if (type == render_event_type::destroy_material)
		{
			render_proxy_material& proxy = get_material(index);
			destroy_material(proxy);
		}
		else if (type == render_event_type::create_skin)
		{
			render_event_skin ev = {};
			ev.deserialize(stream);

			const u32		   sz	 = static_cast<u32>(ev.nodes.size());
			render_proxy_skin& proxy = get_skin(index);
			proxy.status			 = render_proxy_status::rps_active;
			proxy.nodes				 = _aux_memory.allocate<u16>(sz);
			proxy.matrices			 = _aux_memory.allocate<matrix4x3>(sz);
			proxy.node_count		 = sz;

			u16*	   nodes	= _aux_memory.get<u16>(proxy.nodes);
			matrix4x3* matrices = _aux_memory.get<matrix4x3>(proxy.matrices);

			for (u32 i = 0; i < sz; i++)
			{
				nodes[i]	= ev.nodes[i];
				matrices[i] = ev.matrices[i];
			}
		}
		else if (type == render_event_type::destroy_skin)
		{
			render_proxy_skin& proxy = get_skin(index);
			destroy_skin(proxy);

			for (u32 i = 0; i < get_peak_mesh_instances(); i++)
			{
				render_proxy_mesh_instance& mi = get_mesh_instance(i);
				if (mi.skin == index)
					mi.skin = NULL_RESOURCE_ID;
			}
		}
		else if (type == render_event_type::create_mesh)
		{
			render_event_mesh ev = {};
			ev.deserialize(stream);
			render_proxy_mesh& proxy = get_mesh(index);
			proxy.status			 = render_proxy_status::rps_active;
			proxy.handle			 = index;
			proxy.local_aabb		 = ev.local_aabb;

			auto process = [&](auto& list, size_t vtx_type_size) {
				size_t vertex_size = 0;
				size_t index_size  = 0;

				for (const auto& p : list)
				{
					const size_t vertices = p.vertices.size();
					const size_t indices  = p.indices.size();
					vertex_size += vertices * vtx_type_size;
					index_size += indices * sizeof(primitive_index);
				}

				proxy.vertex_buffer.create(
					{
						.size  = static_cast<u32>(vertex_size),
						.flags = resource_flags::rf_cpu_visible,
#ifndef SFG_STRIP_DEBUG_NAMES
						.debug_name = ev.name.c_str(),
#endif
					},
					{
						.size  = static_cast<u32>(vertex_size),
						.flags = resource_flags::rf_vertex_buffer | resource_flags::rf_gpu_only,
#ifndef SFG_STRIP_DEBUG_NAMES
						.debug_name = ev.name.c_str(),
#endif
					});

				proxy.index_buffer.create(
					{
						.size  = static_cast<u32>(index_size),
						.flags = resource_flags::rf_cpu_visible,
#ifndef SFG_STRIP_DEBUG_NAMES
						.debug_name = ev.name.c_str(),
#endif
					},
					{
						.size  = static_cast<u32>(index_size),
						.flags = resource_flags::rf_index_buffer | resource_flags::rf_gpu_only,
#ifndef SFG_STRIP_DEBUG_NAMES
						.debug_name = ev.name.c_str(),
#endif
					});

				u32	   vtx_counter	 = 0;
				u32	   idx_counter	 = 0;
				size_t vertex_offset = 0;
				size_t index_offset	 = 0;

				const u32 primitives_size		 = static_cast<u32>(list.size());
				proxy.primitives				 = _aux_memory.allocate<render_proxy_primitive>(primitives_size);
				proxy.primitive_count			 = primitives_size;
				render_proxy_primitive* prim_ptr = _aux_memory.get<render_proxy_primitive>(proxy.primitives);

				for (u32 i = 0; i < primitives_size; i++)
				{
					render_proxy_primitive& proxy_prim = prim_ptr[i];
					const auto&				p		   = list[i];
					const size_t			vtx_sz	   = p.vertices.size() * vtx_type_size;
					const size_t			idx_sz	   = p.indices.size() * sizeof(primitive_index);

					proxy_prim.material_index = p.material_index;
					proxy_prim.vertex_start	  = vtx_counter;
					proxy_prim.index_start	  = idx_counter;
					proxy_prim.index_count	  = static_cast<u32>(p.indices.size());

					proxy.vertex_buffer.buffer_data(vertex_offset, p.vertices.data(), vtx_sz);
					proxy.index_buffer.buffer_data(index_offset, p.indices.data(), idx_sz);

					vertex_offset += vtx_sz;
					index_offset += idx_sz;
					vtx_counter += static_cast<u32>(p.vertices.size());
					idx_counter += proxy_prim.index_count;
				}

				_buffer_queue.add_request({
					.buffer	  = &proxy.vertex_buffer,
					.to_state = resource_state::resource_state_vertex_cbv,
				});
				_buffer_queue.add_request({
					.buffer	  = &proxy.index_buffer,
					.to_state = resource_state::resource_state_index_buffer,
				});
			};

			if (!ev.primitives_static.empty())
			{
				proxy.is_skinned = 0;
				process(ev.primitives_static, sizeof(vertex_static));
			}
			else if (!ev.primitives_skinned.empty())
			{
				proxy.is_skinned = 1;
				process(ev.primitives_skinned, sizeof(vertex_skinned));
			}
			else
			{
				SFG_ASSERT(false);
			}
		}
		else if (type == render_event_type::destroy_mesh)
		{
			render_proxy_mesh& proxy = get_mesh(index);
			destroy_mesh(proxy);

			for (u32 i = 0; i < get_peak_mesh_instances(); i++)
			{
				render_proxy_mesh_instance& mi = get_mesh_instance(i);
				if (mi.mesh == index)
					mi.mesh = NULL_RESOURCE_ID;
			}
		}
		else if (type == render_event_type::particle_emitter)
		{
			render_proxy_particle_emitter& proxy = _emitters->get(index);
			render_event_particle_emitter  ev	 = {};
			ev.deserialize(stream);
			proxy.material			= ev.material;
			proxy.entity			= ev.entity;
			proxy.particle_res_id	= ev.particle_res;
			proxy.status			= render_proxy_status::rps_active;
			proxy.last_emitted		= 0.0f;
			proxy.current_life		= 0.0f;
			_peak_particle_emitters = math::max(_peak_particle_emitters, index);
		}
		else if (type == render_event_type::sprite)
		{
			render_proxy_sprite& proxy = _sprites->get(index);
			render_event_sprite	 ev	   = {};
			ev.deserialize(stream);
			proxy.material = ev.material;
			proxy.entity   = ev.entity;
			proxy.status   = render_proxy_status::rps_active;
			_peak_sprites  = math::max(_peak_sprites, index);
		}
		else if (type == render_event_type::remove_particle_emitter)
		{
			render_proxy_particle_emitter& proxy = get_emitter(index);
			proxy								 = {};
		}
		else if (type == render_event_type::remove_sprite)
		{
			render_proxy_sprite& proxy = get_sprite(index);
			proxy					   = {};
		}
		else if (type == render_event_type::reset_particle_emitter)
		{
			render_proxy_particle_emitter& proxy = get_emitter(index);
			proxy.last_emitted					 = 0.0f;
			proxy.current_life					 = 0.0f;
		}
		else if (type == render_event_type::particle_res)
		{
			render_event_particle_res ev = {};
			ev.deserialize(stream);

			render_proxy_particle_resource& proxy = get_particle(index);
			proxy.emit_props					  = ev.props;
			proxy.status						  = render_proxy_status::rps_active;
		}
		else if (type == render_event_type::destroy_particle_res)
		{
			render_proxy_particle_resource& proxy = get_particle(index);
			proxy								  = {};
		}
	}

	void proxy_manager::destroy_texture(render_proxy_texture& proxy)
	{
		add_to_destroy_list(proxy.hw, destroy_data_type::texture);
		add_to_destroy_list(proxy.intermediate, destroy_data_type::resource);
		proxy = {};
	}

	void proxy_manager::destroy_sampler(render_proxy_sampler& proxy)
	{
		add_to_destroy_list(proxy.hw, destroy_data_type::sampler);
		proxy = {};
	}

	void proxy_manager::destroy_shader(render_proxy_shader& proxy)
	{
		render_proxy_shader_variant* vars = _aux_memory.get<render_proxy_shader_variant>(proxy.variants);
		for (u32 i = 0; i < proxy.variant_count; i++)
			add_to_destroy_list(vars[i].hw, destroy_data_type::shader);
		proxy = {};
	}

	void proxy_manager::destroy_mesh(render_proxy_mesh& proxy)
	{
		add_to_destroy_list(proxy.vertex_buffer.get_staging(), destroy_data_type::resource);
		add_to_destroy_list(proxy.vertex_buffer.get_gpu(), destroy_data_type::resource);
		add_to_destroy_list(proxy.index_buffer.get_staging(), destroy_data_type::resource);
		add_to_destroy_list(proxy.index_buffer.get_gpu(), destroy_data_type::resource);

		if (proxy.primitives.size != 0)
			_aux_memory.free(proxy.primitives);
		proxy = {};
	}

	void proxy_manager::destroy_canvas(render_proxy_canvas& proxy)
	{
		for (u8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			add_to_destroy_list(proxy.vertex_buffers[i].get_staging(), destroy_data_type::resource);
			add_to_destroy_list(proxy.vertex_buffers[i].get_gpu(), destroy_data_type::resource);
			add_to_destroy_list(proxy.index_buffers[i].get_staging(), destroy_data_type::resource);
			add_to_destroy_list(proxy.index_buffers[i].get_gpu(), destroy_data_type::resource);
		}

		proxy = {};
	}

	void proxy_manager::destroy_skin(render_proxy_skin& proxy)
	{
		proxy = {};
	}

	void proxy_manager::destroy_material(render_proxy_material& proxy)
	{
		gfx_backend* backend = gfx_backend::get();

		for (u8 i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			if (proxy.texture_buffers[i].get_gpu() != NULL_GFX_ID)
				add_to_destroy_list(proxy.texture_buffers[i].get_gpu(), destroy_data_type::resource);

			if (proxy.buffers[i].get_gpu() != NULL_GFX_ID)
				add_to_destroy_list(proxy.buffers[i].get_gpu(), destroy_data_type::resource);
		}
		proxy = {};
	}

	void proxy_manager::add_to_destroy_list(gfx_id id, destroy_data_type type)
	{
		_destroy_list.push_back({.id = id, .type = type, .target_frame = frame_info::get_render_frame() + BACK_BUFFER_COUNT + 1});
	}
}
