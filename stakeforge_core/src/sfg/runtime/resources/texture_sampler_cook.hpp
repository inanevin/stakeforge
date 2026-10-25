// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/sfg_exports.hpp>

namespace sfg
{
	class ostream_t;
	struct sampler_desc_t;
	struct resource_header_t;

	class SFG_API texture_sampler_cooker
	{
	public:
		static bool cook_from_desc(const sampler_desc_t& desc, resource_header_t& out_header, ostream_t& stream);
	};
}
