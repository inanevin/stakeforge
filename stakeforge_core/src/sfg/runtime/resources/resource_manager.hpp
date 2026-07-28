// Copyright (c) 2025 Inan Evin
#pragma once

#include "common_resources.hpp"
#include "resource_reload_listener.hpp"
#include "texture_streamer.hpp"
#include <sfg/data/hash_map.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/memory/chunk_allocator.hpp>
#include <sfg/memory/dynamic_gen_pool.hpp>
#include <sfg/runtime/ui/glyph_atlas.hpp>

namespace sfg
{
	class istream_t;
	class resource_file_system_t;
	struct vec2f_t;
	struct vec4f_t;
	enum class shader_param_type_e : u8;

	struct resource_manager_config_t
	{
		size_t memory_budget_bytes				= 64ull * 1024ull * 1024ull;
		u32	   resource_initial_capacity		= 256;
		u32	   dirty_material_initial_capacity	= 64;
		u32	   reload_listener_initial_capacity = 0;
	};

	class resource_manager_t final
	{
	public:
		static resource_manager_t& get();

		resource_manager_t()									 = default;
		~resource_manager_t()									 = default;
		resource_manager_t(const resource_manager_t&)			 = delete;
		resource_manager_t& operator=(const resource_manager_t&) = delete;

		// -----------------------------------------------------------------------------
		// lifetime
		// -----------------------------------------------------------------------------

		void init(resource_file_system_t& resource_file_system, const resource_manager_config_t& config = {});
		void init(resource_file_system_t& resource_file_system, size_t resource_memory_size);
		void init_atlases(const ui::glyph_atlas_config_t& glyph_atlas_config = {});
		void uninit_atlases();
		void uninit();
		void flush();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		resource_state_e				  load_resource(sid_t hash, resource_type_e type);
		resource_state_e				  load_resource_runtime(sid_t hash, resource_type_e type, istream_t& stream);
		resource_state_e				  reload_resource(sid_t hash);
		void							  unload_resource(sid_t hash, bool force = false);
		resource_reload_listener_handle_t add_reload_listener(resource_reload_listener_fn fn, void* user_data);
		void							  remove_reload_listener(resource_reload_listener_handle_t handle);
		void							  update_material_parameter(resource_handle_t material, sid_t parameter_name, f32 value);
		void							  update_material_parameter(resource_handle_t material, sid_t parameter_name, const vec2f_t& value);
		void							  update_material_parameter(resource_handle_t material, sid_t parameter_name, const vec4f_t& value);
		void							  update_material_parameter(resource_handle_t material, sid_t parameter_name, u32 value);
		void							  update_material_texture(resource_handle_t material, sid_t texture_name, resource_handle_t texture);
		void							  update_material_sampler(resource_handle_t material, sid_t sampler_name, resource_handle_t sampler);
		void							  flush_material_updates();
		const resource_entry_t*			  find_entry(u64 hash) const;
		void							  drain_atlases(u8 frame_slot);

		// -----------------------------------------------------------------------------
		// queries
		// -----------------------------------------------------------------------------

		template <typename T> inline const T* find_internals(u64 hash) const
		{
			const resource_entry_t* entry = find_entry(hash);
			if (entry == nullptr || entry->internals.size == 0)
				return nullptr;

			return _memory.get<T>(entry->internals);
		}

		template <typename T> inline const T* find_runtime(u64 hash)
		{
			const resource_entry_t* entry = find_entry(hash);
			if (entry == nullptr || entry->state == resource_state_e::failed || entry->runtime.size == 0)
				return nullptr;
			return _memory.get<T>(entry->runtime);
		}

		inline bool is_ready(u64 hash) const
		{
			const resource_entry_t* entry = find_entry(hash);
			return entry != nullptr && entry->state != resource_state_e::failed;
		}

		inline resource_state_e get_entry_state(u64 hash) const
		{
			const resource_entry_t* entry = find_entry(hash);
			return entry != nullptr ? entry->state : resource_state_e::failed;
		}

		bool is_material_parameter_valid(resource_handle_t material, sid_t parameter_name, shader_param_type_e parameter_type) const;
		bool is_material_texture_valid(resource_handle_t material, sid_t texture_name, resource_handle_t texture) const;
		bool is_material_sampler_valid(resource_handle_t material, sid_t sampler_name, resource_handle_t sampler) const;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline chunk_allocator_t& get_memory()
		{
			return _memory;
		}

		inline const chunk_allocator_t& get_memory() const
		{
			return _memory;
		}

		inline ui::glyph_atlas_t& get_glyph_atlas()
		{
			return _glyph_atlas;
		}

		inline const ui::glyph_atlas_t& get_glyph_atlas() const
		{
			return _glyph_atlas;
		}

		inline u64 get_generation() const
		{
			return _generation;
		}

		inline const hash_map_t<sid_t, resource_entry_t>& get_entries() const
		{
			return _entries;
		}

		inline resource_file_system_t& get_resource_file_system()
		{
			SFG_ASSERT(_resource_file_system != nullptr);
			return *_resource_file_system;
		}

		inline texture_streamer_t& get_texture_streamer()
		{
			return _texture_streamer;
		}

	private:
		struct resource_reload_listener_t
		{
			resource_reload_listener_fn fn		  = nullptr;
			void*						user_data = nullptr;
		};

		void unload_dependencies(resource_entry_t& entry);
		void unload_entry(resource_entry_t& entry);
		void free_entry(resource_entry_t& entry);
		void notify_reload(sid_t resource_id, resource_type_e resource_type);
		void update_material_parameter_data(resource_handle_t material, sid_t parameter_name, shader_param_type_e type, const void* data, size_t data_size);

	private:
		chunk_allocator_t																	_memory;
		hash_map_t<sid_t, resource_entry_t>													_entries;
		dynamic_gen_pool_t<resource_reload_listener_t, u32, resource_reload_listener_tag_t> _reload_listeners;
		ui::glyph_atlas_t																	_glyph_atlas;
		texture_streamer_t																	_texture_streamer;
		vector_t<resource_handle_t>															_dirty_materials;
		resource_file_system_t*																_resource_file_system = nullptr;
		u64																					_generation			  = 0;
	};
}
