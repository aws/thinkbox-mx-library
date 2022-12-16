// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
#include <boost/shared_ptr.hpp>
#include <frantic/particles/streams/particle_istream.hpp>
#include <frantic/strings/tstring.hpp>

namespace frantic {
namespace max3d {
namespace particles {

/**
 * This class exposes various Thinking Particles related methods. It is implemented as
 * a singleton with a pure virtual interface. I chose this to allow us to completely change the
 * the implementation in the future without affecting users of the interface.
 */
class thinking_particles_interface {
  public:
    typedef boost::shared_ptr<frantic::particles::streams::particle_istream> particle_istream_ptr;

    /**
     * Returns a singleton instance of the appropriate Thinking Particles implementation.
     */
    static thinking_particles_interface& get_instance();

    /**
     * If a newer version of TP is found, the default behavior is to consider that a problem since Cebas doesn't usually
     * maintain backwards compatibility. This can be disabled so that the latest supported version of our TP code is
     * used with newer versions.
     */
    static void disable_version_check();

    /**
     * @return true If thinking particles is loaded and an the version is supported.
     */
    virtual bool is_available() = 0;

    /**
     * @param pNode The node whose base object is to be queried for being a Thinking Particles object.
     * @return true if get_particle_stream() with pNode not throw an exception.
     */
    virtual bool is_node_thinking_particles( INode* pNode ) = 0;

    /**
     * @return The version number of the loaded Thinking Particles dll. Encoded as frantic::win32::GetVersion() is.
     */
    virtual boost::int64_t get_version() = 0;

    /**
     * Collects all the groups from the given Thinking Particles node.
     * @param[in] pNode The node containing a Thinking Particles object.
     * @parma[out] outGroups The vector to fill with the groups.
     */
    virtual void get_groups( INode* pNode, std::vector<ReferenceTarget*>& outGroups ) = 0;

    /**
     * Returns the name of the specified Thinking Particles group.
     * @param group The group whose name we are querying
     * @return The name of the group.
     */
    virtual frantic::tstring get_group_name( ReferenceTarget* group ) = 0;

    /**
     * Returns a particle_istream containing the particles in the specified Thinking Particles node (and optionally a
     * specific group).
     * @param pcm The channel layout for the returned particle_istream to use.
     * @param pNode The node containing the Thinking Particles system. this->is_node_thinking_particles(pNode) must be
     * true.
     * @param group The specific TP group to get particles for, or NULL if all renderable groups should be extracted.
     * @parma t The time to have the particle_istream evaluate the system at.
     */
    virtual particle_istream_ptr get_particle_stream( const frantic::channels::channel_map& pcm, INode* pNode,
                                                      ReferenceTarget* group, TimeValue t ) = 0;

  protected:
    virtual ~thinking_particles_interface() {}
};

typedef thinking_particles_interface tp_interface;

} // namespace particles
} // namespace max3d
} // namespace frantic
#endif
