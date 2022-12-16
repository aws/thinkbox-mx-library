// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file fumefx_field_template.hpp
 *
 * This file implements the general fumefx_field class that can be tweaked on a per-version basis. Anything that needs
 * to be implemented differently across version will be abstracted out and implemented in the template traits class.
 */

#pragma once

#include <frantic/max3d/volumetrics/fumefx_field_factory.hpp>
#include <frantic/max3d/volumetrics/fumefx_io_template.hpp>
#include <frantic/volumetrics/levelset/rle_trilerp.hpp>

#include <boost/bind.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/thread.hpp>

#include <memory>

class VoxelFlowBase; // Forward decl.

namespace frantic {
namespace max3d {
namespace volumetrics {

// This class must be declared in the anonymous namespace in order to reuse the same name (ie. fumefx_field) in multiple
// .cpp files as intended.
namespace {

class fumefx_field : public fumefx_field_interface {
    frantic::graphics::boundbox3f m_bounds;
    frantic::volumetrics::voxel_coord_system m_vcs;
    frantic::channels::channel_map m_channelMap;

    boost::shared_ptr<VoxelFlowBase> m_fumeData;

    int m_shadeReqs;

    frantic::tstring m_fumeDataPath;

    float m_framesPerSec;

    frantic::graphics::transform4f m_toWorldTM;   // Transforms from objectspace to worldspace
    frantic::graphics::transform4f m_fromWorldTM; // Transforms from worldspace to objectspace

    frantic::channels::channel_accessor<float> m_fireAccessor;
    frantic::channels::channel_accessor<float> m_densityAccessor;
    frantic::channels::channel_accessor<float> m_tempAccessor;
    frantic::channels::channel_accessor<frantic::graphics::vector3f> m_texAccessor;
    frantic::channels::channel_accessor<frantic::graphics::vector3f> m_velAccessor;
    frantic::channels::channel_accessor<frantic::graphics::vector3f> m_colorAccessor;
    frantic::channels::channel_accessor<int> m_flagsAccessor;

  public:
    fumefx_field( boost::shared_ptr<VoxelFlowBase> fumeData, const frantic::tstring& fxdPath,
                  const frantic::graphics::transform4f& toWorldTM, int requestedChannels = 0 );

    virtual bool evaluate_field( void* dest, const frantic::graphics::vector3f& pos ) const;

    virtual const frantic::channels::channel_map& get_channel_map() const;

    virtual const frantic::graphics::boundbox3f& get_bounds() const;

    virtual const frantic::volumetrics::voxel_coord_system& get_voxel_coord_sys() const;
};

// NOTE: These methods are not declared 'inline' sine this header should be included only once in the specific
// fumeFX_X_X_field.cpp files.

fumefx_field::fumefx_field( boost::shared_ptr<VoxelFlowBase> fumeData, const frantic::tstring& fxdPath,
                            const frantic::graphics::transform4f& toWorldTM, int requestedChannels )
    : m_fumeData( fumeData )
    , m_fumeDataPath( fxdPath )
    , m_toWorldTM( toWorldTM )
    , m_fromWorldTM( toWorldTM.to_inverse() ) {
    if( requestedChannels == 0 ) {
        requestedChannels = -1;
        requestedChannels &= ~SIM_USEFLAGS; // Mask out channels if we don't want them.
    }

    FumeFXSaveToFileData saveData;
    int loadResult = m_fumeData->LoadOutput( m_fumeDataPath.c_str(), saveData, 0, requestedChannels );
    if( loadResult != LOAD_OK )
        throw std::runtime_error( "fumefx_field::fumefx_field() - Failed to load the FumeFX voxel data from:\n\n\t\"" +
                                  frantic::strings::to_string( m_fumeDataPath ) + "\"" );

    // The channels we actually have access to are the intersection of `loadedOutputVars` and the requested channels.
    int actualChannels = requestedChannels & m_fumeData->loadedOutputVars;

    m_vcs.set_voxel_length( m_fumeData->dx );
    m_bounds.set( frantic::graphics::vector3f( m_fumeData->lx0, m_fumeData->ly0, m_fumeData->lz0 ),
                  frantic::graphics::vector3f( m_fumeData->lx0 + m_fumeData->lx, m_fumeData->ly0 + m_fumeData->ly,
                                               m_fumeData->lz0 + m_fumeData->lz ) );

    m_bounds = toWorldTM * m_bounds;

    m_framesPerSec = static_cast<float>( GetFrameRate() );

    m_shadeReqs = 0;

    // Define all channels so that the ordering is consistent if the available channels change
    m_channelMap.define_channel<float>( _T("Smoke") );
    m_channelMap.define_channel<float>( _T("Fire") );
    m_channelMap.define_channel<float>( _T("Temperature") );
    m_channelMap.define_channel<frantic::graphics::vector3f>( _T("Velocity") );
    m_channelMap.define_channel<frantic::graphics::vector3f>( _T("TextureCoord") );
#ifdef THINKBOX_FUMEFX_HAS_COLOR
    m_channelMap.define_channel<frantic::graphics::vector3f>( _T("Color") );
#endif
    m_channelMap.end_channel_definition( 4u, true, true );

    // if( requestedChannels & SIM_USEFLAGS )
    //	m_channelMap.append_channel<int>( _T("Flags") );

    if( actualChannels & SIM_USEFUEL ) {
        m_fireAccessor = m_channelMap.get_accessor<float>( _T("Fire") );
        m_shadeReqs |= FFXSHADER_REQ_FIRE;
    }

    if( actualChannels & SIM_USEDENS ) {
        m_densityAccessor = m_channelMap.get_accessor<float>( _T("Smoke") );
        m_shadeReqs |= FFXSHADER_REQ_DENS;
    }

    if( actualChannels & SIM_USETEMP ) {
        m_tempAccessor = m_channelMap.get_accessor<float>( _T("Temperature") );
        m_shadeReqs |= FFXSHADER_REQ_TEMP;
    }

    if( actualChannels & SIM_USETEXT ) {
        m_texAccessor = m_channelMap.get_accessor<frantic::graphics::vector3f>( _T("TextureCoord") );
        m_shadeReqs |= FFXSHADER_REQ_TEX;
    }

    if( actualChannels & SIM_USEVEL ) {
        m_velAccessor = m_channelMap.get_accessor<frantic::graphics::vector3f>( _T("Velocity") );
        m_shadeReqs |= FFXSHADER_REQ_VEL;
    }

#ifdef THINKBOX_FUMEFX_HAS_COLOR
    if( actualChannels & SIM_USECOLOR ) {
        m_colorAccessor = m_channelMap.get_accessor<frantic::graphics::vector3f>( _T("Color") );
        m_shadeReqs |= FFXSHADER_REQ_COLOR;
    }
#endif
}

namespace {
struct GetRoOp {
    typedef float result_type;
    inline static result_type apply( VoxelFlowBase* fumeData, int voxel ) { return fumeData->GetRo2( voxel ); }
};

struct GetFuelOp {
    typedef float result_type;
    inline static result_type apply( VoxelFlowBase* fumeData, int voxel ) { return fumeData->GetFuel2( voxel ); }
};

struct GetTempOp {
    typedef float result_type;
    inline static result_type apply( VoxelFlowBase* fumeData, int voxel ) { return fumeData->GetTemp2( voxel ); }
};

struct GetVelOp {
    typedef frantic::graphics::vector3f result_type;
    inline static result_type apply( VoxelFlowBase* fumeData, int voxel ) {
        frantic::graphics::vector3f v;

        fumeData->GetVel2( voxel, v.x, v.y, v.z );

        return v;
    }
};

struct GetTexOp {
    typedef frantic::graphics::vector3f result_type;
    inline static result_type apply( VoxelFlowBase* fumeData, int voxel ) {
        frantic::graphics::vector3f v;

        fumeData->GetXYZ2( voxel, v.x, v.y, v.z );

        return v;
    }
};

#ifdef THINKBOX_FUMEFX_HAS_COLOR
struct GetColorOp {
    typedef frantic::graphics::vector3f result_type;
    inline static result_type apply( VoxelFlowBase* fumeData, int voxel ) {
        SDColor c;

        fumeData->GetColor2( voxel, c );

        return frantic::graphics::vector3f( c.r, c.g, c.b );
    }
};
#endif

template <class Op>
typename Op::result_type apply( VoxelFlowBase* fumeData, const float ( &weights )[8], const int ( &voxelCoord )[3],
                                int voxelIndex ) {
    Op::result_type result( 0.f );

    if( static_cast<unsigned>( voxelCoord[0] ) < static_cast<unsigned>( fumeData->nx ) ) {
        if( static_cast<unsigned>( voxelCoord[1] ) < static_cast<unsigned>( fumeData->ny ) ) {
            if( static_cast<unsigned>( voxelCoord[2] ) < static_cast<unsigned>( fumeData->nz ) )
                result += weights[0] * Op::apply( fumeData, voxelIndex );
            if( static_cast<unsigned>( voxelCoord[2] + 1 ) < static_cast<unsigned>( fumeData->nz ) )
                result += weights[4] * Op::apply( fumeData, voxelIndex + 1 );
        }
        if( static_cast<unsigned>( voxelCoord[1] + 1 ) < static_cast<unsigned>( fumeData->ny ) ) {
            if( static_cast<unsigned>( voxelCoord[2] ) < static_cast<unsigned>( fumeData->nz ) )
                result += weights[2] * Op::apply( fumeData, voxelIndex + fumeData->nz );
            if( static_cast<unsigned>( voxelCoord[2] + 1 ) < static_cast<unsigned>( fumeData->nz ) )
                result += weights[6] * Op::apply( fumeData, voxelIndex + fumeData->nz + 1 );
        }
    }
    if( static_cast<unsigned>( voxelCoord[0] + 1 ) < static_cast<unsigned>( fumeData->nx ) ) {
        if( static_cast<unsigned>( voxelCoord[1] ) < static_cast<unsigned>( fumeData->ny ) ) {
            if( static_cast<unsigned>( voxelCoord[2] ) < static_cast<unsigned>( fumeData->nz ) )
                result += weights[1] * Op::apply( fumeData, voxelIndex + fumeData->nyz );
            if( static_cast<unsigned>( voxelCoord[2] + 1 ) < static_cast<unsigned>( fumeData->nz ) )
                result += weights[5] * Op::apply( fumeData, voxelIndex + fumeData->nyz + 1 );
        }
        if( static_cast<unsigned>( voxelCoord[1] + 1 ) < static_cast<unsigned>( fumeData->ny ) ) {
            if( static_cast<unsigned>( voxelCoord[2] ) < static_cast<unsigned>( fumeData->nz ) )
                result += weights[3] * Op::apply( fumeData, voxelIndex + fumeData->nyz + fumeData->nz );
            if( static_cast<unsigned>( voxelCoord[2] + 1 ) < static_cast<unsigned>( fumeData->nz ) )
                result += weights[7] * Op::apply( fumeData, voxelIndex + fumeData->nyz + fumeData->nz + 1 );
        }
    }

    return result;
}
} // namespace

bool fumefx_field::evaluate_field( void* dest, const frantic::graphics::vector3f& pos ) const {
    frantic::graphics::vector3f objPos = m_fromWorldTM * pos;

    frantic::graphics::vector3f localPos( ( objPos.x + m_fumeData->midx ) * m_fumeData->idx,
                                          ( objPos.y + m_fumeData->midy ) * m_fumeData->idx,
                                          objPos.z * m_fumeData->idx );

    frantic::graphics::vector3f cornerPos( std::floor( localPos.x ), std::floor( localPos.y ),
                                           std::floor( localPos.z ) );
    frantic::graphics::vector3f alpha( localPos.x - cornerPos.x, localPos.y - cornerPos.y, localPos.z - cornerPos.z );
    int voxelPos[] = { (int)cornerPos.x - m_fumeData->nx0, (int)cornerPos.y - m_fumeData->ny0,
                       (int)cornerPos.z - m_fumeData->nz0 };
    int voxel = voxelPos[0] * m_fumeData->nyz + voxelPos[1] * m_fumeData->nz + voxelPos[2];

    float weights[8];
    frantic::volumetrics::levelset::get_trilerp_weights( &alpha.x, weights );

    memset( dest, 0, m_channelMap.structure_size() );

    if( m_fireAccessor.is_valid() )
        m_fireAccessor.get( static_cast<char*>( dest ) ) =
            std::max( 0.f, apply<GetFuelOp>( m_fumeData.get(), weights, voxelPos, voxel ) );

    if( m_densityAccessor.is_valid() )
        m_densityAccessor.get( static_cast<char*>( dest ) ) =
            apply<GetRoOp>( m_fumeData.get(), weights, voxelPos, voxel );

    if( m_tempAccessor.is_valid() )
        m_tempAccessor.get( static_cast<char*>( dest ) ) =
            apply<GetTempOp>( m_fumeData.get(), weights, voxelPos, voxel );

    if( m_velAccessor.is_valid() )
        m_velAccessor.get( static_cast<char*>( dest ) ) = m_toWorldTM.transform_no_translation(
            m_framesPerSec * apply<GetVelOp>( m_fumeData.get(), weights, voxelPos, voxel ) );

    if( m_texAccessor.is_valid() )
        m_texAccessor.get( static_cast<char*>( dest ) ) = apply<GetTexOp>( m_fumeData.get(), weights, voxelPos, voxel );

#ifdef THINKBOX_FUMEFX_HAS_COLOR
    if( m_colorAccessor.is_valid() )
        m_colorAccessor.get( static_cast<char*>( dest ) ) =
            apply<GetColorOp>( m_fumeData.get(), weights, voxelPos, voxel );
#endif

    return true;
}

const frantic::channels::channel_map& fumefx_field::get_channel_map() const { return m_channelMap; }

const frantic::graphics::boundbox3f& fumefx_field::get_bounds() const { return m_bounds; }

const frantic::volumetrics::voxel_coord_system& fumefx_field::get_voxel_coord_sys() const { return m_vcs; }

/**
 * \pre FumeFXTraits::init() Must have been called.
 * \tparam FumeFXTraits A traits class implementing:
 *                       VoxelFlowBase* CreateVoxelFlow();
 *                       void DeleteVoxelFlow( VoxelFlowBase* );
 * \param fxdPath Path to the .fxd file to load the data from
 * \param toWorldTM Transform matrix from object to world space.
 * \param channelsRequested A bitwise combination of the fumefx_channels::option enumeration values. If 0, all channels
 * are loaded. \return A new, unique instance of a fumefx_field_interface subclass that exposes the voxel data of the
 * FumeFX simulation.
 */
template <class FumeFXTraits>
inline std::unique_ptr<fumefx_field_interface> get_fumefx_field_impl( const frantic::tstring& fxdPath,
                                                                      const frantic::graphics::transform4f& toWorldTM,
                                                                      int channelsRequested ) {
    boost::shared_ptr<VoxelFlowBase> fumeData = GetVoxelFlow<FumeFXTraits>( fxdPath );

    std::unique_ptr<fumefx_field_interface> result(
        new fumefx_field( fumeData, fxdPath, toWorldTM, channelsRequested ) );

    return result;
}

/**
 * \pre FumeFXTraits::init() Must have been called.
 * \tparam FumeFXTraits A traits class implementing:
 *                       VoxelFlowBase* CreateVoxelFlow();
 *                       void DeleteVoxelFlow( VoxelFlowBase* );
 *                       frantic::tstring GetDataPath( INode* pNode, TimeValue t );
 * \param pNode The node containing a FumeFX simulation object
 * \param t The time to evaluate the simulation
 * \param channelsRequested A bitwise combination of the fumefx_channels::option enumeration values. If 0, all channels
 * are loaded. \return A new, unique instance of a fumefx_field_interface subclass that exposes the voxel data of the
 * FumeFX simulation.
 */
template <class FumeFXTraits>
inline std::unique_ptr<fumefx_field_interface> get_fumefx_field_impl( INode* pNode, TimeValue t,
                                                                      int channelsRequested ) {
    frantic::tstring fxdPath = FumeFXTraits::GetDataPath( pNode, t );

    boost::shared_ptr<VoxelFlowBase> fumeData;

    if( !fxdPath.empty() )
        fumeData = GetVoxelFlow<FumeFXTraits>( fxdPath, false );

    if( !fumeData )
        return std::unique_ptr<fumefx_field_interface>( new empty_fumefx_field );

    frantic::graphics::transform4f toWorldTM = frantic::max3d::from_max_t( pNode->GetNodeTM( t ) );

    std::unique_ptr<fumefx_field_interface> result(
        new fumefx_field( fumeData, fxdPath, toWorldTM, channelsRequested ) );

    return result;
}

namespace {
boost::shared_ptr<frantic::volumetrics::field_interface>
load_fumefx_field( const boost::shared_ptr<VoxelFlowBase>& fumeData, const frantic::tstring& fxdPath,
                   const frantic::graphics::transform4f& toWorldTM, int channelsRequested ) {
    return boost::shared_ptr<frantic::volumetrics::field_interface>(
        new fumefx_field( fumeData, fxdPath, toWorldTM, channelsRequested ) );
}

const frantic::channels::channel_map& get_native_channel_map() {
    static frantic::channels::channel_map theMap;
    if( !theMap.channel_definition_complete() ) {
        theMap.define_channel<float>( _T("Smoke") );
        theMap.define_channel<float>( _T("Fire") );
        theMap.define_channel<float>( _T("Temperature") );
        theMap.define_channel<frantic::graphics::vector3f>( _T("Velocity") );
        theMap.define_channel<frantic::graphics::vector3f>( _T("TextureCoord") );
#ifdef THINKBOX_FUMEFX_HAS_COLOR
        theMap.define_channel<frantic::graphics::vector3f>( _T("Color") );
#endif
        theMap.end_channel_definition( 4u, true, true );
    }

    return theMap;
}

void set_channel_map_from_mask( frantic::channels::channel_map& theMap, int channelMask ) {
    theMap = get_native_channel_map();

    if( channelMask == 0 )
        return;
    if( !( channelMask & SIM_USEDENS ) )
        theMap.delete_channel( _T("Smoke"), true );
    if( !( channelMask & SIM_USEFUEL ) )
        theMap.delete_channel( _T("Fire"), true );
    if( !( channelMask & SIM_USETEMP ) )
        theMap.delete_channel( _T("Temperature"), true );
    if( !( channelMask & SIM_USEVEL ) )
        theMap.delete_channel( _T("Velocity"), true );
    if( !( channelMask & SIM_USETEXT ) )
        theMap.delete_channel( _T("TextureCoord"), true );
#ifdef THINKBOX_FUMEFX_HAS_COLOR
    if( !( channelMask & SIM_USECOLOR ) )
        theMap.delete_channel( _T("Color"), true );
#endif
}

void init_metadata( fumefx_fxd_metadata& outMetadata, const VoxelFlowBase& fumeData, int channelsRequested ) {
    outMetadata.dx = fumeData.dx;

    outMetadata.simBounds[0] = ( 0.5f ) * fumeData.dx - fumeData.lxmax / 2;
    outMetadata.simBounds[1] = ( 0.5f ) * fumeData.dx - fumeData.lymax / 2;
    outMetadata.simBounds[2] = ( 0.5f ) * fumeData.dx;
    outMetadata.simBounds[3] = ( static_cast<float>( fumeData.nxmax ) + 0.5f ) * fumeData.dx - fumeData.lxmax / 2;
    outMetadata.simBounds[4] = ( static_cast<float>( fumeData.nymax ) + 0.5f ) * fumeData.dx - fumeData.lymax / 2;
    outMetadata.simBounds[5] = ( static_cast<float>( fumeData.nzmax ) + 0.5f ) * fumeData.dx;

    outMetadata.dataBounds[0] = ( static_cast<float>( fumeData.nx0 ) + 0.5f ) * fumeData.dx - fumeData.lxmax / 2;
    outMetadata.dataBounds[1] = ( static_cast<float>( fumeData.ny0 ) + 0.5f ) * fumeData.dx - fumeData.lymax / 2;
    outMetadata.dataBounds[2] = ( static_cast<float>( fumeData.nz0 ) + 0.5f ) * fumeData.dx;
    outMetadata.dataBounds[3] =
        ( static_cast<float>( fumeData.nx0 + fumeData.nx ) + 0.5f ) * fumeData.dx - fumeData.lxmax / 2;
    outMetadata.dataBounds[4] =
        ( static_cast<float>( fumeData.ny0 + fumeData.ny ) + 0.5f ) * fumeData.dx - fumeData.lymax / 2;
    outMetadata.dataBounds[5] = ( static_cast<float>( fumeData.nz0 + fumeData.nz ) + 0.5f ) * fumeData.dx;

    int channels = fumeData.loadedOutputVars;
    if( channelsRequested != 0 )
        channels &= channelsRequested;

    set_channel_map_from_mask( outMetadata.fileChannels, channels );

    outMetadata.memUsage = 0;
    for( std::size_t i = 0, iEnd = outMetadata.fileChannels.channel_count(); i < iEnd; ++i )
        outMetadata.memUsage += outMetadata.fileChannels[i].primitive_size() *
                                static_cast<std::size_t>( fumeData.nx * fumeData.ny * fumeData.nz );
}
} // namespace

template <class FumeFXTraits>
inline boost::shared_future<boost::shared_ptr<frantic::volumetrics::field_interface>>
get_fumefx_field_async_impl( const frantic::tstring& fxdPath, const frantic::graphics::transform4f& toWorldTM,
                             int channelsRequested, fumefx_fxd_metadata& outMetadata ) {
    boost::shared_ptr<VoxelFlowBase> fumeData = GetVoxelFlow<FumeFXTraits>( fxdPath );

    init_metadata( outMetadata, *fumeData, channelsRequested );

    boost::packaged_task<boost::shared_ptr<frantic::volumetrics::field_interface>> task(
        boost::bind( &load_fumefx_field, fumeData, fxdPath, toWorldTM, channelsRequested ) );

    boost::shared_future<boost::shared_ptr<frantic::volumetrics::field_interface>> result = task.get_future();

    // Spawn a new thread to load the file.
    boost::thread::thread( boost::move( task ) ).detach();

    return result;
}

template <class FumeFXTraits>
inline boost::shared_future<boost::shared_ptr<frantic::volumetrics::field_interface>>
get_fumefx_field_async_impl( INode* pNode, TimeValue t, int channelsRequested, fumefx_fxd_metadata& outMetadata ) {
    frantic::tstring fxdPath = FumeFXTraits::GetDataPath( pNode, t );

    boost::shared_ptr<VoxelFlowBase> fumeData;

    if( !fxdPath.empty() )
        fumeData = GetVoxelFlow<FumeFXTraits>( fxdPath, false );

    if( !fumeData ) {
        outMetadata.dx = 1.f;
        outMetadata.simBounds[0] = outMetadata.simBounds[1] = outMetadata.simBounds[2] =
            std::numeric_limits<float>::max();
        outMetadata.simBounds[3] = outMetadata.simBounds[4] = outMetadata.simBounds[5] =
            std::numeric_limits<float>::min();
        outMetadata.dataBounds[0] = outMetadata.dataBounds[1] = outMetadata.dataBounds[2] =
            std::numeric_limits<float>::max();
        outMetadata.dataBounds[3] = outMetadata.dataBounds[4] = outMetadata.dataBounds[5] =
            std::numeric_limits<float>::min();
        outMetadata.fileChannels.reset();
        outMetadata.fileChannels.end_channel_definition();
        set_channel_map_from_mask( outMetadata.fileChannels, 0 );

        outMetadata.memUsage = 0;

        return boost::make_shared_future(
            boost::shared_ptr<frantic::volumetrics::field_interface>( new empty_fumefx_field ) );
    }

    init_metadata( outMetadata, *fumeData, channelsRequested );

    frantic::graphics::transform4f toWorldTM = frantic::max3d::from_max_t( pNode->GetNodeTM( t ) );

    boost::packaged_task<boost::shared_ptr<frantic::volumetrics::field_interface>> task(
        boost::bind( &load_fumefx_field, fumeData, fxdPath, toWorldTM, channelsRequested ) );

    boost::shared_future<boost::shared_ptr<frantic::volumetrics::field_interface>> result = task.get_future();

    // Spawn a new thread to load the file.
    boost::thread::thread( boost::move( task ) ).detach();

    return result;
}

template <class FumeFXTraits>
inline void write_fxd_file_impl( const frantic::tstring& path,
                                 const boost::shared_ptr<frantic::volumetrics::field_interface>& pField,
                                 const frantic::graphics::boundbox3f& simWSBounds,
                                 const frantic::graphics::boundbox3f& curWSBounds, float spacing,
                                 const frantic::channels::channel_map* pOverrideChannels ) {
    if( !pField )
        return;

    if( !pOverrideChannels )
        pOverrideChannels = &pField->get_channel_map();

    int simBounds[] = { static_cast<int>( std::ceil( simWSBounds.minimum().x / spacing - 0.5f ) ),
                        static_cast<int>( std::floor( simWSBounds.maximum().x / spacing - 0.5f ) ) + 1,
                        static_cast<int>( std::ceil( simWSBounds.minimum().y / spacing - 0.5f ) ),
                        static_cast<int>( std::floor( simWSBounds.maximum().y / spacing - 0.5f ) ) + 1,
                        static_cast<int>( std::ceil( simWSBounds.minimum().z / spacing - 0.5f ) ),
                        static_cast<int>( std::floor( simWSBounds.maximum().z / spacing - 0.5f ) ) + 1 };

    int voxelBounds[] = { static_cast<int>( std::ceil( curWSBounds.minimum().x / spacing - 0.5f ) ),
                          static_cast<int>( std::floor( curWSBounds.maximum().x / spacing - 0.5f ) ) + 1,
                          static_cast<int>( std::ceil( curWSBounds.minimum().y / spacing - 0.5f ) ),
                          static_cast<int>( std::floor( curWSBounds.maximum().y / spacing - 0.5f ) ) + 1,
                          static_cast<int>( std::ceil( curWSBounds.minimum().z / spacing - 0.5f ) ),
                          static_cast<int>( std::floor( curWSBounds.maximum().z / spacing - 0.5f ) ) + 1 };

    if( voxelBounds[0] < simBounds[0] )
        voxelBounds[0] = simBounds[0];
    if( voxelBounds[1] > simBounds[1] )
        voxelBounds[1] = simBounds[1];
    if( voxelBounds[2] < simBounds[2] )
        voxelBounds[2] = simBounds[2];
    if( voxelBounds[3] > simBounds[3] )
        voxelBounds[3] = simBounds[3];
    if( voxelBounds[4] < simBounds[4] )
        voxelBounds[4] = simBounds[4];
    if( voxelBounds[5] > simBounds[5] )
        voxelBounds[5] = simBounds[5];

    int size[] = { voxelBounds[1] - voxelBounds[0], voxelBounds[3] - voxelBounds[2], voxelBounds[5] - voxelBounds[4] };

    boost::shared_ptr<VoxelFlowBase> fumeData = CreateEmptyVoxelFlow<FumeFXTraits>();

    fumeData->nx0 = voxelBounds[0] - simBounds[0];
    fumeData->nxmax = simBounds[1] - simBounds[0];
    fumeData->lx0 = std::max( simWSBounds.minimum().x, curWSBounds.minimum().x );

    fumeData->ny0 = voxelBounds[2] - simBounds[2];
    fumeData->nymax = simBounds[3] - simBounds[2];
    fumeData->ly0 = std::max( simWSBounds.minimum().y, curWSBounds.minimum().y );

    fumeData->nz0 = voxelBounds[4] - simBounds[4];
    fumeData->nzmax = simBounds[5] - simBounds[4];
    fumeData->lz0 = std::max( simWSBounds.minimum().z, curWSBounds.minimum().z );

    frantic::channels::channel_cvt_accessor<float> densityAccessor;
    frantic::channels::channel_cvt_accessor<frantic::graphics::vector3f> velocityAccessor;

    int outputVars = 0;
    if( pOverrideChannels->has_channel( _T("Smoke") ) ) {
        outputVars |= SIM_USEDENS;
        densityAccessor = pOverrideChannels->get_cvt_accessor<float>( _T("Smoke") );
    } else if( pOverrideChannels->has_channel( _T("Density") ) ) {
        outputVars |= SIM_USEDENS;
        densityAccessor = pOverrideChannels->get_cvt_accessor<float>( _T("Density") );
    }

    if( pOverrideChannels->has_channel( _T("Velocity") ) ) {
        outputVars |= SIM_USEVEL;
        velocityAccessor = pOverrideChannels->get_cvt_accessor<frantic::graphics::vector3f>( _T("Velocity") );
    }

    fumeData->InitForOutput( size[0], size[1], size[2], static_cast<float>( size[0] ) * spacing,
                             static_cast<float>( size[1] ) * spacing, static_cast<float>( size[2] ) * spacing, spacing,
                             outputVars );

    char* buffer = static_cast<char*>( alloca( pField->get_channel_map().structure_size() ) );

    int voxel = 0;

    frantic::graphics::vector3f p;
    for( int x = 0; x < size[2]; ++x ) {
        p.x = ( static_cast<float>( x + fumeData->nx0 ) + 0.5f ) * fumeData->dx - fumeData->midx;
        for( int y = 0; y < size[2]; ++y ) {
            p.y = ( static_cast<float>( y + fumeData->ny0 ) + 0.5f ) * fumeData->dx - fumeData->midy;
            for( int z = 0; z < size[2]; ++z, ++voxel ) {
                p.z = ( static_cast<float>( z + fumeData->nz0 ) + 0.5f ) * fumeData->dx;

                if( pField->evaluate_field( buffer, p ) ) {
                    if( outputVars & SIM_USEDENS ) {
                        fumeData->SetRo2( voxel, densityAccessor.get( buffer ) );
                    }

                    if( outputVars & SIM_USEVEL ) {
                        frantic::graphics::vector3f v = velocityAccessor.get( buffer );
                        fumeData->SetVel2( voxel, v.x, v.y, v.z );
                    }
                } else {
                    if( outputVars & SIM_USEDENS ) {
                        fumeData->SetRo2( voxel, 0.f );
                    }

                    if( outputVars & SIM_USEVEL ) {
                        fumeData->SetVel2( voxel, 0.f, 0.f, 0.f );
                    }
                }
            }
        }
    }

    TCHAR filename[MAX_PATH];
    filename[path.copy( filename, MAX_PATH - 1 )] = _T( '\0' );

    FumeFXSaveToFileData ffd;
    ffd.scale = 1.f;
    ffd.type = UNITS_METERS;
    ffd.tm.IdentityMatrix();

    long long result = fumeData->SaveOutput( filename, outputVars, ffd );
    std::cout << result;
}

} // namespace

} // namespace volumetrics
} // namespace max3d
} // namespace frantic
