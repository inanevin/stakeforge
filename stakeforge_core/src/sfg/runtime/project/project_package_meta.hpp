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

#include "project_settings.hpp"

#include <sfg/data/hash_map.hpp>
#include <sfg/data/vector.hpp>
#include <sfg/platform/common_window.hpp>
#include <sfg/runtime/resources/common_resources.hpp>
#include <sfg/runtime/resources/resource_file_system.hpp>

namespace sfg
{
	class istream_t;
	class ostream_t;

	struct project_package_meta_t
	{
		static inline constexpr const char* FILE_NAME					 = "project_meta.sfg_bin";
		static inline constexpr const char* RESOURCE_FILE_NAME			 = "resources.sfg_bin";
		static inline constexpr u32			WIRE_MAGIC					 = make_resource_wire_magic('P', 'M', 'E', 'T');
		static inline constexpr u32			WIRE_VERSION				 = 6;
		static inline constexpr u32			RESOURCE_STREAM_WIRE_MAGIC	 = make_resource_wire_magic('R', 'S', 'T', 'R');
		static inline constexpr u32			RESOURCE_STREAM_WIRE_VERSION = 4;

		hash_map_t<sid_t, resource_map_info_t> resource_map		 = {};
		project_settings_t					   project_settings	 = {};
		vector_t<sid_t>						   worlds			 = {};
		sid_t								   main_world		 = NULL_SID;
		vec2u16_t							   window_resolution = {1920, 1080};
		window_style_e						   window_style		 = window_style_e::app_window;
		bool								   is_fullscreen	 = false;

		bool serialize(ostream_t& stream) const;
		bool deserialize(istream_t& stream);
	};
}
