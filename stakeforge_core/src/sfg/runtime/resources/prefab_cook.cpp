// Copyright (c) 2025 Inan Evin

#include "prefab_cook.hpp"

#include <sfg/io/log.hpp>

namespace sfg
{
	bool prefab_cooker::cook_from_file(const char*, resource_header_t&, ostream_t&)
	{
		SFG_ERR("prefab cooking is not implemented");
		return false;
	}
}
