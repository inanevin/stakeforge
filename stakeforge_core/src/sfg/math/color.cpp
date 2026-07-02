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

#include "color.hpp"
#include <sfg/reflection/reflection_registry.hpp>
#include "math.hpp"
#include <sfg/data/istream.hpp>
#include <sfg/data/ostream.hpp>

namespace sfg
{
	color_t color_t::red	= color_t(1, 0, 0, 1);
	color_t color_t::green	= color_t(0, 1, 0);
	color_t color_t::blue	= color_t(0, 0, 1);
	color_t color_t::cyan	= color_t(0, 1, 1);
	color_t color_t::yellow = color_t(1, 1, 0);
	color_t color_t::black	= color_t(0, 0, 0);
	color_t color_t::white	= color_t(1, 1, 1);
	color_t color_t::purple = color_t(1, 0, 1);
	color_t color_t::maroon = color_t(0.5f, 0, 0);
	color_t color_t::beige	= color_t(0.96f, 0.96f, 0.862f);
	color_t color_t::brown	= color_t(0.647f, 0.164f, 0.164f);
	color_t color_t::gray	= color_t(0.5f, 0.5f, 0.5f);

	color_t color_t::from255(f32 r, f32 g, f32 b, f32 a)
	{
		return color_t(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
	}

	void color_t::round()
	{
		x = math::round(x);
		y = math::round(y);
		z = math::round(z);
		w = math::round(w);
	}

	void color_t::serialize(ostream_t& stream) const
	{
		stream << x << y << z << w;
	}

	void color_t::deserialize(istream_t& stream)
	{
		stream >> x >> y >> z >> w;
	}

	vec4f_t color_t::to_vector() const
	{
		return vec4f_t(x, y, z, w);
	}

} // namespace sfg

namespace sfg
{
	color_reflection_t::color_reflection_t()
	{
		reflection_registry_t& registry = reflection_registry_t::get();

		registry.register_type({
			.name = "color_t",
			.fields =
				{
					{.name = "x", .offset = offsetof(color_t, x), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "y", .offset = offsetof(color_t, y), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "z", .offset = offsetof(color_t, z), .size = sizeof(f32), .type = reflected_value_type_e::f32},
					{.name = "w", .offset = offsetof(color_t, w), .size = sizeof(f32), .type = reflected_value_type_e::f32},
				},
			.type_id   = type_id_t<color_t>::value,
			.size	   = sizeof(color_t),
			.alignment = alignof(color_t),
		});
	}
}
