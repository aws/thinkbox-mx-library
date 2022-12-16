// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#pragma warning( push, 3 )

#include <maxversion.h>

#if MAX_VERSION_MAJOR >= 17

#if MAX_RELEASE >= 25000
#include <geom/Box3.h>
#else
#include <Box3.h>
#endif

#include <Graphics/CustomRenderItemHandle.h>
#include <Graphics/IndexBufferHandle.h>
#include <Graphics/SolidColorMaterialHandle.h>
#include <Graphics/VertexBufferHandle.h>
#pragma warning( pop )

namespace frantic {
namespace max3d {
namespace viewport {

class BoxRenderItem : public MaxSDK::Graphics::ICustomRenderItem {
  public:
    BoxRenderItem();

    void Initialize( const Box3& bounds );

    void SetInWorldSpace( bool inWorldSpace );

    virtual void Realize( MaxSDK::Graphics::DrawContext& drawContext );

    virtual void Display( MaxSDK::Graphics::DrawContext& drawContext );

    virtual size_t GetPrimitiveCount() const;

  private:
    bool m_valid;
    bool m_inWorldSpace;
    bool m_realized;
    MaxSDK::Graphics::Matrix44 m_finalTM, m_initTM;
    MaxSDK::Graphics::VertexBufferHandleArray m_vbuffers;
    MaxSDK::Graphics::MaterialRequiredStreams m_vbufferDesc;
    MaxSDK::Graphics::IndexBufferHandle m_ibuffer;
    MaxSDK::Graphics::SolidColorMaterialHandle m_solidColorHandle;
};

} // namespace viewport
} // namespace max3d
} // namespace frantic

#endif
