// Copyright (c) 2025 Inan Evin

#include "resource_pack.hpp"
#include "resource_cache.hpp"
#include "resource_manager.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>

#if !defined(SFG_EMBED_ASSETS)
#include "animation_state_machine_cook.hpp"
#include "audio_cook.hpp"
#include "font_cook.hpp"
#include "glb_cook.hpp"
#include "material_cook.hpp"
#include "physical_material_cook.hpp"
#include "prefab_cook.hpp"
#include "resource_manifest.hpp"
#include "shader_cook.hpp"
#include "texture_cook.hpp"
#include "texture_sampler_cook.hpp"
#include <sfg/vendor/nhlohmann/json.hpp>
#endif

namespace sfg
{
#if !defined(SFG_EMBED_ASSETS)
	namespace
	{
		bool cook_for_schema(const string_t& schema, const nlohmann::json& config, const char* full_path, ostream_t& stream)
		{
			if (schema == "sfg.schema.texture")
			{
				const texture_cook_config_t cfg = config;
				return texture_cooker::cook_from_file(cfg, full_path, stream);
			}
			if (schema == "sfg.schema.shader")
			{
				const shader_cook_config_t cfg = config;
				return shader_cooker::cook_from_file(cfg, full_path, stream);
			}
			if (schema == "sfg.schema.audio")
			{
				const audio_cook_config_t cfg = config;
				return audio_cooker::cook_from_file(cfg, full_path, stream);
			}
			if (schema == "sfg.schema.font")
			{
				const font_cook_config_t cfg = config;
				return font_cooker::cook_from_file(cfg, full_path, stream);
			}
			if (schema == "sfg.schema.material")
			{
				return material_cooker::cook_from_file(full_path, stream);
			}
			if (schema == "sfg.schema.texture_sampler")
			{
				return texture_sampler_cooker::cook_from_json(config, stream);
			}
			if (schema == "sfg.schema.physical_material")
			{
				return physical_material_cooker::cook_from_file(full_path, stream);
			}
			if (schema == "sfg.schema.animation_state_machine")
			{
				return animation_state_machine_cooker::cook_from_file(full_path, stream);
			}
			if (schema == "sfg.schema.prefab")
			{
				return prefab_cooker::cook_from_file(full_path, stream);
			}
			if (schema == "sfg.schema.model")
			{
				const glb_cook_config_t cfg = config;
				return glb_cooker::cook_from_file(cfg, full_path, stream);
			}

			SFG_ERR("unknown cook schema: {0}", schema.c_str());
			return false;
		}

		bool cook_and_cache(const string_t& source_path, const string_t& name, const nlohmann::json& config, const string_t& cache_dir, span_t<u8>& out_data)
		{
			const string_t schema = config.value<string_t>("schema", "");
			ostream_t	   stream;
			if (!cook_for_schema(schema, config, source_path.c_str(), stream))
			{
				SFG_ERR("resource_pack: cook failed for {0}", source_path.c_str());
				return false;
			}

			if (!resource_cache_t::save(cache_dir.c_str(), name.c_str(), stream))
				SFG_WARN("resource_pack: cache save failed for {0}", name.c_str());

			out_data = stream.evict();
			return true;
		}
	}
#endif

#if defined(SFG_EMBED_ASSETS)
	bool resource_pack_t::init(resource_manager_t& mgr, const init_params_t& pa rams)
	{
		_mgr = &mgr;

		if (params.embedded_entries == nullptr || params.embedded_entry_count == 0)
		{
			SFG_WARN("resource_pack: embed mode but no embedded entries provided");
			return true;
		}

		_loaded.reserve(params.embedded_entry_count);
		for (u32 i = 0; i < params.embedded_entry_count; ++i)
		{
			const embedded_resource_t& e = params.embedded_entries[i];
			if (e.path == nullptr || e.data == nullptr || e.size == 0 || e.type == resource_type_e::invalid)
			{
				SFG_ERR("resource_pack: invalid embedded entry at index {0}", i);
				continue;
			}

			const sid_t		 sid  = TO_SID(e.path);
			const span_t<u8> data = {const_cast<u8*>(e.data), e.size};
			const auto		 st	  = mgr.load_resource(sid, data, e.type);
			if (st == resource_state_e::failed)
			{
				SFG_ERR("resource_pack: load_resource failed for embedded {0}", e.path);
				continue;
			}
			_loaded.push_back(sid);
		}

		return true;
	}

	void resource_pack_t::tick()
	{
	}
#else
	bool resource_pack_t::init(resource_manager_t& mgr, const init_params_t& params)
	{
		_mgr	   = &mgr;
		_cache_dir = params.cache_dir;

		if (!file_system_t::exists(params.manifest_path.c_str()))
		{
			SFG_WARN("resource_pack: no manifest at {0}", params.manifest_path.c_str());
			return true;
		}

		const string_t		 json_text = file_system_t::read_file_as_string(params.manifest_path.c_str());
		const nlohmann::json doc	   = nlohmann::json::parse(json_text, nullptr, false);
		if (doc.is_discarded())
		{
			SFG_ERR("resource_pack: failed to parse manifest at {0}", params.manifest_path.c_str());
			return false;
		}

		const resource_manifest_t manifest = doc;
		resource_cache_t::ensure_directory(_cache_dir.c_str());

		_loaded.reserve(manifest.resources.size());
		_watched.reserve(manifest.resources.size());
		_watcher.reserve(static_cast<int>(manifest.resources.size()));

		for (const resource_manifest_entry_t& entry : manifest.resources)
		{
			if (entry.type == resource_type_e::invalid)
			{
				SFG_ERR("resource_pack: invalid type for {0}", entry.path.c_str());
				return false;
			}

			const string_t source_path = params.assets_dir + entry.path;
			const sid_t	   sid		   = TO_SID(entry.path);

			if (!file_system_t::exists(source_path.c_str()))
			{
				SFG_ERR("resource_pack: source missing: {0}", source_path.c_str());
				return false;
			}

			const resource_type_desc_t* const desc	   = find_resource_type_desc(entry.type);
			resource_header_t				  expected = {
								.magic	 = desc != nullptr ? desc->wire_magic : 0,
								.version = desc != nullptr ? desc->wire_version : 0,
			};
			if (entry.type == resource_type_e::shader)
				shader_cooker::collect_source_ticks(source_path.c_str(), expected.source_ticks);
			else if (entry.type == resource_type_e::texture_sampler)
			{
				const string_t config_data = entry.config.dump();
				expected.source_ticks.push_back(hashing_t::hash_u64(config_data.data(), config_data.size()));
			}
			else
				expected.source_ticks.push_back(file_system_t::get_last_modified_ticks(source_path.c_str()));

			span_t<u8> data	  = {};
			istream_t  cached = resource_cache_t::try_load(_cache_dir.c_str(), entry.name.c_str(), expected);
			if (!cached.empty())
				data = cached.evict();
			else if (!cook_and_cache(source_path, entry.name, entry.config, _cache_dir, data))
				return false;

			const auto st = mgr.load_resource(sid, entry.path.c_str(), data, entry.type);
			if (st == resource_state_e::failed)
			{
				SFG_ERR("resource_pack: load_resource failed for {0}", source_path.c_str());
				return false;
			}

			_loaded.push_back(sid);

			const u16 id = static_cast<u16>(_watched.size());
			_watched.push_back({.source_path = source_path, .name = entry.name, .config_json = entry.config.dump(), .type = entry.type, .sid = sid});
			_watcher.add_path(source_path.c_str(), id);
		}

		_watcher.set_callback(&resource_pack_t::on_file_changed, this);

		return true;
	}

	void resource_pack_t::tick()
	{
		_watcher.tick();
	}

	void resource_pack_t::on_file_changed(const char*, u64, u16 id, void* user_data)
	{
		resource_pack_t* self = static_cast<resource_pack_t*>(user_data);
		self->recook_watched(id);
	}

	void resource_pack_t::recook_watched(u16 id)
	{
		SFG_ASSERT(id < _watched.size());
		const watched_entry_t& e = _watched[id];

		const nlohmann::json config = nlohmann::json::parse(e.config_json, nullptr, false);
		if (config.is_discarded())
		{
			SFG_ERR("resource_pack: failed to re-parse config for {0}", e.source_path.c_str());
			return;
		}

		span_t<u8> data = {};
		if (!cook_and_cache(e.source_path, e.name, config, _cache_dir, data))
			return;

		SFG_INFO("resource_pack: recooked {0}", e.source_path.c_str());

		// TODO:
		// _mgr->reload_resource(e.sid, data, e.type);
	}
#endif

	void resource_pack_t::uninit()
	{
		if (_mgr != nullptr)
		{
			for (sid_t sid : _loaded)
				_mgr->unload_resource(sid);
		}
		_loaded.clear();
		_mgr = nullptr;

#if !defined(SFG_EMBED_ASSETS)
		_watcher.clear();
		_watched.clear();
		_cache_dir.clear();
#endif
	}
}
