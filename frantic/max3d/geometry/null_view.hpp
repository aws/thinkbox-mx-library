// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace frantic {
namespace max3d {
namespace geometry {

/**
 * This class provides a trivial View object that can be constructed for calling GetRenderMesh on
 * geometry objects when outside a renderer context.
 */
class null_view : public View {
  public:
    Point2 ViewToScreen( Point3 p ) { return Point2( p.x, p.y ); }
    null_view() {
        worldToView.IdentityMatrix();
        screenW = 640.0f;
        screenH = 480.0f;
    }
};

} // namespace geometry
} // namespace max3d
} // namespace frantic
