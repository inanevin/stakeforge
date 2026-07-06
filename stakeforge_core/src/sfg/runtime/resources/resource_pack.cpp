// Copyright (c) 2025 Inan Evin

#include "resource_pack.hpp"
#include "common_resources.hpp"
#include "resource_manager.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/common/descriptions.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/serialization/serialization.hpp>

#if !defined(SFG_EMBED_ASSETS)
#include "animation_cook.hpp"
#include "animation_def.hpp"
#include "animation_state_machine_cook.hpp"
#include "audio_cook.hpp"
#include "font_cook.hpp"
#include "material_cook.hpp"
#include "material_def.hpp"
#include "physical_material_cook.hpp"
#include "prefab.hpp"
#include "resource_manifest.hpp"
#include "shader_cook.hpp"
#include "skybox_hdr_cook.hpp"
#include "texture_cook.hpp"
#include "texture_sampler_cook.hpp"
#include <sfg/vendor/nhlohmann/json.hpp>
#include <utility>
#endif

namespace sfg
{
#if !defined(SFG_EMBED_ASSETS)
	namespace
	{
		string_t cache_path(const char* dir, sid_t sid)
		{
			string_t p = dir;
			file_system_t::fix_path(p);
			file_system_t::fix_path_end_slash(p);
			p += std::to_string(sid);
			p += ".sfg_bin";
			return p;
		}

		bool save_cache(const char* cache_dir, sid_t sid, const ostream_t& cooked)
		{
			if (!file_system_t::ensure_directory(cache_dir))
			{
				SFG_ERR("failed to create directory {0}", cache_dir);
				return false;
			}

			const string_t path = cache_path(cache_dir, sid);
			if (!serializer_t::save_to_file(path.c_str(), cooked))
			{
				SFG_ERR("failed to write cache {0}", path.c_str());
				return false;
			}

			return true;
		}

		bool is_cached_file_ok(const char* cache_dir, sid_t sid, const resource_header_t& expected)
		{
			const string_t path = cache_path(cache_dir, sid);
			if (!file_system_t::exists(path.c_str()))
				return false;

			istream_t stream = serializer_t::load_from_file_slice(path.c_str(), 0, sizeof(resource_header_t));
			if (stream.empty())
				return false;

			resource_header_t header = {};
			header.deserialize(stream);
			if (header.magic != expected.magic || header.version != expected.version)
				return false;
			if (header.source_tick != expected.source_tick)
				return false;

			return true;
		}

		bool cook_for_schema(const string_t& schema, const nlohmann::json& config, const char* full_path, resource_header_t& out_header, ostream_t& stream)
		{
			if (schema == "sfg.schema.texture")
			{
				texture_cook_config_t cfg = {};
				if (!reflection_registry_t::get().type_from_json(type_id_t<texture_cook_config_t>::value, &cfg, nullptr, config))
				{
					SFG_ERR("failed to deserialize texture cook config");
					return false;
				}
				return texture_cooker::cook_from_file(cfg, full_path, out_header, stream);
			}
			if (schema == "sfg.schema.shader")
			{
				shader_cook_config_t cfg = {};
				if (!reflection_registry_t::get().type_from_json(type_id_t<shader_cook_config_t>::value, &cfg, nullptr, config))
				{
					SFG_ERR("failed to deserialize shader cook config");
					return false;
				}
				return shader_cooker::cook_from_file(cfg, full_path, out_header, stream);
			}
			if (schema == "sfg.schema.audio")
			{
				audio_cook_config_t cfg = {};
				if (!reflection_registry_t::get().type_from_json(type_id_t<audio_cook_config_t>::value, &cfg, nullptr, config))
				{
					SFG_ERR("failed to deserialize audio cook config");
					return false;
				}
				return audio_cooker::cook_from_file(cfg, full_path, out_header, stream);
			}
			if (schema == "sfg.schema.font")
			{
				font_cook_config_t cfg = {};
				if (!reflection_registry_t::get().type_from_json(type_id_t<font_cook_config_t>::value, &cfg, nullptr, config))
				{
					SFG_ERR("failed to deserialize font cook config");
					return false;
				}
				return font_cooker::cook_from_file(cfg, full_path, out_header, stream);
			}
			if (schema == "sfg.schema.material")
			{
				material_def_t def = {};
				if (!reflection_registry_t::get().type_from_json(type_id_t<material_def_t>::value, &def, nullptr, config))
				{
					SFG_ERR("failed to deserialize material definition");
					return false;
				}
				return material_cooker::cook_from_def(def, out_header, stream);
			}
			if (schema == "sfg.schema.animation")
			{
				animation_def_t def = {};
				if (!reflection_registry_t::get().type_from_json(type_id_t<animation_def_t>::value, &def, nullptr, config))
				{
					SFG_ERR("failed to deserialize animation definition");
					return false;
				}
				return animation_cooker::cook_from_def(def, out_header, stream);
			}
			if (schema == "sfg.schema.texture_sampler")
			{
				sampler_desc_t desc = {};
				if (!reflection_registry_t::get().type_from_json(type_id_t<sampler_desc_t>::value, &desc, nullptr, config))
				{
					SFG_ERR("failed to deserialize texture sampler description");
					return false;
				}
				return texture_sampler_cooker::cook_from_desc(desc, out_header, stream);
			}
			if (schema == "sfg.schema.hdr_skybox")
			{
				skybox_hdr_cook_config_t cfg = {};
				if (!reflection_registry_t::get().type_from_json(type_id_t<skybox_hdr_cook_config_t>::value, &cfg, nullptr, config))
				{
					SFG_ERR("failed to deserialize HDR skybox cook config");
					return false;
				}
				return skybox_hdr_cooker::cook_from_file(cfg, full_path, out_header, stream);
			}
			if (schema == "sfg.schema.animation_state_machine")
			{
				return animation_state_machine_cooker::cook_from_file(full_path, out_header, stream);
			}
			if (schema == "sfg.schema.prefab")
			{
				const string_t prefab_source = file_system_t::read_file_as_string(full_path);
				out_header					 = {
									  .magic	   = prefab_loader_t::WIRE_MAGIC,
									  .version	   = prefab_loader_t::WIRE_VERSION,
									  .source_tick = file_system_t::get_last_modified_ticks(full_path),
				  };
				stream << prefab_source;
				return true;
			}
			SFG_ERR("unknown cook schema: {0}", schema.c_str());
			return false;
		}

		bool cook_and_cache(const string_t& source_path, sid_t sid, const nlohmann::json& config, const string_t& cache_dir)
		{
			const string_t	  schema = config.value<string_t>("schema", "");
			ostream_t		  payload;
			resource_header_t out_header = {};
			if (!cook_for_schema(schema, config, source_path.c_str(), out_header, payload))
			{
				SFG_ERR("cook failed for {0}", source_path.c_str());
				return false;
			}

			out_header.set_debug_name_from_path(source_path.c_str());

			ostream_t cached_stream = out_header.make_stream(payload);
			if (!save_cache(cache_dir.c_str(), sid, cached_stream))
				SFG_WARN("cache save failed for {0}", sid);

			return true;
		}
	}
#endif

#if defined(SFG_EMBED_ASSETS)
	bool resource_pack_t::init(resource_manager_t& mgr, const init_params_t& params)
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
				SFG_ERR("invalid embedded entry at index {0}", i);
				continue;
			}

			const sid_t sid = TO_SID(e.path);
			const auto	st	= mgr.load_resource(sid, e.type);
			if (st == resource_state_e::failed)
			{
				SFG_ERR("load_resource failed for embedded {0}", e.path);
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
			SFG_ERR("failed to parse manifest at {0}", params.manifest_path.c_str());
			return false;
		}

		resource_manifest_t	 manifest  = {};
		const nlohmann::json resources = doc.value("resources", nlohmann::json::array());
		if (!resources.is_array())
		{
			SFG_ERR("invalid manifest resources at {0}", params.manifest_path.c_str());
			return false;
		}

		manifest.resources.reserve(resources.size());
		for (const nlohmann::json& item : resources)
		{
			resource_manifest_entry_t entry = {};
			entry.config					= nlohmann::json::object();
			if (!reflection_registry_t::get().type_from_json(type_id_t<resource_manifest_entry_t>::value, &entry, nullptr, item))
			{
				SFG_ERR("invalid manifest entry at {0}", params.manifest_path.c_str());
				return false;
			}
			manifest.resources.push_back(std::move(entry));
		}

		file_system_t::ensure_directory(_cache_dir.c_str());

		_loaded.reserve(manifest.resources.size());
		_watched.reserve(manifest.resources.size());
		_watcher.reserve(static_cast<int>(manifest.resources.size()));

		for (const resource_manifest_entry_t& entry : manifest.resources)
		{
			if (entry.type == resource_type_e::invalid)
			{
				SFG_ERR("invalid resource type for {0}", entry.path.c_str());
				return false;
			}

			const string_t source_path = params.assets_dir + entry.path;
			const sid_t	   sid		   = TO_SID(entry.path);

			if (!file_system_t::exists(source_path.c_str()))
			{
				SFG_ERR("resource source missing: {0}", source_path.c_str());
				return false;
			}

			const resource_type_desc_t* const desc	   = find_resource_type_desc(entry.type);
			resource_header_t				  expected = {
								.magic	 = desc != nullptr ? desc->wire_magic : 0,
								.version = desc != nullptr ? desc->wire_version : 0,
			};
			if (entry.type == resource_type_e::shader)
			{
				shader_cook_config_t cfg = {};
				if (!reflection_registry_t::get().type_from_json(type_id_t<shader_cook_config_t>::value, &cfg, nullptr, entry.config))
				{
					SFG_ERR("failed to deserialize shader cook config for {0}", entry.path.c_str());
					return false;
				}
				expected.source_tick = shader_cooker::collect_source_tick(cfg, source_path.c_str());
			}
			else if (entry.type == resource_type_e::material)
			{
				material_def_t def = {};
				if (!reflection_registry_t::get().type_from_json(type_id_t<material_def_t>::value, &def, nullptr, entry.config))
				{
					SFG_ERR("failed to deserialize material definition for {0}", entry.path.c_str());
					return false;
				}
				if (!material_cooker::collect_source_tick(def, expected.source_tick))
				{
					SFG_ERR("failed to collect material source tick for {0}", entry.path.c_str());
					return false;
				}
			}
			else if (entry.type == resource_type_e::animation)
			{
				animation_def_t def = {};
				if (!reflection_registry_t::get().type_from_json(type_id_t<animation_def_t>::value, &def, nullptr, entry.config))
				{
					SFG_ERR("failed to deserialize animation definition for {0}", entry.path.c_str());
					return false;
				}

				ostream_t def_stream;
				if (!reflection_registry_t::get().type_to_stream(type_id_t<animation_def_t>::value, &def, nullptr, def_stream))
				{
					SFG_ERR("failed to serialize animation definition for {0}", entry.path.c_str());
					return false;
				}

				expected.source_tick = hashing_t::hash_u64(def_stream.get_raw(), def_stream.get_size());
			}
			else if (entry.type == resource_type_e::texture_sampler)
			{
				sampler_desc_t desc = {};
				if (!reflection_registry_t::get().type_from_json(type_id_t<sampler_desc_t>::value, &desc, nullptr, entry.config))
				{
					SFG_ERR("failed to deserialize texture sampler description for {0}", entry.path.c_str());
					return false;
				}

				ostream_t desc_stream;
				if (!reflection_registry_t::get().type_to_stream(type_id_t<sampler_desc_t>::value, &desc, nullptr, desc_stream))
				{
					SFG_ERR("failed to serialize texture sampler description for {0}", entry.path.c_str());
					return false;
				}

				expected.source_tick = hashing_t::hash_u64(desc_stream.get_raw(), desc_stream.get_size());
			}
			else
				expected.source_tick = file_system_t::get_last_modified_ticks(source_path.c_str());

			const bool cache_ok = is_cached_file_ok(_cache_dir.c_str(), sid, expected);

			if (!cache_ok && !cook_and_cache(source_path, sid, entry.config, _cache_dir))
			{
				SFG_ERR("cook & cache failed for {0}", source_path.c_str());
				return false;
			}

			const auto st = mgr.load_resource(sid, entry.type, true);
			if (st == resource_state_e::failed)
			{
				SFG_ERR("load_resource failed for {0}", source_path.c_str());
				return false;
			}

			_loaded.push_back(sid);

			const u16 id = static_cast<u16>(_watched.size());
			_watched.push_back({.source_path = source_path, .config_json = entry.config.dump(), .type = entry.type, .sid = sid});
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
			SFG_ERR("failed to re-parse config for {0}", e.source_path.c_str());
			return;
		}

		if (!cook_and_cache(e.source_path, e.sid, config, _cache_dir))
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
