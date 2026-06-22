// Copyright (c) 2025 Inan Evin

#include "material.hpp"

#include "material_def.hpp"
#include "resource_manager.hpp"
#include "texture.hpp"
#include "texture_sampler.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/gfx/common/gfx_constants.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/render/render_resources.hpp>

namespace sfg
{
	bool material_loader_t::load(resource_entry_t&, resource_context_t&, ostream_t&)
	{
		return false;
	}

	namespace
	{
		enum material_resource_user_data_e : u32
		{
			material_resource_user_data_parameters = 1,
			material_resource_user_data_textures   = 2,
		};

		u32 get_material_parameter_size(material_parameter_type_e type)
		{
			switch (type)
			{
			case material_parameter_type_e::uint2:
			case material_parameter_type_e::vec2f:
				return sizeof(f32) * 2;
			case material_parameter_type_e::uint4:
			case material_parameter_type_e::vec4f:
				return sizeof(f32) * 4;
			default:
				return sizeof(f32);
			}
		}

		void write_material_parameter(u8*& dst, const material_runtime_parameter_t& parameter)
		{
			switch (parameter.type)
			{
			case material_parameter_type_e::u32: {
				const u32 value = static_cast<u32>(parameter.values[0]);
				SFG_MEMCPY(dst, &value, sizeof(value));
				dst += sizeof(value);
				break;
			}
			case material_parameter_type_e::uint2: {
				const u32 values[2] = {static_cast<u32>(parameter.values[0]), static_cast<u32>(parameter.values[1])};
				SFG_MEMCPY(dst, values, sizeof(values));
				dst += sizeof(values);
				break;
			}
			case material_parameter_type_e::uint4: {
				const u32 values[4] = {static_cast<u32>(parameter.values[0]), static_cast<u32>(parameter.values[1]), static_cast<u32>(parameter.values[2]), static_cast<u32>(parameter.values[3])};
				SFG_MEMCPY(dst, values, sizeof(values));
				dst += sizeof(values);
				break;
			}
			case material_parameter_type_e::i32: {
				const i32 value = static_cast<i32>(parameter.values[0]);
				SFG_MEMCPY(dst, &value, sizeof(value));
				dst += sizeof(value);
				break;
			}
			case material_parameter_type_e::vec2f:
				SFG_MEMCPY(dst, parameter.values, sizeof(f32) * 2);
				dst += sizeof(f32) * 2;
				break;
			case material_parameter_type_e::vec4f:
				SFG_MEMCPY(dst, parameter.values, sizeof(f32) * 4);
				dst += sizeof(f32) * 4;
				break;
			default:
				SFG_MEMCPY(dst, parameter.values, sizeof(f32));
				dst += sizeof(f32);
				break;
			}
		}
	}

	bool material_loader_t::load(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&	mem		= ctx.resource_manager.get_memory();
		material_runtime_t* runtime = mem.get<material_runtime_t>(entry.runtime);
		*runtime					= {};

		istream_t stream;
		stream.open(entry.load_data.data, entry.load_data.size);

		material_def_t material = {};
		if (!reflection_registry_t::get().deserialize_from_stream(type_id_t<material_def_t>::value, &material, stream))
			return false;

		if (material.parameters.size() > MATERIAL_MAX_PARAMETERS)
		{
			SFG_ERR("material has too many parameters: {0} / {1}", material.parameters.size(), MATERIAL_MAX_PARAMETERS);
			return false;
		}

		if (material.textures.size() > MATERIAL_MAX_TEXTURES)
		{
			SFG_ERR("material has too many textures: {0} / {1}", material.textures.size(), MATERIAL_MAX_TEXTURES);
			return false;
		}

		runtime->pass_flags		  = material.pass_flags;
		runtime->shader_guid	  = material.shader;
		runtime->sampler_guid	  = material.sampler;
		runtime->double_sided	  = material.double_sided ? 1 : 0;
		runtime->use_alpha_cutoff = material.use_alpha_cutoff ? 1 : 0;
		runtime->parameter_count  = static_cast<u32>(material.parameters.size());
		runtime->texture_count	  = static_cast<u32>(material.textures.size());
		SFG_ASSERT(runtime->texture_count == 0 || runtime->sampler_guid != NULL_SID);

		if (runtime->parameter_count != 0)
		{
			for (u32 i = 0; i < runtime->parameter_count; ++i)
			{
				SFG_ASSERT(material.parameters[i].values.size() == 4);
				runtime->parameters[i].type = material.parameters[i].type;
				SFG_MEMCPY(runtime->parameters[i].values, material.parameters[i].values.data(), sizeof(runtime->parameters[i].values));
				runtime->parameter_data_size += get_material_parameter_size(runtime->parameters[i].type);
			}
		}

		if (runtime->texture_count != 0)
		{
			for (u32 i = 0; i < runtime->texture_count; ++i)
				runtime->texture_guids[i] = material.textures[i];
		}

		return true;
	}

	create_internals_result_e material_loader_t::create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&	  mem		= ctx.resource_manager.get_memory();
		material_runtime_t*	  runtime	= mem.get<material_runtime_t>(entry.runtime);
		material_internals_t* internals = mem.get<material_internals_t>(entry.internals);
		*internals						= {};

		if (runtime->parameter_count != 0)
		{
			resource_desc_t desc = {};
			desc.size			 = runtime->parameter_data_size;
			desc.flags			 = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
			desc.set_name(mem.get_text(entry.debug_name));
			render_resources_t::get().enqueue_create_resource(entry.hash, entry.type, desc, material_resource_user_data_parameters);
			internals->pending_count++;
		}

		if (runtime->sampler_guid != NULL_SID)
		{
			const texture_sampler_internals_t* sampler = ctx.resource_manager.find_internals<texture_sampler_internals_t>(runtime->sampler_guid);
			SFG_ASSERT(sampler != nullptr);
			internals->sampler_index = sampler->gpu_index;
		}

		if (runtime->texture_count != 0)
		{
			SFG_ASSERT(internals->sampler_index != NULL_GPU_INDEX);
			const size_t texture_bytes = static_cast<size_t>(runtime->texture_count) * sizeof(gpu_index_t);
			for (u32 i = 0; i < runtime->texture_count; ++i)
			{
				const texture_internals_t* texture = ctx.resource_manager.find_internals<texture_internals_t>(runtime->texture_guids[i]);
				SFG_ASSERT(texture != nullptr);
				runtime->texture_indices[i] = texture->gpu_index;
			}

			resource_desc_t desc = {};
			desc.size			 = static_cast<u32>(texture_bytes);
			desc.flags			 = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
			desc.set_name(mem.get_text(entry.debug_name));
			render_resources_t::get().enqueue_create_resource(entry.hash, entry.type, desc, material_resource_user_data_textures);
			internals->pending_count++;
		}

		if (internals->pending_count == 0)
		{
			return create_internals_result_e::ready;
		}

		return create_internals_result_e::queued;
	}

	resource_ready_result_e material_loader_t::resource_ready(resource_entry_t& entry, resource_context_t& ctx, const render_resource_completion_t& completion)
	{
		chunk_allocator_t&	  mem		= ctx.resource_manager.get_memory();
		material_runtime_t*	  runtime	= mem.get<material_runtime_t>(entry.runtime);
		material_internals_t* internals = mem.get<material_internals_t>(entry.internals);

		SFG_ASSERT(completion.kind == render_resource_kind_e::resource);
		SFG_ASSERT(internals->pending_count > 0);

		if (completion.state == resource_state_e::failed)
		{
			internals->had_failure = 1;
		}
		else if (completion.user_data == material_resource_user_data_parameters)
		{
			SFG_ASSERT(runtime->parameter_data_size <= MATERIAL_MAX_PARAMETER_DATA_SIZE);
			u8	parameter_values[MATERIAL_MAX_PARAMETER_DATA_SIZE] = {};
			u8* dst												   = parameter_values;
			for (u32 i = 0; i < runtime->parameter_count; ++i)
				write_material_parameter(dst, runtime->parameters[i]);

			internals->parameter_buffer = completion.resource;
			internals->parameter_index	= completion.gpu_index;
			render_resources_t::get().enqueue_data_upload(internals->parameter_buffer, parameter_values, runtime->parameter_data_size);
		}
		else
		{
			SFG_ASSERT(completion.user_data == material_resource_user_data_textures);
			internals->texture_buffer = completion.resource;
			internals->texture_index  = completion.gpu_index;
			render_resources_t::get().enqueue_data_upload(internals->texture_buffer, runtime->texture_indices, runtime->texture_count * sizeof(gpu_index_t));
		}

		internals->pending_count--;
		if (internals->pending_count != 0)
			return resource_ready_result_e::pending;

		if (internals->had_failure)
		{
			render_resources_t::get().enqueue_destroy_resource(internals->parameter_buffer);
			render_resources_t::get().enqueue_destroy_resource(internals->texture_buffer);
			*internals = {};
			return resource_ready_result_e::failed;
		}

		return resource_ready_result_e::ready;
	}

	void material_loader_t::destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&	  mem		= ctx.resource_manager.get_memory();
		material_internals_t* internals = mem.get<material_internals_t>(entry.internals);
		render_resources_t::get().enqueue_destroy_resource(internals->parameter_buffer);
		render_resources_t::get().enqueue_destroy_resource(internals->texture_buffer);
		*internals = {};
	}

	const resource_type_desc_t material_resource_desc = {
		.type				 = resource_type_e::material,
		.runtime_size		 = sizeof(material_runtime_t),
		.runtime_alignment	 = alignof(material_runtime_t),
		.internals_size		 = sizeof(material_internals_t),
		.internals_alignment = alignof(material_internals_t),
		.wire_magic			 = material_loader_t::WIRE_MAGIC,
		.wire_version		 = material_loader_t::WIRE_VERSION,
		.initial_load_offset = 0,
		.initial_load_size	 = 0,
		.async_load_offset	 = 0,
		.async_load			 = false,
		.load				 = material_loader_t::load,
		.load_v2			 = material_loader_t::load,
		.create_internals	 = material_loader_t::create_internals,
		.resource_ready		 = material_loader_t::resource_ready,
		.destroy_internals	 = material_loader_t::destroy_internals,
	};
}
