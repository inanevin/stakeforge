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

#include "shader_raw.hpp"
#include "data/ostream_vector.hpp"
#include "data/istream_vector.hpp"

#ifdef SFG_TOOLMODE
#include "io/file_system.hpp"
#include "editor/editor_settings.hpp"
#include "gfx/backend/backend.hpp"
#include "serialization/serialization.hpp"
#include "common/string_id.hpp"
#include "shader_variant_compiler.hpp"
#include "vendor/nhlohmann/json.hpp"
#include <fstream>
using json = nlohmann::json;
#endif

namespace SFG
{

#ifdef SFG_TOOLMODE

	bool shader_raw::compile_specialized(const string& shader_text, const vector<string>& folder_paths, const string& variant_style)
	{
		if (variant_style.compare("gbuffer_object") == 0)
			return shader_variant_compiler::compile_style_gbuffer_object(*this, shader_text, folder_paths);
		else if (variant_style.compare("forward_object") == 0)
			return shader_variant_compiler::compile_style_forward_object(*this, shader_text, folder_paths);
		else if (variant_style.compare("selection_outline") == 0)
			return shader_variant_compiler::compile_style_object_outline(*this, shader_text, folder_paths);
		else if (variant_style.compare("post_combiner") == 0)
			return shader_variant_compiler::compile_style_post_combiner(*this, shader_text, folder_paths);
		else if (variant_style.compare("gizmo") == 0)
			return shader_variant_compiler::compile_style_gizmo(*this, shader_text, folder_paths);
		else if (variant_style.compare("debug_triangle") == 0)
			return shader_variant_compiler::compile_style_debug_triangle(*this, shader_text, folder_paths);
		else if (variant_style.compare("debug_line_3d") == 0)
			return shader_variant_compiler::compile_style_debug_line(*this, shader_text, folder_paths);
		else if (variant_style.compare("gui") == 0)
			return shader_variant_compiler::compile_style_gui(*this, shader_text, folder_paths);
		else if (variant_style.compare("engine_gui") == 0)
			return shader_variant_compiler::compile_style_engine_gui(*this, shader_text, folder_paths);
		else if (variant_style.compare("particle_additive") == 0)
			return shader_variant_compiler::compile_style_particle_additive(*this, shader_text, folder_paths);
		else if (variant_style.compare("sprite") == 0)
			return shader_variant_compiler::compile_style_sprite(*this, shader_text, folder_paths);
		else if (variant_style.compare("swapchain") == 0)
			return shader_variant_compiler::compile_style_swapchain(*this, shader_text, folder_paths);

		SFG_ERR("Unrecognize shader variant style: {0} {1}", variant_style, name);
		return false;
	}

	bool shader_raw::load_from_file(const char* relative_file, const char* base_directory)
	{
		const string file_path = (base_directory == nullptr ? editor_settings::get().working_dir : string(base_directory)) + string(relative_file);
		if (!file_system::exists(file_path.c_str()))
		{
			SFG_ERR("file doesn't exists: {0}", file_path.c_str());
			return false;
		}
		try
		{
			std::ifstream f(file_path.c_str());
			json		  json_data = json::parse(f);
			f.close();

			// Receive & verify source.
			source			= json_data.value<string>("source", "");
			is_compute		= json_data.value<u8>("is_compute", 0);
			const string wd = base_directory == nullptr ? editor_settings::get().working_dir : base_directory;
			const string p	= file_path;
			name			= p.substr(wd.size(), p.size() - wd.size());

			const string full_source = base_directory == nullptr ? (editor_settings::get().working_dir + source) : (string(base_directory) + source);
			if (!file_system::exists(full_source.c_str()))
			{
				SFG_ERR("file doesn't exists: {0}", full_source.c_str());
				return false;
			}

			const string shader_text = file_system::read_file_as_string(full_source.c_str());
			if (shader_text.empty())
			{
				SFG_ERR("failed reading shader text! {0}", full_source.c_str());
				return false;
			}
			const string   folder_path = file_system::get_directory_of_file(full_source.c_str());
			const string   root		   = SFG_ROOT_DIRECTORY + string("assets/engine/shaders/");
			vector<string> folder_paths;
			folder_paths.push_back(folder_path);
			folder_paths.push_back(root);

			if (json_data.find("desc") != json_data.end())
			{
				const shader_desc default_desc = json_data.value<shader_desc>("desc", {});
				if (!shader_variant_compiler::compile_raw(*this, shader_text, folder_paths, default_desc))
					return false;
			}
			else
			{
				if (json_data.find("variant_style") == json_data.end())
				{
					SFG_ERR("failed compiling shader as it does not define a raw description neither a variant style! {0}", file_path);
					return false;
				}

				const string variant_style = json_data.value<string>("variant_style", "");
				if (!compile_specialized(shader_text, folder_paths, variant_style))
				{
					destroy();
					return false;
				}
			}
		}
		catch (std::exception e)
		{
			SFG_ERR("failed loading: {0}", e.what());
			return false;
		}
		return true;
	}

	bool shader_raw::load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension)
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

		string file_path				  = "";
		string source_path				  = "";
		u64	   saved_file_last_modified	  = 0;
		u64	   saved_source_last_modified = 0;
		stream >> file_path;
		stream >> source_path;
		stream >> saved_file_last_modified;
		stream >> saved_source_last_modified;

		stream.destroy();

		const u64 file_last_modified = file_system::get_last_modified_ticks(file_path);
		const u64 src_last_modified	 = file_system::get_last_modified_ticks(source_path);

		if (file_last_modified != saved_file_last_modified || src_last_modified != saved_source_last_modified)
			return false;

		stream = serialization::load_from_file(data_cache_path.c_str());
		deserialize(stream);
		stream.destroy();
		return true;
	}

	void shader_raw::save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const
	{
		const string sid_str			= std::to_string(TO_SID(name));
		const string relative			= file_system::get_filename_from_path(name);
		const string file_path			= resource_directory_path + name;
		const string source_path		= resource_directory_path + source;
		const u64	 file_last_modified = file_system::get_last_modified_ticks(file_path);
		const u64	 src_last_modified	= file_system::get_last_modified_ticks(source_path);

		const string meta_cache_path = cache_folder_path + relative + "-" + sid_str + "_meta" + extension;
		const string data_cache_path = cache_folder_path + relative + "-" + sid_str + "_data" + extension;

		ostream out_stream;
		out_stream << file_path;
		out_stream << source_path;
		out_stream << file_last_modified;
		out_stream << src_last_modified;
		serialization::save_to_file(meta_cache_path.c_str(), out_stream);

		out_stream.shrink(0);
		serialize(out_stream);
		serialization::save_to_file(data_cache_path.c_str(), out_stream);

		out_stream.destroy();
	}

	void shader_raw::get_dependencies(vector<string>& out_deps) const
	{
		out_deps.push_back(source);
	}

#endif

	void shader_raw::serialize(ostream& stream) const
	{
		stream << name;
		stream << source;
		stream << compile_variants;
		stream << pso_variants;
		stream << is_compute;
	}

	void shader_raw::deserialize(istream& stream)
	{
		stream >> name;
		stream >> source;
		stream >> compile_variants;
		stream >> pso_variants;
		stream >> is_compute;
	}

	void shader_raw::destroy()
	{
		for (compile_variant& cv : compile_variants)
			cv.destroy();
		compile_variants.clear();
	}

}
