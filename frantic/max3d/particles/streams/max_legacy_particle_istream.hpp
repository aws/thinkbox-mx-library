// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/particles/max3d_particle_streams.hpp>
#include <frantic/max3d/particles/max3d_particle_utils.hpp>
#include <frantic/max3d/time.hpp>

#include <frantic/particles/streams/particle_istream.hpp>

#pragma warning( push, 3 )
#include <simpobj.h>
#pragma warning( pop )

namespace frantic {
namespace max3d {
namespace particles {
namespace streams {

class max_legacy_particle_istream : public frantic::particles::streams::particle_istream {
  private:
    typedef frantic::graphics::vector3f vector3f;

    SimpleParticle* m_particles;
    TimeValue m_time;

    bool m_cullOldParticles;

    int m_index;
    boost::int64_t m_totalParticles;
    boost::int64_t m_aliveParticles;
    boost::int64_t m_particlesLeft;
    frantic::tstring m_name;

    frantic::channels::channel_map m_nativeMap;
    frantic::channels::channel_map m_outMap;

    std::vector<char> m_defaultParticleBuffer;

    struct {
        bool hasPosition;
        bool hasVelocity;
        bool hasScale;
        bool hasAge;
        bool hasLifeSpan;
        bool hasDensity;
        bool hasRadius;

        frantic::channels::channel_cvt_accessor<vector3f> position;
        frantic::channels::channel_cvt_accessor<vector3f> velocity;
        frantic::channels::channel_cvt_accessor<vector3f> scale;
        frantic::channels::channel_cvt_accessor<double> age;
        frantic::channels::channel_cvt_accessor<double> lifeSpan;
        frantic::channels::channel_cvt_accessor<float> density;
        frantic::channels::channel_cvt_accessor<float> radius;
    } m_accessors;

    void init_stream( INode* pNode, TimeValue t ) {
        ObjectState os = pNode->EvalWorldState( t );
        if( !os.obj->IsParticleSystem() )
            throw std::runtime_error(
                "max_legacy_particle_istream::init_stream() - object is not a legacy particle system" );

        m_particles = static_cast<SimpleParticle*>( os.obj->GetInterface( I_SIMPLEPARTICLEOBJ ) );
        if( !m_particles )
            throw std::runtime_error(
                "max_legacy_particle_istream::init_stream() - object does not implement SimpleParticle interface" );

        m_particles->UpdateParticles( t, pNode );
        m_totalParticles = m_particles->parts.Count();

        m_time = t;
        m_index = -1;
        m_aliveParticles = 0;

        for( int i = 0; i < m_totalParticles; ++i ) {
            TimeValue age = m_particles->ParticleAge( t, i );
            TimeValue life = m_particles->ParticleLife( t, i );
            if( age >= 0 && ( age < life || !m_cullOldParticles ) )
                ++m_aliveParticles;
        }

        m_particlesLeft = m_aliveParticles;

        m_nativeMap.define_channel<vector3f>( _T("Position") );
        m_nativeMap.define_channel<vector3f>( _T("Velocity") );
        m_nativeMap.define_channel<vector3f>( _T("Scale") );
        m_nativeMap.define_channel<double>( _T("Age") );
        m_nativeMap.define_channel<double>( _T("LifeSpan") );
        m_nativeMap.define_channel<float>( _T("Density") );
        m_nativeMap.define_channel<float>( _T("Radius") );
        m_nativeMap.end_channel_definition();
    }

    void init_accessors( const frantic::channels::channel_map& pcm ) {
        if( m_accessors.hasPosition = pcm.has_channel( _T("Position") ) )
            m_accessors.position = pcm.get_cvt_accessor<vector3f>( _T("Position") );
        if( m_accessors.hasVelocity = pcm.has_channel( _T("Velocity") ) )
            m_accessors.velocity = pcm.get_cvt_accessor<vector3f>( _T("Velocity") );
        if( m_accessors.hasScale = pcm.has_channel( _T("Scale") ) )
            m_accessors.scale = pcm.get_cvt_accessor<vector3f>( _T("Scale") );
        if( m_accessors.hasAge = pcm.has_channel( _T("Age") ) )
            m_accessors.age = pcm.get_cvt_accessor<double>( _T("Age") );
        if( m_accessors.hasLifeSpan = pcm.has_channel( _T("LifeSpan") ) )
            m_accessors.lifeSpan = pcm.get_cvt_accessor<double>( _T("LifeSpan") );
        if( m_accessors.hasDensity = pcm.has_channel( _T("Density") ) )
            m_accessors.density = pcm.get_cvt_accessor<float>( _T("Density") );
        if( m_accessors.hasRadius = pcm.has_channel( _T("Radius") ) )
            m_accessors.radius = pcm.get_cvt_accessor<float>( _T("Radius") );
    }

  public:
    max_legacy_particle_istream( INode* pNode, TimeValue t, bool cullOldParticles = true )
        : m_cullOldParticles( cullOldParticles ) {
        init_stream( pNode, t );
        set_channel_map( m_nativeMap );
    }

    max_legacy_particle_istream( INode* pNode, TimeValue t, const frantic::channels::channel_map& pcm,
                                 bool cullOldParticles = true )
        : m_cullOldParticles( cullOldParticles ) {
        init_stream( pNode, t );
        set_channel_map( pcm );
    }

    void set_channel_map( const frantic::channels::channel_map& pcm ) {
        std::vector<char> newDefaultParticle( pcm.structure_size() );
        if( m_defaultParticleBuffer.size() > 0 ) {
            frantic::channels::channel_map_adaptor defaultAdaptor( pcm, m_outMap );
            defaultAdaptor.copy_structure( &newDefaultParticle[0], &m_defaultParticleBuffer[0] );
        } else
            memset( &newDefaultParticle[0], 0, pcm.structure_size() );
        m_defaultParticleBuffer.swap( newDefaultParticle );

        m_outMap = pcm;
        init_accessors( pcm );
    }

    void set_default_particle( char* buffer ) {
        memcpy( &m_defaultParticleBuffer[0], buffer, m_outMap.structure_size() );
    }

    const frantic::channels::channel_map& get_channel_map() const { return m_outMap; }
    const frantic::channels::channel_map& get_native_channel_map() const { return m_nativeMap; }

    void close() {}
    frantic::tstring name() const { return m_name; }
    std::size_t particle_size() const { return m_outMap.structure_size(); }
    boost::int64_t particle_count() const { return m_aliveParticles; }
    boost::int64_t particle_index() const { return m_index; }
    boost::int64_t particle_count_left() const { return m_particlesLeft; }
    boost::int64_t particle_progress_count() const { return m_aliveParticles; }
    boost::int64_t particle_progress_index() const { return m_index; }

    bool get_particle( char* buffer ) {
        while( ++m_index < m_totalParticles ) {
            TimeValue age = m_particles->ParticleAge( m_time, m_index );
            TimeValue life = m_particles->ParticleLife( m_time, m_index );

            if( age >= 0 && ( age < life || !m_cullOldParticles ) ) {
                --m_particlesLeft;

                memcpy( buffer, &m_defaultParticleBuffer[0], m_outMap.structure_size() );

                if( m_accessors.hasPosition )
                    m_accessors.position.set(
                        buffer, frantic::max3d::from_max_t( m_particles->ParticlePosition( m_time, m_index ) ) );

                if( m_accessors.hasVelocity )
                    m_accessors.velocity.set(
                        buffer, TIME_TICKSPERSEC *
                                    frantic::max3d::from_max_t( m_particles->ParticleVelocity( m_time, m_index ) ) );

                if( m_accessors.hasScale )
                    m_accessors.scale.set( buffer, vector3f( m_particles->ParticleSize( m_time, m_index ) ) );

                if( m_accessors.hasAge )
                    m_accessors.age.set( buffer, frantic::max3d::to_seconds<double>( age ) );

                if( m_accessors.hasLifeSpan )
                    m_accessors.lifeSpan.set( buffer, frantic::max3d::to_seconds<double>( life ) );

                if( m_accessors.hasDensity )
                    m_accessors.density.set( buffer, 1.f );

                // I added a "0.5f *" because all of the particle sources I checked
                // need it.  Is it different for some source?
                if( m_accessors.hasRadius )
                    m_accessors.radius.set( buffer, 0.5f * m_particles->ParticleSize( m_time, m_index ) );

                return true;
            }
        }

        return false;
    }

    bool get_particles( char* buffer, std::size_t& numParticles ) {
        for( std::size_t i = 0; i < numParticles; ++i ) {
            if( !this->get_particle( buffer ) ) {
                numParticles = i;
                return false;
            }
            buffer += m_outMap.structure_size();
        }

        return true;
    }
};

} // namespace streams
} // namespace particles
} // namespace max3d
} // namespace frantic
