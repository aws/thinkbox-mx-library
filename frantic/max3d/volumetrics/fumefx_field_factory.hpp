// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#if defined( FUMEFX_SDK_AVAILABLE )

#include <frantic/particles/streams/particle_istream.hpp>
#include <frantic/volumetrics/field_interface.hpp>
#include <frantic/volumetrics/voxel_coord_system.hpp>

#include <boost/cstdint.hpp>
#include <boost/thread/future.hpp>

#pragma warning( push, 3 )
#include <inode.h>
#include <object.h>
#pragma warning( pop )

#include <memory>

namespace frantic {
namespace max3d {
namespace volumetrics {

inline bool is_fumefx_node( INode* node, TimeValue /*t*/ ) {
    Object* obj = node ? node->GetObjectRef() : NULL;
    if( obj )
        obj = obj->FindBaseObject();

    const Class_ID fumefxId( 902511643, 1454773937 ); // Found via MXS
    const Class_ID fumefxDemoId( 634076187, 1454773937 );

    return obj && ( obj->ClassID() == fumefxId || obj->ClassID() == fumefxDemoId );
}

/**
 * Returns the version # of the FumeFX DLL that is loaded.
 */
boost::int64_t get_fumefx_version();

class fumefx_field_interface : public frantic::volumetrics::field_interface {
  public:
    virtual ~fumefx_field_interface() {}

    /**
     * Retrieves the bounding box of the defined region of the volumetric data. This is in generic units (ie. not
     * voxels).
     */
    virtual const frantic::graphics::boundbox3f& get_bounds() const = 0;

    /**
     * Retrieves the voxel size and offset of the volume data. The voxel size measures the spacing between samples (in
     * generic units).
     */
    virtual const frantic::volumetrics::voxel_coord_system& get_voxel_coord_sys() const = 0;
};

/**
 * Abstract class that extends the particle_istream interface for generating particles in a fumefx grid.
 */
class fumefx_source_particle_istream : public frantic::particles::streams::particle_istream {
  public:
    virtual void set_particle_count( boost::int64_t numParticles ) = 0;

    virtual void set_random_seed( boost::uint32_t seed ) = 0;
};

/**
 * Populated by get_fumefx_field_async before it returns.
 */
struct fumefx_fxd_metadata {
    float dx;             // Grid sample spacing.
    float simBounds[6];   // Bounding box of simulation grid. Arranged { minX, minY, minZ, maxX, maxY, maxZ }
    float dataBounds[6];  // Bounding box of valid data grid. Arranged { minX, minY, minZ, maxX, maxY, maxZ }
    std::size_t memUsage; // Number of bytes this field will use when fully loaded.
    frantic::channels::channel_map fileChannels; // Which channels are present in the file.
};

// Abstract factory for creating various fumefx interfaces depending on the version of FumeFX detected at runtime.
class fumefx_factory_interface {
  public:
    virtual ~fumefx_factory_interface() {}

    virtual std::unique_ptr<fumefx_field_interface> get_fumefx_field( const frantic::tstring& fxdPath,
                                                                      const frantic::graphics::transform4f& toWorldTM,
                                                                      int channelsRequested ) const = 0;
    virtual std::unique_ptr<fumefx_field_interface> get_fumefx_field( INode* pNode, TimeValue t,
                                                                      int channelsRequested ) const = 0;
    virtual boost::shared_future<boost::shared_ptr<frantic::volumetrics::field_interface>>
    get_fumefx_field_async( const frantic::tstring& fxdPath, const frantic::graphics::transform4f& toWorldTM,
                            int channelsRequested, fumefx_fxd_metadata& outMetadata ) const = 0;
    virtual boost::shared_future<boost::shared_ptr<frantic::volumetrics::field_interface>>
    get_fumefx_field_async( INode* pNode, TimeValue t, int channelsRequested,
                            fumefx_fxd_metadata& outMetadata ) const = 0;

    virtual void write_fxd_file( const frantic::tstring& path,
                                 const boost::shared_ptr<frantic::volumetrics::field_interface>& pField,
                                 const frantic::graphics::boundbox3f& simBounds,
                                 const frantic::graphics::boundbox3f& curBounds, float spacing,
                                 const frantic::channels::channel_map* pOverrideChannels ) const;

    virtual boost::shared_ptr<fumefx_source_particle_istream>
    get_fumefx_source_particle_istream( INode* pNode, TimeValue t,
                                        const frantic::channels::channel_map& requestedChannels ) const = 0;
};

fumefx_factory_interface& get_fumefx_factory();

class empty_fumefx_field : public fumefx_field_interface {
    frantic::channels::channel_map m_channelMap;
    frantic::graphics::boundbox3f m_bounds;
    frantic::volumetrics::voxel_coord_system m_vcs;

  public:
    empty_fumefx_field()
        : m_vcs( frantic::graphics::vector3f(), 1.f ) {
        m_channelMap.define_channel<float>( _T("Smoke") );
        m_channelMap.define_channel<float>( _T("Fire") );
        m_channelMap.define_channel<float>( _T("Temperature") );
        m_channelMap.define_channel<frantic::graphics::vector3f>( _T("Velocity") );
        m_channelMap.define_channel<frantic::graphics::vector3f>( _T("TextureCoord") );
        m_channelMap.end_channel_definition();
    }

    virtual ~empty_fumefx_field() {}

    virtual bool evaluate_field( void* /*dest*/, const frantic::graphics::vector3f& /*pos*/ ) const { return false; }
    virtual const frantic::channels::channel_map& get_channel_map() const { return m_channelMap; }
    virtual const frantic::graphics::boundbox3f& get_bounds() const { return m_bounds; }
    virtual const frantic::volumetrics::voxel_coord_system& get_voxel_coord_sys() const { return m_vcs; }
};

/**
 * Returns a fumefx_field_interface subclass instance from the simulation file (.fxd) stored at the specified path.
 */
std::unique_ptr<fumefx_field_interface> get_fumefx_field( const frantic::tstring& fxdPath );

/**
 * Returns an instance of fumefx_field_interface subclass that can extract FumeFX data. The FumeFX sim's "default"
 * simulation data and the frame closest to 't' will be used.
 */
std::unique_ptr<fumefx_field_interface> get_fumefx_field( INode* pNode, TimeValue t );

namespace fumefx_channels {
enum option {
    fire = 1 << 1,
    temperature = 1 << 2,
    smoke = 1 << 3,
    texture = 1 << 4,
    velocity = 1 << 5,
    flags = 1 << 6,
    color = 1 << 7
};
}

/**
 * \overload Allows a mask consisting of a OR combination of fumefx_channels::option values which specifies the channels
 * to provide.
 */
std::unique_ptr<fumefx_field_interface> get_fumefx_field( INode* pNode, TimeValue t, int channelsRequested );

/**
 * Asynchronously loads a FumeFX field from a .fxd file.
 * \param fxdPath The path of the .fxd file to load.
 * \param channelsRequested A bitmask indicating which channels should be loaded. Us an OR combination of
 * fumefx_channels::option values. \param outMetadata Metadata that is immediately populated while the field data is
 * being loaded asynchronously. \return A future that will provide access to the field interface once it has loaded.
 */
boost::shared_future<boost::shared_ptr<frantic::volumetrics::field_interface>>
get_fumefx_field_async( const frantic::tstring& fxdPath, int channelsRequested, fumefx_fxd_metadata& outMetadata );

/**
 * Asynchronously loads a FumeFX field from a .fxd file by reading the file path and retiming info from a FumeFX sim
 * object in 3ds Max. \param pNode The FumeFX simulation node to load data from. \param t The time to load data for.
 * \param channelsRequested A bitmask indicating which channels should be loaded. Us an OR combination of
 * fumefx_channels::option values. \param outMetadata Metadata that is immediately populated while the field data is
 * being loaded asynchronously. \return A future that will provide access to the field interface once it has loaded.
 */
boost::shared_future<boost::shared_ptr<frantic::volumetrics::field_interface>>
get_fumefx_field_async( INode* pNode, TimeValue t, int channelsRequested, fumefx_fxd_metadata& outMetadata );

/**
 * Samples a field onto a FumeFX grid and saves it to a .fxd file.
 * \param path The file path to save to.
 * \param pField The field to sample to the FumeFX grid.
 * \param simBounds The time-invariant maximum bounds of the FumeFX simulation region.
 * \param curBounds The subset of simBounds that has valid data at the current time.
 * \param spacing The inter-sample spacing.
 * \param pOverrideChannels An optional channel_map that can be used to reinterprets the results of pField->evaluate().
 * Typically used to mask out some channels, or rename them. Can be NULL.
 */
void write_fxd_file( const frantic::tstring& path,
                     const boost::shared_ptr<frantic::volumetrics::field_interface>& pField,
                     const frantic::graphics::boundbox3f& simBounds, const frantic::graphics::boundbox3f& curBounds,
                     float spacing, const frantic::channels::channel_map* pOverrideChannels = NULL );

/**
 * Implementation of fumefx_source_particle_istream interface that reports 0 particles.
 */
class empty_fumefx_source_particle_istream : public fumefx_source_particle_istream {
  public:
    empty_fumefx_source_particle_istream( const frantic::tstring& fxdPath,
                                          const frantic::channels::channel_map& particleChannelMap );

    virtual ~empty_fumefx_source_particle_istream();

    virtual void set_particle_count( boost::int64_t numParticles );
    virtual void set_random_seed( boost::uint32_t seed );

    void close();

    std::size_t particle_size() const;

    frantic::tstring name() const;
    boost::int64_t particle_count() const;
    boost::int64_t particle_index() const;
    boost::int64_t particle_count_left() const;
    boost::int64_t particle_progress_count() const;
    boost::int64_t particle_progress_index() const;

    const frantic::channels::channel_map& get_channel_map() const;
    const frantic::channels::channel_map& get_native_channel_map() const;

    void set_default_particle( char* /*buffer*/ );
    void set_channel_map( const frantic::channels::channel_map& particleChannelMap );

    bool get_particle( char* /*rawParticleBuffer*/ );
    bool get_particles( char* /*particleBuffer*/, std::size_t& numParticles );

  private:
    frantic::channels::channel_map m_particleChannelMap;
    frantic::channels::channel_map m_nativeMap;
    frantic::tstring m_fxdPath;
};

/**
 * Creates a fumefx_source_particle_istream subclass that seeds particles in the FumeFX voxels flagged as 'Source',
 * similar to the FumeFX Birth Particle Flow operator. \param pNode The scene node containing a FumeFX simulation object
 * \param t The scene time to evaluate the FumeFX data at
 * \param requestedChannels The channel map to assign to the resulting particle_istream
 * \return A new instance of a fumefx_source_particle_istream subclass.
 */
boost::shared_ptr<fumefx_source_particle_istream>
get_fumefx_source_particle_istream( INode* pNode, TimeValue t,
                                    const frantic::channels::channel_map& requestedChannels );

} // namespace volumetrics
} // namespace max3d
} // namespace frantic
#endif
