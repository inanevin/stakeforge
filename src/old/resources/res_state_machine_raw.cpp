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

#include "res_state_machine_raw.hpp"
#include "data/ostream_vector.hpp"
#include "data/istream_vector.hpp"
#include "io/log.hpp"

#ifdef SFG_TOOLMODE
#include "io/file_system.hpp"
#include "serialization/serialization.hpp"
#include <vendor/nhlohmann/json.hpp>
#include <fstream>
using json = nlohmann::json;
#endif

namespace SFG
{
	void res_state_machine_parameter_raw::serialize(ostream& stream) const
	{
		stream << name;
		stream << value;
	}

	void res_state_machine_parameter_raw::deserialize(istream& stream)
	{
		stream >> name;
		stream >> value;
	}

	void res_state_machine_sample_raw::serialize(ostream& stream) const
	{
		stream << animation_sid;
		stream << base_model_str;
		stream << blend_point;
	}

	void res_state_machine_sample_raw::deserialize(istream& stream)
	{
		stream >> animation_sid;
		stream >> base_model_str;
		stream >> blend_point;
	}

	void res_state_machine_state_raw::serialize(ostream& stream) const
	{
		stream << name;
		stream << mask;
		stream << duration;
		stream << speed;
		stream << is_looping;
		stream << blend_type;
		stream << blend_param_x;
		stream << blend_param_y;
		stream << samples;
	}

	void res_state_machine_state_raw::deserialize(istream& stream)
	{
		stream >> name;
		stream >> mask;
		stream >> duration;
		stream >> speed;
		stream >> is_looping;
		stream >> blend_type;
		stream >> blend_param_x;
		stream >> blend_param_y;
		stream >> samples;
	}

	void res_state_machine_transition_raw::serialize(ostream& stream) const
	{
		stream << from_state;
		stream << to_state;
		stream << parameter;
		stream << duration;
		stream << target;
		stream << priority;
		stream << compare;
	}

	void res_state_machine_transition_raw::deserialize(istream& stream)
	{
		stream >> from_state;
		stream >> to_state;
		stream >> parameter;
		stream >> duration;
		stream >> target;
		stream >> priority;
		stream >> compare;
	}

	void res_state_machine_raw::serialize(ostream& stream) const
	{
		stream << name;
		stream << initial_state;
		stream << parameters;
		stream << states;
		stream << transitions;
	}

	void res_state_machine_raw::deserialize(istream& stream)
	{
		stream >> name;
		stream >> initial_state;
		stream >> parameters;
		stream >> states;
		stream >> transitions;
	}

#ifdef SFG_TOOLMODE

	// JSON (de)serialization helpers so sub-structs convert implicitly
	static animation_transition_compare str_to_compare(const string& s)
	{
		if (s == "equals")
			return animation_transition_compare::equals;
		if (s == "not_equals")
			return animation_transition_compare::not_equals;
		if (s == "greater")
			return animation_transition_compare::greater;
		if (s == "lesser")
			return animation_transition_compare::lesser;
		if (s == "gequals")
			return animation_transition_compare::gequals;
		if (s == "lequals")
			return animation_transition_compare::lequals;
		return animation_transition_compare::greater;
	}

	static const char* compare_to_str(animation_transition_compare c)
	{
		switch (c)
		{
		case animation_transition_compare::equals:
			return "equals";
		case animation_transition_compare::not_equals:
			return "not_equals";
		case animation_transition_compare::greater:
			return "greater";
		case animation_transition_compare::lesser:
			return "lesser";
		case animation_transition_compare::gequals:
			return "gequals";
		case animation_transition_compare::lequals:
			return "lequals";
		default:
			return "greater";
		}
	}

	void to_json(json& j, const res_state_machine_parameter_raw& p)
	{
		j["name"]  = p.name;
		j["value"] = p.value;
	}

	void from_json(const json& j, res_state_machine_parameter_raw& p)
	{
		p.name	= j.value<string>("name", "");
		p.value = j.value<f32>("value", 0.0f);
	}

	void to_json(json& j, const res_state_machine_sample_raw& s)
	{
		j["animation"] = s.base_model_str;
		j["point"]	   = s.blend_point;
	}

	void from_json(const json& j, res_state_machine_sample_raw& s)
	{
		s.animation_str = j.value<string>("animation", "");
		s.animation_sid = TO_SID(s.animation_str);
		s.blend_point	= j.value<vector2>("point", vector2::zero);

		const size_t last_s = s.animation_str.find_last_of('/');
		if (last_s != string::npos)
			s.base_model_str = s.animation_str.substr(0, last_s);
		else
			s.base_model_str = s.animation_str;
	}

	void to_json(json& j, const res_state_machine_state_raw& s)
	{
		j				   = json{};
		j["name"]		   = s.name;
		j["duration"]	   = s.duration;
		j["speed"]		   = s.speed;
		j["loop"]		   = s.is_looping;
		j["blend"]		   = s.blend_type == 0 ? "none" : (s.blend_type == 2 ? "2d" : "1d");
		j["blend_param_x"] = s.blend_param_x;
		j["blend_param_y"] = s.blend_param_y;
		j["samples"]	   = s.samples;
		j["mask"]		   = s.mask;
	}

	void from_json(const json& j, res_state_machine_state_raw& s)
	{
		s.name			   = j.value<string>("name", "");
		s.duration		   = j.value<f32>("duration", 0.0f);
		s.speed			   = j.value<f32>("speed", 1.0f);
		s.is_looping	   = j.value<u8>("loop", 1);
		s.mask			   = j.value<string>("mask", "");
		const string blend = j.value<string>("blend", "1d");
		if (blend == "2d")
			s.blend_type = 2;
		else if (blend == "none")
			s.blend_type = 0;
		else
			s.blend_type = 1;
		s.blend_param_x = j.value<string>("blend_param_x", "");
		s.blend_param_y = j.value<string>("blend_param_y", "");
		s.samples		= j.value<vector<res_state_machine_sample_raw>>("samples", {});
	}

	void to_json(json& j, const res_state_machine_transition_raw& t)
	{
		j["from"]	   = t.from_state;
		j["to"]		   = t.to_state;
		j["parameter"] = t.parameter;
		j["duration"]  = t.duration;
		j["target"]	   = t.target;
		j["priority"]  = t.priority;
		j["compare"]   = compare_to_str(t.compare);
	}

	void from_json(const json& j, res_state_machine_transition_raw& t)
	{
		t.from_state = j.value<string>("from", "");
		t.to_state	 = j.value<string>("to", "");
		t.parameter	 = j.value<string>("parameter", "");
		t.duration	 = j.value<f32>("duration", 0.0f);
		t.target	 = j.value<f32>("target", 0.0f);
		t.priority	 = j.value<u8>("priority", 0);
		t.compare	 = str_to_compare(j.value<string>("compare", "greater"));
	}

	void to_json(json& j, const res_state_machine_mask_raw& m)
	{
		j["name"] = m.name;
	}

	void from_json(const json& j, res_state_machine_mask_raw& m)
	{
		m.name = j.value<string>("name", "");
	}

	bool res_state_machine_raw::load_from_file(const char* relative_file, const char* base_path)
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
			json		  j = json::parse(f);
			f.close();

			name		  = relative_file;
			initial_state = j.value<string>("initial_state", "");
			parameters	  = j.value<vector<res_state_machine_parameter_raw>>("parameters", {});
			states		  = j.value<vector<res_state_machine_state_raw>>("states", {});
			transitions	  = j.value<vector<res_state_machine_transition_raw>>("transitions", {});
			masks		  = j.value<vector<res_state_machine_mask_raw>>("masks", {});
		}
		catch (std::exception e)
		{
			SFG_ERR("failed loading: {0}", e.what());
			return false;
		}
		return true;
	}

	bool res_state_machine_raw::load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension)
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

	void res_state_machine_raw::save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const
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

	void res_state_machine_raw::get_sub_resources(vector<string>& out_res) const
	{
		for (const res_state_machine_state_raw& sr : states)
		{
			for (const res_state_machine_sample_raw& r : sr.samples)
				out_res.push_back(r.base_model_str);
		}
	}

}
