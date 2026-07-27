// Copyright (c) 2025 Inan Evin

#include "shader_cook.hpp"
#include "shader.hpp"
#include "shader_cook_variants.hpp"
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <sfg/common/hashing.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/common/shader_description.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_container_ops.hpp>
#include <sfg/reflection/reflection_registry.hpp>
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

		string_t trim_arg(const string_t& value)
		{
			size_t begin = 0;
			while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0)
				++begin;

			size_t end = value.size();
			while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
				--end;

			return value.substr(begin, end - begin);
		}

		bool find_macro_close(const string_t& source, size_t open, size_t& out_close)
		{
			bool quoted = false;
			u32	 depth	= 1;
			for (size_t i = open + 1; i < source.size(); ++i)
			{
				const char c = source[i];
				if (c == '"' && (i == 0 || source[i - 1] != '\\'))
				{
					quoted = !quoted;
					continue;
				}

				if (quoted)
					continue;

				if (c == '(')
					++depth;
				else if (c == ')')
				{
					--depth;
					if (depth == 0)
					{
						out_close = i;
						return true;
					}
				}
			}

			return false;
		}

		void split_macro_args(const string_t& body, vector_t<string_t>& out)
		{
			out.resize(0);
			bool   quoted = false;
			u32	   depth  = 0;
			size_t start  = 0;

			for (size_t i = 0; i < body.size(); ++i)
			{
				const char c = body[i];
				if (c == '"' && (i == 0 || body[i - 1] != '\\'))
				{
					quoted = !quoted;
					continue;
				}

				if (quoted)
					continue;

				if (c == '(' || c == '{' || c == '[')
					++depth;
				else if (c == ')' || c == '}' || c == ']')
					--depth;
				else if (c == ',' && depth == 0)
				{
					out.push_back(trim_arg(body.substr(start, i - start)));
					start = i + 1;
				}
			}

			out.push_back(trim_arg(body.substr(start)));
		}

		bool copy_definition_name(const string_t& arg, char* out, size_t out_size)
		{
			const string_t value = trim_arg(arg);
			if (value.size() < 2 || value.front() != '"' || value.back() != '"')
				return false;

			const string_t name = value.substr(1, value.size() - 2);
			if (name.size() >= out_size)
				return false;

			SFG_MEMSET(out, 0, out_size);
			SFG_MEMCPY(out, name.data(), name.size());
			return true;
		}

		bool parse_f32_arg(const string_t& arg, f32& out)
		{
			const string_t value = trim_arg(arg);
			char*		   end	 = nullptr;
			out					 = std::strtof(value.c_str(), &end);
			if (end == value.c_str())
				return false;

			while (*end != '\0' && std::isspace(static_cast<unsigned char>(*end)) != 0)
				++end;

			if (*end == 'f' || *end == 'F')
				++end;

			while (*end != '\0' && std::isspace(static_cast<unsigned char>(*end)) != 0)
				++end;

			return *end == '\0';
		}

		bool parse_f32_args(const vector_t<string_t>& args, size_t offset, size_t count, f32* out)
		{
			for (size_t i = 0; i < count; ++i)
			{
				if (!parse_f32_arg(args[offset + i], out[i]))
					return false;
			}
			return true;
		}

		bool parse_u32_arg(const string_t& arg, u32& out)
		{
			const string_t value = trim_arg(arg);
			if (value.empty() || value.front() == '-')
				return false;

			char*	  end	 = nullptr;
			const u64 parsed = std::strtoull(value.c_str(), &end, 0);
			if (end == value.c_str() || parsed > 0xffffffffull)
				return false;

			while (*end != '\0' && std::isspace(static_cast<unsigned char>(*end)) != 0)
				++end;

			if (*end == 'u' || *end == 'U')
				++end;

			while (*end != '\0' && std::isspace(static_cast<unsigned char>(*end)) != 0)
				++end;

			if (*end != '\0')
				return false;

			out = static_cast<u32>(parsed);
			return true;
		}

		bool parse_u32_args(const vector_t<string_t>& args, size_t offset, size_t count, u32* out)
		{
			for (size_t i = 0; i < count; ++i)
			{
				if (!parse_u32_arg(args[offset + i], out[i]))
					return false;
			}
			return true;
		}

		bool find_macro_call(const string_t& source, const char* macro, size_t search_from, size_t& out_open, size_t& out_close)
		{
			const size_t macro_len = string_t(macro).size();
			size_t		 pos	   = source.find(macro, search_from);
			while (pos != string_t::npos)
			{
				const bool valid_before = pos == 0 || (std::isalnum(static_cast<unsigned char>(source[pos - 1])) == 0 && source[pos - 1] != '_');
				const bool valid_after	= pos + macro_len >= source.size() || (std::isalnum(static_cast<unsigned char>(source[pos + macro_len])) == 0 && source[pos + macro_len] != '_');
				if (valid_before && valid_after)
				{
					size_t open = pos + macro_len;
					while (open < source.size() && std::isspace(static_cast<unsigned char>(source[open])) != 0)
						++open;

					if (open < source.size() && source[open] == '(' && find_macro_close(source, open, out_close))
					{
						out_open = open;
						return true;
					}
				}

				pos = source.find(macro, pos + macro_len);
			}

			return false;
		}

		bool parse_texture_definition(const vector_t<string_t>& args, shader_data_definition_t& out)
		{
			if (args.size() != 2 || out.textures.full())
				return false;

			shader_texture_definition_t definition = {};

			if (!copy_definition_name(args[0], definition.texture_name, sizeof(definition.texture_name)))
				return false;

			if (args[1] == "sfg_texture2d")
				definition.type = shader_texture_type_e::texture2d;
			else if (args[1] == "sfg_texturecube")
				definition.type = shader_texture_type_e::texture_cube;
			else
				return false;

			out.textures.push_back(definition);
			return true;
		}

		bool parse_sampler_definition(const vector_t<string_t>& args, shader_data_definition_t& out)
		{
			if (args.size() != 1 || out.samplers.full())
				return false;

			shader_sampler_definition_t definition = {};
			if (!copy_definition_name(args[0], definition.sampler_name, sizeof(definition.sampler_name)))
				return false;

			out.samplers.push_back(definition);
			return true;
		}

		bool parse_param_hint(const string_t& arg, shader_param_hint_e& out)
		{
			if (arg == "sfg_color")
			{
				out = shader_param_hint_e::color;
				return true;
			}
			if (arg == "sfg_pack_uint2")
			{
				out = shader_param_hint_e::pack_uint2;
				return true;
			}
			if (arg == "sfg_toggle")
			{
				out = shader_param_hint_e::toggle;
				return true;
			}
			return false;
		}

		bool parse_param_definition(const vector_t<string_t>& args, shader_param_type_e type, shader_data_definition_t& out)
		{
			if (out.parameters.full())
				return false;

			shader_param_definition_t definition = {};
			if (!copy_definition_name(args[0], definition.param_name, sizeof(definition.param_name)))
				return false;

			definition.type = type;
			switch (type)
			{
			case shader_param_type_e::f32:
				if (args.size() != 4 || !parse_f32_args(args, 1, 1, definition.default_value) || !parse_f32_args(args, 2, 1, definition.min_value) || !parse_f32_args(args, 3, 1, definition.max_value))
					return false;
				break;
			case shader_param_type_e::vec2:
				if (args.size() != 1)
					return false;
				break;
			case shader_param_type_e::vec4:
				if (args.size() != 1 && args.size() != 2)
					return false;
				if (args.size() == 2 && !parse_param_hint(args[1], definition.hint))
					return false;
				break;
			case shader_param_type_e::u32:
				if ((args.size() != 4 && args.size() != 5) || !parse_u32_args(args, 1, 1, definition.default_value_u32) || !parse_u32_args(args, 2, 1, definition.min_value_u32) || !parse_u32_args(args, 3, 1, definition.max_value_u32))
					return false;
				if (args.size() == 5 && (!parse_param_hint(args[4], definition.hint) || definition.hint != shader_param_hint_e::toggle))
					return false;
				break;
			default:
				return false;
			}

			out.parameters.push_back(definition);
			return true;
		}

		template <typename Fn> bool parse_macro_definitions(const string_t& source, const char* macro, const char* full_path, Fn&& fn)
		{
			vector_t<string_t> args		   = {};
			size_t			   search_from = 0;
			size_t			   open		   = 0;
			size_t			   close	   = 0;
			while (find_macro_call(source, macro, search_from, open, close))
			{
				split_macro_args(source.substr(open + 1, close - open - 1), args);
				if (!fn(args))
				{
					SFG_ERR("invalid {0} definition in {1}", macro, full_path);
					return false;
				}
				search_from = close + 1;
			}

			return true;
		}

		bool parse_param_definitions(const string_t& source, const char* full_path, shader_data_definition_t& out)
		{
			vector_t<string_t> args		   = {};
			size_t			   search_from = 0;
			for (;;)
			{
				size_t f32_open	  = string_t::npos;
				size_t f32_close  = string_t::npos;
				size_t u32_open	  = string_t::npos;
				size_t u32_close  = string_t::npos;
				size_t vec2_open  = string_t::npos;
				size_t vec2_close = string_t::npos;
				size_t vec4_open  = string_t::npos;
				size_t vec4_close = string_t::npos;
				find_macro_call(source, "SFG_MATERIAL_PARAM_F32", search_from, f32_open, f32_close);
				find_macro_call(source, "SFG_MATERIAL_PARAM_U32", search_from, u32_open, u32_close);
				find_macro_call(source, "SFG_MATERIAL_PARAM_VEC2", search_from, vec2_open, vec2_close);
				find_macro_call(source, "SFG_MATERIAL_PARAM_VEC4", search_from, vec4_open, vec4_close);

				if (f32_open == string_t::npos && u32_open == string_t::npos && vec2_open == string_t::npos && vec4_open == string_t::npos)
					return true;

				shader_param_type_e type  = shader_param_type_e::f32;
				size_t				open  = f32_open;
				size_t				close = f32_close;
				if (u32_open < open)
				{
					type  = shader_param_type_e::u32;
					open  = u32_open;
					close = u32_close;
				}
				if (vec2_open < open)
				{
					type  = shader_param_type_e::vec2;
					open  = vec2_open;
					close = vec2_close;
				}
				if (vec4_open < open)
				{
					type  = shader_param_type_e::vec4;
					open  = vec4_open;
					close = vec4_close;
				}

				split_macro_args(source.substr(open + 1, close - open - 1), args);
				if (!parse_param_definition(args, type, out))
				{
					SFG_ERR("invalid material parameter definition in {0}", full_path);
					return false;
				}
				search_from = close + 1;
			}
		}

		bool parse_shader_data_definition(const string_t& source, const char* full_path, shader_data_definition_t& out)
		{
			if (!parse_macro_definitions(source, "SFG_MATERIAL_TEXTURE", full_path, [&](const vector_t<string_t>& args) { return parse_texture_definition(args, out); }))
				return false;
			if (!parse_macro_definitions(source, "SFG_MATERIAL_SAMPLER", full_path, [&](const vector_t<string_t>& args) { return parse_sampler_definition(args, out); }))
				return false;
			if (!parse_param_definitions(source, full_path, out))
				return false;

			return true;
		}

		string_t make_compile_source(const string_t& source)
		{
			string_t out = {};
			out.reserve(source.size() + 192);
			out += "#define SFG_MATERIAL_TEXTURE(...)\n";
			out += "#define SFG_MATERIAL_SAMPLER(...)\n";
			out += "#define SFG_MATERIAL_PARAM_F32(...)\n";
			out += "#define SFG_MATERIAL_PARAM_U32(...)\n";
			out += "#define SFG_MATERIAL_PARAM_VEC2(...)\n";
			out += "#define SFG_MATERIAL_PARAM_VEC4(...)\n";
			out += source;
			return out;
		}

		void strip_utf8_bom(string_t& source)
		{
			if (source.size() >= 3 && static_cast<u8>(source[0]) == 0xef && static_cast<u8>(source[1]) == 0xbb && static_cast<u8>(source[2]) == 0xbf)
				source.erase(0, 3);
		}
	}

	bool shader_cooker::cook_from_file(const shader_cook_config_t& cfg, const char* full_path, resource_header_t& out_header, ostream_t& stream, shader_data_definition_t& out_definition)
	{
		if (cfg.type == shader_type_e::invalid)
		{
			SFG_ERR("invalid shader type for {0}", full_path);
			return false;
		}

		string_t source = file_system_t::read_file_as_string(full_path);
		if (source.empty())
		{
			SFG_ERR("failed to read source {0}", full_path);
			return false;
		}

		strip_utf8_bom(source);

		vector_t<string_t> include_paths = {};
		build_include_paths(cfg, full_path, include_paths);

		out_definition = {};
		if (!parse_shader_data_definition(source, full_path, out_definition))
			return false;

		const string_t compile_source = make_compile_source(source);

		vector_t<cook_compile_variant_t> compiles = {};
		vector_t<cook_pso_variant_t>	 psos	  = {};

		switch (cfg.type)
		{
		case shader_type_e::object_shader:
			if (!shader_cook_variants_t::cook_object_shader(compile_source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook object shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::sprite_shader:
			if (!shader_cook_variants_t::cook_sprite_shader(compile_source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook sprite shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::post_process_shader:
			if (!shader_cook_variants_t::cook_post_process_shader(compile_source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook post process shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::ui_shader:
			if (!shader_cook_variants_t::cook_ui_shader(compile_source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook UI shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::ui_text_shader:
			if (!shader_cook_variants_t::cook_ui_text_shader(compile_source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook UI text shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::skybox_shader:
			if (!shader_cook_variants_t::cook_skybox_shader(compile_source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook skybox shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::deferred_lighting:
			if (!shader_cook_variants_t::cook_deferred_lighting_shader(compile_source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook deferred lighting shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::post_combiner:
			if (!shader_cook_variants_t::cook_post_combiner_shader(compile_source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook post combiner shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::texture_blit:
			if (!shader_cook_variants_t::cook_texture_blit_shader(compile_source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook thumbnail capture shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::editor_gizmo:
			if (!shader_cook_variants_t::cook_editor_gizmo_shader(compile_source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook editor gizmo shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::debug_line:
			if (!shader_cook_variants_t::cook_debug_line_shader(compile_source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook debug line shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::debug_text:
			if (!shader_cook_variants_t::cook_debug_text_shader(compile_source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook debug text shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::debug_triangle:
			if (!shader_cook_variants_t::cook_debug_triangle_shader(compile_source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook debug triangle shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::ssao:
		case shader_type_e::ssao_upsample:
		case shader_type_e::bloom_downsample:
		case shader_type_e::bloom_upsample:
		case shader_type_e::clustered_light_culling:
		case shader_type_e::reflection_specular_prefilter:
		case shader_type_e::reflection_diffuse_sh:
			if (!shader_cook_variants_t::cook_compute_shader(compile_source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook compute shader variant: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::editor_ui_default:
			if (!shader_cook_variants_t::cook_editor_ui_default(compile_source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook editor UI default shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::editor_ui_lcd_text:
			if (!shader_cook_variants_t::cook_editor_ui_lcd_text(compile_source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook editor UI lcd text shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::editor_ui_text_grayscale:
			if (!shader_cook_variants_t::cook_editor_ui_text_grayscale(compile_source, include_paths, compiles, psos))
			{
				SFG_ERR("failed to cook editor UI grayscale text shader variants: {0}", full_path);
				return false;
			}
			break;
		case shader_type_e::editor_ui_sdf:
			if (!shader_cook_variants_t::cook_editor_ui_sdf(compile_source, include_paths, compiles, psos))
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

		if (compile_variant_count > SFG_SHADER_MAX_COMPILE_VARIANTS)
		{
			SFG_ERR("too many compile variants ({0}, max {1})", compile_variant_count, SFG_SHADER_MAX_COMPILE_VARIANTS);
			return false;
		}

		if (pso_variant_count > SFG_SHADER_MAX_PSO_VARIANTS)
		{
			SFG_ERR("too many pso variants ({0}, max {1})", pso_variant_count, SFG_SHADER_MAX_PSO_VARIANTS);
			return false;
		}

		out_header = {
			.type		 = resource_type_e::shader,
			.magic		 = shader_loader_t::WIRE_MAGIC,
			.version	 = shader_loader_t::WIRE_VERSION,
			.source_tick = collect_source_tick(cfg, full_path),
		};

		ostream_t payload = {};
		payload << cfg.type;
		payload << compile_variant_count;

		for (const cook_compile_variant_t& v : compiles)
		{
			const u8 stages_count = static_cast<u8>(v.stages.size());
			SFG_ASSERT(stages_count < SFG_SHADER_MAX_STAGE_PER_VARIANT);
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

		vector_t<string_t> include_paths = {};
		build_include_paths(cfg, full_path, include_paths);

		vector_t<string_t> include_lines = {};
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

		registry.register_type({
			.name		  = "shader_cook_config_t",
			.display_name = "Shader Cook Config",
			.fields =
				{
					{.container_ops = reflection_container_ops_t::vector_ops<string_t>(reflected_value_type_e::string),
					 .name			= "include_dirs",
					 .display_name	= "Include Directories",
					 .offset		= offsetof(shader_cook_config_t, include_dirs),
					 .size			= sizeof(vector_t<string_t>),
					 .type			= reflected_value_type_e::container},
					{.name = "type", .display_name = "Type", .sub_type_id = type_id_t<shader_type_e>::value, .offset = offsetof(shader_cook_config_t, type), .size = sizeof(shader_type_e), .type = reflected_value_type_e::u8},
				},
			.type_id   = type_id_t<shader_cook_config_t>::value,
			.size	   = sizeof(shader_cook_config_t),
			.alignment = alignof(shader_cook_config_t),
		});
	}
}
