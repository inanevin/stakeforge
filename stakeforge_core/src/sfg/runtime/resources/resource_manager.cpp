// Copyright (c) 2025 Inan Evin

#include "resource_manager.hpp"
#include "material.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/vec2f.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/engine/engine_threads.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/runtime/resources/resource_file_system.hpp>

#include <sfg/platform/time.hpp>

namespace sfg
{
	resource_manager_t& resource_manager_t::get()
	{
		static resource_manager_t instance;
		return instance;
	}

	void resource_manager_t::init(resource_file_system_t& resource_file_system, const resource_manager_config_t& config)
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(config.memory_budget_bytes != 0);
		SFG_ASSERT(_reload_listeners.empty());

		_resource_file_system = &resource_file_system;
		_generation			  = 0;

		_memory.init(config.memory_budget_bytes);

		_entries.reserve(config.resource_initial_capacity);
		_dirty_materials.reserve(config.dirty_material_initial_capacity);

		if (config.reload_listener_initial_capacity != 0)
			_reload_listeners.reserve(config.reload_listener_initial_capacity);
	}

	void resource_manager_t::init(resource_file_system_t& resource_file_system, size_t resource_memory_size)
	{
		init(resource_file_system, {.memory_budget_bytes = resource_memory_size});
	}

	void resource_manager_t::init_atlases(const ui::glyph_atlas_config_t& glyph_atlas_config)
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		_glyph_atlas.init(glyph_atlas_config);
	}

	void resource_manager_t::uninit_atlases()
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		if (_glyph_atlas.is_initialized())
			_glyph_atlas.uninit();
	}

	void resource_manager_t::uninit()
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(!SFG_IS_RENDER_RUNNING());
		SFG_ASSERT(_reload_listeners.empty());

		_texture_streamer.flush_completed(*this);

		for (auto& pair : _entries)
		{
			unload_entry(pair.second);
			_generation++;
		}

		_entries.clear();
		_dirty_materials.resize(0);

		uninit_atlases();
		_memory.uninit();
		_reload_listeners.clear();
		_resource_file_system = nullptr;
		_generation			  = 0;
	}

	void resource_manager_t::flush()
	{
		SFG_ASSERT(is_main_thread());

		_texture_streamer.flush_completed(*this);
	}

	resource_state_e resource_manager_t::load_resource(sid_t hash, resource_type_e type)
	{
		SFG_ASSERT(is_main_thread());

		auto	   it	  = _entries.find(hash);
		const bool exists = it != _entries.end();

		if (exists)
		{
			it->second.ref_count++;
			_generation++;
			return it->second.state;
		}

		const resource_type_desc_t* desc = find_resource_type_desc(type);

		if (desc == nullptr)
		{
			SFG_WARN("failed loading resource, type description not found! {0}", static_cast<u8>(type));
			return resource_state_e::failed;
		}

		if (desc->load == nullptr)
		{
			SFG_WARN("failed loading resource, load not found! {0}", static_cast<u8>(type));
			return resource_state_e::failed;
		}

		ostream_t header_stream = {};

		if (!_resource_file_system->read_resource(hash, 0, sizeof(resource_header_t), header_stream))
		{
			SFG_WARN("failed reading resource header: {0}", hash);
			return resource_state_e::failed;
		}

		istream_t		  header_data = {};
		resource_header_t header	  = {};

		header_data.open(header_stream.get_raw(), header_stream.get_size());
		header.deserialize(header_data);
		if (header.type != type)
		{
			SFG_ERR("deserialized resource type does not match the requested resource type :( header: {0} res: {1}, hash: {2}", resource_type_to_string(header.type), resource_type_to_string(type), hash);
			return resource_state_e::failed;
		}

		SFG_ASSERT(header.type == type);

		const char*		   debug_name	  = header.debug_name;
		const size_t	   payload_offset = sizeof(resource_header_t) + static_cast<size_t>(header.dependency_count) * sizeof(resource_dependency_t);
		resource_context_t ctx{*this};

		resource_entry_t entry = {};
		entry.type			   = type;
		entry.ref_count		   = 1;
		entry.hash			   = hash;
		entry.source_ticks	   = header.source_tick;
		entry.runtime		   = _memory.allocate_bytes(desc->runtime_size, desc->runtime_alignment);
		entry.internals		   = _memory.allocate_bytes(desc->internals_size, desc->internals_alignment);
		entry.state			   = resource_state_e::failed;
		entry.debug_name	   = _memory.allocate_text(debug_name);

		if (header.dependency_count != 0)
		{
			entry.dependencies	   = _memory.allocate<resource_dependency_t>(header.dependency_count);
			entry.dependency_count = header.dependency_count;

			ostream_t dependency_stream = {};

			if (!_resource_file_system->read_resource(hash, sizeof(resource_header_t), payload_offset - sizeof(resource_header_t), dependency_stream))
			{
				SFG_WARN("failed reading resource dependencies for {0}", header.debug_name);
				free_entry(entry);
				return resource_state_e::failed;
			}

			istream_t dependency_data = {};

			dependency_data.open(dependency_stream.get_raw(), dependency_stream.get_size());
			resource_dependency_t* deps = _memory.get<resource_dependency_t>(entry.dependencies);

			for (u32 i = 0; i < header.dependency_count; i++)
			{
				dependency_data >> deps[i];

				if (load_resource(deps[i].handle, deps[i].type) == resource_state_e::failed)
					SFG_WARN("failed loading dependency for {0}", header.debug_name);
			}
		}

		auto [entry_it, inserted] = _entries.emplace(hash, entry);

		SFG_ASSERT(inserted);
		resource_entry_t& loaded_entry = entry_it->second;

		if (!desc->load(loaded_entry, ctx, *_resource_file_system, payload_offset))
		{
			SFG_WARN("failed loading resource: {0} {1}", debug_name, hash);
			unload_dependencies(loaded_entry);
			free_entry(loaded_entry);
			_entries.erase(entry_it);
			return resource_state_e::failed;
		}

		loaded_entry.state = resource_state_e::ready;
		_generation++;
		SFG_TRACE("loaded resource: {0}", debug_name);

		return _entries.find(hash)->second.state;
	}

	resource_state_e resource_manager_t::reload_resource(sid_t hash)
	{
		SFG_ASSERT(is_main_thread());

		auto it = _entries.find(hash);

		if (it == _entries.end())
		{
			SFG_WARN("can't reload resource as it's not loaded! {0}", hash);
			return resource_state_e::failed;
		}

		resource_entry_t&	  entry		= it->second;
		const u32			  ref_count = entry.ref_count;
		const resource_type_e type		= entry.type;

		SFG_ASSERT(ref_count != 0);

		unload_dependencies(entry);
		unload_entry(entry);
		_entries.erase(it);
		_generation++;

		const resource_state_e state = load_resource(hash, type);

		if (state == resource_state_e::failed)
			return state;

		auto reloaded_it = _entries.find(hash);
		SFG_ASSERT(reloaded_it != _entries.end());
		reloaded_it->second.ref_count = ref_count;

		notify_reload(hash, type);

		return state;
	}

	resource_reload_listener_handle_t resource_manager_t::add_reload_listener(resource_reload_listener_fn fn, void* user_data)
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(fn != nullptr);

		const resource_reload_listener_handle_t handle	 = _reload_listeners.emplace();
		resource_reload_listener_t&				listener = _reload_listeners.get(handle);
		listener.fn										 = fn;
		listener.user_data								 = user_data;

		return handle;
	}

	void resource_manager_t::remove_reload_listener(resource_reload_listener_handle_t handle)
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(_reload_listeners.is_valid(handle));

		_reload_listeners.remove(handle);
	}

	void resource_manager_t::notify_reload(sid_t resource_id, resource_type_e resource_type)
	{
		for (auto it = _reload_listeners.begin_handle(); it != _reload_listeners.end_handle(); ++it)
		{
			const resource_reload_listener_handle_t handle	 = *it;
			const resource_reload_listener_t&		listener = _reload_listeners.get(handle);

			listener.fn(*this, resource_id, resource_type, listener.user_data);
		}
	}

	void resource_manager_t::update_material_parameter(resource_handle_t material, sid_t parameter_name, f32 value)
	{
		update_material_parameter_data(material, parameter_name, shader_param_type_e::f32, &value, sizeof(value));
	}

	void resource_manager_t::update_material_parameter(resource_handle_t material, sid_t parameter_name, const vec2f_t& value)
	{
		update_material_parameter_data(material, parameter_name, shader_param_type_e::vec2, &value, sizeof(value));
	}

	void resource_manager_t::update_material_parameter(resource_handle_t material, sid_t parameter_name, const vec4f_t& value)
	{
		update_material_parameter_data(material, parameter_name, shader_param_type_e::vec4, &value, sizeof(value));
	}

	void resource_manager_t::update_material_parameter(resource_handle_t material, sid_t parameter_name, u32 value)
	{
		update_material_parameter_data(material, parameter_name, shader_param_type_e::u32, &value, sizeof(value));
	}

	void resource_manager_t::update_material_parameter_data(resource_handle_t material, sid_t parameter_name, shader_param_type_e type, const void* data, size_t data_size)
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(material != NULL_RESOURCE_HANDLE);
		SFG_ASSERT(parameter_name != NULL_SID);
		SFG_ASSERT(data != nullptr);

		auto entry_it = _entries.find(material);
		SFG_ASSERT(entry_it != _entries.end());
		SFG_ASSERT(entry_it->second.type == resource_type_e::material);
		SFG_ASSERT(entry_it->second.state == resource_state_e::ready);

		material_runtime_t* runtime			= _memory.get<material_runtime_t>(entry_it->second.runtime);
		u32					parameter_index = UINT32_MAX;

		for (u32 i = 0; i < runtime->parameter_count; ++i)
		{
			if (runtime->parameter_name_hashes[i] == parameter_name)
			{
				parameter_index = i;
				break;
			}
		}

		SFG_ASSERT(parameter_index != UINT32_MAX);

		material_runtime_parameter_t& parameter = runtime->parameters[parameter_index];
		SFG_ASSERT(parameter.type == type);

		if (SFG_MEMCMP(parameter.values, data, data_size) == 0)
			return;

		SFG_MEMCPY(parameter.values, data, data_size);

		if (runtime->parameters_dirty == 0)
		{
			runtime->parameters_dirty = 1;
			_dirty_materials.push_back(material);
		}
	}

	void resource_manager_t::update_material_texture(resource_handle_t material, sid_t texture_name, resource_handle_t texture)
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(material != NULL_RESOURCE_HANDLE);
		SFG_ASSERT(texture_name != NULL_SID);
		SFG_ASSERT(texture != NULL_RESOURCE_HANDLE);

		auto material_it = _entries.find(material);
		SFG_ASSERT(material_it != _entries.end());
		SFG_ASSERT(material_it->second.type == resource_type_e::material);
		SFG_ASSERT(material_it->second.state == resource_state_e::ready);

		material_runtime_t* runtime		  = _memory.get<material_runtime_t>(material_it->second.runtime);
		u32					texture_index = UINT32_MAX;

		for (u32 i = 0; i < runtime->texture_count; ++i)
		{
			if (runtime->texture_name_hashes[i] == texture_name)
			{
				texture_index = i;
				break;
			}
		}

		SFG_ASSERT(texture_index != UINT32_MAX);

		const auto texture_it = _entries.find(texture);
		SFG_ASSERT(texture_it != _entries.end());
		SFG_ASSERT(texture_it->second.state == resource_state_e::ready);

		const resource_type_e expected_type = shader_texture_type_to_resource_type(runtime->texture_types[texture_index]);
		SFG_ASSERT(texture_it->second.type == expected_type);

		runtime->texture_guids[texture_index] = texture;
	}

	void resource_manager_t::update_material_sampler(resource_handle_t material, sid_t sampler_name, resource_handle_t sampler)
	{
		SFG_ASSERT(is_main_thread());
		SFG_ASSERT(material != NULL_RESOURCE_HANDLE);
		SFG_ASSERT(sampler_name != NULL_SID);
		SFG_ASSERT(sampler != NULL_RESOURCE_HANDLE);

		auto material_it = _entries.find(material);
		SFG_ASSERT(material_it != _entries.end());
		SFG_ASSERT(material_it->second.type == resource_type_e::material);
		SFG_ASSERT(material_it->second.state == resource_state_e::ready);

		material_runtime_t* runtime		  = _memory.get<material_runtime_t>(material_it->second.runtime);
		u32					sampler_index = UINT32_MAX;

		for (u32 i = 0; i < runtime->sampler_count; ++i)
		{
			if (runtime->sampler_name_hashes[i] == sampler_name)
			{
				sampler_index = i;
				break;
			}
		}

		SFG_ASSERT(sampler_index != UINT32_MAX);

		const auto sampler_it = _entries.find(sampler);
		SFG_ASSERT(sampler_it != _entries.end());
		SFG_ASSERT(sampler_it->second.state == resource_state_e::ready);
		SFG_ASSERT(sampler_it->second.type == resource_type_e::texture_sampler);

		runtime->sampler_guids[sampler_index] = sampler;
	}

	bool resource_manager_t::is_material_parameter_valid(resource_handle_t material, sid_t parameter_name, shader_param_type_e parameter_type) const
	{
		if (material == NULL_RESOURCE_HANDLE || parameter_name == NULL_SID || parameter_type == shader_param_type_e::invalid)
			return false;

		const resource_entry_t* material_entry = find_entry(material);

		if (material_entry == nullptr || material_entry->type != resource_type_e::material || material_entry->state != resource_state_e::ready || material_entry->runtime.size == 0)
			return false;

		const material_runtime_t* runtime = _memory.get<material_runtime_t>(material_entry->runtime);

		for (u32 parameter_index = 0; parameter_index < runtime->parameter_count; ++parameter_index)
		{
			if (runtime->parameter_name_hashes[parameter_index] == parameter_name)
				return runtime->parameters[parameter_index].type == parameter_type;
		}

		return false;
	}

	bool resource_manager_t::is_material_texture_valid(resource_handle_t material, sid_t texture_name, resource_handle_t texture) const
	{
		if (material == NULL_RESOURCE_HANDLE || texture_name == NULL_SID || texture == NULL_RESOURCE_HANDLE)
			return false;

		const resource_entry_t* material_entry = find_entry(material);
		const resource_entry_t* texture_entry  = find_entry(texture);

		if (material_entry == nullptr || material_entry->type != resource_type_e::material || material_entry->state != resource_state_e::ready || material_entry->runtime.size == 0 || texture_entry == nullptr || texture_entry->state != resource_state_e::ready)
			return false;

		const material_runtime_t* runtime = _memory.get<material_runtime_t>(material_entry->runtime);

		for (u32 texture_index = 0; texture_index < runtime->texture_count; ++texture_index)
		{
			if (runtime->texture_name_hashes[texture_index] != texture_name)
				continue;

			return texture_entry->type == shader_texture_type_to_resource_type(runtime->texture_types[texture_index]);
		}

		return false;
	}

	bool resource_manager_t::is_material_sampler_valid(resource_handle_t material, sid_t sampler_name, resource_handle_t sampler) const
	{
		if (material == NULL_RESOURCE_HANDLE || sampler_name == NULL_SID || sampler == NULL_RESOURCE_HANDLE)
			return false;

		const resource_entry_t* material_entry = find_entry(material);
		const resource_entry_t* sampler_entry  = find_entry(sampler);

		if (material_entry == nullptr || material_entry->type != resource_type_e::material || material_entry->state != resource_state_e::ready || material_entry->runtime.size == 0 || sampler_entry == nullptr ||
			sampler_entry->type != resource_type_e::texture_sampler || sampler_entry->state != resource_state_e::ready)
			return false;

		const material_runtime_t* runtime = _memory.get<material_runtime_t>(material_entry->runtime);

		for (u32 sampler_index = 0; sampler_index < runtime->sampler_count; ++sampler_index)
		{
			if (runtime->sampler_name_hashes[sampler_index] == sampler_name)
				return true;
		}

		return false;
	}

	void resource_manager_t::flush_material_updates()
	{
		SFG_ASSERT(is_main_thread());

		for (resource_handle_t material : _dirty_materials)
		{
			auto entry_it = _entries.find(material);

			if (entry_it == _entries.end() || entry_it->second.type != resource_type_e::material)
				continue;

			material_runtime_t* runtime = _memory.get<material_runtime_t>(entry_it->second.runtime);

			if (runtime->parameters_dirty == 0)
				continue;

			const material_internals_t* internals											 = _memory.get<material_internals_t>(entry_it->second.internals);
			u8							parameter_data[SFG_MATERIAL_MAX_PARAMETER_DATA_SIZE] = {};
			pack_material_parameters(*runtime, parameter_data);

			render_material_parameter_update_desc_t desc = {
				.material  = material,
				.data	   = parameter_data,
				.data_size = static_cast<u16>(runtime->parameter_data_size),
			};

			for (u8 frame_index = 0; frame_index < BACK_BUFFER_COUNT; ++frame_index)
				desc.parameter_buffers[frame_index] = internals->parameter_buffers[frame_index];

			render_resources_t::get().enqueue_material_parameter_update(desc);
			runtime->parameters_dirty = 0;
		}

		_dirty_materials.resize(0);
	}

	resource_state_e resource_manager_t::load_resource_runtime(sid_t hash, resource_type_e type, istream_t& stream)
	{
		SFG_ASSERT(is_main_thread());

		auto	   it	  = _entries.find(hash);
		const bool exists = it != _entries.end();
		if (exists)
		{
			it->second.ref_count++;
			_generation++;
			return it->second.state;
		}

		const resource_type_desc_t* desc = find_resource_type_desc(type);
		if (desc == nullptr)
		{
			SFG_WARN("failed loading runtime resource, type description not found! {0}", static_cast<u8>(type));
			return resource_state_e::failed;
		}

		if (desc->runtime_load == nullptr)
		{
			SFG_WARN("failed loading runtime resource, runtime load not found! {0}", static_cast<u8>(type));
			return resource_state_e::failed;
		}

		resource_entry_t entry = {};
		entry.type			   = type;
		entry.ref_count		   = 1;
		entry.hash			   = hash;
		entry.runtime		   = _memory.allocate_bytes(desc->runtime_size, desc->runtime_alignment);
		entry.internals		   = _memory.allocate_bytes(desc->internals_size, desc->internals_alignment);
		entry.state			   = resource_state_e::failed;
		entry.debug_name	   = _memory.allocate_text("runtime_resource");

		auto [entry_it, inserted] = _entries.emplace(hash, entry);
		SFG_ASSERT(inserted);
		resource_entry_t&  loaded_entry = entry_it->second;
		resource_context_t ctx{*this};

		if (!desc->runtime_load(loaded_entry, ctx, stream))
		{
			SFG_WARN("failed loading runtime resource: {0}", hash);
			free_entry(loaded_entry);
			_entries.erase(entry_it);
			return resource_state_e::failed;
		}

		loaded_entry.state = resource_state_e::ready;
		_generation++;
		SFG_TRACE("loaded runtime resource: {0}", hash);
		return _entries.find(hash)->second.state;
	}

	void resource_manager_t::unload_resource(sid_t hash, bool force)
	{
		SFG_ASSERT(is_main_thread());

		auto it = _entries.find(hash);

		if (it == _entries.end())
		{
			SFG_WARN("can't unload resource as it's not loaded! {0}", hash);
			return;
		}

		resource_entry_t& entry = it->second;

		if (entry.ref_count == 0)
			return;

		if (force)
			entry.ref_count = 0;
		else
			entry.ref_count--;

		_generation++;

		if (entry.ref_count != 0)
			return;

		unload_dependencies(entry);

		unload_entry(entry);
		_entries.erase(it);
		_generation++;
	}

	const resource_entry_t* resource_manager_t::find_entry(u64 hash) const
	{
		auto it = _entries.find(hash);

		if (it == _entries.end())
			return nullptr;

		return &it->second;
	}

	void resource_manager_t::unload_dependencies(resource_entry_t& entry)
	{
		if (entry.dependency_count == 0)
			return;

		resource_dependency_t* dependencies = _memory.get<resource_dependency_t>(entry.dependencies);

		for (u32 i = 0; i < entry.dependency_count; i++)
		{
			if (dependencies[i].handle != NULL_SID)
				unload_resource(dependencies[i].handle);
		}
	}

	void resource_manager_t::unload_entry(resource_entry_t& entry)
	{
		const resource_type_desc_t* desc = find_resource_type_desc(entry.type);
		SFG_ASSERT(desc != nullptr);

		const char* dbg = reinterpret_cast<const char*>(_memory.get(entry.debug_name.head));
		SFG_TRACE("unloaded resource: {0}", dbg);

		if (desc->unload != nullptr)
		{
			resource_context_t ctx{*this};
			desc->unload(entry, ctx);
		}

		free_entry(entry);
	}

	void resource_manager_t::free_entry(resource_entry_t& entry)
	{
		if (entry.runtime)
		{
			_memory.free(entry.runtime);
			entry.runtime = chunk_handle32_t{};
		}

		if (entry.internals)
		{
			_memory.free(entry.internals);
			entry.internals = chunk_handle32_t{};
		}

		if (entry.debug_name)
		{
			_memory.free(entry.debug_name);
			entry.debug_name = {};
		}

		if (entry.dependencies)
		{
			_memory.free(entry.dependencies);
			entry.dependencies = {};
		}
	}

}
