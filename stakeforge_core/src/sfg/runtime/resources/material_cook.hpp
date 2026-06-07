// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>
#include <sfg/data/vector.hpp>

namespace sfg
{
	struct material_def_t;
	class ostream_t;

	class material_cooker
	{
	public:
		static bool cook_from_def(const material_def_t& def, ostream_t& stream);
		static bool collect_source_ticks(const material_def_t& def, vector_t<u64>& out);
	};
}
