// Copyright (c) 2025 Inan Evin

#include "material.hpp"

#include "material_def.hpp"
#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include "texture.hpp"
#include "shader.hpp"
#include "texture_sampler.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/reflection/reflection_registry.hpp>
#include <sfg/runtime/render/render_resources.hpp>

namespace sfg
{
	namespace
	{
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

	bool material_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs)
	{
		ostream_t file_stream;
		if (!rfs.read_resource(entry.hash, sizeof(resource_header_t), 0, file_stream))
		{
			SFG_ERR("failed to read material resource: {0}", entry.hash);
			return false;
		}

		istream_t stream;
		stream.open(file_stream.get_raw(), file_stream.get_size());

		chunk_allocator_t&	  mem		= ctx.resource_manager.get_memory();
		material_runtime_t*	  runtime	= mem.get<material_runtime_t>(entry.runtime);
		material_internals_t* internals = mem.get<material_internals_t>(entry.internals);
		*runtime						= {};
		*internals						= {};

		material_def_t material = {};
		if (!reflection_registry_t::get().type_from_stream(type_id_t<material_def_t>::value, &material, nullptr, stream))
		{
			SFG_ERR("failed to deserialize material definition: {0}", entry.hash);
			return false;
		}

		if (material.parameters.size() > SFG_MATERIAL_MAX_PARAMS)
		{
			SFG_ERR("material has too many parameters: {0} / {1}", material.parameters.size(), SFG_MATERIAL_MAX_PARAMS);
			return false;
		}

		if (material.textures.size() > SFG_MATERIAL_MAX_TEXTURES)
		{
			SFG_ERR("material has too many textures: {0} / {1}", material.textures.size(), SFG_MATERIAL_MAX_TEXTURES);
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
		SFG_ASSERT(runtime->shader_guid);

		if (runtime->parameter_count != 0)
		{
			for (u32 i = 0; i < runtime->parameter_count; ++i)
			{
				SFG_ASSERT(material.parameters[i].values.size() == 4);
				runtime->parameters[i].type = material.parameters[i].type;
				SFG_MEMCPY(runtime->parameters[i].values, material.parameters[i].values.data(), sizeof(runtime->parameters[i].values));
				runtime->parameter_data_size += get_material_parameter_size(runtime->parameters[i].type);
			}

			resource_desc_t desc = {};
			desc.size			 = runtime->parameter_data_size;
			desc.flags			 = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
			desc.set_name(mem.get_text(entry.debug_name));
			internals->parameter_buffer = render_resources_t::get().enqueue_create_resource(entry.hash, entry.type, desc);

			SFG_ASSERT(runtime->parameter_data_size <= SFG_MATERIAL_MAX_PARAMETER_DATA_SIZE);
			u8	parameter_values[SFG_MATERIAL_MAX_PARAMETER_DATA_SIZE] = {};
			u8* dst													   = parameter_values;
			for (u32 i = 0; i < runtime->parameter_count; ++i)
				write_material_parameter(dst, runtime->parameters[i]);

			render_resources_t::get().enqueue_data_upload({.data = parameter_values, .resource = internals->parameter_buffer, .data_size = runtime->parameter_data_size});
		}

		for (u32 i = 0; i < runtime->texture_count; ++i)
		{
			runtime->texture_guids[i] = material.textures[i];
		}

		return true;
	}

	void material_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&	  mem		= ctx.resource_manager.get_memory();
		material_internals_t* internals = mem.get<material_internals_t>(entry.internals);
		render_resources_t::get().enqueue_destroy_resource(internals->parameter_buffer);
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
		.use_async_load		 = false,
		.use_render_pending	 = true,
		.load				 = material_loader_t::load,
		.unload				 = material_loader_t::unload,
	};
}
