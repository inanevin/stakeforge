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

#include "world_raw.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

#ifdef SFG_TOOLMODE
#include "io/file_system.hpp"
#include "io/log.hpp"
#include "editor/editor_settings.hpp"
#include "gui/vekt.hpp"
#include "world/world.hpp"
#include "world/entity_manager.hpp"
#include "world/component_manager.hpp"
#include "resources/entity_template_raw.hpp"
#include "resources/entity_template_utils.hpp"
#include <fstream>
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{
	world_raw::~world_raw()
	{
		SFG_ASSERT(entities_raw.component_buffer.get_size() == 0);
	}

	void world_raw::serialize(ostream& stream) const
	{
		entities_raw.serialize(stream);
	}

	void world_raw::deserialize(istream& stream)
	{
		entities_raw.deserialize(stream);
	}

	void world_raw::destroy()
	{
		entities_raw.destroy();
	}

#ifdef SFG_TOOLMODE

	bool world_raw::load_from_file(const char* relative_file, const char* base_path)
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

			destroy();

			tool_cam_pos = json_data.value<vector3>("tool_cam_pos", vector3::zero);
			tool_cam_rot = json_data.value<quat>("tool_cam_rot", quat::identity);

			entity_template_raw::load_from_json(json_data, entities_raw);

			f.close();
		}
		catch (std::exception e)
		{
			SFG_ERR("Failed loading: {0}", e.what());
			return false;
		}
		return true;
	}

	bool world_raw::save_to_file(const char* path, world& w)
	{
		entity_manager& em		 = w.get_entity_manager();
		const auto&		entities = em.get_entities();

		vector<world_handle> to_serialize;

		for (auto it = entities->handles_begin(); it != entities->handles_end(); ++it)
		{
			const world_handle	 handle = *it;
			const entity_family& f		= em.get_entity_family(handle);
			if (!f.parent.is_null())
				continue;

			to_serialize.push_back(handle);
		}

		json j			  = {};
		j["tool_cam_pos"] = w.get_tool_camera_pos();
		j["tool_cam_rot"] = w.get_tool_camera_rot();

		entity_template_raw::save_to_json(j, w, to_serialize);

		std::ofstream file(path);
		if (file.is_open())
		{
			file << j.dump(4);
			file.close();
			return true;
		}
		return false;
	}

	void world_raw::fill_from_world(world& w)
	{
		entity_manager&	   em = w.get_entity_manager();
		component_manager& cm = w.get_comp_manager();
		resource_manager&  rm = w.get_resource_manager();

		destroy();

		// top-level entities
		vector<world_handle> roots;
		{
			const auto& entities = em.get_entities();
			for (auto it = entities->handles_begin(); it != entities->handles_end(); ++it)
			{
				const world_handle	 h			  = *it;
				const entity_family& f			  = em.get_entity_family(h);
				const bool			 is_transient = em.get_entity_flags(h).is_set(entity_flags::entity_flags_no_save);

				if (f.parent.is_null() && !is_transient)
					roots.push_back(h);
			}
		}

		vector<world_handle> order;
		order.reserve(512);
		for (world_handle h : roots)
			entity_template_raw::collect_entities(em, h, order);

		hash_map<u32, i32> index_by_world;
		index_by_world.reserve(order.size());
		for (size_t i = 0; i < order.size(); ++i)
			index_by_world[order[i].index] = static_cast<i32>(i);

		for (size_t i = 0; i < order.size(); ++i)
		{
			const world_handle				 h	= order[i];
			const entity_template_entity_raw er = entity_template_utils::entity_to_entity_template_entity_raw(h, em, rm, index_by_world);
			entities_raw.entities.push_back(er);

			if (er.template_reference.empty())
				entity_template_utils::entity_components_to_component_buffer(h, static_cast<u32>(i), em, cm, rm, index_by_world, entities_raw.component_buffer);
		}
	}

#endif
}
