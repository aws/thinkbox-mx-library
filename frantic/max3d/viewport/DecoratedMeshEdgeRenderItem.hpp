// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#pragma warning( push, 3 )

#include <maxversion.h>

#if MAX_VERSION_MAJOR >= 17

#if MAX_RELEASE >= 25000
#include <SharedMesh.h>
#include <geom/Box3.h>
#else
#include <Box3.h>
#include <mesh.h>
#endif

#include <Graphics/CustomRenderItemHandle.h>
#include <Graphics/IndexBufferHandle.h>
#include <Graphics/SolidColorMaterialHandle.h>
#include <Graphics/VertexBufferHandle.h>

#include <Graphics/Utilities/MeshEdgeRenderItem.h>
#pragma warning( pop )

namespace frantic {
namespace max3d {
namespace viewport {

// I have confirmed (at least in RTM Max2015) that the MeshEdgeRenderItem when used with a RenderItemHandleDecorator (to
// apply a scale for example) causes the node TM to be applied twice for the HitTest routine (it displays just fine).
// This object hacks around that stupidity.
class DecoratedMeshEdgeRenderItem : public MaxSDK::Graphics::Utilities::MeshEdgeRenderItem {
  public:
#if MAX_VERSION_MAJOR >= 25
    DecoratedMeshEdgeRenderItem( MaxSDK::SharedMeshPtr mesh, bool fixedSize, const MaxSDK::Graphics::Matrix44& tm );
#else
    DecoratedMeshEdgeRenderItem( const Mesh* mesh, bool fixedSize, bool copyMesh,
                                 const MaxSDK::Graphics::Matrix44& tm );
#endif

    virtual void Realize( MaxSDK::Graphics::DrawContext& drawContext );

    virtual void Display( MaxSDK::Graphics::DrawContext& drawContext );

    virtual void HitTest( MaxSDK::Graphics::HitTestContext& htContext, MaxSDK::Graphics::DrawContext& drawContext );

  private:
    MaxSDK::Graphics::Matrix44 m_iconTM, m_finalTM;
};

} // namespace viewport
} // namespace max3d
} // namespace frantic

#endif