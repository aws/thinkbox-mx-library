// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <maxversion.h>

// TP 7.0 is only available in Max 2019+
// TP 7.0 SDK supports ThinkingParticles plugin version 6 and 7.
#if MAX_VERSION_MAJOR >= 21

#include <algorithm>

#pragma warning( push, 3 )
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

using std::max;
using std::min;

#include <max.h>
#pragma warning( pop )

/**
 * NOTE: If you are finding an error of missing include <MatterWaves.h> it is because you need to override the include
 * path for ONLY THIS CPP file. Do not add the Thinking Particles headers to the entire project because we need
 * tp_particle_istreamXX.cpp to have a different include path for each one. Therefore, select only this .cpp and adjust
 * the include paths for it specifically!!!
 */
#pragma warning( push, 3 )
#include <MaxParticleInterface.h>
#if defined( FRANTIC_CONDA_BUILD )
#include <thinking_particles_7_0/MatterWaves.h>
#else
#include <MatterWaves.h>
#endif
#pragma warning( pop )

#include <frantic/particles/streams/concatenated_particle_istream.hpp>
#include <frantic/particles/streams/empty_particle_istream.hpp>

#pragma warning( push )
#pragma warning( disable : 4706 )

#include <frantic/max3d/particles/tp_interface.hpp>
#include <frantic/max3d/particles/tp_particle_istream_template.hpp>

namespace frantic {
namespace max3d {
namespace particles {

extern boost::int64_t g_tpVersion;
extern bool is_node_thinking_particles( INode* pNode );

class tp_interface70 : public thinking_particles_interface {
  public:
    virtual bool is_available();

    virtual bool is_node_thinking_particles( INode* pNode );

    virtual boost::int64_t get_version();

    virtual void get_groups( INode* pNode, std::vector<ReferenceTarget*>& outGroups );

    virtual frantic::tstring get_group_name( ReferenceTarget* group );

    virtual particle_istream_ptr get_particle_stream( const frantic::channels::channel_map& pcm, INode* pNode,
                                                      ReferenceTarget* group, TimeValue t );
};

bool tp_interface70::is_available() { return true; }

bool tp_interface70::is_node_thinking_particles( INode* pNode ) {
    return frantic::max3d::particles::is_node_thinking_particles( pNode );
}

boost::int64_t tp_interface70::get_version() { return g_tpVersion; }

void tp_interface70::get_groups( INode* pNode, std::vector<ReferenceTarget*>& outNamedGroups ) {
    Object* pObj = pNode->GetObjectRef();
    if( !pObj || !( pObj = pObj->FindBaseObject() ) || pObj->ClassID() != MATTERWAVES_CLASS_ID )
        return;

    Tab<DynNameBase*> groups;
    static_cast<ParticleMat*>( pObj )->GetALLGroups( &groups );

    for( int i = 0; i < groups.Count(); ++i )
        outNamedGroups.push_back( groups[i] );
}

frantic::tstring tp_interface70::get_group_name( ReferenceTarget* group ) {
    if( group && ( group->ClassID() == PGROUP_CLASS_ID ) )
        return static_cast<PGroup*>( group )->GetName();
    return _T("");
}

tp_interface70::particle_istream_ptr tp_interface70::get_particle_stream( const frantic::channels::channel_map& pcm,
                                                                          INode* pNode, ReferenceTarget* group,
                                                                          TimeValue t ) {
    Object* pObj = pNode->GetObjectRef();
    if( !pObj || !( pObj = pObj->FindBaseObject() ) || pObj->ClassID() != MATTERWAVES_CLASS_ID )
        return particle_istream_ptr( new frantic::particles::streams::empty_particle_istream( pcm, pcm ) );

    std::vector<particle_istream_ptr> pins;

    if( group != NULL ) {
        pins.push_back(
            particle_istream_ptr( new tp_particle_istream_template( pcm, pNode, static_cast<PGroup*>( group ), t ) ) );
    } else {
        ParticleMat* pMat = static_cast<ParticleMat*>( pObj );

        Tab<DynNameBase*> groups;

        // Ok to ParticleMat::GetALLGroups() because it is virtual so doesn't link to anything.
        pMat->GetALLGroups( &groups );

        for( int i = 0; i < groups.Count(); ++i ) {
            PGroup* pGroup = static_cast<PGroup*>( groups[i] );

            if( pGroup->GetRenderable() )
                pins.push_back( particle_istream_ptr( new tp_particle_istream_template( pcm, pNode, pGroup, t ) ) );
        }
    }

    if( pins.size() > 0 ) {
        if( pins.size() > 1 )
            return tp_interface::particle_istream_ptr(
                new frantic::particles::streams::concatenated_particle_istream( pins ) );
        return pins[0];
    }
    return tp_interface::particle_istream_ptr( new frantic::particles::streams::empty_particle_istream( pcm, pcm ) );
}

tp_interface* create_tp_interface70() { return new tp_interface70; }

} // namespace particles
} // namespace max3d
} // namespace frantic

#pragma warning( pop )

#endif
