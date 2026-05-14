// Copyright (c) 2025 Inan Evin

#include "panels/editor_panel_factory.hpp"
#include "panels/editor_panel.hpp"

namespace sfg
{
	editor_panel_t* editor_panel_factory_t::create_panel(editor_panel_type_e type)
	{
		editor_panel_t* panel = nullptr;
		switch (type)
		{
		case editor_panel_type_e::entities:
			panel = new editor_panel_t();
			break;
		case editor_panel_type_e::assets:
			panel = new editor_panel_t();
			break;
		case editor_panel_type_e::log:
			panel = new editor_panel_t();
			break;
		case editor_panel_type_e::world:
			panel = new editor_panel_t();
			break;
		case editor_panel_type_e::inspector:
			panel = new editor_panel_t();
			break;
		case editor_panel_type_e::animation:
			panel = new editor_panel_t();
			break;
		case editor_panel_type_e::profiling:
			panel = new editor_panel_t();
			break;
		default:
			return nullptr;
		}

		panel->set_title(editor_panel_type_to_string(type));
		return panel;
	}

	void editor_panel_factory_t::delete_panel(editor_panel_t* panel)
	{
		delete panel;
	}
}
