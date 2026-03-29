#include "comp_sprite.hpp"
#include "reflection/reflection.hpp"
#include "world/world.hpp"
#include "gfx/event_stream/render_event_stream.hpp"
#include "gfx/event_stream/render_events_trait.hpp"
#include "resources/material.hpp"

namespace SFG
{
	void comp_sprite::reflect()
	{
		meta& m = reflection::get().register_meta(type_id<comp_sprite>::value, 0, "component");
		m.set_title("sprite");
		m.set_category("rendering");

		m.add_field<&comp_sprite::_material, comp_sprite>("material", reflected_field_type::rf_resource, "", type_id<material>::value);

		m.add_function<void, const reflected_field_changed_params&>("on_reflected_changed"_hs, [](const reflected_field_changed_params& params) {
			comp_sprite* c = static_cast<comp_sprite*>(params.object_ptr);
			c->set_material(params.w, c->_material);
		});

		m.add_function<void, void*, world&>("on_reflect_load"_hs, [](void* obj, world& w) {
			comp_sprite* c = static_cast<comp_sprite*>(obj);
			c->set_material(w, c->_material);
		});

		m.add_function<void, void*, vector<resource_handle_and_type>&>("gather_resources"_hs, [](void* obj, vector<resource_handle_and_type>& h) {
			comp_sprite* c = static_cast<comp_sprite*>(obj);
			h.push_back({.handle = c->_material, .type_id = type_id<material>::value});
		});
	}

	void comp_sprite::on_add(world& w)
	{
		w.get_entity_manager().add_render_proxy(_header.entity);
		const render_event_sprite ev = {
			.entity	  = _header.entity.index,
			.material = NULL_RESOURCE_ID,
		};
		w.get_render_stream().add_event({.index = _header.own_handle.index, .event_type = render_event_type::sprite}, ev);
	}

	void comp_sprite::on_remove(world& w)
	{
		w.get_entity_manager().remove_render_proxy(_header.entity);
		w.get_render_stream().add_event({.index = _header.own_handle.index, .event_type = render_event_type::remove_sprite});
	}

	void comp_sprite::set_material(world& w, resource_handle material)
	{
		const render_event_sprite ev = {
			.entity	  = _header.entity.index,
			.material = material.is_null() ? NULL_RESOURCE_ID : material.index,
		};
		_material = material;
		w.get_render_stream().add_event({.index = _header.own_handle.index, .event_type = render_event_type::sprite}, ev);
	}
}
