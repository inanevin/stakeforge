#pragma once

#include <sfg/common/size_definitions.hpp>

namespace sfg
{
	enum class editor_payload_type_e : u8
	{
		panel,
		resource,
	};

	const char* get_editor_payload_type_name(editor_payload_type_e type);
}
