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

#include "assets/editor_asset_cooker.hpp"
#include "assets/editor_asset_builtin_types.hpp"
#include "assets/editor_asset_manager.hpp"
#include "editor_app.hpp"
#include "editor_directories.hpp"
#include "editor_project.hpp"

#include <sfg/common/hashing.hpp>
#include <sfg/common/packing.hpp>
#include <sfg/data/hash_map.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/data/string_view.hpp>
#include <sfg/data/string_util.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/resources/material_def.hpp>
#include <sfg/runtime/resources/mesh.hpp>
#include <sfg/runtime/resources/skeleton.hpp>
#include <sfg/runtime/resources/texture_cook.hpp>
#include <sfg/serialization/serialization.hpp>
#include <sfg/vendor/stb/stb_image.h>
#include <sfg/vendor/stb/stb_image_write.h>
#include <sfg/vendor/taskflow/taskflow.hpp>

#define TINYGLTF3_IMPLEMENTATION
#include <sfg/vendor/tinygltf/tiny_gltf_v3.h>

#include <atomic>
#include <cstring>
#include <iterator>
#include <limits>
#include <utility>

namespace sfg
{
	namespace
	{
		struct glb_texture_transform_t
		{
			u32 tiling = 0;
			u32 offset = 0;
		};

		struct glb_texture_import_t
		{
			u32 texture_index = 0;
		};

		struct glb_texture_import_result_t
		{
			editor_asset_t asset;
			sid_t		   guid	   = NULL_SID;
			bool		   success = false;
		};

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

		string_t get_glb_name(const tg3_str& name)
		{
			return name.data != nullptr && name.len != 0 ? string_t(name.data, name.len) : string_t();
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

		bool write_blob(const char* path, const u8* data, size_t size)
		{
			SFG_ASSERT(path != nullptr);
			SFG_ASSERT(path[0] != '\0');
			SFG_ASSERT(data != nullptr);
			SFG_ASSERT(size != 0);
			ostream_t stream;
			stream.write_raw(data, size);
			return serializer_t::save_to_file_compressed(path, stream);
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

		mat4x3_t make_mat4x3_from_glb_matrix(const f32* matrix)
		{
			return mat4x3_t(matrix[0], matrix[1], matrix[2], matrix[4], matrix[5], matrix[6], matrix[8], matrix[9], matrix[10], matrix[12], matrix[13], matrix[14]);
		}

		bool read_inverse_bind_matrix(const tg3_model& model, const tg3_skin& skin, u32 joint_index, mat4x3_t& out_matrix)
		{
			out_matrix = mat4x3_t::identity;
			if (skin.inverse_bind_matrices < 0)
				return true;

			if (static_cast<u32>(skin.inverse_bind_matrices) >= model.accessors_count)
				return false;

			const tg3_accessor& accessor = model.accessors[skin.inverse_bind_matrices];
			if (accessor.component_type != TG3_COMPONENT_TYPE_FLOAT || accessor.type != TG3_TYPE_MAT4 || accessor.count <= joint_index || accessor.buffer_view < 0 || accessor.sparse.is_sparse != 0)
				return false;

			if (static_cast<u32>(accessor.buffer_view) >= model.buffer_views_count)
				return false;

			const tg3_buffer_view& buffer_view = model.buffer_views[accessor.buffer_view];
			if (buffer_view.buffer < 0 || static_cast<u32>(buffer_view.buffer) >= model.buffers_count)
				return false;

			const tg3_buffer& buffer = model.buffers[buffer_view.buffer];
			if (buffer.data.data == nullptr)
				return false;

			const i32 stride = tg3_accessor_byte_stride(&accessor, &buffer_view);
			if (stride < static_cast<i32>(sizeof(f32) * 16))
				return false;

			const u64 matrix_offset = buffer_view.byte_offset + accessor.byte_offset + static_cast<u64>(stride) * joint_index;
			if (matrix_offset > buffer.data.count || sizeof(f32) * 16 > buffer.data.count - matrix_offset)
				return false;

			f32 matrix[16] = {};
			std::memcpy(matrix, buffer.data.data + matrix_offset, sizeof(matrix));
			out_matrix = make_mat4x3_from_glb_matrix(matrix);
			return true;
		}

		i32 find_attribute(const tg3_primitive& primitive, const char* name)
		{
			for (u32 i = 0; i < primitive.attributes_count; ++i)
			{
				const tg3_str_int_pair& attr = primitive.attributes[i];
				if (attr.key.data != nullptr && string_view_t(attr.key.data, attr.key.len) == name)
					return attr.value;
			}
			return -1;
		}

		const u8* get_accessor_data(const tg3_model& model, const tg3_accessor& accessor, const tg3_buffer_view*& out_buffer_view)
		{
			if (accessor.buffer_view < 0 || static_cast<u32>(accessor.buffer_view) >= model.buffer_views_count || accessor.sparse.is_sparse != 0)
				return nullptr;

			const tg3_buffer_view& buffer_view = model.buffer_views[accessor.buffer_view];
			if (buffer_view.buffer < 0 || static_cast<u32>(buffer_view.buffer) >= model.buffers_count)
				return nullptr;

			const tg3_buffer& buffer = model.buffers[buffer_view.buffer];
			const u64		  offset = buffer_view.byte_offset + accessor.byte_offset;
			if (buffer.data.data == nullptr || offset > buffer.data.count)
				return nullptr;

			out_buffer_view = &buffer_view;
			return buffer.data.data + offset;
		}

		const tg3_accessor* get_accessor(const tg3_model& model, i32 accessor_index)
		{
			if (accessor_index < 0 || static_cast<u32>(accessor_index) >= model.accessors_count)
				return nullptr;

			return &model.accessors[accessor_index];
		}

		bool read_float_attribute(const tg3_model& model, i32 accessor_index, i32 type, u32 expected_count, vector_t<f32>& out)
		{
			const tg3_accessor* accessor = get_accessor(model, accessor_index);
			if (accessor == nullptr || accessor->component_type != TG3_COMPONENT_TYPE_FLOAT || accessor->type != type || accessor->count != expected_count)
				return false;

			const tg3_buffer_view* buffer_view = nullptr;
			const u8*			   data		   = get_accessor_data(model, *accessor, buffer_view);
			if (data == nullptr)
				return false;

			const i32 components = tg3_num_components(type);
			const i32 stride	 = tg3_accessor_byte_stride(accessor, buffer_view);
			if (components <= 0 || stride < components * static_cast<i32>(sizeof(f32)))
				return false;

			out.resize(static_cast<size_t>(expected_count) * static_cast<size_t>(components));
			for (u32 i = 0; i < expected_count; ++i)
				SFG_MEMCPY(out.data() + static_cast<size_t>(i) * components, data + static_cast<size_t>(i) * stride, static_cast<size_t>(components) * sizeof(f32));

			return true;
		}

		bool read_indices(const tg3_model& model, i32 accessor_index, u32 vertex_count, vector_t<primitive_index>& out)
		{
			if (accessor_index < 0)
			{
				out.resize(vertex_count);
				for (u32 i = 0; i < vertex_count; ++i)
					out[i] = i;
				return true;
			}

			const tg3_accessor* accessor = get_accessor(model, accessor_index);
			if (accessor == nullptr || accessor->type != TG3_TYPE_SCALAR || accessor->count > UINT32_MAX)
				return false;

			const tg3_buffer_view* buffer_view = nullptr;
			const u8*			   data		   = get_accessor_data(model, *accessor, buffer_view);
			if (data == nullptr)
				return false;

			const i32 component_size = tg3_component_size(accessor->component_type);
			const i32 stride		 = tg3_accessor_byte_stride(accessor, buffer_view);
			if (component_size <= 0 || stride < component_size)
				return false;

			const u32 count = static_cast<u32>(accessor->count);
			out.resize(count);
			for (u32 i = 0; i < count; ++i)
			{
				const u8* src = data + static_cast<size_t>(i) * stride;
				switch (accessor->component_type)
				{
				case TG3_COMPONENT_TYPE_UNSIGNED_BYTE:
					out[i] = *src;
					break;
				case TG3_COMPONENT_TYPE_UNSIGNED_SHORT:
					out[i] = *reinterpret_cast<const u16*>(src);
					break;
				case TG3_COMPONENT_TYPE_UNSIGNED_INT:
					out[i] = *reinterpret_cast<const u32*>(src);
					break;
				default:
					return false;
				}
			}

			return true;
		}

		bool read_joint_attribute(const tg3_model& model, i32 accessor_index, u32 vertex_count, vector_t<vec4u_t>& out)
		{
			const tg3_accessor* accessor = get_accessor(model, accessor_index);
			if (accessor == nullptr || accessor->type != TG3_TYPE_VEC4 || accessor->count != vertex_count)
				return false;

			const tg3_buffer_view* buffer_view = nullptr;
			const u8*			   data		   = get_accessor_data(model, *accessor, buffer_view);
			if (data == nullptr)
				return false;

			const i32 component_size = tg3_component_size(accessor->component_type);
			const i32 stride		 = tg3_accessor_byte_stride(accessor, buffer_view);
			if (component_size <= 0 || stride < component_size * 4)
				return false;

			out.resize(vertex_count);
			for (u32 i = 0; i < vertex_count; ++i)
			{
				const u8* src = data + static_cast<size_t>(i) * stride;
				switch (accessor->component_type)
				{
				case TG3_COMPONENT_TYPE_UNSIGNED_BYTE:
					out[i] = {src[0], src[1], src[2], src[3]};
					break;
				case TG3_COMPONENT_TYPE_UNSIGNED_SHORT: {
					const u16* joints = reinterpret_cast<const u16*>(src);
					out[i]			  = {joints[0], joints[1], joints[2], joints[3]};
					break;
				}
				case TG3_COMPONENT_TYPE_UNSIGNED_INT: {
					const u32* joints = reinterpret_cast<const u32*>(src);
					out[i]			  = {joints[0], joints[1], joints[2], joints[3]};
					break;
				}
				default:
					return false;
				}
			}

			return true;
		}

		u32 get_primitive_vertex_count(const tg3_model& model, const tg3_primitive& primitive)
		{
			const tg3_accessor* accessor = get_accessor(model, find_attribute(primitive, "POSITION"));
			if (accessor == nullptr || accessor->count > UINT32_MAX)
				return 0;

			return static_cast<u32>(accessor->count);
		}

		bool import_static_primitive(const tg3_model& model, const tg3_primitive& primitive, u32 material_index, primitive_static_def_t& out)
		{
			if (primitive.mode != TG3_MODE_TRIANGLES)
				return false;

			const u32 vertex_count = get_primitive_vertex_count(model, primitive);
			if (vertex_count == 0)
				return false;

			vector_t<f32> positions;
			if (!read_float_attribute(model, find_attribute(primitive, "POSITION"), TG3_TYPE_VEC3, vertex_count, positions))
				return false;

			vector_t<f32> normals;
			const i32	  normal_accessor = find_attribute(primitive, "NORMAL");
			if (normal_accessor >= 0 && !read_float_attribute(model, normal_accessor, TG3_TYPE_VEC3, vertex_count, normals))
				return false;

			vector_t<f32> tangents;
			const i32	  tangent_accessor = find_attribute(primitive, "TANGENT");
			if (tangent_accessor >= 0 && !read_float_attribute(model, tangent_accessor, TG3_TYPE_VEC4, vertex_count, tangents))
				return false;

			vector_t<f32> uvs;
			const i32	  uv_accessor = find_attribute(primitive, "TEXCOORD_0");
			if (uv_accessor >= 0 && !read_float_attribute(model, uv_accessor, TG3_TYPE_VEC2, vertex_count, uvs))
				return false;

			out.vertices.resize(vertex_count);
			out.material_index = material_index;
			for (u32 i = 0; i < vertex_count; ++i)
			{
				vertex_static_t& vertex = out.vertices[i];
				vertex.pos				= {positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]};
				if (!normals.empty())
					vertex.normal = {normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]};
				if (!tangents.empty())
					vertex.tangent = {tangents[i * 4], tangents[i * 4 + 1], tangents[i * 4 + 2], tangents[i * 4 + 3]};
				if (!uvs.empty())
					vertex.uv = {uvs[i * 2], uvs[i * 2 + 1]};
			}

			return read_indices(model, primitive.indices, vertex_count, out.indices);
		}

		bool import_skinned_primitive(const tg3_model& model, const tg3_primitive& primitive, u32 material_index, primitive_skinned_def_t& out)
		{
			if (primitive.mode != TG3_MODE_TRIANGLES)
				return false;

			const u32 vertex_count = get_primitive_vertex_count(model, primitive);
			if (vertex_count == 0)
				return false;

			vector_t<f32> positions;
			if (!read_float_attribute(model, find_attribute(primitive, "POSITION"), TG3_TYPE_VEC3, vertex_count, positions))
				return false;

			vector_t<f32> normals;
			const i32	  normal_accessor = find_attribute(primitive, "NORMAL");
			if (normal_accessor >= 0 && !read_float_attribute(model, normal_accessor, TG3_TYPE_VEC3, vertex_count, normals))
				return false;

			vector_t<f32> tangents;
			const i32	  tangent_accessor = find_attribute(primitive, "TANGENT");
			if (tangent_accessor >= 0 && !read_float_attribute(model, tangent_accessor, TG3_TYPE_VEC4, vertex_count, tangents))
				return false;

			vector_t<f32> uvs;
			const i32	  uv_accessor = find_attribute(primitive, "TEXCOORD_0");
			if (uv_accessor >= 0 && !read_float_attribute(model, uv_accessor, TG3_TYPE_VEC2, vertex_count, uvs))
				return false;

			vector_t<f32> weights;
			const i32	  weights_accessor = find_attribute(primitive, "WEIGHTS_0");
			if (weights_accessor >= 0 && !read_float_attribute(model, weights_accessor, TG3_TYPE_VEC4, vertex_count, weights))
				return false;

			vector_t<vec4u_t> joints;
			const i32		  joints_accessor = find_attribute(primitive, "JOINTS_0");
			if (joints_accessor >= 0 && !read_joint_attribute(model, joints_accessor, vertex_count, joints))
				return false;

			out.vertices.resize(vertex_count);
			out.material_index = material_index;
			for (u32 i = 0; i < vertex_count; ++i)
			{
				vertex_skinned_t& vertex = out.vertices[i];
				vertex.pos				 = {positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]};
				if (!normals.empty())
					vertex.normal = {normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]};
				if (!tangents.empty())
					vertex.tangent = {tangents[i * 4], tangents[i * 4 + 1], tangents[i * 4 + 2], tangents[i * 4 + 3]};
				if (!uvs.empty())
					vertex.uv = {uvs[i * 2], uvs[i * 2 + 1]};
				if (!weights.empty())
					vertex.bone_weights = {weights[i * 4], weights[i * 4 + 1], weights[i * 4 + 2], weights[i * 4 + 3]};
				if (!joints.empty())
					vertex.bone_indices = joints[i];
			}

			return read_indices(model, primitive.indices, vertex_count, out.indices);
		}

		bool import_texture(const editor_asset_node_t&			 parent_node,
							const char*							 source_full_path,
							const tg3_model&					 model,
							const tg3_texture&					 texture,
							const texture_cook_config_t&		 texture_config_base,
							u32									 texture_index,
							const editor_asset_import_context_t& context,
							editor_asset_t&						 out_asset)
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
			if (buffer.data.data == nullptr || buffer_view.byte_offset > buffer.data.count || buffer_view.byte_length > buffer.data.count - buffer_view.byte_offset || buffer_view.byte_length > static_cast<u64>(std::numeric_limits<int>::max()))
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

			string_t status = "Importing texture ";
			status += asset_name;
			status += " (";
			status += std::to_string(texture_index + 1);
			status += "/";
			status += std::to_string(model.textures_count);
			status += ")";
			context.report_status(status.c_str());

			texture_cook_config_t texture_config = texture_config_base;

			int		   decoded_width	= 0;
			int		   decoded_height	= 0;
			int		   decoded_channels = 0;
			stbi_uc*   decoded			= stbi_load_from_memory(buffer.data.data + buffer_view.byte_offset, static_cast<int>(buffer_view.byte_length), &decoded_width, &decoded_height, &decoded_channels, 4);
			const bool decoded_valid	= decoded != nullptr && decoded_width > 0 && decoded_height > 0 && decoded_width <= UINT16_MAX && decoded_height <= UINT16_MAX;
			if (!decoded_valid)
			{
				if (decoded != nullptr)
					stbi_image_free(decoded);
				return false;
			}

			texture_config.size = vec2u16_t(static_cast<u16>(decoded_width), static_cast<u16>(decoded_height));

			editor_asset_t asset = {};
			if (!reflection_registry_t::get().serialize_to_json(type_id_t<texture_cook_config_t>::value, &texture_config, asset.cook_options))
			{
				stbi_image_free(decoded);
				return false;
			}

			const string_t asset_path	 = editor_asset_util_t::make_asset_path(parent_node.full_path.c_str(), asset_name.c_str());
			const string_t source_path	 = editor_asset_util_t::make_source_path(parent_node.full_path.c_str(), asset_name.c_str(), "png");
			const sid_t	   existing_guid = editor_asset_util_t::try_read_existing_guid(asset_path.c_str());
			if (stbi_write_png(source_path.c_str(), decoded_width, decoded_height, 4, decoded, decoded_width * 4) == 0)
			{
				stbi_image_free(decoded);
				return false;
			}

			stbi_image_free(decoded);
			asset.version		  = editor_asset_t::VERSION;
			asset.guid			  = existing_guid != NULL_SID ? existing_guid : editor_asset_util_t::generate_unique_asset_guid();
			asset.asset_type	  = editor_asset_type_e::texture;
			asset.source_type	  = editor_asset_source_type_e::file;
			asset.source_relative = editor_asset_util_t::get_source_relative(editor_project_t::get()._runtime.assets_path.c_str(), source_path.c_str());

			if (!editor_asset_util_t::write_asset(asset_path.c_str(), asset))
				return false;

			if (!editor_asset_cooker_t::cook_texture(asset))
				return false;

			out_asset = std::move(asset);
			return true;
		}

		bool import_material(const editor_asset_node_t&			  parent_node,
							 const char*						  source_full_path,
							 const tg3_model&					  model,
							 const tg3_material&				  material,
							 u32								  material_index,
							 const hash_map_t<u32, sid_t>&		  texture_guid_map,
							 hash_map_t<u32, sid_t>&			  material_guid_map,
							 const editor_asset_import_context_t& context,
							 vector_t<editor_asset_t>&			  out_assets)
		{
			string_t asset_name = get_asset_name(material.name);
			if (asset_name.empty())
			{
				asset_name = file_system_t::get_filename_from_path(source_full_path);
				asset_name += "_material_";
				asset_name += std::to_string(material_index);
			}

			string_t status = "Importing material ";
			status += asset_name;
			status += " (";
			status += std::to_string(material_index + 1);
			status += "/";
			status += std::to_string(model.materials_count);
			status += ")";
			context.report_status(status.c_str());

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
			if (!reflection_registry_t::get().serialize_to_json(type_id_t<material_def_t>::value, &material_def, asset.embedded_source))
				return false;

			if (!editor_asset_util_t::write_asset(asset_path.c_str(), asset))
				return false;

			if (!editor_asset_cooker_t::cook_material(asset))
				return false;

			material_guid_map[material_index] = asset.guid;
			out_assets.push_back(asset);
			return true;
		}

		bool import_skeleton(const editor_asset_node_t& parent_node, const char* source_full_path, const tg3_model& model, const tg3_skin& skin, u32 skin_index, const editor_asset_import_context_t& context, vector_t<editor_asset_t>& out_assets)
		{
			if (skin.joints_count > skeleton_loader_t::MAX_JOINTS)
				return false;

			string_t asset_name = get_asset_name(skin.name);
			if (asset_name.empty())
			{
				asset_name = file_system_t::get_filename_from_path(source_full_path);
				asset_name += "_skeleton_";
				asset_name += std::to_string(skin_index);
			}

			string_t status = "Importing skeleton ";
			status += asset_name;
			status += " (";
			status += std::to_string(skin_index + 1);
			status += "/";
			status += std::to_string(model.skins_count);
			status += ")";
			context.report_status(status.c_str());

			vector_t<u32> node_to_joint_index;
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
					return false;

				const u32 node_index_u32 = static_cast<u32>(node_index);
				if (node_to_joint_index[node_index_u32] != SKELETON_JOINT_NO_PARENT)
					return false;

				node_to_joint_index[node_index_u32] = i;

				const tg3_node& node = model.nodes[node_index_u32];
				string_t		name = get_glb_name(node.name);
				if (name.empty())
				{
					name = "joint_";
					name += std::to_string(i);
				}

				skeleton_joint_def_t& joint = skeleton.joints[i];
				joint.name					= name;
				joint.name_hash				= hashing_t::to_sid(name);
				if (!read_inverse_bind_matrix(model, skin, i, joint.inverse_bind))
					return false;
			}

			for (u32 i = 0; i < skin.joints_count; ++i)
			{
				const tg3_node& node = model.nodes[skin.joints[i]];
				for (u32 child_i = 0; child_i < node.children_count; ++child_i)
				{
					const i32 child_node_index = node.children[child_i];
					if (child_node_index < 0 || static_cast<u32>(child_node_index) >= model.nodes_count)
						return false;

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
				return false;

			editor_asset_t asset		 = {};
			const string_t asset_path	 = editor_asset_util_t::make_asset_path(parent_node.full_path.c_str(), asset_name.c_str());
			const sid_t	   existing_guid = editor_asset_util_t::try_read_existing_guid(asset_path.c_str());

			asset.version	  = editor_asset_t::VERSION;
			asset.guid		  = existing_guid != NULL_SID ? existing_guid : editor_asset_util_t::generate_unique_asset_guid();
			asset.asset_type  = editor_asset_type_e::skeleton;
			asset.source_type = editor_asset_source_type_e::embedded;
			if (!reflection_registry_t::get().serialize_to_json(type_id_t<skeleton_def_t>::value, &skeleton, asset.embedded_source))
				return false;

			if (!editor_asset_util_t::write_asset(asset_path.c_str(), asset))
				return false;

			if (!editor_asset_cooker_t::cook_skeleton(asset))
				return false;

			out_assets.push_back(asset);
			return true;
		}

		bool build_mesh_def(const tg3_model& model, const tg3_mesh* meshes, u32 mesh_count, const hash_map_t<u32, sid_t>& material_guid_map, const char* name, mesh_def_t& out)
		{
			out.name = name;
			out.materials.reserve(model.materials_count);
			for (u32 i = 0; i < model.materials_count; ++i)
			{
				const auto it = material_guid_map.find(i);
				out.materials.push_back(it != material_guid_map.end() ? it->second : DEFAULT_GBUFFER_MATERIAL_ASSET_GUID);
			}

			bool is_skinned = false;
			for (u32 mesh_i = 0; mesh_i < mesh_count; ++mesh_i)
			{
				const tg3_mesh& mesh = meshes[mesh_i];
				for (u32 primitive_i = 0; primitive_i < mesh.primitives_count; ++primitive_i)
				{
					const tg3_primitive& primitive = mesh.primitives[primitive_i];
					is_skinned					   = is_skinned || find_attribute(primitive, "JOINTS_0") >= 0 || find_attribute(primitive, "WEIGHTS_0") >= 0;
				}
			}

			for (u32 mesh_i = 0; mesh_i < mesh_count; ++mesh_i)
			{
				const tg3_mesh& mesh = meshes[mesh_i];
				for (u32 primitive_i = 0; primitive_i < mesh.primitives_count; ++primitive_i)
				{
					const tg3_primitive& primitive		= mesh.primitives[primitive_i];
					const u32			 material_index = primitive.material >= 0 && static_cast<u32>(primitive.material) < out.materials.size() ? static_cast<u32>(primitive.material) : UINT32_MAX;
					if (is_skinned)
					{
						primitive_skinned_def_t primitive_def = {};
						if (!import_skinned_primitive(model, primitive, material_index, primitive_def))
							return false;
						out.skinned_primitives.push_back(std::move(primitive_def));
					}
					else
					{
						primitive_static_def_t primitive_def = {};
						if (!import_static_primitive(model, primitive, material_index, primitive_def))
							return false;
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
				return false;

			out.local_bounds = aabb_t(bounds_min, bounds_max);
			return true;
		}

		bool import_mesh(const editor_asset_node_t&			  parent_node,
						 const char*						  source_full_path,
						 const tg3_model&					  model,
						 const tg3_mesh*					  meshes,
						 u32								  mesh_count,
						 const hash_map_t<u32, sid_t>&		  material_guid_map,
						 const editor_asset_import_context_t& context,
						 vector_t<editor_asset_t>&			  out_assets)
		{
			SFG_ASSERT(meshes != nullptr);
			SFG_ASSERT(mesh_count != 0);

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
			if (!build_mesh_def(model, meshes, mesh_count, material_guid_map, asset_name.c_str(), mesh_def))
				return false;

			editor_asset_t asset		 = {};
			const string_t asset_path	 = editor_asset_util_t::make_asset_path(parent_node.full_path.c_str(), asset_name.c_str());
			const string_t blob_path	 = editor_asset_util_t::make_blob_path(parent_node.full_path.c_str(), asset_name.c_str());
			const sid_t	   existing_guid = editor_asset_util_t::try_read_existing_guid(asset_path.c_str());
			ostream_t	   mesh_def_stream;
			if (!reflection_registry_t::get().serialize_to_stream(type_id_t<mesh_def_t>::value, &mesh_def, mesh_def_stream))
				return false;
			if (!write_blob(blob_path.c_str(), mesh_def_stream.get_raw(), mesh_def_stream.get_size()))
				return false;

			asset.version		  = editor_asset_t::VERSION;
			asset.guid			  = existing_guid != NULL_SID ? existing_guid : editor_asset_util_t::generate_unique_asset_guid();
			asset.asset_type	  = editor_asset_type_e::mesh;
			asset.source_type	  = editor_asset_source_type_e::file_blob;
			asset.source_relative = editor_asset_util_t::get_source_relative(editor_project_t::get()._runtime.assets_path.c_str(), blob_path.c_str());

			if (!editor_asset_util_t::write_asset(asset_path.c_str(), asset))
				return false;

			if (!editor_asset_cooker_t::cook_mesh(asset))
				return false;

			out_assets.push_back(asset);
			return true;
		}
	}

	bool editor_glb_importer_t::import_glb(editor_asset_node_handle_t directory_node, const char* source_full_path, const glb_cook_config_t& cook_config, const editor_asset_import_context_t& context, vector_t<editor_asset_t>& out_assets)
	{
		const editor_asset_tree_t& tree = editor_asset_manager_t::get().get_asset_tree();
		SFG_ASSERT(!directory_node.is_null());
		SFG_ASSERT(tree.is_valid(directory_node));
		const editor_asset_node_t& parent_node = tree.value(directory_node);
		SFG_ASSERT(parent_node.type == editor_asset_node_type_e::folder);
		SFG_ASSERT(!parent_node.full_path.empty());
		SFG_ASSERT(source_full_path != nullptr);
		SFG_ASSERT(source_full_path[0] != '\0');

		editor_asset_t glb_source_asset = {};
		const string_t glb_asset_name	= file_system_t::get_filename_from_path(source_full_path);
		string_t	   status			= "Copying GLB ";
		status += glb_asset_name;
		context.report_status(status.c_str());
		if (!editor_asset_util_t::set_source_relative_or_copy(glb_source_asset, parent_node.full_path.c_str(), glb_asset_name.c_str(), source_full_path))
			return false;

		status = "Reading GLB ";
		status += glb_asset_name;
		context.report_status(status.c_str());

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

		status = "Parsing GLB ";
		status += glb_asset_name;
		context.report_status(status.c_str());

		const string_t		 base_dir	  = file_system_t::get_directory_of_file(source_full_path);
		const tg3_error_code parse_result = tg3_parse_glb(&model, &errors, reinterpret_cast<const u8*>(glb_data), static_cast<u64>(glb_size), base_dir.c_str(), static_cast<u32>(base_dir.size()), &parse_options);
		delete[] glb_data;

		bool result = parse_result == TG3_OK;
		if (result)
		{
			const texture_cook_config_t texture_config = {
				.payload_type	  = cook_config.texture_payload_type,
				.ktx2_compression = cook_config.ktx2_compression,
				.generate_mipmaps = cook_config.generate_mipmaps,
			};

			hash_map_t<u32, sid_t> texture_guid_map;
			texture_guid_map.reserve(model.textures_count);
			hash_map_t<u64, u32> imported_texture_indices;
			imported_texture_indices.reserve(model.textures_count);
			vector_t<glb_texture_import_t> texture_imports;
			texture_imports.reserve(model.textures_count);
			vector_t<u32> texture_import_indices;
			texture_import_indices.resize(model.textures_count);

			const u32 reserve_texture_count	  = cook_config.import_textures ? model.textures_count : 0;
			const u32 reserve_animation_count = cook_config.import_animations ? model.skins_count : 0;
			const u32 reserve_material_count  = cook_config.import_materials ? model.materials_count : 0;
			const u32 reserve_mesh_count	  = cook_config.import_meshes ? (cook_config.combine_meshes ? 1 : model.meshes_count) : 0;
			out_assets.reserve(out_assets.size() + reserve_texture_count + reserve_animation_count + reserve_material_count + reserve_mesh_count);
			if (cook_config.import_textures)
			{
				for (u32 i = 0; i < model.textures_count; ++i)
				{
					const tg3_texture& texture = model.textures[i];
					if (texture.source < 0 || static_cast<u32>(texture.source) >= model.images_count)
					{
						result = false;
						break;
					}

					const u64  texture_import_key  = (static_cast<u64>(static_cast<u32>(texture.source)) << 32) | static_cast<u32>(texture.sampler);
					const auto imported_texture_it = imported_texture_indices.find(texture_import_key);
					if (imported_texture_it != imported_texture_indices.end())
					{
						texture_import_indices[i] = imported_texture_it->second;
						continue;
					}

					const u32 import_index						 = static_cast<u32>(texture_imports.size());
					imported_texture_indices[texture_import_key] = import_index;
					texture_import_indices[i]					 = import_index;
					texture_imports.push_back({.texture_index = i});
				}
			}

			if (result && !texture_imports.empty())
			{
				string_t status = "Importing textures 0/";
				status += std::to_string(texture_imports.size());
				context.report_status(status.c_str());

				vector_t<glb_texture_import_result_t> texture_import_results;
				texture_import_results.resize(texture_imports.size());
				std::atomic<u32> textures_finished = 0;

				tf::Taskflow texture_import_flow;
				for (u32 import_index = 0; import_index < texture_imports.size(); ++import_index)
				{
					texture_import_flow.emplace([&, import_index]() {
						const glb_texture_import_t&			texture_import	= texture_imports[import_index];
						glb_texture_import_result_t&		import_result	= texture_import_results[import_index];
						const tg3_texture&					texture			= model.textures[texture_import.texture_index];
						const editor_asset_import_context_t texture_context = {};

						import_result.success = import_texture(parent_node, source_full_path, model, texture, texture_config, texture_import.texture_index, texture_context, import_result.asset);
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

				if (result)
				{
					for (u32 i = 0; i < model.textures_count; ++i)
					{
						const u32 import_index = texture_import_indices[i];
						SFG_ASSERT(import_index < texture_import_results.size());
						texture_guid_map[i] = texture_import_results[import_index].guid;
					}

					for (glb_texture_import_result_t& import_result : texture_import_results)
					{
						SFG_ASSERT(import_result.success);
						out_assets.push_back(std::move(import_result.asset));
					}
				}
			}

			hash_map_t<u32, sid_t> material_guid_map;
			material_guid_map.reserve(model.materials_count);
			if (result && cook_config.import_materials)
			{
				for (u32 i = 0; i < model.materials_count; ++i)
				{
					if (!import_material(parent_node, source_full_path, model, model.materials[i], i, texture_guid_map, material_guid_map, context, out_assets))
					{
						result = false;
						break;
					}
				}
			}

			if (result && cook_config.import_animations)
			{
				for (u32 i = 0; i < model.skins_count; ++i)
				{
					if (!import_skeleton(parent_node, source_full_path, model, model.skins[i], i, context, out_assets))
					{
						result = false;
						break;
					}
				}
			}

			if (result && cook_config.import_meshes && model.meshes_count != 0)
			{
				if (cook_config.combine_meshes)
				{
					result = import_mesh(parent_node, source_full_path, model, model.meshes, model.meshes_count, material_guid_map, context, out_assets);
				}
				else
				{
					for (u32 i = 0; i < model.meshes_count; ++i)
					{
						if (!import_mesh(parent_node, source_full_path, model, model.meshes + i, 1, material_guid_map, context, out_assets))
						{
							result = false;
							break;
						}
					}
				}
			}
		}

		tg3_model_free(&model);
		tg3_error_stack_free(&errors);
		return result;
	}

	glb_cook_config_reflection_t::glb_cook_config_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();
		if (registry.find_type(type_id_t<glb_cook_config_t>::value) != nullptr)
			return;

		static const reflected_field_desc_t fields[] = {
			{.name = "import_textures", .display_name = "Import Textures", .type = reflected_value_type_e::bool8, .offset = offsetof(glb_cook_config_t, import_textures), .size = sizeof(bool)},
			{.name = "import_materials", .display_name = "Import Materials", .type = reflected_value_type_e::bool8, .offset = offsetof(glb_cook_config_t, import_materials), .size = sizeof(bool)},
			{.name = "import_animations", .display_name = "Import Animations", .type = reflected_value_type_e::bool8, .offset = offsetof(glb_cook_config_t, import_animations), .size = sizeof(bool)},
			{.name = "import_meshes", .display_name = "Import Meshes", .type = reflected_value_type_e::bool8, .offset = offsetof(glb_cook_config_t, import_meshes), .size = sizeof(bool)},
			{.name		   = "texture_payload_type",
			 .display_name = "Texture Payload Type",
			 .type		   = reflected_value_type_e::enum8,
			 .sub_type_id  = type_id_t<texture_payload_type_e>::value,
			 .offset	   = offsetof(glb_cook_config_t, texture_payload_type),
			 .size		   = sizeof(texture_payload_type_e)},
			{.name		   = "ktx2_compression",
			 .display_name = "KTX2 Compression",
			 .type		   = reflected_value_type_e::enum8,
			 .sub_type_id  = type_id_t<texture_ktx2_compression_e>::value,
			 .offset	   = offsetof(glb_cook_config_t, ktx2_compression),
			 .size		   = sizeof(texture_ktx2_compression_e)},
			{.name = "generate_mipmaps", .display_name = "Generate Mipmaps", .type = reflected_value_type_e::bool8, .offset = offsetof(glb_cook_config_t, generate_mipmaps), .size = sizeof(bool)},
			{.name = "combine_meshes", .display_name = "Combine Meshes", .type = reflected_value_type_e::bool8, .offset = offsetof(glb_cook_config_t, combine_meshes), .size = sizeof(bool)},
		};

		registry.register_type({
			.fields	   = {.data = fields, .size = std::size(fields)},
			.name	   = "glb_cook_config_t",
			.type_id   = type_id_t<glb_cook_config_t>::value,
			.size	   = sizeof(glb_cook_config_t),
			.alignment = alignof(glb_cook_config_t),
		});
	}
}
