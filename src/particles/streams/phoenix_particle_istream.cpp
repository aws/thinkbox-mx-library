// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#if defined( PHOENIXFD_SD_AVAILABLE )

#include <frantic/particles/streams/concatenated_particle_istream.hpp>

#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/particles/streams/phoenix_particle_istream.hpp>

#include "aurinterface.h"
#include "phxcontent.h"

#pragma warning( push, 3 )
#include <inode.h>
#include <object.h>
#pragma warning( pop )

namespace frantic {
namespace max3d {
namespace particles {
namespace streams {

#pragma warning( push )
#pragma warning( disable : 4706 )
boost::shared_ptr<frantic::particles::streams::particle_istream>
GetPhoenixParticleIstream( INode* pNode, TimeValue t, const frantic::channels::channel_map& pcm ) {
    ObjectState os = pNode->EvalWorldState( t );

    if( IPhoenixFD* phoenix = (IPhoenixFD*)( os.obj ? os.obj->GetInterface( PHOENIXFD_INTERFACE ) : NULL ) ) {
        std::vector<boost::shared_ptr<frantic::particles::streams::particle_istream>> pins;

        ////////////////////////////////////////////////////////////////////////////////////////////////////////
        // WORKAROUND: There seems to be a bug in the PhoenixFD plugin where calling 'EvalWorldState' will not
        // actually cause it to re-evaluate the node's internal cache of particles.  Hence, when saving a sequence
        // of frames to a file sequence, it would just write the current viewport frame over and over, since that's
        // the only information it would have cached.  Calling 'InvalidateRect' seems to force it to re-cache its
        // information for the specified time, and hence seems to be the simplest workaround.
        // TODO: I'm sending a bug report to Chaos Group; if this ever gets resolved, one could consider removing
        // this, although it seems otherwise benign.
        ////////////////////////////////////////////////////////////////////////////////////////////////////////
#if MAX_VERSION_MAJOR > 13
        pNode->InvalidateRect( t ); // This call doesn't exist in Max 2010. We may need to find an alternative.
#endif

        IAur* aur = phoenix->GetSimData( pNode );

        for( int i = 0, iEnd = aur ? aur->NumGroups() : 0; i < iEnd; ++i ) {
            IPhoenixFDPrtGroup* particles = aur->GetParticleGroup( i );
            if( particles ) {
                float objToGridTM[12];
                aur->GetObject2GridTransform( objToGridTM );

                frantic::graphics::transform4f toWorldTM =
                    frantic::graphics::transform4f(
                        frantic::graphics::vector3f( objToGridTM[0], objToGridTM[1], objToGridTM[2] ),
                        frantic::graphics::vector3f( objToGridTM[3], objToGridTM[4], objToGridTM[5] ),
                        frantic::graphics::vector3f( objToGridTM[6], objToGridTM[7], objToGridTM[8] ),
                        frantic::graphics::vector3f( objToGridTM[9], objToGridTM[10], objToGridTM[11] ) )
                        .to_inverse();

                toWorldTM = frantic::max3d::from_max_t( pNode->GetNodeTM( t ) ) * toWorldTM;

                pins.push_back( boost::shared_ptr<frantic::particles::streams::particle_istream>(
                    new phoenix_particle_istream( particles, toWorldTM, pcm ) ) );
            }
        }

        if( !pins.empty() ) {
            if( pins.size() == 1 )
                return pins[0];
            else
                return boost::shared_ptr<frantic::particles::streams::particle_istream>(
                    new frantic::particles::streams::concatenated_particle_istream( pins ) );
        }
    }

    // This can happen sometimes when there are 0 particles for the current time.
    return boost::shared_ptr<frantic::particles::streams::particle_istream>();
}

std::pair<bool, int> IsPhoenixObject( INode* pNode, TimeValue t ) {
    ObjectState os = pNode->EvalWorldState( t );

    if( os.obj ) {
        if( IPhoenixFD* phoenix = (IPhoenixFD*)os.obj->GetInterface( PHOENIXFD_INTERFACE ) ) {
            if( IAur* aur = phoenix->GetSimData( pNode ) )
                return std::make_pair( true, aur->NumGroups() );
        }
    }

    return std::make_pair( false, 0 );
}
#pragma warning( pop )

inline void phoenix_particle_istream::init_accessors() {
    m_accessors.pos = m_outMap.get_accessor<frantic::graphics::vector3f>( _T("Position") );

    m_accessors.vel.reset();
    m_accessors.texturecoord.reset();
    m_accessors.orientation.reset();
    m_accessors.size.reset();
    m_accessors.age.reset();
    m_accessors.temp.reset();
    m_accessors.density.reset();
    m_accessors.fuel.reset();

    if( m_outMap.has_channel( _T("Velocity") ) && m_particles->IsChannelSupported( PHXPRT_VEL ) )
        m_accessors.vel = m_outMap.get_cvt_accessor<frantic::graphics::vector3f>( _T("Velocity") );

    if( m_outMap.has_channel( _T("TextureCoord") ) && m_particles->IsChannelSupported( PHXPRT_OR ) )
        m_accessors.texturecoord = m_outMap.get_cvt_accessor<frantic::graphics::vector3f>( _T("TextureCoord") );

    if( m_outMap.has_channel( _T("Orientation") ) && m_particles->IsChannelSupported( PHXPRT_OR ) )
        m_accessors.orientation = m_outMap.get_cvt_accessor<frantic::graphics::quat4f>( _T("Orientation") );

    if( m_outMap.has_channel( _T("Size") ) && m_particles->IsChannelSupported( PHXPRT_SIZE ) )
        m_accessors.size = m_outMap.get_cvt_accessor<float>( _T("Size") );

    if( m_outMap.has_channel( _T("Age") ) && m_particles->IsChannelSupported( PHXPRT_AGE ) )
        m_accessors.age = m_outMap.get_cvt_accessor<float>( _T("Age") );

    if( m_outMap.has_channel( _T("Temperature") ) && m_particles->IsChannelSupported( PHXPRT_T ) )
        m_accessors.temp = m_outMap.get_cvt_accessor<float>( _T("Temperature") );

    if( m_outMap.has_channel( _T("Density") ) && m_particles->IsChannelSupported( PHXPRT_SM ) )
        m_accessors.density = m_outMap.get_cvt_accessor<float>( _T("Density") );

    if( m_outMap.has_channel( _T("Fuel") ) && m_particles->IsChannelSupported( PHXPRT_FL ) )
        m_accessors.fuel = m_outMap.get_cvt_accessor<float>( _T("Fuel") );
}

inline phoenix_particle_istream::phoenix_particle_istream( IPhoenixFDPrtGroup* particles,
                                                           const frantic::graphics::transform4f& toWorldTM,
                                                           const frantic::channels::channel_map& pcm )
    : m_outMap( pcm ) {
    m_particles = particles;
    m_particleCount = particles->NumParticles();
    m_particleIndex = -1;

    if( m_particleCount > 0 ) {
        if( !m_particles->IsChannelSupported( PHXPRT_POS ) )
            throw std::runtime_error(
                "phoenix_particle_istream() Cannot use particles without a position from source: \"" +
                frantic::strings::to_string( particles->GetName() ) + "\"" );

        m_nativeMap.define_channel<frantic::graphics::vector3f>( _T( "Position" ) );
        if( m_particles->IsChannelSupported( PHXPRT_VEL ) )
            m_nativeMap.define_channel<frantic::graphics::vector3f>( _T( "Velocity" ) );
        if( m_particles->IsChannelSupported( PHXPRT_UVW ) )
            m_nativeMap.define_channel<frantic::graphics::vector3f>( _T( "TextureCoord" ) );
        if( m_particles->IsChannelSupported( PHXPRT_OR ) )
            m_nativeMap.define_channel<frantic::graphics::quat4f>( _T( "Orientation" ) );
        if( m_particles->IsChannelSupported( PHXPRT_SIZE ) )
            m_nativeMap.define_channel<float>( _T( "Size" ) );
        if( m_particles->IsChannelSupported( PHXPRT_AGE ) )
            m_nativeMap.define_channel<float>( _T( "Age" ) );
        if( m_particles->IsChannelSupported( PHXPRT_SM ) )
            m_nativeMap.define_channel<float>( _T( "Density" ) );
        if( m_particles->IsChannelSupported( PHXPRT_T ) )
            m_nativeMap.define_channel<float>( _T( "Temperature" ) );
        if( m_particles->IsChannelSupported( PHXPRT_FL ) )
            m_nativeMap.define_channel<float>( _T( "Fuel" ) );
    }

    m_nativeMap.end_channel_definition();

    m_gridTM = toWorldTM;

    init_accessors();
}

inline void phoenix_particle_istream::close() {}

inline frantic::tstring phoenix_particle_istream::name() const {
    return frantic::strings::to_tstring( m_particles->GetName() );
}

inline std::size_t phoenix_particle_istream::particle_size() const { return m_outMap.structure_size(); }

inline boost::int64_t phoenix_particle_istream::particle_count() const { return m_particleCount; }

inline boost::int64_t phoenix_particle_istream::particle_index() const { return m_particleIndex; }

inline boost::int64_t phoenix_particle_istream::particle_count_left() const {
    return m_particleCount - m_particleIndex - 1;
}

inline boost::int64_t phoenix_particle_istream::particle_progress_count() const { return m_particleCount; }

inline boost::int64_t phoenix_particle_istream::particle_progress_index() const { return m_particleIndex; }

inline void phoenix_particle_istream::set_channel_map( const frantic::channels::channel_map& particleChannelMap ) {
    if( m_outMap != particleChannelMap ) {
        m_outMap = particleChannelMap;
        m_defaultParticle.reset();
        init_accessors();
    }
}

inline const frantic::channels::channel_map& phoenix_particle_istream::get_channel_map() const { return m_outMap; }

inline const frantic::channels::channel_map& phoenix_particle_istream::get_native_channel_map() const {
    return m_nativeMap;
}

inline void phoenix_particle_istream::set_default_particle( char* rawParticleBuffer ) {
    if( !m_defaultParticle )
        m_defaultParticle.reset( new char[m_outMap.structure_size()] );
    m_outMap.copy_structure( m_defaultParticle.get(), rawParticleBuffer );
}

inline bool phoenix_particle_istream::get_particle( char* rawParticleBuffer ) {
    if( ++m_particleIndex >= m_particleCount )
        return false;

    if( m_defaultParticle )
        m_outMap.copy_structure( rawParticleBuffer, m_defaultParticle.get() );

    float tempSpace[9];

    m_particles->GetChannel( (int)m_particleIndex, PHXPRT_POS, tempSpace );
    m_accessors.pos.get( rawParticleBuffer ) =
        m_gridTM * frantic::graphics::vector3f( tempSpace[0], tempSpace[1], tempSpace[2] );

    if( m_accessors.vel.is_valid() ) {
        m_particles->GetChannel( (int)m_particleIndex, PHXPRT_VEL, tempSpace );
        m_accessors.vel.set( rawParticleBuffer, m_gridTM.transform_no_translation( frantic::graphics::vector3f(
                                                    tempSpace[0], tempSpace[1], tempSpace[2] ) ) );
    }

    if( m_accessors.texturecoord.is_valid() ) {
        m_particles->GetChannel( (int)m_particleIndex, PHXPRT_UVW, tempSpace );
        m_accessors.texturecoord.set( rawParticleBuffer,
                                      frantic::graphics::vector3f( tempSpace[0], tempSpace[1], tempSpace[2] ) );
    }

    if( m_accessors.orientation.is_valid() ) {
        m_particles->GetChannel( (int)m_particleIndex, PHXPRT_OR, tempSpace );

        frantic::graphics::quat4f result = frantic::graphics::quat4f::from_coord_sys(
            frantic::graphics::vector3f( tempSpace[0], tempSpace[1], tempSpace[2] ),
            frantic::graphics::vector3f( tempSpace[3], tempSpace[4], tempSpace[5] ),
            frantic::graphics::vector3f( tempSpace[6], tempSpace[7], tempSpace[8] ) );

        m_accessors.orientation.set( rawParticleBuffer, result );
    }

    if( m_accessors.size.is_valid() ) {
        m_particles->GetChannel( (int)m_particleIndex, PHXPRT_SIZE, tempSpace );
        m_accessors.size.set( rawParticleBuffer, tempSpace[0] );
    }

    if( m_accessors.age.is_valid() ) {
        m_particles->GetChannel( (int)m_particleIndex, PHXPRT_AGE, tempSpace );
        m_accessors.age.set( rawParticleBuffer, tempSpace[0] );
    }

    if( m_accessors.temp.is_valid() ) {
        m_particles->GetChannel( (int)m_particleIndex, PHXPRT_T, tempSpace );
        m_accessors.temp.set( rawParticleBuffer, tempSpace[0] );
    }

    if( m_accessors.density.is_valid() ) {
        m_particles->GetChannel( (int)m_particleIndex, PHXPRT_SM, tempSpace );
        m_accessors.density.set( rawParticleBuffer, tempSpace[0] );
    }

    if( m_accessors.fuel.is_valid() ) {
        m_particles->GetChannel( (int)m_particleIndex, PHXPRT_FL, tempSpace );
        m_accessors.fuel.set( rawParticleBuffer, tempSpace[0] );
    }

    return true;
}

inline bool phoenix_particle_istream::get_particles( char* rawParticleBuffer, std::size_t& numParticles ) {
    for( std::size_t i = 0; i < numParticles; ++i, rawParticleBuffer += m_outMap.structure_size() ) {
        if( !this->get_particle( rawParticleBuffer ) ) {
            numParticles = i;
            return false;
        }
    }

    return true;
}

} // namespace streams
} // namespace particles
} // namespace max3d
} // namespace frantic
#endif
