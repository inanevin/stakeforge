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

#include "editor_glb_import_util.hpp"
#include "editor_glb_importer.hpp"

#include <sfg/data/string_view.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/mat4x3.hpp>
#include <sfg/math/quat.hpp>
#include <sfg/math/vec3f.hpp>
#include <sfg/math/vec4f.hpp>
#include <sfg/math/vec4u.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/resources/animation_def.hpp>
#include <sfg/runtime/resources/mesh_def.hpp>
#include <sfg/runtime/resources/mesh_util.hpp>
#include <sfg/runtime/resources/physics_collision_mesh_def.hpp>
#include <sfg/vendor/tinygltf/tiny_gltf_v3.h>

namespace sfg
{
	namespace
	{
		const tg3_accessor* get_accessor(const tg3_model& model, i32 accessor_index)
		{
			if (accessor_index < 0 || static_cast<u32>(accessor_index) >= model.accessors_count)
				return nullptr;

			return &model.accessors[accessor_index];
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

		bool read_float_attribute(const tg3_model& model, i32 accessor_index, i32 type, u32 expected_count, vector_t<f32>& out)
		{
			const tg3_accessor* accessor = get_accessor(model, accessor_index);
			if (accessor == nullptr || accessor->component_type != TG3_COMPONENT_TYPE_FLOAT || accessor->type != type || accessor->count != expected_count)
			{
				SFG_ERR("glb float attribute accessor is invalid: {0}", accessor_index);
				return false;
			}

			const tg3_buffer_view* buffer_view = nullptr;
			const u8*			   data		   = get_accessor_data(model, *accessor, buffer_view);
			if (data == nullptr)
			{
				SFG_ERR("glb float attribute data is invalid: {0}", accessor_index);
				return false;
			}

			const i32 components = tg3_num_components(type);
			const i32 stride	 = tg3_accessor_byte_stride(accessor, buffer_view);
			if (components <= 0 || stride < components * static_cast<i32>(sizeof(f32)))
			{
				SFG_ERR("glb float attribute stride is invalid: {0}", accessor_index);
				return false;
			}

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
			{
				SFG_ERR("glb index accessor is invalid: {0}", accessor_index);
				return false;
			}

			const tg3_buffer_view* buffer_view = nullptr;
			const u8*			   data		   = get_accessor_data(model, *accessor, buffer_view);
			if (data == nullptr)
			{
				SFG_ERR("glb index data is invalid: {0}", accessor_index);
				return false;
			}

			const i32 component_size = tg3_component_size(accessor->component_type);
			const i32 stride		 = tg3_accessor_byte_stride(accessor, buffer_view);
			if (component_size <= 0 || stride < component_size)
			{
				SFG_ERR("glb index stride is invalid: {0}", accessor_index);
				return false;
			}

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
					SFG_ERR("glb index component type is unsupported: {0}", accessor->component_type);
					return false;
				}
			}

			return true;
		}

		bool read_joint_attribute(const tg3_model& model, i32 accessor_index, u32 vertex_count, vector_t<vec4u_t>& out)
		{
			const tg3_accessor* accessor = get_accessor(model, accessor_index);
			if (accessor == nullptr || accessor->type != TG3_TYPE_VEC4 || accessor->count != vertex_count)
			{
				SFG_ERR("glb joint accessor is invalid: {0}", accessor_index);
				return false;
			}

			const tg3_buffer_view* buffer_view = nullptr;
			const u8*			   data		   = get_accessor_data(model, *accessor, buffer_view);
			if (data == nullptr)
			{
				SFG_ERR("glb joint data is invalid: {0}", accessor_index);
				return false;
			}

			const i32 component_size = tg3_component_size(accessor->component_type);
			const i32 stride		 = tg3_accessor_byte_stride(accessor, buffer_view);
			if (component_size <= 0 || stride < component_size * 4)
			{
				SFG_ERR("glb joint stride is invalid: {0}", accessor_index);
				return false;
			}

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
					SFG_ERR("glb joint component type is unsupported: {0}", accessor->component_type);
					return false;
				}
			}

			return true;
		}

		u32 get_primitive_vertex_count(const tg3_model& model, const tg3_primitive& primitive)
		{
			const tg3_accessor* accessor = get_accessor(model, editor_glb_import_util_t::find_attribute(primitive, "POSITION"));
			if (accessor == nullptr || accessor->count > UINT32_MAX)
				return 0;

			return static_cast<u32>(accessor->count);
		}
	}

	glb_basis_conversion_t editor_glb_import_util_t::make_basis(glb_axis_e up_axis, glb_axis_e forward_axis)
	{
		vec3f_t source_up	   = vec3f_t::zero;
		vec3f_t source_forward = vec3f_t::zero;

		switch (up_axis)
		{
		case glb_axis_e::positive_x:
			source_up = {1.0f, 0.0f, 0.0f};
			break;
		case glb_axis_e::negative_x:
			source_up = {-1.0f, 0.0f, 0.0f};
			break;
		case glb_axis_e::positive_y:
			source_up = {0.0f, 1.0f, 0.0f};
			break;
		case glb_axis_e::negative_y:
			source_up = {0.0f, -1.0f, 0.0f};
			break;
		case glb_axis_e::positive_z:
			source_up = {0.0f, 0.0f, 1.0f};
			break;
		case glb_axis_e::negative_z:
			source_up = {0.0f, 0.0f, -1.0f};
			break;
		}

		switch (forward_axis)
		{
		case glb_axis_e::positive_x:
			source_forward = {1.0f, 0.0f, 0.0f};
			break;
		case glb_axis_e::negative_x:
			source_forward = {-1.0f, 0.0f, 0.0f};
			break;
		case glb_axis_e::positive_y:
			source_forward = {0.0f, 1.0f, 0.0f};
			break;
		case glb_axis_e::negative_y:
			source_forward = {0.0f, -1.0f, 0.0f};
			break;
		case glb_axis_e::positive_z:
			source_forward = {0.0f, 0.0f, 1.0f};
			break;
		case glb_axis_e::negative_z:
			source_forward = {0.0f, 0.0f, -1.0f};
			break;
		}

		const vec3f_t source_right = vec3f_t::cross(source_forward, source_up);

		const mat4x3_t source_basis(source_right.x, source_right.y, source_right.z, source_up.x, source_up.y, source_up.z, source_forward.x, source_forward.y, source_forward.z, 0.0f, 0.0f, 0.0f);
		const mat4x3_t target_basis(vec3f_t::right.x, vec3f_t::right.y, vec3f_t::right.z, vec3f_t::up.x, vec3f_t::up.y, vec3f_t::up.z, vec3f_t::forward.x, vec3f_t::forward.y, vec3f_t::forward.z, 0.0f, 0.0f, 0.0f);
		const mat4x3_t transform = target_basis * source_basis.inverse();

		return {
			.transform		   = transform,
			.inverse_transform = transform.inverse(),
		};
	}

	vec3f_t editor_glb_import_util_t::convert_vector(const glb_basis_conversion_t& basis, const vec3f_t& value)
	{
		return basis.transform * value;
	}

	vec3f_t editor_glb_import_util_t::convert_scale(const glb_basis_conversion_t& basis, const vec3f_t& value)
	{
		return mat3x3_t::abs(basis.transform.to_linear3x3()) * value;
	}

	vec4f_t editor_glb_import_util_t::convert_tangent(const glb_basis_conversion_t& basis, const vec4f_t& value)
	{
		const vec3f_t tangent = convert_vector(basis, {value.x, value.y, value.z});

		return {tangent.x, tangent.y, tangent.z, value.w};
	}

	quat_t editor_glb_import_util_t::convert_rotation(const glb_basis_conversion_t& basis, const quat_t& value)
	{
		quat_t converted = convert_rotation_tangent(basis, value);

		converted.normalize();

		return converted;
	}

	quat_t editor_glb_import_util_t::convert_rotation_tangent(const glb_basis_conversion_t& basis, const quat_t& value)
	{
		const mat3x3_t transform		= basis.transform.to_linear3x3();
		const vec3f_t  converted_vector = transform * vec3f_t(value.x, value.y, value.z);
		const f32	   sign				= transform.determinant() < 0.0f ? -1.0f : 1.0f;

		return {converted_vector.x * sign, converted_vector.y * sign, converted_vector.z * sign, value.w};
	}

	mat4x3_t editor_glb_import_util_t::convert_transform(const glb_basis_conversion_t& basis, const mat4x3_t& value)
	{
		return basis.transform * value * basis.inverse_transform;
	}

	i32 editor_glb_import_util_t::find_attribute(const tg3_primitive& primitive, const char* name)
	{
		for (u32 i = 0; i < primitive.attributes_count; ++i)
		{
			const tg3_str_int_pair& attr = primitive.attributes[i];
			if (attr.key.data != nullptr && string_view_t(attr.key.data, attr.key.len) == name)
				return attr.value;
		}
		return -1;
	}

	bool editor_glb_import_util_t::import_animation(const tg3_model& model, const tg3_animation& animation, const glb_basis_conversion_t& basis, animation_def_t& out)
	{
		if (model.skins_count == 0)
		{
			SFG_ERR("glb animation has no skeleton");
			return false;
		}

		vector_t<u32> node_to_joint_index = {};

		node_to_joint_index.resize(model.nodes_count);

		bool found_compatible_skin = false;

		// find a skin that contains every transform channel
		for (u32 skin_index = 0; skin_index < model.skins_count; ++skin_index)
		{
			for (u32& joint_index : node_to_joint_index)
				joint_index = UINT32_MAX;

			const tg3_skin& skin		  = model.skins[skin_index];
			bool			skin_is_valid = true;

			for (u32 joint_index = 0; joint_index < skin.joints_count; ++joint_index)
			{
				const i32 node_index = skin.joints[joint_index];

				if (node_index < 0 || static_cast<u32>(node_index) >= model.nodes_count || node_to_joint_index[node_index] != UINT32_MAX)
				{
					skin_is_valid = false;
					break;
				}

				node_to_joint_index[node_index] = joint_index;
			}

			if (!skin_is_valid)
				continue;

			for (u32 channel_index = 0; channel_index < animation.channels_count; ++channel_index)
			{
				const tg3_animation_channel& channel = animation.channels[channel_index];
				const string_view_t			 path(channel.target.path.data, channel.target.path.len);

				if (path == "weights")
					continue;

				if (channel.target.node < 0 || static_cast<u32>(channel.target.node) >= model.nodes_count || node_to_joint_index[channel.target.node] == UINT32_MAX)
				{
					skin_is_valid = false;
					break;
				}
			}

			if (skin_is_valid)
			{
				found_compatible_skin = true;
				break;
			}
		}

		if (!found_compatible_skin)
		{
			SFG_ERR("glb animation does not target a compatible skeleton");
			return false;
		}

		u32 position_channel_count = 0;
		u32 rotation_channel_count = 0;
		u32 scale_channel_count	   = 0;

		for (u32 channel_index = 0; channel_index < animation.channels_count; ++channel_index)
		{
			const tg3_animation_channel& channel = animation.channels[channel_index];
			const string_view_t			 path(channel.target.path.data, channel.target.path.len);

			if (path == "translation")
				++position_channel_count;
			else if (path == "rotation")
				++rotation_channel_count;
			else if (path == "scale")
				++scale_channel_count;
			else if (path != "weights")
			{
				SFG_ERR("glb animation channel path is unsupported");
				return false;
			}
		}

		out.position_channels.reserve(position_channel_count);
		out.rotation_channels.reserve(rotation_channel_count);
		out.scale_channels.reserve(scale_channel_count);

		vector_t<f32> keyframe_times  = {};
		vector_t<f32> keyframe_values = {};

		for (u32 channel_index = 0; channel_index < animation.channels_count; ++channel_index)
		{
			const tg3_animation_channel& source_channel = animation.channels[channel_index];
			const string_view_t			 path(source_channel.target.path.data, source_channel.target.path.len);

			if (path == "weights")
				continue;

			if (source_channel.sampler < 0 || static_cast<u32>(source_channel.sampler) >= animation.samplers_count)
			{
				SFG_ERR("glb animation channel sampler is out of range: {0}", source_channel.sampler);
				return false;
			}

			const tg3_animation_sampler& sampler		 = animation.samplers[source_channel.sampler];
			const tg3_accessor*			 input_accessor	 = get_accessor(model, sampler.input);
			const tg3_accessor*			 output_accessor = get_accessor(model, sampler.output);

			if (input_accessor == nullptr || input_accessor->component_type != TG3_COMPONENT_TYPE_FLOAT || input_accessor->type != TG3_TYPE_SCALAR || input_accessor->count == 0 || input_accessor->count > UINT32_MAX)
			{
				SFG_ERR("glb animation input accessor is invalid: {0}", sampler.input);
				return false;
			}

			const i32 output_type = path == "rotation" ? TG3_TYPE_VEC4 : TG3_TYPE_VEC3;

			if (output_accessor == nullptr || output_accessor->component_type != TG3_COMPONENT_TYPE_FLOAT || output_accessor->type != output_type)
			{
				SFG_ERR("glb animation output accessor is invalid: {0}", sampler.output);
				return false;
			}

			animation_interpolation_e interpolation = animation_interpolation_e::linear;
			const string_view_t		  interpolation_name(sampler.interpolation.data, sampler.interpolation.len);

			if (interpolation_name == "STEP")
				interpolation = animation_interpolation_e::step;
			else if (interpolation_name == "CUBICSPLINE")
				interpolation = animation_interpolation_e::cubic_spline;
			else if (interpolation_name != "LINEAR")
			{
				SFG_ERR("glb animation interpolation is unsupported");
				return false;
			}

			const u32 keyframe_count = static_cast<u32>(input_accessor->count);

			if (interpolation == animation_interpolation_e::cubic_spline && keyframe_count > UINT32_MAX / 3)
			{
				SFG_ERR("glb animation keyframe count is too large");
				return false;
			}

			const u32 output_count = interpolation == animation_interpolation_e::cubic_spline ? keyframe_count * 3 : keyframe_count;

			keyframe_times.resize(0);
			keyframe_values.resize(0);

			if (!read_float_attribute(model, sampler.input, TG3_TYPE_SCALAR, keyframe_count, keyframe_times) || !read_float_attribute(model, sampler.output, output_type, output_count, keyframe_values))
			{
				SFG_ERR("failed to read GLB animation sampler {0}", source_channel.sampler);
				return false;
			}

			for (u32 keyframe_index = 0; keyframe_index < keyframe_count; ++keyframe_index)
			{
				if (keyframe_index != 0 && keyframe_times[keyframe_index] <= keyframe_times[keyframe_index - 1])
				{
					SFG_ERR("glb animation keyframe times are not increasing");
					return false;
				}

				if (keyframe_times[keyframe_index] > out.duration)
					out.duration = keyframe_times[keyframe_index];
			}

			const i32 joint_index = static_cast<i32>(node_to_joint_index[source_channel.target.node]);

			if (path == "rotation")
			{
				animation_channel_q_def_t& channel = out.rotation_channels.emplace_back();

				channel.interpolation = interpolation;
				channel.node_index	  = joint_index;

				if (interpolation == animation_interpolation_e::cubic_spline)
				{
					channel.keyframes_spline.resize(keyframe_count);

					for (u32 keyframe_index = 0; keyframe_index < keyframe_count; ++keyframe_index)
					{
						const size_t				   base		= static_cast<size_t>(keyframe_index) * 12;
						animation_keyframe_q_spline_t& keyframe = channel.keyframes_spline[keyframe_index];

						keyframe.in_tangent	 = convert_rotation_tangent(basis, {keyframe_values[base], keyframe_values[base + 1], keyframe_values[base + 2], keyframe_values[base + 3]});
						keyframe.value		 = convert_rotation(basis, {keyframe_values[base + 4], keyframe_values[base + 5], keyframe_values[base + 6], keyframe_values[base + 7]});
						keyframe.out_tangent = convert_rotation_tangent(basis, {keyframe_values[base + 8], keyframe_values[base + 9], keyframe_values[base + 10], keyframe_values[base + 11]});
						keyframe.time		 = keyframe_times[keyframe_index];
					}
				}
				else
				{
					channel.keyframes.resize(keyframe_count);

					for (u32 keyframe_index = 0; keyframe_index < keyframe_count; ++keyframe_index)
					{
						const size_t			base	 = static_cast<size_t>(keyframe_index) * 4;
						animation_keyframe_q_t& keyframe = channel.keyframes[keyframe_index];

						keyframe.value = convert_rotation(basis, {keyframe_values[base], keyframe_values[base + 1], keyframe_values[base + 2], keyframe_values[base + 3]});
						keyframe.time  = keyframe_times[keyframe_index];
					}
				}

				continue;
			}

			animation_channel_v3_def_t& channel = path == "translation" ? out.position_channels.emplace_back() : out.scale_channels.emplace_back();

			channel.interpolation = interpolation;
			channel.node_index	  = joint_index;

			if (interpolation == animation_interpolation_e::cubic_spline)
			{
				channel.keyframes_spline.resize(keyframe_count);

				for (u32 keyframe_index = 0; keyframe_index < keyframe_count; ++keyframe_index)
				{
					const size_t					base	 = static_cast<size_t>(keyframe_index) * 9;
					animation_keyframe_v3_spline_t& keyframe = channel.keyframes_spline[keyframe_index];
					const vec3f_t					in_tangent(keyframe_values[base], keyframe_values[base + 1], keyframe_values[base + 2]);
					const vec3f_t					value(keyframe_values[base + 3], keyframe_values[base + 4], keyframe_values[base + 5]);
					const vec3f_t					out_tangent(keyframe_values[base + 6], keyframe_values[base + 7], keyframe_values[base + 8]);

					if (path == "translation")
					{
						keyframe.in_tangent	 = convert_vector(basis, in_tangent);
						keyframe.value		 = convert_vector(basis, value);
						keyframe.out_tangent = convert_vector(basis, out_tangent);
					}
					else
					{
						keyframe.in_tangent	 = convert_scale(basis, in_tangent);
						keyframe.value		 = convert_scale(basis, value);
						keyframe.out_tangent = convert_scale(basis, out_tangent);
					}

					keyframe.time = keyframe_times[keyframe_index];
				}
			}
			else
			{
				channel.keyframes.resize(keyframe_count);

				for (u32 keyframe_index = 0; keyframe_index < keyframe_count; ++keyframe_index)
				{
					const size_t			 base	  = static_cast<size_t>(keyframe_index) * 3;
					animation_keyframe_v3_t& keyframe = channel.keyframes[keyframe_index];
					const vec3f_t			 value(keyframe_values[base], keyframe_values[base + 1], keyframe_values[base + 2]);

					keyframe.value = path == "translation" ? convert_vector(basis, value) : convert_scale(basis, value);
					keyframe.time  = keyframe_times[keyframe_index];
				}
			}
		}

		if (out.position_channels.empty() && out.rotation_channels.empty() && out.scale_channels.empty())
		{
			SFG_ERR("glb animation has no supported channels");
			return false;
		}

		return true;
	}

	bool editor_glb_import_util_t::read_inverse_bind_matrix(const tg3_model& model, const tg3_skin& skin, const glb_basis_conversion_t& basis, u32 joint_index, mat4x3_t& out_matrix)
	{
		out_matrix = mat4x3_t::identity;

		if (skin.inverse_bind_matrices < 0)
			return true;

		if (static_cast<u32>(skin.inverse_bind_matrices) >= model.accessors_count)
		{
			SFG_ERR("glb inverse bind matrix accessor is out of range");
			return false;
		}

		const tg3_accessor& accessor = model.accessors[skin.inverse_bind_matrices];

		if (accessor.component_type != TG3_COMPONENT_TYPE_FLOAT || accessor.type != TG3_TYPE_MAT4 || accessor.count <= joint_index || accessor.buffer_view < 0 || accessor.sparse.is_sparse != 0)
		{
			SFG_ERR("glb inverse bind matrix accessor has unsupported layout");
			return false;
		}

		if (static_cast<u32>(accessor.buffer_view) >= model.buffer_views_count)
		{
			SFG_ERR("glb inverse bind matrix buffer view is out of range");
			return false;
		}

		const tg3_buffer_view& buffer_view = model.buffer_views[accessor.buffer_view];

		if (buffer_view.buffer < 0 || static_cast<u32>(buffer_view.buffer) >= model.buffers_count)
		{
			SFG_ERR("glb inverse bind matrix buffer is out of range");
			return false;
		}

		const tg3_buffer& buffer = model.buffers[buffer_view.buffer];

		if (buffer.data.data == nullptr)
		{
			SFG_ERR("glb inverse bind matrix buffer has no data");
			return false;
		}

		const i32 stride = tg3_accessor_byte_stride(&accessor, &buffer_view);

		if (stride < static_cast<i32>(sizeof(f32) * 16))
		{
			SFG_ERR("glb inverse bind matrix stride is too small");
			return false;
		}

		const u64 matrix_offset = buffer_view.byte_offset + accessor.byte_offset + static_cast<u64>(stride) * joint_index;

		if (matrix_offset > buffer.data.count || sizeof(f32) * 16 > buffer.data.count - matrix_offset)
		{
			SFG_ERR("glb inverse bind matrix data is out of range");
			return false;
		}

		f32 matrix[16] = {};
		SFG_MEMCPY(matrix, buffer.data.data + matrix_offset, sizeof(matrix));

		out_matrix = convert_transform(basis, mat4x3_t(matrix[0], matrix[1], matrix[2], matrix[4], matrix[5], matrix[6], matrix[8], matrix[9], matrix[10], matrix[12], matrix[13], matrix[14]));
		return true;
	}

	bool editor_glb_import_util_t::import_static_primitive(const tg3_model& model, const tg3_primitive& primitive, const glb_basis_conversion_t& basis, u32 material_index, primitive_static_def_t& out)
	{
		if (primitive.mode != TG3_MODE_TRIANGLES)
		{
			SFG_ERR("glb primitive mode is unsupported: {0}", primitive.mode);
			return false;
		}

		const u32 vertex_count = get_primitive_vertex_count(model, primitive);

		if (vertex_count == 0)
		{
			SFG_ERR("glb primitive has no vertices");
			return false;
		}

		vector_t<f32> positions = {};

		if (!read_float_attribute(model, find_attribute(primitive, "POSITION"), TG3_TYPE_VEC3, vertex_count, positions))
		{
			SFG_ERR("failed to read GLB POSITION attribute");
			return false;
		}

		vector_t<f32> normals		  = {};
		const i32	  normal_accessor = find_attribute(primitive, "NORMAL");

		if (normal_accessor >= 0 && !read_float_attribute(model, normal_accessor, TG3_TYPE_VEC3, vertex_count, normals))
		{
			SFG_ERR("failed to read GLB NORMAL attribute");
			return false;
		}

		vector_t<f32> tangents		   = {};
		const i32	  tangent_accessor = find_attribute(primitive, "TANGENT");

		if (tangent_accessor >= 0 && !read_float_attribute(model, tangent_accessor, TG3_TYPE_VEC4, vertex_count, tangents))
		{
			SFG_ERR("failed to read GLB TANGENT attribute");
			return false;
		}

		vector_t<f32> uvs		  = {};
		const i32	  uv_accessor = find_attribute(primitive, "TEXCOORD_0");

		if (uv_accessor >= 0 && !read_float_attribute(model, uv_accessor, TG3_TYPE_VEC2, vertex_count, uvs))
		{
			SFG_ERR("failed to read GLB TEXCOORD_0 attribute");
			return false;
		}

		out.vertices.resize(vertex_count);
		out.material_index = material_index;

		for (u32 i = 0; i < vertex_count; ++i)
		{
			vertex_static_t& vertex = out.vertices[i];
			vertex.pos				= convert_vector(basis, {positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]});

			if (!normals.empty())
				vertex.normal = convert_vector(basis, {normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]});

			if (!tangents.empty())
				vertex.tangent = convert_tangent(basis, {tangents[i * 4], tangents[i * 4 + 1], tangents[i * 4 + 2], tangents[i * 4 + 3]});

			if (!uvs.empty())
				vertex.uv = {uvs[i * 2], uvs[i * 2 + 1]};
		}

		if (!read_indices(model, primitive.indices, vertex_count, out.indices))
		{
			SFG_ERR("failed to read GLB static primitive indices");
			return false;
		}

		if (tangent_accessor < 0 && normal_accessor >= 0 && uv_accessor >= 0 && !mesh_util_t::generate_tangents(out))
		{
			SFG_ERR("failed to generate GLB static primitive tangents");
			return false;
		}

		return true;
	}

	bool editor_glb_import_util_t::import_collision_primitive(const tg3_model& model, const tg3_primitive& primitive, const glb_basis_conversion_t& basis, physics_collision_mesh_def_t& out)
	{
		if (primitive.mode != TG3_MODE_TRIANGLES)
		{
			SFG_ERR("glb collision primitive mode is unsupported: {0}", primitive.mode);
			return false;
		}

		const u32 vertex_count = get_primitive_vertex_count(model, primitive);

		if (vertex_count == 0 || out.vertices.size() > UINT32_MAX - vertex_count)
		{
			SFG_ERR("glb collision primitive has invalid vertex count");
			return false;
		}

		vector_t<f32> positions = {};

		if (!read_float_attribute(model, find_attribute(primitive, "POSITION"), TG3_TYPE_VEC3, vertex_count, positions))
		{
			SFG_ERR("failed to read GLB collision POSITION attribute");
			return false;
		}

		vector_t<primitive_index> indices = {};

		if (!read_indices(model, primitive.indices, vertex_count, indices) || indices.empty() || indices.size() % 3 != 0)
		{
			SFG_ERR("failed to read GLB collision primitive indices");
			return false;
		}

		const primitive_index base_vertex = static_cast<primitive_index>(out.vertices.size());
		out.vertices.reserve(out.vertices.size() + vertex_count);

		for (u32 i = 0; i < vertex_count; ++i)
			out.vertices.push_back(convert_vector(basis, {positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]}));

		out.indices.reserve(out.indices.size() + indices.size());

		for (primitive_index index : indices)
			out.indices.push_back(base_vertex + index);

		return true;
	}

	bool editor_glb_import_util_t::import_skinned_primitive(const tg3_model& model, const tg3_primitive& primitive, const glb_basis_conversion_t& basis, u32 material_index, primitive_skinned_def_t& out)
	{
		if (primitive.mode != TG3_MODE_TRIANGLES)
		{
			SFG_ERR("glb primitive mode is unsupported: {0}", primitive.mode);
			return false;
		}

		const u32 vertex_count = get_primitive_vertex_count(model, primitive);

		if (vertex_count == 0)
		{
			SFG_ERR("glb primitive has no vertices");
			return false;
		}

		vector_t<f32> positions = {};

		if (!read_float_attribute(model, find_attribute(primitive, "POSITION"), TG3_TYPE_VEC3, vertex_count, positions))
		{
			SFG_ERR("failed to read GLB POSITION attribute");
			return false;
		}

		vector_t<f32> normals		  = {};
		const i32	  normal_accessor = find_attribute(primitive, "NORMAL");

		if (normal_accessor >= 0 && !read_float_attribute(model, normal_accessor, TG3_TYPE_VEC3, vertex_count, normals))
		{
			SFG_ERR("failed to read GLB NORMAL attribute");
			return false;
		}

		vector_t<f32> tangents		   = {};
		const i32	  tangent_accessor = find_attribute(primitive, "TANGENT");

		if (tangent_accessor >= 0 && !read_float_attribute(model, tangent_accessor, TG3_TYPE_VEC4, vertex_count, tangents))
		{
			SFG_ERR("failed to read GLB TANGENT attribute");
			return false;
		}

		vector_t<f32> uvs		  = {};
		const i32	  uv_accessor = find_attribute(primitive, "TEXCOORD_0");

		if (uv_accessor >= 0 && !read_float_attribute(model, uv_accessor, TG3_TYPE_VEC2, vertex_count, uvs))
		{
			SFG_ERR("failed to read GLB TEXCOORD_0 attribute");
			return false;
		}

		vector_t<f32> weights		   = {};
		const i32	  weights_accessor = find_attribute(primitive, "WEIGHTS_0");

		if (weights_accessor >= 0 && !read_float_attribute(model, weights_accessor, TG3_TYPE_VEC4, vertex_count, weights))
		{
			SFG_ERR("failed to read GLB WEIGHTS_0 attribute");
			return false;
		}

		vector_t<vec4u_t> joints		  = {};
		const i32		  joints_accessor = find_attribute(primitive, "JOINTS_0");

		if (joints_accessor >= 0 && !read_joint_attribute(model, joints_accessor, vertex_count, joints))
		{
			SFG_ERR("failed to read GLB JOINTS_0 attribute");
			return false;
		}

		out.vertices.resize(vertex_count);
		out.material_index = material_index;

		for (u32 i = 0; i < vertex_count; ++i)
		{
			vertex_skinned_t& vertex = out.vertices[i];
			vertex.pos				 = convert_vector(basis, {positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]});

			if (!normals.empty())
				vertex.normal = convert_vector(basis, {normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]});

			if (!tangents.empty())
				vertex.tangent = convert_tangent(basis, {tangents[i * 4], tangents[i * 4 + 1], tangents[i * 4 + 2], tangents[i * 4 + 3]});

			if (!uvs.empty())
				vertex.uv = {uvs[i * 2], uvs[i * 2 + 1]};

			if (!weights.empty())
				vertex.bone_weights = {weights[i * 4], weights[i * 4 + 1], weights[i * 4 + 2], weights[i * 4 + 3]};

			if (!joints.empty())
				vertex.bone_indices = joints[i];
		}

		if (!read_indices(model, primitive.indices, vertex_count, out.indices))
		{
			SFG_ERR("failed to read GLB skinned primitive indices");
			return false;
		}

		if (tangent_accessor < 0 && normal_accessor >= 0 && uv_accessor >= 0 && !mesh_util_t::generate_tangents(out))
		{
			SFG_ERR("failed to generate GLB skinned primitive tangents");
			return false;
		}

		return true;
	}
}
