// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "frantic/graphics/boundbox3f.hpp"
#include "frantic/graphics/color3f.hpp"
#include "frantic/graphics/quat4f.hpp"
#include "frantic/graphics/transform4f.hpp"
#include "frantic/graphics/vector3f.hpp"
#include "frantic/graphics2d/vector2f.hpp"

#pragma warning( push, 3 )
#if MAX_RELEASE >= 25000
#include <geom/point2.h>
#include <geom/point3.h>
#include <geom/quat.h>
#include <geom/color.h>
#include <geom/matrix3.h>
#include <geom/box3.h>
#else
#include <point2.h>
#include <point3.h>
#include <quat.h>
#include <color.h>
#include <matrix3.h>
#include <box3.h>
#endif
#pragma warning( pop )

namespace frantic {
namespace max3d {

inline Point2 to_max_t( const frantic::graphics2d::vector2f& v ) { return Point2( v.x, v.y ); }

inline Point3 to_max_t( const frantic::graphics::vector3f& v ) { return Point3( v.x, v.y, v.z ); }

inline Color to_max_t( const frantic::graphics::color3f& c ) { return Color( c.r, c.g, c.b ); }

inline Quat to_max_t( const frantic::graphics::quat4f& q ) { return Quat( q.x, q.y, q.z, q.w ); }

inline frantic::graphics2d::vector2f from_max_t( const Point2& p ) { return frantic::graphics2d::vector2f( p.x, p.y ); }

inline frantic::graphics::vector3f from_max_t( const Point3& p ) {
    return frantic::graphics::vector3f( p.x, p.y, p.z );
}

inline frantic::graphics::color3f from_max_t( const Color& c ) { return frantic::graphics::color3f( c.r, c.g, c.b ); }

inline frantic::graphics::quat4f from_max_t( const Quat& q ) { return frantic::graphics::quat4f( q.w, q.x, q.y, q.z ); }

inline Box3 to_max_t( const frantic::graphics::boundbox3f& b ) {
    return Box3( to_max_t( b.minimum() ), to_max_t( b.maximum() ) );
}

inline frantic::graphics::boundbox3f from_max_t( const Box3& b ) {
    return frantic::graphics::boundbox3f( from_max_t( b.Min() ), from_max_t( b.Max() ) );
}

// NOTE: Matrix3 "row"s are the same as transform3f "column"s.  We are using
//       column vectors, while 3ds Max is using row vectors.
inline frantic::graphics::transform4f from_max_t( const Matrix3& maxmat ) {
    const Point3& column1 = maxmat[0];
    const Point3& column2 = maxmat[1];
    const Point3& column3 = maxmat[2];
    const Point3& column4 = maxmat[3];

    return frantic::graphics::transform4f( column1.x, column1.y, column1.z, 0.0f, column2.x, column2.y, column2.z, 0.0f,
                                           column3.x, column3.y, column3.z, 0.0f, column4.x, column4.y, column4.z,
                                           1.0f );
}

inline Matrix3 to_max_t( const frantic::graphics::transform4f& t ) {
    Matrix3 mat;
    // Make sure it can be assigned to a Matrix3
    assert( t[3] == 0 );
    assert( t[7] == 0 );
    assert( t[11] == 0 );
    assert( t[15] == 1 );

    mat.Set( Point3( t[0], t[1], t[2] ), Point3( t[4], t[5], t[6] ), Point3( t[8], t[9], t[10] ),
             Point3( t[12], t[13], t[14] ) );
    return mat;
}

inline bool to_bool( BOOL b ) { return ( b != FALSE ); }

} // namespace max3d
} // namespace frantic
