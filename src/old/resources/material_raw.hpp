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

#include "data/ostream.hpp"
#include "data/static_vector.hpp"
#include "common/string_id.hpp"
#include "resources/common_resources.hpp"
#include "gfx/common/gfx_constants.hpp"
#include "gfx/common/descriptions.hpp"
#include "gfx/common/common_material.hpp"

#ifdef SFG_TOOLMODE
#include "data/string.hpp"
#include "vendor/nhlohmann/json.hpp"
#endif

namespace SFG
{
	class istream;

#ifdef SFG_TOOLMODE
	struct parameter_entry
	{
		string		   name;
		nlohmann::json value;
	};

	void to_json(nlohmann::json& j, const parameter_entry& p);
	void from_json(const nlohmann::json& j, parameter_entry& p);

#endif

	struct material_raw
	{
		ostream			   material_data	  = {};
		string_id		   shader			  = 0;
		vector<string_id>  textures			  = {};
		string			   shader_path		  = "";
		vector<string>	   textures_path	  = {};
		material_pass_mode pass_mode		  = material_pass_mode::gbuffer;
		u8				   double_sided		  = 0;
		u8				   use_alpha_cutoff	  = 0;
		string			   name				  = "";
		string_id		   sid				  = 0;
		sampler_desc	   sampler_definition = {};
		u16				   draw_priority	  = 0;

		inline void destroy()
		{
			if (material_data.get_size() != 0)
				material_data.destroy();
		}

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);

#ifdef SFG_TOOLMODE
		bool load_from_file(const char* relative_file, const char* base_path);
		bool load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension);
		void save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const;
		void get_dependencies(vector<string>& out_deps) const {};
#endif

		void get_sub_resources(vector<string>& out_res) const;
	};
}
