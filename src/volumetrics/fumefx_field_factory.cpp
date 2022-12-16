// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#if defined( FUMEFX_SDK_AVAILABLE )

#include <frantic/max3d/volumetrics/fumefx_field_factory.hpp>
#include <frantic/win32/utility.hpp>

#include <memory>

namespace frantic {
namespace max3d {
namespace volumetrics {

class missing_fumefx_error : public std::exception {
  public:
    virtual const char* what() const throw() {
        return "Could not load FumeFX.dlo. Unable to determine the installed version of FumeFX.";
    }
};

boost::int64_t get_fumefx_version() {
    HMODULE hFumeDLL = LoadLibrary( _T("FumeFX.dlo") );
    if( !hFumeDLL )
        throw missing_fumefx_error();
    FreeLibrary( hFumeDLL );

    return frantic::win32::GetVersion( _T("FumeFX.dlo") );
}

void fumefx_factory_interface::write_fxd_file(
    const frantic::tstring& /*path*/, const boost::shared_ptr<frantic::volumetrics::field_interface>& /*pField*/,
    const frantic::graphics::boundbox3f& /*simBounds*/, const frantic::graphics::boundbox3f& /*curBounds*/,
    float /*spacing*/, const frantic::channels::channel_map* /*pOverrideChannels*/ ) const {
    throw std::runtime_error( "Cannot write .FXD files with the current FumeFX version" );
}

std::unique_ptr<fumefx_factory_interface> create_fumefx_iodll_factory();

namespace {
std::unique_ptr<fumefx_factory_interface> g_factory;
}

fumefx_factory_interface& get_fumefx_factory() {
    if( g_factory.get() )
        return *g_factory;

    g_factory = create_fumefx_iodll_factory();

    return *g_factory;
}

/**
 * Returns a fumefx_field_interface subclass instance from the simulation file (.fxd) stored at the specified path.
 */
std::unique_ptr<fumefx_field_interface> get_fumefx_field( const frantic::tstring& fxdPath ) {
    return get_fumefx_factory().get_fumefx_field( fxdPath, frantic::graphics::transform4f::identity(), 0 );
}

/**
 * Returns an instance of fumefx_field_interface subclass that can extract FumeFX data. The FumeFX sim's "default"
 * simulation data and the frame closest to 't' will be used.
 */
std::unique_ptr<fumefx_field_interface> get_fumefx_field( INode* pNode, TimeValue t ) {
    return get_fumefx_factory().get_fumefx_field( pNode, t, 0 );
}

std::unique_ptr<fumefx_field_interface> get_fumefx_field( INode* pNode, TimeValue t, int channelsRequested ) {
    return get_fumefx_factory().get_fumefx_field( pNode, t, channelsRequested );
}

boost::shared_future<boost::shared_ptr<frantic::volumetrics::field_interface>>
get_fumefx_field_async( const frantic::tstring& fxdPath, int channelsRequested, fumefx_fxd_metadata& outMetadata ) {
    return get_fumefx_factory().get_fumefx_field_async( fxdPath, frantic::graphics::transform4f::identity(),
                                                        channelsRequested, outMetadata );
}

boost::shared_future<boost::shared_ptr<frantic::volumetrics::field_interface>>
get_fumefx_field_async( INode* pNode, TimeValue t, int channelsRequested, fumefx_fxd_metadata& outMetadata ) {
    return get_fumefx_factory().get_fumefx_field_async( pNode, t, channelsRequested, outMetadata );
}

void write_fxd_file( const frantic::tstring& path,
                     const boost::shared_ptr<frantic::volumetrics::field_interface>& pField,
                     const frantic::graphics::boundbox3f& simBounds, const frantic::graphics::boundbox3f& curBounds,
                     float spacing, const frantic::channels::channel_map* pOverrideChannels ) {
    return get_fumefx_factory().write_fxd_file( path, pField, simBounds, curBounds, spacing, pOverrideChannels );
}

boost::shared_ptr<fumefx_source_particle_istream>
get_fumefx_source_particle_istream( INode* pNode, TimeValue t,
                                    const frantic::channels::channel_map& requestedChannels ) {
    return get_fumefx_factory().get_fumefx_source_particle_istream( pNode, t, requestedChannels );
}

empty_fumefx_source_particle_istream::empty_fumefx_source_particle_istream(
    const frantic::tstring& fxdPath, const frantic::channels::channel_map& particleChannelMap )
    : m_particleChannelMap( particleChannelMap )
    , m_fxdPath( fxdPath ) {
    m_nativeMap.define_channel<float>( _T("Smoke") );
    m_nativeMap.define_channel<float>( _T("Fire") );
    m_nativeMap.define_channel<float>( _T("Temperature") );
    m_nativeMap.define_channel<frantic::graphics::vector3f>( _T("Velocity") );
    m_nativeMap.define_channel<frantic::graphics::vector3f>( _T("TextureCoord") );
    m_nativeMap.define_channel<frantic::graphics::vector3f>( _T("Color") );
    m_nativeMap.end_channel_definition();
}

empty_fumefx_source_particle_istream::~empty_fumefx_source_particle_istream() {}

void empty_fumefx_source_particle_istream::set_particle_count( boost::int64_t /*numParticles*/ ) {}

void empty_fumefx_source_particle_istream::set_random_seed( boost::uint32_t /*seed*/ ) {}

void empty_fumefx_source_particle_istream::close() {}

std::size_t empty_fumefx_source_particle_istream::particle_size() const {
    return m_particleChannelMap.structure_size();
}

frantic::tstring empty_fumefx_source_particle_istream::name() const { return m_fxdPath; }

boost::int64_t empty_fumefx_source_particle_istream::particle_count() const { return 0; }

boost::int64_t empty_fumefx_source_particle_istream::particle_index() const { return -1; }

boost::int64_t empty_fumefx_source_particle_istream::particle_count_left() const { return 0; }

boost::int64_t empty_fumefx_source_particle_istream::particle_progress_count() const { return 0; }

boost::int64_t empty_fumefx_source_particle_istream::particle_progress_index() const { return -1; }

const frantic::channels::channel_map& empty_fumefx_source_particle_istream::get_channel_map() const {
    return m_particleChannelMap;
}

const frantic::channels::channel_map& empty_fumefx_source_particle_istream::get_native_channel_map() const {
    return m_nativeMap;
}

void empty_fumefx_source_particle_istream::set_default_particle( char* /*buffer*/ ) {}

void empty_fumefx_source_particle_istream::set_channel_map( const frantic::channels::channel_map& particleChannelMap ) {
    m_particleChannelMap = particleChannelMap;
}

bool empty_fumefx_source_particle_istream::get_particle( char* /*rawParticleBuffer*/ ) { return false; }

bool empty_fumefx_source_particle_istream::get_particles( char* /*particleBuffer*/, std::size_t& numParticles ) {
    numParticles = 0;
    return false;
}

} // namespace volumetrics
} // namespace max3d
} // namespace frantic
#endif