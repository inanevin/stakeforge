// Copyright (c) 2025 Inan Evin
#pragma once

namespace sfg
{
	class ostream_t;
	struct sampler_desc_t;

	class texture_sampler_cooker
	{
	public:
		static bool cook_from_desc(const sampler_desc_t& desc, ostream_t& stream);
	};
}
