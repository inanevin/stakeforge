// Copyright (c) 2025 Inan Evin

#include "shader.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/gfx/common/shader_description.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/runtime/render/render_globals.hpp>
#include <sfg/runtime/render/render_resources.hpp>

namespace sfg
{
	bool shader_loader_t::load(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t& mem	 = ctx.resource_manager.get_memory();
		u8*				   bytes = mem.get(entry.runtime.head);

		istream_t stream;
		stream.open(bytes, entry.runtime.size);

		shader_runtime_t local = {};
		stream >> local.type >> local.compile_variant_count >> local.pso_variant_count;

		SFG_ASSERT(local.compile_variant_count <= MAX_COMPILE_VARIANTS);
		SFG_ASSERT(local.pso_variant_count <= MAX_PSO_VARIANTS);

		for (u8 i = 0; i < local.compile_variant_count; ++i)
		{
			shader_runtime_compile_variant_t& cv = local.compile_variants[i];
			stream >> cv.stage_count;
			SFG_ASSERT(cv.stage_count <= MAX_STAGE_PER_VARIANT);
			for (u8 j = 0; j < cv.stage_count; ++j)
			{
				shader_runtime_stage_entry_t& s = cv.stages[j];
				stream >> s.stage >> s.offset >> s.size;
			}
		}

		for (u8 i = 0; i < local.pso_variant_count; ++i)
		{
			shader_runtime_pso_variant_t& pv = local.pso_variants[i];
			stream >> pv.compile_variant_index;
			stream >> pv.variant_flags;
			stream >> pv.desc_offset;
			stream >> pv.desc_size;
		}

		*mem.get<shader_runtime_t>(entry.runtime) = local;
		return true;
	}

	create_internals_result_e shader_loader_t::create_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&		mem		  = ctx.resource_manager.get_memory();
		const shader_runtime_t* data	  = mem.get<shader_runtime_t>(entry.runtime);
		shader_internals_t*		internals = mem.get<shader_internals_t>(entry.internals);

		*internals				 = shader_internals_t{};
		internals->pso_count	 = data->pso_variant_count;
		internals->pending_count = data->pso_variant_count;

		if (data->pso_variant_count == 0)
			return create_internals_result_e::ready;

		u8* payload = entry.payload.data;

		for (u8 i = 0; i < data->pso_variant_count; ++i)
		{
			const shader_runtime_pso_variant_t& pv = data->pso_variants[i];
			internals->pso_flags[i]				   = pv.variant_flags;

			SFG_ASSERT(pv.compile_variant_index < data->compile_variant_count);
			const shader_runtime_compile_variant_t& cv = data->compile_variants[pv.compile_variant_index];

			vector_t<shader_blob_t> blobs;
			blobs.resize(cv.stage_count);
			for (u8 j = 0; j < cv.stage_count; ++j)
			{
				const shader_runtime_stage_entry_t& s = cv.stages[j];
				blobs[j].stage						  = static_cast<shader_stage_e>(s.stage);
				blobs[j].data						  = {.data = payload + s.offset, .size = s.size};
			}

			shader_desc_t desc = {};
			if (pv.desc_size != 0)
			{
				istream_t desc_stream;
				desc_stream.open(payload + pv.desc_offset, pv.desc_size);
				desc.deserialize(desc_stream);
			}

			render_resources_t::get().enqueue_create_shader(entry.hash, entry.type, static_cast<u32>(i), desc, std::move(blobs), render_globals_t::get_global_bind_layout());
		}

		return create_internals_result_e::queued;
	}

	complete_internals_result_e shader_loader_t::complete_internals(resource_entry_t& entry, resource_context_t& ctx, const render_resource_completion_t& completion)
	{
		chunk_allocator_t&	mem		  = ctx.resource_manager.get_memory();
		shader_internals_t* internals = mem.get<shader_internals_t>(entry.internals);

		SFG_ASSERT(completion.user_data < internals->pso_count);
		SFG_ASSERT(internals->pending_count > 0);

		if (completion.state == resource_state_e::failed)
			internals->had_failure = true;
		else
			internals->psos[completion.user_data] = completion.shader;

		internals->pending_count--;
		if (internals->pending_count != 0)
			return complete_internals_result_e::pending;

		if (internals->had_failure)
		{
			for (u8 i = 0; i < internals->pso_count; ++i)
			{
				if (!internals->psos[i].is_null())
				{
					render_resources_t::get().enqueue_destroy_shader(internals->psos[i]);
					internals->psos[i] = {};
				}
			}
			return complete_internals_result_e::failed;
		}

		return complete_internals_result_e::ready;
	}

	void shader_loader_t::destroy_internals(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t&	mem		  = ctx.resource_manager.get_memory();
		shader_internals_t* internals = mem.get<shader_internals_t>(entry.internals);

		for (u8 i = 0; i < internals->pso_count; ++i)
		{
			if (!internals->psos[i].is_null())
			{
				render_resources_t::get().enqueue_destroy_shader(internals->psos[i]);
				internals->psos[i] = {};
			}
		}
		internals->pso_count	 = 0;
		internals->pending_count = 0;
	}

	gfx_shader_handle shader_internals_t::find_pso(bitmask_t<u32> flags) const
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
		.load				 = shader_loader_t::load,
		.create_internals	 = shader_loader_t::create_internals,
		.complete_internals	 = shader_loader_t::complete_internals,
		.destroy_internals	 = shader_loader_t::destroy_internals,
	};

}
