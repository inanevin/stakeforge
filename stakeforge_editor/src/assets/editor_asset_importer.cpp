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

#include "assets/editor_asset_importer.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_path.hpp"
#include "assets/editor_asset_util.hpp"
#include "assets/editor_asset_cooker.hpp"
#include "assets/editor_asset_writer.hpp"
#include "assets/editor_glb_importer.hpp"
#include "editor_directories.hpp"
#include <sfg/data/string_util.hpp>
#include <sfg/gfx/util/image_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/audio_cook.hpp>
#include <sfg/runtime/resources/cubemap_cook.hpp>
#include <sfg/runtime/resources/texture_cook.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>
#include <sfg/vendor/stb/stb_image_write.h>

namespace sfg
{
	namespace
	{
		editor_asset_import_type_e get_import_type_from_extension(string_t extension)
		{
			string_util::to_lower(extension);
			if (extension == "mp3")
				return editor_asset_import_type_e::audio;
			if (extension == "png" || extension == "jpg" || extension == "jpeg")
				return editor_asset_import_type_e::texture;
			if (extension == "glb")
				return editor_asset_import_type_e::model;
			if (extension == "hdr")
				return editor_asset_import_type_e::cubemap;
			if (extension == "ttf")
				return editor_asset_import_type_e::font;
			return editor_asset_import_type_e::invalid;
		}

		const editor_asset_import_options_t* find_import_options(span_t<const editor_asset_import_options_t> options, editor_asset_import_type_e import_type)
		{
			for (size_t i = 0; i < options.size; ++i)
			{
				const editor_asset_import_options_t& option = options.data[i];
				if (option.type == import_type)
					return &option;
			}
			return nullptr;
		}
	}

	editor_texture_orm_import_sources_reflection_t::editor_texture_orm_import_sources_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name		  = "editor_texture_orm_import_sources_t",
			.display_name = "ORM Sources",
			.fields =
				{
					{.name		   = "occlusion",
					 .display_name = "Occlusion (R)",
					 .tooltip	   = "Texture written to the red occlusion channel.",
					 .sub_type_id  = REFLECTION_SUB_TYPE_IDENTIFIER_PATH,
					 .offset	   = offsetof(editor_texture_orm_import_sources_t, occlusion),
					 .size		   = sizeof(string_t),
					 .type		   = reflected_value_type_e::string},
					{.name		   = "roughness",
					 .display_name = "Roughness (G)",
					 .tooltip	   = "Texture written to the green roughness channel.",
					 .sub_type_id  = REFLECTION_SUB_TYPE_IDENTIFIER_PATH,
					 .offset	   = offsetof(editor_texture_orm_import_sources_t, roughness),
					 .size		   = sizeof(string_t),
					 .type		   = reflected_value_type_e::string},
					{.name		   = "metallic",
					 .display_name = "Metallic (B)",
					 .tooltip	   = "Texture written to the blue metallic channel.",
					 .sub_type_id  = REFLECTION_SUB_TYPE_IDENTIFIER_PATH,
					 .offset	   = offsetof(editor_texture_orm_import_sources_t, metallic),
					 .size		   = sizeof(string_t),
					 .type		   = reflected_value_type_e::string},
				},
			.type_id   = type_id_t<editor_texture_orm_import_sources_t>::value,
			.size	   = sizeof(editor_texture_orm_import_sources_t),
			.alignment = alignof(editor_texture_orm_import_sources_t),
		});
	}

	void editor_asset_import_context_t::report_status(const char* text) const
	{
		if (set_status != nullptr)
			set_status(user_data, text);
	}

	bool editor_asset_importer_t::make_import_options(editor_asset_import_options_t& out_options, const char* asset_name)
	{
		const editor_asset_import_type_e import_type = get_import_type_from_extension(file_system_t::get_file_extension(asset_name));
		out_options									 = {};
		out_options.type							 = import_type;
		switch (import_type)
		{
		case editor_asset_import_type_e::texture:
			out_options.texture_cook_config = {};
			return true;
		case editor_asset_import_type_e::audio:
			out_options.audio_cook_config = {};
			return true;
		case editor_asset_import_type_e::model:
			out_options.glb_cook_config = {};
			return true;
		case editor_asset_import_type_e::cubemap:
			out_options.cubemap_cook_config = {};
			return true;
		case editor_asset_import_type_e::font:
			return true;
		default:
			return false;
		}
	}

	bool editor_asset_importer_t::import_asset(
		const char* target_directory, const char* source_full_path, span_t<const editor_asset_import_options_t> options, const editor_asset_import_context_t& context, vector_t<editor_asset_t>& out_assets, vector_t<string_t>& out_asset_paths)
	{
		const string_t source_path = file_system_t::get_absolute_path(source_full_path);
		if (!file_system_t::exists(source_path.c_str()))
		{
			SFG_ERR("import source does not exist: {0}", source_path.c_str());
			return false;
		}

		string_t extension = file_system_t::get_file_extension(source_path);
		string_util::to_lower(extension);

		editor_asset_import_type_e import_type = get_import_type_from_extension(extension);

		if (extension == "png" && find_import_options(options, editor_asset_import_type_e::sprite) != nullptr)
			import_type = editor_asset_import_type_e::sprite;

		if (import_type == editor_asset_import_type_e::invalid)
		{
			SFG_ERR("unsupported import extension: {0}", extension.c_str());
			return false;
		}

		const editor_asset_import_options_t* import_options = find_import_options(options, import_type);
		if (import_options == nullptr)
		{
			SFG_ERR("missing import options for {0}", source_path.c_str());
			return false;
		}

		if (import_type == editor_asset_import_type_e::model)
			return editor_glb_importer_t::import_glb(target_directory, source_path.c_str(), import_options->glb_cook_config, context, out_assets, out_asset_paths);

		const string_t asset_name = file_system_t::get_filename_from_path(source_path);
		if (!editor_directories_t::is_valid_asset_name(asset_name.c_str()))
		{
			SFG_ERR("invalid imported asset name: {0}", asset_name.c_str());
			return false;
		}

		editor_asset_t asset	  = {};
		string_t	   asset_path = {};

		switch (import_type)
		{
		case editor_asset_import_type_e::texture: {
			string_t status = "Importing texture: ";
			status += asset_name;
			context.report_status(status.c_str());
			const texture_cook_config_t& texture_config = import_options->texture_cook_config;
			nlohmann::json				 cook_options	= nlohmann::json::object();
			if (!reflection_registry_t::get().type_to_json(type_id_t<texture_cook_config_t>::value, const_cast<texture_cook_config_t*>(&texture_config), nullptr, cook_options))
			{
				SFG_ERR("failed to serialize texture import options for {0}", source_path.c_str());
				return false;
			}
			const editor_asset_write_existing_file_desc_t write_desc{
				.cook_options	  = &cook_options,
				.parent_path	  = target_directory,
				.name			  = asset_name.c_str(),
				.source_full_path = source_path.c_str(),
				.asset_type		  = editor_asset_type_e::texture,
			};
			if (!editor_asset_writer_t::write_existing_file_asset(write_desc, &asset, &asset_path))
			{
				SFG_ERR("failed to create imported texture asset {0}", asset_name.c_str());
				return false;
			}
			if (!editor_asset_cooker_t::cook_texture(asset, asset_name.c_str()))
			{
				SFG_ERR("failed to cook imported texture asset {0}", asset.guid);
				return false;
			}
			break;
		}
		case editor_asset_import_type_e::sprite: {
			string_t status = "Importing sprite: ";
			status += asset_name;
			context.report_status(status.c_str());

			const sprite_cook_config_t& sprite_config = import_options->sprite_cook_config;
			nlohmann::json				cook_options  = nlohmann::json::object();

			if (!reflection_registry_t::get().type_to_json(type_id_t<sprite_cook_config_t>::value, const_cast<sprite_cook_config_t*>(&sprite_config), nullptr, cook_options))
			{
				SFG_ERR("failed to serialize sprite import options for {0}", source_path.c_str());
				return false;
			}

			const editor_asset_write_existing_file_desc_t write_desc{
				.cook_options	  = &cook_options,
				.parent_path	  = target_directory,
				.name			  = asset_name.c_str(),
				.source_full_path = source_path.c_str(),
				.asset_type		  = editor_asset_type_e::sprite,
			};

			if (!editor_asset_writer_t::write_existing_file_asset(write_desc, &asset, &asset_path))
			{
				SFG_ERR("failed to create imported sprite asset {0}", asset_name.c_str());
				return false;
			}

			if (!editor_asset_cooker_t::cook_sprite(asset, asset_name.c_str()))
			{
				SFG_ERR("failed to cook imported sprite asset {0}", asset.guid);
				return false;
			}

			break;
		}
		case editor_asset_import_type_e::font: {
			string_t status = "Importing font: ";
			status += asset_name;
			context.report_status(status.c_str());
			const editor_asset_write_existing_file_desc_t write_desc{
				.parent_path	  = target_directory,
				.name			  = asset_name.c_str(),
				.source_full_path = source_path.c_str(),
				.asset_type		  = editor_asset_type_e::font,
			};
			if (!editor_asset_writer_t::write_existing_file_asset(write_desc, &asset, &asset_path))
			{
				SFG_ERR("failed to create imported font asset {0}", asset_name.c_str());
				return false;
			}
			if (!editor_asset_cooker_t::cook_font(asset, asset_name.c_str()))
			{
				SFG_ERR("failed to cook imported font asset {0}", asset.guid);
				return false;
			}
			break;
		}
		case editor_asset_import_type_e::audio: {
			string_t status = "Importing audio: ";
			status += asset_name;
			context.report_status(status.c_str());
			const audio_cook_config_t& audio_config = import_options->audio_cook_config;
			nlohmann::json			   cook_options = nlohmann::json::object();
			if (!reflection_registry_t::get().type_to_json(type_id_t<audio_cook_config_t>::value, const_cast<audio_cook_config_t*>(&audio_config), nullptr, cook_options))
			{
				SFG_ERR("failed to serialize audio import options for {0}", source_path.c_str());
				return false;
			}
			const editor_asset_write_existing_file_desc_t write_desc{
				.cook_options	  = &cook_options,
				.parent_path	  = target_directory,
				.name			  = asset_name.c_str(),
				.source_full_path = source_path.c_str(),
				.asset_type		  = editor_asset_type_e::audio,
			};
			if (!editor_asset_writer_t::write_existing_file_asset(write_desc, &asset, &asset_path))
			{
				SFG_ERR("failed to create imported audio asset {0}", asset_name.c_str());
				return false;
			}
			if (!editor_asset_cooker_t::cook_audio(asset, asset_name.c_str()))
			{
				SFG_ERR("failed to cook imported audio asset {0}", asset.guid);
				return false;
			}
			break;
		}
		case editor_asset_import_type_e::cubemap: {
			string_t status = "Importing cubemap: ";
			status += asset_name;
			context.report_status(status.c_str());

			const cubemap_cook_config_t& cubemap_config = import_options->cubemap_cook_config;
			nlohmann::json				 cook_options	= nlohmann::json::object();

			if (!reflection_registry_t::get().type_to_json(type_id_t<cubemap_cook_config_t>::value, const_cast<cubemap_cook_config_t*>(&cubemap_config), nullptr, cook_options))
			{
				SFG_ERR("failed to serialize cubemap import options for {0}", source_path.c_str());
				return false;
			}

			const editor_asset_write_existing_file_desc_t write_desc{
				.cook_options	  = &cook_options,
				.parent_path	  = target_directory,
				.name			  = asset_name.c_str(),
				.source_full_path = source_path.c_str(),
				.asset_type		  = editor_asset_type_e::cubemap,
			};

			if (!editor_asset_writer_t::write_existing_file_asset(write_desc, &asset, &asset_path))
			{
				SFG_ERR("failed to create imported cubemap asset {0}", asset_name.c_str());
				return false;
			}

			if (!editor_asset_cooker_t::cook_cubemap(asset, asset_name.c_str()))
			{
				SFG_ERR("failed to cook imported cubemap asset {0}", asset.guid);
				return false;
			}

			break;
		}
		default:
			SFG_ERR("unsupported import type: {0}", static_cast<u8>(import_type));
			return false;
		}

		out_assets.push_back(asset);
		out_asset_paths.push_back(asset_path);
		return true;
	}

	bool editor_asset_importer_t::import_texture_orm(
		const char* target_directory, span_t<const string_t> source_paths, const texture_cook_config_t& texture_config, const editor_asset_import_context_t& context, vector_t<editor_asset_t>& out_assets, vector_t<string_t>& out_asset_paths)
	{
		SFG_ASSERT(source_paths.size == 3);

		u8*		  source_pixels[3] = {};
		vec2u16_t source_sizes[3]  = {
			vec2u16_t::zero,
			vec2u16_t::zero,
			vec2u16_t::zero,
		};
		vec2u16_t		output_size	  = vec2u16_t::zero;
		const string_t* first_source  = nullptr;
		bool			sources_valid = true;

		for (u32 i = 0; i < 3; ++i)
		{
			const string_t& source_path = source_paths.data[i];
			if (source_path.empty())
				continue;

			if (!file_system_t::exists(source_path.c_str()) || get_import_type_from_extension(file_system_t::get_file_extension(source_path)) != editor_asset_import_type_e::texture)
			{
				SFG_ERR("invalid ORM texture source: {0}", source_path.c_str());
				sources_valid = false;
				break;
			}

			source_pixels[i] = static_cast<u8*>(image_util_t::load_from_file_ch(source_path.c_str(), source_sizes[i], 4));
			if (source_pixels[i] == nullptr)
			{
				sources_valid = false;
				break;
			}

			if (first_source == nullptr)
			{
				first_source = &source_path;
				output_size	 = source_sizes[i];
			}
			else if (source_sizes[i] != output_size)
			{
				SFG_ERR("ORM texture sources must have matching dimensions: {0}", source_path.c_str());
				sources_valid = false;
				break;
			}
		}

		if (first_source == nullptr)
		{
			SFG_ERR("at least one ORM texture source is required");
			sources_valid = false;
		}

		if (!sources_valid)
		{
			for (u8* pixels : source_pixels)
			{
				if (pixels != nullptr)
					image_util_t::free(pixels);
			}

			return false;
		}

		const size_t pixel_count = static_cast<size_t>(output_size.x) * output_size.y;
		vector_t<u8> pixels		 = {};

		pixels.resize(pixel_count * 4);

		for (size_t i = 0; i < pixel_count; ++i)
		{
			const size_t source_offset = i * 4;
			u8*			 dst		   = pixels.data() + source_offset;

			dst[0] = source_pixels[0] != nullptr ? source_pixels[0][source_offset] : 255;
			dst[1] = source_pixels[1] != nullptr ? source_pixels[1][source_offset] : 255;
			dst[2] = source_pixels[2] != nullptr ? source_pixels[2][source_offset] : 0;
			dst[3] = 255;
		}

		for (u8* source : source_pixels)
		{
			if (source != nullptr)
				image_util_t::free(source);
		}

		string_t asset_name = file_system_t::get_filename_from_path(*first_source);
		asset_name += "_orm";

		if (!editor_directories_t::is_valid_asset_name(asset_name.c_str()))
		{
			SFG_ERR("invalid imported ORM texture asset name: {0}", asset_name.c_str());
			return false;
		}

		string_t status = "Importing ORM texture: ";
		status += asset_name;

		context.report_status(status.c_str());

		texture_cook_config_t output_config = texture_config;
		output_config.size					= output_size;
		output_config.is_linear				= true;
		output_config.force_4_channels		= true;

		nlohmann::json cook_options = nlohmann::json::object();
		if (!reflection_registry_t::get().type_to_json(type_id_t<texture_cook_config_t>::value, &output_config, nullptr, cook_options))
		{
			SFG_ERR("failed to serialize ORM texture import options");
			return false;
		}

		const string_t generated_source_path = editor_asset_path_t::make_source_path(target_directory, asset_name.c_str(), "png");
		if (stbi_write_png(generated_source_path.c_str(), output_size.x, output_size.y, 4, pixels.data(), static_cast<int>(output_size.x) * 4) == 0)
		{
			SFG_ERR("failed to write ORM texture source {0}", generated_source_path.c_str());
			return false;
		}

		editor_asset_t								  asset		 = {};
		string_t									  asset_path = {};
		const editor_asset_write_existing_file_desc_t write_desc{
			.cook_options	  = &cook_options,
			.parent_path	  = target_directory,
			.name			  = asset_name.c_str(),
			.source_full_path = generated_source_path.c_str(),
			.asset_type		  = editor_asset_type_e::texture,
			.source_type	  = editor_asset_source_type_e::file_blob,
		};

		if (!editor_asset_writer_t::write_existing_file_asset(write_desc, &asset, &asset_path))
		{
			SFG_ERR("failed to create imported ORM texture asset {0}", asset_name.c_str());
			return false;
		}

		if (!editor_asset_cooker_t::cook_texture(asset, asset_name.c_str()))
		{
			SFG_ERR("failed to cook imported ORM texture asset {0}", asset.guid);
			return false;
		}

		out_assets.push_back(std::move(asset));
		out_asset_paths.push_back(std::move(asset_path));
		return true;
	}
}
