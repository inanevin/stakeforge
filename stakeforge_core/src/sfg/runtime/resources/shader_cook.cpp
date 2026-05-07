// Copyright (c) 2025 Inan Evin

#include "shader_cook.hpp"
#include "shader_cook_variants.hpp"
#include <sfg/data/ostream.hpp>
#include <sfg/gfx/common/shader_description.hpp>
#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	bool shader_cooker::cook_from_file(const shader_cook_config_t& cfg, const char* full_path, ostream_t& stream)
	{
		if (cfg.type == shader_type_e::invalid)
		{
			SFG_ERR("invalid shader type for {0}", full_path);
			return false;
		}

		const string_t source = file_system_t::read_file_as_string(full_path);
		if (source.empty())
		{
			SFG_ERR("failed to read source {0}", full_path);
			return false;
		}

		const string_t			 directory	   = file_system_t::get_directory_of_file(full_path);
		const vector_t<string_t> include_paths = directory.empty() ? vector_t<string_t>{} : vector_t<string_t>{directory};

		vector_t<cook_compile_variant_t> compiles;
		vector_t<cook_pso_variant_t>	 psos;

		switch (cfg.type)
		{
		case shader_type_e::editor_ui_default:
			if (!shader_cook_variants_t::cook_editor_ui(source, include_paths, compiles, psos))
				return false;
			break;
		default:
			SFG_ERR("unsupported shader type {0}", static_cast<u8>(cfg.type));
			return false;
		}

		const u8 compile_variant_count = static_cast<u8>(compiles.size());
		const u8 pso_variant_count	   = static_cast<u8>(psos.size());

		if (compile_variant_count > shader_loader_t::MAX_COMPILE_VARIANTS)
		{
			SFG_ERR("too many compile variants ({0}, max {1})", compile_variant_count, shader_loader_t::MAX_COMPILE_VARIANTS);
			return false;
		}
		if (pso_variant_count > shader_loader_t::MAX_PSO_VARIANTS)
		{
			SFG_ERR("too many pso variants ({0}, max {1})", pso_variant_count, shader_loader_t::MAX_PSO_VARIANTS);
			return false;
		}

		struct stage_entry_t
		{
			u32 offset;
			u32 size;
			u8	stage;
		};
		vector_t<vector_t<stage_entry_t>> per_compile_entries;
		per_compile_entries.resize(compile_variant_count);

		u32 blobs_size = 0;
		for (u8 i = 0; i < compile_variant_count; ++i)
		{
			const cook_compile_variant_t& cv = compiles[i];
			if (cv.stages.size() > shader_loader_t::MAX_STAGE_PER_VARIANT)
			{
				SFG_ERR("compile variant {0} has too many stages ({1})", i, cv.stages.size());
				return false;
			}
			per_compile_entries[i].reserve(cv.stages.size());
			for (const cook_stage_blob_t& b : cv.stages)
			{
				per_compile_entries[i].push_back({.offset = blobs_size, .size = static_cast<u32>(b.bytes.size()), .stage = b.stage});
				blobs_size += static_cast<u32>(b.bytes.size());
			}
		}

		vector_t<ostream_t> pso_desc_streams;
		pso_desc_streams.resize(pso_variant_count);
		vector_t<u32> pso_desc_offsets;
		pso_desc_offsets.resize(pso_variant_count);
		vector_t<u32> pso_desc_sizes;
		pso_desc_sizes.resize(pso_variant_count);

		u32 desc_cursor = blobs_size;
		for (u8 i = 0; i < pso_variant_count; ++i)
		{
			psos[i].desc.serialize(pso_desc_streams[i]);
			const u32 sz		= static_cast<u32>(pso_desc_streams[i].get_size());
			pso_desc_offsets[i] = desc_cursor;
			pso_desc_sizes[i]	= sz;
			desc_cursor += sz;
		}

		const u32 payload_size = desc_cursor;

		const size_t	  header_pos = stream.get_size();
		resource_header_t header	 = {
				.magic			= shader_loader_t::WIRE_MAGIC,
				.version		= shader_loader_t::WIRE_VERSION,
				.payload_size	= payload_size,
				.modified_ticks = file_system_t::get_last_modified_ticks(full_path),
		};
		header.serialize(stream);

		stream << cfg.type;
		stream << compile_variant_count;
		stream << pso_variant_count;

		for (u8 i = 0; i < compile_variant_count; ++i)
		{
			const cook_compile_variant_t& cv		  = compiles[i];
			const u8					  stage_count = static_cast<u8>(cv.stages.size());
			stream << stage_count;
			for (u8 j = 0; j < stage_count; ++j)
			{
				const stage_entry_t& e = per_compile_entries[i][j];
				stream << e.stage << e.offset << e.size;
			}
		}

		for (u8 i = 0; i < pso_variant_count; ++i)
		{
			const cook_pso_variant_t& pv = psos[i];
			stream << pv.compile_variant_index;
			stream << pv.variant_flags;
			stream << pso_desc_offsets[i];
			stream << pso_desc_sizes[i];
		}

		header.payload_offset = static_cast<u32>(stream.get_size() - header_pos);
		header.patch_payload_offset(stream, header_pos);

		for (u8 i = 0; i < compile_variant_count; ++i)
		{
			for (const cook_stage_blob_t& b : compiles[i].stages)
			{
				if (!b.bytes.empty())
					stream.write_raw(b.bytes.data(), b.bytes.size());
			}
		}

		for (u8 i = 0; i < pso_variant_count; ++i)
		{
			ostream_t& s = pso_desc_streams[i];
			if (s.get_size() != 0)
				stream.write_raw(s.get_raw(), s.get_size());
		}

		return true;
	}

	void from_json(const nlohmann::json& j, shader_cook_config_t& c)
	{
		c.type = j.value<shader_type_e>("type", shader_type_e::invalid);
	}
}
