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
#include "editor_layout.hpp"
#include "editor_surface.hpp"
#include "ui/panels/editor_primary_base.hpp"
#include <sfg/vendor/nhlohmann/json.hpp>

namespace sfg
{
	void editor_layout_t::load_surface_default_layout(editor_surface_t& surface)
	{
		nlohmann::json layout = {
			{"version", 1},
			{"root",
			 nlohmann::json{
				 {"type", "split"},
				 {"direction", "horizontal"},
				 {"split_value", 0.22f},
				 {"negative", nlohmann::json{{"type", "leaf"}, {"panels", nlohmann::json::array({nlohmann::json{{"type", "Entities"}, {"data", nlohmann::json::object()}}})}}},
				 {"positive",
				  nlohmann::json{
					  {"type", "split"},
					  {"direction", "horizontal"},
					  {"split_value", 0.72f},
					  {"negative",
					   nlohmann::json{
						   {"type", "split"},
						   {"direction", "vertical"},
						   {"split_value", 0.68f},
						   {"negative", nlohmann::json{{"type", "leaf"}, {"panels", nlohmann::json::array({nlohmann::json{{"type", "World"}, {"data", nlohmann::json::object()}}})}}},
						   {"positive",
							nlohmann::json{
								{"type", "leaf"},
								{"panels",
								 nlohmann::json::array(
									 {nlohmann::json{{"type", "Assets"}, {"data", nlohmann::json::object()}}, nlohmann::json{{"type", "Log"}, {"data", nlohmann::json::object()}}, nlohmann::json{{"type", "Profiling"}, {"data", nlohmann::json::object()}}})}}},
					   }},
					  {"positive", nlohmann::json{{"type", "leaf"}, {"panels", nlohmann::json::array({nlohmann::json{{"type", "Inspector"}, {"data", nlohmann::json::object()}}})}}},
				  }},
			 }},
		};

		if (surface.type == editor_surface_type_e::primary)
			surface.primary->get_dock_widget().from_json(layout);
	}
}
