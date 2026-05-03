// Copyright (c) 2025 Inan Evin

#include "resource_cooker.hpp"
#include "font_cook.hpp"
#include "shader_cook.hpp"
#include "shader_variant_compiler.hpp"
#include "texture_cook.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/data/string.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/data/unique.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/serialization/serialization.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	namespace
	{
		using json = nlohmann::json;

		cook_kind_e resolve_kind_from_extension(const string_t& ext)
		{
			if (ext == "png" || ext == "jpg")
				return cook_kind_e::texture;
			if (ext == "mp3")
				return cook_kind_e::audio;
			if (ext == "glb")
				return cook_kind_e::glb;
			if (ext == "ttf")
				return cook_kind_e::font;
			if (ext == "hlsl")
				return cook_kind_e::shader;
			return cook_kind_e::invalid;
		}

		cook_kind_e resolve_kind_from_schema(const string_t& schema)
		{
			if (schema == "sfg.schema.mat")
				return cook_kind_e::material;
			if (schema == "sfg.schema.particle")
				return cook_kind_e::particle;
			if (schema == "sfg.schema.sampler")
				return cook_kind_e::sampler;
			if (schema == "sfg.schema.phy")
				return cook_kind_e::physical_material;
			if (schema == "sfg.schema.asm")
				return cook_kind_e::animation_state_machine;
			if (schema == "sfg.schema.prefab")
				return cook_kind_e::prefab;
			return cook_kind_e::invalid;
		}

		const char* extension_for_kind(cook_kind_e kind)
		{
			switch (kind)
			{
			case cook_kind_e::texture:
				return ".sfg_texture";
			case cook_kind_e::audio:
				return ".sfg_audio";
			case cook_kind_e::glb:
				return ".sfg_glb";
			case cook_kind_e::font:
				return ".sfg_font";
			case cook_kind_e::shader:
				return ".sfg_shader";
			case cook_kind_e::material:
				return ".sfg_material";
			case cook_kind_e::particle:
				return ".sfg_particle";
			case cook_kind_e::sampler:
				return ".sfg_sampler";
			case cook_kind_e::physical_material:
				return ".sfg_phy";
			case cook_kind_e::animation_state_machine:
				return ".sfg_asm";
			case cook_kind_e::prefab:
				return ".sfg_prefab";
			default:
				return ".sfg_unknown";
			}
		}

		string_t make_output_path(const char* full_path, const char* output_directory, const char* extension)
		{
			string_t out_dir(output_directory);
			file_system::fix_path(out_dir);
			if (!out_dir.empty() && out_dir.back() != '/')
				out_dir.push_back('/');
			const string_t name = file_system::get_filename_from_path(string_t(full_path));
			out_dir += name;
			out_dir += extension;
			return out_dir;
		}

		shader_type_e parse_shader_type(const string_t& s)
		{
			if (s == "ui_default")
				return shader_type_e::editor_ui_default;
			if (s == "ui_text")
				return shader_type_e::editor_ui_text;
			if (s == "ui_sdf")
				return shader_type_e::editor_ui_sdf;
			return shader_type_e::invalid;
		}

		void parse_shader_arguments(const string_t& arguments, shader_config_t& cfg)
		{
			if (arguments.empty())
				return;

			vector_t<string_t> tokens;
			string_util::split(tokens, arguments, ",");

			for (string_t& tok : tokens)
			{
				string_util::remove_whitespace(tok);
				if (tok.empty())
					continue;

				const size_t eq = tok.find('=');
				if (eq == string_t::npos)
					continue;

				const string_t key = tok.substr(0, eq);
				const string_t val = tok.substr(eq + 1);
				if (key.empty() || val.empty())
					continue;

				if (key == "shader_type" || key == "type")
					cfg.type = parse_shader_type(val);
			}
		}

		void parse_texture_arguments(const string_t& arguments, texture_config_t& cfg)
		{
			if (arguments.empty())
				return;

			vector_t<string_t> tokens;
			string_util::split(tokens, arguments, ",");

			for (string_t& tok : tokens)
			{
				string_util::remove_whitespace(tok);
				if (tok.empty())
					continue;

				const size_t eq = tok.find('=');
				if (eq == string_t::npos)
					continue;

				const string_t key = tok.substr(0, eq);
				const string_t val = tok.substr(eq + 1);
				if (key.empty() || val.empty())
					continue;

				bool b = false;
				if (key == "generate_mipmaps" || key == "mips")
				{
					if (string_util::to_bool(val, b))
						cfg.generate_mipmaps = b;
				}
				else if (key == "is_linear" || key == "linear")
				{
					if (string_util::to_bool(val, b))
						cfg.is_linear = b;
				}
			}
		}

		void parse_font_arguments(const string_t& arguments, font_config_t& cfg)
		{
			if (arguments.empty())
				return;

			vector_t<string_t> tokens;
			string_util::split(tokens, arguments, ",");

			for (string_t& tok : tokens)
			{
				string_util::remove_whitespace(tok);
				if (tok.empty())
					continue;

				const size_t eq = tok.find('=');
				if (eq == string_t::npos)
					continue;

				const string_t key = tok.substr(0, eq);
				const string_t val = tok.substr(eq + 1);
				if (key.empty() || val.empty())
					continue;

				int ival = 0;
				f32 fval = 0.0f;
				u32 udec = 0;

				if (key == "size")
				{
					if (string_util::to_int(val, ival))
						cfg.size = static_cast<u32>(ival);
				}
				else if (key == "range_start")
				{
					if (string_util::to_int(val, ival))
						cfg.range_start = static_cast<u32>(ival);
				}
				else if (key == "range_end")
				{
					if (string_util::to_int(val, ival))
						cfg.range_end = static_cast<u32>(ival);
				}
				else if (key == "kind")
				{
					if (val == "bitmap")
						cfg.kind = font_kind_e::bitmap;
					else if (val == "sdf")
						cfg.kind = font_kind_e::sdf;
					else if (val == "lcd")
						cfg.kind = font_kind_e::lcd;
				}
				else if (key == "sdf_padding")
				{
					if (string_util::to_int(val, ival))
						cfg.sdf_padding = ival;
				}
				else if (key == "sdf_edge")
				{
					if (string_util::to_int(val, ival))
						cfg.sdf_edge = ival;
				}
				else if (key == "sdf_distance")
				{
					if (string_util::to_float(val, fval, udec))
						cfg.sdf_distance = fval;
				}
			}
		}

		cook_result_e cook_texture(const char* full_path, const cooking_options_t& options, ostream_t& stream)
		{
			texture_config_t cfg = {};
			parse_texture_arguments(options.arguments, cfg);

			texture_cook_t tex = {};
			if (!texture_cook_from_file(full_path, cfg, tex))
			{
				SFG_ERR("failed to load texture {0}", full_path);
				return cook_result_e::cook_failed;
			}

			if (!texture_cook_serialize(tex, stream))
			{
				SFG_ERR("failed to serialize cooked texture {0}", full_path);
				return cook_result_e::cook_failed;
			}

			return cook_result_e::success;
		}

		cook_result_e cook_audio(const char*, const cooking_options_t&, ostream_t&)
		{
			return cook_result_e::success;
		}

		cook_result_e cook_glb(const char*, const cooking_options_t&, ostream_t&)
		{
			return cook_result_e::success;
		}

		cook_result_e cook_font(const char* full_path, const cooking_options_t& options, ostream_t& stream)
		{
			char*  ttf_data = nullptr;
			size_t ttf_size = 0;
			file_system::read_file(full_path, ttf_data, ttf_size);
			if (ttf_data == nullptr || ttf_size == 0)
			{
				SFG_ERR("failed to read font file {0}", full_path);
				return cook_result_e::cook_failed;
			}

			font_config_t cfg = {};
			parse_font_arguments(options.arguments, cfg);

			unique_t<font_cook_t>  fnt	  = make_unique<font_cook_t>();
			const span_t<const u8> ttf_sp = {reinterpret_cast<const u8*>(ttf_data), ttf_size};
			const bool			   loaded = font_cook_from_ttf(ttf_sp, cfg, *fnt);
			delete[] ttf_data;

			if (!loaded)
			{
				SFG_ERR("failed to load font {0}", full_path);
				return cook_result_e::cook_failed;
			}

			if (!font_cook_serialize(*fnt, stream))
			{
				SFG_ERR("failed to serialize cooked font {0}", full_path);
				return cook_result_e::cook_failed;
			}

			return cook_result_e::success;
		}

		cook_result_e cook_shader(const char* full_path, const cooking_options_t& options, ostream_t& stream)
		{
			shader_config_t cfg = {};
			parse_shader_arguments(options.arguments, cfg);

			if (cfg.type == shader_type_e::invalid)
			{
				SFG_ERR("shader cook: missing or invalid shader_type for {0}", full_path);
				return cook_result_e::cook_failed;
			}

			const string_t source = file_system::read_file_as_string(full_path);
			if (source.empty())
			{
				SFG_ERR("shader cook: failed to read source {0}", full_path);
				return cook_result_e::cook_failed;
			}

			const string_t			 directory	   = file_system::get_directory_of_file(full_path);
			const vector_t<string_t> include_paths = directory.empty() ? vector_t<string_t>{} : vector_t<string_t>{directory};

			shader_compile_t compile = {};
			if (!shader_variant_compiler::compile(cfg.type, source, include_paths, compile))
			{
				SFG_ERR("shader cook: variant compile failed for {0}", full_path);
				return cook_result_e::cook_failed;
			}

			if (!shader_cook_serialize(compile, stream))
			{
				SFG_ERR("shader cook: serialize failed for {0}", full_path);
				return cook_result_e::cook_failed;
			}

			return cook_result_e::success;
		}

		cook_result_e cook_material(const char*, const cooking_options_t&, const json&, ostream_t&)
		{
			return cook_result_e::success;
		}

		cook_result_e cook_particle(const char*, const cooking_options_t&, const json&, ostream_t&)
		{
			return cook_result_e::success;
		}

		cook_result_e cook_sampler(const char*, const cooking_options_t&, const json&, ostream_t&)
		{
			return cook_result_e::success;
		}

		cook_result_e cook_physical_material(const char*, const cooking_options_t&, const json&, ostream_t&)
		{
			return cook_result_e::success;
		}

		cook_result_e cook_animation_state_machine(const char*, const cooking_options_t&, const json&, ostream_t&)
		{
			return cook_result_e::success;
		}

		cook_result_e cook_prefab(const char*, const cooking_options_t&, const json&, ostream_t&)
		{
			return cook_result_e::success;
		}

		cook_result_e dispatch_meta(cook_kind_e kind, const char* full_path, const cooking_options_t& options, const json& meta, ostream_t& stream)
		{
			switch (kind)
			{
			case cook_kind_e::material:
				return cook_material(full_path, options, meta, stream);
			case cook_kind_e::particle:
				return cook_particle(full_path, options, meta, stream);
			case cook_kind_e::sampler:
				return cook_sampler(full_path, options, meta, stream);
			case cook_kind_e::physical_material:
				return cook_physical_material(full_path, options, meta, stream);
			case cook_kind_e::animation_state_machine:
				return cook_animation_state_machine(full_path, options, meta, stream);
			case cook_kind_e::prefab:
				return cook_prefab(full_path, options, meta, stream);
			default:
				return cook_result_e::unsupported_schema;
			}
		}

		cook_result_e dispatch_raw(cook_kind_e kind, const char* full_path, const cooking_options_t& options, ostream_t& stream)
		{
			switch (kind)
			{
			case cook_kind_e::texture:
				return cook_texture(full_path, options, stream);
			case cook_kind_e::audio:
				return cook_audio(full_path, options, stream);
			case cook_kind_e::glb:
				return cook_glb(full_path, options, stream);
			case cook_kind_e::font:
				return cook_font(full_path, options, stream);
			case cook_kind_e::shader:
				return cook_shader(full_path, options, stream);
			default:
				return cook_result_e::unsupported_extension;
			}
		}

		cook_kind_e resolve_cook_kind(const char* full_path, json& out_meta, bool& out_has_meta, cook_result_e& out_err)
		{
			out_has_meta	   = false;
			out_err			   = cook_result_e::success;
			const string_t ps  = string_t(full_path);
			const string_t ext = file_system::get_file_extension(ps);

			if (ext == "sfg_meta")
			{
				const string_t contents = file_system::read_file_as_string(full_path);
				if (contents.empty())
				{
					SFG_ERR("meta file is empty or unreadable: {0}", full_path);
					out_err = cook_result_e::invalid_meta_file;
					return cook_kind_e::invalid;
				}

				try
				{
					out_meta = json::parse(contents);
				}
				catch (const json::parse_error& e)
				{
					SFG_ERR("failed to parse meta json {0}: {1}", full_path, e.what());
					out_err = cook_result_e::invalid_meta_file;
					return cook_kind_e::invalid;
				}

				const auto schema_it = out_meta.find("schema");
				if (schema_it == out_meta.end() || !schema_it->is_string())
				{
					SFG_ERR("meta file missing 'schema' string: {0}", full_path);
					out_err = cook_result_e::invalid_meta_file;
					return cook_kind_e::invalid;
				}

				const string_t	  schema = schema_it->get<string_t>();
				const cook_kind_e kind	 = resolve_kind_from_schema(schema);
				if (kind == cook_kind_e::invalid)
				{
					SFG_ERR("unsupported schema '{0}' in {1}", schema.c_str(), full_path);
					out_err = cook_result_e::unsupported_schema;
					return cook_kind_e::invalid;
				}

				out_has_meta = true;
				return kind;
			}

			const cook_kind_e kind = resolve_kind_from_extension(ext);
			if (kind == cook_kind_e::invalid)
			{
				SFG_ERR("unsupported extension '{0}' for {1}", ext.c_str(), full_path);
				out_err = cook_result_e::unsupported_extension;
				return cook_kind_e::invalid;
			}

			return kind;
		}
	}

	cook_result_e cook_resource(const char* full_path, const cooking_options_t& options, ostream_t& stream)
	{
		if (full_path == nullptr)
			return cook_result_e::invalid_path;

		if (!file_system::exists(full_path))
		{
			SFG_ERR("source file does not exist: {0}", full_path);
			return cook_result_e::invalid_path;
		}

		json			  meta;
		bool			  has_meta = false;
		cook_result_e	  err	   = cook_result_e::success;
		const cook_kind_e kind	   = resolve_cook_kind(full_path, meta, has_meta, err);
		if (kind == cook_kind_e::invalid)
			return err;

		if (has_meta)
			return dispatch_meta(kind, full_path, options, meta, stream);

		return dispatch_raw(kind, full_path, options, stream);
	}

	cook_result_e cook_resource(const char* full_path, const char* output_directory, const cooking_options_t& options)
	{
		if (full_path == nullptr || output_directory == nullptr)
			return cook_result_e::invalid_path;

		if (!file_system::exists(full_path))
		{
			SFG_ERR("source file does not exist: {0}", full_path);
			return cook_result_e::invalid_path;
		}

		json			  meta;
		bool			  has_meta = false;
		cook_result_e	  err	   = cook_result_e::success;
		const cook_kind_e kind	   = resolve_cook_kind(full_path, meta, has_meta, err);
		if (kind == cook_kind_e::invalid)
			return err;

		ostream_t			stream;
		const cook_result_e r = has_meta ? dispatch_meta(kind, full_path, options, meta, stream) : dispatch_raw(kind, full_path, options, stream);
		if (r != cook_result_e::success)
			return r;

		const string_t out_path = make_output_path(full_path, output_directory, extension_for_kind(kind));
		if (!serialization::save_to_file(out_path.c_str(), stream))
		{
			SFG_ERR("failed to write cooked resource to {0}", out_path.c_str());
			return cook_result_e::cook_failed;
		}

		return cook_result_e::success;
	}
}
