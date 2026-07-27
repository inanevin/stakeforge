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

#include "curve.hpp"
#include "resource_file_system.hpp"
#include "resource_manager.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>
#include <sfg/math/math.hpp>
#include <sfg/reflection/reflection_registry.hpp>

namespace sfg
{
	vec4f_t curve_runtime_t::sample(f32 time) const
	{
		SFG_ASSERT(samples != nullptr);
		SFG_ASSERT(sample_count >= 2);

		const f32 scaled_time = math::clamp(time, 0.0f, 1.0f) * static_cast<f32>(sample_count - 1);
		const u32 left_index  = static_cast<u32>(math::floor(scaled_time));
		const u32 right_index = math::min(left_index + 1, sample_count - 1);

		if (interpolation == curve_interpolation_e::step)
			return samples[left_index];

		const f32 factor = scaled_time - static_cast<f32>(left_index);
		return samples[left_index] + (samples[right_index] - samples[left_index]) * factor;
	}

	bool curve_loader_t::load(resource_entry_t& entry, resource_context_t& ctx, resource_file_system_t& rfs, size_t payload_offset)
	{
		ostream_t file_stream = {};

		if (!rfs.read_resource(entry.hash, payload_offset, 0, file_stream))
		{
			SFG_ERR("failed to read curve resource: {0}", entry.hash);
			return false;
		}

		istream_t stream = {};
		stream.open(file_stream.get_raw(), file_stream.get_size());

		curve_def_t def = {};

		if (!reflection_registry_t::get().type_from_stream(type_id_t<curve_def_t>::value, &def, nullptr, stream))
		{
			SFG_ERR("failed to deserialize curve definition: {0}", entry.hash);
			return false;
		}

		std::sort(def.keys.begin(), def.keys.end(), [](const curve_key_t& left, const curve_key_t& right) { return left.time < right.time; });
		SFG_ASSERT(def.granularity >= 2);

		chunk_allocator_t& memory  = ctx.resource_manager.get_memory();
		curve_runtime_t*   runtime = memory.get<curve_runtime_t>(entry.runtime);
		*runtime				   = {};

		vec4f_t* samples	   = nullptr;
		runtime->samples_chunk = memory.allocate<vec4f_t>(def.granularity, samples);
		runtime->samples	   = samples;
		runtime->sample_count  = def.granularity;
		runtime->type		   = def.type;
		runtime->interpolation = def.interpolation;

		for (u32 sample_index = 0; sample_index < def.granularity; ++sample_index)
		{
			const f32 time		  = static_cast<f32>(sample_index) / static_cast<f32>(def.granularity - 1);
			samples[sample_index] = def.evaluate(time);
		}

		return true;
	}

	void curve_loader_t::unload(resource_entry_t& entry, resource_context_t& ctx)
	{
		chunk_allocator_t& memory  = ctx.resource_manager.get_memory();
		curve_runtime_t*   runtime = memory.get<curve_runtime_t>(entry.runtime);
		memory.free(runtime->samples_chunk);
		*runtime = {};
	}

	const resource_type_desc_t curve_resource_desc = {
		.type				 = resource_type_e::curve,
		.runtime_size		 = sizeof(curve_runtime_t),
		.runtime_alignment	 = alignof(curve_runtime_t),
		.internals_size		 = sizeof(curve_internals_t),
		.internals_alignment = alignof(curve_internals_t),
		.wire_magic			 = curve_loader_t::WIRE_MAGIC,
		.wire_version		 = curve_loader_t::WIRE_VERSION,
		.load				 = curve_loader_t::load,
		.unload				 = curve_loader_t::unload,
	};
}
