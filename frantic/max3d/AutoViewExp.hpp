// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace frantic {
namespace max3d {

class AutoViewExp {
    ViewExp* m_pView;

  public:
    AutoViewExp()
        : m_pView( NULL ) {}

    AutoViewExp( ViewExp* pView )
        : m_pView( pView ) {}

    ~AutoViewExp() {
#if MAX_VERSION_MAJOR < 15
        if( m_pView )
            GetCOREInterface()->ReleaseViewport( m_pView );
#endif
        m_pView = NULL;
    }

    void reset( ViewExp* pView ) {
#if MAX_VERSION_MAJOR < 15
        if( m_pView && m_pView != pView )
            GetCOREInterface()->ReleaseViewport( m_pView );
#endif
        m_pView = pView;
    }

    ViewExp* get() { return m_pView; }
};

} // namespace max3d
} // namespace frantic
