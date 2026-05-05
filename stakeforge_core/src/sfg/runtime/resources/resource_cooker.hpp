// Copyright (c) 2025 Inan Evin
#pragma once

#include <sfg/common/size_definitions.hpp>

namespace sfg
{
	class ostream_t;

	struct texture_cook_config_t;
	struct shader_cook_config_t;
	struct audio_cook_config_t;
	struct glb_cook_config_t;
	struct font_cook_config_t;
	struct material_cook_config_t;
	struct particle_properties_cook_config_t;
	struct texture_sampler_cook_config_t;
	struct physical_material_cook_config_t;
	struct animation_state_machine_cook_config_t;
	struct prefab_cook_config_t;

	class resource_cooker_t
	{
	public:
		enum class result_e : u8
		{
			success,
			invalid_path,
			cook_failed,
		};

		static result_e cook_texture(const char* full_path, const texture_cook_config_t& cfg, ostream_t& stream);
		static result_e cook_shader(const char* full_path, const shader_cook_config_t& cfg, ostream_t& stream);
		static result_e cook_audio(const char* full_path, const audio_cook_config_t& cfg, ostream_t& stream);
		static result_e cook_glb(const char* full_path, const glb_cook_config_t& cfg, ostream_t& stream);
		static result_e cook_font(const char* full_path, const font_cook_config_t& cfg, ostream_t& stream);
		static result_e cook_material(const char* full_path, const material_cook_config_t& cfg, ostream_t& stream);
		static result_e cook_particle_properties(const char* full_path, const particle_properties_cook_config_t& cfg, ostream_t& stream);
		static result_e cook_texture_sampler(const char* full_path, const texture_sampler_cook_config_t& cfg, ostream_t& stream);
		static result_e cook_physical_material(const char* full_path, const physical_material_cook_config_t& cfg, ostream_t& stream);
		static result_e cook_animation_state_machine(const char* full_path, const animation_state_machine_cook_config_t& cfg, ostream_t& stream);
		static result_e cook_prefab(const char* full_path, const prefab_cook_config_t& cfg, ostream_t& stream);
	};
}
