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

#include "app/package_manager.hpp"
#include "io/file_system.hpp"

#ifdef SFG_TOOLMODE
#include "app/engine_resources.hpp"
#include "editor/editor_settings.hpp"
#include "io/file_system.hpp"
#include "io/log.hpp"
#include "reflection/reflection.hpp"
#include "resources/world_raw.hpp"
#include "resources/shader.hpp"
#include "resources/material.hpp"
#include "resources/texture.hpp"
#include "resources/texture_sampler.hpp"
#include "resources/audio.hpp"
#include "resources/font.hpp"
#include "resources/model.hpp"
#include "resources/physical_material.hpp"
#include "resources/particle_properties.hpp"
#include "resources/res_state_machine.hpp"
#include "resources/entity_template.hpp"
#include "resources/shader_raw.hpp"
#include "resources/material_raw.hpp"
#include "resources/texture_raw.hpp"
#include "resources/texture_sampler_raw.hpp"
#include "resources/audio_raw.hpp"
#include "resources/font_raw.hpp"
#include "resources/model_raw.hpp"
#include "resources/physical_material_raw.hpp"
#include "resources/particle_properties_raw.hpp"
#include "resources/res_state_machine_raw.hpp"
#include "resources/entity_template_raw.hpp"
#include "data/hash_map.hpp"
#include "data/string.hpp"
#endif

namespace SFG
{
#ifdef SFG_TOOLMODE
	namespace
	{
		template <typename T> bool load_raw(T& raw, const char* path, const char* base_dir, const char* cache_dir)
		{
			if (cache_dir && cache_dir[0] != '\0')
			{
				if (raw.load_from_cache(cache_dir, path, ".stkcache"))
					return true;
			}

			if (!raw.load_from_file(path, base_dir))
				return false;

			if (cache_dir && cache_dir[0] != '\0')
				raw.save_to_cache(cache_dir, base_dir, ".stkcache");

			return true;
		}

		void append_sub_resources(vector<string>& pending, const vector<string>& subs)
		{
			for (const string& res : subs)
			{
				if (!res.empty())
					pending.push_back(res);
			}
		}

		void package_resources(const vector<string>& relative_paths, package& pkg, const char* base_dir, const char* cache_dir)
		{
			vector<string>			pending = relative_paths;
			hash_map<string_id, u8> packed;

			vector<string> sub_resources;
			sub_resources.reserve(16);

			size_t index = 0;
			while (index < pending.size())
			{
				const string path = pending[index++];
				if (path.empty())
					continue;

				const string_id sid = TO_SID(path);
				if (packed.find(sid) != packed.end())
					continue;

				packed[sid] = 1;

				const string ext = file_system::get_file_extension(path);
				if (ext.empty())
				{
					SFG_ERR("could not deduce extension: {0}", path.c_str());
					continue;
				}

				const meta* m = reflection::get().find_by_tag(ext.c_str());
				if (m == nullptr)
				{
					SFG_ERR("no metadata found associated with this tag: {0}", ext.c_str());
					continue;
				}

				const string_id type = m->get_type_id();

				sub_resources.resize(0);

				if (type == type_id<shader>::value)
				{
					shader_raw raw = {};
					if (!load_raw(raw, path.c_str(), base_dir, cache_dir))
						continue;
					pkg.write_resource(path.c_str());
					raw.serialize(pkg.get_write_stream());
					raw.destroy();
					raw.get_sub_resources(sub_resources);
				}
				else if (type == type_id<material>::value)
				{
					material_raw raw = {};
					if (!load_raw(raw, path.c_str(), base_dir, cache_dir))
						continue;
					pkg.write_resource(path.c_str());
					raw.serialize(pkg.get_write_stream());
					raw.get_sub_resources(sub_resources);
					raw.destroy();
				}
				else if (type == type_id<texture>::value)
				{
					texture_raw raw = {};
					if (!load_raw(raw, path.c_str(), base_dir, cache_dir))
						continue;
					pkg.write_resource(path.c_str());
					raw.serialize(pkg.get_write_stream());
					raw.get_sub_resources(sub_resources);
				}
				else if (type == type_id<texture_sampler>::value)
				{
					texture_sampler_raw raw = {};
					if (!load_raw(raw, path.c_str(), base_dir, cache_dir))
						continue;
					pkg.write_resource(path.c_str());
					raw.serialize(pkg.get_write_stream());
					raw.get_sub_resources(sub_resources);
				}
				else if (type == type_id<audio>::value)
				{
					audio_raw raw = {};
					if (!load_raw(raw, path.c_str(), base_dir, cache_dir))
						continue;
					pkg.write_resource(path.c_str());
					raw.serialize(pkg.get_write_stream());
					raw.get_sub_resources(sub_resources);
				}
				else if (type == type_id<font>::value)
				{
					font_raw raw = {};
					if (!load_raw(raw, path.c_str(), base_dir, cache_dir))
						continue;
					pkg.write_resource(path.c_str());
					raw.serialize(pkg.get_write_stream());
					raw.get_sub_resources(sub_resources);
				}
				else if (type == type_id<model>::value)
				{
					model_raw raw = {};
					if (!load_raw(raw, path.c_str(), base_dir, cache_dir))
						continue;
					pkg.write_resource(path.c_str());
					raw.serialize(pkg.get_write_stream());
					raw.get_sub_resources(sub_resources);

					for (material_raw& m : raw.loaded_materials)
						m.destroy();
				}
				else if (type == type_id<physical_material>::value)
				{
					physical_material_raw raw = {};
					if (!load_raw(raw, path.c_str(), base_dir, cache_dir))
						continue;
					pkg.write_resource(path.c_str());
					raw.serialize(pkg.get_write_stream());
					raw.get_sub_resources(sub_resources);
				}
				else if (type == type_id<particle_properties>::value)
				{
					particle_properties_raw raw = {};
					if (!load_raw(raw, path.c_str(), base_dir, cache_dir))
						continue;
					pkg.write_resource(path.c_str());
					raw.serialize(pkg.get_write_stream());
					raw.get_sub_resources(sub_resources);
				}
				else if (type == type_id<res_state_machine>::value)
				{
					res_state_machine_raw raw = {};
					if (!load_raw(raw, path.c_str(), base_dir, cache_dir))
						continue;
					pkg.write_resource(path.c_str());
					raw.serialize(pkg.get_write_stream());
					raw.get_sub_resources(sub_resources);
				}
				else if (type == type_id<entity_template>::value)
				{
					entity_template_raw raw = {};
					if (!load_raw(raw, path.c_str(), base_dir, cache_dir))
						continue;
					pkg.write_resource(path.c_str());
					raw.serialize(pkg.get_write_stream());
					raw.get_sub_resources(sub_resources);
					raw.destroy();
				}
				else
				{
					SFG_ERR("unknown resource type: {0}", path.c_str());
					continue;
				}

				append_sub_resources(pending, sub_resources);
			}
		}
	}
#endif

	bool package_manager::init()
	{
#ifndef SFG_TOOLMODE
		if (!file_system::exists(WORLD_PKG_PATH) || !file_system::exists(RES_PKG_PATH) || !file_system::exists(ENGINE_PKG_PATH))
			return false;

			// TODO: checksum
#endif

		return true;
	}

	void package_manager::uninit()
	{
	}

#ifdef SFG_TOOLMODE

	void package_manager::package_project(const vector<string>& levels, const char* output_directory)
	{
		const string working_dir	  = editor_settings::get().working_dir;
		const string cache_dir		  = editor_settings::get().cache_dir;
		const string engine_cache_dir = editor_settings::get()._resource_cache;

		string out_dir = output_directory == nullptr ? "" : output_directory;
		file_system::fix_path(out_dir);
		if (!out_dir.empty() && out_dir.back() != '/')
			out_dir += "/";

		if (!out_dir.empty() && !file_system::exists(out_dir.c_str()))
			file_system::create_directory(out_dir.c_str());

		package world_pkg  = {};
		package res_pkg	   = {};
		package engine_pkg = {};

		world_pkg.start_writing();

		vector<string> resource_paths;

		for (const string& level : levels)
		{
			if (level.empty())
				continue;

			string fixed_path = level;
			file_system::fix_path(fixed_path);

			world_raw raw = {};
			if (!raw.load_from_file(fixed_path.c_str(), working_dir.c_str()))
			{
				SFG_ERR("failed loading world: {0}", fixed_path.c_str());
				continue;
			}

			world_pkg.write_resource(fixed_path.c_str());
			raw.serialize(world_pkg.get_write_stream());

			for (const string& res : raw.entities_raw.resources)
			{
				if (!res.empty())
					resource_paths.push_back(res);
			}

			raw.destroy();
		}

		const string world_path = out_dir + WORLD_PKG_PATH;
		world_pkg.close_writing(world_path.c_str());

		res_pkg.start_writing();
		package_resources(resource_paths, res_pkg, working_dir.c_str(), cache_dir.c_str());
		const string res_path = out_dir + RES_PKG_PATH;
		res_pkg.close_writing(res_path.c_str());

		vector<string> engine_paths;
		engine_paths.reserve(64);

		const auto& defs = engine_resources::get().get_resources();
		for (const engine_resource_def& def : defs)
		{
			if (def.path && def.path[0] != '\0')
				engine_paths.push_back(def.path);
		}

		engine_pkg.start_writing();
		package_resources(engine_paths, engine_pkg, SFG_ROOT_DIRECTORY, engine_cache_dir.c_str());

		const string engine_path = out_dir + ENGINE_PKG_PATH;
		engine_pkg.close_writing(engine_path.c_str());
	}

#endif

	package& package_manager::open_package_engine_data()
	{
		_pk_engine_data.open(ENGINE_PKG_PATH);
		return _pk_engine_data;
	}

	package& package_manager::open_package_world()
	{
		_pk_world_data.open(WORLD_PKG_PATH);
		return _pk_world_data;
	}

	package& package_manager::open_package_res()
	{
		_pk_res_data.open(RES_PKG_PATH);
		return _pk_res_data;
	}
}
