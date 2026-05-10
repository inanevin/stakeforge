// Copyright (c) 2025 Inan Evin

#include "freetype_runtime.hpp"
#include <sfg/io/assert.hpp>
#include <sfg/io/log.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_LCD_FILTER_H

namespace sfg
{
	void* freetype_runtime_t::s_library = nullptr;

	bool freetype_runtime_t::init()
	{
		SFG_ASSERT(s_library == nullptr);

		FT_Library library = nullptr;
		if (FT_Init_FreeType(&library) != 0)
		{
			SFG_ERR("FT_Init_FreeType failed");
			return false;
		}

		FT_Library_SetLcdFilter(library, FT_LCD_FILTER_DEFAULT);
		s_library = library;
		return true;
	}

	void freetype_runtime_t::uninit()
	{
		SFG_ASSERT(s_library != nullptr);
		FT_Done_FreeType(static_cast<FT_Library>(s_library));
		s_library = nullptr;
	}

	void* freetype_runtime_t::get_library()
	{
		SFG_ASSERT(s_library != nullptr);
		return s_library;
	}
}
