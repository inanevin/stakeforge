// Copyright (c) 2025 Inan Evin

#include "shader_cook.hpp"
#include <iterator>
#include <cstddef>
#include <sfg/reflection/reflection_registry.hpp>
#include "shader_cook_variants.hpp"
#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/common/shader_description.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/serialization/compression.hpp>

namespace sfg
{
	namespace
	{
		void append_include_dir(const string_t& include_dir, vector_t<string_t>& out)
		{
			if (include_dir.empty())
				return;

			string_t path = include_dir;
			file_system_t::fix_path(path);

			if (!file_system_t::is_absolute_path(path.c_str()))
			{
				const string_t running_dir = file_system_t::get_running_directory();
				path					   = running_dir + path;
			}

			out.push_back(path);
		}

		void build_include_paths(const shader_cook_config_t& cfg, const char* full_path, vector_t<string_t>& out)
		{
			string_t directory = file_system_t::get_directory_of_file(full_path);
			file_system_t::fix_path(directory);
			file_system_t::fix_path_end_slash(directory);
			if (!directory.empty())
				out.push_back(directory);

			out.reserve(out.size() + cfg.include_dirs.size());
			for (const string_t& include_dir : cfg.include_dirs)
				append_include_dir(include_dir, out);
		}
	}

	bool shader_cooker::cook_from_file(const shader_cook_config_t& cfg, const char* full_path, resource_header_t& out_header, ostream_t& stream)
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

		vector_t<string_t> include_paths;
		build_include_paths(cfg, full_path, include_paths);

		vector_t<cook_compile_variant_t> compiles;
		vector_t<cook_pso_variant_t>	 psos;

		switch (cfg.type)
		{
		case shader_type_e::opaque_shader:
			if (!shader_cook_variants_t::cook_opaque_shader(source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook opaque shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::transparent_shader:
			if (!shader_cook_variants_t::cook_transparent_shader(source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook transparent shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::post_process_shader:
			if (!shader_cook_variants_t::cook_post_process_shader(source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook post process shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::ui_shader:
			if (!shader_cook_variants_t::cook_ui_shader(source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook UI shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::ui_text_shader:
			if (!shader_cook_variants_t::cook_ui_text_shader(source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook UI text shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::deferred_lighting:
			if (!shader_cook_variants_t::cook_deferred_lighting_shader(source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook deferred lighting shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::post_combiner:
			if (!shader_cook_variants_t::cook_post_combiner_shader(source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook post combiner shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::editor_ui_default:
			if (!shader_cook_variants_t::cook_editor_ui_default(source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook editor UI default shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::editor_ui_lcd_text:
			if (!shader_cook_variants_t::cook_editor_ui_lcd_text(source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook editor UI lcd text shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::editor_ui_text_grayscale:
			if (!shader_cook_variants_t::cook_editor_ui_text_grayscale(source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook editor UI grayscale text shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::editor_ui_sdf:
			if (!shader_cook_variants_t::cook_editor_ui_sdf(source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook editor UI sdf shader variants: {0}", full_path);
				return false;
			}
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

		out_header = {
			.magic		 = shader_loader_t::WIRE_MAGIC,
			.version	 = shader_loader_t::WIRE_VERSION,
			.source_tick = collect_source_tick(cfg, full_path),
		};

		ostream_t payload;
		payload << cfg.type;
		payload << compile_variant_count;

		for (const cook_compile_variant_t& v : compiles)
		{
			const u8 stages_count = static_cast<u8>(v.stages.size());
			SFG_ASSERT(stages_count < shader_loader_t::MAX_STAGE_PER_VARIANT);
			payload << stages_count;

			for (const cook_stage_blob_t& b : v.stages)
			{
				payload << b.stage;
				payload << static_cast<u32>(b.bytes.size());
				payload.write_raw(b.bytes.data(), b.bytes.size());
			}
		}

		payload << pso_variant_count;

		for (const cook_pso_variant_t& v : psos)
		{
			payload << v.compile_variant_index;
			payload << v.variant_flags;

			const size_t size = payload.get_size();
			payload << 0;

			v.desc.serialize(payload);
			u32 total = static_cast<u32>(payload.get_size() - size - sizeof(u32));
			SFG_MEMCPY(payload.get_raw() + size, &total, sizeof(u32));
		}

		stream = compressor_t::compress(payload);
		if (stream.get_size() == 0)
		{
			SFG_ERR("failed to compress shader payload: {0}", full_path);
			return false;
		}

		return true;
	}

	u64 shader_cooker::collect_source_tick(const char* full_path)
	{
		const shader_cook_config_t cfg = {};
		return collect_source_tick(cfg, full_path);
	}

	u64 shader_cooker::collect_source_tick(const shader_cook_config_t& cfg, const char* full_path)
	{
		u64 source_tick = file_system_t::get_last_modified_ticks(full_path);

		vector_t<string_t> include_paths;
		build_include_paths(cfg, full_path, include_paths);

		vector_t<string_t> include_lines;
		file_system_t::find_lines_with_keyword(full_path, "#include", include_lines);

		for (const string_t& line : include_lines)
		{
			const size_t open_quote = line.find('"');
			if (open_quote == string_t::npos)
				continue;
			const size_t close_quote = line.find('"', open_quote + 1);
			if (close_quote == string_t::npos)
				continue;

			const string_t rel_path = line.substr(open_quote + 1, close_quote - open_quote - 1);
			if (file_system_t::is_absolute_path(rel_path.c_str()))
			{
				source_tick = hashing_t::hash_u64_combine(source_tick, file_system_t::get_last_modified_ticks(rel_path.c_str()));
				continue;
			}

			for (const string_t& include_path : include_paths)
			{
				const string_t resolved = include_path + rel_path;
				if (file_system_t::exists(resolved.c_str()))
				{
					source_tick = hashing_t::hash_u64_combine(source_tick, file_system_t::get_last_modified_ticks(resolved.c_str()));
					break;
				}
			}
		}

		return source_tick;
	}

}

namespace sfg
{
	shader_cook_config_reflection_t::shader_cook_config_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<shader_cook_config_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{
				.name		  = "include_dirs",
				.display_name = "Include Directories",
				.type		  = reflected_value_type_e::vector,
				.sub_type_id  = "string"_hs,
				.offset		  = offsetof(shader_cook_config_t, include_dirs),
				.size		  = sizeof(vector_t<string_t>),
			},
			{
				.name		  = "type",
				.display_name = "Type",
				.type		  = reflected_value_type_e::enum8,
				.sub_type_id  = type_id_t<shader_type_e>::value,
				.offset		  = offsetof(shader_cook_config_t, type),
				.size		  = sizeof(shader_type_e),
			},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "shader_cook_config_t",
			.type_id   = type_id_t<shader_cook_config_t>::value,
			.size	   = sizeof(shader_cook_config_t),
			.alignment = alignof(shader_cook_config_t),
		});
	}
}
