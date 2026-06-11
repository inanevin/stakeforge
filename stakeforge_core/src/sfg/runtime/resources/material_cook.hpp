// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>

namespace sfg
{
	struct material_def_t;
	class ostream_t;
	struct resource_header_t;

	class material_cooker
	{
	public:
		static bool cook_from_def(const material_def_t& def, resource_header_t& out_header, ostream_t& stream);
		static bool collect_source_tick(const material_def_t& def, u64& out);
	};
}
