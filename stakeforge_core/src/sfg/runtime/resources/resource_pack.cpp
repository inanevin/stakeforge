// Copyright (c) 2025 Inan Evin

#include "resource_pack.hpp"
#include "resource_cache.hpp"
#include "resource_cooker.hpp"
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

		resource_type_e resolve_resource_type(const string_t& s)
		{
			if (s == "audio")
				return resource_type_e::audio;
			if (s == "font")
				return resource_type_e::font;
			if (s == "mesh")
				return resource_type_e::mesh;
			if (s == "skeleton")
				return resource_type_e::skeleton;
			if (s == "animation")
				return resource_type_e::animation;
			if (s == "particle_properties")
				return resource_type_e::particle_properties;
			if (s == "material")
				return resource_type_e::material;
			if (s == "shader")
				return resource_type_e::shader;
			if (s == "texture")
				return resource_type_e::texture;
			if (s == "texture_sampler")
				return resource_type_e::texture_sampler;
			if (s == "physical_material")
				return resource_type_e::physical_material;
			if (s == "prefab")
				return resource_type_e::prefab;
			if (s == "animation_state_machine")
				return resource_type_e::animation_state_machine;
			return resource_type_e::invalid;
		}

		string_t options_to_arguments(const json& options)
		{
			if (options.is_string())
				return options.get<string_t>();

			if (!options.is_object())
				return string_t{};

			string_t result;
			bool	 first = true;
			for (auto it = options.begin(); it != options.end(); ++it)
			{
				if (!first)
					result += ",";
				first = false;
				result += it.key();
				result += "=";
				if (it->is_string())
					result += it->get<string_t>();
				else if (it->is_boolean())
					result += it->get<bool>() ? "true" : "false";
				else if (it->is_number_integer())
					result += std::to_string(it->get<i64>());
				else if (it->is_number())
					result += std::to_string(it->get<double>());
			}
			return result;
		}

		bool load_one_entry(resource_manager_t& mgr, const json& entry, const string_t& assets_dir, const string_t& cache_dir, sid_t& out_sid)
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

			const auto		  opts_it = entry.find("options");
			cooking_options_t options;
			if (opts_it != entry.end())
				options.arguments = options_to_arguments(*opts_it);

			const string_t source_path = assets_dir + path;
			if (!file_system::exists(source_path.c_str()))
			{
				SFG_ERR("resource_pack: source missing: {0}", source_path.c_str());
				return false;
			}

			const sid_t sid = TO_SID(path);

			vector_t<u8> bytes;
			const bool	 fresh = resource_cache::try_load_fresh(cache_dir.c_str(), name.c_str(), source_path.c_str(), bytes);

			if (!fresh)
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

				bytes.resize(stream.get_size());
				SFG_MEMCPY(bytes.data(), stream.get_raw(), stream.get_size());
			}

			const span_t<u8> data = {bytes.data(), bytes.size()};
			const auto		 st	  = mgr.load_resource(sid, data, rtype);
			if (st == resource_state_e::failed)
			{
				SFG_ERR("resource_pack: load_resource failed for {0}", path.c_str());
				return false;
			}

			out_sid = sid;
			return true;
		}

		bool load_from_manifest(resource_manager_t& mgr, const resource_pack_t::init_params_t& params, vector_t<sid_t>& out_loaded)
		{
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

			resource_cache::ensure_directory(params.cache_dir.c_str());

			out_loaded.reserve(resources_it->size());
			for (const auto& entry : *resources_it)
			{
				sid_t	   sid = 0;
				const bool ok  = load_one_entry(mgr, entry, params.assets_dir, params.cache_dir, sid);
				if (ok)
					out_loaded.push_back(sid);
			}
			return true;
		}

		bool load_embedded(resource_manager_t& mgr, const resource_pack_t::init_params_t& params, vector_t<sid_t>& out_loaded)
		{
			if (params.embedded_entries == nullptr || params.embedded_entry_count == 0)
			{
				SFG_WARN("resource_pack: embed mode but no embedded entries provided");
				return true;
			}

			out_loaded.reserve(params.embedded_entry_count);
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
				out_loaded.push_back(sid);
			}
			return true;
		}
	}

	bool resource_pack_t::init(resource_manager_t& mgr, const init_params_t& params)
	{
		_mgr = &mgr;
#if defined(SFG_EMBED_ASSETS)
		return load_embedded(mgr, params, _loaded);
#else
		return load_from_manifest(mgr, params, _loaded);
#endif
	}

	void resource_pack_t::uninit()
	{
		if (_mgr != nullptr)
		{
			for (sid_t sid : _loaded)
				_mgr->unload_resource(sid);
		}
		_loaded.clear();
		_mgr = nullptr;
	}
}
