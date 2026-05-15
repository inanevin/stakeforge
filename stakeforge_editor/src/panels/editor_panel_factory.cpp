// Copyright (c) 2025 Inan Evin

#include "panels/editor_panel_factory.hpp"
#include "panels/editor_panel_animation.hpp"
#include "panels/editor_panel_assets.hpp"
#include "panels/editor_panel_entities.hpp"
#include "panels/editor_panel_inspector.hpp"
#include "panels/editor_panel_log.hpp"
#include "panels/editor_panel_profiling.hpp"
#include "panels/editor_panel_world.hpp"

namespace sfg
{
	editor_panel_t* editor_panel_factory_t::create_panel(editor_panel_type_e type)
	{
		switch (type)
		{
		case editor_panel_type_e::entities:
			return new editor_panel_entities_t();
		case editor_panel_type_e::assets:
			return new editor_panel_assets_t();
		case editor_panel_type_e::log:
			return new editor_panel_log_t();
		case editor_panel_type_e::world:
			return new editor_panel_world_t();
		case editor_panel_type_e::inspector:
			return new editor_panel_inspector_t();
		case editor_panel_type_e::animation:
			return new editor_panel_animation_t();
		case editor_panel_type_e::profiling:
			return new editor_panel_profiling_t();
		default:
			return nullptr;
		}
	}

	void editor_panel_factory_t::delete_panel(editor_panel_t* panel)
	{
		delete panel;
	}
}
