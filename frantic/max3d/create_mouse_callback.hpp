// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/function.hpp>

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>

#pragma warning( push, 3 )
#include <max.h>
#pragma warning( pop )

namespace frantic {
namespace max3d {

/**
 * A CreateMouseCallback implementation that creates and sizes an object with a click and drag. The object is complete
 * after the mouse click is released.
 */
class ClickAndDragCreateCallBack : public CreateMouseCallBack {
    IPoint2 sp1;
    boost::function<void( float )> m_resizeCallback;

  public:
    /**
     * Registers the callback that is invoked to size the object being created. This is invoked when a mouse-move event
     * is registered while holding the initial click.
     */
    template <class Callable>
    void set_resize_callback( const Callable& callable ) {
        m_resizeCallback = callable;
    }

    /**
     * Called to handle a mouse event.
     */
    virtual int proc( ViewExp* vpt, int msg, int point, int /*flags*/, IPoint2 m, Matrix3& mat ) {
        if( msg == MOUSE_POINT ) {
            switch( point ) {
            case 0: {
                sp1 = m;

                Point3 p = vpt->SnapPoint( m, m, NULL, SNAP_IN_3D );
                mat.IdentityMatrix();
                mat.SetTrans( p );
            } break;
            case 1:
                return CREATE_STOP;
            }
        } else if( msg == MOUSE_MOVE ) {
            switch( point ) {
            case 1: {
                float dist = vpt->GetCPDisp( Point3( 0, 0, 0 ), Point3( (float)M_SQRT2, (float)M_SQRT2, 0.f ), sp1, m );
                dist = vpt->SnapLength( dist );
                dist = 2.f * fabsf( dist );

                if( m_resizeCallback )
                    m_resizeCallback( dist );
            } break;
            }
        } else if( msg == MOUSE_ABORT ) {
            return CREATE_ABORT;
        }

        return CREATE_CONTINUE;
    }
};

} // namespace max3d
} // namespace frantic
