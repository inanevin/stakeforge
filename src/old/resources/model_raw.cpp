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

#include "model_raw.hpp"
#include "data/ostream_vector.hpp"
#include "data/istream_vector.hpp"
#include "common/packing.hpp"

#ifdef SFG_TOOLMODE
#include "data/bitmask.hpp"
#include "io/log.hpp"
#include "io/file_system.hpp"
#include "serialization/serialization.hpp"
#include "data/vector_util.hpp"
#include "math/math.hpp"
#include "gfx/common/format.hpp"
#include "vendor/nhlohmann/json.hpp"
#include "world/components/comp_light.hpp"

#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "vendor/stb/stb_image.h"
#include "vendor/stb/stb_image_write.h"
#include "vendor/syoyo/tiny_gltf.h"
using json = nlohmann::json;
#include "platform/time.hpp"

#endif

namespace SFG
{
	model_raw::model_raw()
	{
		// SFG_WARN("yeah");
	}
	model_raw::~model_raw()
	{
		// SFG_ERR("nah");
	}
	void model_raw::serialize(ostream& stream) const
	{
		stream << name;
		stream << source;
		stream << loaded_textures;
		stream << loaded_materials;
		stream << loaded_nodes;
		stream << loaded_meshes;
		stream << loaded_skins;
		stream << loaded_animations;
		stream << loaded_lights;
		stream << total_aabb;
	}

	void model_raw::deserialize(istream& stream)
	{
		stream >> name;
		stream >> source;
		stream >> loaded_textures;
		stream >> loaded_materials;
		stream >> loaded_nodes;
		stream >> loaded_meshes;
		stream >> loaded_skins;
		stream >> loaded_animations;
		stream >> loaded_lights;
		stream >> total_aabb;
	}

#ifdef SFG_TOOLMODE
	namespace
	{
		matrix4x4 make_mat(const vector<double>& d)
		{
			matrix4x4 mat;
			for (size_t i = 0; i < 16; ++i)
				mat.m[i] = static_cast<f32>(d[i]);
			return mat;
		}

		matrix4x3 make_mat43(const vector<double>& d)
		{
			matrix4x3 mat;
			for (size_t i = 0; i < 12; ++i)
				mat.m[i] = static_cast<f32>(d[i]);
			return mat;
		}

		auto fill_prim = [](auto&						prim,
							const tinygltf::Model&		model,
							const tinygltf::Primitive&	tprim,
							const tinygltf::Accessor&	vertex_a,
							const tinygltf::BufferView& vertex_bv,
							const tinygltf::Buffer&		vertex_b,
							size_t						num_vertices,
							size_t						start_vertices,
							size_t						start_indices) {
			for (size_t j = 0; j < num_vertices; ++j)
			{
				const size_t stride					  = vertex_bv.byteStride == 0 ? sizeof(f32) * 3 : vertex_bv.byteStride;
				const f32*	 rawFloatData			  = reinterpret_cast<const f32*>(vertex_b.data.data() + vertex_a.byteOffset + vertex_bv.byteOffset + j * stride);
				prim.vertices[start_vertices + j].pos = vector3(rawFloatData[0], rawFloatData[1], rawFloatData[2]);
			}

			if (tprim.indices != -1)
			{
				const tinygltf::Accessor&	index_a	 = model.accessors[tprim.indices];
				const tinygltf::BufferView& index_bv = model.bufferViews[index_a.bufferView];
				const tinygltf::Buffer&		index_b	 = model.buffers[index_bv.buffer];
				SFG_ASSERT(
					(index_a.type == TINYGLTF_TYPE_SCALAR && (index_a.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT || index_a.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE || index_a.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)),
					"Unsupported component type!");

				if (index_a.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					SFG_ASSERT(sizeof(primitive_index) >= sizeof(u16));
				}

				if (index_a.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					SFG_ASSERT(sizeof(primitive_index) >= sizeof(u32));
				}

				const size_t num_indices = index_a.count;
				prim.indices.resize(start_indices + num_indices);
				SFG_ASSERT(num_indices % 3 == 0);

				if (index_a.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
				{
					u8* data = (u8*)&index_b.data[index_a.byteOffset + index_bv.byteOffset];
					for (u32 k = 0; k < num_indices; k++)
						prim.indices[start_indices + k] = static_cast<primitive_index>(data[k]);
				}
				else if (index_a.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					if (sizeof(primitive_index) == sizeof(u16))
					{
						SFG_MEMCPY(prim.indices.data() + start_indices * sizeof(primitive_index), &index_b.data[index_a.byteOffset + index_bv.byteOffset], num_indices * sizeof(u16));
					}
					else
					{
						// our indices are 32 bit, need to cast
						u16* data = (u16*)&index_b.data[index_a.byteOffset + index_bv.byteOffset];

						for (u32 k = 0; k < num_indices; k++)
							prim.indices[start_indices + k] = static_cast<u32>(data[k]);
					}
				}
				else
				{
					SFG_MEMCPY(prim.indices.data() + start_indices * sizeof(primitive_index), &index_b.data[index_a.byteOffset + index_bv.byteOffset], num_indices * sizeof(u32));
				}
			}

			auto vtx_colors_attribute = tprim.attributes.find("COLOR");
			if (vtx_colors_attribute != tprim.attributes.end())
			{
				SFG_WARN("GLTF model has vertex colors. welp they are not supported.");
			}

			auto normals_attribute = tprim.attributes.find("NORMAL");
			if (normals_attribute != tprim.attributes.end())
			{
				const tinygltf::Accessor&	normals_a  = model.accessors[normals_attribute->second];
				const tinygltf::BufferView& normals_bv = model.bufferViews[normals_a.bufferView];
				const tinygltf::Buffer&		normals_b  = model.buffers[normals_bv.buffer];
				SFG_ASSERT((normals_a.type == TINYGLTF_TYPE_VEC3 && normals_a.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT), "Unsupported component type!");

				const size_t num_normals = normals_a.count;
				SFG_ASSERT(num_normals == num_vertices);

				for (size_t j = 0; j < num_normals; ++j)
				{
					const size_t stride						 = normals_bv.byteStride == 0 ? sizeof(f32) * 3 : normals_bv.byteStride;
					const f32*	 raw_data					 = reinterpret_cast<const f32*>(normals_b.data.data() + normals_a.byteOffset + normals_bv.byteOffset + j * stride);
					prim.vertices[start_vertices + j].normal = vector3(raw_data[0], raw_data[1], raw_data[2]);
				}
			}
			else
			{
				SFG_ASSERT(false);
			}

			auto uv_attribute = tprim.attributes.find("TEXCOORD_0");
			if (uv_attribute != tprim.attributes.end())
			{
				const tinygltf::Accessor&	uv_a  = model.accessors[uv_attribute->second];
				const tinygltf::BufferView& uv_bv = model.bufferViews[uv_a.bufferView];
				const tinygltf::Buffer&		uv_b  = model.buffers[uv_bv.buffer];
				SFG_ASSERT((uv_a.type == TINYGLTF_TYPE_VEC2 && uv_a.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT), "Unsupported component type!");

				const size_t num_uv = uv_a.count;
				SFG_ASSERT(num_uv == num_vertices);

				for (size_t j = 0; j < num_uv; ++j)
				{
					const size_t stride					 = uv_bv.byteStride == 0 ? sizeof(f32) * 2 : uv_bv.byteStride;
					const f32*	 raw_data				 = reinterpret_cast<const f32*>(uv_b.data.data() + uv_a.byteOffset + uv_bv.byteOffset + j * stride);
					prim.vertices[start_vertices + j].uv = vector2(raw_data[0], raw_data[1]);
				}
			}
			else
			{
				SFG_ASSERT(false);
			}

			auto tangents_attribute = tprim.attributes.find("TANGENT");
			if (tangents_attribute != tprim.attributes.end())
			{
				const tinygltf::Accessor&	tangents_a	= model.accessors[tangents_attribute->second];
				const tinygltf::BufferView& targents_bv = model.bufferViews[tangents_a.bufferView];
				const tinygltf::Buffer&		tangents_b	= model.buffers[targents_bv.buffer];
				SFG_ASSERT((tangents_a.type == TINYGLTF_TYPE_VEC4 && tangents_a.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT), "Unsupported component type!");

				const size_t num_tangents = tangents_a.count;
				SFG_ASSERT(num_tangents == num_vertices);

				for (size_t j = 0; j < num_tangents; ++j)
				{
					const size_t stride						  = targents_bv.byteStride == 0 ? sizeof(f32) * 4 : targents_bv.byteStride;
					const f32*	 raw_data					  = reinterpret_cast<const f32*>(tangents_b.data.data() + tangents_a.byteOffset + targents_bv.byteOffset + j * stride);
					prim.vertices[start_vertices + j].tangent = vector4(raw_data[0], raw_data[1], raw_data[2], raw_data[3]);
				}
			}
			else
			{
				SFG_ASSERT(false);
			}

			/*
			auto colorsAttribute = tprim.attributes.find("COLOR_0");
			if (colorsAttribute != tprim.attributes.end())
			{
				const tinygltf::Accessor&	colorsAccessor	 = model.accessors[colorsAttribute->second];
				const tinygltf::BufferView& colorsBufferView = model.bufferViews[colorsAccessor.bufferView];
				const tinygltf::Buffer&		colorsBuffer	 = model.buffers[colorsBufferView.buffer];
				SFG_ASSERT((colorsAccessor.type == TINYGLTF_TYPE_VEC4 && colorsAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT), "Unsupported component type!");

				const size_t numColors = colorsAccessor.count;
				primitive->colors.resize(numColors);

				for (size_t j = 0; j < numColors; ++j)
				{
					const size_t stride		  = colorsBufferView.byteStride == 0 ? sizeof(f32) * 4 : colorsBufferView.byteStride;
					const f32* rawFloatData = reinterpret_cast<const f32*>(colorsBuffer.data.data() + colorsAccessor.byteOffset + colorsBufferView.byteOffset + j * stride);
					LGXVector4&	 color		  = primitive->colors[j];
					color.x					  = rawFloatData[0];
					color.y					  = rawFloatData[1];
					color.z					  = rawFloatData[2];
					color.w					  = rawFloatData[3];
				}
			}


			*/
		};

		template <typename VertexType> void append_collider_primitive(vector<vector3>& vertices, vector<primitive_index>& indices, const vector<VertexType>& src_vertices, const vector<primitive_index>& src_indices)
		{
			const u32 base = static_cast<u32>(vertices.size());
			for (const auto& v : src_vertices)
				vertices.push_back(v.pos);

			if (!src_indices.empty())
			{
				for (const primitive_index idx : src_indices)
					indices.push_back(static_cast<primitive_index>(base + idx));
				return;
			}

			const size_t count = src_vertices.size();
			if (count % 3 != 0)
				return;

			for (size_t i = 0; i < count; ++i)
				indices.push_back(static_cast<primitive_index>(base + static_cast<u32>(i)));
		}

		void build_mesh_colliders(mesh_raw& mesh)
		{
			mesh.collider_vertices.clear();
			mesh.collider_indices.clear();

			for (const auto& prim : mesh.primitives_static)
				append_collider_primitive(mesh.collider_vertices, mesh.collider_indices, prim.vertices, prim.indices);

			for (const auto& prim : mesh.primitives_skinned)
				append_collider_primitive(mesh.collider_vertices, mesh.collider_indices, prim.vertices, prim.indices);
		}
	}

	bool model_raw::import_gtlf(const char* file, const char* relative_path, bool create_materials, bool import_textures)
	{
		tinygltf::Model	   model;
		tinygltf::TinyGLTF loader;
		loader.SetPreserveImageChannels(false);

		string err = "", warn = "";

		bool ret = false;

		const string ext = file_system::get_file_extension(file);

		if (ext.compare("glb") == 0)
			ret = loader.LoadBinaryFromFile(&model, &err, &warn, file);
		else
			ret = loader.LoadASCIIFromFile(&model, &err, &warn, file);

		if (!warn.empty())
		{
			SFG_WARN("{0}", warn);
		}

		if (!err.empty())
		{
			SFG_ERR("{0}", err);
			return false;
		}

		if (!ret)
		{
			SFG_ERR("Loading model failed! {0}", file);
			return false;
		}

		const size_t all_meshes_sz = model.meshes.size();
		loaded_meshes.resize(all_meshes_sz);

		u32	 empty_mesh_name_ctr = 0;
		auto get_empty_mesh_name = [&]() -> string {
			const string str = "empty_mesh_" + std::to_string(empty_mesh_name_ctr);
			empty_mesh_name_ctr++;
			return str;
		};
		for (size_t i = 0; i < all_meshes_sz; i++)
		{
			const tinygltf::Mesh& tmesh		= model.meshes[i];
			mesh_raw&			  mesh		= loaded_meshes[i];
			const string		  hash_path = string(relative_path) + "/" + (tmesh.name.empty() ? get_empty_mesh_name() : tmesh.name);
			mesh.name						= hash_path;
			const string_id hash			= TO_SID(hash_path);
			mesh.sid						= hash;

			for (const tinygltf::Primitive& tprim : tmesh.primitives)
			{
				if (tprim.material == -1)
				{
					int a = 5;
				}
				mesh.materials.push_back(tprim.material);

				const tinygltf::Accessor&	vertex_accessor	   = model.accessors[tprim.attributes.find("POSITION")->second];
				const tinygltf::BufferView& vertex_buffer_view = model.bufferViews[vertex_accessor.bufferView];
				const tinygltf::Buffer&		vertex_buffer	   = model.buffers[vertex_buffer_view.buffer];

				SFG_ASSERT((vertex_accessor.type == TINYGLTF_TYPE_VEC3 && vertex_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT), "Unsupported component type!");
				const size_t num_vertices = vertex_accessor.count;

				const vector3 min_position = {static_cast<f32>(vertex_accessor.minValues[0]), static_cast<f32>(vertex_accessor.minValues[1]), static_cast<f32>(vertex_accessor.minValues[2])};
				const vector3 max_position = {static_cast<f32>(vertex_accessor.maxValues[0]), static_cast<f32>(vertex_accessor.maxValues[1]), static_cast<f32>(vertex_accessor.maxValues[2])};

				total_aabb.bounds_min	   = vector3::min(total_aabb.bounds_min, min_position);
				total_aabb.bounds_max	   = vector3::max(total_aabb.bounds_max, max_position);
				mesh.local_aabb.bounds_min = vector3::min(mesh.local_aabb.bounds_min, min_position);
				mesh.local_aabb.bounds_max = vector3::max(mesh.local_aabb.bounds_max, max_position);

				auto joints0  = tprim.attributes.find("JOINTS_0");
				auto weights0 = tprim.attributes.find("WEIGHTS_0");

				// if skinned prim, fill joints & weights here & call generic fill_prim.
				if (joints0 != tprim.attributes.end() && weights0 != tprim.attributes.end())
				{
					const u16  mat	 = tprim.material < 0 ? 0 : static_cast<u16>(tprim.material);
					auto	   it	 = vector_util::find_if(mesh.primitives_skinned, [mat](const primitive_skinned_raw& p) -> bool { return p.material_index == mat; });
					const bool found = it != mesh.primitives_skinned.end();
					if (!found)
						mesh.primitives_skinned.push_back({});

					primitive_skinned_raw& prim			= found ? *it : mesh.primitives_skinned.back();
					const size_t		   start_vertex = prim.vertices.size();
					const size_t		   start_index	= prim.indices.size();
					prim.vertices.resize(start_vertex + num_vertices);

					const tinygltf::Accessor&	joints_a   = model.accessors[joints0->second];
					const tinygltf::BufferView& joints_bv  = model.bufferViews[joints_a.bufferView];
					const tinygltf::Buffer&		joints_b   = model.buffers[joints_bv.buffer];
					const tinygltf::Accessor&	weights_a  = model.accessors[weights0->second];
					const tinygltf::BufferView& weights_bv = model.bufferViews[weights_a.bufferView];
					const tinygltf::Buffer&		weights_b  = model.buffers[weights_bv.buffer];

					SFG_ASSERT((joints_a.type == TINYGLTF_TYPE_VEC4 && (joints_a.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT || joints_a.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)), "Unsupported component type!");
					SFG_ASSERT((weights_a.type == TINYGLTF_TYPE_VEC4 && weights_a.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT), "Unsupported component type!");
					SFG_ASSERT(joints_a.count == weights_a.count);
					SFG_ASSERT(joints_a.count == num_vertices);

					const size_t num_joints = joints_a.count;

					for (size_t j = 0; j < num_joints; j++)
					{
						const u32 vertex_index = start_vertex + j;

						if (joints_a.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
						{
							const size_t stride						 = joints_bv.byteStride == 0 ? sizeof(u16) * 4 : joints_bv.byteStride;
							const u16*	 rawData					 = reinterpret_cast<const u16*>(joints_b.data.data() + joints_a.byteOffset + joints_bv.byteOffset + j * stride);
							prim.vertices[vertex_index].bone_indices = vector4i(rawData[0], rawData[1], rawData[2], rawData[3]);
						}
						else if (joints_a.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
						{
							const size_t   stride					 = joints_bv.byteStride == 0 ? sizeof(u8) * 4 : joints_bv.byteStride;
							const u8*	   rawData					 = reinterpret_cast<const u8*>(joints_b.data.data() + joints_a.byteOffset + joints_bv.byteOffset + j * stride);
							const vector4i idx						 = vector4i(rawData[0], rawData[1], rawData[2], rawData[3]);
							prim.vertices[vertex_index].bone_indices = idx;
						}
						else
						{
							SFG_ASSERT(false);
						}
					}

					const size_t num_weights = weights_a.count;
					for (size_t j = 0; j < num_weights; ++j)
					{
						const u32	  vertex_index				 = start_vertex + j;
						const size_t  stride					 = weights_bv.byteStride == 0 ? sizeof(f32) * 4 : weights_bv.byteStride;
						const f32*	  rawData					 = reinterpret_cast<const f32*>(weights_b.data.data() + weights_a.byteOffset + weights_bv.byteOffset + j * stride);
						const vector4 weights					 = vector4(rawData[0], rawData[1], rawData[2], rawData[3]);
						prim.vertices[vertex_index].bone_weights = weights;
					}

					fill_prim(prim, model, tprim, vertex_accessor, vertex_buffer_view, vertex_buffer, num_vertices, start_vertex, start_index);

					const i16 prim_mat_idx = vector_util::index_of(mesh.materials, static_cast<i16>(tprim.material));
					SFG_ASSERT(prim_mat_idx != -1);
					prim.material_index = static_cast<u16>(prim_mat_idx);
					continue;
				}
				const u16  mat	 = tprim.material < 0 ? 0 : static_cast<u16>(tprim.material);
				auto	   it	 = vector_util::find_if(mesh.primitives_static, [mat](const primitive_static_raw& p) -> bool { return p.material_index == mat; });
				const bool found = it != mesh.primitives_static.end();
				if (!found)
					mesh.primitives_static.push_back({});
				primitive_static_raw& prim		   = found ? *it : mesh.primitives_static.back();
				const size_t		  start_vertex = prim.vertices.size();
				const size_t		  start_index  = prim.indices.size();
				prim.vertices.resize(start_vertex + num_vertices);

				fill_prim(prim, model, tprim, vertex_accessor, vertex_buffer_view, vertex_buffer, num_vertices, start_vertex, start_index);
				const i16 prim_mat_idx = vector_util::index_of(mesh.materials, static_cast<i16>(tprim.material));
				SFG_ASSERT(prim_mat_idx != -1);
				prim.material_index = static_cast<u16>(prim_mat_idx);
			}
		}

		const size_t all_nodes_sz = model.nodes.size();
		loaded_nodes.resize(all_nodes_sz);

		for (size_t i = 0; i < all_nodes_sz; i++)
		{
			const tinygltf::Node& tnode = model.nodes[i];
			model_node_raw&		  node	= loaded_nodes[i];
			node.name					= tnode.name;
			node.mesh_index				= static_cast<i16>(tnode.mesh);
			node.skin_index				= static_cast<i16>(tnode.skin);
			node.light_index			= static_cast<i16>(tnode.light);

			if (tnode.matrix.empty())
			{
				const vector3 p	  = tnode.translation.empty() ? vector3::zero : vector3(tnode.translation[0], tnode.translation[1], tnode.translation[2]);
				const vector3 s	  = tnode.scale.empty() ? vector3::one : vector3(tnode.scale[0], tnode.scale[1], tnode.scale[2]);
				const quat	  r	  = tnode.rotation.empty() ? quat::identity : quat(tnode.rotation[0], tnode.rotation[1], tnode.rotation[2], tnode.rotation[3]);
				node.local_matrix = matrix4x3::transform(p, r, s);
			}
			else
			{
				node.local_matrix = make_mat43(tnode.matrix);
			}

			for (int child : tnode.children)
				loaded_nodes[child].parent_index = i;

			if (tnode.mesh != -1)
			{
				loaded_meshes[tnode.mesh].node_index = static_cast<u16>(i);
				loaded_meshes[tnode.mesh].skin_index = node.skin_index;
			}
		}

		const size_t all_skins_sz = model.skins.size();

		for (size_t i = 0; i < all_skins_sz; i++)
		{
			const tinygltf::Skin& tskin = model.skins[i];

			const string	hash_path = string(relative_path) + "/" + tskin.name;
			const string_id hash	  = TO_SID(hash_path);
			loaded_skins.push_back({});
			skin_raw& skin = loaded_skins.back();
			skin.sid	   = hash;

			const size_t joints_sz = tskin.joints.size();
			skin.name			   = hash_path;

			skin.joints.resize(joints_sz);
			skin.root_joint = static_cast<i16>(tskin.skeleton);

			const tinygltf::Accessor&	inverse_bind_a	= model.accessors[tskin.inverseBindMatrices];
			const tinygltf::BufferView& inverse_bind_bv = model.bufferViews[inverse_bind_a.bufferView];
			const tinygltf::Buffer&		inverse_bind_b	= model.buffers[inverse_bind_bv.buffer];
			const size_t				matrix_count	= inverse_bind_a.count;
			SFG_ASSERT((inverse_bind_a.type == TINYGLTF_TYPE_MAT4 && inverse_bind_a.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT), "Unsupported component type!");
			SFG_ASSERT(matrix_count == joints_sz);

			for (size_t j = 0; j < joints_sz; j++)
			{
				const size_t stride		 = inverse_bind_bv.byteStride == 0 ? sizeof(f32) * 16 : inverse_bind_bv.byteStride;
				const f32*	 raw		 = reinterpret_cast<const f32*>(inverse_bind_b.data.data() + inverse_bind_a.byteOffset + inverse_bind_bv.byteOffset + j * stride);
				const i32	 joint_index = tskin.joints[j];

				const model_node_raw& node = loaded_nodes[joint_index];

				skin_joint& sj		= skin.joints[j];
				sj.parent_index		= node.parent_index;
				sj.model_node_index = static_cast<i16>(joint_index);
				sj.local_matrix		= loaded_nodes[joint_index].local_matrix;
				sj.name_hash		= TO_SID(loaded_nodes[joint_index].name);

				sj.inverse_bind_matrix[0]  = raw[0];
				sj.inverse_bind_matrix[1]  = raw[1];
				sj.inverse_bind_matrix[2]  = raw[2];
				sj.inverse_bind_matrix[3]  = raw[4];
				sj.inverse_bind_matrix[4]  = raw[5];
				sj.inverse_bind_matrix[5]  = raw[6];
				sj.inverse_bind_matrix[6]  = raw[8];
				sj.inverse_bind_matrix[7]  = raw[9];
				sj.inverse_bind_matrix[8]  = raw[10];
				sj.inverse_bind_matrix[9]  = raw[12];
				sj.inverse_bind_matrix[10] = raw[13];
				sj.inverse_bind_matrix[11] = raw[14];
			}
		}

		vector<i32>			 loaded_indices;
		vector<i32>			 loaded_sampler_indices;
		vector<sampler_desc> samplers;

		if (import_textures)
		{
			const size_t all_textures_sz = model.textures.size();

			auto check_if_linear = [&](int index) -> bool {
				for (const tinygltf::Material& tmat : model.materials)
				{
					if (tmat.pbrMetallicRoughness.metallicRoughnessTexture.index == index)
						return true;
					if (tmat.normalTexture.index == index)
						return true;
				}
				return false;
			};

			auto get_texture_order = [&](int index) -> int {
				for (const tinygltf::Material& tmat : model.materials)
				{
					if (tmat.pbrMetallicRoughness.baseColorTexture.index == index)
						return 0;
					if (tmat.normalTexture.index == index)
						return 1;
					if (tmat.pbrMetallicRoughness.metallicRoughnessTexture.index == index)
						return 2;
					if (tmat.emissiveTexture.index == index)
						return 3;
				}

				return 0;
			};

			auto load_smp = [&](i32 smp_index) {
				const tinygltf::Sampler& smp = model.samplers[smp_index];

				if (smp_index < samplers.size())
				{
					loaded_sampler_indices.push_back(smp_index);
					return;
				}

				loaded_sampler_indices.push_back(samplers.size());

				const int min_filter = smp.minFilter;
				const int mag_filter = smp.magFilter;

				samplers.push_back({});
				sampler_desc& raw = samplers.back();

				raw.min_lod = 0.0f;
				raw.max_lod = 10.0f;

				bitmask<u16> flags = 0;
				if (min_filter == TINYGLTF_TEXTURE_FILTER_NEAREST)
					flags.set(sampler_flags::saf_min_nearest);
				else if (min_filter == TINYGLTF_TEXTURE_FILTER_LINEAR)
					flags.set(sampler_flags::saf_min_linear);
				if (min_filter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST)
					flags.set(sampler_flags::saf_min_nearest | sampler_flags::saf_mip_nearest);
				if (min_filter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST)
					flags.set(sampler_flags::saf_min_linear | sampler_flags::saf_mip_nearest);
				if (min_filter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR)
					flags.set(sampler_flags::saf_min_nearest | sampler_flags::saf_mip_linear);
				if (min_filter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR)
					flags.set(sampler_flags::saf_min_linear | sampler_flags::saf_mip_linear);

				if (mag_filter == 0)
					flags.set(sampler_flags::saf_mag_nearest);
				else if (mag_filter == 1)
					flags.set(sampler_flags::saf_mag_linear);

				raw.flags = flags;

				const int address_u = smp.wrapS;
				const int address_v = smp.wrapT;

				if (address_u == TINYGLTF_TEXTURE_WRAP_REPEAT)
					raw.address_u = address_mode::repeat;
				else if (address_u == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE)
					raw.address_u = address_mode::clamp;
				else if (address_u == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT)
					raw.address_u = address_mode::mirrored_repeat;

				if (address_v == TINYGLTF_TEXTURE_WRAP_REPEAT)
					raw.address_v = address_mode::repeat;
				else if (address_v == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE)
					raw.address_v = address_mode::clamp;
				else if (address_v == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT)
					raw.address_v = address_mode::mirrored_repeat;
			};

			for (size_t i = 0; i < all_textures_sz; i++)
			{
				tinygltf::Texture& ttexture = model.textures[i];

				if (ttexture.source < loaded_textures.size())
				{
					loaded_indices.push_back(ttexture.source);
					continue;
				}

				if (ttexture.sampler != -1)
					load_smp(ttexture.sampler);

				SFG_ASSERT(ttexture.source != -1);
				tinygltf::Image& img = model.images[ttexture.source];

				SFG_ASSERT(!img.image.empty());
				loaded_indices.push_back(loaded_textures.size());

				SFG_ASSERT(img.bits == 8 && img.component == 4);
				loaded_textures.push_back({});
				texture_raw& raw = loaded_textures.back();

				const string	hash_path = string(relative_path) + "/" + (img.name.empty() ? ttexture.name : img.name);
				const string_id hash	  = TO_SID(hash_path);
				raw.sid					  = hash;
				raw.name				  = hash_path;

				/*
				const string cache_path = engine_data::get().get_cache_dir() + std::to_string(raw.sid) + engine_data::CACHE_EXTENSION;
				if (file_system::exists(cache_path.c_str()))
				{
					istream stream = serialization::load_from_file(cache_path.c_str());
					raw.deserialize(stream);
					stream.destroy();
				}
				*/

				u8* data = reinterpret_cast<u8*>(SFG_MALLOC(img.image.size()));
				SFG_MEMCPY(data, img.image.data(), img.image.size());
				const vector2ui16 size = vector2ui16(static_cast<u16>(img.width), static_cast<u16>(img.height));
				const format	  fmt  = check_if_linear(i) ? format::r8g8b8a8_unorm : format::r8g8b8a8_srgb;
				raw.load_from_data(data, size, static_cast<u8>(fmt), true);
			}
		}

		if (create_materials)
		{
			// gotta have something.
			if (samplers.empty())
			{
				samplers.push_back({
					.anisotropy = 0,
					.min_lod	= 0.0f,
					.max_lod	= 10.0f,
					.lod_bias	= 0.0f,
					.flags		= sampler_flags::saf_min_anisotropic | sampler_flags::saf_mag_anisotropic | sampler_flags::saf_mip_linear,
					.address_u	= address_mode::repeat,
					.address_v	= address_mode::repeat,
					.address_w	= address_mode::repeat,
				});
			}

			auto find_sampler = [&](i32 texture_index) -> i32 { return loaded_sampler_indices.at(model.textures.at(texture_index).sampler); };

			auto get_tiling_offet_for_texture = [](tinygltf::ExtensionMap& map, u32& out_tiling, u32& out_offset) {
				auto ext_transform_base = map.find("KHR_texture_transform");
				if (ext_transform_base != map.end())
				{
					const tinygltf::Value& val = ext_transform_base->second;

					if (val.IsObject())
					{
						if (val.Has("offset"))
						{
							const tinygltf::Value&		  offset = val.Get("offset");
							const tinygltf::Value::Array& array	 = offset.Get<tinygltf::Value::Array>();
							out_offset							 = packing::pack_half2x16(static_cast<f32>(array[0].GetNumberAsDouble()), static_cast<f32>(array[1].GetNumberAsDouble()));
						}

						if (val.Has("scale"))
						{
							const tinygltf::Value&		  scale = val.Get("scale");
							const tinygltf::Value::Array& array = scale.Get<tinygltf::Value::Array>();
							out_tiling							= packing::pack_half2x16(static_cast<f32>(array[0].GetNumberAsDouble()), static_cast<f32>(array[1].GetNumberAsDouble()));
						}
					}
				}
			};

			const size_t all_materials_sz = model.materials.size();
			for (size_t i = 0; i < all_materials_sz; i++)
			{
				tinygltf::Material& tmat = model.materials[i];
				loaded_materials.push_back({});
				material_raw&	raw		  = loaded_materials.back();
				const string	hash_path = string(relative_path) + "/" + tmat.name;
				const string_id hash	  = TO_SID(hash_path);
				raw.sid					  = hash;
				raw.name				  = hash_path;

				i32 constructed_orm = -1;

				if (tmat.pbrMetallicRoughness.metallicRoughnessTexture.index == -1 && tmat.occlusionTexture.index != -1)
				{
					tinygltf::Texture& ttexture = model.textures[tmat.occlusionTexture.index];
					SFG_ASSERT(ttexture.source != -1);
					tinygltf::Image& img = model.images[ttexture.source];

					SFG_ASSERT(!img.image.empty());

					SFG_ASSERT(img.bits == 8 && img.component == 4);

					const int		width		 = img.width;
					const int		height		 = img.height;
					const size_t	pixels		 = width * height * 4;
					const string	name		 = (img.name.empty() ? ttexture.name : img.name) + "-constructed_rm";
					const string	hash_path	 = string(relative_path) + "/" + name;
					const string_id hash		 = TO_SID(hash_path);
					u8*				texture_data = reinterpret_cast<u8*>(SFG_MALLOC(pixels));

					for (u32 j = 0; j < height; j++)
					{
						for (u32 k = 0; k < width; k++)
						{
							const u32 pixel = width * j + k;

							const u8* src = &img.image[pixel * 4];
							u8*		  dst = texture_data + pixel * 4;

							dst[0] = src[0];
							dst[1] = 255;
							dst[2] = 0;
							dst[3] = 255;
						}
					}

					loaded_indices.push_back(loaded_textures.size());
					loaded_textures.push_back({});
					texture_raw& txt_raw = loaded_textures.back();
					txt_raw.load_from_data(texture_data, vector2ui16(width, height), (u8)format::r8g8b8a8_unorm, true);
					txt_raw.sid	 = hash;
					txt_raw.name = hash_path;

					constructed_orm = static_cast<i32>(loaded_textures.size() - 1);
				}

				// if orm is missing but occlusion is there, use occlusion as orm.
				const int base_index	 = tmat.pbrMetallicRoughness.baseColorTexture.index;
				const int normal_index	 = tmat.normalTexture.index;
				const int orm_index		 = tmat.pbrMetallicRoughness.metallicRoughnessTexture.index == -1 ? -1 : tmat.pbrMetallicRoughness.metallicRoughnessTexture.index;
				const int emissive_index = tmat.emissiveTexture.index;

				// get tiling and offset.
				u32 base_tiling		= packing::pack_half2x16(1.0f, 1.0f);
				u32 orm_tiling		= base_tiling;
				u32 normal_tiling	= base_tiling;
				u32 emissive_tiling = base_tiling;
				u32 base_offset = 0, normal_offset = 0, orm_offset = 0, emissive_offset = 0;
				get_tiling_offet_for_texture(tmat.pbrMetallicRoughness.baseColorTexture.extensions, base_tiling, base_offset);
				get_tiling_offet_for_texture(tmat.normalTexture.extensions, normal_tiling, normal_offset);
				get_tiling_offet_for_texture(tmat.pbrMetallicRoughness.metallicRoughnessTexture.extensions, orm_tiling, orm_offset);
				get_tiling_offet_for_texture(tmat.emissiveTexture.extensions, emissive_tiling, emissive_offset);

				const vector4 base_color = vector4(static_cast<f32>(tmat.pbrMetallicRoughness.baseColorFactor[0]),
												   static_cast<f32>(tmat.pbrMetallicRoughness.baseColorFactor[1]),
												   static_cast<f32>(tmat.pbrMetallicRoughness.baseColorFactor[2]),
												   static_cast<f32>(tmat.pbrMetallicRoughness.baseColorFactor[3]));

				const vector3 emissive		  = vector3(static_cast<f32>(tmat.emissiveFactor[0]), static_cast<f32>(tmat.emissiveFactor[1]), static_cast<f32>(tmat.emissiveFactor[2]));
				const f32	  metallic		  = static_cast<f32>(tmat.pbrMetallicRoughness.metallicFactor);
				const f32	  roughness		  = static_cast<f32>(tmat.pbrMetallicRoughness.roughnessFactor);
				const f32	  alpha_cutoff	  = static_cast<f32>(tmat.alphaCutoff);
				const f32	  normal_strength = tmat.normalTexture.scale;
				raw.pass_mode				  = tmat.alphaMode.compare("BLEND") == 0 ? material_pass_mode::forward : material_pass_mode::gbuffer;
				raw.use_alpha_cutoff		  = tmat.alphaMode.compare("MASK") == 0;
				raw.double_sided			  = tmat.doubleSided;
				const f32 pad				  = 0.0f;

				bool sampler_set = false;

				if (raw.pass_mode == material_pass_mode::forward)
				{
					raw.material_data << base_color;
					raw.material_data << base_tiling << base_offset;
					raw.textures.resize(1);

					if (base_index != -1)
						raw.sampler_definition = samplers.at(find_sampler(base_index));

					raw.textures[0] = base_index == -1 ? DUMMY_COLOR_TEXTURE_SID : loaded_textures[loaded_indices.at(base_index)].sid;
				}
				else
				{
					raw.material_data << base_color;
					raw.material_data << emissive;
					raw.material_data << metallic;
					raw.material_data << roughness;
					raw.material_data << normal_strength;
					raw.material_data << alpha_cutoff;
					raw.material_data << pad;
					raw.material_data << base_tiling << base_offset;
					raw.material_data << normal_tiling << normal_offset;
					raw.material_data << orm_tiling << orm_offset;
					raw.material_data << emissive_tiling << emissive_offset;

					if (base_index != -1)
					{
						raw.sampler_definition = samplers.at(find_sampler(base_index));
						sampler_set			   = true;
					}

					if (normal_index != -1 && !sampler_set)
					{
						raw.sampler_definition = samplers.at(find_sampler(normal_index));
						sampler_set			   = true;
					}

					if (orm_index != -1 && !sampler_set)
					{
						raw.sampler_definition = samplers.at(find_sampler(orm_index));
						sampler_set			   = true;
					}

					if (emissive_index != -1 && !sampler_set)
					{
						raw.sampler_definition = samplers.at(find_sampler(emissive_index));
						sampler_set			   = true;
					}

					raw.textures.resize(4);
					raw.textures[0] = base_index == -1 ? DUMMY_COLOR_TEXTURE_SID : loaded_textures[loaded_indices.at(base_index)].sid;
					raw.textures[1] = normal_index == -1 ? DUMMY_NORMAL_TEXTURE_SID : loaded_textures[loaded_indices.at(normal_index)].sid;
					raw.textures[2] = orm_index == -1 ? (constructed_orm == -1 ? DUMMY_ORM_TEXTURE_SID : loaded_textures[constructed_orm].sid) : loaded_textures[loaded_indices.at(orm_index)].sid;
					raw.textures[3] = emissive_index == -1 ? DUMMY_COLOR_TEXTURE_SID : loaded_textures[loaded_indices.at(emissive_index)].sid;
				}

				if (!sampler_set)
				{
					raw.sampler_definition = samplers.at(0);
				}

				raw.shader = raw.pass_mode == material_pass_mode::gbuffer ? DEFAULT_OPAQUE_SHADER_SID : DEFAULT_FORWARD_SHADER_SID;
				SFG_INFO("created material from gltf: {0}", raw.name);
			}
		}

		const size_t all_anims_sz = model.animations.size();

		for (size_t i = 0; i < all_anims_sz; i++)
		{
			tinygltf::Animation& tanim	   = model.animations[i];
			const string		 hash_path = string(relative_path) + "/" + tanim.name;
			const string_id		 hash	   = TO_SID(hash_path);
			loaded_animations.push_back({});
			animation_raw& anim = loaded_animations.back();
			anim.sid			= hash;
			anim.name			= hash_path;

			const size_t channels_sz = tanim.channels.size();

			for (size_t j = 0; j < channels_sz; j++)
			{
				tinygltf::AnimationChannel& tchannel = tanim.channels[j];

				const tinygltf::AnimationSampler sampler   = tanim.samplers[tchannel.sampler];
				const tinygltf::Accessor&		 input_a   = model.accessors[sampler.input];
				const tinygltf::BufferView&		 input_bv  = model.bufferViews[input_a.bufferView];
				const tinygltf::Buffer&			 input_b   = model.buffers[input_bv.buffer];
				const tinygltf::Accessor&		 output_a  = model.accessors[sampler.output];
				const tinygltf::BufferView&		 output_bv = model.bufferViews[output_a.bufferView];
				const tinygltf::Buffer&			 output_b  = model.buffers[output_bv.buffer];

				SFG_ASSERT((input_a.type == TINYGLTF_TYPE_SCALAR && input_a.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT), "Unsupported component type!");
				SFG_ASSERT(output_a.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Unsupported component type!");

				const size_t input_count  = input_a.count;
				const size_t output_count = output_a.count;

				vector<f32> keyframe_times = {};
				keyframe_times.resize(input_count);

				const size_t   stride = input_bv.byteStride == 0 ? sizeof(f32) : input_bv.byteStride;
				const uint8_t* base	  = input_b.data.data() + input_a.byteOffset + input_bv.byteOffset;

				for (size_t k = 0; k < input_count; k++)
				{
					const f32* raw	  = reinterpret_cast<const f32*>(base + k * stride);
					keyframe_times[k] = *raw;
					anim.duration	  = math::max(anim.duration, keyframe_times[k]);
				}

				animation_interpolation interpolation = animation_interpolation::linear;
				if (sampler.interpolation.compare("LINEAR") == 0)
					interpolation = animation_interpolation::linear;
				else if (sampler.interpolation.compare("STEP") == 0)
					interpolation = animation_interpolation::step;
				else if (sampler.interpolation.compare("CUBICSPLINE") == 0)
					interpolation = animation_interpolation::cubic_spline;

				if (interpolation == animation_interpolation::cubic_spline)
				{
					SFG_ASSERT(output_count == input_count * 3, "Output count mismatch for CubicSpline!");
				}
				else
				{
					SFG_ASSERT(input_count == output_count, "Input & output counts do not match!");
				}

				const f32* raw_float_data = reinterpret_cast<const f32*>(output_b.data.data() + output_a.byteOffset + output_bv.byteOffset);

				const bool is_translation = tchannel.target_path.compare("translation") == 0;
				const bool is_scale		  = tchannel.target_path.compare("scale") == 0;

				if (is_translation || is_scale)
				{
					animation_channel_v3_raw* channel = nullptr;

					if (is_translation)
					{
						anim.position_channels.push_back({});
						channel = &anim.position_channels.back();
					}
					else
					{
						anim.scale_channels.push_back({});
						channel = &anim.scale_channels.back();
					}

					channel->interpolation = interpolation;
					channel->node_index	   = static_cast<i16>(tchannel.target_node);

					if (interpolation == animation_interpolation::cubic_spline)
					{
						for (size_t k = 0; k < input_count; k++)
						{
							size_t base = k * 9;
							channel->keyframes_spline.push_back({});
							animation_keyframe_v3_spline& kf = channel->keyframes_spline.back();
							kf.in_tangent					 = vector3(raw_float_data[base], raw_float_data[base + 1], raw_float_data[base + 2]);
							kf.value						 = vector3(raw_float_data[base + 3], raw_float_data[base + 4], raw_float_data[base + 5]);
							kf.out_tangent					 = vector3(raw_float_data[base + 6], raw_float_data[base + 7], raw_float_data[base + 8]);
							kf.time							 = keyframe_times[k];
						}
					}
					else
					{
						for (size_t k = 0; k < input_count; k++)
						{
							size_t base = k * 3;
							channel->keyframes.push_back({});
							animation_keyframe_v3& kf = channel->keyframes.back();
							kf.value				  = vector3(raw_float_data[base], raw_float_data[base + 1], raw_float_data[base + 2]);
							kf.time					  = keyframe_times[k];
						}
					}
				}
				else if (tchannel.target_path.compare("rotation") == 0)
				{
					anim.rotation_channels.push_back({});
					animation_channel_q_raw& channel = anim.rotation_channels.back();
					channel.interpolation			 = interpolation;
					channel.node_index				 = static_cast<i16>(tchannel.target_node);

					if (interpolation == animation_interpolation::cubic_spline)
					{
						for (size_t k = 0; k < input_count; k++)
						{
							size_t base = k * 12;
							channel.keyframes_spline.push_back({});
							animation_keyframe_q_spline& kf = channel.keyframes_spline.back();

							kf.in_tangent  = quat(raw_float_data[base], raw_float_data[base + 1], raw_float_data[base + 2], raw_float_data[base + 3]);
							kf.value	   = quat(raw_float_data[base + 4], raw_float_data[base + 5], raw_float_data[base + 6], raw_float_data[base + 7]);
							kf.out_tangent = quat(raw_float_data[base + 8], raw_float_data[base + 9], raw_float_data[base + 10], raw_float_data[base + 11]);
							kf.time		   = keyframe_times[k];
						}
					}
					else
					{
						for (size_t k = 0; k < input_count; k++)
						{
							size_t base = k * 4;
							channel.keyframes.push_back({});
							animation_keyframe_q& kf = channel.keyframes.back();
							kf.value				 = quat(raw_float_data[base], raw_float_data[base + 1], raw_float_data[base + 2], raw_float_data[base + 3]);
							kf.time					 = keyframe_times[k];
						}
					}
				}
				else if (tchannel.target_path.compare("weights") == 0)
				{
				}
			}
		}

		const size_t all_lights_sz = model.lights.size();
		for (size_t i = 0; i < all_lights_sz; i++)
		{
			tinygltf::Light& gltf_light = model.lights[i];

			light_raw_type type = {};
			if (gltf_light.type.compare("point") == 0)
				type = light_raw_type::point;
			else if (gltf_light.type.compare("spot") == 0)
				type = light_raw_type::spot;
			else
				type = light_raw_type::sun;

			loaded_lights.push_back(light_raw{
				.base_color = color(gltf_light.color[0], gltf_light.color[1], gltf_light.color[2], 1.0f),
				.intensity	= static_cast<f32>(gltf_light.intensity * SFG_LIGHT_CANDELA_MULT),
				.range		= static_cast<f32>(gltf_light.range),
				.inner_cone = static_cast<f32>(gltf_light.spot.innerConeAngle),
				.outer_cone = static_cast<f32>(gltf_light.spot.outerConeAngle),
				.type		= type,
			});
		}

		total_aabb.update_half_extents();
		return true;
	}

	bool model_raw::load_from_file(const char* relative_file, const char* base_path)
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
			json		  json_data = json::parse(f);
			f.close();

			name   = relative_file;
			source = json_data.value<string>("source", "");

			const string full_source = base_path + source;
			if (!file_system::exists(full_source.c_str()))
			{
				SFG_ERR("File doesn't exists! {0}", full_source.c_str());
				return false;
			}

			const u8 import_pbr_materials = json_data.value<u8>("import_pbr_materials", 0);
			const u8 generate_colliders	  = json_data.value<u8>("generate_colliders", 0);
			const u8 import_textures	  = import_pbr_materials;

			const bool success = import_gtlf(full_source.c_str(), name.c_str(), import_pbr_materials, import_textures);
			if (!success)
			{
				return false;
			}

			if (generate_colliders != 0)
			{
				for (mesh_raw& mesh : loaded_meshes)
					build_mesh_colliders(mesh);
			}
		}
		catch (std::exception e)
		{
			SFG_ERR("failed loading: {0}", e.what());
			return false;
		}

		return true;
	}

	bool model_raw::load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension)
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
		u32	   loaded_textures_size		  = 0;
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

	void model_raw::save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const
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

	void model_raw::get_dependencies(vector<string>& out_deps) const
	{
		out_deps.push_back(source);
	}
#endif
}
