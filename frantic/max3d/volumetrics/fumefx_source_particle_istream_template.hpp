// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
/**
 * @file fumefx_source_particle_istream_impl.hpp
 *
 */
#pragma once

#include <frantic/channels/channel_map_adaptor.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

#include <boost/random.hpp>

class VoxelFlowBase; // Forward decl.

namespace frantic {
namespace max3d {
namespace volumetrics {

// This class must be declared in the anonymous namespace in order to reuse the same name (ie.
// fumefx_source_particle_istream_impl) in multiple .cpp files as intended.
namespace {

/**
 * Generates N particles randomly inside the voxels tagged as 'source' via the SIM_USEFLAGS channel.
 */
class fumefx_source_particle_istream_impl : public fumefx_source_particle_istream {
  public:
    fumefx_source_particle_istream_impl( boost::shared_ptr<VoxelFlowBase> fumeData,
                                         const frantic::channels::channel_map& requestedChannels,
                                         const frantic::tstring& fxdPath );

    virtual void set_particle_count( boost::int64_t numParticles );
    virtual void set_random_seed( boost::uint32_t seed );

    virtual void close();
    virtual frantic::tstring name() const;

    virtual std::size_t particle_size() const;

    virtual boost::int64_t particle_count() const;
    virtual boost::int64_t particle_index() const;
    virtual boost::int64_t particle_count_left() const;

    virtual boost::int64_t particle_progress_count() const;
    virtual boost::int64_t particle_progress_index() const;

    virtual void set_channel_map( const frantic::channels::channel_map& particleChannelMap );

    virtual const frantic::channels::channel_map& get_channel_map() const;

    virtual const frantic::channels::channel_map& get_native_channel_map() const;

    virtual void set_default_particle( char* rawParticleBuffer );

    virtual bool get_particle( char* rawParticleBuffer );
    virtual bool get_particles( char* rawParticleBuffer, std::size_t& numParticles );

  private:
    void init();

  private:
    frantic::channels::channel_map m_outMap;
    frantic::channels::channel_map m_nativeMap;

    boost::int64_t m_particleIndex;
    boost::int64_t m_particleCount;
    boost::scoped_array<char> m_defaultParticle;

    frantic::channels::channel_accessor<frantic::graphics::vector3f> m_posAccessor;

    std::vector<unsigned> m_taggedVoxels; // The collection of voxels tagged as sources

    boost::mt19937 m_rndGen;

    frantic::tstring m_fxdPath;

    boost::shared_ptr<VoxelFlowBase> m_fumeData;
};

fumefx_source_particle_istream_impl::fumefx_source_particle_istream_impl(
    boost::shared_ptr<VoxelFlowBase> fumeData, const frantic::channels::channel_map& requestedChannels,
    const frantic::tstring& fxdPath )
    : m_outMap( requestedChannels )
    , m_particleCount( 0 )
    , m_fxdPath( fxdPath )
    , m_fumeData( fumeData )
    , m_rndGen( 1234u ) {
    m_particleIndex = -1;

    m_nativeMap.define_channel<frantic::graphics::vector3f>( _T("Position") );
    /*if( fumeData->loadedOutputVars & SIM_USEDENS )
            m_nativeMap.define_channel<float>( _T("Smoke") );
    if( fumeData->loadedOutputVars & SIM_USEFIRE )
            m_nativeMap.define_channel<float>( _T("Fire") );
    if( fumeData->loadedOutputVars & SIM_USETEMP )
            m_nativeMap.define_channel<float>( _T("Temperature") );
    if( fumeData->loadedOutputVars & SIM_USEVEL )
            m_nativeMap.define_channel<frantic::graphics::vector3f>( _T("Velocity") );
    if( fumeData->loadedOutputVars & SIM_USEXYZ )
            m_nativeMap.define_channel<frantic::graphics::vector3f>( _T("TextureCoord") );*/
    m_nativeMap.end_channel_definition( 4u );

    this->set_channel_map( requestedChannels );
}

void fumefx_source_particle_istream_impl::set_particle_count( boost::int64_t numParticles ) {
    m_particleCount = ( std::max )( 0i64, numParticles );
}

void fumefx_source_particle_istream_impl::set_random_seed( boost::uint32_t seed ) { m_rndGen.seed( seed ); }

void fumefx_source_particle_istream_impl::close() {}

frantic::tstring fumefx_source_particle_istream_impl::name() const { return m_fxdPath; }

std::size_t fumefx_source_particle_istream_impl::particle_size() const { return m_outMap.structure_size(); }

boost::int64_t fumefx_source_particle_istream_impl::particle_count() const { return m_particleCount; }

boost::int64_t fumefx_source_particle_istream_impl::particle_index() const { return m_particleIndex; }

boost::int64_t fumefx_source_particle_istream_impl::particle_count_left() const {
    return m_particleCount - m_particleIndex - 1;
}

boost::int64_t fumefx_source_particle_istream_impl::particle_progress_count() const {
    return fumefx_source_particle_istream_impl::particle_count();
}

boost::int64_t fumefx_source_particle_istream_impl::particle_progress_index() const {
    return fumefx_source_particle_istream_impl::particle_index();
}

void fumefx_source_particle_istream_impl::set_channel_map( const frantic::channels::channel_map& particleChannelMap ) {
    boost::scoped_array<char> newDefault( new char[particleChannelMap.structure_size()] );

    particleChannelMap.construct_structure( newDefault.get() );

    if( m_defaultParticle ) {
        frantic::channels::channel_map_adaptor tempAdaptor( particleChannelMap, m_outMap );

        tempAdaptor.copy_structure( newDefault.get(), m_defaultParticle.get() );
    }

    m_defaultParticle.swap( newDefault );

    m_outMap = particleChannelMap;

    m_posAccessor = m_outMap.get_accessor<frantic::graphics::vector3f>( _T("Position") );
}

const frantic::channels::channel_map& fumefx_source_particle_istream_impl::get_channel_map() const { return m_outMap; }

const frantic::channels::channel_map& fumefx_source_particle_istream_impl::get_native_channel_map() const {
    return m_nativeMap;
}

void fumefx_source_particle_istream_impl::set_default_particle( char* rawParticleBuffer ) {
    if( !m_defaultParticle )
        m_defaultParticle.reset( new char[m_outMap.structure_size()] );

    m_outMap.copy_structure( m_defaultParticle.get(), rawParticleBuffer );
}

bool fumefx_source_particle_istream_impl::get_particle( char* rawParticleBuffer ) {
    if( m_particleIndex < 0 )
        this->init();

    if( ++m_particleIndex >= m_particleCount )
        return false;

    m_outMap.copy_structure( rawParticleBuffer, m_defaultParticle.get() );

    boost::variate_generator<boost::mt19937&, boost::uniform_int<std::size_t>> rng(
        m_rndGen, boost::uniform_int<std::size_t>( 0, m_taggedVoxels.size() - 1u ) );

    unsigned voxel = m_taggedVoxels[rng()];

    unsigned voxelPos[3];
    voxelPos[0] = voxel / m_fumeData->nyz;
    voxelPos[1] = ( voxel % m_fumeData->nyz ) / m_fumeData->nz;
    voxelPos[2] = ( voxel % m_fumeData->nyz ) % m_fumeData->nz;

    boost::variate_generator<boost::mt19937&, boost::uniform_01<float>> rngFloat( m_rndGen,
                                                                                  boost::uniform_01<float>() );

    frantic::graphics::vector3f& p = m_posAccessor.get( rawParticleBuffer );

    // nx0 may be negative if using a boundless grid.
    // We need to cast voxelPos to an int to avoid casting negative nx0 values
    // to a huge unsigned value.
    p.x = ( static_cast<float>( (int)voxelPos[0] + m_fumeData->nx0 ) + rngFloat() ) * m_fumeData->dx - m_fumeData->midx;
    p.y = ( static_cast<float>( (int)voxelPos[1] + m_fumeData->ny0 ) + rngFloat() ) * m_fumeData->dx - m_fumeData->midy;
    p.z = ( static_cast<float>( (int)voxelPos[2] + m_fumeData->nz0 ) + rngFloat() ) * m_fumeData->dx;

    return true;
}

bool fumefx_source_particle_istream_impl::get_particles( char* rawParticleBuffer, std::size_t& numParticles ) {
    for( std::size_t i = 0, iEnd = numParticles; i < iEnd; ++i, rawParticleBuffer += m_outMap.structure_size() ) {
        if( !fumefx_source_particle_istream_impl::get_particle( rawParticleBuffer ) ) {
            numParticles = i;
            return false;
        }
    }

    return true;
}

void fumefx_source_particle_istream_impl::init() {
    m_taggedVoxels.clear();

    if( m_fumeData ) {
        // According to Kresimir (FumeFX developer), there is no way to load just the flags channel. It is loaded as a
        // side-effect of loading any other channel.
        int requestedChannels = SIM_USEDENS | SIM_USEFLAGS;

        FumeFXSaveToFileData saveData;
        int loadResult = m_fumeData->LoadOutput( m_fxdPath.c_str(), saveData, 0, requestedChannels );
        if( loadResult != LOAD_OK )
            throw std::runtime_error(
                "fumefx_source_particle_istream_impl::init() - Failed to load the FumeFX file: \n\n\t\"" +
                frantic::strings::to_string( m_fxdPath ) + "\"" );

        // Flags channel is loaded with any other channel
        if( m_fumeData->loadedOutputVars != 0 ) {
            for( unsigned i = 0; i < m_fumeData->cells; ++i ) {
                if( m_fumeData->GetF( i ) & 4 )
                    m_taggedVoxels.push_back( i );
            }
        }
    }

    // If we had no tagged voxels, we cannot create any particles
    if( m_taggedVoxels.empty() )
        m_particleCount = 0;
}

/**
 * Creates a fumefx_source_particle_istream subclass that seeds particles in the FumeFX voxels flagged as 'Source',
 * similar to the FumeFX Birth Particle Flow operator. \pre FumeFXTraits::init() Must have been called. \tparam
 * FumeFXTraits A traits class implementing: VoxelFlowBase* CreateVoxelFlow(); void DeleteVoxelFlow( VoxelFlowBase* );
 *                       frantic::tstring GetDataPath( INode* pNode, TimeValue t );
 * \param pNode The scene node containing a FumeFX simulation object
 * \param t The scene time to evaluate the FumeFX data at
 * \param requestedChannels The channel map to assign to the resulting particle_istream
 * \return A new instance of a fumefx_source_particle_istream subclass.
 */
template <class FumeFXTraits>
inline boost::shared_ptr<fumefx_source_particle_istream>
get_fumefx_source_particle_istream_impl( INode* pNode, TimeValue t,
                                         const frantic::channels::channel_map& requestedChannels ) {
    frantic::tstring fxdPath = FumeFXTraits::GetDataPath( pNode, t );

    boost::shared_ptr<VoxelFlowBase> fumeData;

    if( !fxdPath.empty() )
        fumeData = GetVoxelFlow<FumeFXTraits>( fxdPath, false );

    if( !fumeData )
        return boost::shared_ptr<fumefx_source_particle_istream>(
            new empty_fumefx_source_particle_istream( fxdPath, requestedChannels ) );

    return boost::shared_ptr<fumefx_source_particle_istream>(
        new fumefx_source_particle_istream_impl( fumeData, requestedChannels, fxdPath ) );
}

} // namespace

} // namespace volumetrics
} // namespace max3d
} // namespace frantic
