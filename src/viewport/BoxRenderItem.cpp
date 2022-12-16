// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <maxversion.h>

#if MAX_VERSION_MAJOR >= 17

#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/viewport/BoxRenderItem.hpp>

#pragma warning( push, 3 )
#include <Graphics/IVirtualDevice.h>
#pragma warning( pop )

namespace frantic {
namespace max3d {
namespace viewport {

BoxRenderItem::BoxRenderItem()
    : m_valid( false )
    , m_inWorldSpace( true )
    , m_realized( false )
    , m_finalTM( MaxSDK::Graphics::Matrix44().MakeIdentity() ) {}

void BoxRenderItem::Initialize( const Box3& bounds ) {
    m_vbuffers.removeAll();
    m_vbufferDesc.Clear();
#if MAX_VERSION_MAJOR < 19
    m_ibuffer.Initialize( MaxSDK::Graphics::IndexTypeShort );
#else
    m_ibuffer.Initialize( MaxSDK::Graphics::IndexTypeShort, 24 );
#endif
    m_solidColorHandle.Initialize();

    MaxSDK::Graphics::MaterialRequiredStreamElement posElem, normElem;

    posElem.SetType( MaxSDK::Graphics::VertexFieldFloat3 );
    posElem.SetUsageIndex( 0 );
    posElem.SetOffset( 0 );
    posElem.SetStreamIndex( 0 );
    posElem.SetChannelCategory( MaxSDK::Graphics::MeshChannelPosition );
    m_vbufferDesc.AddStream( posElem );

    // The built-in SolidColorMaterial requires a normal channel, even though it isn't used. Dumb. I just reuse the
    // position data since it doesn't affect anything.
    normElem.SetType( MaxSDK::Graphics::VertexFieldFloat3 );
    normElem.SetUsageIndex( MaxSDK::Graphics::VertexFieldUsageNormal );
    normElem.SetOffset( 0 );
    normElem.SetStreamIndex( 0 );
    normElem.SetChannelCategory( MaxSDK::Graphics::MeshChannelVertexNormal );
    m_vbufferDesc.AddStream( normElem );

    MaxSDK::Graphics::VertexBufferHandle boundsBuffer;
#if MAX_VERSION_MAJOR < 19
    boundsBuffer.Initialize( MaxSDK::Graphics::GetVertexStride( MaxSDK::Graphics::VertexFieldFloat3 ) );
    boundsBuffer.SetBufferUsageType( MaxSDK::Graphics::BufferUsageStatic );
    boundsBuffer.SetNumberOfVertices( 8 );
#else
    boundsBuffer.Initialize( MaxSDK::Graphics::GetVertexStride( MaxSDK::Graphics::VertexFieldFloat3 ), 8,
                             (void*)nullptr, MaxSDK::Graphics::BufferUsageStatic );
#endif

    Point3* pBuffer = static_cast<Point3*>( static_cast<void*>( boundsBuffer.Lock( 0, 8 ) ) );
    if( pBuffer ) {
        pBuffer[0] = bounds.pmin;
        pBuffer[1].Set( bounds.pmax.x, bounds.pmin.y, bounds.pmin.z );
        pBuffer[2].Set( bounds.pmin.x, bounds.pmax.y, bounds.pmin.z );
        pBuffer[3].Set( bounds.pmax.x, bounds.pmax.y, bounds.pmin.z );
        pBuffer[4].Set( bounds.pmin.x, bounds.pmin.y, bounds.pmax.z );
        pBuffer[5].Set( bounds.pmax.x, bounds.pmin.y, bounds.pmax.z );
        pBuffer[6].Set( bounds.pmin.x, bounds.pmax.y, bounds.pmax.z );
        pBuffer[7] = bounds.pmax;
    }
    boundsBuffer.Unlock();

    m_vbuffers.append( boundsBuffer );
#if MAX_VERSION_MAJOR < 19
    m_ibuffer.SetNumberOfIndices( 24 );
#endif
    short* pIBuffer =
        static_cast<short*>( static_cast<void*>( m_ibuffer.Lock( 0, 24, MaxSDK::Graphics::WriteAcess ) ) );
    if( pIBuffer ) {
        pIBuffer[0] = 0;
        pIBuffer[1] = 1;
        pIBuffer[2] = 1;
        pIBuffer[3] = 3;
        pIBuffer[4] = 3;
        pIBuffer[5] = 2;
        pIBuffer[6] = 2;
        pIBuffer[7] = 0;
        pIBuffer[8] = 4;
        pIBuffer[9] = 5;
        pIBuffer[10] = 5;
        pIBuffer[11] = 7;
        pIBuffer[12] = 7;
        pIBuffer[13] = 6;
        pIBuffer[14] = 6;
        pIBuffer[15] = 4;
        pIBuffer[16] = 0;
        pIBuffer[17] = 4;
        pIBuffer[18] = 1;
        pIBuffer[19] = 5;
        pIBuffer[20] = 3;
        pIBuffer[21] = 7;
        pIBuffer[22] = 2;
        pIBuffer[23] = 6;
    }
    m_ibuffer.Unlock();

    m_valid = true;
}

void BoxRenderItem::SetInWorldSpace( bool inWorldSpace ) { m_inWorldSpace = inWorldSpace; }

void BoxRenderItem::Realize( MaxSDK::Graphics::DrawContext& drawContext ) {
    MaxSDK::Graphics::IVirtualDevice& dev = drawContext.GetVirtualDevice();
    if( !m_valid || !dev.IsValid() )
        return;

    // Capture the node's object to world transformation
    INode* pNode = drawContext.GetCurrentNode();
    if( pNode ) {
        MaxSDK::Graphics::Matrix44 worldTM;
        MaxSDK::Graphics::MaxWorldMatrixToMatrix44( worldTM, pNode->GetNodeTM( drawContext.GetTime() ) );
        // SetWorldMatrix get's applied on top of the initial node TM causing the node TM to be applied twice
        // so we must apply the inverse of the init node TM to get it to display properly which we get the first time
        // realize is called
        if( !m_realized ) {
            MaxSDK::Graphics::MaxWorldMatrixToMatrix44(
                m_initTM,
                ( frantic::max3d::to_max_t(
                    frantic::max3d::from_max_t( pNode->GetNodeTM( drawContext.GetTime() ) ).to_inverse() ) ) );
            m_realized = true;
        }
        MaxSDK::Graphics::Matrix44::Multiply( m_finalTM, m_initTM, worldTM );
    } else {
        m_finalTM = MaxSDK::Graphics::Matrix44().MakeIdentity();
    }

    if( m_inWorldSpace )
        drawContext.SetWorldMatrix( m_finalTM );
}

void BoxRenderItem::Display( MaxSDK::Graphics::DrawContext& drawContext ) {
    MaxSDK::Graphics::IVirtualDevice& dev = drawContext.GetVirtualDevice();
    if( !m_valid || !dev.IsValid() )
        return;

    if( m_inWorldSpace )
        drawContext.SetWorldMatrix( m_finalTM );

    dev.SetVertexStreams( m_vbuffers );
    dev.SetStreamFormat( m_vbufferDesc );
#if MAX_VERSION_MAJOR >= 19
    dev.SetIndexBuffer( m_ibuffer );
#else
    dev.SetIndexBuffer( &m_ibuffer );
#endif

    if( INode* pNode = drawContext.GetCurrentNode() )
        m_solidColorHandle.SetColor( pNode->Selected() ? Color( 1, 1, 1 ) : Color( pNode->GetWireColor() ) );

    m_solidColorHandle.Activate( drawContext );
    unsigned passCount = m_solidColorHandle.GetPassCount( drawContext );
    for( unsigned pass = 0; pass < passCount; ++pass ) {
        m_solidColorHandle.ActivatePass( drawContext, pass );
        dev.Draw( MaxSDK::Graphics::PrimitiveLineList, 0, 12 );
    }
    m_solidColorHandle.PassesFinished( drawContext );
    m_solidColorHandle.Terminate();
}

size_t BoxRenderItem::GetPrimitiveCount() const { return m_valid ? 12 : 0; }

} // namespace viewport
} // namespace max3d
} // namespace frantic

#endif
