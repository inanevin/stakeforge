// Copyright (c) 2025 Inan Evin

#include "material.hpp"

#include "material_def.hpp"
#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include "texture.hpp"
#include "shader.hpp"
#include <sfg/common/packing.hpp>
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/memory/memory.hpp>
#include <sfg/runtime/render/render_resources.hpp>

namespace sfg
{
	namespace
	{
		u32 get_material_parameter_size(const material_runtime_parameter_t& parameter)
		{
			switch (parameter.type)
			{
			case shader_param_type_e::u32:
				return sizeof(u32);
			case shader_param_type_e::vec2:
				return sizeof(f32) * 2;
			case shader_param_type_e::vec4:
				if (parameter.hint == shader_param_hint_e::pack_uint2)
					return sizeof(u32) * 2;
				return sizeof(f32) * 4;
			default:
				return sizeof(f32);
			}
		}

		u32 get_material_parameter_offset(u32 offset, const material_runtime_parameter_t& parameter)
		{
			const u32 size			  = get_material_parameter_size(parameter);
			const u32 register_offset = offset % (sizeof(f32) * 4);
			if (register_offset + size > sizeof(f32) * 4)
				return offset + sizeof(f32) * 4 - register_offset;
			return offset;
		}

		void write_material_parameter(u8*& dst, const material_runtime_parameter_t& parameter)
		{
			switch (parameter.type)
			{
			case shader_param_type_e::u32:
				SFG_MEMCPY(dst, parameter.values_u32, sizeof(u32));
				dst += sizeof(u32);
				break;
			case shader_param_type_e::vec2:
				SFG_MEMCPY(dst, parameter.values, sizeof(f32) * 2);
				dst += sizeof(f32) * 2;
				break;
			case shader_param_type_e::vec4:
				if (parameter.hint == shader_param_hint_e::pack_uint2)
				{
					const u32 values[2] = {packing_t::pack_half2x16(parameter.values[0], parameter.values[1]), packing_t::pack_half2x16(parameter.values[2], parameter.values[3])};
					SFG_MEMCPY(dst, values, sizeof(values));
					dst += sizeof(values);
					break;
				}
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

	void pack_material_parameters(const material_runtime_t& material, u8* data)
	{
		for (u32 i = 0; i < material.parameter_count; ++i)
		{
			u8* dst = data + material.parameters[i].data_offset;
			write_material_parameter(dst, material.parameters[i]);
		}
	}

	bool material_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset)
	{
		ostream_t file_stream = {};

		if (!rfs.read_resource(entry.hash, payload_offset, 0, file_stream))
		{
			SFG_ERR("failed to read material resource: {0}", entry.hash);
			return false;
		}

		istream_t stream = {};

		stream.open(file_stream.get_raw(), file_stream.get_size());

		chunk_allocator_t&	  mem		= ctx.resource_manager.get_memory();
		material_runtime_t*	  runtime	= mem.get<material_runtime_t>(entry.runtime);
		material_internals_t* internals = mem.get<material_internals_t>(entry.internals);
		*runtime						= {};
		*internals						= {};

		material_def_t material = {};

		stream >> material;

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

		if (material.samplers.size() > SFG_MATERIAL_MAX_TEXTURES)
		{
			SFG_ERR("material has too many samplers: {0} / {1}", material.samplers.size(), SFG_MATERIAL_MAX_TEXTURES);
			return false;
		}

		runtime->shader_guid	   = material.shader;
		runtime->write_depth	   = material.write_depth ? 1 : 0;
		runtime->write_shadows	   = material.write_shadows ? 1 : 0;
		runtime->write_reflections = material.write_reflections ? 1 : 0;
		runtime->is_transparent	   = material.is_transparent ? 1 : 0;
		runtime->double_sided	   = material.double_sided ? 1 : 0;
		runtime->use_alpha_cutoff  = material.use_alpha_cutoff ? 1 : 0;
		runtime->parameter_count   = static_cast<u32>(material.parameters.size());
		runtime->texture_count	   = static_cast<u32>(material.textures.size());
		runtime->sampler_count	   = static_cast<u32>(material.samplers.size());

		if (runtime->parameter_count != 0)
		{
			for (u32 i = 0; i < runtime->parameter_count; ++i)
			{
				runtime->parameter_name_hashes[i] = material.parameters[i].name_hash;
				runtime->parameters[i].type		  = material.parameters[i].type;
				runtime->parameters[i].hint		  = material.parameters[i].hint;
				SFG_MEMCPY(runtime->parameters[i].values, material.parameters[i].value, sizeof(runtime->parameters[i].values));
				runtime->parameter_data_size	   = get_material_parameter_offset(runtime->parameter_data_size, runtime->parameters[i]);
				runtime->parameters[i].data_offset = static_cast<u16>(runtime->parameter_data_size);
				runtime->parameter_data_size += get_material_parameter_size(runtime->parameters[i]);
			}

			SFG_ASSERT(runtime->parameter_data_size <= SFG_MATERIAL_MAX_PARAMETER_DATA_SIZE);
			u8 parameter_values[SFG_MATERIAL_MAX_PARAMETER_DATA_SIZE] = {};
			pack_material_parameters(*runtime, parameter_values);

			resource_desc_t desc = {};
			desc.size			 = runtime->parameter_data_size;
			desc.flags			 = resource_flags::rf_constant_buffer | resource_flags::rf_cpu_visible;
			desc.set_name(mem.get_text(entry.debug_name));

			for (u8 frame_index = 0; frame_index < BACK_BUFFER_COUNT; ++frame_index)
			{
				internals->parameter_buffers[frame_index] = render_resources_t::get().enqueue_create_resource(desc);

				render_resources_t::get().enqueue_data_upload({
					.data	   = parameter_values,
					.resource  = internals->parameter_buffers[frame_index],
					.data_size = runtime->parameter_data_size,
				});
			}
		}

		for (u32 i = 0; i < runtime->texture_count; ++i)
		{
			runtime->texture_name_hashes[i] = material.textures[i].name_hash;
			runtime->texture_guids[i]		= material.textures[i].texture;
			runtime->texture_types[i]		= material.textures[i].type;
		}

		for (u32 i = 0; i < runtime->sampler_count; ++i)
		{
			runtime->sampler_name_hashes[i] = material.samplers[i].name_hash;
			runtime->sampler_guids[i]		= material.samplers[i].sampler;
		}

		return true;
	}

	void material_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&	  mem		= ctx.resource_manager.get_memory();
		material_internals_t* internals = mem.get<material_internals_t>(entry.internals);

		for (u8 frame_index = 0; frame_index < BACK_BUFFER_COUNT; ++frame_index)
			render_resources_t::get().enqueue_destroy_resource(internals->parameter_buffers[frame_index]);

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
		.load				 = material_loader_t::load,
		.unload				 = material_loader_t::unload,
	};
}
