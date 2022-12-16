// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#pragma warning( push, 3 )
#include <tbb/parallel_for.h>
#pragma warning( pop )

#include <frantic/particles/particle_array_iterator.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

#include <frantic/max3d/particles/streams/material_affected_particle_istream.hpp>
#include <frantic/max3d/rendering/renderplugin_utils.hpp>

namespace frantic {
namespace max3d {
namespace particles {
namespace streams {

using frantic::graphics::color3f;
using frantic::graphics::transform4f;
using frantic::graphics::vector3f;

class material_colored_particle_istream_data {
    Mtl* m_pMtl;
    TimeValue m_time;
    BitArray m_requiredMaps;

    Point3 m_cameraPosition;
    Matrix3 m_worldToObjectTM;

    bool m_shadeColor;
    bool m_shadeDensity;

    frantic::channels::channel_accessor<vector3f> m_posAccessor;
    frantic::channels::channel_cvt_accessor<vector3f> m_normalAccessor;
    frantic::channels::channel_cvt_accessor<vector3f> m_colorAccessor;
    frantic::channels::channel_cvt_accessor<vector3f> m_texCoordAccessor;
    frantic::channels::channel_cvt_accessor<float> m_densityAccessor;
    frantic::channels::channel_cvt_accessor<int> m_mtlIndexAccessor;
    std::vector<std::pair<int, frantic::channels::channel_cvt_accessor<vector3f>>> m_channelAccessors;

  public:
    material_colored_particle_istream_data( Mtl* pMtl, const Matrix3& worldToObjTM, const Point3& camPos, TimeValue t,
                                            bool shadeColor, bool shadeDensity )
        : m_pMtl( pMtl )
        , m_worldToObjectTM( worldToObjTM )
        , m_cameraPosition( camPos )
        , m_time( t )
        , m_shadeColor( shadeColor )
        , m_shadeDensity( shadeDensity ) {
        frantic::max3d::shaders::update_material_for_shading( pMtl, t );

        BitArray bumpRequiredMaps;
        pMtl->MappingsRequired( 0, m_requiredMaps, bumpRequiredMaps );

        if( frantic::logging::is_logging_debug() ) {
            for( int i = 0; i < MAX_MESHMAPS; ++i ) {
                if( m_requiredMaps[i] != FALSE )
                    frantic::logging::debug << "Map \"" << pMtl->GetName() << "\" requires map channel " << i
                                            << std::endl;
            }
        }

        // If there are no channels set, force the TextureCoord channel so that most materials will work.
        if( !m_requiredMaps.AnyBitSet() )
            m_requiredMaps.Set( 1 );
    }

    bool requires_map_channel( int i ) const { return ( m_requiredMaps[i] != FALSE ); }

    void init_shade_context( shaders::multimapping_shadecontext& shadeContext ) const {
        shadeContext.shadeTime = m_time;
        shadeContext.toWorldSpaceTM.IdentityMatrix();
        shadeContext.toObjectSpaceTM = m_worldToObjectTM;
        shadeContext.camPos = m_cameraPosition;
    }

    void set_channel_map( const frantic::channels::channel_map& pcm ) {
        m_posAccessor = pcm.get_accessor<vector3f>( _T("Position") );
        m_colorAccessor = pcm.get_cvt_accessor<vector3f>( _T("Color") );

        if( pcm.has_channel( _T("TextureCoord") ) )
            m_texCoordAccessor = pcm.get_cvt_accessor<vector3f>( _T("TextureCoord") );
        else
            m_texCoordAccessor.reset( vector3f( 0 ) );

        if( pcm.has_channel( _T("Normal") ) )
            m_normalAccessor = pcm.get_cvt_accessor<vector3f>( _T("Normal") );
        else
            m_normalAccessor.reset( vector3f::from_zaxis() );

        m_channelAccessors.clear();
        for( int i = 2; i < MAX_MESHMAPS; ++i ) {
            if( m_requiredMaps[i] ) {
                frantic::tstring channelName = _T("Mapping") + boost::lexical_cast<frantic::tstring>( i );
                if( pcm.has_channel( channelName ) )
                    m_channelAccessors.push_back( std::make_pair( i, pcm.get_cvt_accessor<vector3f>( channelName ) ) );
            }
        }

        if( pcm.has_channel( _T("MtlIndex") ) )
            m_mtlIndexAccessor = pcm.get_cvt_accessor<int>( _T("MtlIndex") );
        else
            m_mtlIndexAccessor.reset( 0 );

        if( pcm.has_channel( _T("Density") ) )
            m_densityAccessor = pcm.get_cvt_accessor<float>( _T("Density") );
        else
            m_densityAccessor.reset( 1.f );
    }

    void shade_particle( char* buffer, shaders::multimapping_shadecontext& shadeContext ) const {
        shadeContext.ResetOutput();
        shadeContext.position = to_max_t( m_posAccessor.get( buffer ) );
        shadeContext.view = ( shadeContext.position - shadeContext.camPos ).Normalize();
        shadeContext.normal = to_max_t( vector3f::normalize( m_normalAccessor.get( buffer ) ) );
        shadeContext.mtlNum = m_mtlIndexAccessor.get( buffer );
        shadeContext.uvwArray[0] = to_max_t( m_colorAccessor.get( buffer ) );
        shadeContext.uvwArray[1] = to_max_t( m_texCoordAccessor.get( buffer ) );

        for( std::size_t i = 0; i < m_channelAccessors.size(); ++i )
            shadeContext.uvwArray[m_channelAccessors[i].first] = to_max_t( m_channelAccessors[i].second.get( buffer ) );

        const_cast<Mtl*>( m_pMtl )->Shade( shadeContext );

        if( m_shadeColor ) {
            m_colorAccessor.set(
                buffer,
                vector3f( shadeContext.out.t.r != 1.f ? shadeContext.out.c.r / ( 1.f - shadeContext.out.t.r ) : 0,
                          shadeContext.out.t.g != 1.f ? shadeContext.out.c.g / ( 1.f - shadeContext.out.t.g ) : 0,
                          shadeContext.out.t.b != 1.f ? shadeContext.out.c.b / ( 1.f - shadeContext.out.t.b ) : 0 ) );
        }

        if( m_shadeDensity ) {
            float curAlpha = ( 3.f - shadeContext.out.t.r - shadeContext.out.t.g - shadeContext.out.t.b ) / 3.f;
            m_densityAccessor.set( buffer, curAlpha * m_densityAccessor.get( buffer ) );
        }
    }
};

class parallel_shade_particle {
    const material_colored_particle_istream_data* m_pData;
    mutable shaders::multimapping_shadecontext m_shadeContext;

  public:
    typedef frantic::particles::particle_array_iterator range_iterator;
    typedef tbb::blocked_range<range_iterator> range_type;

  public:
    parallel_shade_particle( const material_colored_particle_istream_data& data )
        : m_pData( &data ) {
        m_pData->init_shade_context( m_shadeContext );
    }

    void operator()( const range_type& range ) const {
        for( range_iterator it = range.begin(), itEnd = range.end(); it != itEnd; ++it )
            m_pData->shade_particle( *it, m_shadeContext );
    }
};

material_colored_particle_istream::material_colored_particle_istream(
    boost::shared_ptr<frantic::particles::streams::particle_istream> pDelegate, Mtl* pMtl, TimeValue t,
    frantic::graphics::transform4f worldToObjectTM, bool doShading, bool doDensityShading,
    frantic::max3d::shaders::renderInformation renderInfo )
    : delegated_particle_istream( pDelegate ) {
    m_pData = new material_colored_particle_istream_data( pMtl, frantic::max3d::to_max_t( worldToObjectTM ),
                                                          renderInfo.m_cameraPosition, t, doShading, doDensityShading );
    m_pData->init_shade_context( m_shadeContext );

    m_nativePcm = m_delegate->get_native_channel_map();
    if( !m_nativePcm.has_channel( _T("Color") ) )
        m_nativePcm.append_channel<vector3f>( _T("Color") );

    set_channel_map( m_delegate->get_channel_map() );
}

material_colored_particle_istream::~material_colored_particle_istream() {
    delete m_pData;
    m_pData = NULL;
}

void material_colored_particle_istream::set_channel_map( const frantic::channels::channel_map& pcm ) {
    const frantic::channels::channel_map& delegateNativePcm = m_delegate->get_native_channel_map();

    frantic::channels::channel_map newPcm = m_outPcm = pcm;

    if( !pcm.has_channel( _T("Color") ) )
        newPcm.append_channel<vector3f>( _T("Color") );
    if( !pcm.has_channel( _T("Normal") ) && delegateNativePcm.has_channel( _T("Normal") ) )
        newPcm.append_channel<vector3f>( _T("Normal") );
    if( !pcm.has_channel( _T("MtlIndex") ) && delegateNativePcm.has_channel( _T("MtlIndex") ) )
        newPcm.append_channel<int>( _T("MtlIndex") );
    if( !pcm.has_channel( _T("TextureCoord") ) && m_pData->requires_map_channel( 1 ) &&
        delegateNativePcm.has_channel( _T("TextureCoord") ) )
        newPcm.append_channel<vector3f>( _T("TextureCoord") );

    for( int i = 2; i < MAX_MESHMAPS; ++i ) {
        if( m_pData->requires_map_channel( i ) ) {
            frantic::tstring channelName = _T("Mapping") + boost::lexical_cast<frantic::tstring>( i );
            if( !pcm.has_channel( channelName ) && delegateNativePcm.has_channel( channelName ) )
                newPcm.append_channel<vector3f>( channelName );
        }
    }

    m_adaptor.set( m_outPcm, newPcm );

    m_pData->set_channel_map( newPcm );
    m_delegate->set_channel_map( newPcm );
}

void material_colored_particle_istream::set_default_particle( char* buffer ) {
    frantic::channels::channel_map_adaptor tempAdaptor( m_delegate->get_channel_map(), m_outPcm );

    if( tempAdaptor.is_identity() )
        m_delegate->set_default_particle( buffer );
    else {
        char* tempBuffer = (char*)_alloca( tempAdaptor.dest_size() );
        memset( tempBuffer, 0, tempAdaptor.dest_size() );

        tempAdaptor.copy_structure( tempBuffer, buffer );
        m_delegate->set_default_particle( tempBuffer );
    }
}

bool material_colored_particle_istream::get_particle( char* outBuffer ) {
    if( m_adaptor.is_identity() ) {
        if( !m_delegate->get_particle( outBuffer ) )
            return false;
        m_pData->shade_particle( outBuffer, m_shadeContext );
    } else {
        char* tempBuffer = (char*)_alloca( m_adaptor.source_size() );
        if( !m_delegate->get_particle( tempBuffer ) )
            return false;
        m_pData->shade_particle( tempBuffer, m_shadeContext );
        m_adaptor.copy_structure( outBuffer, tempBuffer );
    }

    return true;
}

bool material_colored_particle_istream::get_particles( char* outBuffer, std::size_t& numParticles ) {
    bool result;

    if( m_adaptor.is_identity() ) {
        result = m_delegate->get_particles( outBuffer, numParticles );
        parallel_shade_particle::range_iterator itBegin( outBuffer, m_adaptor.dest_size() );
        parallel_shade_particle::range_iterator itEnd = itBegin + numParticles;

#ifndef FRANTIC_DISABLE_THREADS
        tbb::parallel_for( parallel_shade_particle::range_type( itBegin, itEnd ), parallel_shade_particle( *m_pData ) );
#else
#pragma message( "Threads are disabled" )
        parallel_shade_particle func( *m_pData );
        func( parallel_shade_particle::range_type( itBegin, itEnd ) );
#endif
    } else {
        boost::scoped_array<char> tempBuffer( new char[m_adaptor.source_size() * numParticles] );
        result = m_delegate->get_particles( tempBuffer.get(), numParticles );
        parallel_shade_particle::range_iterator itBegin( tempBuffer.get(), m_adaptor.source_size() );
        parallel_shade_particle::range_iterator itEnd = itBegin + numParticles;

#ifndef FRANTIC_DISABLE_THREADS
        tbb::parallel_for( parallel_shade_particle::range_type( itBegin, itEnd, 2000 ),
                           parallel_shade_particle( *m_pData ) );
#else
#pragma message( "Threads are disabled" )
        parallel_shade_particle func( *m_pData );
        func( parallel_shade_particle::range_type( itBegin, itEnd, 2000 ) );
#endif

        for( std::size_t i = 0; i < numParticles; ++i )
            m_adaptor.copy_structure( outBuffer + i * m_adaptor.dest_size(),
                                      tempBuffer.get() + i * m_adaptor.source_size() );
    }

    return result;
}

} // namespace streams
} // namespace particles
} // namespace max3d
}; // namespace frantic
