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

#include "assets/editor_asset_dependencies.hpp"
#include "assets/editor_asset.hpp"
#include "assets/editor_asset_io.hpp"

#include <sfg/runtime/resources/material_def.hpp>
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void editor_asset_dependencies_t::fetch_dependencies(const editor_asset_t& asset, vector_t<sid_t>& out_dependencies)
	{
		const auto push_dependency = [&](sid_t dependency) {
			if (dependency != NULL_SID)
				out_dependencies.push_back(dependency);
		};

		switch (asset.asset_type)
		{
		case editor_asset_type_e::material: {
			const nlohmann::json embedded_source = editor_asset_io_t::get_embedded_source_json(asset);
			if (!embedded_source.is_object())
				break;

			material_def_t material = {};
			embedded_source.get_to(material);
			push_dependency(material.shader);
			out_dependencies.reserve(out_dependencies.size() + material.textures.size() + material.samplers.size());
			for (const material_texture_value_t& texture : material.textures)
				push_dependency(texture.texture);
			for (const material_sampler_value_t& sampler : material.samplers)
				push_dependency(sampler.sampler);
			break;
		}
		default:
			break;
		}
	}
}
