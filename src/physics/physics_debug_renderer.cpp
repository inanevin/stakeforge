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

#ifdef JPH_DEBUG_RENDERER

#include "physics_debug_renderer.hpp"
#include "physics/physics_convert.hpp"
#include "world/world.hpp"
#include "math/color.hpp"

#include <Jolt/Physics/PhysicsSystem.h>

namespace SFG
{
	physics_debug_renderer::physics_debug_renderer()
	{
		JPH::DebugRenderer::Initialize();
	}

	physics_debug_renderer::~physics_debug_renderer()
	{
	}

	void physics_debug_renderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
	{
		const vector3 from		= from_jph_vec3(inFrom);
		const vector3 to		= from_jph_vec3(inTo);
		constexpr f32 thickness = 0.25f;
		_w->get_debug_rendering().draw_line(from, to, color::from255(inColor.r, inColor.g, inColor.b, inColor.a), thickness);
	}

	void physics_debug_renderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, JPH::DebugRenderer::ECastShadow inCastShadow)
	{
		_w->get_debug_rendering().draw_triangle(from_jph_vec3(inV1), from_jph_vec3(inV2), from_jph_vec3(inV3), color::from255(inColor.r, inColor.g, inColor.b, inColor.a));
	}

	void physics_debug_renderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, f32 inHeight)
	{
	}

	JPH::DebugRenderer::Batch physics_debug_renderer::CreateTriangleBatch(const JPH::DebugRenderer::Triangle* inTriangles, int inTriangleCount)
	{
		BatchImpl* batch = new BatchImpl;
		if (inTriangles == nullptr || inTriangleCount == 0)
			return batch;

		batch->mTriangles.assign(inTriangles, inTriangles + inTriangleCount);
		return batch;
	}

	JPH::DebugRenderer::Batch physics_debug_renderer::CreateTriangleBatch(const JPH::DebugRenderer::Vertex* inVertices, int inVertexCount, const u32* inIndices, int inIndexCount)
	{
		BatchImpl* batch = new BatchImpl;
		if (inVertices == nullptr || inVertexCount == 0 || inIndices == nullptr || inIndexCount == 0)
			return batch;

		// Convert indexed triangle list to triangle list
		batch->mTriangles.resize(inIndexCount / 3);
		for (size_t t = 0; t < batch->mTriangles.size(); ++t)
		{
			JPH::DebugRenderer::Triangle& triangle = batch->mTriangles[t];
			triangle.mV[0]						   = inVertices[inIndices[t * 3 + 0]];
			triangle.mV[1]						   = inVertices[inIndices[t * 3 + 1]];
			triangle.mV[2]						   = inVertices[inIndices[t * 3 + 2]];
		}

		return batch;
	}

	void physics_debug_renderer::DrawGeometry(JPH::RMat44Arg						 inModelMatrix,
											  const JPH::AABox&						 inWorldSpaceBounds,
											  f32									 inLODScaleSq,
											  JPH::ColorArg							 inModelColor,
											  const JPH::DebugRenderer::GeometryRef& inGeometry,
											  JPH::DebugRenderer::ECullMode			 inCullMode,
											  JPH::DebugRenderer::ECastShadow		 inCastShadow,
											  JPH::DebugRenderer::EDrawMode			 inDrawMode)
	{
		// Figure out which LOD to use
		const JPH::DebugRenderer::LOD* lod = inGeometry->mLODs.data();

		// Draw the batch
		const BatchImpl* batch = static_cast<const BatchImpl*>(lod->mTriangleBatch.GetPtr());
		for (const JPH::DebugRenderer::Triangle& triangle : batch->mTriangles)
		{
			JPH::RVec3 v0	 = inModelMatrix * JPH::Vec3(triangle.mV[0].mPosition);
			JPH::RVec3 v1	 = inModelMatrix * JPH::Vec3(triangle.mV[1].mPosition);
			JPH::RVec3 v2	 = inModelMatrix * JPH::Vec3(triangle.mV[2].mPosition);
			JPH::Color color = inModelColor * triangle.mV[0].mColor;

			switch (inDrawMode)
			{
			case EDrawMode::Wireframe:
				DrawLine(v0, v1, color);
				DrawLine(v1, v2, color);
				DrawLine(v2, v0, color);
				break;

			case EDrawMode::Solid:
				DrawTriangle(v0, v1, v2, color, inCastShadow);
				break;
			}
		}
	}

	void physics_debug_renderer::draw(world& w)
	{
		_w								  = &w;
		JPH::BodyManager::DrawSettings ds = {};
		ds.mDrawShape					  = true;
		ds.mDrawVelocity				  = true;
		w.get_physics_world().get_system()->DrawBodies(ds, this);
	}

}

#endif
