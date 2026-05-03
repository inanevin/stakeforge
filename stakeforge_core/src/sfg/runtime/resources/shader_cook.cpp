// Copyright (c) 2025 Inan Evin

#include "shader_cook.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/io/log.hpp>

namespace sfg
{
	bool shader_cook_serialize(const shader_compile_t& src, ostream_t& stream)
	{
		const u8 compile_variant_count = static_cast<u8>(src.compile_variants.size());
		const u8 pso_variant_count	   = static_cast<u8>(src.pso_variants.size());

		if (compile_variant_count > shader_max_compile_variants)
		{
			SFG_ERR("shader_cook: too many compile variants ({0}, max {1})", compile_variant_count, shader_max_compile_variants);
			return false;
		}
		if (pso_variant_count > shader_max_pso_variants)
		{
			SFG_ERR("shader_cook: too many pso variants ({0}, max {1})", pso_variant_count, shader_max_pso_variants);
			return false;
		}

		struct entry_t
		{
			u32 offset;
			u32 size;
			u8	stage;
		};
		vector_t<vector_t<entry_t>> per_compile_entries;
		per_compile_entries.resize(compile_variant_count);

		u32 blobs_size = 0;
		for (u8 i = 0; i < compile_variant_count; ++i)
		{
			const shader_compile_variant_scratch_t& cv = src.compile_variants[i];
			if (cv.blobs.size() > shader_max_stages_per_variant)
			{
				SFG_ERR("shader_cook: compile variant {0} has too many stages ({1})", i, cv.blobs.size());
				return false;
			}
			per_compile_entries[i].reserve(cv.blobs.size());
			for (const shader_compile_blob_t& b : cv.blobs)
			{
				per_compile_entries[i].push_back({.offset = blobs_size, .size = static_cast<u32>(b.bytes.size()), .stage = b.stage});
				blobs_size += static_cast<u32>(b.bytes.size());
			}
		}

		stream << shader_wire_magic;
		stream << shader_wire_version;
		stream << src.type;
		stream << compile_variant_count;
		stream << pso_variant_count;
		stream << blobs_size;

		for (u8 i = 0; i < compile_variant_count; ++i)
		{
			const shader_compile_variant_scratch_t& cv			= src.compile_variants[i];
			const u8								stage_count = static_cast<u8>(cv.blobs.size());
			stream << stage_count;
			for (u8 j = 0; j < stage_count; ++j)
			{
				const entry_t& e = per_compile_entries[i][j];
				stream << e.stage << e.offset << e.size;
			}
		}

		for (u8 i = 0; i < pso_variant_count; ++i)
		{
			const shader_pso_variant_scratch_t& pv = src.pso_variants[i];
			stream << pv.compile_variant_index << pv.variant_flags;
		}

		for (u8 i = 0; i < compile_variant_count; ++i)
		{
			for (const shader_compile_blob_t& b : src.compile_variants[i].blobs)
			{
				if (!b.bytes.empty())
					stream.write_raw(b.bytes.data(), b.bytes.size());
			}
		}

		return true;
	}
}
