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

#ifdef JPH_DEBUG_RENDERER

#include "common/size_definitions.hpp"
#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRenderer.h>

namespace SFG
{

	class world;

	class physics_debug_renderer : public JPH::DebugRenderer
	{
	public:
		physics_debug_renderer();
		virtual ~physics_debug_renderer();

		// -----------------------------------------------------------------------------
		// JPH override
		// -----------------------------------------------------------------------------

		virtual void					  DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
		virtual void					  DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, JPH::DebugRenderer::ECastShadow inCastShadow) override;
		virtual void					  DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, f32 inHeight) override;
		virtual JPH::DebugRenderer::Batch CreateTriangleBatch(const JPH::DebugRenderer::Triangle* inTriangles, int inTriangleCount) override;
		virtual JPH::DebugRenderer::Batch CreateTriangleBatch(const JPH::DebugRenderer::Vertex* inVertices, int inVertexCount, const u32* inIndices, int inIndexCount) override;

		virtual void DrawGeometry(JPH::RMat44Arg						 inModelMatrix,
								  const JPH::AABox&						 inWorldSpaceBounds,
								  f32									 inLODScaleSq,
								  JPH::ColorArg							 inModelColor,
								  const JPH::DebugRenderer::GeometryRef& inGeometry,
								  JPH::DebugRenderer::ECullMode			 inCullMode	  = JPH::DebugRenderer::ECullMode::CullBackFace,
								  JPH::DebugRenderer::ECastShadow		 inCastShadow = JPH::DebugRenderer::ECastShadow::On,
								  JPH::DebugRenderer::EDrawMode			 inDrawMode	  = JPH::DebugRenderer::EDrawMode::Solid) override;

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void draw(world& w);

	private:
		class BatchImpl : public JPH::RefTargetVirtual
		{
		public:
			JPH_OVERRIDE_NEW_DELETE

			virtual void AddRef() override
			{
				++mRefCount;
			}
			virtual void Release() override
			{
				if (--mRefCount == 0)
					delete this;
			}

			JPH::Array<JPH::DebugRenderer::Triangle> mTriangles;

		private:
			JPH::atomic<u32> mRefCount = 0;
		};

	private:
		world* _w = nullptr;
	};
}

#endif