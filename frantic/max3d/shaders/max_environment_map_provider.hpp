// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/graphics/color3f.hpp>
#include <frantic/rendering/environment_map_provider.hpp>

#include <boost/thread/tss.hpp>
#include <frantic/max3d/shaders/map_query.hpp>

#include <imtl.h>

namespace frantic {
namespace max3d {
namespace shaders {

class env_query_shadecontext : public map_query_shadecontext {
  public:
    // AColor EvalEnvironMap(Texmap * map, Point3 viewd) { return ShadeContext::EvalEnvironMap(map, viewd); }
};

class bkgrd_shadecontext : public env_query_shadecontext {
  public:
    int screenX, screenY;
    float screenUVX, screenUVY;
    float screenDUVX, screenDUVY;

  public:
    virtual void ScreenUV( Point2& uv, Point2& duv ) {
        uv.x = screenUVX;
        uv.y = screenUVY;
        duv.x = screenDUVX;
        duv.y = screenDUVY;
    }

    virtual IPoint2 ScreenCoord() { return IPoint2( screenX, screenY ); }
};

class max_environment_map_provider : public frantic::rendering::environment_map_provider<frantic::graphics::color3f> {
    Texmap* m_map;
    TimeValue m_time;

    const RenderGlobalContext* m_globContext;

    // We need a mutable shade context in order to use it as an argument holder
    // in const functions.
    //
    // We need a thread_specific_ptr, so that the shade context will get created
    // for each thread when this is called in a multi-threaded context.
    mutable boost::thread_specific_ptr<env_query_shadecontext> m_pShadeContext;

  public:
    max_environment_map_provider( Texmap* map, TimeValue t ) {
        update_map_for_shading( map, t );
        map->LoadMapFiles( t );

        m_map = map;
        m_time = t;
    }

    void set_context( const RenderGlobalContext* globContext ) { m_globContext = globContext; }

    virtual ~max_environment_map_provider() {}

    // Simple environment lookup with no filter width.
    virtual frantic::graphics::color3f lookup_environment( const frantic::graphics::vector3f& direction ) const {
        if( !m_pShadeContext.get() ) {
            m_pShadeContext.reset( new env_query_shadecontext );
            m_pShadeContext->shadeTime = m_time;

            if( m_globContext ) {
                m_pShadeContext->toObjectSpaceTM = m_globContext->camToWorld;
                m_pShadeContext->toWorldSpaceTM = m_globContext->camToWorld;
            }
        }

        Point3 envDir = frantic::max3d::to_max_t( direction );
        if( m_globContext )
            envDir = m_globContext->worldToCam.VectorTransform( envDir );
        envDir = Normalize( envDir );

        m_pShadeContext->view = envDir;
        m_pShadeContext->origView = envDir;

        AColor result = m_map->EvalColor( *m_pShadeContext );

        if( result.a == 0 )
            return frantic::graphics::color3f( 0 );
        return frantic::graphics::color3f( result.r / result.a, result.g / result.a, result.b / result.a );
    }

    virtual void render_background( const frantic::graphics::camera<float>& cam,
                                    frantic::graphics2d::framebuffer<pixel_type>& outImage, float mblurTime = 0.5f ) {
        bkgrd_shadecontext ctx;
        ctx.shadeTime = m_time;
        ctx.screenDUVX = 1.f / (float)outImage.width();
        ctx.screenDUVY = 1.f / (float)outImage.height();

        frantic::graphics::transform4f toWorld = cam.world_transform( mblurTime );

        ctx.toWorldSpaceTM = frantic::max3d::to_max_t( toWorld );
        ctx.toObjectSpaceTM = ctx.toWorldSpaceTM;

        // TODO: This doesn't do any sort of antialiasing. It probably should. Maybe. Ya know. Also this should
        // definitely be done in parallel.
        for( int y = 0, yEnd = outImage.height(); y < yEnd; ++y ) {
            for( int x = 0, xEnd = outImage.width(); x < xEnd; ++x ) {
                bool isValid = true;
                frantic::graphics2d::vector2f p( (float)x + 0.5f, (float)y + 0.5f );
                frantic::graphics::vector3f dir = cam.to_cameraspace_direction( p, isValid );

                ctx.screenX = x;
                ctx.screenY = y;
                ctx.screenUVX = p.x / (float)outImage.width();
                ctx.screenUVY = p.y / (float)outImage.height();

                ctx.view = frantic::max3d::to_max_t( frantic::graphics::vector3f::normalize( dir ) );
                ctx.origView = ctx.view;

                AColor cRaw = m_map->EvalColor( ctx );

                frantic::graphics::color3f c( cRaw.r, cRaw.g, cRaw.b );
                frantic::graphics::alpha3f a( /*cRaw.a*/ 0.f );

                outImage.blend_over( x, y, pixel_type( c, a ) );
            }
        }
    }
};

} // namespace shaders
} // namespace max3d
} // namespace frantic
