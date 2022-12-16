// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace frantic {
namespace max3d {

class renderglobalcontext_view : public View {
    RenderGlobalContext* pRendParams;

  public:
    void set_renderglobalcontext( RenderGlobalContext* rgc ) { pRendParams = rgc; }

    Point2 ViewToScreen( Point3 p ) {
        if( pRendParams )
            return pRendParams->MapToScreen( p );
        else
            return Point2( p.x, p.y );
    }

    renderglobalcontext_view() {
        worldToView.IdentityMatrix();
        screenW = 640.0f;
        screenH = 480.0f;
        pRendParams = 0;
    }

    renderglobalcontext_view( RenderGlobalContext* rgc ) {
        worldToView.IdentityMatrix();
        screenW = 640.0f;
        screenH = 480.0f;
        pRendParams = rgc;
    }
};

} // namespace max3d
} // namespace frantic
