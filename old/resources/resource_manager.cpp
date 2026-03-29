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

#include "resource_manager.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "data/pair.hpp"
#include "game/game_max_defines.hpp"
#include "app/engine_resources.hpp"
#include "app/package.hpp"
#include "reflection/reflection.hpp"

// resources
#include "resources/texture.hpp"
#include "resources/texture_sampler.hpp"
#include "resources/texture_sampler_raw.hpp"
#include "resources/particle_properties.hpp"
#include "resources/particle_properties_raw.hpp"
#include "resources/physical_material.hpp"
#include "resources/material_raw.hpp"
#include "resources/shader_raw.hpp"
#include "resources/shader.hpp"
#include "resources/material.hpp"

#ifdef SFG_TOOLMODE
#include "serialization/serialization.hpp"
#include "editor/editor_settings.hpp"
#include "resources/model.hpp"
#include "resources/mesh.hpp"
#include "resources/entity_template.hpp"
#include "gfx/event_stream/render_events_gfx.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "world/components/comp_particle_emitter.hpp"
#include "world/components/comp_physics.hpp"
#include "world/world.hpp"
#include "platform/time.hpp"
#include "editor/editor.hpp"
#endif

#include <algorithm>
#include <execution>
#include <future>
namespace SFG
{

	resource_manager::resource_manager(world& w) : _world(w)
	{
		_aux_memory.init(MAX_WORLD_RESOURCES_AUX_MEMORY);

		// dummy texture raw
		{
			_dummy_color_raw.name  = "dummy_color";
			_dummy_normal_raw.name = "dummy_normal";
			_dummy_orm_raw.name	   = "dummy_orm";

			_dummy_color_data  = reinterpret_cast<u8*>(SFG_MALLOC(4));
			_dummy_normal_data = reinterpret_cast<u8*>(SFG_MALLOC(4));
			_dummy_orm_data	   = reinterpret_cast<u8*>(SFG_MALLOC(4));

			u8 color_data[4]  = {255, 255, 255, 255};
			u8 normal_data[4] = {128, 128, 255, 255};
			u8 orm_data[4]	  = {255, 255, 0, 255};

			if (_dummy_color_data)
				SFG_MEMCPY(_dummy_color_data, color_data, 4);

			if (_dummy_normal_data)
				SFG_MEMCPY(_dummy_normal_data, normal_data, 4);

			if (_dummy_orm_data)
				SFG_MEMCPY(_dummy_orm_data, orm_data, 4);

			_dummy_color_raw.load_from_data(_dummy_color_data, vector2ui16(1, 1), static_cast<u8>(format::r8g8b8a8_srgb), false);
			_dummy_normal_raw.load_from_data(_dummy_normal_data, vector2ui16(1, 1), static_cast<u8>(format::r8g8b8a8_unorm), false);
			_dummy_orm_raw.load_from_data(_dummy_orm_data, vector2ui16(1, 1), static_cast<u8>(format::r8g8b8a8_unorm), false);
			_dummy_color_raw.buffers_persistent	 = true;
			_dummy_normal_raw.buffers_persistent = true;
			_dummy_orm_raw.buffers_persistent	 = true;
		}
	}

	resource_manager::~resource_manager()
	{
		// dummies
		{
			SFG_FREE(_dummy_color_data);
			SFG_FREE(_dummy_normal_data);
			SFG_FREE(_dummy_orm_data);
		}

		for (const cache_storage& stg : _storages)
			delete stg.cache_ptr;

		_aux_memory.uninit();

#ifdef SFG_TOOLMODE
		_file_watch.clear();
#endif
	}

	void resource_manager::init()
	{
		shader_raw&	  sh_raw_gbuffer   = engine_resources::get().get_shader_raw(engine_resource_ident::shader_world_gbuffer);
		shader_raw&	  sh_raw_fw		   = engine_resources::get().get_shader_raw(engine_resource_ident::shader_world_forward);
		shader_raw&	  sh_raw_gui	   = engine_resources::get().get_shader_raw(engine_resource_ident::shader_world_gui_default);
		shader_raw&	  sh_raw_gui_text  = engine_resources::get().get_shader_raw(engine_resource_ident::shader_world_gui_text);
		shader_raw&	  sh_raw_gui_sdf   = engine_resources::get().get_shader_raw(engine_resource_ident::shader_world_gui_sdf);
		material_raw& mat_raw_gui	   = engine_resources::get().get_material_raw(engine_resource_ident::mat_world_gui_default);
		material_raw& mat_raw_gui_sdf  = engine_resources::get().get_material_raw(engine_resource_ident::mat_world_gui_sdf);
		material_raw& mat_raw_gui_text = engine_resources::get().get_material_raw(engine_resource_ident::mat_world_gui_text);

		sh_raw_gbuffer.persistent_blobs	 = true;
		sh_raw_fw.persistent_blobs		 = true;
		sh_raw_gui.persistent_blobs		 = true;
		sh_raw_gui_text.persistent_blobs = true;
		sh_raw_gui_sdf.persistent_blobs	 = true;

		// Dummys.
		{
			_dummy_color_texture	  = add_resource<texture>(DUMMY_COLOR_TEXTURE_SID);
			_dummy_orm_texture		  = add_resource<texture>(DUMMY_ORM_TEXTURE_SID);
			_dummy_normal_texture	  = add_resource<texture>(DUMMY_NORMAL_TEXTURE_SID);
			texture& dummy_color_txt  = get_resource<texture>(_dummy_color_texture);
			texture& dummy_orm_txt	  = get_resource<texture>(_dummy_orm_texture);
			texture& dummy_normal_txt = get_resource<texture>(_dummy_normal_texture);
			dummy_color_txt.create_from_loader(_dummy_color_raw, _world, _dummy_color_texture);
			dummy_normal_txt.create_from_loader(_dummy_normal_raw, _world, _dummy_normal_texture);
			dummy_orm_txt.create_from_loader(_dummy_orm_raw, _world, _dummy_orm_texture);

			// -----------------------------------------------------------------------------
			// default shaders
			// -----------------------------------------------------------------------------

			_default_gbuffer_shader	 = add_resource<shader>(DEFAULT_OPAQUE_SHADER_SID);
			_default_forward_shader	 = add_resource<shader>(DEFAULT_FORWARD_SHADER_SID);
			_default_gui_shader		 = add_resource<shader>(DEFAULT_GUI_SHADER_SID);
			_default_gui_text_shader = add_resource<shader>(DEFAULT_GUI_TEXT_SHADER_SID);
			_default_gui_sdf_shader	 = add_resource<shader>(DEFAULT_GUI_SDF_SHADER_SID);
			shader& gbuffer_sh		 = get_resource<shader>(_default_gbuffer_shader);
			shader& forward_sh		 = get_resource<shader>(_default_forward_shader);
			shader& gui_sh			 = get_resource<shader>(_default_gui_shader);
			shader& gui_text_sh		 = get_resource<shader>(_default_gui_text_shader);
			shader& gui_sdf_sh		 = get_resource<shader>(_default_gui_sdf_shader);

			gbuffer_sh.create_from_loader(sh_raw_gbuffer, _world, _default_gbuffer_shader);
			forward_sh.create_from_loader(sh_raw_fw, _world, _default_forward_shader);
			gui_sh.create_from_loader(sh_raw_gui, _world, _default_gui_shader);
			gui_text_sh.create_from_loader(sh_raw_gui_text, _world, _default_gui_text_shader);
			gui_sdf_sh.create_from_loader(sh_raw_gui_sdf, _world, _default_gui_sdf_shader);

			// -----------------------------------------------------------------------------
			// default materials
			// -----------------------------------------------------------------------------
			_default_gui_mat			 = add_resource<material>(DEFAULT_GUI_MAT_SID);
			_default_gui_text_mat		 = add_resource<material>(DEFAULT_GUI_TEXT_MAT_SID);
			_default_gui_sdf_mat		 = add_resource<material>(DEFAULT_GUI_SDF_MAT_SID);
			material& default_gui_m		 = get_resource<material>(_default_gui_mat);
			material& default_gui_text_m = get_resource<material>(_default_gui_text_mat);
			material& default_gui_sdf_m	 = get_resource<material>(_default_gui_sdf_mat);

			default_gui_m.create_from_loader(mat_raw_gui, _world, _default_gui_mat);
			default_gui_text_m.create_from_loader(mat_raw_gui_text, _world, _default_gui_text_mat);
			default_gui_sdf_m.create_from_loader(mat_raw_gui_sdf, _world, _default_gui_sdf_mat);
		}

#ifdef SFG_TOOLMODE
		_file_watch.set_callback(resource_manager::on_watched_resource_modified, this);
		_file_watch.reserve(250);
		_file_watch.set_tick_interval(15);
		add_resource_watch(_default_gbuffer_shader, DEFAULT_OPAQUE_SHADER_PATH, {sh_raw_gbuffer.source}, type_id<shader>::value, SFG_ROOT_DIRECTORY);
		add_resource_watch(_default_forward_shader, DEFAULT_FORWARD_SHADER_PATH, {sh_raw_fw.source}, type_id<shader>::value, SFG_ROOT_DIRECTORY);
		add_resource_watch(_default_gui_shader, DEFAULT_GUI_SHADER_PATH, {sh_raw_gui.source}, type_id<shader>::value, SFG_ROOT_DIRECTORY);
		add_resource_watch(_default_gui_text_shader, DEFAULT_GUI_TEXT_SHADER_PATH, {sh_raw_gui_text.source}, type_id<shader>::value, SFG_ROOT_DIRECTORY);
		add_resource_watch(_default_gui_sdf_shader, DEFAULT_GUI_SDF_SHADER_PATH, {sh_raw_gui_sdf.source}, type_id<shader>::value, SFG_ROOT_DIRECTORY);
		add_resource_watch(_default_gui_mat, DEFAULT_GUI_MAT_PATH, {sh_raw_gui.source}, type_id<shader>::value, SFG_ROOT_DIRECTORY);
		add_resource_watch(_default_gui_text_mat, DEFAULT_GUI_TEXT_MAT_PATH, {sh_raw_gui_text.source}, type_id<shader>::value, SFG_ROOT_DIRECTORY);
		add_resource_watch(_default_gui_sdf_mat, DEFAULT_GUI_SDF_MAT_PATH, {sh_raw_gui_sdf.source}, type_id<shader>::value, SFG_ROOT_DIRECTORY);
#endif
	}

	void resource_manager::uninit()
	{
		for (cache_storage& stg : _storages)
		{
			stg.cache_ptr->reset(_world);
		}
		_aux_memory.reset();

#ifdef SFG_TOOLMODE
		_file_watch.clear();
#endif

		_dynamic_sampler_count = 0;
	}

	void resource_manager::tick()
	{
#ifdef SFG_TOOLMODE
		_file_watch.tick();
#endif
	}

	resource_handle resource_manager::get_or_add_sampler(const sampler_desc& desc)
	{
		const cache_storage& storage = get_storage(type_id<texture_sampler>::value);

		auto& sampler_pool = underlying_pool<resource_cache<texture_sampler, texture_sampler_raw, MAX_WORLD_SAMPLERS>, texture_sampler>();

		resource_handle out_handle = {};

		auto it = sampler_pool.handles_begin();
		for (auto it = sampler_pool.handles_begin(); it != sampler_pool.handles_end(); ++it)
		{
			const resource_handle  h   = *it;
			const texture_sampler& smp = get_resource<texture_sampler>(h);
			if (smp.get_desc() == desc)
			{
				out_handle = h;
				break;
			}
		}
		if (out_handle.is_null())
		{
			out_handle			 = add_resource<texture_sampler>(_dynamic_sampler_count++);
			texture_sampler& smp = get_resource<texture_sampler>(out_handle);

			texture_sampler_raw raw = {};
			raw.desc				= desc;
			smp.create_from_loader(raw, _world, out_handle);
		}

		return out_handle;
	}

	void resource_manager::load_resources(const vector<string>& relative_paths, package& pkg)
	{
		vector<string> filtered_relative_paths = {};
		filtered_relative_paths.reserve(relative_paths.size());

		for (const string& p : relative_paths)
		{
			if (p.empty())
				continue;

			auto it = std::find_if(filtered_relative_paths.begin(), filtered_relative_paths.end(), [&](const string& str) -> bool { return str.compare(p) == 0; });
			if (it == filtered_relative_paths.end())
				filtered_relative_paths.push_back(p);
		}

		const u32 size = static_cast<u32>(filtered_relative_paths.size());

		vector<void*>	  resolved_loaders(filtered_relative_paths.size());
		vector<string_id> resolved_types(filtered_relative_paths.size());

		vector<int> indices(filtered_relative_paths.size());
		std::iota(indices.begin(), indices.end(), 0);

		for (u32 i = 0; i < filtered_relative_paths.size(); i++)
		{
			const string&	path = filtered_relative_paths.at(i);
			const string_id sid	 = TO_SID(path);
			const size_t	dot	 = path.find_last_of(".");
			if (dot == string::npos)
			{
				SFG_ERR("could not deduce extension: {0}", path);
				continue;
			}

			const string ext = path.substr(dot + 1, path.size() - dot - 1);
			const meta*	 m	 = reflection::get().find_by_tag(ext.c_str());
			if (m == nullptr)
			{
				SFG_ASSERT(false, "no metadata found associated with this tag: {0}", ext);
				continue;
			}

			const string_id type   = m->get_type_id();
			void*			loader = nullptr;

			const resource_handle h = get_resource_handle_by_hash_if_exists(type, TO_SID(path));
			if (!h.is_null())
				continue;

			istream stream;
			if (!pkg.get_stream(sid, stream))
				continue;

			loader				= load_from_stream(type, stream);
			resolved_loaders[i] = loader;
			resolved_types[i]	= type;
		}

		// create actual resources.
		const u32	   max_passes = _max_load_priority + 1;
		vector<string> dependencies;
		vector<string> out_subs = {};

		for (u32 pass = 0; pass < max_passes; pass++)
		{
			for (u32 i = 0; i < size; i++)
			{
				const string&	p	   = filtered_relative_paths[i];
				const string_id type   = resolved_types[i];
				void*			loader = resolved_loaders[i];

				if (loader == nullptr)
				{
					continue;
				}

				const cache_storage& stg = get_storage(type);
				if (pass != stg.load_priority)
				{
					continue;
				}

				out_subs.resize(0);
				get_loader_sub_resources(type, loader, out_subs);
				if (!out_subs.empty())
					load_resources(out_subs, pkg);

				const string_id hash = TO_SID(p);

				resource_handle handle = {};
				handle				   = add_from_loader(type, loader, pass, hash);
				if (handle.is_null())
					continue;

				SFG_INFO("loaded resource: {0}", p.c_str());
				// store by hash-path
				store_relative_path(type, handle, p);
				delete_loader(type, loader);
				resolved_loaders[i] = nullptr;
			}
		}
	}

#ifdef SFG_TOOLMODE

	void resource_manager::load_resources(const vector<string>& relative_paths, bool skip_cache, const char* root_directory)
	{
		vector<string> filtered_relative_paths = {};
		filtered_relative_paths.reserve(relative_paths.size());

		for (const string& p : relative_paths)
		{
			if (p.empty())
				continue;

			auto it = std::find_if(filtered_relative_paths.begin(), filtered_relative_paths.end(), [&](const string& str) -> bool { return str.compare(p) == 0; });
			if (it == filtered_relative_paths.end())
				filtered_relative_paths.push_back(p);
		}

		const u32	 size		 = static_cast<u32>(filtered_relative_paths.size());
		const string working_dir = root_directory == nullptr ? editor_settings::get().working_dir : (root_directory);

		vector<void*>	  resolved_loaders(filtered_relative_paths.size());
		vector<string_id> resolved_types(filtered_relative_paths.size());

		vector<int> indices(filtered_relative_paths.size());
		std::iota(indices.begin(), indices.end(), 0);

		std::for_each(std::execution::par, indices.begin(), indices.end(), [&](int& i) {
			const string&	path = filtered_relative_paths.at(i);
			const string_id sid	 = TO_SID(path);

			const size_t dot = path.find_last_of(".");

			if (dot == string::npos)
			{
				SFG_ERR("Could not deduce extension: {0}", path);
				return;
			}

			const string ext = path.substr(dot + 1, path.size() - dot - 1);
			const meta*	 m	 = reflection::get().find_by_tag(ext.c_str());

			if (m == nullptr)
			{
				SFG_ASSERT(false, "No metadata found associated with this tag: {0}", ext);
				return;
			}

			const string_id type	  = m->get_type_id();
			void*			loader	  = nullptr;
			const string&	cache_dir = editor_settings::get().cache_dir;

			const resource_handle h = get_resource_handle_by_hash_if_exists(type, TO_SID(path));
			if (!h.is_null())
				return;

			if (!skip_cache)
			{
				loader = load_from_cache(type, cache_dir.c_str(), path.c_str(), ".stkcache");
			}

			if (!loader)
			{
				loader = load_from_file(type, path.c_str(), working_dir.c_str());
				if (loader)
					save_to_cache(type, loader, cache_dir.c_str(), working_dir.c_str(), ".stkcache");
			}

			resolved_loaders[i] = loader;
			resolved_types[i]	= type;
		});

		// create actual resources.
		const u32	   max_passes = _max_load_priority + 1;
		vector<string> dependencies;
		vector<string> out_subs = {};

		for (u32 pass = 0; pass < max_passes; pass++)
		{
			for (u32 i = 0; i < size; i++)
			{
				const string&	p	   = filtered_relative_paths[i];
				const string_id type   = resolved_types[i];
				void*			loader = resolved_loaders[i];

				if (loader == nullptr)
					continue;

				const cache_storage& stg = get_storage(type);
				if (pass != stg.load_priority)
					continue;

				out_subs.resize(0);
				get_loader_sub_resources(type, loader, out_subs);
				load_resources(out_subs, skip_cache, root_directory);

				const string_id hash = TO_SID(p);

				dependencies.resize(0);
				get_dependencies(type, loader, dependencies);

				resource_handle handle = {};
				handle				   = add_from_loader(type, loader, pass, hash);
				if (handle.is_null())
					continue;

				SFG_INFO("loaded resource: {0}", p.c_str());

				// store by hash-path
				store_relative_path(type, handle, p);
				delete_loader(type, loader);
				resolved_loaders[i] = nullptr;

				add_resource_watch(handle, p.c_str(), dependencies, type, root_directory == nullptr ? editor_settings::get().working_dir.c_str() : root_directory);
			}
		}
	}

	void resource_manager::add_resource_watch(resource_handle base_handle, const char* relative_path, const vector<string>& dependencies, string_id type, const char* root_dir)
	{
		_watched_resources.push_back({});
		resource_watch& w = _watched_resources.back();
		w.type_id		  = type;
		w.path			  = relative_path;
		w.base_handle	  = base_handle;
		w.dependencies	  = dependencies;
		w.root_dir		  = root_dir;

		const string full_path = w.root_dir + w.path;
		const u16	 id		   = static_cast<u16>(_watched_resources.size() - 1);
		_file_watch.add_path(full_path.c_str(), id);

		for (const string& str : dependencies)
		{
			const string p = w.root_dir + str;
			_file_watch.add_path(p.c_str(), id);
		}
	}

	void resource_manager::on_watched_resource_modified(const char* path, u64 last_modified, u16 id, void* user_data)
	{
		resource_manager* rm = static_cast<resource_manager*>(user_data);
		entity_manager&	  em = rm->_world.get_entity_manager();

		SFG_ASSERT(id < rm->_watched_resources.size());
		resource_watch& w = rm->_watched_resources[id];

		// load new resource
		const u64 ticks = time::get_cpu_microseconds();

		void* loader = rm->load_from_file(w.type_id, w.path.c_str(), w.root_dir.c_str());
		if (loader == nullptr)
			return;

		const resource_handle	prev_handle = w.base_handle;
		vector<resource_handle> prev_sub_handles;
		vector<string_id>		prev_sub_ids;

		if (w.type_id == type_id<model>::value)
		{
			model&				 m		= rm->get_resource<model>(w.base_handle);
			const chunk_handle32 meshes = m.get_created_meshes();
			const u16			 count	= m.get_mesh_count();
			if (count > 0)
			{
				resource_handle* mesh_handles = rm->_aux_memory.get<resource_handle>(meshes);
				for (u16 i = 0; i < count; i++)
				{
					const resource_handle handle = mesh_handles[i];
					prev_sub_handles.push_back(handle);
					mesh& mm = rm->get_resource<mesh>(handle);
					prev_sub_ids.push_back(mm.get_sid());
				}
			}
		}

		rm->destroy(w.type_id, w.base_handle);
		rm->remove_resource(w.type_id, w.base_handle);

		const string& cache_dir = editor_settings::get().cache_dir;
		rm->save_to_cache(w.type_id, loader, cache_dir.c_str(), w.root_dir.c_str(), ".stkcache");

		const u32 max_passes = rm->_max_load_priority + 1;

		resource_handle new_handle = {};
		for (u32 pass = 0; pass < max_passes; pass++)
		{
			new_handle = rm->add_from_loader(w.type_id, loader, pass, TO_SID(w.path));
			rm->store_relative_path(w.type_id, new_handle, w.path);

			if (!new_handle.is_null())
			{
				rm->delete_loader(w.type_id, loader);
				break;
			}
		}

		SFG_ASSERT(!new_handle.is_null());
		w.base_handle = new_handle;

		if (w.type_id == type_id<entity_template>::value)
		{
			const auto& entities = em.get_entities();
			for (auto it = entities->handles_begin(); it != entities->handles_end(); ++it)
			{
				const world_handle	  eh		   = *it;
				const resource_handle existing_ref = em.get_entity_template_ref(eh);
				if (existing_ref == prev_handle)
					em.set_entity_template(eh, new_handle);
			}
		}
		else if (w.type_id == type_id<shader>::value)
		{
			const render_event_resource_reloaded ev = {
				.prev_id = prev_handle.index,
				.new_id	 = new_handle.index,
			};

			rm->_world.get_render_stream().add_event({.event_type = render_event_type::reload_shader}, ev);
		}
		else if (w.type_id == type_id<model>::value)
		{
			vector<resource_handle> new_sub_handles;
			vector<string_id>		new_sids;

			model&				 m		= rm->get_resource<model>(w.base_handle);
			const chunk_handle32 meshes = m.get_created_meshes();

			const u16 count = m.get_mesh_count();
			if (count > 0)
			{
				resource_handle* mesh_handles = rm->_aux_memory.get<resource_handle>(meshes);
				for (u16 i = 0; i < count; i++)
				{
					const resource_handle handle = mesh_handles[i];
					const mesh&			  mm	 = rm->get_resource<mesh>(handle);
					new_sub_handles.push_back(handle);
					new_sids.push_back(mm.get_sid());
				}
			}

			em.reload_instantiated_model(prev_handle, new_handle);
		}
		else if (w.type_id == type_id<particle_properties>::value)
		{
			component_manager& cm				 = rm->_world.get_comp_manager();
			auto&			   particle_emitters = cm.underlying_pool<comp_cache<comp_particle_emitter, MAX_WORLD_COMP_PARTICLE_EMITTERS>, comp_particle_emitter>();
			for (comp_particle_emitter& pe : particle_emitters)
			{
				if (pe.get_particle_resource() == prev_handle)
				{
					pe.set_values(rm->_world, new_handle, pe.get_material());
				}
			}
		}
		else if (w.type_id == type_id<physical_material>::value)
		{
			component_manager& cm				   = rm->_world.get_comp_manager();
			auto&			   physical_components = cm.underlying_pool<comp_cache<comp_physics, MAX_WORLD_COMP_PHYSICS>, comp_physics>();
			for (comp_physics& p : physical_components)
			{
				if (p.get_material_handle() == prev_handle)
					p.set_material_handle(new_handle);
			}
		}
		else
		{
			SFG_WARN("Reloading of this resource type is not supported yet: {0}", path);
			return;
		}

		SFG_INFO("reloaded resource: {0}", path);
	}
#endif

	resource_handle resource_manager::add_resource(string_id type, string_id hash)
	{
		const cache_storage& stg = get_storage(type);
		return stg.cache_ptr->add(hash);
	}

	void resource_manager::remove_resource(string_id type, resource_handle handle)
	{
		const cache_storage& stg = get_storage(type);
		return stg.cache_ptr->remove(handle);
	}

	void* resource_manager::get_resource(string_id type, resource_handle handle) const
	{
		const cache_storage& stg = get_storage(type);
		return stg.cache_ptr->get_ptr(handle);
	}

	void* resource_manager::get_resource_by_hash(string_id type, string_id hash) const
	{
		const cache_storage& stg = get_storage(type);
		return stg.cache_ptr->get_by_hash_ptr(hash);
	}

	resource_handle resource_manager::get_resource_handle_by_hash(string_id type, string_id hash) const
	{
		const cache_storage& stg = get_storage(type);
		return stg.cache_ptr->get_handle_by_hash(hash);
	}

	resource_handle resource_manager::get_resource_handle_by_hash_if_exists(string_id type, string_id hash) const
	{
		const cache_storage& stg = get_storage(type);
		return stg.cache_ptr->get_handle_by_hash_if_exists(hash);
	}

	string_id resource_manager::get_resource_hash(string_id type, resource_handle handle) const
	{
		const cache_storage& stg = get_storage(type);
		return stg.cache_ptr->get_hash(handle);
	}

	const string& resource_manager::get_loaded_path_by_handle(string_id type, resource_handle h) const
	{
		return get_storage(type).cache_ptr->_paths_by_hashes.at(get_resource_hash(type, h));
	}

	void* resource_manager::load_from_stream(string_id type, istream& stream) const
	{
		const cache_storage& stg	= get_storage(type);
		void*				 loader = stg.cache_ptr->load_from_stream(stream);
		return loader;
	}

	void resource_manager::save_to_stream(string_id type, const void* loader, ostream& stream) const
	{
		const cache_storage& stg = get_storage(type);
		stg.cache_ptr->save_to_stream(loader, stream);
	}

#ifdef SFG_TOOLMODE

	void* resource_manager::load_from_cache(string_id type, const char* cache_folder_path, const char* relative_path, const char* extension) const
	{
		const cache_storage& stg	= get_storage(type);
		void*				 loader = stg.cache_ptr->load_from_cache(cache_folder_path, relative_path, extension);
		return loader;
	}

	void resource_manager::save_to_cache(string_id type, const void* loader, const char* cache_folder_path, const char* resource_directory_path, const char* extension) const
	{
		const cache_storage& stg = get_storage(type);
		stg.cache_ptr->save_to_cache(loader, cache_folder_path, resource_directory_path, extension);
	}

	void* resource_manager::load_from_file(string_id type, const char* relative_file, const char* base_path) const
	{
		const cache_storage& stg	= get_storage(type);
		void*				 loader = stg.cache_ptr->load_from_file(relative_file, base_path);
		return loader;
	}

	void resource_manager::get_dependencies(string_id type, const void* loader, vector<string>& out_dependencies) const
	{
		const cache_storage& stg = get_storage(type);
		stg.cache_ptr->get_dependencies(loader, out_dependencies);
	}
#endif

	resource_handle resource_manager::add_from_loader(string_id type, void* loader, u32 priority, string_id hash) const
	{
		const cache_storage& stg = get_storage(type);
		if (priority != stg.load_priority)
			return {};
		return stg.cache_ptr->add_from_loader(loader, _world, hash);
	}

	void resource_manager::get_loader_sub_resources(string_id type, void* loader, vector<string>& out_subs) const
	{
		const cache_storage& stg = get_storage(type);
		return stg.cache_ptr->get_loader_sub_resources(loader, out_subs);
	}

	void resource_manager::delete_loader(string_id type, void* loader) const
	{
		const cache_storage& stg = get_storage(type);
		stg.cache_ptr->delete_loader(loader);
	}

	void resource_manager::destroy(string_id type, resource_handle handle)
	{
		const cache_storage& stg = get_storage(type);
		stg.cache_ptr->destroy(handle, _world);
	}

	bool resource_manager::is_valid(string_id type, resource_handle handle) const
	{
		const cache_storage& stg = get_storage(type);
		return stg.cache_ptr->is_valid(handle);
	}

	void resource_manager::store_relative_path(string_id type, resource_handle handle, const string& p)
	{
		const string_id hash								= get_resource_hash(type, handle);
		get_storage(type).cache_ptr->_paths_by_hashes[hash] = p;
	}

	const resource_manager::cache_storage& resource_manager::get_storage(string_id type) const
	{
		auto it = std::find_if(_storages.begin(), _storages.end(), [type](const cache_storage& stg) -> bool { return stg.type == type; });
		if (it == _storages.end())
		{
			SFG_ASSERT(false);
			throw std::runtime_error("Storage can't be found, forgot to register it?");
		}
		return *it;
	}

	resource_manager::cache_storage& resource_manager::get_storage(string_id type)
	{
		auto it = std::find_if(_storages.begin(), _storages.end(), [type](const cache_storage& stg) -> bool { return stg.type == type; });
		if (it == _storages.end())
		{
			SFG_ASSERT(false);
			throw std::runtime_error("Storage can't be found, forgot to register it?");
		}
		return *it;
	}

}
