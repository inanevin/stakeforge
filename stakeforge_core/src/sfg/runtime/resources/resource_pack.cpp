// Copyright (c) 2025 Inan Evin

#include "resource_pack.hpp"
#include "manifest_util.hpp"
#include "resource_cache.hpp"
#include "resource_manager.hpp"

#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		using json = nlohmann::json;

#if !defined(SFG_EMBED_ASSETS)
		struct parsed_entry_t
		{
			string_t		  source_path;
			string_t		  name;
			cooking_options_t options;
			resource_type_e	  type = resource_type_e::invalid;
			sid_t			  sid  = 0;
		};

		bool parse_entry(const json& entry, const string_t& assets_dir, parsed_entry_t& out)
		{
			const auto name_it = entry.find("name");
			const auto path_it = entry.find("path");
			const auto type_it = entry.find("type");

			if (path_it == entry.end() || !path_it->is_string())
			{
				SFG_ERR("resource_pack: manifest entry missing 'path'");
				return false;
			}
			if (type_it == entry.end() || !type_it->is_string())
			{
				SFG_ERR("resource_pack: manifest entry missing 'type'");
				return false;
			}

			const string_t path = path_it->get<string_t>();
			const string_t type = type_it->get<string_t>();
			const string_t name = (name_it != entry.end() && name_it->is_string()) ? name_it->get<string_t>() : path;

			const resource_type_e rtype = resolve_resource_type(type);
			if (rtype == resource_type_e::invalid)
			{
				SFG_ERR("resource_pack: unknown type '{0}' for {1}", type.c_str(), path.c_str());
				return false;
			}

			const auto opts_it = entry.find("options");
			if (opts_it != entry.end())
				out.options.arguments = manifest_util::options_to_arguments(*opts_it);

			out.source_path = assets_dir + path;
			out.name		= name;
			out.type		= rtype;
			out.sid			= TO_SID(path);
			return true;
		}

		bool cook_and_cache(const string_t& source_path, const string_t& name, const cooking_options_t& options, const string_t& cache_dir, vector_t<u8>& out_bytes)
		{
			ostream_t			stream;
			const cook_result_e r = cook_resource(source_path.c_str(), options, stream);
			if (r != cook_result_e::success)
			{
				SFG_ERR("resource_pack: cook failed for {0}", source_path.c_str());
				return false;
			}

			if (!resource_cache::save(cache_dir.c_str(), name.c_str(), source_path.c_str(), stream))
				SFG_WARN("resource_pack: cache save failed for {0}", name.c_str());

			out_bytes.resize(stream.get_size());
			SFG_MEMCPY(out_bytes.data(), stream.get_raw(), stream.get_size());
			return true;
		}
#endif
	}

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

		_mgr->wait_for_all();
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

		if (!file_system::exists(params.manifest_path.c_str()))
		{
			SFG_WARN("resource_pack: no manifest at {0}", params.manifest_path.c_str());
			return true;
		}

		const string_t json_text = file_system::read_file_as_string(params.manifest_path.c_str());

		json manifest;
		try
		{
			manifest = json::parse(json_text);
		}
		catch (const json::parse_error& e)
		{
			SFG_ERR("resource_pack: parse error: {0}", e.what());
			return false;
		}

		const auto resources_it = manifest.find("resources");
		if (resources_it == manifest.end() || !resources_it->is_array())
		{
			SFG_WARN("resource_pack: 'resources' array missing in {0}", params.manifest_path.c_str());
			return true;
		}

		resource_cache::ensure_directory(_cache_dir.c_str());

		_loaded.reserve(resources_it->size());
		_watched.reserve(resources_it->size());
		_watcher.reserve(static_cast<int>(resources_it->size()));

		for (const auto& entry : *resources_it)
		{
			parsed_entry_t pe;
			if (!parse_entry(entry, params.assets_dir, pe))
				continue;

			if (!file_system::exists(pe.source_path.c_str()))
			{
				SFG_ERR("resource_pack: source missing: {0}", pe.source_path.c_str());
				continue;
			}

			vector_t<u8> bytes;
			const bool	 fresh = resource_cache::try_load_fresh(_cache_dir.c_str(), pe.name.c_str(), pe.source_path.c_str(), bytes);
			if (!fresh && !cook_and_cache(pe.source_path, pe.name, pe.options, _cache_dir, bytes))
				continue;

			const span_t<u8> data = {bytes.data(), bytes.size()};
			const auto		 st	  = mgr.load_resource(pe.sid, data, pe.type);
			if (st == resource_state_e::failed)
			{
				SFG_ERR("resource_pack: load_resource failed for {0}", pe.source_path.c_str());
				continue;
			}

			_loaded.push_back(pe.sid);

			const u16 id = static_cast<u16>(_watched.size());
			_watched.push_back({.source_path = pe.source_path, .name = pe.name, .options = pe.options, .type = pe.type, .sid = pe.sid});
			_watcher.add_path(pe.source_path.c_str(), id);
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

		vector_t<u8> bytes;
		if (!cook_and_cache(e.source_path, e.name, e.options, _cache_dir, bytes))
			return;

		SFG_INFO("resource_pack: recooked {0}", e.source_path.c_str());

		// TODO:
		// const span_t<u8> data = {bytes.data(), bytes.size()};
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
