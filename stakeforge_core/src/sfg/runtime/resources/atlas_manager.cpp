// Copyright (c) 2025 Inan Evin

#include "atlas_manager.hpp"
#include "font.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>

namespace sfg
{
	void atlas_manager_t::init(u32 default_atlas_width, u32 default_atlas_height)
	{
		SFG_ASSERT(default_atlas_width > 0 && default_atlas_height > 0);
		_atlas_width  = default_atlas_width;
		_atlas_height = default_atlas_height;
	}

	void atlas_manager_t::uninit()
	{
		for (unique_t<atlas_t>& a : _atlases)
			a->uninit();
		_atlases.clear();
		_atlas_width  = 0;
		_atlas_height = 0;
	}

	bool atlas_manager_t::add_font(font_runtime_t* font)
	{
		SFG_ASSERT(font != nullptr);
		const bool need_lcd = font->kind == font_kind_e::lcd;

		for (unique_t<atlas_t>& a : _atlases)
		{
			if (a->get_is_lcd() != need_lcd)
				continue;
			if (a->add_font(font))
				return true;
		}

		unique_t<atlas_t> a = make_unique<atlas_t>();
		a->init(_atlas_width, _atlas_height, need_lcd);
		a->set_id(static_cast<u32>(_atlases.size()));
		const bool ok = a->add_font(font);
		if (!ok)
		{
			SFG_ERR("atlas_manager: font does not fit in a fresh atlas of {0}x{1}", _atlas_width, _atlas_height);
			a->uninit();
			return false;
		}
		_atlases.push_back(std::move(a));
		return true;
	}

	void atlas_manager_t::remove_font(font_runtime_t* font)
	{
		SFG_ASSERT(font != nullptr);
		// TODO: route remove_font once create_internals/destroy_internals wires fonts to specific atlases.
		// The font would need to know which atlas it was placed in (e.g. an atlas id stored in
		// font_runtime_t at runtime) so the manager can dispatch to the right atlas here.
	}
}
