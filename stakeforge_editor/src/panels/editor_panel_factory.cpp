// Copyright (c) 2025 Inan Evin

#include "panels/editor_panel_factory.hpp"
#include "panels/editor_panel.hpp"

namespace sfg
{
	editor_panel_t* editor_panel_factory_t::create_panel(editor_panel_type_e type)
	{
		switch (type)
		{
		case editor_panel_type_e::entities:
			return new editor_panel_t();
		case editor_panel_type_e::assets:
			return new editor_panel_t();
		case editor_panel_type_e::log:
			return new editor_panel_t();
		case editor_panel_type_e::world:
			return new editor_panel_t();
		case editor_panel_type_e::inspector:
			return new editor_panel_t();
		case editor_panel_type_e::animation:
			return new editor_panel_t();
		case editor_panel_type_e::profiling:
			return new editor_panel_t();
		default:
			return nullptr;
		}
	}

	void editor_panel_factory_t::delete_panel(editor_panel_t* panel)
	{
		delete panel;
	}
}
