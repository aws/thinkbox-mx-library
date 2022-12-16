// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/time.hpp>

#include <frantic/particles/streams/particle_istream.hpp>

#pragma warning( push, 3 )
#include <maxtypes.h>
#pragma warning( pop )

#pragma warning( push, 3 )
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#pragma warning( pop )

#include <boost/bind.hpp>

namespace frantic {
namespace max3d {
namespace particles {
namespace streams {

/**
 * Possibly applies a seconds_to_ticks_particle_istream decorator in to a particle_istream if it contains time channel
 * that has a floating point datatype.
 * @param pin The stream to modify
 * @return The same stream if there was nothing to modify, or a new decorated stream that has the time channels
 * converted to ticks.
 */
frantic::particles::particle_istream_ptr convert_time_channels_to_ticks( frantic::particles::particle_istream_ptr pin );

/**
 * This function will populate 'outMap' with the same channels as 'requestedChannels' except it will convert time
 * channels (ie. Age and LifeSpan) to float32 seconds.
 * @param requestedChannels The original channel_map
 * @param[out] outMap Will be assigned the same channels and layout as 'requestedChannels' except with the time channels
 * data types changed to floating point.
 */
void convert_time_channels_to_seconds( const frantic::channels::channel_map& requestedChannels,
                                       frantic::channels::channel_map& outMap );

/**
 * This particle_istream decorator will convert time channels (ie. "Age" and "LifeSpan") from float32 seconds to 3ds Max
 * TimeValue ticks. If the underlying stream is already in TimeValue format, no conversion is applied.
 */
class seconds_to_ticks_particle_istream : public frantic::particles::streams::delegated_particle_istream {
  public:
    seconds_to_ticks_particle_istream( frantic::particles::particle_istream_ptr delegateStream );

    virtual ~seconds_to_ticks_particle_istream();

    virtual const frantic::channels::channel_map& get_channel_map() const;

    virtual const frantic::channels::channel_map& get_native_channel_map() const;

    virtual void set_channel_map( const frantic::channels::channel_map& newMap );

    virtual void set_default_particle( char* rawParticleBuffer );

    virtual bool get_particle( char* rawParticleBuffer );

    virtual bool get_particles( char* particleBuffer, std::size_t& numParticles );

  private:
    // This is a non-virtual member function since it will be called by the constructor.
    void set_channel_map_impl( const frantic::channels::channel_map& newMap );

    void process_particle( char* pParticle ) const;

    void process_particles( const tbb::blocked_range<std::size_t>& range, char* pParticleArray ) const;

  private:
    frantic::channels::channel_accessor<TimeValue> m_outAgeAccessor, m_outLifeSpanAccessor;
    frantic::channels::channel_accessor<float> m_inAgeAccessor, m_inLifeSpanAccessor;
    frantic::channels::channel_map m_outMap, m_nativeMap;
};

inline seconds_to_ticks_particle_istream::seconds_to_ticks_particle_istream(
    frantic::particles::particle_istream_ptr delegateStream )
    : delegated_particle_istream( delegateStream ) {
    const frantic::channels::channel_map& delegateNativeMap = delegated_particle_istream::get_native_channel_map();

    m_nativeMap.reset();
    for( std::size_t i = 0, iEnd = delegateNativeMap.channel_count(); i < iEnd; ++i ) {
        const frantic::channels::channel& ch = delegateNativeMap[i];

        // We can change the type without adjusting the offsets since sizeof(TimeValue) == sizeof(float32)
        // We only do the conversion if the delegate stream has float time.
        if( ( ch.name() == _T("Age") || ch.name() == _T("LifeSpan") ) &&
            frantic::channels::is_channel_data_type_float( ch.data_type() ) ) {
            m_nativeMap.define_channel( ch.name(), ch.arity(),
                                        frantic::channels::channel_data_type_traits<TimeValue>::data_type(),
                                        ch.offset() );
        } else {
            m_nativeMap.define_channel( ch.name(), ch.arity(), ch.data_type(), ch.offset() );
        }
    }
    m_nativeMap.end_channel_definition( delegateNativeMap.structure_size(), true, false );

    this->set_channel_map_impl( m_delegate->get_channel_map() );
}

inline seconds_to_ticks_particle_istream::~seconds_to_ticks_particle_istream() {}

inline void seconds_to_ticks_particle_istream::set_channel_map_impl( const frantic::channels::channel_map& newMap ) {
    using frantic::channels::is_channel_data_type_float;

    const frantic::channels::channel_map& delegateNativeMap = delegated_particle_istream::get_native_channel_map();

    frantic::channels::channel_map delegateMap;

    // We are going to make the exact same channel_map, except we will change the type of Age and LifeSpan if
    // encountered.
    for( std::size_t i = 0, iEnd = newMap.channel_count(); i < iEnd; ++i ) {
        const frantic::channels::channel& ch = newMap[i];

        // We can change the type without adjusting the offsets since sizeof(TimeValue) == sizeof(float32)
        // We only do the conversion if the delegate stream has float time, and the requested stream has TimeValue time.
        if( ( ch.name() == _T("Age") || ch.name() == _T("LifeSpan") ) && delegateNativeMap.has_channel( ch.name() ) &&
            is_channel_data_type_float( delegateNativeMap[ch.name()].data_type() ) &&
            !is_channel_data_type_float( ch.data_type() ) ) {
            delegateMap.define_channel( ch.name(), ch.arity(), frantic::channels::data_type_float32, ch.offset() );
        } else {
            delegateMap.define_channel( ch.name(), ch.arity(), ch.data_type(), ch.offset() );
        }
    }
    delegateMap.end_channel_definition( newMap.structure_size(), true, false );

    if( delegated_particle_istream::get_channel_map() != delegateMap )
        delegated_particle_istream::set_channel_map( delegateMap );

    m_outMap = newMap;

    m_inAgeAccessor.reset();
    m_outAgeAccessor.reset();

    if( newMap.has_channel( _T("Age") ) && delegateMap.has_channel( _T("Age") ) &&
        is_channel_data_type_float( delegateMap[_T("Age")].data_type() ) &&
        !is_channel_data_type_float( newMap[_T("Age")].data_type() ) ) {
        m_outAgeAccessor = newMap.get_accessor<TimeValue>( _T("Age") );
        m_inAgeAccessor = delegateMap.get_accessor<float>( _T("Age") );
    }

    m_inLifeSpanAccessor.reset();
    m_outLifeSpanAccessor.reset();

    if( newMap.has_channel( _T("LifeSpan") ) && delegateMap.has_channel( _T("LifeSpan") ) &&
        is_channel_data_type_float( delegateMap[_T("LifeSpan")].data_type() ) &&
        !is_channel_data_type_float( newMap[_T("LifeSpan")].data_type() ) ) {
        m_outLifeSpanAccessor = newMap.get_accessor<TimeValue>( _T("LifeSpan") );
        m_inLifeSpanAccessor = delegateMap.get_accessor<float>( _T("LifeSpan") );
    }
}

inline void seconds_to_ticks_particle_istream::process_particle( char* pParticle ) const {
    if( m_outAgeAccessor.is_valid() ) { // We only set the accessor if m_inAgeAccessor is also valid
        m_outAgeAccessor.get( pParticle ) = frantic::max3d::to_ticks( m_inAgeAccessor.get( pParticle ) );
    }

    if( m_outLifeSpanAccessor.is_valid() ) { // We only set the accessor if m_inLifeSpanAccessor is also valid
        m_outLifeSpanAccessor.get( pParticle ) = frantic::max3d::to_ticks( m_inLifeSpanAccessor.get( pParticle ) );
    }
}

inline void seconds_to_ticks_particle_istream::process_particles( const tbb::blocked_range<std::size_t>& range,
                                                                  char* pParticleArray ) const {
    char* it = pParticleArray + range.begin() * m_outMap.structure_size();
    char* itEnd = pParticleArray + range.end() * m_outMap.structure_size();

    for( ; it != itEnd; it += m_outMap.structure_size() )
        this->process_particle( it );
}

inline const frantic::channels::channel_map& seconds_to_ticks_particle_istream::get_channel_map() const {
    return m_outMap;
}

inline const frantic::channels::channel_map& seconds_to_ticks_particle_istream::get_native_channel_map() const {
    return m_nativeMap;
}

inline void seconds_to_ticks_particle_istream::set_channel_map( const frantic::channels::channel_map& newMap ) {
    this->set_channel_map_impl( newMap );
}

inline void seconds_to_ticks_particle_istream::set_default_particle( char* rawParticleBuffer ) {
    // We don't need a full channel_map_adaptor because we only need to (in-place) change the type of a couple channels.
    if( m_outAgeAccessor.is_valid() && m_inAgeAccessor.is_valid() )
        m_inAgeAccessor.get( rawParticleBuffer ) = TicksToSec( m_outAgeAccessor.get( rawParticleBuffer ) );

    if( m_outLifeSpanAccessor.is_valid() && m_inLifeSpanAccessor.is_valid() )
        m_inLifeSpanAccessor.get( rawParticleBuffer ) = TicksToSec( m_outLifeSpanAccessor.get( rawParticleBuffer ) );

    delegated_particle_istream::set_default_particle( rawParticleBuffer );
}

inline bool seconds_to_ticks_particle_istream::get_particle( char* rawParticleBuffer ) {
    if( !m_delegate->get_particle( rawParticleBuffer ) )
        return false;

    this->process_particle( rawParticleBuffer );

    return true;
}

inline bool seconds_to_ticks_particle_istream::get_particles( char* particleBuffer, std::size_t& numParticles ) {
    bool eos = !m_delegate->get_particles( particleBuffer, numParticles );

    tbb::parallel_for( tbb::blocked_range<std::size_t>( 0, numParticles ),
                       boost::bind( &seconds_to_ticks_particle_istream::process_particles, this, _1, particleBuffer ),
                       tbb::auto_partitioner() );

    return !eos;
}

inline frantic::particles::particle_istream_ptr
convert_time_channels_to_ticks( frantic::particles::particle_istream_ptr pin ) {
    const frantic::channels::channel_map& nativeMap = pin->get_native_channel_map();

    if( ( nativeMap.has_channel( _T("Age") ) &&
          frantic::channels::is_channel_data_type_float( nativeMap[_T("Age")].data_type() ) ) ||
        ( nativeMap.has_channel( _T("LifeSpan") ) &&
          frantic::channels::is_channel_data_type_float( nativeMap[_T("LifeSpan")].data_type() ) ) ) {
        pin.reset( new frantic::max3d::particles::streams::seconds_to_ticks_particle_istream( pin ) );
    }

    return pin;
}

inline void convert_time_channels_to_seconds( const frantic::channels::channel_map& requestedChannels,
                                              frantic::channels::channel_map& outMap ) {
    outMap.reset();
    for( std::size_t i = 0, iEnd = requestedChannels.channel_count(); i < iEnd; ++i ) {
        const frantic::channels::channel& ch = requestedChannels[i];

        // We can change the type without adjusting the offsets since sizeof(TimeValue) == sizeof(float32)
        // We only do the conversion if the delegate stream has non-float time.
        if( ( ch.name() == _T("Age") || ch.name() == _T("LifeSpan") ) &&
            !frantic::channels::is_channel_data_type_float( ch.data_type() ) ) {
            outMap.define_channel( ch.name(), ch.arity(), frantic::channels::data_type_float32, ch.offset() );
        } else {
            outMap.define_channel( ch.name(), ch.arity(), ch.data_type(), ch.offset() );
        }
    }
    outMap.end_channel_definition( requestedChannels.structure_size(), true, false );
}

} // namespace streams
} // namespace particles
} // namespace max3d
} // namespace frantic
