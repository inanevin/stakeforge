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
#pragma once

#include "package.hpp"
#include "data/vector.hpp"
#include "data/string.hpp"

namespace SFG
{

	class package_manager
	{
	public:
		static package_manager& get()
		{
			static package_manager inst;
			return inst;
		}

		static constexpr const char* ENGINE_PKG_PATH = "engine.stkpkg";
		static constexpr const char* RES_PKG_PATH	 = "res.stkpkg";
		static constexpr const char* WORLD_PKG_PATH	 = "world.stkpkg";

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		bool init();
		void uninit();

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

#ifdef SFG_TOOLMODE
		void package_project(const vector<string>& levels, const char* output_directory);
#endif
		package& open_package_engine_data();
		package& open_package_world();
		package& open_package_res();

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

	private:
		package _pk_engine_data = {};
		package _pk_world_data	= {};
		package _pk_res_data	= {};
	};
}
