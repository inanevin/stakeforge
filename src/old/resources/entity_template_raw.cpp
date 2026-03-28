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

#include "entity_template_raw.hpp"
#include "entity_template.hpp"
#include "data/ostream_vector.hpp"
#include "data/istream_vector.hpp"
#include "data/hash_map.hpp"
#include "math/vector2ui16.hpp"
#include "io/log.hpp"

#ifdef SFG_TOOLMODE
#include "reflection/reflection.hpp"
#include "io/file_system.hpp"
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "world/common_entity.hpp"
#include "world/component_manager.hpp"
#include "math/color.hpp"
#include "serialization/serialization.hpp"
#include <fstream>
#include <vendor/nhlohmann/json.hpp>
#include "resources/entity_template_utils.hpp"
using json = nlohmann::json;
#endif

namespace SFG
{
	void entity_template_entity_raw::serialize(ostream& stream) const
	{
		stream << name;
		stream << tag;
		stream << position;
		stream << rotation;
		stream << scale;
		stream << parent;
		stream << first_child;
		stream << next_sibling;
		stream << visible;
		stream << template_reference;
	}

	void entity_template_entity_raw::deserialize(istream& stream)
	{
		stream >> name;
		stream >> tag;
		stream >> position;
		stream >> rotation;
		stream >> scale;
		stream >> parent;
		stream >> first_child;
		stream >> next_sibling;
		stream >> visible;
		stream >> template_reference;
	}

#ifdef SFG_TOOLMODE
	void to_json(nlohmann::json& j, const entity_template_entity_raw& r)
	{
		j["name"]		  = r.name;
		j["tag"]		  = r.tag;
		j["pos"]		  = r.position;
		j["rot"]		  = r.rotation;
		j["scale"]		  = r.scale;
		j["visible"]	  = r.visible;
		j["template"]	  = r.template_reference;
		j["parent"]		  = r.parent;
		j["first_child"]  = r.first_child;
		j["next_sibling"] = r.next_sibling;
	}

	void from_json(const nlohmann::json& j, entity_template_entity_raw& r)
	{
		r.name				 = j.value<string>("name", "");
		r.tag				 = j.value<string>("tag", "");
		r.position			 = j.value<vector3>("pos", vector3::zero);
		r.rotation			 = j.value<quat>("rot", quat::identity);
		r.scale				 = j.value<vector3>("scale", vector3::one);
		r.visible			 = j.value<u8>("visible", 1);
		r.template_reference = j.value<string>("template", "");
		r.parent			 = j.value<i32>("parent", -1);
		r.first_child		 = j.value<i32>("first_child", -1);
		r.next_sibling		 = j.value<i32>("next_sibling", -1);
	}

#endif

	void entity_template_raw::serialize(ostream& stream) const
	{
		stream << entities;
		stream << resources;
		const u32 sz = static_cast<u32>(component_buffer.get_size());
		stream << sz;
		stream.write_raw(component_buffer.get_raw(), sz);
	}

	void entity_template_raw::deserialize(istream& stream)
	{
		stream >> entities;
		stream >> resources;
		u32 sz = 0;
		stream >> sz;
		if (sz > 0)
		{
			component_buffer.create(sz);
			stream.read_to_raw(component_buffer.get_raw(), sz);
			component_buffer.shrink(sz);
		}
	}

	void entity_template_raw::destroy()
	{
		if (component_buffer.get_size() != 0)
			component_buffer.destroy();

		entities.resize(0);
		resources.resize(0);
	}

#ifdef SFG_TOOLMODE

	void entity_template_raw::save_to_file(const char* path, world& w, const vector<world_handle>& handles)
	{
		json j = {};
		save_to_json(j, w, handles);

		std::ofstream file(path);
		if (file.is_open())
		{
			file << j.dump(4);
			file.close();
		}
	}

	void entity_template_raw::collect_entities(entity_manager& em, world_handle h, vector<world_handle>& out)
	{
		out.push_back(h);

		const resource_handle tmp = em.get_entity_template_ref(h);
		if (!tmp.is_null())
			return;

		const entity_family& f	   = em.get_entity_family(h);
		world_handle		 child = f.first_child;
		while (!child.is_null())
		{
			if (em.get_entity_flags(h).is_set(entity_flags::entity_flags_no_save))
				continue;

			collect_entities(em, child, out);
			const entity_family& cf = em.get_entity_family(child);
			child					= cf.next_sibling;
		}
	}

	void entity_template_raw::save_to_json(nlohmann::json& j, world& w, const vector<world_handle>& handles)
	{
		entity_manager&	   em = w.get_entity_manager();
		component_manager& cm = w.get_comp_manager();
		resource_manager&  rm = w.get_resource_manager();

		vector<world_handle> order;
		order.reserve(512);

		for (world_handle h : handles)
		{
			if (em.get_entity_flags(h).is_set(entity_flags::entity_flags_no_save))
				continue;
			collect_entities(em, h, order);
		}

		hash_map<u32, i32> index_by_world;
		index_by_world.reserve(order.size());
		for (size_t i = 0; i < order.size(); ++i)
			index_by_world[order[i].index] = static_cast<i32>(i);

		j["entities"]	= json::array();
		j["components"] = json::array();
		j["resources"]	= json::array();

		json& je = j["entities"];
		json& jc = j["components"];

		vector<string> resource_paths = {};

		for (size_t i = 0; i < order.size(); i++)
		{
			const world_handle h = order[i];

			entity_template_entity_raw er = entity_template_utils::entity_to_entity_template_entity_raw(h, em, rm, index_by_world);
			je.push_back(er);

			if (!er.template_reference.empty())
			{
				const resource_handle handle = rm.get_resource_handle_by_hash<entity_template>(TO_SID(er.template_reference));
				entity_template&	  et	 = rm.get_resource<entity_template>(handle);

				auto it = std::find_if(resource_paths.begin(), resource_paths.end(), [&](const string& str) -> bool { return str.compare(er.template_reference) == 0; });
				if (it == resource_paths.end())
					resource_paths.push_back(er.template_reference);

				// const entity_template_raw& et_raw = et.get_raw();
				// for (const string& r : et_raw.resources)
				// {
				// 	auto it2 = std::find_if(resource_paths.begin(), resource_paths.end(), [&](const string& str) -> bool { return str.compare(r) == 0; });
				// 	if (it2 == resource_paths.end())
				// 		resource_paths.push_back(r);
				// }
				continue;
			}

			entity_template_utils::append_entity_components_as_json(jc, h, static_cast<u32>(i), em, cm, rm, index_by_world, resource_paths);
		}

		json& jr = j["resources"];
		for (const string& p : resource_paths)
		{
			jr.push_back(p);
		}
	}

	void entity_template_raw::load_from_json(const nlohmann::json& json_data, entity_template_raw& r)
	{
		r.destroy();
		r.entities	= json_data.value<vector<entity_template_entity_raw>>("entities", {});
		r.resources = json_data.value<vector<string>>("resources", {});

		const json& comps = json_data["components"];
		if (comps.is_array())
		{
			for (const auto& c : comps)
			{
				entity_template_utils::component_json_to_component_buffer(c, r.component_buffer);
			}
		}
	}

	bool entity_template_raw::load_from_file(const char* relative_file, const char* base_path)
	{
		const string target_path = base_path + string(relative_file);
		if (!file_system::exists(target_path.c_str()))
		{
			SFG_ERR("file doesn't exists: {0}", target_path.c_str());
			return false;
		}

		try
		{
			std::ifstream f(target_path);
			json		  json_data = json::parse(f);
			f.close();

			name = relative_file;
			load_from_json(json_data, *this);
		}
		catch (std::exception e)
		{
			SFG_ERR("failed loading: {0}", e.what());
			return false;
		}

		return true;
	}

	bool entity_template_raw::load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension)
	{
		const string sid_str		 = std::to_string(TO_SID(relative_path));
		const string relative		 = file_system::get_filename_from_path(relative_path);
		const string meta_cache_path = cache_folder_path + relative + "-" + sid_str + "_meta" + extension;
		const string data_cache_path = cache_folder_path + relative + "-" + sid_str + "_data" + extension;

		if (!file_system::exists(meta_cache_path.c_str()))
			return false;

		if (!file_system::exists(data_cache_path.c_str()))
			return false;

		istream stream = serialization::load_from_file(meta_cache_path.c_str());

		string file_path				= "";
		u64	   saved_file_last_modified = 0;
		stream >> file_path;
		stream >> saved_file_last_modified;

		stream.destroy();

		const u64 file_last_modified = file_system::get_last_modified_ticks(file_path);

		if (file_last_modified != saved_file_last_modified)
			return false;

		stream = serialization::load_from_file(data_cache_path.c_str());
		deserialize(stream);
		stream.destroy();
		return true;
	}

	void entity_template_raw::save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const
	{
		const string sid_str			= std::to_string(TO_SID(name));
		const string relative			= file_system::get_filename_from_path(name);
		const string file_path			= resource_directory_path + name;
		const u64	 file_last_modified = file_system::get_last_modified_ticks(file_path);

		const string meta_cache_path = cache_folder_path + relative + "-" + sid_str + "_meta" + extension;
		const string data_cache_path = cache_folder_path + relative + "-" + sid_str + "_data" + extension;

		ostream out_stream;
		out_stream << file_path;
		out_stream << file_last_modified;
		serialization::save_to_file(meta_cache_path.c_str(), out_stream);

		out_stream.shrink(0);
		serialize(out_stream);
		serialization::save_to_file(data_cache_path.c_str(), out_stream);

		out_stream.destroy();
	}

#endif

	void entity_template_raw::get_sub_resources(vector<string>& out_res) const
	{
		for (const string& res : resources)
			out_res.push_back(res);
	}

}
