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

#pragma once

#include "common/string_id.hpp"
#include "data/vector.hpp"
#include "data/string.hpp"
#include "math/vector3.hpp"
#include "math/quat.hpp"
#include "data/ostream.hpp"
#include "world/world_constants.hpp"
#include "resources/common_resources.hpp"

namespace SFG
{
	class ostream;
	class istream;
	class world;

	struct entity_template_entity_raw
	{
		string	name			   = "";
		string	tag				   = "";
		string	template_reference = "";
		quat	rotation		   = quat::identity;
		vector3 position		   = vector3::zero;
		vector3 scale			   = vector3::one;
		i32		parent			   = -1;
		i32		first_child		   = -1;
		i32		next_sibling	   = -1;
		u8		visible			   = 0;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

#ifdef SFG_TOOLMODE
	void to_json(nlohmann::json& j, const entity_template_entity_raw& r);
	void from_json(const nlohmann::json& j, entity_template_entity_raw& r);
#endif

	class entity_manager;

	struct entity_template_raw
	{
		string							   name = "";
		vector<entity_template_entity_raw> entities;
		vector<string>					   resources;
		ostream							   component_buffer;

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
		void destroy();

#ifdef SFG_TOOLMODE
		static void collect_entities(entity_manager& em, world_handle h, vector<world_handle>& out);
		static void save_to_file(const char* file, world& w, const vector<world_handle>& handles);
		static void save_to_json(nlohmann::json& out, world& w, const vector<world_handle>& handles);
		static void load_from_json(const nlohmann::json& in, entity_template_raw& r);

		bool load_from_file(const char* relative_file, const char* base_path);
		bool load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension);
		void save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const;
		void get_dependencies(vector<string>& out_deps) const {};
#endif

		void get_sub_resources(vector<string>& out_res) const;
	};

}
