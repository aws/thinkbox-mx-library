// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#if defined( PHOENIX_SDK_AVAILABLE )
class IPhoenixFDPrtGroup;

#include <frantic/graphics/quat4f.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

namespace frantic {
namespace max3d {
namespace particles {
namespace streams {

class phoenix_particle_istream : public frantic::particles::streams::particle_istream {
    boost::int64_t m_particleCount, m_particleIndex;

    frantic::channels::channel_map m_outMap, m_nativeMap;
    frantic::graphics::transform4f m_gridTM;

    boost::scoped_array<char> m_defaultParticle;

    IPhoenixFDPrtGroup* m_particles;

    struct {
        frantic::channels::channel_accessor<frantic::graphics::vector3f> pos;
        frantic::channels::channel_cvt_accessor<frantic::graphics::vector3f> vel;
        frantic::channels::channel_cvt_accessor<frantic::graphics::vector3f> texturecoord;
        frantic::channels::channel_cvt_accessor<frantic::graphics::quat4f> orientation;
        frantic::channels::channel_cvt_accessor<float> size;
        frantic::channels::channel_cvt_accessor<float> age;
        frantic::channels::channel_cvt_accessor<float> density;
        frantic::channels::channel_cvt_accessor<float> temp;
        frantic::channels::channel_cvt_accessor<float> fuel;
    } m_accessors;

    inline void init_accessors();

  public:
    phoenix_particle_istream( IPhoenixFDPrtGroup* particles, const frantic::graphics::transform4f& toWorldTM,
                              const frantic::channels::channel_map& pcm );

    // Virtual destructor so that we can use allocated pointers (generally with boost::shared_ptr)
    virtual ~phoenix_particle_istream() {}

    virtual void close();

    // The stream can return its filename or other identifier for better error messages.
    virtual frantic::tstring name() const;

    // TODO: We should add a verbose_name function, which all wrapping streams are required to mark up in some way

    // This is the size of the particle structure which will be loaded, in bytes.
    virtual std::size_t particle_size() const;

    // Returns the number of particles, or -1 if unknown
    virtual boost::int64_t particle_count() const;
    virtual boost::int64_t particle_index() const;
    virtual boost::int64_t particle_count_left() const;

    virtual boost::int64_t particle_progress_count() const;
    virtual boost::int64_t particle_progress_index() const;

    // This allows you to change the particle layout that's being loaded on the fly, in case it couldn't
    // be set correctly at creation time.
    virtual void set_channel_map( const frantic::channels::channel_map& particleChannelMap );

    // This is the particle channel map which specifies the byte layout of the particle structure that is being used.
    virtual const frantic::channels::channel_map& get_channel_map() const;

    // This is the particle channel map which specifies the byte layout of the input to this stream.
    // NOTE: This value is allowed to change after the following conditions:
    //    * set_channel_map() is called (for example, the empty_particle_istream equates the native map with the
    //    external map)
    virtual const frantic::channels::channel_map& get_native_channel_map() const;

    /** This provides a default particle which should be used to fill in channels of the requested channel map
     *	which are not supplied by the native channel map.
     *	IMPORTANT: Make sure the buffer you pass in is at least as big as particle_size() bytes.
     */
    virtual void set_default_particle( char* rawParticleBuffer );

    // This reads a particle into a buffer matching the channel_map.
    // It returns true if a particle was read, false otherwise.
    // IMPORTANT: Make sure the buffer you pass in is at least as big as particle_size() bytes.
    virtual bool get_particle( char* rawParticleBuffer );

    // This reads a group of particles. Returns false if the end of the source
    // was reached during the read.
    virtual bool get_particles( char* rawParticleBuffer, std::size_t& numParticles );
};

std::pair<bool, int> IsPhoenixObject( INode* pNode, TimeValue t );

boost::shared_ptr<frantic::particles::streams::particle_istream>
GetPhoenixParticleIstream( INode* pNode, TimeValue t, const frantic::channels::channel_map& pcm );

} // namespace streams
} // namespace particles
} // namespace max3d
} // namespace frantic
#endif
