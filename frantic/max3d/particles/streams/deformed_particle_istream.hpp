// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/channels/channel_map_adaptor.hpp>
#include <frantic/particles/particle_array.hpp>
#include <frantic/particles/particle_array_iterator.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

#include <frantic/max3d/max_utility.hpp>
#include <frantic/max3d/particles/modifier_utils.hpp>

namespace frantic {
namespace max3d {
namespace particles {
namespace streams {

namespace detail {
inline boost::shared_ptr<frantic::particles::streams::particle_istream>
apply_modifiers_to_particle_istream( boost::shared_ptr<frantic::particles::streams::particle_istream> pin,
                                     SClass_ID modifierType, INode* pModifiedNode, TimeValue t, TimeValue timeStep,
                                     bool renderMode = false );
}

class deformed_particle_istream : public frantic::particles::streams::delegated_particle_istream {
  private:
    static const size_t BUFFER_SIZE = 5000;

    TriObject* m_pTempObj;
    INode* m_pNode;
    TimeValue m_time;
    TimeValue m_timeStep;

    std::vector<frantic::max3d::particles::modifier_info_t> m_modifiers;

    frantic::channels::channel_map m_outPcm, m_internalPcm, m_nativeMap;
    frantic::channels::channel_map_adaptor m_adaptor;

    frantic::particles::particle_array m_particleBuffer;
    frantic::particles::particle_array::iterator m_particleIterator;

    boost::int64_t m_bufferIndex;

    bool m_noDefaultSelection;
    bool m_done;

    boost::scoped_array<char> m_defaultParticle;

  private:
    void load_next_particle_chunk() {
        const channels::channel_map& pcm = m_particleBuffer.get_channel_map();

        m_particleBuffer.resize( BUFFER_SIZE );
        m_bufferIndex = 0;

        size_t count;
        for( count = 0; count < BUFFER_SIZE; ++count ) {
            if( !m_delegate->get_particle( m_particleBuffer[count] ) ) {
                m_done = true;
                break;
            }
        }

        m_particleBuffer.resize( count );

        m_particleIterator = m_particleBuffer.begin();
        frantic::max3d::particles::detail::apply_modifiers_to_particles(
            pcm, m_particleBuffer.begin(), m_particleBuffer.end(), m_noDefaultSelection, m_pTempObj, m_modifiers,
            m_pNode, m_time, m_timeStep );
    }

    void internal_set_channel_map( const frantic::channels::channel_map& map, bool setDelegateMap ) {
        if( !m_defaultParticle ) {
            m_defaultParticle.reset( new char[map.structure_size()] );
            map.construct_structure( m_defaultParticle.get() );
        } else {
            boost::scoped_array<char> newDefault( new char[map.structure_size()] );

            frantic::channels::channel_map_adaptor adaptor( map, m_outPcm );

            map.construct_structure( newDefault.get() );
            adaptor.copy_structure( newDefault.get(), m_defaultParticle.get() );

            m_defaultParticle.swap( newDefault );
        }

        m_outPcm = m_internalPcm = map;

        const frantic::channels::channel_map& delegateMap = m_delegate->get_native_channel_map();

        // The selection of particles can affect the behavior of modifiers, regardless of whether the downstream object
        // plans to use it.
        if( !m_internalPcm.has_channel( _T("Selection") ) && delegateMap.has_channel( _T("Selection") ) ) {
            m_internalPcm.append_channel<float>( _T("Selection") );
            setDelegateMap = true;
        }

        // Strip out any channels that aren't in the delegate map. Keep the selection channel if this modifier will
        // populate it though.
        std::vector<frantic::tstring> channelsToDelete;
        for( std::size_t i = 0, iEnd = m_internalPcm.channel_count(); i < iEnd; ++i ) {
            if( !m_nativeMap.has_channel( m_internalPcm[i].name() ) )
                channelsToDelete.push_back( m_internalPcm[i].name() );
        }

        for( std::vector<frantic::tstring>::const_iterator it = channelsToDelete.begin(),
                                                           itEnd = channelsToDelete.end();
             it != itEnd; ++it )
            m_internalPcm.delete_channel( *it, true );

        m_particleBuffer.reset( m_internalPcm );
        m_particleIterator = m_particleBuffer.end();
        m_adaptor.set( m_outPcm, m_internalPcm );

        m_bufferIndex = 0;

        if( setDelegateMap )
            m_delegate->set_channel_map( m_internalPcm );
    }

  public:
    deformed_particle_istream( boost::shared_ptr<frantic::particles::streams::particle_istream> pin,
                               const std::vector<frantic::max3d::particles::modifier_info_t>& modifiers, INode* pNode,
                               TimeValue t, TimeValue timeStep )
        : delegated_particle_istream( pin )
        , m_modifiers( modifiers )
        , m_pNode( pNode )
        , m_time( t )
        , m_timeStep( timeStep ) {
        m_noDefaultSelection = false;
        m_done = false;

        m_nativeMap = m_delegate->get_native_channel_map();
        if( !m_nativeMap.has_channel( _T("Selection") ) ) {
            m_noDefaultSelection = true;

            bool writesSelection = false;
            for( std::size_t i = 0, iEnd = modifiers.size(); i < iEnd && !writesSelection; ++i ) {
                if( modifiers[i].first->ChannelsChanged() & SELECT_CHANNEL )
                    writesSelection = true;
            }
            if( writesSelection )
                m_nativeMap.append_channel<float>( _T("Selection") );
        }

        internal_set_channel_map( m_delegate->get_channel_map(), false );

        m_pTempObj = (TriObject*)CreateInstance( GEOMOBJECT_CLASS_ID, triObjectClassID );
    }

    virtual ~deformed_particle_istream() { close(); }

    void close() {
        if( m_pTempObj ) {
            m_pTempObj->MaybeAutoDelete();
            m_pTempObj = 0;
        }

        delegated_particle_istream::close();
    }

    const frantic::channels::channel_map& get_channel_map() const { return m_outPcm; }

    const frantic::channels::channel_map& get_native_channel_map() const { return m_nativeMap; }

    std::size_t particle_size() { return m_outPcm.structure_size(); }

    boost::int64_t particle_index() const {
        // NOTE: This assumes m_delegate->particle_index() returns -1 before the first call to get_particle().
        // call| m_delegate->particle_index() | m_particleBuffer.size() | m_bufferIndex | Result
        //-------------------------------------------------------------------------------------
        //   0 | -1                           | 0                       | 0             | -1 - 0 + 0        = -1
        //   1 |  N - 1                       | N                       | 1             | N - 1 - N + 1     = 0
        //
        //
        //  N-1|  N - 1                       | N                       | N - 1         | N - 1 - N + N - 1 = N - 2
        //  N  |  N - 1                       | N                       | N             | N - 1 - N + N     = N - 1
        //  N+1|  2* N - 1                    | N                       | 1             | 2N - 1 - N + 1    = N
        return m_delegate->particle_index() - (boost::int64_t)m_particleBuffer.size() + m_bufferIndex;
    }

    boost::int64_t particle_count_left() const {
        boost::int64_t delegateLeft = m_delegate->particle_count_left();
        if( delegateLeft >= 0 )
            return delegateLeft + (boost::int64_t)m_particleBuffer.size() - m_bufferIndex;
        return -1;
    }

    void set_channel_map( const channels::channel_map& map ) { internal_set_channel_map( map, true ); }

    void set_default_particle( char* rawParticleBuffer ) {
        frantic::channels::channel_map_adaptor internalAdaptor( m_internalPcm, m_outPcm );

        char* internalDefault = static_cast<char*>( alloca( m_internalPcm.structure_size() ) );

        m_internalPcm.construct_structure( internalDefault );

        internalAdaptor.copy_structure( internalDefault, rawParticleBuffer );

        m_delegate->set_default_particle( internalDefault );

        m_outPcm.copy_structure( m_defaultParticle.get(), rawParticleBuffer );
    }

    bool get_particle( char* rawParticleBuffer ) {
        if( m_particleIterator == m_particleBuffer.end() ) {
            if( m_done )
                return false;
            load_next_particle_chunk();
            if( m_particleIterator == m_particleBuffer.end() )
                return false;
        }

        if( m_adaptor.is_identity() )
            memcpy( rawParticleBuffer, *m_particleIterator, m_particleBuffer.get_channel_map().structure_size() );
        else
            m_adaptor.copy_structure( rawParticleBuffer, *m_particleIterator, m_defaultParticle.get() );

        ++m_bufferIndex;
        ++m_particleIterator;

        return true;
    }

    bool get_particles( char* buffer, std::size_t& numParticles ) {
        std::size_t particleSize = m_adaptor.source_size();
        bool notEos;

        if( m_adaptor.is_identity() ) {
            notEos = m_delegate->get_particles( buffer, numParticles );

            frantic::max3d::particles::detail::apply_modifiers_to_particles(
                m_internalPcm, frantic::particles::particle_array_iterator( buffer, particleSize ),
                frantic::particles::particle_array_iterator( buffer, particleSize ) + numParticles,
                m_noDefaultSelection, m_pTempObj, m_modifiers, m_pNode, m_time, m_timeStep );
        } else {
            boost::scoped_array<char> tempBuffer( new char[particleSize * numParticles] );

            notEos = m_delegate->get_particles( tempBuffer.get(), numParticles );
            frantic::particles::particle_array_iterator begin( tempBuffer.get(), particleSize );
            frantic::particles::particle_array_iterator end = begin + numParticles;

            frantic::max3d::particles::detail::apply_modifiers_to_particles(
                m_internalPcm, begin, end, m_noDefaultSelection, m_pTempObj, m_modifiers, m_pNode, m_time, m_timeStep );

            for( std::size_t i = 0; i < numParticles; ++i )
                m_adaptor.copy_structure( buffer + i * m_adaptor.dest_size(),
                                          tempBuffer.get() + i * m_adaptor.source_size(), m_defaultParticle.get() );
        }

        return notEos;
    }
};

inline boost::shared_ptr<frantic::particles::streams::particle_istream>
apply_wsm_modifiers_to_particle_istream( boost::shared_ptr<frantic::particles::streams::particle_istream> pin,
                                         INode* pModifiedNode, TimeValue t, TimeValue timeStep,
                                         bool renderMode = false ) {
    return detail::apply_modifiers_to_particle_istream( pin, WSM_CLASS_ID, pModifiedNode, t, timeStep, renderMode );
}

inline boost::shared_ptr<frantic::particles::streams::particle_istream>
apply_osm_modifiers_to_particle_istream( boost::shared_ptr<frantic::particles::streams::particle_istream> pin,
                                         INode* pModifiedNode, TimeValue t, TimeValue timeStep,
                                         bool renderMode = false ) {
    return detail::apply_modifiers_to_particle_istream( pin, OSM_CLASS_ID, pModifiedNode, t, timeStep, renderMode );
}

namespace detail {
inline boost::shared_ptr<frantic::particles::streams::particle_istream>
apply_modifiers_to_particle_istream( boost::shared_ptr<frantic::particles::streams::particle_istream> pin,
                                     SClass_ID modifierType, INode* pModifiedNode, TimeValue t, TimeValue timeStep,
                                     bool renderMode ) {
    std::vector<frantic::max3d::particles::modifier_info_t> modifiers;
    frantic::max3d::collect_node_modifiers( pModifiedNode, modifiers, modifierType, renderMode );

    if( modifiers.size() > 0 )
        pin.reset( new deformed_particle_istream( pin, modifiers, pModifiedNode, t, timeStep ) );

    return pin;
}
} // namespace detail

} // namespace streams
} // namespace particles
} // namespace max3d
} // namespace frantic
