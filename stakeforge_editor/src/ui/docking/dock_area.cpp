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
#include "ui/docking/dock_area.hpp"
#include <sfg/common/hashing.hpp>

namespace sfg
{
	const char* dock_node_type_to_string(dock_node_type_e type)
	{
		switch (type)
		{
		case dock_node_type_e::leaf:
			return "leaf";
		case dock_node_type_e::split:
			return "split";
		}
		return "leaf";
	}

	dock_node_type_e dock_node_type_from_string(const char* value)
	{
		const sid_t id = TO_SID(value);
		if (id == TO_SID("split"))
			return dock_node_type_e::split;
		return dock_node_type_e::leaf;
	}

	const char* dock_split_direction_to_string(dock_split_direction_e direction)
	{
		switch (direction)
		{
		case dock_split_direction_e::horizontal:
			return "horizontal";
		case dock_split_direction_e::vertical:
			return "vertical";
		}
		return "horizontal";
	}

	dock_split_direction_e dock_split_direction_from_string(const char* value)
	{
		const sid_t id = TO_SID(value);
		if (id == TO_SID("vertical"))
			return dock_split_direction_e::vertical;
		return dock_split_direction_e::horizontal;
	}
}
