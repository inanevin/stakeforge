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

#include "assets/editor_glb_importer.hpp"

#include "assets/editor_asset_creator.hpp"
#include "assets/editor_asset_manager.hpp"
#include "editor_directories.hpp"

#include <sfg/common/packing.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/material_def_reflection.hpp>
#include <sfg/runtime/resources/texture_cook.hpp>
#include <sfg/runtime/resources/texture_cook_reflection.hpp>
#include <sfg/serialization/serialization.hpp>

#define TINYGLTF3_IMPLEMENTATION
#include <sfg/vendor/tinygltf/tiny_gltf_v3.h>

#include <limits>

namespace sfg
{
	namespace
	{
		struct glb_texture_transform_t
		{
			u32 tiling = 0;
			u32 offset = 0;
		};

		string_t get_image_extension(const tg3_image& image)
		{
			if (image.uri.data != nullptr && image.uri.len != 0)
			{
				const string_t image_uri(image.uri.data, image.uri.len);
				string_t	   extension = file_system_t::get_file_extension(image_uri);
				string_util::to_lower(extension);
				if (extension == "png" || extension == "jpg" || extension == "jpeg")
					return extension;
			}

			if (image.mime_type.data != nullptr && image.mime_type.len != 0)
			{
				const string_t mime_type(image.mime_type.data, image.mime_type.len);
				if (mime_type == "image/png")
					return "png";
				if (mime_type == "image/jpeg")
					return "jpg";
			}

			return {};
		}

		string_t get_asset_name(const tg3_str& name)
		{
			string_t result;
			if (name.data != nullptr && name.len != 0)
			{
				result.assign(name.data, name.len);
				if (!editor_directories_t::is_valid_asset_name(result.c_str()))
					result.clear();
			}

			return result;
		}

		bool is_glb_string(const tg3_str& str, const char* value)
		{
			return str.data != nullptr && string_view_t(str.data, str.len) == value;
		}

		const tg3_value* find_object_value(const tg3_value& value, const char* key)
		{
			if (value.type != TG3_VALUE_OBJECT)
				return nullptr;

			for (u32 i = 0; i < value.object_count; ++i)
			{
				const tg3_kv_pair& pair = value.object_data[i];
				if (pair.key.data != nullptr && string_view_t(pair.key.data, pair.key.len) == key)
					return &pair.value;
			}

			return nullptr;
		}

		bool get_vec2_value(const tg3_value& value, f32& out_x, f32& out_y)
		{
			if (value.type != TG3_VALUE_ARRAY || value.array_count < 2)
				return false;

			if (value.array_data[0].type != TG3_VALUE_REAL && value.array_data[0].type != TG3_VALUE_INT)
				return false;

			if (value.array_data[1].type != TG3_VALUE_REAL && value.array_data[1].type != TG3_VALUE_INT)
				return false;

			out_x = value.array_data[0].type == TG3_VALUE_REAL ? static_cast<f32>(value.array_data[0].real_val) : static_cast<f32>(value.array_data[0].int_val);
			out_y = value.array_data[1].type == TG3_VALUE_REAL ? static_cast<f32>(value.array_data[1].real_val) : static_cast<f32>(value.array_data[1].int_val);
			return true;
		}

		glb_texture_transform_t get_texture_transform(const tg3_extras_ext& ext)
		{
			glb_texture_transform_t transform = {
				.tiling = packing_t::pack_half2x16(1.0f, 1.0f),
				.offset = 0,
			};

			for (u32 i = 0; i < ext.extensions_count; ++i)
			{
				const tg3_extension& extension = ext.extensions[i];
				if (extension.name.data == nullptr || string_view_t(extension.name.data, extension.name.len) != "KHR_texture_transform")
					continue;

				if (const tg3_value* offset = find_object_value(extension.value, "offset"))
				{
					f32 x = 0.0f;
					f32 y = 0.0f;
					if (get_vec2_value(*offset, x, y))
						transform.offset = packing_t::pack_half2x16(x, y);
				}

				if (const tg3_value* scale = find_object_value(extension.value, "scale"))
				{
					f32 x = 1.0f;
					f32 y = 1.0f;
					if (get_vec2_value(*scale, x, y))
						transform.tiling = packing_t::pack_half2x16(x, y);
				}
			}

			return transform;
		}

		sid_t get_texture_guid(const hash_map_t<u32, sid_t>& texture_guid_map, i32 texture_index, sid_t default_guid)
		{
			if (texture_index < 0)
				return default_guid;

			const auto it = texture_guid_map.find(static_cast<u32>(texture_index));
			return it != texture_guid_map.end() ? it->second : default_guid;
		}

		sid_t get_sampler_guid(const tg3_model& model, const tg3_material& material)
		{
			i32 texture_index = material.pbr_metallic_roughness.base_color_texture.index;
			if (texture_index < 0)
				texture_index = material.normal_texture.index;
			if (texture_index < 0)
				texture_index = material.pbr_metallic_roughness.metallic_roughness_texture.index;
			if (texture_index < 0)
				texture_index = material.emissive_texture.index;

			if (texture_index < 0 || static_cast<u32>(texture_index) >= model.textures_count)
				return DEFAULT_LINEAR_SAMPLER_ASSET_GUID;

			const tg3_texture& texture = model.textures[texture_index];
			if (texture.sampler < 0 || static_cast<u32>(texture.sampler) >= model.samplers_count)
				return DEFAULT_LINEAR_SAMPLER_ASSET_GUID;

			const tg3_sampler& sampler	   = model.samplers[texture.sampler];
			const bool		   min_nearest = sampler.min_filter == TG3_TEXTURE_FILTER_NEAREST || sampler.min_filter == TG3_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST || sampler.min_filter == TG3_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR;
			const bool		   mag_nearest = sampler.mag_filter == TG3_TEXTURE_FILTER_NEAREST;
			return min_nearest || mag_nearest ? DEFAULT_NEAREST_SAMPLER_ASSET_GUID : DEFAULT_LINEAR_SAMPLER_ASSET_GUID;
		}

		material_parameter_t make_vec4_parameter(f32 x, f32 y, f32 z, f32 w)
		{
			return {
				.values = {x, y, z, w},
				.type	= material_parameter_type_e::vec4f,
			};
		}

		material_parameter_t make_uint2_parameter(u32 x, u32 y)
		{
			return {
				.values = {static_cast<f32>(x), static_cast<f32>(y), 0.0f, 0.0f},
				.type	= material_parameter_type_e::uint2,
			};
		}

		bool import_texture(const editor_asset_node_t&		parent_node,
							const char*						source_full_path,
							const tg3_model&				model,
							const tg3_texture&				texture,
							const texture_cook_config_t&	texture_config_base,
							u32								texture_index,
							hash_map_t<u32, sid_t>&			out_texture_guid_map,
							frame_vector_t<editor_asset_t>& out_assets)
		{
			if (texture.source < 0 || static_cast<u32>(texture.source) >= model.images_count)
				return false;

			const tg3_image& image = model.images[texture.source];
			if (image.buffer_view < 0 || static_cast<u32>(image.buffer_view) >= model.buffer_views_count)
				return false;

			const tg3_buffer_view& buffer_view = model.buffer_views[image.buffer_view];
			if (buffer_view.buffer < 0 || static_cast<u32>(buffer_view.buffer) >= model.buffers_count)
				return false;

			const tg3_buffer& buffer = model.buffers[buffer_view.buffer];
			if (buffer.data.data == nullptr || buffer_view.byte_offset > buffer.data.count || buffer_view.byte_length > buffer.data.count - buffer_view.byte_offset || buffer_view.byte_length > static_cast<u64>(std::numeric_limits<size_t>::max()))
				return false;

			const string_t extension = get_image_extension(image);
			if (extension.empty())
				return false;

			texture_cook_config_t texture_config = texture_config_base;

			editor_asset_t asset = {};
			if (!reflection_registry_t::get().serialize_to_json(texture_cook_config_reflection_t::TYPE_ID, &texture_config, asset.cook_options))
				return false;

			string_t asset_name;
			asset_name = get_asset_name(texture.name);

			if (asset_name.empty())
				asset_name = get_asset_name(image.name);

			if (asset_name.empty() && image.uri.data != nullptr && image.uri.len != 0)
			{
				const string_t image_uri(image.uri.data, image.uri.len);
				asset_name = file_system_t::get_filename_from_path(image_uri);
				if (!editor_directories_t::is_valid_asset_name(asset_name.c_str()))
					asset_name.clear();
			}

			if (asset_name.empty())
			{
				asset_name = file_system_t::get_filename_from_path(source_full_path);
				asset_name += "_texture_";
				asset_name += std::to_string(texture_index);
			}

			const string_t source_path = editor_asset_util_t::make_unique_source_path(parent_node.full_path.c_str(), asset_name.c_str(), extension.c_str());
			if (!serializer_t::write_to_file(string_view_t(reinterpret_cast<const char*>(buffer.data.data + buffer_view.byte_offset), static_cast<size_t>(buffer_view.byte_length)), source_path.c_str()))
				return false;

			const string_t asset_path	 = editor_asset_util_t::make_asset_path(parent_node.full_path.c_str(), asset_name.c_str());
			const sid_t	   existing_guid = editor_asset_util_t::try_read_existing_guid(asset_path.c_str());

			asset.version	  = editor_asset_t::VERSION;
			asset.guid		  = existing_guid != NULL_SID ? existing_guid : editor_asset_util_t::generate_unique_asset_guid();
			asset.asset_type  = editor_asset_type_e::texture;
			asset.source_type = editor_asset_source_type_e::file;

			if (!editor_asset_util_t::set_source_relative_or_copy(asset, parent_node.full_path.c_str(), asset_name.c_str(), source_path.c_str()))
				return false;

			if (!editor_asset_util_t::write_asset(asset_path.c_str(), asset))
				return false;

			out_texture_guid_map[texture_index] = asset.guid;
			out_assets.push_back(asset);
			return true;
		}

		bool import_material(
			const editor_asset_node_t& parent_node, const char* source_full_path, const tg3_model& model, const tg3_material& material, u32 material_index, const hash_map_t<u32, sid_t>& texture_guid_map, frame_vector_t<editor_asset_t>& out_assets)
		{
			string_t asset_name = get_asset_name(material.name);
			if (asset_name.empty())
			{
				asset_name = file_system_t::get_filename_from_path(source_full_path);
				asset_name += "_material_";
				asset_name += std::to_string(material_index);
			}

			const i32 base_index	 = material.pbr_metallic_roughness.base_color_texture.index;
			const i32 normal_index	 = material.normal_texture.index;
			const i32 orm_index		 = material.pbr_metallic_roughness.metallic_roughness_texture.index;
			const i32 emissive_index = material.emissive_texture.index;

			const glb_texture_transform_t base_transform	 = get_texture_transform(material.pbr_metallic_roughness.base_color_texture.ext);
			const glb_texture_transform_t normal_transform	 = get_texture_transform(material.normal_texture.ext);
			const glb_texture_transform_t orm_transform		 = get_texture_transform(material.pbr_metallic_roughness.metallic_roughness_texture.ext);
			const glb_texture_transform_t emissive_transform = get_texture_transform(material.emissive_texture.ext);

			const material_def_t material_def = {
				.textures =
					{
						get_texture_guid(texture_guid_map, base_index, DEFAULT_ALBEDO_TEXTURE_ASSET_GUID),
						get_texture_guid(texture_guid_map, normal_index, DEFAULT_NORMAL_TEXTURE_ASSET_GUID),
						get_texture_guid(texture_guid_map, orm_index, DEFAULT_ORM_TEXTURE_ASSET_GUID),
						get_texture_guid(texture_guid_map, emissive_index, DEFAULT_EMISSIVE_TEXTURE_ASSET_GUID),
					},
				.parameters =
					{
						make_vec4_parameter(static_cast<f32>(material.pbr_metallic_roughness.base_color_factor[0]),
											static_cast<f32>(material.pbr_metallic_roughness.base_color_factor[1]),
											static_cast<f32>(material.pbr_metallic_roughness.base_color_factor[2]),
											static_cast<f32>(material.pbr_metallic_roughness.base_color_factor[3])),
						make_vec4_parameter(static_cast<f32>(material.emissive_factor[0]), static_cast<f32>(material.emissive_factor[1]), static_cast<f32>(material.emissive_factor[2]), static_cast<f32>(material.pbr_metallic_roughness.metallic_factor)),
						make_vec4_parameter(static_cast<f32>(material.pbr_metallic_roughness.roughness_factor), static_cast<f32>(material.normal_texture.scale), static_cast<f32>(material.alpha_cutoff), 0.0f),
						make_uint2_parameter(base_transform.tiling, base_transform.offset),
						make_uint2_parameter(normal_transform.tiling, normal_transform.offset),
						make_uint2_parameter(orm_transform.tiling, orm_transform.offset),
						make_uint2_parameter(emissive_transform.tiling, emissive_transform.offset),
					},
				.shader			  = DEFAULT_GBUFFER_SHADER_ASSET_GUID,
				.sampler		  = get_sampler_guid(model, material),
				.pass_flags		  = wpf_gbuffer,
				.double_sided	  = material.double_sided != 0,
				.use_alpha_cutoff = is_glb_string(material.alpha_mode, "MASK"),
			};

			editor_asset_t asset		 = {};
			const string_t asset_path	 = editor_asset_util_t::make_asset_path(parent_node.full_path.c_str(), asset_name.c_str());
			const sid_t	   existing_guid = editor_asset_util_t::try_read_existing_guid(asset_path.c_str());

			asset.version	  = editor_asset_t::VERSION;
			asset.guid		  = existing_guid != NULL_SID ? existing_guid : editor_asset_util_t::generate_unique_asset_guid();
			asset.asset_type  = editor_asset_type_e::material;
			asset.source_type = editor_asset_source_type_e::embedded;
			asset.sub_type	  = static_cast<u8>(editor_material_type_e::gbuffer);
			if (!reflection_registry_t::get().serialize_to_json(material_def_reflection_t::TYPE_ID, &material_def, asset.embedded_source))
				return false;

			if (!editor_asset_util_t::write_asset(asset_path.c_str(), asset))
				return false;

			out_assets.push_back(asset);
			return true;
		}
	}

	bool editor_glb_importer_t::import_glb(editor_asset_node_handle_t directory_node, const char* source_full_path, const glb_cook_config_t& cook_config, frame_vector_t<editor_asset_t>& out_assets)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!directory_node.is_null());
		SFG_ASSERT(tree.is_valid(directory_node));
		const editor_asset_node_t& parent_node = tree.value(directory_node);
		SFG_ASSERT(parent_node.type == editor_asset_node_type_e::folder);
		SFG_ASSERT(!parent_node.full_path.empty());
		SFG_ASSERT(source_full_path != nullptr);
		SFG_ASSERT(source_full_path[0] != '\0');

		char*  glb_data = nullptr;
		size_t glb_size = 0;
		file_system_t::read_file(source_full_path, glb_data, glb_size);
		if (glb_data == nullptr || glb_size == 0)
			return false;

		tg3_model		model  = {};
		tg3_error_stack errors = {};
		tg3_error_stack_init(&errors);

		tg3_parse_options parse_options = {};
		tg3_parse_options_init(&parse_options);

		const string_t		 base_dir	  = file_system_t::get_directory_of_file(source_full_path);
		const tg3_error_code parse_result = tg3_parse_glb(&model, &errors, reinterpret_cast<const u8*>(glb_data), static_cast<u64>(glb_size), base_dir.c_str(), static_cast<u32>(base_dir.size()), &parse_options);
		delete[] glb_data;

		bool result = parse_result == TG3_OK;
		if (result)
		{
			texture_cook_config_t texture_config = {
				.payload_type	  = cook_config.texture_payload_type,
				.generate_mipmaps = cook_config.generate_mipmaps,
			};

			hash_map_t<u32, sid_t> texture_guid_map;
			texture_guid_map.reserve(model.textures_count);

			out_assets.reserve(out_assets.size() + model.textures_count + model.materials_count);
			for (u32 i = 0; i < model.textures_count; ++i)
			{
				if (!import_texture(parent_node, source_full_path, model, model.textures[i], texture_config, i, texture_guid_map, out_assets))
				{
					result = false;
					break;
				}
			}

			if (result)
			{
				for (u32 i = 0; i < model.materials_count; ++i)
				{
					if (!import_material(parent_node, source_full_path, model, model.materials[i], i, texture_guid_map, out_assets))
					{
						result = false;
						break;
					}
				}
			}
		}

		tg3_model_free(&model);
		tg3_error_stack_free(&errors);
		return result;
	}
}
