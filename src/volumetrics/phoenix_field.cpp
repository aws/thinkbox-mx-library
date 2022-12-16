// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#if defined( PHOENIX_SDK_AVAILABLE )

#include <aurinterface.h>
#include <phxcontent.h>

#include <frantic/graphics/transform4f.hpp>
#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/volumetrics/phoenix_field.hpp>
#include <frantic/volumetrics/levelset/rle_trilerp.hpp>

#include <memory>

namespace frantic {
namespace max3d {
namespace volumetrics {

bool is_phoenix_node( INode* pNode, TimeValue t ) {
    if( pNode ) {
        ObjectState os = pNode->EvalWorldState( t );

        if( os.obj && os.obj->GetInterface( PHOENIXFD_INTERFACE ) != NULL )
            return true;
    }
    return false;
}

class phoenix_field : public frantic::volumetrics::field_interface {
    frantic::channels::channel_map m_channelMap;
    frantic::channels::channel_accessor<float> m_tempAccessor;
    frantic::channels::channel_accessor<float> m_smokeAccessor;
    frantic::channels::channel_accessor<float> m_fuelAccessor;
    frantic::channels::channel_accessor<frantic::graphics::vector3f> m_velAccessor;
    frantic::channels::channel_accessor<frantic::graphics::vector3f> m_texAccessor;

    // maximum PhoenixFD channel number + 1
    static const int NUM_CHANNELS = 11;

    IAur* m_aur;

    int m_dim[3];

    // Make a copy of the PhoenixFD channel data, because it seems to be
    // de-allocated between calls, despite what the PhoenixFD header says.
    std::vector<std::vector<float>> m_data;

    frantic::graphics::transform4f m_toWorldTM;
    frantic::graphics::transform4f m_fromWorldTM;

    static void copy_channel_data( std::vector<std::vector<float>>& out, IAur* aur, int channel,
                                   const int ( &dim )[3] ) {
        assert( aur );

        if( channel > out.size() ) {
            throw std::runtime_error( "phoenix_field::copy_channel_data Error: "
                                      "channel " +
                                      boost::lexical_cast<std::string>( channel ) +
                                      " "
                                      "is out of range (" +
                                      boost::lexical_cast<std::string>( out.size() ) + ")" );
        }
        std::vector<float>& outData = out[channel];

        const float* data = aur->ExpandChannel( channel );
        if( !data ) {
            throw std::runtime_error( "phoenix_field::copy_channel_data Error: "
                                      "data is NULL for channel " +
                                      boost::lexical_cast<std::string>( channel ) );
        }

        const std::size_t count = dim[0] * dim[1] * dim[2];
        outData.resize( count );
        if( count > 0 ) {
            memcpy( &outData[0], data, count * sizeof( float ) );
        }
    }

  public:
    phoenix_field( INode* pNode, TimeValue t ) {
        m_aur = NULL;

        ObjectState os = pNode->EvalWorldState( t );
        if( os.obj ) {
            if( IPhoenixFD* phx = (IPhoenixFD*)os.obj->GetInterface( PHOENIXFD_INTERFACE ) )
                m_aur = phx->GetSimData( pNode );
        }

        if( !m_aur )
            throw std::runtime_error( "phoenix_field() - Node \"" + frantic::strings::to_string( pNode->GetName() ) +
                                      "\" is not a PhoenixFD node" );

        m_data.resize( NUM_CHANNELS );

        m_aur->GetDim( m_dim );

        m_channelMap.define_channel<float>( _T("Smoke") );
        m_channelMap.define_channel<float>( _T("Fuel") );
        m_channelMap.define_channel<float>( _T("Temperature") );
        m_channelMap.define_channel<frantic::graphics::vector3f>( _T("Velocity") );
        m_channelMap.define_channel<frantic::graphics::vector3f>( _T("TextureCoord") );
        m_channelMap.end_channel_definition( 4u, true, true );

        if( m_aur->ChannelPresent( PHX_T ) ) {
            copy_channel_data( m_data, m_aur, PHX_T, m_dim );
            m_tempAccessor = m_channelMap.get_accessor<float>( _T("Temperature") );
        }

        if( m_aur->ChannelPresent( PHX_SM ) ) {
            copy_channel_data( m_data, m_aur, PHX_SM, m_dim );
            m_smokeAccessor = m_channelMap.get_accessor<float>( _T("Smoke") );
        }

        if( m_aur->ChannelPresent( PHX_Fl ) ) {
            copy_channel_data( m_data, m_aur, PHX_Fl, m_dim );
            m_fuelAccessor = m_channelMap.get_accessor<float>( _T("Fuel") );
        }

        if( m_aur->ChannelPresent( PHX_Vx ) && m_aur->ChannelPresent( PHX_Vy ) && m_aur->ChannelPresent( PHX_Vz ) ) {
            copy_channel_data( m_data, m_aur, PHX_Vx, m_dim );
            copy_channel_data( m_data, m_aur, PHX_Vy, m_dim );
            copy_channel_data( m_data, m_aur, PHX_Vz, m_dim );
            m_velAccessor = m_channelMap.get_accessor<frantic::graphics::vector3f>( _T("Velocity") );
        }

        if( m_aur->ChannelPresent( PHX_U ) && m_aur->ChannelPresent( PHX_V ) && m_aur->ChannelPresent( PHX_W ) ) {
            copy_channel_data( m_data, m_aur, PHX_U, m_dim );
            copy_channel_data( m_data, m_aur, PHX_V, m_dim );
            copy_channel_data( m_data, m_aur, PHX_W, m_dim );
            m_texAccessor = m_channelMap.get_accessor<frantic::graphics::vector3f>( _T("TextureCoord") );
        }

        float rawObjToGridTM[12];
        m_aur->GetObject2GridTransform( rawObjToGridTM );

        const frantic::graphics::transform4f objToGridTM = frantic::graphics::transform4f(
            frantic::graphics::vector3f( rawObjToGridTM[0], rawObjToGridTM[1], rawObjToGridTM[2] ),
            frantic::graphics::vector3f( rawObjToGridTM[3], rawObjToGridTM[4], rawObjToGridTM[5] ),
            frantic::graphics::vector3f( rawObjToGridTM[6], rawObjToGridTM[7], rawObjToGridTM[8] ),
            frantic::graphics::vector3f( rawObjToGridTM[9], rawObjToGridTM[10], rawObjToGridTM[11] ) );

        m_toWorldTM = frantic::max3d::from_max_t( pNode->GetNodeTM( t ) ) * objToGridTM.to_inverse();
        m_fromWorldTM = m_toWorldTM.to_inverse();
    }

    virtual ~phoenix_field() {}

    virtual bool evaluate_field( void* dest, const frantic::graphics::vector3f& pos ) const;

    virtual const frantic::channels::channel_map& get_channel_map() const { return m_channelMap; }
};

namespace {
struct GetSmokeOp {
    typedef float result_type;
    inline static result_type apply( const std::vector<std::vector<float>>& phxData, int voxel ) {
        return phxData[PHX_SM][voxel];
    }
};

struct GetTempOp {
    typedef float result_type;
    inline static result_type apply( const std::vector<std::vector<float>>& phxData, int voxel ) {
        return phxData[PHX_T][voxel];
    }
};

struct GetFuelOp {
    typedef float result_type;
    inline static result_type apply( const std::vector<std::vector<float>>& phxData, int voxel ) {
        return phxData[PHX_Fl][voxel];
    }
};

struct GetVelOp {
    typedef frantic::graphics::vector3f result_type;
    inline static result_type apply( const std::vector<std::vector<float>>& phxData, int voxel ) {
        return frantic::graphics::vector3f( phxData[PHX_Vx][voxel], phxData[PHX_Vy][voxel], phxData[PHX_Vz][voxel] );
    }
};

struct GetTexOp {
    typedef frantic::graphics::vector3f result_type;
    inline static result_type apply( const std::vector<std::vector<float>>& phxData, int voxel ) {
        return frantic::graphics::vector3f( phxData[PHX_U][voxel], phxData[PHX_V][voxel], phxData[PHX_W][voxel] );
    }
};

template <class Op>
typename Op::result_type apply( const std::vector<std::vector<float>>& phxData, const int ( &dim )[3],
                                const float ( &weights )[8], const int ( &voxelCoord )[3], int voxelIndex ) {
    Op::result_type result( 0.f );

    if( static_cast<unsigned>( voxelCoord[2] ) < static_cast<unsigned>( dim[2] ) ) {
        if( static_cast<unsigned>( voxelCoord[1] ) < static_cast<unsigned>( dim[1] ) ) {
            if( static_cast<unsigned>( voxelCoord[0] ) < static_cast<unsigned>( dim[0] ) )
                result += weights[0] * Op::apply( phxData, voxelIndex );
            if( static_cast<unsigned>( voxelCoord[0] + 1 ) < static_cast<unsigned>( dim[0] ) )
                result += weights[1] * Op::apply( phxData, voxelIndex + 1 );
        }
        if( static_cast<unsigned>( voxelCoord[1] + 1 ) < static_cast<unsigned>( dim[1] ) ) {
            if( static_cast<unsigned>( voxelCoord[0] ) < static_cast<unsigned>( dim[0] ) )
                result += weights[2] * Op::apply( phxData, voxelIndex + dim[0] );
            if( static_cast<unsigned>( voxelCoord[0] + 1 ) < static_cast<unsigned>( dim[0] ) )
                result += weights[3] * Op::apply( phxData, voxelIndex + dim[0] + 1 );
        }
    }
    if( static_cast<unsigned>( voxelCoord[2] + 1 ) < static_cast<unsigned>( dim[2] ) ) {
        // Offset 'voxelIndex' to refer to the +Z plane.
        voxelIndex += dim[0] * dim[1];

        if( static_cast<unsigned>( voxelCoord[1] ) < static_cast<unsigned>( dim[1] ) ) {
            if( static_cast<unsigned>( voxelCoord[0] ) < static_cast<unsigned>( dim[0] ) )
                result += weights[4] * Op::apply( phxData, voxelIndex );
            if( static_cast<unsigned>( voxelCoord[0] + 1 ) < static_cast<unsigned>( dim[0] ) )
                result += weights[5] * Op::apply( phxData, voxelIndex + 1 );
        }
        if( static_cast<unsigned>( voxelCoord[1] + 1 ) < static_cast<unsigned>( dim[1] ) ) {
            if( static_cast<unsigned>( voxelCoord[0] ) < static_cast<unsigned>( dim[0] ) )
                result += weights[6] * Op::apply( phxData, voxelIndex + dim[0] );
            if( static_cast<unsigned>( voxelCoord[0] + 1 ) < static_cast<unsigned>( dim[0] ) )
                result += weights[7] * Op::apply( phxData, voxelIndex + dim[0] + 1 );
        }
    }

    return result;
}
} // namespace

bool phoenix_field::evaluate_field( void* dest, const frantic::graphics::vector3f& pos ) const {
    frantic::graphics::vector3f localPos =
        m_fromWorldTM *
        pos; // This transforms into a 'voxel' space where the samples are stored on the integer lattice.

    frantic::graphics::vector3f cornerPos( std::floor( localPos.x ), std::floor( localPos.y ),
                                           std::floor( localPos.z ) );
    frantic::graphics::vector3f alpha( localPos.x - cornerPos.x, localPos.y - cornerPos.y, localPos.z - cornerPos.z );
    int voxelPos[] = { (int)cornerPos.x, (int)cornerPos.y, (int)cornerPos.z };
    int voxel = voxelPos[0] + m_dim[0] * ( voxelPos[1] + m_dim[1] * voxelPos[2] );

    float weights[8];
    frantic::volumetrics::levelset::get_trilerp_weights( &alpha.x, weights );

    memset( dest, 0, m_channelMap.structure_size() );

    if( m_smokeAccessor.is_valid() )
        m_smokeAccessor.get( static_cast<char*>( dest ) ) =
            apply<GetSmokeOp>( m_data, m_dim, weights, voxelPos, voxel );

    if( m_fuelAccessor.is_valid() )
        m_fuelAccessor.get( static_cast<char*>( dest ) ) = apply<GetFuelOp>( m_data, m_dim, weights, voxelPos, voxel );

    if( m_tempAccessor.is_valid() )
        m_tempAccessor.get( static_cast<char*>( dest ) ) = apply<GetTempOp>( m_data, m_dim, weights, voxelPos, voxel );

    if( m_velAccessor.is_valid() )
        m_velAccessor.get( static_cast<char*>( dest ) ) =
            m_toWorldTM.transform_no_translation( apply<GetVelOp>( m_data, m_dim, weights, voxelPos, voxel ) );

    if( m_texAccessor.is_valid() )
        m_texAccessor.get( static_cast<char*>( dest ) ) = apply<GetTexOp>( m_data, m_dim, weights, voxelPos, voxel );

    return true;
}

std::unique_ptr<frantic::volumetrics::field_interface> get_phoenix_field( INode* pNode, TimeValue t ) {
    std::unique_ptr<phoenix_field> result;

    if( is_phoenix_node( pNode, t ) )
        result.reset( new phoenix_field( pNode, t ) );

    return static_cast<std::unique_ptr<frantic::volumetrics::field_interface>>( result );
}

} // namespace volumetrics
} // namespace max3d
} // namespace frantic

#endif
