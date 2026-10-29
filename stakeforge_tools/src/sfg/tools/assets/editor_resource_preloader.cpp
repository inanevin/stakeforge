/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

*/

#include "tools/assets/editor_resource_preloader.hpp"
#include <sfg/core/runtime/resources/animation_cook.hpp>
#include <sfg/core/runtime/resources/animation_def.hpp>
#include <sfg/core/runtime/resources/audio_cook.hpp>
#include <sfg/core/runtime/resources/common_resources.hpp>
#include <sfg/core/runtime/resources/cubemap_cook.hpp>
#include <sfg/core/runtime/resources/curve_cook.hpp>
#include <sfg/core/runtime/resources/curve_def.hpp>
#include <sfg/core/runtime/resources/font_cook.hpp>
#include <sfg/core/runtime/resources/material_cook.hpp>
#include <sfg/core/runtime/resources/material_def.hpp>
#include <sfg/core/runtime/resources/ragdoll_cook.hpp>
#include <sfg/core/runtime/resources/ragdoll_def.hpp>
#include <sfg/core/runtime/resources/prefab.hpp>
#include <sfg/core/runtime/resources/resource_manager.hpp>
#include <sfg/core/runtime/resources/resource_manifest.hpp>
#include <sfg/core/runtime/resources/shader_cook.hpp>
#include <sfg/core/runtime/resources/texture_cook.hpp>
#include <sfg/core/runtime/resources/texture_sampler_cook.hpp>

#include <sfg/core/common/hashing.hpp>
#include <sfg/core/data/istream.hpp>
#include <sfg/core/data/ostream.hpp>
#include <sfg/core/gfx/common/descriptions.hpp>
#include <sfg/core/io/file_system.hpp>
#include <sfg/core/io/log.hpp>
#include <sfg/core/reflection/reflection_registry.hpp>
#include <sfg/core/serialization/serialization.hpp>
#include <sfg/core/vendor/nhlohmann/json.hpp>

namespace sfg
{
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

			if (!serializer_t::save_to_file_atomic(path.c_str(), cooked))
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

			if (header.type != expected.type)
				return false;

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

				shader_data_definition_t definition = {};

				return shader_cooker::cook_from_file(cfg, full_path, out_header, stream, definition);
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

				config.get_to(def);

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

			if (schema == "sfg.schema.curve")
			{
				curve_def_t def = {};

				if (!reflection_registry_t::get().type_from_json(type_id_t<curve_def_t>::value, &def, nullptr, config))
				{
					SFG_ERR("failed to deserialize curve definition");

					return false;
				}

				return curve_cooker::cook_from_def(def, out_header, stream);
			}

			if (schema == "sfg.schema.ragdoll")
			{
				ragdoll_def_t def = {};

				if (!reflection_registry_t::get().type_from_json(type_id_t<ragdoll_def_t>::value, &def, nullptr, config))
				{
					SFG_ERR("failed to deserialize ragdoll definition");
					return false;
				}

				return ragdoll_cooker::cook_from_def(def, out_header, stream);
			}

			if (schema == "sfg.schema.cubemap")
			{
				cubemap_cook_config_t cfg = {};

				if (!reflection_registry_t::get().type_from_json(type_id_t<cubemap_cook_config_t>::value, &cfg, nullptr, config))
				{
					SFG_ERR("failed to deserialize cubemap cook config");

					return false;
				}

				return cubemap_cooker::cook_from_file(cfg, full_path, out_header, stream);
			}

			if (schema == "sfg.schema.prefab")
			{
				const string_t prefab_source = file_system_t::read_file_as_string(full_path);

				if (prefab_source.empty())
				{
					SFG_ERR("prefab has no source: {0}", full_path);
					return false;
				}

				out_header = {
					.type		 = resource_type_e::prefab,
					.magic		 = prefab_loader_t::WIRE_MAGIC,
					.version	 = prefab_loader_t::WIRE_VERSION,
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
			const string_t	  schema	 = config.value<string_t>("schema", "");
			ostream_t		  payload	 = {};
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

	bool editor_resource_preloader_t::init(resource_manager_t& mgr, const init_params_t& params)
	{
		_mgr = &mgr;

		if (!file_system_t::exists(params.manifest_path.c_str()))
		{
			SFG_WARN("no manifest at {0}", params.manifest_path.c_str());
			return true;
		}

		resource_manifest_t manifest = {};

		if (!manifest.load_from_file(params.manifest_path.c_str()))
			return false;

		file_system_t::ensure_directory(params.cache_dir.c_str());

		_loaded.reserve(manifest.resources.size());

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
				.type	 = entry.type,
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

				entry.config.get_to(def);

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

				ostream_t def_stream = {};

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

				ostream_t desc_stream = {};

				if (!reflection_registry_t::get().type_to_stream(type_id_t<sampler_desc_t>::value, &desc, nullptr, desc_stream))
				{
					SFG_ERR("failed to serialize texture sampler description for {0}", entry.path.c_str());
					return false;
				}

				expected.source_tick = hashing_t::hash_u64(desc_stream.get_raw(), desc_stream.get_size());
			}
			else if (entry.type == resource_type_e::curve)
			{
				curve_def_t def = {};

				if (!reflection_registry_t::get().type_from_json(type_id_t<curve_def_t>::value, &def, nullptr, entry.config))
				{
					SFG_ERR("failed to deserialize curve definition for {0}", entry.path.c_str());
					return false;
				}

				ostream_t def_stream = {};

				if (!reflection_registry_t::get().type_to_stream(type_id_t<curve_def_t>::value, &def, nullptr, def_stream))
				{
					SFG_ERR("failed to serialize curve definition for {0}", entry.path.c_str());
					return false;
				}

				expected.source_tick = hashing_t::hash_u64(def_stream.get_raw(), def_stream.get_size());
			}
			else
				expected.source_tick = file_system_t::get_last_modified_ticks(source_path.c_str());

			const bool cache_ok = is_cached_file_ok(params.cache_dir.c_str(), sid, expected);

			if (!cache_ok && !cook_and_cache(source_path, sid, entry.config, params.cache_dir))
			{
				SFG_ERR("cook & cache failed for {0}", source_path.c_str());
				return false;
			}

			const auto st = mgr.load_resource(sid, entry.type);

			if (st == resource_state_e::failed)
			{
				SFG_ERR("load_resource failed for {0}", source_path.c_str());
				return false;
			}

			_loaded.push_back(sid);
		}

		return true;
	}

	void editor_resource_preloader_t::uninit()
	{
		if (_mgr != nullptr)
		{
			for (sid_t sid : _loaded)
				_mgr->unload_resource(sid);
		}

		_loaded.resize(0);
		_mgr = nullptr;
	}
}
