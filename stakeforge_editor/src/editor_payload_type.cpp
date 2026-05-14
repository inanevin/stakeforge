#include "editor_payload_type.hpp"

namespace sfg
{
	const char* get_editor_payload_type_name(editor_payload_type_e type)
	{
		switch (type)
		{
		case editor_payload_type_e::panel:
			return "panel";
		case editor_payload_type_e::resource:
			return "resource";
		default:
			return "";
		}
	}
}
