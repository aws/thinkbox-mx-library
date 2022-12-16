// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stack>

#pragma warning( push, 3 )
#include <max.h>
#if MAX_RELEASE >= 25000
#include <geom/matrix3.h>
#else
#include <matrix3.h>
#endif
#include <imtl.h>
#include <render.h>
#include <stdmat.h>
#pragma warning( pop )

#include <frantic/graphics/camera.hpp>

namespace frantic {
namespace max3d {
namespace shaders {

/**
 * Defines a class that can carry any additional information the particle sources might need when rendering (camera
 * position, and anything that might come up)
 */
class renderInformation {
  public:
    Point3 m_cameraPosition; // This is here for legacy reasons. Try to make it match the position of 'm_camera'.

    frantic::graphics::camera<float> m_camera;   // The camera active during the render.
    frantic::graphics::transform4f m_toObjectTM; // Transform to object coordinates.
    frantic::graphics::transform4f m_toWorldTM;  // Transform to world coordinates.

    renderInformation() { m_cameraPosition = Point3( 0.f, 0.f, 0.f ); }
    ~renderInformation() {}
} static defaultRenderInfo;

/**
 * This defines a shade context which allows you to query a map outside of a renderer.  To
 * use it, set the uvw, duvw, and shadeTime parameters, then pass it in as the parameter to
 * a map evaluation
 */
class map_query_shadecontext : public ShadeContext {
  public:
    map_query_shadecontext()
        : toObjectSpaceTM( 1 )
        , toWorldSpaceTM( 1 ) {
        position = Point3( 0, 0, 0 );
        dposition = Point3( 0, 0, 0 );
        uvw = Point3( 0, 0, 0 );
        duvw = Point3( 0, 0, 0 );
        shadeTime = 0;
        evalObject = 0;
        inode = 0;
        globContext = NULL;

        camPos = Point3( 0, 0, 0 );
        origView = view = Point3( 0, 0, -1 );
        origNormal = normal = Point3( 0, 0, 1 );

        // Set values in the base class
        mode = SCMODE_NORMAL;
        mtlNum = 0;
        doMaps = TRUE;
        filterMaps = TRUE;
        shadow = TRUE;
        backFace = FALSE;
        ambientLight = Color( 1, 1, 1 );
        nLights = 0;
        rayLevel = 0;
        ior = 1.f;
        xshadeID = 0;

        ResetOutput();
    }

    // These are the members to set
    Point3 position, dposition;
    Point3 uvw, duvw;
    TimeValue shadeTime;

    // These matrices default to the identity
    Matrix3 toObjectSpaceTM; // from camera-space to object-space
    Matrix3 toWorldSpaceTM;  // from camera-space to world-space

    // For querying a particle system's material, you can set the result of GetEvalObject here.
    Object* evalObject;
    INode* inode;

    // Index of refraction
    float ior;

    // For camera related calculations (camera->point view vector)
    Point3 camPos;
    Point3 view, origView;
    Point3 normal, origNormal;

    Class_ID ClassID() {
        return Class_ID( 0x15d154ab, 0x7de246d ); // Generated for kicks.
    }
    BOOL InMtlEditor() {
        if( globContext )
            return globContext->inMtlEdit;
        return FALSE;
    }
    LightDesc* Light( int n ) {
        int nodeId;
        if( globContext && ( nodeId = NodeID() ) >= 0 )
            return globContext->GetRenderInstance( nodeId )->Light( n );
        return NULL;
    }
    Object* GetEvalObject() { return evalObject; }
    INode* Node() { return inode; }
    int NodeID() {
        // return inode ? (int)inode->GetRenderID() : -1;
        if( inode )
            return inode->GetRenderID();
        return -1;
    }
    int ProjType() {
        // returns: 0: perspective, 1: parallel
        if( globContext )
            return globContext->projType;
        return PROJ_PERSPECTIVE;
    }
    int FaceNumber() { return 0; }
    TimeValue CurTime() { return shadeTime; }
    Point3 Normal() { return normal; }
    Point3 OrigNormal() { return origNormal; }
    Point3 GNormal() { return origNormal; }
    void SetNormal( Point3 p ) { normal = p; }
    Point3 ReflectVector() { return view - 2 * DotProd( view, normal ) * normal; }
    Point3 RefractVector( float ior ) {
        // Taken from Max SDK cjrender sample.

        float VN, nur, k;
        VN = DotProd( -view, normal );
        if( backFace )
            nur = ior;
        else
            nur = ( ior != 0.0f ) ? 1.0f / ior : 1.0f;
        k = 1.0f - nur * nur * ( 1.0f - VN * VN );
        if( k <= 0.0f ) {
            // Total internal reflection:
            return ReflectVector();
        } else {
            return ( nur * VN - (float)sqrt( k ) ) * normal + nur * view;
        }
    }
    void SetIOR( float _ior ) { ior = _ior; }
    float GetIOR() { return ior; }
    Point3 CamPos() { return camPos; }
    Point3 V() { return view; }
    Point3 OrigView() { return origView; }
    void SetView( Point3 v ) { view = v; }
    Point3 P() { return position; }
    Point3 DP() { return dposition; }
    Point3 PObj() { return toObjectSpaceTM.PointTransform( position ); }
    Point3 DPObj() { return toObjectSpaceTM.VectorTransform( dposition ); }
    Box3 ObjectBox() {
        int nodeId;
        if( globContext && ( nodeId = NodeID() ) >= 0 )
            return globContext->GetRenderInstance( nodeId )->obBox;
        return Box3( Point3( 0, 0, 0 ), Point3( 0, 0, 0 ) );
    }
    Point3 PObjRelBox() {
        // not dealing with this one
        return Point3( 0, 0, 0 );
    }
    Point3 DPObjRelBox() {
        // not dealing with this one
        return Point3( 0, 0, 0 );
    }
    Point3 UVW( int /*chan*/ ) { return uvw; }
    Point3 DUVW( int /*chan*/ ) { return duvw; }
    void DPdUVW( Point3 dP[3], int /*chan*/ ) { dP[0] = dP[1] = dP[2] = Point3( 0, 0, 0 ); }
    AColor EvalEnvironMap( Texmap* map, Point3 viewd ) {
        return ShadeContext::EvalEnvironMap( map, viewd );
        /*Point3 oldView = view;

        view = viewd;
        AColor result = map->EvalColor( *this );
        view = oldView;

        return result;*/
    }

    void ScreenUV( Point2& uv, Point2& duv ) {
        if( globContext ) {
            Point2 p2 = globContext->MapToScreen( P() );
            uv.x = p2.x / (float)globContext->devWidth;
            uv.y = p2.y / (float)globContext->devHeight;
            duv.x = duvw.x;
            duv.y = duvw.y;
        } else {
            uv.x = uvw.x;
            uv.y = uvw.y;
            duv.x = duvw.x;
            duv.y = duvw.y;
        }
    }
    IPoint2 ScreenCoord() {
        // return IPoint2(0,0);
        if( globContext ) {
            Point2 p2 = globContext->MapToScreen( P() );
            return IPoint2( (int)floorf( p2.x + 0.5f ), (int)floorf( p2.y + 0.5f ) );
        }
        return IPoint2( 0, 0 );
    }

    Point3 PointTo( const Point3& p, RefFrame ito ) {
        switch( ito ) {
        case REF_CAMERA:
            return p;
        case REF_OBJECT:
            return toObjectSpaceTM.PointTransform( p );
        case REF_WORLD:
            return toWorldSpaceTM.PointTransform( p );
        default:
            throw std::runtime_error( "map_query_shadecontext::PointTo() - Unknown RefFrame" );
        }
    }
    Point3 PointFrom( const Point3& p, RefFrame ifrom ) {
        int nodeID;

        switch( ifrom ) {
        case REF_OBJECT:
            return ( globContext && ( nodeID = NodeID() ) >= 0 )
                       ? globContext->GetRenderInstance( nodeID )->objToCam.PointTransform( p )
                       : p;
        case REF_WORLD:
            return ( globContext ) ? globContext->worldToCam.PointTransform( p ) : p;
        case REF_CAMERA:
        default:
            return p;
        }
    }
    Point3 VectorTo( const Point3& p, RefFrame ito ) {
        switch( ito ) {
        case REF_CAMERA:
            return p;
        case REF_OBJECT:
            return toObjectSpaceTM.VectorTransform( p );
        case REF_WORLD:
            return toWorldSpaceTM.VectorTransform( p );
        default:
            throw std::runtime_error( "map_query_shadecontext::VectorTo() - Unknown RefFrame" );
        }
    }
    Point3 VectorFrom( const Point3& p, RefFrame ifrom ) {
        int nodeID;

        switch( ifrom ) {
        case REF_OBJECT:
            return ( globContext && ( nodeID = NodeID() ) >= 0 )
                       ? globContext->GetRenderInstance( nodeID )->objToCam.VectorTransform( p )
                       : p;
        case REF_WORLD:
            return ( globContext ) ? globContext->worldToCam.VectorTransform( p ) : p;
        case REF_CAMERA:
        default:
            return p;
        }
    }
    void GetBGColor( Color& bgcol, Color& transp, BOOL /*fogBG*/ ) {
        bgcol.Black();
        transp.White();
    }
};

/**
 * This is the same as map_query_shadecontext, but adds the ability to independently specify
 * all the values in the map channels, instead of a single UVW value.
 */
class multimapping_shadecontext : public map_query_shadecontext {
  public:
    Point3 uvwArray[100];
    Point3 duvwArray[100];
    multimapping_shadecontext() {
        for( int i = 0; i < 100; ++i ) {
            uvwArray[i] = Point3( 0, 0, 0 );
            duvwArray[i] = Point3( 0, 0, 0 );
        }
    }

    Point3 UVW( int chan ) { return uvwArray[chan]; }

    Point3 DUVW( int chan ) { return duvwArray[chan]; }
};

namespace detail {

inline void call_load_map_files( Texmap* map, TimeValue t ) {
    if( map != 0 ) {
        for( int i = 0; i < map->NumSubTexmaps(); ++i )
            call_load_map_files( map->GetSubTexmap( i ), t );
        map->LoadMapFiles( t );
    }
}

inline void call_load_map_files( Mtl* mtl, TimeValue t ) {
    if( mtl ) {
        std::stack<Mtl*> toEval;
        toEval.push( mtl );
        while( !toEval.empty() ) {
            Mtl* currMtl = toEval.top();
            toEval.pop();
            currMtl->Update( t, FOREVER );

            for( int i = 0; i < currMtl->NumSubMtls(); ++i ) {
                Mtl* subMtl = currMtl->GetSubMtl( i );
                if( subMtl ) {
                    toEval.push( subMtl );
                }
            }

            for( int i = 0; i < currMtl->NumSubTexmaps(); ++i ) {
                Texmap* subTexmap = currMtl->GetSubTexmap( i );
                if( subTexmap ) {
                    call_load_map_files( subTexmap, t );
                }
            }
        }
    }
}

inline void collect_map_requirements_recursive( Texmap* map, BitArray& outReqs, BitArray& outGarbage ) {
    if( !map )
        return;

    map->LocalMappingsRequired( -1, outReqs, outGarbage );
    if( map->MapSlotType( 0 ) == MAPSLOT_TEXTURE ) {
        UVGen* theUVGen = map->GetTheUVGen();
        if( theUVGen ) {
            int uvwSource = theUVGen->GetUVWSource();
            if( uvwSource == UVWSRC_EXPLICIT )
                outReqs.Set( map->GetMapChannel() );
            else if( uvwSource == UVWSRC_EXPLICIT2 )
                outReqs.Set( 0 );
        }

        XYZGen* theXYZGen = map->GetTheXYZGen();
        if( theXYZGen && theXYZGen->IsStdXYZGen() ) {
            StdXYZGen* stdXYZGen = static_cast<StdXYZGen*>( theXYZGen );
            int xyzSource = stdXYZGen->GetCoordSystem();
            if( xyzSource == UVW_COORDS )
                outReqs.Set( stdXYZGen->GetMapChannel() );
            else if( xyzSource == UVW2_COORDS )
                outReqs.Set( 0 );
        }
    }

    for( int i = 0, iEnd = map->NumSubTexmaps(); i < iEnd; ++i ) {
        map->LocalMappingsRequired( i, outReqs, outGarbage );
        collect_map_requirements_recursive( map->GetSubTexmap( i ), outReqs, outGarbage );
    }
}

inline void collect_mtl_requirements_recursive( Mtl* pMtl, BitArray& outReqs, BitArray& outGarbage ) {
    if( !pMtl )
        return;

    pMtl->LocalMappingsRequired( -1, outReqs, outGarbage );
    for( int i = 0, iEnd = pMtl->NumSubTexmaps(); i < iEnd; ++i ) {
        pMtl->LocalMappingsRequired( i, outReqs, outGarbage );
        collect_map_requirements_recursive( pMtl->GetSubTexmap( i ), outReqs, outGarbage );
    }
    for( int i = 0, iEnd = pMtl->NumSubMtls(); i < iEnd; ++i )
        collect_mtl_requirements_recursive( pMtl->GetSubMtl( i ), outReqs, outGarbage );
}

} // namespace detail

/**
 * This function updates a map so that it's ready to have its EvalColor method called.
 *
 * @param  map  The texture map to update.
 * @param  t  The time for which to update it.
 * @param  ivalid  If not NULL, this interval gets intersected with the validity interval from the Update call.
 */
inline void update_map_for_shading( Texmap* map, TimeValue t, Interval* ivalid = 0 ) {
    Interval ivalidLocal = FOREVER;
    // Note that calling Update before calling LoadMapFiles is important.
    // Things will not work correctly if Update is called after LoadMapFiles!
    map->Update( t, ivalidLocal );
    detail::call_load_map_files( map, t );
    // Update the ivalid interval if requested.
    if( ivalid )
        ( *ivalid ) &= ivalidLocal;
}

/**
 * This function updates a material so that it's ready to have its Shade method called.
 *
 * @param  map  The texture map to update.
 * @param  t  The time for which to update it.
 * @param  ivalid  If not NULL, this interval gets intersected with the validity interval from the Update call.
 */
inline void update_material_for_shading( Mtl* mtl, TimeValue t, Interval* ivalid = 0 ) {
    Interval ivalidLocal = FOREVER;
    // Note that calling Update before calling LoadMapFiles is important.
    // Things will not work correctly if Update is called after LoadMapFiles!
    mtl->Update( t, ivalidLocal );
    detail::call_load_map_files( mtl, t );
    // Update the ivalid interval if requested.
    if( ivalid )
        ( *ivalid ) &= ivalidLocal;
}

/**
 * This function collects the required UVW channels of a map
 *
 * @param map The texture map to collect requirements for
 * @param outRequirements A bitarray of 100 bits that will be set if that channel is required
 */
inline void collect_map_requirements( Texmap* map, BitArray& outRequirements ) {
    BitArray garbage( MAX_MESHMAPS );
    outRequirements.SetSize( MAX_MESHMAPS );

    detail::collect_map_requirements_recursive( map, outRequirements, garbage );
}

/**
 * This function collects the required UVW channels of a material tree
 *
 * @param pMtl The material to collect requirements for
 * @param outRequirements A bitarray of 100 bits that will be set if that channel is required
 */
inline void collect_material_requirements( Mtl* pMtl, BitArray& outRequirements ) {
    BitArray garbage( MAX_MESHMAPS );
    outRequirements.SetSize( MAX_MESHMAPS );

    detail::collect_mtl_requirements_recursive( pMtl, outRequirements, garbage );
}

} // namespace shaders
} // namespace max3d
} // namespace frantic
