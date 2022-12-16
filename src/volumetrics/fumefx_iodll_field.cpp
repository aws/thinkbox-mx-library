// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#if defined( FUMEFX_SDK_AVAILABLE )

class shdParams_Standard;
class vfShader;
class vfLightSampler;
class vfMapSampler;

#include <fumefxio/VoxelFlowBase.h>

#include <maxversion.h>

#define THINKBOX_FUMEFX_HAS_COLOR
#define THINKBOX_FUMEFX_TCHAR_HACK
#include <frantic/max3d/volumetrics/fumefx_field_factory.hpp>
#include <frantic/max3d/volumetrics/fumefx_field_template.hpp>
#include <frantic/max3d/volumetrics/fumefx_io_template.hpp>
#include <frantic/max3d/volumetrics/fumefx_source_particle_istream_template.hpp>
#undef THINKBOX_FUMEFX_HAS_COLOR
#undef THINKBOX_FUMEFX_TCHAR_HACK

#include <frantic/max3d/maxscript/maxscript.hpp>

#include <frantic/particles/streams/empty_particle_istream.hpp>
#include <frantic/win32/utility.hpp>

#include <memory>

namespace frantic {
namespace max3d {
namespace volumetrics {

extern boost::int64_t get_fumefx_version();

namespace {

class FumeFXIO_ImplementationDetails {
  public:
    static VoxelFlowBase* CreateVoxelFlow() { return ::CreateVoxelFlow(); }

    static void DeleteVoxelFlow( VoxelFlowBase* fumeData ) { ::DeleteVoxelFlow( fumeData ); }

    static bool MakeOutputName( frantic::tstring& outFilePath, const frantic::tstring& sequencePath, int curFrame ) {
        TCHAR outPath[MAX_PATH];

        BOOL result = ::vfMakeOutputName( outPath, const_cast<TCHAR*>( sequencePath.c_str() ), curFrame, -1 );

        // Set the NULL here, just in case.
        outPath[MAX_PATH - 1] = '\0';

        outFilePath = frantic::strings::to_tstring( outPath );

        return result != FALSE;
    }

    /**
     * \param pFumeNode The scene node that contains a FumeFX simulation object
     * \param t The scene time to get data for.
     * \return A path to the .fxd file that stores the data for the fume sim at the specified time.
     */
    static frantic::tstring GetDataPath( INode* pFumeNode, TimeValue t ) {
        int frameOffset =
            frantic::max3d::mxs::expression( _T("fumeNode.Offset") ).bind( _T("fumeNode"), pFumeNode ).evaluate<int>();
        int startFrame = frantic::max3d::mxs::expression( _T("fumeNode.playFrom") )
                             .bind( _T("fumeNode"), pFumeNode )
                             .evaluate<int>();
        int endFrame =
            frantic::max3d::mxs::expression( _T("fumeNode.playTo") ).bind( _T("fumeNode"), pFumeNode ).evaluate<int>();

        frantic::tstring simPath;

        if( get_fumefx_version() >= 0x0002000100000000 ) {
            static const frantic::tchar* CACHE_TYPES[] = { _T("default"), _T("wavelet"), _T("retimer"), _T("preview") };

            int cacheType = frantic::max3d::mxs::expression( _T("fumeNode.selectedCache") )
                                .bind( _T("fumeNode"), pFumeNode )
                                .evaluate<int>();

            if( cacheType < 0 || cacheType > 2 )
                throw std::runtime_error(
                    "Unexpected FumeFX cache type: " + boost::lexical_cast<std::string>( cacheType ) + " for node " +
                    frantic::strings::to_string( pFumeNode->GetName() ) );

            simPath = frantic::max3d::mxs::expression( _T("fumeNode.GetPath \"") +
                                                       frantic::strings::tstring( CACHE_TYPES[cacheType] ) + _T("\"") )
                          .bind( _T("fumeNode"), pFumeNode )
                          .evaluate<frantic::tstring>();
        } else {
            simPath = frantic::max3d::mxs::expression( _T("fumeNode.GetPath()") )
                          .bind( _T("fumeNode"), pFumeNode )
                          .evaluate<frantic::tstring>();
        }

        // The frameOffset changes the apparent Max frame time.
        // The startFrame is the first frame to load when the maxTime is 'frameOffset'
        // The endFrame is the last frame to load when maxTime is 'frameOffset' + endFrame - startFrame
        int curFrame = t / GetTicksPerFrame();
        if( curFrame < frameOffset )
            return frantic::tstring();

        curFrame = ( curFrame - frameOffset ) + startFrame;
        if( curFrame > endFrame )
            return frantic::tstring();

        frantic::tstring fxdPath;

        MakeOutputName( fxdPath, simPath, curFrame );

        return fxdPath;
    }
};

} // namespace

class fumefx_iodll_factory : public fumefx_factory_interface {
  public:
    fumefx_iodll_factory() {}

    virtual ~fumefx_iodll_factory() {}

    virtual std::unique_ptr<fumefx_field_interface> get_fumefx_field( const frantic::tstring& fxdPath,
                                                                      const frantic::graphics::transform4f& toWorldTM,
                                                                      int channelsRequested ) const {
        return get_fumefx_field_impl<FumeFXIO_ImplementationDetails>( fxdPath, toWorldTM, channelsRequested );
    }

    virtual std::unique_ptr<fumefx_field_interface> get_fumefx_field( INode* pNode, TimeValue t,
                                                                      int channelsRequested ) const {
        return get_fumefx_field_impl<FumeFXIO_ImplementationDetails>( pNode, t, channelsRequested );
    }

    virtual boost::shared_future<boost::shared_ptr<frantic::volumetrics::field_interface>>
    get_fumefx_field_async( const frantic::tstring& fxdPath, const frantic::graphics::transform4f& toWorldTM,
                            int channelsRequested, fumefx_fxd_metadata& outMetadata ) const {
        return get_fumefx_field_async_impl<FumeFXIO_ImplementationDetails>( fxdPath, toWorldTM, channelsRequested,
                                                                            outMetadata );
    }

    virtual boost::shared_future<boost::shared_ptr<frantic::volumetrics::field_interface>>
    get_fumefx_field_async( INode* pNode, TimeValue t, int channelsRequested, fumefx_fxd_metadata& outMetadata ) const {
        return get_fumefx_field_async_impl<FumeFXIO_ImplementationDetails>( pNode, t, channelsRequested, outMetadata );
    }

    virtual boost::shared_ptr<fumefx_source_particle_istream>
    get_fumefx_source_particle_istream( INode* pNode, TimeValue t,
                                        const frantic::channels::channel_map& requestedChannels ) const {
        return get_fumefx_source_particle_istream_impl<FumeFXIO_ImplementationDetails>( pNode, t, requestedChannels );
    }

    virtual void write_fxd_file( const frantic::tstring& path,
                                 const boost::shared_ptr<frantic::volumetrics::field_interface>& pField,
                                 const frantic::graphics::boundbox3f& simWSBounds,
                                 const frantic::graphics::boundbox3f& curWSBounds, float spacing,
                                 const frantic::channels::channel_map* pOverrideChannels ) const {
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

        int size[] = { voxelBounds[1] - voxelBounds[0], voxelBounds[3] - voxelBounds[2],
                       voxelBounds[5] - voxelBounds[4] };

        boost::shared_ptr<VoxelFlowBase> fumeData = CreateEmptyVoxelFlow<FumeFXIO_ImplementationDetails>();

        fumeData->Reset();

        fumeData->nx0 = voxelBounds[0] - simBounds[0];
        fumeData->nxmax = simBounds[1] - simBounds[0];
        fumeData->lx0 = spacing * static_cast<float>( fumeData->nx0 );

        fumeData->ny0 = voxelBounds[2] - simBounds[2];
        fumeData->nymax = simBounds[3] - simBounds[2];
        fumeData->ly0 = spacing * static_cast<float>( fumeData->ny0 );

        fumeData->nz0 = voxelBounds[4] - simBounds[4];
        fumeData->nzmax = simBounds[5] - simBounds[4];
        fumeData->lz0 = spacing * static_cast<float>( fumeData->nz0 );

        frantic::channels::channel_cvt_accessor<float> densityAccessor;
        frantic::channels::channel_cvt_accessor<float> fireAccessor;
        frantic::channels::channel_cvt_accessor<float> temperatureAccessor;
        frantic::channels::channel_cvt_accessor<frantic::graphics::vector3f> velocityAccessor;
        frantic::channels::channel_cvt_accessor<frantic::graphics::vector3f> textureCoordAccessor;
        frantic::channels::channel_cvt_accessor<frantic::graphics::color3f> colorAccessor;

        int outputVars = 0;
        if( pOverrideChannels->has_channel( _T("Smoke") ) ) {
            outputVars |= SIM_USEDENS;
            densityAccessor = pOverrideChannels->get_cvt_accessor<float>( _T("Smoke") );
        } else if( pOverrideChannels->has_channel( _T("Density") ) ) {
            outputVars |= SIM_USEDENS;
            densityAccessor = pOverrideChannels->get_cvt_accessor<float>( _T("Density") );
        }

        if( pOverrideChannels->has_channel( _T("Fire") ) ) {
            outputVars |= SIM_USEFUEL;
            fireAccessor = pOverrideChannels->get_cvt_accessor<float>( _T("Fire") );
        }

        if( pOverrideChannels->has_channel( _T("Temperature") ) ) {
            outputVars |= SIM_USETEMP;
            temperatureAccessor = pOverrideChannels->get_cvt_accessor<float>( _T("Temperature") );
        }

        if( pOverrideChannels->has_channel( _T("Velocity") ) ) {
            outputVars |= SIM_USEVEL;
            velocityAccessor = pOverrideChannels->get_cvt_accessor<frantic::graphics::vector3f>( _T("Velocity") );
        }

        if( pOverrideChannels->has_channel( _T("TextureCoord") ) ) {
            outputVars |= SIM_USETEXT;
            textureCoordAccessor =
                pOverrideChannels->get_cvt_accessor<frantic::graphics::vector3f>( _T("TextureCoord") );
        }

        if( pOverrideChannels->has_channel( _T("Color") ) ) {
            outputVars |= SIM_USECOLOR;
            colorAccessor = pOverrideChannels->get_cvt_accessor<frantic::graphics::color3f>( _T("Color") );
        }

        fumeData->InitForOutput( size[0], size[1], size[2], static_cast<float>( size[0] ) * spacing,
                                 static_cast<float>( size[1] ) * spacing, static_cast<float>( size[2] ) * spacing,
                                 spacing, outputVars );

        float origin[] = { spacing * static_cast<float>( voxelBounds[0] ),
                           spacing * static_cast<float>( voxelBounds[2] ),
                           spacing * static_cast<float>( voxelBounds[4] ) };

        char* buffer = static_cast<char*>( alloca( pField->get_channel_map().structure_size() ) );

        int voxel = 0;
        frantic::graphics::vector3f p;

        // NOTE: FumeFX seems to force fumeData->nz to a minimum value (8) in at least one test case. So that's weird.
        for( int x = 0; x < fumeData->nx; ++x ) {
            p.x = ( static_cast<float>( x + fumeData->nx0 ) + 0.5f ) * fumeData->dx + origin[0];
            for( int y = 0; y < fumeData->ny; ++y ) {
                p.y = ( static_cast<float>( y + fumeData->ny0 ) + 0.5f ) * fumeData->dx + origin[1];
                for( int z = 0; z < fumeData->nz; ++z, ++voxel ) {
                    p.z = ( static_cast<float>( z + fumeData->nz0 ) + 0.5f ) * fumeData->dx + origin[2];

                    if( pField->evaluate_field( buffer, p ) ) {
                        if( outputVars & SIM_USEDENS ) {
                            fumeData->SetRo2( voxel, densityAccessor.get( buffer ) );
                        }

                        if( outputVars & SIM_USEFUEL ) {
                            fumeData->SetFuel2( voxel, fireAccessor.get( buffer ) );
                        }

                        if( outputVars & SIM_USETEMP ) {
                            fumeData->SetTemp2( voxel, temperatureAccessor.get( buffer ) );
                        }

                        if( outputVars & SIM_USEVEL ) {
                            frantic::graphics::vector3f v = velocityAccessor.get( buffer );
                            fumeData->SetVel2( voxel, v.x, v.y, v.z );
                        }

                        if( outputVars & SIM_USETEXT ) {
                            frantic::graphics::vector3f t = textureCoordAccessor.get( buffer );
                            fumeData->SetXYZ2( voxel, t.x, t.y, t.z );
                        }

                        if( outputVars & SIM_USECOLOR ) {
                            frantic::graphics::color3f c = colorAccessor.get( buffer );

                            SDColor sdC( c.r, c.g, c.b );
                            fumeData->SetColor2( voxel, sdC );
                        }

                    } else {
                        if( outputVars & SIM_USEDENS ) {
                            fumeData->SetRo2( voxel, 0.f );
                        }

                        if( outputVars & SIM_USEFUEL ) {
                            fumeData->SetFuel2( voxel, 0.f );
                        }

                        if( outputVars & SIM_USETEMP ) {
                            fumeData->SetTemp2( voxel, 0.f );
                        }

                        if( outputVars & SIM_USEVEL ) {
                            fumeData->SetVel2( voxel, 0.f, 0.f, 0.f );
                        }

                        if( outputVars & SIM_USETEXT ) {
                            fumeData->SetXYZ2( voxel, 0.f, 0.f, 0.f );
                        }

                        if( outputVars & SIM_USECOLOR ) {
                            SDColor c( 0, 0, 0 );
                            fumeData->SetColor2( voxel, c );
                        }
                    }
                }
            }
        }

        FumeFXSaveToFileData saveData;
#if MAX_VERSION_MAJOR >= 24
        GetSystemUnitInfo( &saveData.type, &saveData.scale );
#else
        GetMasterUnitInfo( &saveData.type, &saveData.scale );
#endif
        saveData.tm.IdentityMatrix();
        saveData.tm.data[3] = 0.5f * ( simWSBounds.minimum().x + simWSBounds.maximum().x );
        saveData.tm.data[7] = 0.5f * ( simWSBounds.minimum().y + simWSBounds.maximum().y );
        saveData.tm.data[11] = simWSBounds.minimum().z;

        fumeData->SaveOutput( path.c_str(), outputVars, saveData, VoxelFlowBase::SaveField::spare_field );
    }
};

std::unique_ptr<fumefx_factory_interface> create_fumefx_iodll_factory() {
    return std::unique_ptr<fumefx_factory_interface>( new fumefx_iodll_factory() );
}

} // namespace volumetrics
} // namespace max3d
} // namespace frantic
#endif
