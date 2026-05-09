// Copyright (c) 2025 Inan Evin

#include "shader_cook.hpp"
#include "shader_cook_variants.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/common/shader_description.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	bool shader_cooker::cook_from_file(const shader_cook_config_t& cfg, const char* full_path, ostream_t& stream)
	{
		if (cfg.type == shader_type_e::invalid)
		{
			SFG_ERR("invalid shader type for {0}", full_path);
			return false;
		}

		const string_t source = file_system_t::read_file_as_string(full_path);
		if (source.empty())
		{
			SFG_ERR("failed to read source {0}", full_path);
			return false;
		}

		const string_t			 directory	   = file_system_t::get_directory_of_file(full_path);
		const vector_t<string_t> include_paths = directory.empty() ? vector_t<string_t>{} : vector_t<string_t>{directory};

		vector_t<cook_compile_variant_t> compiles;
		vector_t<cook_pso_variant_t>	 psos;

		switch (cfg.type)
		{
		case shader_type_e::editor_ui_default:
			if (!shader_cook_variants_t::cook_editor_ui(source, include_paths, compiles, psos))
				return false;
			break;
		default:
			SFG_ERR("unsupported shader type {0}", static_cast<u8>(cfg.type));
			return false;
		}

		const u8 compile_variant_count = static_cast<u8>(compiles.size());
		const u8 pso_variant_count	   = static_cast<u8>(psos.size());

		if (compile_variant_count > shader_loader_t::MAX_COMPILE_VARIANTS)
		{
			SFG_ERR("too many compile variants ({0}, max {1})", compile_variant_count, shader_loader_t::MAX_COMPILE_VARIANTS);
			return false;
		}
		if (pso_variant_count > shader_loader_t::MAX_PSO_VARIANTS)
		{
			SFG_ERR("too many pso variants ({0}, max {1})", pso_variant_count, shader_loader_t::MAX_PSO_VARIANTS);
			return false;
		}

		stream << cfg.type;
		stream << compile_variant_count;

		for (const cook_compile_variant_t& v : compiles)
		{
			const u8 stages_count = static_cast<u8>(v.stages.size());
			SFG_ASSERT(stages_count < shader_loader_t::MAX_STAGE_PER_VARIANT);
			stream << stages_count;

			for (const cook_stage_blob_t& b : v.stages)
			{
				stream << b.stage;
				stream << static_cast<u32>(b.bytes.size());
				stream.write_raw(b.bytes.data(), b.bytes.size());
			}
		}

		stream << pso_variant_count;

		for (const cook_pso_variant_t& v : psos)
		{
			stream << v.compile_variant_index;
			stream << v.variant_flags;
			v.desc.serialize(stream);
		}

		return true;
	}

	void shader_cooker::collect_source_ticks(const char* full_path, vector_t<u64>& out)
	{
		out.push_back(file_system_t::get_last_modified_ticks(full_path));

		string_t directory = file_system_t::get_directory_of_file(full_path);
		if (!directory.empty() && directory.back() != '/')
			directory += '/';

		vector_t<string_t> include_lines;
		file_system_t::find_lines_with_keyword(full_path, "#include", include_lines);

		out.reserve(out.size() + include_lines.size());
		for (const string_t& line : include_lines)
		{
			const size_t open_quote = line.find('"');
			if (open_quote == string_t::npos)
				continue;
			const size_t close_quote = line.find('"', open_quote + 1);
			if (close_quote == string_t::npos)
				continue;

			const string_t rel_path = line.substr(open_quote + 1, close_quote - open_quote - 1);
			const string_t resolved = directory + rel_path;
			out.push_back(file_system_t::get_last_modified_ticks(resolved.c_str()));
		}
	}

	void from_json(const nlohmann::json& j, shader_cook_config_t& c)
	{
		c.type = j.value<shader_type_e>("type", shader_type_e::invalid);
	}
}
