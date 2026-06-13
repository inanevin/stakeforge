// Copyright (c) 2025 Inan Evin

#include "shader.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/inplace_vector.hpp>
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
		stream.open(entry.after_header_data.data, entry.after_header_data.size);

		shader_runtime_t* runtime = mem.get<shader_runtime_t>(entry.runtime);
		*runtime				  = {};

		stream >> runtime->type;
		stream >> runtime->compile_variant_count;

		for (u8 i = 0; i < runtime->compile_variant_count; i++)
		{
			shader_runtime_compile_variant_t& v = runtime->compile_variants[i];
			stream >> v.stage_count;

			for (u8 j = 0; j < v.stage_count; j++)
			{
				shader_runtime_stage_entry_t& s = v.stages[j];
				stream >> s.stage;
				u32 size = 0;
				stream >> size;
				s.data = {stream.get_data_current(), static_cast<size_t>(size)};
				stream.skip_by(s.data.size);
			}
		}

		stream >> runtime->pso_variant_count;

		for (u8 i = 0; i < runtime->pso_variant_count; i++)
		{
			shader_runtime_pso_variant_t& v = runtime->pso_variants[i];
			stream >> v.compile_variant_index;
			stream >> v.variant_flags;
			u32 sz = 0;
			stream >> sz;

			v.desc_stream = {stream.get_data_current(), static_cast<size_t>(sz)};
			stream.skip_by(v.desc_stream.size);
		}

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

		istream_t stream;

		for (u8 i = 0; i < data->pso_variant_count; ++i)
		{
			const shader_runtime_pso_variant_t& pv = data->pso_variants[i];
			internals->pso_flags[i]				   = pv.variant_flags;

			SFG_ASSERT(pv.compile_variant_index < data->compile_variant_count);
			const shader_runtime_compile_variant_t& cv = data->compile_variants[pv.compile_variant_index];

			SFG_ASSERT(cv.stage_count <= MAX_STAGE_PER_VARIANT);
			inplace_vector_t<shader_blob_t, MAX_STAGE_PER_VARIANT> blobs;
			for (u8 j = 0; j < cv.stage_count; ++j)
			{
				const shader_runtime_stage_entry_t& s = cv.stages[j];
				blobs.push_back({
					.stage = static_cast<shader_stage_e>(s.stage),
					.data  = s.data,
				});
			}

			shader_desc_t desc = {};
			stream.open(pv.desc_stream.data, pv.desc_stream.size);
			desc.deserialize(stream);
			desc.set_name(mem.get_text(entry.debug_name));

			render_resources_t::get().enqueue_create_shader(entry.hash, entry.type, static_cast<u32>(i), desc, {.data = blobs.data(), .size = blobs.size()}, render_globals_t::get_global_bind_layout());
		}

		// done with runtime data.
		shader_runtime_t* runtime = mem.get<shader_runtime_t>(entry.runtime);
		for (u8 i = 0; i < runtime->compile_variant_count; i++)
		{
			shader_runtime_compile_variant_t& v = runtime->compile_variants[i];
			for (u8 j = 0; j < v.stage_count; j++)
				v.stages[j].data = {};
		}

		return create_internals_result_e::queued;
	}

	resource_ready_result_e shader_loader_t::resource_ready(resource_entry_t& entry, resource_context_t& ctx, const render_resource_completion_t& completion)
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
			return resource_ready_result_e::pending;

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
			return resource_ready_result_e::failed;
		}

		return resource_ready_result_e::ready;
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
		.resource_ready		 = shader_loader_t::resource_ready,
		.destroy_internals	 = shader_loader_t::destroy_internals,
	};

}
