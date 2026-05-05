// Copyright (c) 2025 Inan Evin

#include "resource_cooker.hpp"
#include "animation_state_machine_cook.hpp"
#include "audio_cook.hpp"
#include "font_cook.hpp"
#include "glb_cook.hpp"
#include "material_cook.hpp"
#include "particle_properties_cook.hpp"
#include "physical_material_cook.hpp"
#include "prefab_cook.hpp"
#include "shader_cook.hpp"
#include "texture_cook.hpp"
#include "texture_sampler_cook.hpp"

#include <sfg/io/file_system.hpp>
#include <sfg/io/log.hpp>

namespace sfg
{
	namespace
	{
		template <typename cooker_t, typename config_t> resource_cooker_t::result_e dispatch(const char* full_path, const config_t& cfg, ostream_t& stream)
		{
			if (full_path == nullptr || !file_system_t::exists(full_path))
			{
				SFG_ERR("source file does not exist: {0}", full_path);
				return resource_cooker_t::result_e::invalid_path;
			}

			if (!cooker_t::cook_from_file(cfg, full_path, stream))
			{
				SFG_ERR("failed to cook {0}", full_path);
				return resource_cooker_t::result_e::cook_failed;
			}

			return resource_cooker_t::result_e::success;
		}
	}

	resource_cooker_t::result_e resource_cooker_t::cook_texture(const char* full_path, const texture_cook_config_t& cfg, ostream_t& stream)
	{
		return dispatch<texture_cooker>(full_path, cfg, stream);
	}

	resource_cooker_t::result_e resource_cooker_t::cook_shader(const char* full_path, const shader_cook_config_t& cfg, ostream_t& stream)
	{
		return dispatch<shader_cooker>(full_path, cfg, stream);
	}

	resource_cooker_t::result_e resource_cooker_t::cook_audio(const char* full_path, const audio_cook_config_t& cfg, ostream_t& stream)
	{
		return dispatch<audio_cooker>(full_path, cfg, stream);
	}

	resource_cooker_t::result_e resource_cooker_t::cook_glb(const char* full_path, const glb_cook_config_t& cfg, ostream_t& stream)
	{
		return dispatch<glb_cooker>(full_path, cfg, stream);
	}

	resource_cooker_t::result_e resource_cooker_t::cook_font(const char* full_path, const font_cook_config_t& cfg, ostream_t& stream)
	{
		return dispatch<font_cooker>(full_path, cfg, stream);
	}

	resource_cooker_t::result_e resource_cooker_t::cook_material(const char* full_path, const material_cook_config_t& cfg, ostream_t& stream)
	{
		return dispatch<material_cooker>(full_path, cfg, stream);
	}

	resource_cooker_t::result_e resource_cooker_t::cook_particle_properties(const char* full_path, const particle_properties_cook_config_t& cfg, ostream_t& stream)
	{
		return dispatch<particle_properties_cooker>(full_path, cfg, stream);
	}

	resource_cooker_t::result_e resource_cooker_t::cook_texture_sampler(const char* full_path, const texture_sampler_cook_config_t& cfg, ostream_t& stream)
	{
		return dispatch<texture_sampler_cooker>(full_path, cfg, stream);
	}

	resource_cooker_t::result_e resource_cooker_t::cook_physical_material(const char* full_path, const physical_material_cook_config_t& cfg, ostream_t& stream)
	{
		return dispatch<physical_material_cooker>(full_path, cfg, stream);
	}

	resource_cooker_t::result_e resource_cooker_t::cook_animation_state_machine(const char* full_path, const animation_state_machine_cook_config_t& cfg, ostream_t& stream)
	{
		return dispatch<animation_state_machine_cooker>(full_path, cfg, stream);
	}

	resource_cooker_t::result_e resource_cooker_t::cook_prefab(const char* full_path, const prefab_cook_config_t& cfg, ostream_t& stream)
	{
		return dispatch<prefab_cooker>(full_path, cfg, stream);
	}
}
