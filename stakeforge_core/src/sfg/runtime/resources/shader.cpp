// Copyright (c) 2025 Inan Evin

#include "shader.hpp"
#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/inplace_vector.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/common/shader_description.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/runtime/render/render_globals.hpp>
#include <sfg/runtime/render/render_resources.hpp>
#include <sfg/serialization/compression.hpp>

namespace sfg
{
	bool shader_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs)
	{
		ostream_t file_stream;
		if (!rfs.read_resource(entry.hash, sizeof(resource_header_t), 0, file_stream))
		{
			SFG_ERR("failed to read shader resource: {0}", entry.hash);
			return false;
		}

		istream_t stream;
		stream.open(file_stream.get_raw(), file_stream.get_size());
		istream_t payload = compressor_t::decompress(stream);
		if (payload.empty())
		{
			SFG_ERR("failed to decompress shader payload: {0}", entry.hash);
			return false;
		}

		chunk_allocator_t&	mem		  = ctx.resource_manager.get_memory();
		shader_runtime_t*	runtime	  = mem.get<shader_runtime_t>(entry.runtime);
		shader_internals_t* internals = mem.get<shader_internals_t>(entry.internals);
		*runtime					  = {};
		*internals					  = {};

		payload >> runtime->type;
		payload >> runtime->compile_variant_count;

		for (u8 i = 0; i < runtime->compile_variant_count; i++)
		{
			shader_runtime_compile_variant_t& v = runtime->compile_variants[i];
			payload >> v.stage_count;

			for (u8 j = 0; j < v.stage_count; j++)
			{
				shader_runtime_stage_entry_t& s = v.stages[j];
				payload >> s.stage;
				u32 size = 0;
				payload >> size;
				s.data = {payload.get_data_current(), static_cast<size_t>(size)};
				payload.skip_by(s.data.size);
			}
		}

		payload >> runtime->pso_variant_count;

		for (u8 i = 0; i < runtime->pso_variant_count; i++)
		{
			shader_runtime_pso_variant_t& v = runtime->pso_variants[i];
			payload >> v.compile_variant_index;
			payload >> v.variant_flags;
			u32 sz = 0;
			payload >> sz;

			v.desc_stream = {payload.get_data_current(), static_cast<size_t>(sz)};
			payload.skip_by(v.desc_stream.size);
		}

		internals->pso_count = runtime->pso_variant_count;

		istream_t desc_stream;

		ctx.resource_manager.bump_render_pending(entry, runtime->pso_variant_count);
		for (u8 i = 0; i < runtime->pso_variant_count; ++i)
		{
			const shader_runtime_pso_variant_t& pv = runtime->pso_variants[i];
			internals->pso_flags[i]				   = pv.variant_flags;

			SFG_ASSERT(pv.compile_variant_index < runtime->compile_variant_count);
			const size_t							idx = static_cast<size_t>(pv.compile_variant_index);
			const shader_runtime_compile_variant_t& cv	= runtime->compile_variants[idx];

			SFG_ASSERT(cv.stage_count <= SFG_SHADER_MAX_STAGE_PER_VARIANT);
			inplace_vector_t<shader_blob_t, SFG_SHADER_MAX_STAGE_PER_VARIANT> blobs;
			for (u8 j = 0; j < cv.stage_count; ++j)
			{
				const shader_runtime_stage_entry_t& s = cv.stages[j];
				blobs.push_back({
					.stage = static_cast<shader_stage_e>(s.stage),
					.data  = s.data,
				});
			}

			shader_desc_t desc = {};
			desc_stream.open(pv.desc_stream.data, pv.desc_stream.size);
			desc.deserialize(desc_stream);
			desc.set_name(mem.get_text(entry.debug_name));

			internals->psos[i] = render_resources_t::get().enqueue_create_shader(entry.hash, entry.type, static_cast<u32>(i), desc, {.data = blobs.data(), .size = blobs.size()}, render_globals_t::get_global_bind_layout());
		}

		for (u8 i = 0; i < runtime->compile_variant_count; i++)
		{
			shader_runtime_compile_variant_t& v = runtime->compile_variants[i];
			for (u8 j = 0; j < v.stage_count; j++)
				v.stages[j].data = {};
		}

		return true;
	}

	void shader_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&	mem		  = ctx.resource_manager.get_memory();
		shader_internals_t* internals = mem.get<shader_internals_t>(entry.internals);

		for (u8 i = 0; i < internals->pso_count; ++i)
			render_resources_t::get().enqueue_destroy_shader(internals->psos[i]);
		*internals = {};
	}

	render_resource_handle_t shader_internals_t::find_pso(bitmask_t<u32> flags) const
	{
		const u32 want = flags.value();
		for (u8 i = 0; i < pso_count; ++i)
		{
			if (pso_flags[i].value() == want)
				return psos[i];
		}
		return {};
	}

	const resource_type_desc_t shader_resource_desc = {
		.type				 = resource_type_e::shader,
		.runtime_size		 = sizeof(shader_runtime_t),
		.runtime_alignment	 = alignof(shader_runtime_t),
		.internals_size		 = sizeof(shader_internals_t),
		.internals_alignment = alignof(shader_internals_t),
		.wire_magic			 = shader_loader_t::WIRE_MAGIC,
		.wire_version		 = shader_loader_t::WIRE_VERSION,
		.use_render_pending	 = true,
		.load				 = shader_loader_t::load,
		.unload				 = shader_loader_t::unload,
	};

}
