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

namespace SFG
{

	// -----------------------------------------------------------------------------
	// entities
	// -----------------------------------------------------------------------------

#define MAX_ENTITIES 256000

	// -----------------------------------------------------------------------------
	// resources
	// -----------------------------------------------------------------------------

#define MAX_WORLD_RESOURCES_AUX_MEMORY 1024 * 1024 * 32
#define MAX_WORLD_TEXTURES			   32
#define MAX_WORLD_MODELS			   32
#define MAX_WORLD_MESHES			   256
#define MAX_WORLD_ANIMS				   1024
#define MAX_WORLD_SKINS				   12
#define MAX_WORLD_MATERIALS			   256
#define MAX_WORLD_SHADERS			   30
#define MAX_WORLD_AUDIO				   50
#define MAX_WORLD_FONTS				   20
#define MAX_WORLD_PHYSICAL_MATERIALS   30
#define MAX_WORLD_SAMPLERS			   16
#define MAX_TEXTURE_MIPS			   10
#define MAX_MATERIAL_TEXTURES		   8
#define MAX_WORLD_PARTICLE_PROPERTIES  512
#define MAX_WORLD_ANIM_STATE_MACHINES  512
#define MAX_WORLD_ENTITY_TEMPLATES	   512

	// -----------------------------------------------------------------------------
	// traits
	// -----------------------------------------------------------------------------

#define MAX_WORLD_COMPONENTS_AUX_MEMORY		 1024 * 1024 * 6
#define MAX_WORLD_COMP_CAMERAS				 32
#define MAX_WORLD_COMP_DIR_LIGHTS			 8
#define MAX_WORLD_COMP_POINT_LIGHTS			 128
#define MAX_WORLD_COMP_SPOT_LIGHTS			 128
#define MAX_WORLD_COMP_MESH_INSTANCES		 1024 * 10
#define MAX_WORLD_COMP_AMBIENT				 1
#define MAX_WORLD_COMP_BLOOM				 1
#define MAX_WORLD_COMP_SSAO					 1
#define MAX_WORLD_COMP_POST_PROCESS			 1
#define MAX_WORLD_COMP_SKYBOX				 1
#define MAX_WORLD_COMP_PHYSICS_SETTINGS		 1
#define MAX_WORLD_COMP_PHYSICS				 1000
#define MAX_WORLD_COMP_CHARACTER_CONTROLLERS 128
#define MAX_WORLD_COMP_AUDIO				 256
#define MAX_WORLD_COMP_CANVAS				 32
#define MAX_WORLD_COMP_ANIMS				 1024 * 10
#define MAX_WORLD_SKELETON_JOINTS			 128
#define MAX_WORLD_COMP_PARTICLE_EMITTERS	 384
#define MAX_WORLD_COMP_SPRITES				 2048

	// -----------------------------------------------------------------------------
	// animations
	// -----------------------------------------------------------------------------
#define MAX_WORLD_BLEND_STATE_ANIMS		   5
#define MAX_WORLD_ANIM_GRAPH_STATES		   MAX_WORLD_COMP_ANIMS * 10
#define MAX_WORLD_ANIM_GRAPH_STATE_SAMPLES MAX_WORLD_COMP_ANIMS * 10
#define MAX_WORLD_ANIM_GRAPH_TRANSITION	   MAX_WORLD_COMP_ANIMS * 20
#define MAX_WORLD_ANIM_GRAPH_PARAMETER	   MAX_WORLD_COMP_ANIMS * 10
#define MAX_WORLD_ANIM_GRAPH_MASK		   32

	// 128K bones in total, 64 max limit per skeleton.
#define MAX_WORLD_BONE_BATCH_SIZE 64
#define MAX_WORLD_BONE_BATCHES	  10000

	// -----------------------------------------------------------------------------
	// particles
	// -----------------------------------------------------------------------------

#define MAX_WORLD_PARTICLES_PER_EMITTER 512
#define MAX_WORLD_PARTICLES				MAX_WORLD_COMP_PARTICLE_EMITTERS* MAX_WORLD_PARTICLES_PER_EMITTER

	// -----------------------------------------------------------------------------
	// gfx
	// -----------------------------------------------------------------------------

#define MAX_RENDERABLES_ALL				1024 * 20
#define MAX_DRAW_CALLS_CANVAS_2D		1024
#define MAX_DRAW_CALLS_FORWARD			1024 * 2
#define MAX_DRAW_CALLS_ALL_OBJECTS		1024 * 24
#define MAX_DRAW_CALLS_OPAQUE			1024 * 20
#define MAX_DRAW_CALLS_DEBUG_RENDERING	1024 * 10
#define MAX_DRAW_CALLS_SPRITE			1024 * 2
#define PASS_ALLOC_SIZE_CANVAS_2D		MAX_DRAW_CALLS_CANVAS_2D * 64
#define PASS_ALLOC_SIZE_FORWARD			MAX_DRAW_CALLS_FORWARD * 64
#define PASS_ALLOC_SIZE_ALL_OBJECTS		MAX_DRAW_CALLS_ALL_OBJECTS * 64
#define PASS_ALLOC_SIZE_OPAQUE			MAX_DRAW_CALLS_OPAQUE * 64
#define PASS_ALLOC_SIZE_PARTICLES		MAX_WORLD_COMP_PARTICLE_EMITTERS * 20
#define PASS_ALLOC_SIZE_DEBUG_RENDERING MAX_DRAW_CALLS_DEBUG_RENDERING * 64
#define PASS_ALLOC_SIZE_SPRITE			MAX_DRAW_CALLS_SPRITE * 64
#define MAX_GPU_BONES					MAX_ENTITIES
#define MAX_GPU_ENTITIES				MAX_ENTITIES
#define MAX_GPU_SHADOW_DATA				64
#define MAX_SHADOW_CASCADES				8
#define LIGHT_CULLING_ENERGY_THRESHOLD	0.01f
#define RENDER_STREAM_BATCH_SIZE		1024 * 1024 * 64 // * 5 default
#define RENDER_STREAM_MAX_BATCHES		4

}
