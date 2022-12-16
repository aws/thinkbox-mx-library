// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/max3d/particles/IMaxKrakatoaParticleInterface.hpp>

#include <frantic/max3d/particles/IMaxKrakatoaPRTObject.hpp>
#include <frantic/max3d/particles/max3d_particle_utils.hpp>
#include <frantic/max3d/rendering/renderplugin_utils.hpp>
#include <frantic/particles/streams/particle_array_particle_istream.hpp>
#include <set>

//
//
// KrakatoaParticleChannelAccessor_impl
//
//

class KrakatoaParticleChannelAccessor_impl : public KrakatoaParticleChannelAccessor {
  private:
    std::string m_name;
    channel_data_type_t m_dataType;
    std::size_t m_arity;
    std::size_t m_byteOffsetInParticle;

  public:
    KrakatoaParticleChannelAccessor_impl( std::string name, channel_data_type_t dataType, std::size_t arity,
                                          std::size_t byteOffsetInParticle )
        : m_name( name )
        , m_dataType( dataType )
        , m_arity( arity )
        , m_byteOffsetInParticle( byteOffsetInParticle ) {}

    virtual const char* get_name() const { return m_name.c_str(); };
    virtual channel_data_type_t get_data_type() const { return m_dataType; };
    virtual unsigned int get_arity() const { return (unsigned int)m_arity; };

    virtual bool is_int_channel() const {
        switch( m_dataType ) {
        case DATA_TYPE_INT64:
        case DATA_TYPE_INT32:
        case DATA_TYPE_INT16:
        case DATA_TYPE_INT8:
            return true;
        default:
            return false;
        }
    }
    virtual bool is_uint_channel() const {
        switch( m_dataType ) {
        case DATA_TYPE_UINT64:
        case DATA_TYPE_UINT32:
        case DATA_TYPE_UINT16:
        case DATA_TYPE_UINT8:
            return true;
        default:
            return false;
        }
    }
    virtual bool is_float_channel() const {
        switch( m_dataType ) {
        case DATA_TYPE_FLOAT64:
        case DATA_TYPE_FLOAT32:
        case DATA_TYPE_FLOAT16:
            return true;
        default:
            return false;
        }
    }

    void get_int64( INT64& outValue, const void* particleData ) const {
        if( !( m_arity == 1 ) )
            throw std::runtime_error(
                "get_int64: Expected arity of one. Found arity: " + boost::lexical_cast<std::string>( m_arity ) +
                ". Channel name: \"" + m_name + "\"" );

        switch( m_dataType ) {
        case DATA_TYPE_INT64: {
            get_channel_value( &outValue, particleData );
            break;
        }
        case DATA_TYPE_INT32: {
            boost::int32_t tempValue;
            get_channel_value( &tempValue, particleData );
            outValue = tempValue;
            break;
        }
        case DATA_TYPE_INT16: {
            boost::int16_t tempValue;
            get_channel_value( &tempValue, particleData );
            outValue = tempValue;
            break;
        }
        case DATA_TYPE_INT8: {
            boost::int8_t tempValue;
            get_channel_value( &tempValue, particleData );
            outValue = tempValue;
            break;
        }
        default:
            throw std::runtime_error( "get_int64: Data type not compatible. Channel name: \"" + m_name + "\"" );
            break;
        }
    }
    void get_uint64( UINT64& outValue, const void* particleData ) const {
        if( m_arity != 1 )
            throw std::runtime_error(
                "get_uint64: Expected arity of one. Found arity: " + boost::lexical_cast<std::string>( m_arity ) +
                ". Channel name: \"" + m_name + "\"" );

        switch( m_dataType ) {
        case DATA_TYPE_UINT64: {
            get_channel_value( &outValue, particleData );
            break;
        }
        case DATA_TYPE_UINT32: {
            boost::uint32_t tempValue;
            get_channel_value( &tempValue, particleData );
            outValue = tempValue;
            break;
        }
        case DATA_TYPE_UINT16: {
            boost::uint16_t tempValue;
            get_channel_value( &tempValue, particleData );
            outValue = tempValue;
            break;
        }
        case DATA_TYPE_UINT8: {
            boost::uint8_t tempValue;
            get_channel_value( &tempValue, particleData );
            outValue = tempValue;
            break;
        }
        default:
            throw std::runtime_error( "get_uint64: Data type not compatible. Channel name: \"" + m_name + "\"" );
            break;
        }
    }
    void get_float( float& outValue, const void* particleData ) const {
        if( m_arity != 1 )
            throw std::runtime_error(
                "get_float: Expected arity of one. Found arity: " + boost::lexical_cast<std::string>( m_arity ) +
                ". Channel name: \"" + m_name + "\"" );

        switch( m_dataType ) {
        case DATA_TYPE_FLOAT64: {
            double tempValue;
            get_channel_value( &tempValue, particleData );
            outValue = (float)tempValue;
            break;
        }
        case DATA_TYPE_FLOAT32: {
            get_channel_value( &outValue, particleData );
            break;
        }
        case DATA_TYPE_FLOAT16: {
            half tempValue;
            get_channel_value( &tempValue, particleData );
            outValue = tempValue;
            break;
        }
        default:
            throw std::runtime_error( "get_float: Data type not compatible. Channel name: \"" + m_name + "\"" );
            break;
        }
    }
    void get_float_vector( float outVector[3], const void* particleData ) const {
        if( m_arity != 3 )
            throw std::runtime_error( "get_float_vector: Expected arity of three. Found arity: " +
                                      boost::lexical_cast<std::string>( m_arity ) + ". Channel name: \"" + m_name +
                                      "\"" );

        switch( m_dataType ) {
        case DATA_TYPE_FLOAT64: {
            double tempVector[3];
            get_channel_value( tempVector, particleData );
            outVector[0] = (float)tempVector[0];
            outVector[1] = (float)tempVector[1];
            outVector[2] = (float)tempVector[2];
            break;
        }
        case DATA_TYPE_FLOAT32: {
            get_channel_value( outVector, particleData );
            break;
        }
        case DATA_TYPE_FLOAT16: {
            half tempVector[3];
            get_channel_value( tempVector, particleData );
            outVector[0] = tempVector[0];
            outVector[1] = tempVector[1];
            outVector[2] = tempVector[2];
            break;
        }
        default:
            throw std::runtime_error( "get_float_vector: Data type not compatible. Channel name: \"" + m_name + "\"" );
            break;
        }
    }
    void get_double( double& outValue, const void* particleData ) const {
        if( m_arity != 1 )
            throw std::runtime_error(
                "get_double: Expected arity of one. Found arity: " + boost::lexical_cast<std::string>( m_arity ) +
                ". Channel name: \"" + m_name + "\"" );

        switch( m_dataType ) {
        case DATA_TYPE_FLOAT64: {
            get_channel_value( &outValue, particleData );
            break;
        }
        case DATA_TYPE_FLOAT32: {
            float tempValue;
            get_channel_value( &tempValue, particleData );
            outValue = tempValue;
            break;
        }
        case DATA_TYPE_FLOAT16: {
            half tempValue;
            get_channel_value( &tempValue, particleData );
            outValue = tempValue;
            break;
        }
        default:
            throw std::runtime_error( "get_double: Data type not compatible. Channel name: \"" + m_name + "\"" );
            break;
        }
    }
    void get_double_vector( double outVector[3], const void* particleData ) const {
        if( m_arity != 3 )
            throw std::runtime_error( "get_double_vector: Expected arity of three. Found arity: " +
                                      boost::lexical_cast<std::string>( m_arity ) + ". Channel name: \"" + m_name +
                                      "\"" );

        switch( m_dataType ) {
        case DATA_TYPE_FLOAT64: {
            get_channel_value( outVector, particleData );
            break;
        }
        case DATA_TYPE_FLOAT32: {
            float tempVector[3];
            get_channel_value( tempVector, particleData );
            outVector[0] = tempVector[0];
            outVector[1] = tempVector[1];
            outVector[2] = tempVector[2];
            break;
        }
        case DATA_TYPE_FLOAT16: {
            half tempVector[3];
            get_channel_value( tempVector, particleData );
            outVector[0] = tempVector[0];
            outVector[1] = tempVector[1];
            outVector[2] = tempVector[2];
            break;
        }
        default:
            throw std::runtime_error( "get_double_vector: Data type not compatible. Channel name: \"" + m_name + "\"" );
            break;
        }
    }

    virtual void get_channel_value( void* outValue, const void* particleData ) const {
        memcpy( (char*)outValue, (char*)particleData + m_byteOffsetInParticle,
                ( sizeof_channel_data_type( (frantic::channels::data_type_t)m_dataType ) * m_arity ) );
    }
};

//
//
// KrakatoaParticleStream_impl
//
//

class KrakatoaParticleStream_impl : public KrakatoaParticleStream {
    frantic::particles::particle_istream_ptr m_internalStream;
    frantic::channels::channel_map m_map;
    std::vector<KrakatoaParticleChannelAccessor_impl> m_channelAccessors;

  public:
    KrakatoaParticleStream_impl( frantic::particles::particle_istream_ptr stream )
        : m_internalStream( stream )
        , m_map( stream->get_channel_map() ) {
        for( std::size_t i = 0, iEnd = m_map.channel_count(); i < iEnd; ++i ) {
            const frantic::channels::channel& ch = m_map[i];
            m_channelAccessors.push_back( KrakatoaParticleChannelAccessor_impl(
                frantic::strings::to_string( ch.name() ), (channel_data_type_t)ch.data_type(), ch.arity(),
                m_map.channel_offset( ch.name() ) ) );
        }
    }
    ~KrakatoaParticleStream_impl() { m_internalStream->close(); }

    virtual bool get_next_particle( void* outParticleData ) {
        return m_internalStream->get_particle( (char*)outParticleData );
    }
    virtual INT64 particle_count() const { return m_internalStream->particle_count(); }
    virtual unsigned int particle_size() const { return (unsigned int)m_internalStream->particle_size(); }
    virtual bool has_channel( const char* name ) const {
        return m_map.has_channel( frantic::strings::to_tstring( name ) );
    }
    virtual const KrakatoaParticleChannelAccessor* get_channel_data_accessor( const char* name ) const {
        if( !has_channel( name ) )
            throw std::runtime_error( "get_channel: Channel not found. Channel name: \"" + std::string( name ) + "\"" );

        size_t index = m_map.channel_index( frantic::strings::to_tstring( name ) );
        return &m_channelAccessors[index];
    }
    virtual unsigned int channel_count() const { return (unsigned int)m_channelAccessors.size(); };
    virtual const KrakatoaParticleChannelAccessor* get_channel_data_accessor( unsigned int index ) const {
        if( index >= m_channelAccessors.size() )
            throw std::runtime_error( "get_channel: There is no channel index of " +
                                      boost::lexical_cast<std::string>( index ) + ". There are only " +
                                      boost::lexical_cast<std::string>( m_channelAccessors.size() ) + " channels." );

        return &m_channelAccessors[index];
    };
};

//
//
// IMaxKrakatoaParticleInterface
//
//

// annoyingly this has been reimplemented in so many places, and now here it is again. what a pain.
class render_mode_forcer {
  private:
    TimeValue m_t;
    INode* m_node;

  public:
    render_mode_forcer( bool forceRenderMode, TimeValue t, INode* node ) {
        m_node = NULL;
        if( forceRenderMode ) {
            m_t = t;
            m_node = node;
            std::set<ReferenceMaker*> doneNodes;
            frantic::max3d::rendering::refmaker_call_recursive(
                m_node, doneNodes, frantic::max3d::rendering::render_begin_function( m_t, 0 ) );
        }
    }
    ~render_mode_forcer() {
        if( m_node ) {
            std::set<ReferenceMaker*> doneNodes;
            frantic::max3d::rendering::refmaker_call_recursive( m_node, doneNodes,
                                                                frantic::max3d::rendering::render_end_function( m_t ) );
        }
    }
};

KrakatoaParticleStream* IMaxKrakatoaParticleInterface::create_stream( INode* pNode, TimeValue t, Interval& outValidity,
                                                                      bool inWorldSpace, bool applyMaterial,
                                                                      bool forceRenderMode ) {
    using namespace frantic::max3d::particles;

    IMaxKrakatoaPRTObject* prtObject = dynamic_cast<frantic::max3d::particles::IMaxKrakatoaPRTObject*>( this );

    // forces switching over to render mode if requested. it's scoped, so it will switch back automatically.
    render_mode_forcer renderModeForcer( forceRenderMode, t, pNode );

    IMaxKrakatoaPRTEvalContextPtr pEvalContext = CreateMaxKrakatoaPRTEvalContext(
        t, Class_ID( 0x57de093f, 0x621075b1 ), NULL, NULL, inWorldSpace, applyMaterial );
    frantic::particles::particle_istream_ptr pin = prtObject->CreateStream( pNode, outValidity, pEvalContext );
    pin = visibility_density_scale_stream_with_inode( pNode, t, pin );
    pin->set_channel_map( pin->get_native_channel_map() );

    return new KrakatoaParticleStream_impl( pin );
}

void IMaxKrakatoaParticleInterface::destroy_stream( KrakatoaParticleStream* stream ) {
    if( stream ) {
        KrakatoaParticleStream_impl* streamImpl = dynamic_cast<KrakatoaParticleStream_impl*>( stream );
        delete streamImpl;
    }
}
