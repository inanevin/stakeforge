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

#include "common/size_definitions.hpp"
#include "render_proxy_resources.hpp"
#include "render_proxy_entity.hpp"
#include "render_proxy_components.hpp"
#include "memory/static_array.hpp"
#include "memory/chunk_allocator.hpp"
#include "data/vector.hpp"
#include "data/static_vector.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "game/game_max_defines.hpp"

namespace SFG
{
	class buffer_queue;
	class texture_queue;
	class render_event_stream;
	class istream;
	struct render_event_header;

	class proxy_manager
	{
	public:
		proxy_manager() = delete;
		proxy_manager(buffer_queue& b, texture_queue& t);
		~proxy_manager();

		void   init();
		void   uninit();
		void   fetch_render_events(render_event_stream& stream, u8 frame_index);
		void   flush_destroys(bool force);
		gfx_id get_shader_variant(resource_id shader_handle, u32 flags);

		inline render_proxy_texture& get_texture(resource_id idx)
		{
			return _textures->get(idx);
		}

		inline render_proxy_sampler& get_sampler(resource_id idx)
		{
			return _samplers->get(idx);
		}

		inline render_proxy_mesh& get_mesh(resource_id idx)
		{
			return _meshes->get(idx);
		}

		inline render_proxy_skin& get_skin(resource_id idx)
		{
			return _skins->get(idx);
		}

		inline render_proxy_shader& get_shader(resource_id idx)
		{
			return _shaders->get(idx);
		}

		inline render_proxy_material& get_material(resource_id idx)
		{
			return _materials->get(idx);
		}

		inline render_proxy_material_runtime& get_material_runtime(resource_id idx)
		{
			return _material_runtimes->get(idx);
		}

		inline render_proxy_entity& get_entity(world_id idx)
		{
			return _entities->get(idx);
		}

		inline render_proxy_mesh_instance& get_mesh_instance(world_id idx)
		{
			return _mesh_instances->get(idx);
		}

		inline render_proxy_camera& get_camera(world_id idx)
		{
			return _cameras->get(idx);
		}

		inline render_proxy_ambient& get_ambient()
		{
			return _ambients->get(0);
		}

		inline render_proxy_bloom& get_bloom()
		{
			return _blooms->get(0);
		}

		inline render_proxy_ssao& get_ssao()
		{
			return _ssaos->get(0);
		}

		inline render_proxy_post_process& get_post_process()
		{
			return _post_processes->get(0);
		}

		inline render_proxy_skybox& get_skybox()
		{
			return _skyboxes->get(0);
		}

		inline render_proxy_canvas& get_canvas(world_id idx)
		{
			return _canvases->get(idx);
		}

		inline render_proxy_particle_resource& get_particle(resource_id idx)
		{
			return _particle_resources->get(idx);
		}
		inline chunk_allocator32& get_aux()
		{
			return _aux_memory;
		}

		inline auto get_particle_emitters()
		{
			return _emitters;
		}

		inline auto get_mesh_instances()
		{
			return _mesh_instances;
		}

		inline auto get_entities()
		{
			return _entities;
		}

		inline auto get_skins()
		{
			return _skins;
		}

		inline auto& get_cameras()
		{
			return _cameras;
		}

		inline auto get_point_lights()
		{
			return _point_lights;
		}

		inline auto get_spot_lights()
		{
			return _spot_lights;
		}

		inline auto get_dir_lights()
		{
			return _dir_lights;
		}

		inline auto get_materials()
		{
			return _materials;
		}

		inline auto get_material_runtimes()
		{
			return _material_runtimes;
		}

		inline auto get_canvases()
		{
			return _canvases;
		}

		inline auto get_sprites()
		{
			return _sprites;
		}

		inline render_proxy_particle_emitter& get_emitter(world_id id)
		{
			return _emitters->get(id);
		}

		inline render_proxy_sprite& get_sprite(world_id id)
		{
			return _sprites->get(id);
		}

		inline world_id get_main_camera() const
		{
			return _main_camera_trait;
		}

		inline u32 get_peak_particle_emitters() const
		{
			return _peak_particle_emitters + 1;
		}

		inline u32 get_peak_mesh_instances() const
		{
			return _peak_mesh_instances + 1;
		}

		inline u32 get_peak_sprites() const
		{
			return _peak_sprites + 1;
		}

		inline u32 get_ambient_exists() const
		{
			return _ambient_exists;
		}

		inline u32 get_bloom_exists() const
		{
			return _bloom_exists;
		}

		inline u32 get_ssao_exists() const
		{
			return _ssao_exists;
		}

		inline u32 get_post_process_exists() const
		{
			return _post_process_exists;
		}

		inline u32 get_skybox_exists() const
		{
			return _skybox_exists;
		}

		inline u32 get_peak_dir_lights() const
		{
			return _peak_dir_lights + 1;
		}

		inline u32 get_peak_spot_lights() const
		{
			return _peak_spot_lights + 1;
		}

		inline u32 get_peak_point_lights() const
		{
			return _peak_point_lights + 1;
		}

		inline u32 get_count_dir_lights() const
		{
			return _count_dir_lights;
		}

		inline u32 get_count_spot_lights() const
		{
			return _count_spot_lights;
		}

		inline u32 get_count_point_lights() const
		{
			return _count_point_lights;
		}

		inline u32 get_peak_entities() const
		{
			return _peak_entities + 1;
		}

		inline u32 get_peak_canvases() const
		{
			return _peak_canvases + 1;
		}

	private:
		enum destroy_data_type : u8
		{
			texture,
			sampler,
			shader,
			bind_group,
			resource,
		};

		struct destroy_data
		{
			gfx_id			  id		   = 0;
			destroy_data_type type		   = {};
			u64				  target_frame = 0;
			bool			  destroyed	   = false;
			bool			  aq		   = false;
		};

		struct destroy_bucket
		{
			vector<destroy_data> list;
		};

	private:
		void process_event(const render_event_header& header, istream& stream, u8 frame_index);
		void destroy_texture(render_proxy_texture& proxy);
		void destroy_sampler(render_proxy_sampler& proxy);
		void destroy_shader(render_proxy_shader& proxy);
		void destroy_material(render_proxy_material& proxy);
		void destroy_mesh(render_proxy_mesh& proxy);
		void destroy_canvas(render_proxy_canvas& proxy);
		void destroy_skin(render_proxy_skin& proxy);
		void add_to_destroy_list(gfx_id id, destroy_data_type type);
		void reset();

	private:
		buffer_queue&  _buffer_queue;
		texture_queue& _texture_queue;

		struct material_update
		{
			chunk_handle32 data		   = {};
			u32			   padding	   = 0;
			world_id	   material_id = 0;
			u8			   is_texture  = 0;
			u8			   count	   = 0;
		};

		using textures_type			 = static_array<render_proxy_texture, MAX_WORLD_TEXTURES>;
		using samplers_type			 = static_array<render_proxy_sampler, MAX_WORLD_SAMPLERS>;
		using materials_type		 = static_array<render_proxy_material, MAX_WORLD_MATERIALS>;
		using material_runtimes_type = static_array<render_proxy_material_runtime, MAX_WORLD_MATERIALS>;
		using shaders_type			 = static_array<render_proxy_shader, MAX_WORLD_SHADERS>;
		using meshes_type			 = static_array<render_proxy_mesh, MAX_WORLD_MESHES>;
		using skins_type			 = static_array<render_proxy_skin, MAX_WORLD_SKINS>;
		using entity_type			 = static_array<render_proxy_entity, MAX_ENTITIES>;
		using mesh_instances_type	 = static_array<render_proxy_mesh_instance, MAX_WORLD_COMP_MESH_INSTANCES>;
		using cameras_type			 = static_array<render_proxy_camera, MAX_WORLD_COMP_CAMERAS>;
		using ambients_type			 = static_array<render_proxy_ambient, MAX_WORLD_COMP_AMBIENT>;
		using blooms_type			 = static_array<render_proxy_bloom, MAX_WORLD_COMP_BLOOM>;
		using ssaos_type			 = static_array<render_proxy_ssao, MAX_WORLD_COMP_SSAO>;
		using post_processes_type	 = static_array<render_proxy_post_process, MAX_WORLD_COMP_POST_PROCESS>;
		using skyboxes_type			 = static_array<render_proxy_skybox, MAX_WORLD_COMP_SKYBOX>;
		using point_lights_type		 = static_array<render_proxy_point_light, MAX_WORLD_COMP_POINT_LIGHTS>;
		using spot_lights_type		 = static_array<render_proxy_spot_light, MAX_WORLD_COMP_SPOT_LIGHTS>;
		using dir_lights_type		 = static_array<render_proxy_dir_light, MAX_WORLD_COMP_DIR_LIGHTS>;
		using canvas_type			 = static_array<render_proxy_canvas, MAX_WORLD_COMP_CANVAS>;
		using emitters_type			 = static_array<render_proxy_particle_emitter, MAX_WORLD_COMP_PARTICLE_EMITTERS>;
		using sprites_type			 = static_array<render_proxy_sprite, MAX_WORLD_COMP_SPRITES>;
		using particle_res_type		 = static_array<render_proxy_particle_resource, MAX_WORLD_PARTICLE_PROPERTIES>;
		using material_updates_type	 = static_vector<material_update, 124>;

		vector<destroy_data> _destroy_list;

		chunk_allocator32		_aux_memory;
		textures_type*			_textures			= nullptr;
		samplers_type*			_samplers			= nullptr;
		materials_type*			_materials			= nullptr;
		material_runtimes_type* _material_runtimes	= nullptr;
		shaders_type*			_shaders			= nullptr;
		meshes_type*			_meshes				= nullptr;
		skins_type*				_skins				= nullptr;
		entity_type*			_entities			= nullptr;
		mesh_instances_type*	_mesh_instances		= nullptr;
		cameras_type*			_cameras			= nullptr;
		ambients_type*			_ambients			= nullptr;
		blooms_type*			_blooms				= nullptr;
		ssaos_type*				_ssaos				= nullptr;
		post_processes_type*	_post_processes		= nullptr;
		skyboxes_type*			_skyboxes			= nullptr;
		point_lights_type*		_point_lights		= nullptr;
		spot_lights_type*		_spot_lights		= nullptr;
		dir_lights_type*		_dir_lights			= nullptr;
		canvas_type*			_canvases			= nullptr;
		emitters_type*			_emitters			= nullptr;
		sprites_type*			_sprites			= nullptr;
		material_updates_type*	_material_updates	= nullptr;
		particle_res_type*		_particle_resources = nullptr;

		world_id _main_camera_trait		 = NULL_WORLD_ID;
		u32		 _peak_particle_emitters = 0;
		u32		 _peak_mesh_instances	 = 0;
		u32		 _peak_point_lights		 = 0;
		u32		 _peak_spot_lights		 = 0;
		u32		 _peak_dir_lights		 = 0;
		u32		 _peak_sprites			 = 0;
		u32		 _count_dir_lights		 = 0;
		u32		 _count_spot_lights		 = 0;
		u32		 _count_point_lights	 = 0;
		u32		 _peak_entities			 = 0;
		u32		 _peak_canvases			 = 0;
		u8		 _ambient_exists		 = 0;
		u8		 _bloom_exists			 = 0;
		u8		 _ssao_exists			 = 0;
		u8		 _post_process_exists	 = 0;
		u8		 _skybox_exists			 = 0;
	};
}
