// Copyright (c) 2025 Inan Evin
#pragma once

namespace sfg
{
	class freetype_runtime_t
	{
	public:
		static bool	 init();
		static void	 uninit();
		static void* get_library();

	private:
		static void* s_library;
	};
}
