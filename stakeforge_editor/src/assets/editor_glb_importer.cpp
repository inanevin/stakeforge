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
#include "assets/editor_glb_import_util.hpp"
#include "assets/editor_asset_cooker.hpp"
#include "assets/editor_asset_builtin_types.hpp"
#include "assets/editor_asset_path.hpp"
#include "assets/editor_asset_util.hpp"
#include "assets/editor_asset_writer.hpp"
#include "editor_app.hpp"
#include "editor_directories.hpp"
#include "editor_project.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/math/mat4x3.hpp>
#include <sfg/math/mat4x4.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/animation_def.hpp>
#include <sfg/runtime/resources/material_def.hpp>
#include <sfg/runtime/resources/mesh.hpp>
#include <sfg/runtime/resources/physics_collision_mesh_def.hpp>
#include <sfg/runtime/resources/skeleton.hpp>
#include <sfg/runtime/resources/texture_cook.hpp>
#include <sfg/runtime/resources/world_cook_entity_header.hpp>
#include <sfg/runtime/world/engine_components.hpp>
#include <sfg/serialization/serialization.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>
#include <sfg/vendor/stb/stb_image.h>
#include <sfg/vendor/stb/stb_image_write.h>
#include <sfg/vendor/taskflow/taskflow.hpp>

#define TINYGLTF3_IMPLEMENTATION
#include <sfg/vendor/tinygltf/tiny_gltf_v3.h>

namespace sfg
{
	namespace
	{
		struct glb_texture_transform_t
		{
			f32 tiling[2] = {1.0f, 1.0f};
			f32 offset[2] = {};
		};

		struct glb_texture_import_t
		{
			string_t asset_name;
			u32		 texture_index = 0;
			bool	 is_linear	   = false;
		};

		struct glb_texture_import_result_t
		{
			editor_asset_t asset;
			string_t	   asset_path;
			sid_t		   guid	   = NULL_SID;
			bool		   success = false;
		};

		struct decoded_glb_texture_t
		{
			stbi_uc* pixels = nullptr;
			u32		 width	= 0;
			u32		 height = 0;
		};

		struct glb_asset_name_registry_t
		{
			vector_t<string_t> names;
		};

		void get_node_transform(const tg3_node& node, const glb_basis_conversion_t& basis, vec3f_t& out_pos, quat_t& out_rot, vec3f_t& out_scale);

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

		string_t reserve_glb_asset_name(glb_asset_name_registry_t& registry, const string_t& requested_name)
		{
			string_t candidate = requested_name;
			u32		 suffix	   = 1;
			for (;;)
			{
				string_t candidate_lower = candidate;
				string_util::to_lower(candidate_lower);

				bool used = false;
				for (const string_t& name : registry.names)
				{
					string_t name_lower = name;
					string_util::to_lower(name_lower);
					if (name_lower == candidate_lower)
					{
						used = true;
						break;
					}
				}

				if (!used)
				{
					registry.names.push_back(candidate);
					return candidate;
				}

				candidate = requested_name;
				candidate += "_";
				candidate += std::to_string(suffix++);
			}
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
			glb_texture_transform_t transform = {};

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
					{
						transform.offset[0] = x;
						transform.offset[1] = y;
					}
				}

				if (const tg3_value* scale = find_object_value(extension.value, "scale"))
				{
					f32 x = 1.0f;
					f32 y = 1.0f;
					if (get_vec2_value(*scale, x, y))
					{
						transform.tiling[0] = x;
						transform.tiling[1] = y;
					}
				}
			}

			return transform;
		}

		u64 get_texture_import_key(u32 texture_index, bool is_linear)
		{
			return (static_cast<u64>(texture_index) << 1) | (is_linear ? 1llu : 0llu);
		}

		u64 get_texture_dedupe_key(const tg3_texture& texture, bool is_linear)
		{
			return (static_cast<u64>(static_cast<u32>(texture.source)) << 33) | (static_cast<u64>(static_cast<u32>(texture.sampler + 1)) << 1) | (is_linear ? 1llu : 0llu);
		}

		sid_t get_texture_guid(const hash_map_t<u64, sid_t>& texture_guid_map, i32 texture_index, bool is_linear, sid_t default_guid)
		{
			if (texture_index < 0)
				return default_guid;

			const auto it = texture_guid_map.find(get_texture_import_key(static_cast<u32>(texture_index), is_linear));
			return it != texture_guid_map.end() ? it->second : default_guid;
		}

		string_t get_texture_asset_name(const char* source_full_path, const tg3_model& model, const tg3_texture& texture, u32 texture_index, bool is_linear, bool force_index)
		{
			string_t asset_name = get_asset_name(texture.name);

			if (asset_name.empty())
			{
				const tg3_image& image = model.images[texture.source];
				asset_name			   = get_asset_name(image.name);

				if (asset_name.empty() && image.uri.data != nullptr && image.uri.len != 0)
				{
					const string_t image_uri(image.uri.data, image.uri.len);
					asset_name = file_system_t::get_filename_from_path(image_uri);
					if (!editor_directories_t::is_valid_asset_name(asset_name.c_str()))
						asset_name.clear();
				}
			}

			if (asset_name.empty())
				asset_name = file_system_t::get_filename_from_path(source_full_path);

			if (is_linear)
				asset_name += "_linear";
			if (force_index)
			{
				asset_name += "_";
				asset_name += std::to_string(texture_index);
			}

			return asset_name;
		}

		bool write_blob(const char* path, const u8* data, size_t size)
		{
			ostream_t stream;
			stream.write_raw(data, size);
			if (!serializer_t::save_to_file_compressed(path, stream))
			{
				SFG_ERR("failed to write compressed GLB asset blob {0}", path);
				return false;
			}

			return true;
		}

		void free_decoded_glb_texture(decoded_glb_texture_t& texture)
		{
			if (texture.pixels != nullptr)
				stbi_image_free(texture.pixels);
			texture = {};
		}

		bool decode_glb_texture(const tg3_model& model, u32 texture_index, decoded_glb_texture_t& out)
		{
			if (texture_index >= model.textures_count)
			{
				SFG_ERR("glb texture index is out of range: {0}", texture_index);
				return false;
			}

			const tg3_texture& texture = model.textures[texture_index];
			if (texture.source < 0 || static_cast<u32>(texture.source) >= model.images_count)
			{
				SFG_ERR("glb texture source is out of range: {0}", texture_index);
				return false;
			}

			const tg3_image& image = model.images[texture.source];
			if (image.buffer_view < 0 || static_cast<u32>(image.buffer_view) >= model.buffer_views_count)
			{
				SFG_ERR("glb texture image buffer view is out of range: {0}", texture_index);
				return false;
			}

			const tg3_buffer_view& buffer_view = model.buffer_views[image.buffer_view];
			if (buffer_view.buffer < 0 || static_cast<u32>(buffer_view.buffer) >= model.buffers_count)
			{
				SFG_ERR("glb texture image buffer is out of range: {0}", texture_index);
				return false;
			}

			const tg3_buffer& buffer = model.buffers[buffer_view.buffer];
			if (buffer.data.data == nullptr || buffer_view.byte_offset > buffer.data.count || buffer_view.byte_length > buffer.data.count - buffer_view.byte_offset || buffer_view.byte_length > static_cast<u64>(std::numeric_limits<int>::max()))
			{
				SFG_ERR("glb texture image data is invalid: {0}", texture_index);
				return false;
			}

			int		   decoded_width	= 0;
			int		   decoded_height	= 0;
			int		   decoded_channels = 0;
			stbi_uc*   decoded			= stbi_load_from_memory(buffer.data.data + buffer_view.byte_offset, static_cast<int>(buffer_view.byte_length), &decoded_width, &decoded_height, &decoded_channels, 4);
			const bool decoded_valid	= decoded != nullptr && decoded_width > 0 && decoded_height > 0 && decoded_width <= UINT16_MAX && decoded_height <= UINT16_MAX;
			if (!decoded_valid)
			{
				SFG_ERR("failed to decode GLB texture image: {0}", texture_index);
				if (decoded != nullptr)
					stbi_image_free(decoded);
				return false;
			}

			out.pixels = decoded;
			out.width  = static_cast<u32>(decoded_width);
			out.height = static_cast<u32>(decoded_height);
			return true;
		}

		u8 sample_texture_channel(const decoded_glb_texture_t& texture, u32 x, u32 y, u32 target_width, u32 target_height, u32 channel, u8 fallback)
		{
			if (texture.pixels == nullptr)
				return fallback;

			u32 sx = target_width > 0 ? static_cast<u32>((static_cast<u64>(x) * texture.width) / target_width) : 0;
			u32 sy = target_height > 0 ? static_cast<u32>((static_cast<u64>(y) * texture.height) / target_height) : 0;
			if (sx >= texture.width)
				sx = texture.width - 1;
			if (sy >= texture.height)
				sy = texture.height - 1;
			return texture.pixels[(sy * texture.width + sx) * 4 + channel];
		}

		template <typename T> bool serialize_reflected_to_json(const T& value, nlohmann::json& out)
		{
			reflection_registry_t&	registry = reflection_registry_t::get();
			const reflected_type_t* type	 = registry.find_type(type_id_t<T>::value);
			if (type == nullptr)
				return false;

			nlohmann::json wrapped = nlohmann::json::object();
			if (!registry.type_to_json(type_id_t<T>::value, const_cast<T*>(&value), nullptr, wrapped))
				return false;
			out = wrapped;
			return true;
		}

		template <typename T> bool serialize_reflected_to_stream(const T& value, ostream_t& out)
		{
			reflection_registry_t& registry = reflection_registry_t::get();
			if (registry.find_type(type_id_t<T>::value) == nullptr)
				return false;

			return registry.type_to_stream(type_id_t<T>::value, const_cast<T*>(&value), nullptr, out);
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
				return DEFAULT_LINEAR_SAMPLER_REPEAT_ASSET_GUID;

			const tg3_texture& texture = model.textures[texture_index];
			if (texture.sampler < 0 || static_cast<u32>(texture.sampler) >= model.samplers_count)
				return DEFAULT_LINEAR_SAMPLER_REPEAT_ASSET_GUID;

			const tg3_sampler& sampler	   = model.samplers[texture.sampler];
			const bool		   min_nearest = sampler.min_filter == TG3_TEXTURE_FILTER_NEAREST || sampler.min_filter == TG3_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST || sampler.min_filter == TG3_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR;
			const bool		   mag_nearest = sampler.mag_filter == TG3_TEXTURE_FILTER_NEAREST;
			const bool		   repeat	   = sampler.wrap_s != TG3_TEXTURE_WRAP_CLAMP_TO_EDGE || sampler.wrap_t != TG3_TEXTURE_WRAP_CLAMP_TO_EDGE;

			if (min_nearest || mag_nearest)
				return repeat ? DEFAULT_NEAREST_SAMPLER_REPEAT_ASSET_GUID : DEFAULT_NEAREST_SAMPLER_ASSET_GUID;
			return repeat ? DEFAULT_LINEAR_SAMPLER_REPEAT_ASSET_GUID : DEFAULT_LINEAR_SAMPLER_ASSET_GUID;
		}

		material_texture_value_t make_texture_value(const char* name, resource_handle_t texture)
		{
			return {
				.name	 = name,
				.texture = texture,
			};
		}

		material_sampler_value_t make_sampler_value(const char* name, resource_handle_t sampler)
		{
			return {
				.name	 = name,
				.sampler = sampler,
			};
		}

		material_param_value_t make_vec4_parameter(const char* name, f32 x, f32 y, f32 z, f32 w, shader_param_hint_e hint = shader_param_hint_e::none)
		{
			return {
				.name  = name,
				.value = {x, y, z, w},
				.type  = shader_param_type_e::vec4,
				.hint  = hint,
			};
		}

		material_param_value_t make_f32_parameter(const char* name, f32 value)
		{
			return {
				.name  = name,
				.value = {value, 0.0f, 0.0f, 0.0f},
				.type  = shader_param_type_e::f32,
			};
		}

		bool import_texture(const char*							 target_directory,
							const char*							 source_full_path,
							const tg3_model&					 model,
							const tg3_texture&					 texture,
							const texture_cook_config_t&		 texture_config_base,
							const string_t&						 asset_name,
							u32									 texture_index,
							bool								 is_linear,
							const editor_asset_import_context_t& context,
							editor_asset_t&						 out_asset,
							string_t&							 out_asset_path)
		{
			if (texture.source < 0 || static_cast<u32>(texture.source) >= model.images_count)
			{
				SFG_ERR("glb texture source is out of range: {0}", texture_index);
				return false;
			}

			const tg3_image& image = model.images[texture.source];
			if (image.buffer_view < 0 || static_cast<u32>(image.buffer_view) >= model.buffer_views_count)
			{
				SFG_ERR("glb texture image buffer view is out of range: {0}", texture_index);
				return false;
			}

			const tg3_buffer_view& buffer_view = model.buffer_views[image.buffer_view];
			if (buffer_view.buffer < 0 || static_cast<u32>(buffer_view.buffer) >= model.buffers_count)
			{
				SFG_ERR("glb texture image buffer is out of range: {0}", texture_index);
				return false;
			}

			const tg3_buffer& buffer = model.buffers[buffer_view.buffer];
			if (buffer.data.data == nullptr || buffer_view.byte_offset > buffer.data.count || buffer_view.byte_length > buffer.data.count - buffer_view.byte_offset || buffer_view.byte_length > static_cast<u64>(std::numeric_limits<int>::max()))
			{
				SFG_ERR("glb texture image data is invalid: {0}", texture_index);
				return false;
			}

			string_t status = "Importing texture ";
			status += asset_name;
			status += " (";
			status += std::to_string(texture_index + 1);
			status += "/";
			status += std::to_string(model.textures_count);
			status += ")";
			context.report_status(status.c_str());

			texture_cook_config_t texture_config = texture_config_base;
			texture_config.is_linear			 = is_linear;

			int		   decoded_width	= 0;
			int		   decoded_height	= 0;
			int		   decoded_channels = 0;
			stbi_uc*   decoded			= stbi_load_from_memory(buffer.data.data + buffer_view.byte_offset, static_cast<int>(buffer_view.byte_length), &decoded_width, &decoded_height, &decoded_channels, 4);
			const bool decoded_valid	= decoded != nullptr && decoded_width > 0 && decoded_height > 0 && decoded_width <= UINT16_MAX && decoded_height <= UINT16_MAX;
			if (!decoded_valid)
			{
				SFG_ERR("failed to decode GLB texture image: {0}", texture_index);
				if (decoded != nullptr)
					stbi_image_free(decoded);
				return false;
			}

			texture_config.size = vec2u16_t(static_cast<u16>(decoded_width), static_cast<u16>(decoded_height));

			nlohmann::json cook_options = nlohmann::json::object();
			if (!serialize_reflected_to_json(texture_config, cook_options))
			{
				SFG_ERR("failed to serialize GLB texture cook options: {0}", texture_index);
				stbi_image_free(decoded);
				return false;
			}
			const string_t source_path = editor_asset_path_t::make_source_path(target_directory, asset_name.c_str(), "png");
			if (stbi_write_png(source_path.c_str(), decoded_width, decoded_height, 4, decoded, decoded_width * 4) == 0)
			{
				SFG_ERR("failed to write GLB texture source {0}", source_path.c_str());
				stbi_image_free(decoded);
				return false;
			}

			stbi_image_free(decoded);
			editor_asset_t								  asset = {};
			const editor_asset_write_existing_file_desc_t write_desc{
				.cook_options	  = &cook_options,
				.parent_path	  = target_directory,
				.name			  = asset_name.c_str(),
				.source_full_path = source_path.c_str(),
				.asset_type		  = editor_asset_type_e::texture,
				.source_type	  = editor_asset_source_type_e::file_blob,
			};
			string_t asset_path;
			if (!editor_asset_writer_t::write_existing_file_asset(write_desc, &asset, &asset_path))
			{
				SFG_ERR("failed to write GLB texture asset {0}", asset_name.c_str());
				return false;
			}

			if (!editor_asset_cooker_t::cook_texture(asset, asset_name.c_str()))
			{
				SFG_ERR("failed to cook GLB texture asset {0}", asset.guid);
				return false;
			}

			out_asset	   = std::move(asset);
			out_asset_path = std::move(asset_path);
			return true;
		}

		bool import_orm_texture(const char*							 target_directory,
								const tg3_model&					 model,
								const tg3_material&					 material,
								const texture_cook_config_t&		 texture_config_base,
								const char*							 asset_name_base,
								glb_asset_name_registry_t&			 asset_names,
								const editor_asset_import_context_t& context,
								editor_asset_t&						 out_asset,
								string_t&							 out_asset_path)
		{
			const i32 metallic_roughness_index = material.pbr_metallic_roughness.metallic_roughness_texture.index;
			const i32 occlusion_index		   = material.occlusion_texture.index;
			SFG_ASSERT(metallic_roughness_index >= 0 || occlusion_index >= 0);

			decoded_glb_texture_t metallic_roughness = {};
			decoded_glb_texture_t occlusion			 = {};
			if (metallic_roughness_index >= 0 && !decode_glb_texture(model, static_cast<u32>(metallic_roughness_index), metallic_roughness))
				return false;
			if (occlusion_index >= 0 && !decode_glb_texture(model, static_cast<u32>(occlusion_index), occlusion))
			{
				free_decoded_glb_texture(metallic_roughness);
				return false;
			}

			const u32 width	 = metallic_roughness.pixels != nullptr ? metallic_roughness.width : occlusion.width;
			const u32 height = metallic_roughness.pixels != nullptr ? metallic_roughness.height : occlusion.height;

			vector_t<u8> pixels;
			pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);

			const f32 occlusion_strength = math::clamp(static_cast<f32>(material.occlusion_texture.strength), 0.0f, 1.0f);
			for (u32 y = 0; y < height; ++y)
			{
				for (u32 x = 0; x < width; ++x)
				{
					const u8  occlusion_sample = sample_texture_channel(occlusion, x, y, width, height, 0, 255);
					const f32 occlusion_value  = 1.0f + (static_cast<f32>(occlusion_sample) * (1.0f / 255.0f) - 1.0f) * occlusion_strength;
					u8*		  dst			   = pixels.data() + (static_cast<size_t>(y) * width + x) * 4;
					dst[0]					   = static_cast<u8>(math::clamp(occlusion_value, 0.0f, 1.0f) * 255.0f + 0.5f);
					dst[1]					   = sample_texture_channel(metallic_roughness, x, y, width, height, 1, 255);
					dst[2]					   = sample_texture_channel(metallic_roughness, x, y, width, height, 2, 255);
					dst[3]					   = 255;
				}
			}

			free_decoded_glb_texture(metallic_roughness);
			free_decoded_glb_texture(occlusion);

			string_t asset_name = asset_name_base;
			asset_name += "_orm";
			asset_name = reserve_glb_asset_name(asset_names, asset_name);

			string_t status = "Importing ORM texture ";
			status += asset_name;
			context.report_status(status.c_str());

			texture_cook_config_t texture_config = texture_config_base;
			texture_config.is_linear			 = true;
			texture_config.size					 = vec2u16_t(static_cast<u16>(width), static_cast<u16>(height));

			nlohmann::json cook_options = nlohmann::json::object();
			if (!serialize_reflected_to_json(texture_config, cook_options))
			{
				SFG_ERR("failed to serialize GLB ORM texture cook options");
				return false;
			}

			const string_t source_path = editor_asset_path_t::make_source_path(target_directory, asset_name.c_str(), "png");
			if (stbi_write_png(source_path.c_str(), static_cast<int>(width), static_cast<int>(height), 4, pixels.data(), static_cast<int>(width * 4)) == 0)
			{
				SFG_ERR("failed to write GLB ORM texture source {0}", source_path.c_str());
				return false;
			}

			editor_asset_t								  asset = {};
			const editor_asset_write_existing_file_desc_t write_desc{
				.cook_options	  = &cook_options,
				.parent_path	  = target_directory,
				.name			  = asset_name.c_str(),
				.source_full_path = source_path.c_str(),
				.asset_type		  = editor_asset_type_e::texture,
				.source_type	  = editor_asset_source_type_e::file_blob,
			};
			string_t asset_path;
			if (!editor_asset_writer_t::write_existing_file_asset(write_desc, &asset, &asset_path))
			{
				SFG_ERR("failed to write GLB ORM texture asset {0}", asset_name.c_str());
				return false;
			}

			if (!editor_asset_cooker_t::cook_texture(asset, asset_name.c_str()))
			{
				SFG_ERR("failed to cook GLB ORM texture asset {0}", asset.guid);
				return false;
			}

			out_asset	   = std::move(asset);
			out_asset_path = std::move(asset_path);
			return true;
		}

		bool import_material(const char*						  target_directory,
							 const char*						  source_full_path,
							 const tg3_model&					  model,
							 const tg3_material&				  material,
							 u32								  material_index,
							 const texture_cook_config_t&		  texture_config,
							 bool								  import_textures,
							 const hash_map_t<u64, sid_t>&		  texture_guid_map,
							 hash_map_t<u32, sid_t>&			  material_guid_map,
							 glb_asset_name_registry_t&			  asset_names,
							 const editor_asset_import_context_t& context,
							 vector_t<editor_asset_t>&			  out_assets,
							 vector_t<string_t>&				  out_asset_paths)
		{
			string_t asset_name = get_asset_name(material.name);
			if (asset_name.empty())
			{
				asset_name = file_system_t::get_filename_from_path(source_full_path);
				asset_name += "_material_";
				asset_name += std::to_string(material_index);
			}
			asset_name = reserve_glb_asset_name(asset_names, asset_name);

			string_t status = "Importing material ";
			status += asset_name;
			status += " (";
			status += std::to_string(material_index + 1);
			status += "/";
			status += std::to_string(model.materials_count);
			status += ")";
			context.report_status(status.c_str());

			const i32 base_index	  = material.pbr_metallic_roughness.base_color_texture.index;
			const i32 normal_index	  = material.normal_texture.index;
			const i32 orm_index		  = material.pbr_metallic_roughness.metallic_roughness_texture.index;
			const i32 emissive_index  = material.emissive_texture.index;
			const i32 occlusion_index = material.occlusion_texture.index;

			const glb_texture_transform_t base_transform	 = get_texture_transform(material.pbr_metallic_roughness.base_color_texture.ext);
			const glb_texture_transform_t normal_transform	 = get_texture_transform(material.normal_texture.ext);
			const glb_texture_transform_t orm_transform		 = orm_index >= 0 ? get_texture_transform(material.pbr_metallic_roughness.metallic_roughness_texture.ext) : get_texture_transform(material.occlusion_texture.ext);
			const glb_texture_transform_t emissive_transform = get_texture_transform(material.emissive_texture.ext);

			const bool is_transparent = material.alpha_mode.data != nullptr && string_view_t(material.alpha_mode.data, material.alpha_mode.len) == "BLEND";
			const bool is_cutoff	  = material.alpha_mode.data != nullptr && string_view_t(material.alpha_mode.data, material.alpha_mode.len) == "MASK";
			const f32  ao_multiplier  = import_textures && orm_index >= 0 && orm_index == occlusion_index ? math::clamp(static_cast<f32>(material.occlusion_texture.strength), 0.0f, 1.0f) : 1.0f;

			resource_handle_t orm_guid = DEFAULT_ORM_TEXTURE_ASSET_GUID;
			if (import_textures && orm_index >= 0 && orm_index == occlusion_index)
			{
				orm_guid = get_texture_guid(texture_guid_map, orm_index, true, DEFAULT_ORM_TEXTURE_ASSET_GUID);
			}
			else if (import_textures && (orm_index >= 0 || occlusion_index >= 0))
			{
				editor_asset_t orm_asset = {};
				string_t	   orm_asset_path;
				if (!import_orm_texture(target_directory, model, material, texture_config, asset_name.c_str(), asset_names, context, orm_asset, orm_asset_path))
				{
					SFG_ERR("failed to import GLB ORM texture for material {0}", material_index);
					return false;
				}
				orm_guid = orm_asset.guid;
				out_assets.push_back(std::move(orm_asset));
				out_asset_paths.push_back(std::move(orm_asset_path));
			}
			else
			{
				orm_guid = get_texture_guid(texture_guid_map, orm_index, true, DEFAULT_ORM_TEXTURE_ASSET_GUID);
			}

			const resource_handle_t sampler_guid = get_sampler_guid(model, material);
			const material_def_t	material_def = {
				.textures =
					{
						make_texture_value("albedo", get_texture_guid(texture_guid_map, base_index, false, DEFAULT_ALBEDO_TEXTURE_ASSET_GUID)),
						make_texture_value("normal", get_texture_guid(texture_guid_map, normal_index, true, DEFAULT_NORMAL_TEXTURE_ASSET_GUID)),
						make_texture_value("orm", orm_guid),
						make_texture_value("emissive", get_texture_guid(texture_guid_map, emissive_index, false, DEFAULT_EMISSIVE_TEXTURE_ASSET_GUID)),
					},
				.samplers =
					{
						make_sampler_value("albedo", sampler_guid),
						make_sampler_value("normal", sampler_guid),
						make_sampler_value("orm", sampler_guid),
						make_sampler_value("emissive", sampler_guid),
					},
				.parameters =
					{
						make_vec4_parameter("base_color_factor",
											static_cast<f32>(material.pbr_metallic_roughness.base_color_factor[0]),
											static_cast<f32>(material.pbr_metallic_roughness.base_color_factor[1]),
											static_cast<f32>(material.pbr_metallic_roughness.base_color_factor[2]),
											static_cast<f32>(material.pbr_metallic_roughness.base_color_factor[3]),
											shader_param_hint_e::color),
						make_vec4_parameter("emissive_factor", static_cast<f32>(material.emissive_factor[0]), static_cast<f32>(material.emissive_factor[1]), static_cast<f32>(material.emissive_factor[2]), 1.0f, shader_param_hint_e::color),
						make_f32_parameter("ao_multiplier", ao_multiplier),
						make_f32_parameter("roughness_multiplier", static_cast<f32>(material.pbr_metallic_roughness.roughness_factor)),
						make_f32_parameter("metallic_multiplier", static_cast<f32>(material.pbr_metallic_roughness.metallic_factor)),
						make_f32_parameter("normal_strength", static_cast<f32>(material.normal_texture.scale)),
						make_f32_parameter("emissive_multiplier", 1.0f),
						make_f32_parameter("alpha_cutoff", static_cast<f32>(material.alpha_cutoff)),
						make_vec4_parameter("albedo_tiling_offset", base_transform.tiling[0], base_transform.tiling[1], base_transform.offset[0], base_transform.offset[1], shader_param_hint_e::pack_uint2),
						make_vec4_parameter("normal_tiling_offset", normal_transform.tiling[0], normal_transform.tiling[1], normal_transform.offset[0], normal_transform.offset[1], shader_param_hint_e::pack_uint2),
						make_vec4_parameter("orm_tiling_offset", orm_transform.tiling[0], orm_transform.tiling[1], orm_transform.offset[0], orm_transform.offset[1], shader_param_hint_e::pack_uint2),
						make_vec4_parameter("emissive_tiling_offset", emissive_transform.tiling[0], emissive_transform.tiling[1], emissive_transform.offset[0], emissive_transform.offset[1], shader_param_hint_e::pack_uint2),
					},
				.shader			   = DEFAULT_LIT_SHADER_ASSET_GUID,
				.write_depth	   = !is_transparent,
				.write_shadows	   = !is_transparent,
				.write_reflections = true,
				.is_transparent	   = is_transparent,
				.double_sided	   = material.double_sided != 0,
				.use_alpha_cutoff  = is_cutoff,
			};

			nlohmann::json embedded_source = material_def;

			editor_asset_t							 asset = {};
			const editor_asset_write_embedded_desc_t write_desc{
				.embedded_source = &embedded_source,
				.parent_path	 = target_directory,
				.name			 = asset_name.c_str(),
				.asset_type		 = editor_asset_type_e::material,
				.sub_type		 = static_cast<u8>(is_transparent ? editor_material_type_e::transparent : editor_material_type_e::opaque),
				.allow_overwrite = true,
			};
			string_t asset_path;
			if (!editor_asset_writer_t::write_embedded_asset(write_desc, &asset, &asset_path))
			{
				SFG_ERR("failed to write GLB material asset {0}", asset_name.c_str());
				return false;
			}

			if (!editor_asset_cooker_t::cook_material(asset, asset_name.c_str()))
			{
				SFG_ERR("failed to cook GLB material asset {0}", asset.guid);
				return false;
			}

			material_guid_map[material_index] = asset.guid;
			out_assets.push_back(asset);
			out_asset_paths.push_back(std::move(asset_path));
			return true;
		}

		bool import_skeleton(const char*						  target_directory,
							 const char*						  source_full_path,
							 const tg3_model&					  model,
							 const tg3_skin&					  skin,
							 const glb_basis_conversion_t&		  basis,
							 u32								  skin_index,
							 hash_map_t<u32, sid_t>&			  skeleton_guid_map,
							 glb_asset_name_registry_t&			  asset_names,
							 const editor_asset_import_context_t& context,
							 vector_t<editor_asset_t>&			  out_assets,
							 vector_t<string_t>&				  out_asset_paths)
		{
			string_t asset_name = get_asset_name(skin.name);

			if (asset_name.empty())
			{
				asset_name = file_system_t::get_filename_from_path(source_full_path);
				asset_name += "_skeleton_";
				asset_name += std::to_string(skin_index);
			}

			asset_name = reserve_glb_asset_name(asset_names, asset_name);

			string_t status = "Importing skeleton ";
			status += asset_name;
			status += " (";
			status += std::to_string(skin_index + 1);
			status += "/";
			status += std::to_string(model.skins_count);
			status += ")";
			context.report_status(status.c_str());

			if (skin.joints_count > MAX_SKELETON_BONES)
			{
				SFG_ERR("glb skeleton has too many bones: {0} (maximum is {1})", skin.joints_count, MAX_SKELETON_BONES);
				return false;
			}

			vector_t<u32> node_to_joint_index = {};
			node_to_joint_index.resize(model.nodes_count);

			for (u32& index : node_to_joint_index)
				index = SKELETON_JOINT_NO_PARENT;

			skeleton_def_t skeleton = {
				.name = asset_name,
			};
			skeleton.joints.resize(skin.joints_count);

			for (u32 i = 0; i < skin.joints_count; ++i)
			{
				const i32 node_index = skin.joints[i];

				if (node_index < 0 || static_cast<u32>(node_index) >= model.nodes_count)
				{
					SFG_ERR("glb skeleton joint node is out of range: {0}", node_index);
					return false;
				}

				const u32 node_index_u32 = static_cast<u32>(node_index);

				if (node_to_joint_index[node_index_u32] != SKELETON_JOINT_NO_PARENT)
				{
					SFG_ERR("glb skeleton contains duplicate joint node: {0}", node_index_u32);
					return false;
				}

				node_to_joint_index[node_index_u32] = i;

				const tg3_node& node = model.nodes[node_index_u32];
				string_t		name = node.name.data != nullptr && node.name.len != 0 ? string_t(node.name.data, node.name.len) : string_t();

				if (name.empty())
				{
					name = "joint_";
					name += std::to_string(i);
				}

				skeleton_joint_def_t& joint = skeleton.joints[i];
				joint.name					= name;
				joint.name_hash				= hashing_t::to_sid(name);

				vec3f_t local_pos	= vec3f_t::zero;
				quat_t	local_rot	= quat_t::identity;
				vec3f_t local_scale = vec3f_t::one;
				get_node_transform(node, basis, local_pos, local_rot, local_scale);
				joint.local = mat4x3_t::transform(local_pos, local_rot, local_scale);

				if (!editor_glb_import_util_t::read_inverse_bind_matrix(model, skin, basis, i, joint.inverse_bind))
				{
					SFG_ERR("failed to read GLB inverse bind matrix for joint {0}", i);
					return false;
				}
			}

			for (u32 i = 0; i < skin.joints_count; ++i)
			{
				const tg3_node& node = model.nodes[skin.joints[i]];

				for (u32 child_i = 0; child_i < node.children_count; ++child_i)
				{
					const i32 child_node_index = node.children[child_i];

					if (child_node_index < 0 || static_cast<u32>(child_node_index) >= model.nodes_count)
					{
						SFG_ERR("glb skeleton child node is out of range: {0}", child_node_index);
						return false;
					}

					const u32 child_joint_index = node_to_joint_index[child_node_index];

					if (child_joint_index != SKELETON_JOINT_NO_PARENT)
						skeleton.joints[child_joint_index].parent_index = i;
				}
			}

			if (skin.skeleton >= 0 && static_cast<u32>(skin.skeleton) < model.nodes_count)
				skeleton.root_joint_index = node_to_joint_index[skin.skeleton];

			if (skeleton.root_joint_index == SKELETON_JOINT_NO_PARENT)
			{
				for (u32 i = 0; i < skin.joints_count; ++i)
				{
					if (skeleton.joints[i].parent_index == SKELETON_JOINT_NO_PARENT)
					{
						skeleton.root_joint_index = i;
						break;
					}
				}
			}

			if (skin.joints_count != 0 && skeleton.root_joint_index == SKELETON_JOINT_NO_PARENT)
			{
				SFG_ERR("glb skeleton has no root joint");
				return false;
			}

			if (!skeleton.build_evaluation_order())
			{
				SFG_ERR("glb skeleton hierarchy is invalid: {0}", skin_index);
				return false;
			}

			nlohmann::json embedded_source = nlohmann::json::object();

			if (!serialize_reflected_to_json(skeleton, embedded_source))
			{
				SFG_ERR("failed to serialize GLB skeleton definition: {0}", skin_index);
				return false;
			}

			editor_asset_t							 asset = {};
			const editor_asset_write_embedded_desc_t write_desc{
				.embedded_source = &embedded_source,
				.parent_path	 = target_directory,
				.name			 = asset_name.c_str(),
				.asset_type		 = editor_asset_type_e::skeleton,
				.allow_overwrite = true,
			};
			string_t asset_path = {};

			if (!editor_asset_writer_t::write_embedded_asset(write_desc, &asset, &asset_path))
			{
				SFG_ERR("failed to write GLB skeleton asset {0}", asset_name.c_str());
				return false;
			}

			if (!editor_asset_cooker_t::cook_skeleton(asset, asset_name.c_str()))
			{
				SFG_ERR("failed to cook GLB skeleton asset {0}", asset.guid);
				return false;
			}

			skeleton_guid_map[skin_index] = asset.guid;
			out_assets.push_back(asset);
			out_asset_paths.push_back(std::move(asset_path));
			return true;
		}

		bool import_animation(const char*						   target_directory,
							  const char*						   source_full_path,
							  const tg3_model&					   model,
							  const tg3_animation&				   animation,
							  const glb_basis_conversion_t&		   basis,
							  u32								   animation_index,
							  glb_asset_name_registry_t&		   asset_names,
							  const editor_asset_import_context_t& context,
							  vector_t<editor_asset_t>&			   out_assets,
							  vector_t<string_t>&				   out_asset_paths)
		{
			string_t asset_name = get_asset_name(animation.name);

			if (asset_name.empty())
			{
				asset_name = file_system_t::get_filename_from_path(source_full_path);
				asset_name += "_animation_";
				asset_name += std::to_string(animation_index);
			}

			asset_name = reserve_glb_asset_name(asset_names, asset_name);

			string_t status = "Importing animation ";

			status += asset_name;
			status += " (";
			status += std::to_string(animation_index + 1);
			status += "/";
			status += std::to_string(model.animations_count);
			status += ")";
			context.report_status(status.c_str());

			animation_def_t animation_def{
				.name	   = asset_name,
				.name_hash = hashing_t::to_sid(asset_name),
			};

			if (!editor_glb_import_util_t::import_animation(model, animation, basis, animation_def))
			{
				SFG_ERR("failed to build GLB animation definition {0}", asset_name.c_str());
				return false;
			}

			const string_t blob_path  = editor_asset_path_t::make_blob_path(target_directory, asset_name.c_str());
			ostream_t	   def_stream = {};

			if (!serialize_reflected_to_stream(animation_def, def_stream))
			{
				SFG_ERR("failed to serialize GLB animation definition {0}", asset_name.c_str());
				return false;
			}

			if (!write_blob(blob_path.c_str(), def_stream.get_raw(), def_stream.get_size()))
			{
				SFG_ERR("failed to write GLB animation blob {0}", blob_path.c_str());
				return false;
			}

			editor_asset_t								  asset = {};
			const editor_asset_write_existing_file_desc_t write_desc{
				.parent_path	  = target_directory,
				.name			  = asset_name.c_str(),
				.source_full_path = blob_path.c_str(),
				.asset_type		  = editor_asset_type_e::animation,
				.source_type	  = editor_asset_source_type_e::file_blob,
			};

			string_t asset_path = {};

			if (!editor_asset_writer_t::write_existing_file_asset(write_desc, &asset, &asset_path))
			{
				SFG_ERR("failed to write GLB animation asset {0}", asset_name.c_str());
				return false;
			}

			if (!editor_asset_cooker_t::cook_animation(asset, asset_name.c_str()))
			{
				SFG_ERR("failed to cook GLB animation asset {0}", asset.guid);
				return false;
			}

			out_assets.push_back(asset);
			out_asset_paths.push_back(std::move(asset_path));
			return true;
		}

		void collect_mesh_materials(
			const tg3_model& model, const tg3_mesh* meshes, u32 mesh_count, const hash_map_t<u32, sid_t>& material_guid_map, vector_t<resource_handle_t>& out_materials, vector_t<u32>* out_local_material_indices, u32* out_default_material_index)
		{
			bool		 has_default_material = false;
			vector_t<u8> referenced_materials;
			referenced_materials.resize(model.materials_count);
			if (out_local_material_indices != nullptr)
				out_local_material_indices->resize(model.materials_count, UINT32_MAX);
			for (u32 mesh_i = 0; mesh_i < mesh_count; ++mesh_i)
			{
				const tg3_mesh& mesh = meshes[mesh_i];
				for (u32 primitive_i = 0; primitive_i < mesh.primitives_count; ++primitive_i)
				{
					const tg3_primitive& primitive = mesh.primitives[primitive_i];
					if (primitive.material >= 0 && static_cast<u32>(primitive.material) < model.materials_count)
						referenced_materials[static_cast<u32>(primitive.material)] = 1;
					else
						has_default_material = true;
				}
			}

			for (u32 i = 0; i < model.materials_count; ++i)
			{
				if (referenced_materials[i] == 0)
					continue;

				const auto it = material_guid_map.find(i);
				if (out_local_material_indices != nullptr)
					(*out_local_material_indices)[i] = static_cast<u32>(out_materials.size());
				out_materials.push_back(it != material_guid_map.end() ? it->second : DEFAULT_OPAQUE_MATERIAL_ASSET_GUID);
			}

			if (has_default_material)
			{
				if (out_default_material_index != nullptr)
					*out_default_material_index = static_cast<u32>(out_materials.size());
				out_materials.push_back(DEFAULT_OPAQUE_MATERIAL_ASSET_GUID);
			}
		}

		bool build_mesh_def(const tg3_model& model, const tg3_mesh* meshes, const glb_basis_conversion_t& basis, u32 mesh_count, const hash_map_t<u32, sid_t>& material_guid_map, const char* name, mesh_def_t& out)
		{
			out.name = name;

			vector_t<resource_handle_t> materials			   = {};
			vector_t<u32>				local_material_indices = {};
			u32							default_material_index = UINT32_MAX;
			collect_mesh_materials(model, meshes, mesh_count, material_guid_map, materials, &local_material_indices, &default_material_index);

			bool is_skinned = false;

			for (u32 mesh_i = 0; mesh_i < mesh_count; ++mesh_i)
			{
				const tg3_mesh& mesh = meshes[mesh_i];

				for (u32 primitive_i = 0; primitive_i < mesh.primitives_count; ++primitive_i)
				{
					const tg3_primitive& primitive = mesh.primitives[primitive_i];
					is_skinned					   = is_skinned || editor_glb_import_util_t::find_attribute(primitive, "JOINTS_0") >= 0 || editor_glb_import_util_t::find_attribute(primitive, "WEIGHTS_0") >= 0;
				}
			}

			for (u32 mesh_i = 0; mesh_i < mesh_count; ++mesh_i)
			{
				const tg3_mesh& mesh = meshes[mesh_i];

				for (u32 primitive_i = 0; primitive_i < mesh.primitives_count; ++primitive_i)
				{
					const tg3_primitive& primitive		= mesh.primitives[primitive_i];
					const u32			 material_index = primitive.material >= 0 && static_cast<u32>(primitive.material) < model.materials_count ? local_material_indices[static_cast<u32>(primitive.material)] : default_material_index;
					SFG_ASSERT(material_index != UINT32_MAX);

					if (is_skinned)
					{
						primitive_skinned_def_t primitive_def = {};

						if (!editor_glb_import_util_t::import_skinned_primitive(model, primitive, basis, material_index, primitive_def))
						{
							SFG_ERR("failed to import GLB skinned primitive {0}", primitive_i);
							return false;
						}

						out.skinned_primitives.push_back(std::move(primitive_def));
					}
					else
					{
						primitive_static_def_t primitive_def = {};

						if (!editor_glb_import_util_t::import_static_primitive(model, primitive, basis, material_index, primitive_def))
						{
							SFG_ERR("failed to import GLB static primitive {0}", primitive_i);
							return false;
						}

						out.static_primitives.push_back(std::move(primitive_def));
					}
				}
			}

			vec3f_t bounds_min = {std::numeric_limits<f32>::max(), std::numeric_limits<f32>::max(), std::numeric_limits<f32>::max()};
			vec3f_t bounds_max = {std::numeric_limits<f32>::lowest(), std::numeric_limits<f32>::lowest(), std::numeric_limits<f32>::lowest()};
			bool	has_bounds = false;

			for (const primitive_static_def_t& primitive : out.static_primitives)
			{
				for (const vertex_static_t& vertex : primitive.vertices)
				{
					bounds_min = vec3f_t::min(bounds_min, vertex.pos);
					bounds_max = vec3f_t::max(bounds_max, vertex.pos);
					has_bounds = true;
				}
			}

			for (const primitive_skinned_def_t& primitive : out.skinned_primitives)
			{
				for (const vertex_skinned_t& vertex : primitive.vertices)
				{
					bounds_min = vec3f_t::min(bounds_min, vertex.pos);
					bounds_max = vec3f_t::max(bounds_max, vertex.pos);
					has_bounds = true;
				}
			}

			if (!has_bounds)
			{
				SFG_ERR("glb mesh has no bounds");
				return false;
			}

			out.local_bounds = aabb_t(bounds_min, bounds_max);
			return true;
		}

		string_t make_mesh_asset_base_name(const char* source_full_path, const tg3_model& model, const tg3_mesh* meshes, u32 mesh_count)
		{
			string_t asset_name;
			if (mesh_count == 1)
				asset_name = get_asset_name(meshes[0].name);

			if (asset_name.empty())
			{
				asset_name = file_system_t::get_filename_from_path(source_full_path);
				asset_name += "_mesh";
				if (mesh_count == 1)
				{
					const u32 mesh_index = static_cast<u32>(meshes - model.meshes);
					asset_name += "_";
					asset_name += std::to_string(mesh_index);
				}
			}

			return asset_name;
		}

		bool build_physics_collision_mesh_def(const tg3_model& model, const tg3_mesh* meshes, const glb_basis_conversion_t& basis, u32 mesh_count, physics_collision_mesh_def_t& out)
		{
			for (u32 mesh_i = 0; mesh_i < mesh_count; ++mesh_i)
			{
				const tg3_mesh& mesh = meshes[mesh_i];

				for (u32 primitive_i = 0; primitive_i < mesh.primitives_count; ++primitive_i)
				{
					if (!editor_glb_import_util_t::import_collision_primitive(model, mesh.primitives[primitive_i], basis, out))
					{
						SFG_ERR("failed to import GLB collision primitive {0}", primitive_i);
						return false;
					}
				}
			}

			return !out.vertices.empty() && !out.indices.empty();
		}

		bool import_mesh(const char*						  target_directory,
						 const char*						  source_full_path,
						 const tg3_model&					  model,
						 const tg3_mesh*					  meshes,
						 const glb_basis_conversion_t&		  basis,
						 u32								  mesh_count,
						 const hash_map_t<u32, sid_t>&		  material_guid_map,
						 hash_map_t<u32, sid_t>*			  mesh_guid_map,
						 glb_asset_name_registry_t&			  asset_names,
						 const editor_asset_import_context_t& context,
						 sid_t*								  out_mesh_guid,
						 vector_t<editor_asset_t>&			  out_assets,
						 vector_t<string_t>&				  out_asset_paths)
		{
			string_t asset_name = make_mesh_asset_base_name(source_full_path, model, meshes, mesh_count);
			asset_name			= reserve_glb_asset_name(asset_names, asset_name);

			string_t status = "Importing mesh ";
			status += asset_name;

			if (mesh_count == 1)
			{
				const u32 mesh_index = static_cast<u32>(meshes - model.meshes);
				status += " (";
				status += std::to_string(mesh_index + 1);
				status += "/";
				status += std::to_string(model.meshes_count);
				status += ")";
			}

			context.report_status(status.c_str());

			mesh_def_t mesh_def = {};

			if (!build_mesh_def(model, meshes, basis, mesh_count, material_guid_map, asset_name.c_str(), mesh_def))
			{
				SFG_ERR("failed to build GLB mesh definition {0}", asset_name.c_str());
				return false;
			}

			const string_t blob_path	   = editor_asset_path_t::make_blob_path(target_directory, asset_name.c_str());
			ostream_t	   mesh_def_stream = {};

			if (!serialize_reflected_to_stream(mesh_def, mesh_def_stream))
			{
				SFG_ERR("failed to serialize GLB mesh definition {0}", asset_name.c_str());
				return false;
			}

			if (!write_blob(blob_path.c_str(), mesh_def_stream.get_raw(), mesh_def_stream.get_size()))
			{
				SFG_ERR("failed to write GLB mesh blob {0}", blob_path.c_str());
				return false;
			}

			editor_asset_t								  asset = {};
			const editor_asset_write_existing_file_desc_t write_desc{
				.parent_path	  = target_directory,
				.name			  = asset_name.c_str(),
				.source_full_path = blob_path.c_str(),
				.asset_type		  = editor_asset_type_e::mesh,
				.source_type	  = editor_asset_source_type_e::file_blob,
			};

			string_t asset_path = {};

			if (!editor_asset_writer_t::write_existing_file_asset(write_desc, &asset, &asset_path))
			{
				SFG_ERR("failed to write GLB mesh asset {0}", asset_name.c_str());
				return false;
			}

			if (!editor_asset_cooker_t::cook_mesh(asset, asset_name.c_str()))
			{
				SFG_ERR("failed to cook GLB mesh asset {0}", asset.guid);
				return false;
			}

			if (mesh_guid_map != nullptr && mesh_count == 1)
			{
				const u32 mesh_index		 = static_cast<u32>(meshes - model.meshes);
				(*mesh_guid_map)[mesh_index] = asset.guid;
			}

			if (out_mesh_guid != nullptr)
				*out_mesh_guid = asset.guid;

			out_assets.push_back(asset);
			out_asset_paths.push_back(std::move(asset_path));
			return true;
		}

		bool import_physics_collision_mesh(const char*							target_directory,
										   const char*							source_full_path,
										   const tg3_model&						model,
										   const tg3_mesh*						meshes,
										   const glb_basis_conversion_t&		basis,
										   u32									mesh_count,
										   hash_map_t<u32, sid_t>*				collision_guid_map,
										   glb_asset_name_registry_t&			asset_names,
										   const editor_asset_import_context_t& context,
										   sid_t*								out_collision_guid,
										   vector_t<editor_asset_t>&			out_assets,
										   vector_t<string_t>&					out_asset_paths)
		{
			string_t asset_name = make_mesh_asset_base_name(source_full_path, model, meshes, mesh_count);
			asset_name += "_collision";
			asset_name = reserve_glb_asset_name(asset_names, asset_name);

			string_t status = "Importing collision mesh ";
			status += asset_name;
			context.report_status(status.c_str());

			physics_collision_mesh_def_t collision_def = {};

			if (!build_physics_collision_mesh_def(model, meshes, basis, mesh_count, collision_def))
			{
				SFG_ERR("failed to build GLB physics collision mesh definition {0}", asset_name.c_str());
				return false;
			}

			const string_t blob_path  = editor_asset_path_t::make_blob_path(target_directory, asset_name.c_str());
			ostream_t	   def_stream = {};

			if (!serialize_reflected_to_stream(collision_def, def_stream))
			{
				SFG_ERR("failed to serialize GLB physics collision mesh definition {0}", asset_name.c_str());
				return false;
			}

			if (!write_blob(blob_path.c_str(), def_stream.get_raw(), def_stream.get_size()))
			{
				SFG_ERR("failed to write GLB physics collision mesh blob {0}", blob_path.c_str());
				return false;
			}

			editor_asset_t								  asset = {};
			const editor_asset_write_existing_file_desc_t write_desc{
				.parent_path	  = target_directory,
				.name			  = asset_name.c_str(),
				.source_full_path = blob_path.c_str(),
				.asset_type		  = editor_asset_type_e::physics_collision_mesh,
				.source_type	  = editor_asset_source_type_e::file_blob,
			};

			string_t asset_path = {};

			if (!editor_asset_writer_t::write_existing_file_asset(write_desc, &asset, &asset_path))
			{
				SFG_ERR("failed to write GLB physics collision mesh asset {0}", asset_name.c_str());
				return false;
			}

			if (!editor_asset_cooker_t::cook_physics_collision_mesh(asset, asset_name.c_str()))
			{
				SFG_ERR("failed to cook GLB physics collision mesh asset {0}", asset.guid);
				return false;
			}

			if (collision_guid_map != nullptr && mesh_count == 1)
			{
				const u32 mesh_index			  = static_cast<u32>(meshes - model.meshes);
				(*collision_guid_map)[mesh_index] = asset.guid;
			}

			if (out_collision_guid != nullptr)
				*out_collision_guid = asset.guid;

			out_assets.push_back(asset);
			out_asset_paths.push_back(std::move(asset_path));
			return true;
		}

		void get_node_transform(const tg3_node& node, const glb_basis_conversion_t& basis, vec3f_t& out_pos, quat_t& out_rot, vec3f_t& out_scale)
		{
			if (node.has_matrix)
			{
				const mat4x4_t matrix(static_cast<f32>(node.matrix[0]),
									  static_cast<f32>(node.matrix[1]),
									  static_cast<f32>(node.matrix[2]),
									  static_cast<f32>(node.matrix[3]),
									  static_cast<f32>(node.matrix[4]),
									  static_cast<f32>(node.matrix[5]),
									  static_cast<f32>(node.matrix[6]),
									  static_cast<f32>(node.matrix[7]),
									  static_cast<f32>(node.matrix[8]),
									  static_cast<f32>(node.matrix[9]),
									  static_cast<f32>(node.matrix[10]),
									  static_cast<f32>(node.matrix[11]),
									  static_cast<f32>(node.matrix[12]),
									  static_cast<f32>(node.matrix[13]),
									  static_cast<f32>(node.matrix[14]),
									  static_cast<f32>(node.matrix[15]));
				editor_glb_import_util_t::convert_transform(basis, mat4x3_t::from_matrix4x4(matrix)).decompose(out_pos, out_rot, out_scale);
				return;
			}

			out_pos	  = editor_glb_import_util_t::convert_vector(basis, vec3f_t(static_cast<f32>(node.translation[0]), static_cast<f32>(node.translation[1]), static_cast<f32>(node.translation[2])));
			out_rot	  = editor_glb_import_util_t::convert_rotation(basis, quat_t(static_cast<f32>(node.rotation[0]), static_cast<f32>(node.rotation[1]), static_cast<f32>(node.rotation[2]), static_cast<f32>(node.rotation[3])));
			out_scale = editor_glb_import_util_t::convert_scale(basis, vec3f_t(static_cast<f32>(node.scale[0]), static_cast<f32>(node.scale[1]), static_cast<f32>(node.scale[2])));
		}

		bool import_prefab(const char*							target_directory,
						   const char*							source_full_path,
						   const tg3_model&						model,
						   const glb_basis_conversion_t&		basis,
						   const hash_map_t<u32, sid_t>&		mesh_guid_map,
						   const hash_map_t<u32, sid_t>&		skeleton_guid_map,
						   const hash_map_t<u32, sid_t>&		collision_guid_map,
						   const hash_map_t<u32, sid_t>&		material_guid_map,
						   glb_asset_name_registry_t&			asset_names,
						   const editor_asset_import_context_t& context,
						   sid_t								combined_mesh_guid,
						   sid_t								combined_collision_guid,
						   vector_t<editor_asset_t>&			out_assets,
						   vector_t<string_t>&					out_asset_paths)
		{
			string_t asset_name = file_system_t::get_filename_from_path(source_full_path);
			asset_name += "_prefab";
			asset_name = reserve_glb_asset_name(asset_names, asset_name);

			string_t status = "Importing prefab ";
			status += asset_name;
			context.report_status(status.c_str());

			nlohmann::json prefab_json	  = nlohmann::json::object();
			prefab_json["local_entities"] = nlohmann::json::array();
			prefab_json["components"]	  = nlohmann::json::array();

			entity_guid_t		next_guid = 1;
			const entity_guid_t root_guid = next_guid++;
			string_t			root_name = file_system_t::get_filename_from_path(source_full_path);

			prefab_json["local_entities"].push_back(world_cook_entity_header_t{
				.guid		 = root_guid,
				.parent_guid = NULL_ENTITY_GUID,
				.name		 = root_name,
				.local_pos	 = vec3f_t::zero,
				.local_rot	 = quat_t::identity,
				.local_scale = vec3f_t::one,
				.prefab		 = NULL_RESOURCE_HANDLE,
			});

			vector_t<entity_guid_t> node_guids = {};
			node_guids.resize(model.nodes_count, NULL_ENTITY_GUID);

			vector_t<u32> node_parents = {};
			node_parents.resize(model.nodes_count, UINT32_MAX);

			for (u32 node_index = 0; node_index < model.nodes_count; ++node_index)
			{
				const tg3_node& node = model.nodes[node_index];

				for (u32 child_i = 0; child_i < node.children_count; ++child_i)
				{
					const i32 child_index = node.children[child_i];

					if (child_index >= 0 && static_cast<u32>(child_index) < model.nodes_count)
						node_parents[static_cast<u32>(child_index)] = node_index;
				}
			}

			vector_t<u8> joint_nodes = {};
			joint_nodes.resize(model.nodes_count);

			vector_t<u8> omitted_skin_nodes = {};
			omitted_skin_nodes.resize(model.nodes_count);

			for (u32 skin_index = 0; skin_index < model.skins_count; ++skin_index)
			{
				const tg3_skin& skin = model.skins[skin_index];

				for (u32 joint_index = 0; joint_index < skin.joints_count; ++joint_index)
				{
					const i32 joint_node_index = skin.joints[joint_index];

					if (joint_node_index < 0 || static_cast<u32>(joint_node_index) >= model.nodes_count)
						continue;

					const u32 joint_node_index_u32			 = static_cast<u32>(joint_node_index);
					joint_nodes[joint_node_index_u32]		 = 1;
					omitted_skin_nodes[joint_node_index_u32] = 1;

					const u32 parent_index = node_parents[joint_node_index_u32];

					if (parent_index != UINT32_MAX)
					{
						const tg3_node& parent = model.nodes[parent_index];

						if (parent.mesh < 0 && parent.camera < 0 && parent.light < 0 && parent.emitter < 0)
							omitted_skin_nodes[parent_index] = 1;
					}
				}
			}

			bool prefab_valid = true;

			const auto add_mesh_renderer = [&](entity_guid_t entity_guid, sid_t mesh_guid, const vector_t<resource_handle_t>& materials) -> void {
				nlohmann::json material_json = nlohmann::json::array();

				for (resource_handle_t material : materials)
					material_json.push_back(material);

				prefab_json["components"].push_back({
					{"type", type_id_t<component_mesh_renderer_t>::value},
					{"entity", entity_guid},
					{"data",
					 {
						 {"mesh", mesh_guid},
						 {"materials", material_json},
					 }},
				});
			};

			const auto add_skinned_mesh_renderer = [&](entity_guid_t entity_guid, sid_t mesh_guid, sid_t skeleton_guid, const vector_t<resource_handle_t>& materials) -> void {
				nlohmann::json material_json = nlohmann::json::array();

				for (resource_handle_t material : materials)
					material_json.push_back(material);

				prefab_json["components"].push_back({
					{"type", type_id_t<component_skinned_mesh_renderer_t>::value},
					{"entity", entity_guid},
					{"data",
					 {
						 {"mesh", mesh_guid},
						 {"skeleton", skeleton_guid},
						 {"materials", material_json},
					 }},
				});
			};

			const auto traverse_node = [&](const auto& self, u32 node_index, entity_guid_t parent_guid, const mat4x3_t& omitted_parent_matrix) -> void {
				if (!prefab_valid || node_index >= model.nodes_count || node_guids[node_index] != NULL_ENTITY_GUID)
					return;

				const tg3_node& node = model.nodes[node_index];

				node_guids[node_index] = root_guid;

				if (joint_nodes[node_index] != 0)
					return;

				vec3f_t node_pos   = vec3f_t::zero;
				quat_t	node_rot   = quat_t::identity;
				vec3f_t node_scale = vec3f_t::one;
				get_node_transform(node, basis, node_pos, node_rot, node_scale);

				const mat4x3_t node_matrix = omitted_parent_matrix * mat4x3_t::transform(node_pos, node_rot, node_scale);

				if (omitted_skin_nodes[node_index] != 0)
				{
					for (u32 child_i = 0; child_i < node.children_count; ++child_i)
					{
						const i32 child_index = node.children[child_i];

						if (child_index >= 0)
							self(self, static_cast<u32>(child_index), parent_guid, node_matrix);
					}

					return;
				}

				const entity_guid_t guid = next_guid++;
				node_guids[node_index]	 = guid;

				string_t node_name = get_asset_name(node.name);

				if (node_name.empty())
				{
					node_name = "node_";
					node_name += std::to_string(node_index);
				}

				vec3f_t local_pos	= vec3f_t::zero;
				quat_t	local_rot	= quat_t::identity;
				vec3f_t local_scale = vec3f_t::one;
				node_matrix.decompose(local_pos, local_rot, local_scale);

				prefab_json["local_entities"].push_back(world_cook_entity_header_t{
					.guid		 = guid,
					.parent_guid = parent_guid,
					.name		 = node_name,
					.local_pos	 = local_pos,
					.local_rot	 = local_rot,
					.local_scale = local_scale,
					.prefab		 = NULL_RESOURCE_HANDLE,
				});

				if (node.mesh >= 0 && static_cast<u32>(node.mesh) < model.meshes_count)
				{
					const u32 mesh_index = static_cast<u32>(node.mesh);

					if (!mesh_guid_map.empty())
					{
						const auto mesh_it = mesh_guid_map.find(mesh_index);

						if (mesh_it == mesh_guid_map.end())
						{
							SFG_ERR("failed to find imported GLB mesh {0} for prefab", mesh_index);
							prefab_valid = false;
							return;
						}

						vector_t<resource_handle_t> materials = {};
						collect_mesh_materials(model, model.meshes + mesh_index, 1, material_guid_map, materials, nullptr, nullptr);

						if (materials.size() > 16)
						{
							SFG_ERR("GLB mesh {0} has too many prefab material slots", mesh_index);
							prefab_valid = false;
							return;
						}

						if (node.skin >= 0)
						{
							const u32  skin_index  = static_cast<u32>(node.skin);
							const auto skeleton_it = skeleton_guid_map.find(skin_index);

							if (skin_index >= model.skins_count || skeleton_it == skeleton_guid_map.end())
							{
								SFG_ERR("failed to find imported GLB skeleton {0} for prefab", skin_index);
								prefab_valid = false;
								return;
							}

							add_skinned_mesh_renderer(guid, mesh_it->second, skeleton_it->second, materials);
						}
						else
							add_mesh_renderer(guid, mesh_it->second, materials);
					}
				}

				for (u32 child_i = 0; child_i < node.children_count; ++child_i)
				{
					const i32 child_index = node.children[child_i];

					if (child_index >= 0)
						self(self, static_cast<u32>(child_index), guid, mat4x3_t::identity);
				}
			};

			if (combined_mesh_guid != NULL_SID || combined_collision_guid != NULL_SID)
			{
				vector_t<resource_handle_t> materials = {};

				if (combined_mesh_guid != NULL_SID)
					collect_mesh_materials(model, model.meshes, model.meshes_count, material_guid_map, materials, nullptr, nullptr);

				if (combined_mesh_guid != NULL_SID && materials.size() > 16)
				{
					SFG_ERR("combined GLB mesh has too many prefab material slots");
					prefab_valid = false;
				}
				else
				{
					u32		 mesh_node_count		 = 0;
					u32		 render_node_index		 = UINT32_MAX;
					i32		 combined_skin_index	 = -1;
					mat4x3_t render_node_matrix		 = mat4x3_t::identity;
					bool	 has_skinned_mesh_node	 = false;
					bool	 has_unskinned_mesh_node = false;
					bool	 has_multiple_skins		 = false;

					const auto get_node_matrix = [&](const tg3_node& node) -> mat4x3_t {
						vec3f_t local_pos	= vec3f_t::zero;
						quat_t	local_rot	= quat_t::identity;
						vec3f_t local_scale = vec3f_t::one;
						get_node_transform(node, basis, local_pos, local_rot, local_scale);
						return mat4x3_t::transform(local_pos, local_rot, local_scale);
					};

					const auto scan_render_node = [&](const auto& self, u32 node_index, const mat4x3_t& parent_matrix) -> void {
						if (node_index >= model.nodes_count || node_guids[node_index] != NULL_ENTITY_GUID)
							return;

						node_guids[node_index] = root_guid;

						const tg3_node& node		= model.nodes[node_index];
						const mat4x3_t	node_matrix = parent_matrix * get_node_matrix(node);

						if (node.mesh >= 0 && static_cast<u32>(node.mesh) < model.meshes_count)
						{
							mesh_node_count++;
							render_node_index  = node_index;
							render_node_matrix = node_matrix;

							if (node.skin >= 0)
							{
								has_skinned_mesh_node = true;

								if (combined_skin_index < 0)
									combined_skin_index = node.skin;
								else if (combined_skin_index != node.skin)
									has_multiple_skins = true;
							}
							else
								has_unskinned_mesh_node = true;
						}

						for (u32 child_i = 0; child_i < node.children_count; ++child_i)
						{
							const i32 child_index = node.children[child_i];

							if (child_index >= 0)
								self(self, static_cast<u32>(child_index), node_matrix);
						}
					};

					const tg3_scene* scene = nullptr;

					if (model.default_scene >= 0 && static_cast<u32>(model.default_scene) < model.scenes_count)
						scene = model.scenes + model.default_scene;
					else if (model.scenes_count != 0)
						scene = model.scenes;

					if (scene != nullptr)
					{
						for (u32 i = 0; i < scene->nodes_count; ++i)
						{
							const i32 node_index = scene->nodes[i];

							if (node_index >= 0)
								scan_render_node(scan_render_node, static_cast<u32>(node_index), mat4x3_t::identity);
						}
					}
					else
					{
						vector_t<u8> child_nodes = {};
						child_nodes.resize(model.nodes_count);

						for (u32 i = 0; i < model.nodes_count; ++i)
						{
							const tg3_node& node = model.nodes[i];

							for (u32 child_i = 0; child_i < node.children_count; ++child_i)
							{
								const i32 child_index = node.children[child_i];

								if (child_index >= 0 && static_cast<u32>(child_index) < model.nodes_count)
									child_nodes[static_cast<u32>(child_index)] = 1;
							}
						}

						for (u32 i = 0; i < model.nodes_count; ++i)
						{
							if (child_nodes[i] == 0)
								scan_render_node(scan_render_node, i, mat4x3_t::identity);
						}
					}

					if (combined_mesh_guid != NULL_SID && has_skinned_mesh_node && has_unskinned_mesh_node)
					{
						SFG_ERR("combined GLB mesh cannot mix skinned and unskinned nodes");
						prefab_valid = false;
					}

					if (combined_mesh_guid != NULL_SID && has_multiple_skins)
					{
						SFG_ERR("combined GLB mesh cannot use multiple skins");
						return false;
					}

					node_guids.resize(0);
					node_guids.resize(model.nodes_count, NULL_ENTITY_GUID);

					string_t mesh_name = root_name;
					mesh_name += "_mesh";

					if (mesh_node_count == 1)
					{
						mesh_name = get_asset_name(model.nodes[render_node_index].name);

						if (mesh_name.empty())
						{
							mesh_name = "node_";
							mesh_name += std::to_string(render_node_index);
						}
					}

					vec3f_t local_pos	= vec3f_t::zero;
					quat_t	local_rot	= quat_t::identity;
					vec3f_t local_scale = vec3f_t::one;

					if (mesh_node_count == 1)
						render_node_matrix.decompose(local_pos, local_rot, local_scale);

					const entity_guid_t mesh_guid = next_guid++;
					prefab_json["local_entities"].push_back(world_cook_entity_header_t{
						.guid		 = mesh_guid,
						.parent_guid = root_guid,
						.name		 = mesh_name,
						.local_pos	 = local_pos,
						.local_rot	 = local_rot,
						.local_scale = local_scale,
						.prefab		 = NULL_RESOURCE_HANDLE,
					});

					if (combined_mesh_guid != NULL_SID && combined_skin_index >= 0)
					{
						const u32  skin_index  = static_cast<u32>(combined_skin_index);
						const auto skeleton_it = skeleton_guid_map.find(skin_index);

						if (skin_index >= model.skins_count || skeleton_it == skeleton_guid_map.end())
						{
							SFG_ERR("failed to find imported GLB skeleton {0} for combined prefab", skin_index);
							return false;
						}

						add_skinned_mesh_renderer(mesh_guid, combined_mesh_guid, skeleton_it->second, materials);
					}
					else if (combined_mesh_guid != NULL_SID)
						add_mesh_renderer(mesh_guid, combined_mesh_guid, materials);
				}
			}
			else
			{
				const tg3_scene* scene = nullptr;

				if (model.default_scene >= 0 && static_cast<u32>(model.default_scene) < model.scenes_count)
					scene = model.scenes + model.default_scene;
				else if (model.scenes_count != 0)
					scene = model.scenes;

				if (scene != nullptr)
				{
					for (u32 i = 0; i < scene->nodes_count; ++i)
					{
						const i32 node_index = scene->nodes[i];

						if (node_index >= 0)
							traverse_node(traverse_node, static_cast<u32>(node_index), root_guid, mat4x3_t::identity);
					}
				}
				else
				{
					vector_t<u8> child_nodes = {};
					child_nodes.resize(model.nodes_count);

					for (u32 i = 0; i < model.nodes_count; ++i)
					{
						const tg3_node& node = model.nodes[i];

						for (u32 child_i = 0; child_i < node.children_count; ++child_i)
						{
							const i32 child_index = node.children[child_i];

							if (child_index >= 0 && static_cast<u32>(child_index) < model.nodes_count)
								child_nodes[static_cast<u32>(child_index)] = 1;
						}
					}

					for (u32 i = 0; i < model.nodes_count; ++i)
					{
						if (child_nodes[i] == 0)
							traverse_node(traverse_node, i, root_guid, mat4x3_t::identity);
					}
				}
			}

			if (!prefab_valid)
				return false;

			editor_asset_t							 asset = {};
			const editor_asset_write_embedded_desc_t write_desc{
				.embedded_source = &prefab_json,
				.parent_path	 = target_directory,
				.name			 = asset_name.c_str(),
				.asset_type		 = editor_asset_type_e::prefab,
				.allow_overwrite = true,
			};

			string_t asset_path = {};

			if (!editor_asset_writer_t::write_embedded_asset(write_desc, &asset, &asset_path))
			{
				SFG_ERR("failed to write GLB prefab asset {0}", asset_name.c_str());
				return false;
			}

			if (!editor_asset_cooker_t::cook_prefab(asset, asset_name.c_str()))
			{
				SFG_ERR("failed to cook GLB prefab asset {0}", asset.guid);
				return false;
			}

			out_assets.push_back(asset);
			out_asset_paths.push_back(std::move(asset_path));
			return true;
		}
	}

	bool editor_glb_importer_t::import_glb(
		const char* target_directory, const char* source_full_path, const glb_cook_config_t& cook_config, const editor_asset_import_context_t& context, vector_t<editor_asset_t>& out_assets, vector_t<string_t>& out_asset_paths)
	{
		if (!cook_config.is_basis_valid())
		{
			SFG_ERR("GLB source up and forward axes must use different dimensions");
			return false;
		}

		const glb_basis_conversion_t basis = editor_glb_import_util_t::make_basis(cook_config.source_up_axis, cook_config.source_forward_axis);

		editor_asset_t glb_source_asset = {};
		const string_t glb_asset_name	= file_system_t::get_filename_from_path(source_full_path);
		string_t	   status			= "Copying GLB ";
		status += glb_asset_name;
		context.report_status(status.c_str());

		if (!editor_asset_path_t::set_source_relative_or_copy(glb_source_asset, target_directory, glb_asset_name.c_str(), source_full_path))
		{
			SFG_ERR("failed to copy GLB source {0}", source_full_path);
			return false;
		}

		status = "Reading GLB ";
		status += glb_asset_name;
		context.report_status(status.c_str());

		char*  glb_data = nullptr;
		size_t glb_size = 0;
		file_system_t::read_file(source_full_path, glb_data, glb_size);

		if (glb_data == nullptr || glb_size == 0)
		{
			SFG_ERR("failed to read GLB file {0}", source_full_path);
			return false;
		}

		tg3_model		model  = {};
		tg3_error_stack errors = {};
		tg3_error_stack_init(&errors);

		tg3_parse_options parse_options = {};
		tg3_parse_options_init(&parse_options);

		status = "Parsing GLB ";
		status += glb_asset_name;
		context.report_status(status.c_str());

		const string_t		 base_dir	  = file_system_t::get_directory_of_file(source_full_path);
		const tg3_error_code parse_result = tg3_parse_glb(&model, &errors, reinterpret_cast<const u8*>(glb_data), static_cast<u64>(glb_size), base_dir.c_str(), static_cast<u32>(base_dir.size()), &parse_options);
		delete[] glb_data;

		bool result = parse_result == TG3_OK;

		if (!result)
			SFG_ERR("failed to parse GLB file {0}: {1}", source_full_path, static_cast<u32>(parse_result));

		if (result)
		{
			const texture_cook_config_t texture_config = {
				.payload_type	  = cook_config.texture_payload_type,
				.ktx2_compression = cook_config.ktx2_compression,
				.generate_mipmaps = cook_config.generate_mipmaps,
			};

			const bool import_skeletons = cook_config.import_animations || (cook_config.import_meshes && model.skins_count != 0);

			const u32 reserve_texture_count	  = cook_config.import_textures ? model.textures_count * 2 : 0;
			const u32 reserve_skeleton_count  = import_skeletons ? model.skins_count : 0;
			const u32 reserve_animation_count = cook_config.import_animations ? model.animations_count : 0;
			const u32 reserve_material_count  = cook_config.import_materials ? model.materials_count : 0;
			const u32 reserve_mesh_count	  = cook_config.import_meshes ? (cook_config.combine_meshes ? 1 : model.meshes_count) : 0;
			const u32 reserve_collision_count = cook_config.import_collisions ? (cook_config.combine_meshes ? 1 : model.meshes_count) : 0;

			hash_map_t<u64, sid_t> texture_guid_map = {};
			texture_guid_map.reserve(model.textures_count * 2);
			hash_map_t<u64, u32> imported_texture_indices = {};
			imported_texture_indices.reserve(model.textures_count);
			hash_map_t<u64, u32> texture_import_indices = {};
			texture_import_indices.reserve(model.textures_count * 2);
			vector_t<glb_texture_import_t> texture_imports = {};
			texture_imports.reserve(reserve_texture_count);
			glb_asset_name_registry_t asset_names = {};

			asset_names.names.reserve(reserve_texture_count + reserve_skeleton_count + reserve_animation_count + reserve_material_count + reserve_mesh_count + reserve_collision_count + 1);
			out_assets.reserve(out_assets.size() + reserve_texture_count + reserve_skeleton_count + reserve_animation_count + reserve_material_count + reserve_mesh_count + reserve_collision_count);
			out_asset_paths.reserve(out_asset_paths.size() + reserve_texture_count + reserve_skeleton_count + reserve_animation_count + reserve_material_count + reserve_mesh_count + reserve_collision_count);

			if (cook_config.import_textures)
			{
				auto push_texture_import = [&](i32 texture_index, bool is_linear) {
					if (texture_index < 0)
						return;

					const u32 texture_index_u32 = static_cast<u32>(texture_index);

					if (texture_index_u32 >= model.textures_count)
					{
						SFG_ERR("glb texture index is out of range: {0}", texture_index);
						result = false;
						return;
					}

					const tg3_texture& texture = model.textures[texture_index_u32];

					if (texture.source < 0 || static_cast<u32>(texture.source) >= model.images_count)
					{
						SFG_ERR("glb texture source is out of range: {0}", texture_index_u32);
						result = false;
						return;
					}

					const u64 logical_key = get_texture_import_key(texture_index_u32, is_linear);

					if (texture_import_indices.find(logical_key) != texture_import_indices.end())
						return;

					const u64  dedupe_key		   = get_texture_dedupe_key(texture, is_linear);
					const auto imported_texture_it = imported_texture_indices.find(dedupe_key);

					if (imported_texture_it != imported_texture_indices.end())
					{
						texture_import_indices[logical_key] = imported_texture_it->second;
						return;
					}

					const u32 import_index				 = static_cast<u32>(texture_imports.size());
					imported_texture_indices[dedupe_key] = import_index;
					texture_import_indices[logical_key]	 = import_index;
					texture_imports.push_back({.texture_index = texture_index_u32, .is_linear = is_linear});
				};

				for (u32 i = 0; i < model.materials_count; ++i)
				{
					const tg3_material& material = model.materials[i];
					push_texture_import(material.pbr_metallic_roughness.base_color_texture.index, false);
					push_texture_import(material.normal_texture.index, true);
					push_texture_import(material.emissive_texture.index, false);

					if (material.pbr_metallic_roughness.metallic_roughness_texture.index >= 0 && material.pbr_metallic_roughness.metallic_roughness_texture.index == material.occlusion_texture.index)
						push_texture_import(material.pbr_metallic_roughness.metallic_roughness_texture.index, true);

					if (!result)
						break;
				}

				if (result && !cook_config.import_materials)
				{
					for (u32 i = 0; i < model.textures_count; ++i)
						push_texture_import(static_cast<i32>(i), false);
				}
			}

			if (result && !texture_imports.empty())
			{
				vector_t<string_t> texture_asset_base_names = {};
				texture_asset_base_names.reserve(texture_imports.size());

				for (const glb_texture_import_t& texture_import : texture_imports)
				{
					const tg3_texture& texture = model.textures[texture_import.texture_index];
					texture_asset_base_names.push_back(get_texture_asset_name(source_full_path, model, texture, texture_import.texture_index, texture_import.is_linear, false));
				}

				for (u32 i = 0; i < texture_imports.size(); ++i)
				{
					bool duplicate_name = false;

					for (u32 j = 0; j < texture_asset_base_names.size(); ++j)
					{
						if (i != j && texture_asset_base_names[i] == texture_asset_base_names[j])
						{
							duplicate_name = true;
							break;
						}
					}

					const glb_texture_import_t& texture_import = texture_imports[i];
					const tg3_texture&			texture		   = model.textures[texture_import.texture_index];
					const string_t				requested_name = duplicate_name ? get_texture_asset_name(source_full_path, model, texture, texture_import.texture_index, texture_import.is_linear, true) : texture_asset_base_names[i];
					texture_imports[i].asset_name			   = reserve_glb_asset_name(asset_names, requested_name);
				}

				string_t status = "Importing textures 0/";
				status += std::to_string(texture_imports.size());
				context.report_status(status.c_str());

				vector_t<glb_texture_import_result_t> texture_import_results = {};
				texture_import_results.resize(texture_imports.size());
				std::atomic<u32> textures_finished = 0;

				tf::Taskflow texture_import_flow = {};

				for (u32 import_index = 0; import_index < texture_imports.size(); ++import_index)
				{
					texture_import_flow.emplace([&, import_index]() {
						const glb_texture_import_t&			texture_import	= texture_imports[import_index];
						glb_texture_import_result_t&		import_result	= texture_import_results[import_index];
						const tg3_texture&					texture			= model.textures[texture_import.texture_index];
						const editor_asset_import_context_t texture_context = {};

						import_result.success =
							import_texture(target_directory, source_full_path, model, texture, texture_config, texture_import.asset_name, texture_import.texture_index, texture_import.is_linear, texture_context, import_result.asset, import_result.asset_path);

						if (import_result.success)
							import_result.guid = import_result.asset.guid;

						const u32 finished		 = textures_finished.fetch_add(1, std::memory_order_relaxed) + 1;
						string_t  texture_status = "Importing textures ";
						texture_status += std::to_string(finished);
						texture_status += "/";
						texture_status += std::to_string(texture_imports.size());
						context.report_status(texture_status.c_str());
					});
				}

				tf::Executor& editor_work_executor = editor_app_t::get().get_editor_work_executor();

				if (editor_work_executor.this_worker() != nullptr)
					editor_work_executor.corun(texture_import_flow);
				else
					editor_work_executor.run(texture_import_flow).wait();

				for (const glb_texture_import_result_t& import_result : texture_import_results)
					result = import_result.success && result;

				if (!result)
					SFG_ERR("failed to import GLB textures from {0}", source_full_path);

				if (result)
				{
					for (const auto& it : texture_import_indices)
					{
						const u32 import_index = it.second;
						SFG_ASSERT(import_index < texture_import_results.size());
						texture_guid_map[it.first] = texture_import_results[import_index].guid;
					}

					for (glb_texture_import_result_t& import_result : texture_import_results)
					{
						SFG_ASSERT(import_result.success);
						out_assets.push_back(std::move(import_result.asset));
						out_asset_paths.push_back(std::move(import_result.asset_path));
					}
				}
			}

			hash_map_t<u32, sid_t> material_guid_map = {};
			material_guid_map.reserve(model.materials_count);

			if (result && cook_config.import_materials)
			{
				for (u32 i = 0; i < model.materials_count; ++i)
				{
					if (!import_material(target_directory, source_full_path, model, model.materials[i], i, texture_config, cook_config.import_textures, texture_guid_map, material_guid_map, asset_names, context, out_assets, out_asset_paths))
					{
						SFG_ERR("failed to import GLB material {0}", i);
						result = false;
						break;
					}
				}
			}

			hash_map_t<u32, sid_t> skeleton_guid_map = {};
			skeleton_guid_map.reserve(model.skins_count);

			if (result && import_skeletons)
			{
				for (u32 i = 0; i < model.skins_count; ++i)
				{
					if (!import_skeleton(target_directory, source_full_path, model, model.skins[i], basis, i, skeleton_guid_map, asset_names, context, out_assets, out_asset_paths))
					{
						SFG_ERR("failed to import GLB skeleton {0}", i);
						result = false;
						break;
					}
				}
			}

			if (result && cook_config.import_animations)
			{
				for (u32 i = 0; result && i < model.animations_count; ++i)
				{
					if (!import_animation(target_directory, source_full_path, model, model.animations[i], basis, i, asset_names, context, out_assets, out_asset_paths))
					{
						SFG_ERR("failed to import GLB animation {0}", i);
						result = false;
					}
				}
			}

			if (result && (cook_config.import_meshes || cook_config.import_collisions) && model.meshes_count != 0)
			{
				hash_map_t<u32, sid_t> mesh_guid_map	  = {};
				hash_map_t<u32, sid_t> collision_guid_map = {};
				mesh_guid_map.reserve(model.meshes_count);
				collision_guid_map.reserve(model.meshes_count);

				if (cook_config.combine_meshes)
				{
					sid_t combined_mesh_guid	  = NULL_SID;
					sid_t combined_collision_guid = NULL_SID;

					if (cook_config.import_meshes)
						result = import_mesh(target_directory, source_full_path, model, model.meshes, basis, model.meshes_count, material_guid_map, nullptr, asset_names, context, &combined_mesh_guid, out_assets, out_asset_paths);

					if (!result && cook_config.import_meshes)
						SFG_ERR("failed to import combined GLB mesh");

					if (result && cook_config.import_collisions)
						result = import_physics_collision_mesh(target_directory, source_full_path, model, model.meshes, basis, model.meshes_count, nullptr, asset_names, context, &combined_collision_guid, out_assets, out_asset_paths);

					if (!result && cook_config.import_collisions)
						SFG_ERR("failed to import combined GLB collision mesh");

					if (result)
					{
						if (!import_prefab(
								target_directory, source_full_path, model, basis, mesh_guid_map, skeleton_guid_map, collision_guid_map, material_guid_map, asset_names, context, combined_mesh_guid, combined_collision_guid, out_assets, out_asset_paths))
						{
							SFG_ERR("failed to import GLB prefab");
							result = false;
						}
					}
				}
				else
				{
					for (u32 i = 0; i < model.meshes_count; ++i)
					{
						if (cook_config.import_meshes && !import_mesh(target_directory, source_full_path, model, model.meshes + i, basis, 1, material_guid_map, &mesh_guid_map, asset_names, context, nullptr, out_assets, out_asset_paths))
						{
							SFG_ERR("failed to import GLB mesh {0}", i);
							result = false;
							break;
						}
						if (cook_config.import_collisions && !import_physics_collision_mesh(target_directory, source_full_path, model, model.meshes + i, basis, 1, &collision_guid_map, asset_names, context, nullptr, out_assets, out_asset_paths))
						{
							SFG_ERR("failed to import GLB collision mesh {0}", i);
							result = false;
							break;
						}
					}
				}

				if (result && !cook_config.combine_meshes && model.nodes_count != 0)
				{
					if (!import_prefab(target_directory, source_full_path, model, basis, mesh_guid_map, skeleton_guid_map, collision_guid_map, material_guid_map, asset_names, context, NULL_SID, NULL_SID, out_assets, out_asset_paths))
					{
						SFG_ERR("failed to import GLB prefab");
						result = false;
					}
				}
			}
		}

		tg3_model_free(&model);
		tg3_error_stack_free(&errors);
		return result;
	}

	bool glb_cook_config_t::is_basis_valid() const
	{
		const u8 up_axis	  = static_cast<u8>(source_up_axis);
		const u8 forward_axis = static_cast<u8>(source_forward_axis);
		const u8 max_axis	  = static_cast<u8>(glb_axis_e::negative_z);

		if (up_axis > max_axis || forward_axis > max_axis)
			return false;

		return up_axis / 2 != forward_axis / 2;
	}

	glb_axis_reflection_t::glb_axis_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "glb_axis_e",
			.fields =
				{
					{.name = "positive_x", .display_name = "+X"},
					{.name = "negative_x", .display_name = "-X"},
					{.name = "positive_y", .display_name = "+Y"},
					{.name = "negative_y", .display_name = "-Y"},
					{.name = "positive_z", .display_name = "+Z"},
					{.name = "negative_z", .display_name = "-Z"},
				},
			.type_id   = type_id_t<glb_axis_e>::value,
			.size	   = sizeof(glb_axis_e),
			.alignment = alignof(glb_axis_e),
			.flags	   = reflected_type_flag_enum,
		});
	}

	glb_cook_config_reflection_t::glb_cook_config_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "glb_cook_config_t",
			.fields =
				{
					{.name = "source_up_axis", .display_name = "Source Up Axis", .sub_type_id = type_id_t<glb_axis_e>::value, .offset = offsetof(glb_cook_config_t, source_up_axis), .size = sizeof(glb_axis_e), .type = reflected_value_type_e::u8},
					{.name = "source_forward_axis", .display_name = "Source Forward Axis", .sub_type_id = type_id_t<glb_axis_e>::value, .offset = offsetof(glb_cook_config_t, source_forward_axis), .size = sizeof(glb_axis_e), .type = reflected_value_type_e::u8},
					{.name = "import_textures", .display_name = "Import Textures", .offset = offsetof(glb_cook_config_t, import_textures), .size = sizeof(bool), .type = reflected_value_type_e::boolean},
					{.name = "import_materials", .display_name = "Import Materials", .offset = offsetof(glb_cook_config_t, import_materials), .size = sizeof(bool), .type = reflected_value_type_e::boolean},
					{.name = "import_animations", .display_name = "Import Animations", .offset = offsetof(glb_cook_config_t, import_animations), .size = sizeof(bool), .type = reflected_value_type_e::boolean},
					{.name = "import_meshes", .display_name = "Import Meshes", .offset = offsetof(glb_cook_config_t, import_meshes), .size = sizeof(bool), .type = reflected_value_type_e::boolean},
					{.name = "import_collisions", .display_name = "Import Collisions", .offset = offsetof(glb_cook_config_t, import_collisions), .size = sizeof(bool), .type = reflected_value_type_e::boolean},
					{.name		   = "texture_payload_type",
					 .display_name = "Texture Payload Type",
					 .sub_type_id  = type_id_t<texture_payload_type_e>::value,
					 .offset	   = offsetof(glb_cook_config_t, texture_payload_type),
					 .size		   = sizeof(texture_payload_type_e),
					 .type		   = reflected_value_type_e::u8},
					{.name		   = "ktx2_compression",
					 .display_name = "KTX2 Compression",
					 .sub_type_id  = type_id_t<texture_ktx2_compression_e>::value,
					 .offset	   = offsetof(glb_cook_config_t, ktx2_compression),
					 .size		   = sizeof(texture_ktx2_compression_e),
					 .type		   = reflected_value_type_e::u8},
					{.name = "generate_mipmaps", .display_name = "Generate Mipmaps", .offset = offsetof(glb_cook_config_t, generate_mipmaps), .size = sizeof(bool), .type = reflected_value_type_e::boolean},
					{.name = "combine_meshes", .display_name = "Combine Meshes", .offset = offsetof(glb_cook_config_t, combine_meshes), .size = sizeof(bool), .type = reflected_value_type_e::boolean},
				},
			.type_id   = type_id_t<glb_cook_config_t>::value,
			.size	   = sizeof(glb_cook_config_t),
			.alignment = alignof(glb_cook_config_t),
		});
	}
}
