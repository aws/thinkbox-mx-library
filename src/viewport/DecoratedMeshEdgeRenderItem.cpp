// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <maxversion.h>

#if MAX_VERSION_MAJOR >= 17

#include <frantic/max3d/viewport/DecoratedMeshEdgeRenderItem.hpp>

#if MAX_VERSION_MAJOR >= 25
#include <SharedMesh.h>
#endif

namespace frantic {
namespace max3d {
namespace viewport {

#if MAX_VERSION_MAJOR >= 25

DecoratedMeshEdgeRenderItem::DecoratedMeshEdgeRenderItem( MaxSDK::SharedMeshPtr mesh, bool fixedSize,
                                                          const MaxSDK::Graphics::Matrix44& tm )
    : MeshEdgeRenderItem( mesh, fixedSize )
    , m_iconTM( tm ) {}

#else

DecoratedMeshEdgeRenderItem::DecoratedMeshEdgeRenderItem( const Mesh* mesh, bool fixedSize, bool copyMesh,
                                                          const MaxSDK::Graphics::Matrix44& tm )
    : MeshEdgeRenderItem( const_cast<Mesh*>( mesh ), fixedSize, copyMesh )
    , m_iconTM( tm ) {}

#endif

void DecoratedMeshEdgeRenderItem::Realize( MaxSDK::Graphics::DrawContext& drawContext ) {
    // Capture the node's object to world transformation
    if( INode* pNode = drawContext.GetCurrentNode() ) {
        MaxSDK::Graphics::Matrix44 worldTM;
        MaxSDK::Graphics::MaxWorldMatrixToMatrix44( worldTM, pNode->GetNodeTM( drawContext.GetTime() ) );
        MaxSDK::Graphics::Matrix44::Multiply( m_finalTM, m_iconTM, worldTM );
    } else {
        m_finalTM = m_iconTM;
    }

    if( INode* pNode = drawContext.GetCurrentNode() ) {
        Color c( 1, 1, 1 );
        if( !pNode->Selected() )
            c = Color( pNode->GetWireColor() );
        this->SetColor( c );
    }

    drawContext.SetWorldMatrix( m_finalTM );

    MeshEdgeRenderItem::Realize( drawContext );
}

void DecoratedMeshEdgeRenderItem::Display( MaxSDK::Graphics::DrawContext& drawContext ) {
    // const MaxSDK::Graphics::Matrix44& wrongTM = drawContext.GetWorldMatrix();

    drawContext.SetWorldMatrix( m_finalTM );

    MeshEdgeRenderItem::Display( drawContext );
}

void DecoratedMeshEdgeRenderItem::HitTest( MaxSDK::Graphics::HitTestContext& htContext,
                                           MaxSDK::Graphics::DrawContext& drawContext ) {
    // const MaxSDK::Graphics::Matrix44& wrongTM = drawContext.GetWorldMatrix();

    drawContext.SetWorldMatrix( m_finalTM );

    MeshEdgeRenderItem::HitTest( htContext, drawContext );
}

} // namespace viewport
} // namespace max3d
} // namespace frantic

#endif
