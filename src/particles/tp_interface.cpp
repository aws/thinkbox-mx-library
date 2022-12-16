// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#if defined( THINKING_PARTICLES_SDK_AVAILABLE )

#include <frantic/max3d/particles/tp_interface.hpp>
#include <frantic/win32/utility.hpp>

namespace frantic {
namespace max3d {
namespace particles {

thinking_particles_interface* g_tpSingleton = NULL;
boost::int64_t g_tpVersion = 0;
bool g_tpAllowNewVersions = false;

// Copied from the Thinking Particles SDK 'Matterwaves.h'
#ifndef MATTERWAVES_CLASS_ID
#define MATTERWAVES_CLASS_ID Class_ID( 0x490e5a33, 0x45da39cf )
#endif

#pragma warning( push )
#pragma warning( disable : 4706 )
bool is_node_thinking_particles( INode* pNode ) {
    Object* pObj;

    // Yeah, I did this. I'm not ashamed.
    return ( pNode && ( pObj = pNode->GetObjectRef() ) && ( pObj = pObj->FindBaseObject() ) &&
             pObj->ClassID() == MATTERWAVES_CLASS_ID );
}
#pragma warning( pop )

class null_thinking_particles_interface_impl : public thinking_particles_interface {
    virtual bool is_available() { return false; }

    virtual bool is_node_thinking_particles( INode* pNode ) {
        return frantic::max3d::particles::is_node_thinking_particles( pNode );
    }

    virtual boost::int64_t get_version() { return g_tpVersion; }

    virtual void get_groups( INode* /*pNode*/, std::vector<ReferenceTarget*>& /*outGroups*/ ) {}

    virtual frantic::tstring get_group_name( ReferenceTarget* /*group*/ ) { return _T(""); }

    virtual particle_istream_ptr get_particle_stream( const frantic::channels::channel_map& /*pcm*/, INode* /*pNode*/,
                                                      ReferenceTarget* /*group*/, TimeValue /*t*/ ) {
        if( g_tpVersion == 0 )
            throw std::runtime_error(
                "Thinking Particles is not currently loaded so particles cannot be extracted from it." );
        else if( g_tpVersion < 0x0006000000000000 )
            throw std::runtime_error(
                "Thinking Particles 5 and older is no longer supported by Thinkbox Software. Sorry :(" );
        throw std::runtime_error(
            "Thinking Particles version " +
            frantic::strings::to_string( frantic::win32::GetExecutableVersion( _T( "ThinkingParticles.dlo" ) ) ) +
            " is not supported yet. Check http://www.thinkboxsoftware.com/downloads/ to see if support is now "
            "available." );
    }
};

#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
#if MAX_VERSION_MAJOR >= 21
extern thinking_particles_interface* create_tp_interface70();
#endif

thinking_particles_interface& thinking_particles_interface::get_instance() {
    if( !g_tpSingleton ) {
        HMODULE tpHandle = LoadLibrary( _T( "ThinkingParticles.dlo" ) );
        if( tpHandle ) {
            g_tpVersion = frantic::win32::GetVersion( _T( "ThinkingParticles.dlo" ) );
            FreeLibrary( tpHandle );
        }

#if MAX_VERSION_MAJOR >= 21
        // TP 6 + 7 is supported in Max 2019, 2020, 2021, and 2022
        if( ( g_tpVersion >= 0x0006000000000000 && g_tpVersion < 0x0008000000000000 ) ||
            ( g_tpAllowNewVersions && g_tpVersion >= 0x0008000000000000 ) ) {
            // Try to use the most recent version but expect to have some sort of problem in general.
            g_tpSingleton = create_tp_interface70(); // The interface for 6 and 7 is the same.
        } else {
            g_tpSingleton = new null_thinking_particles_interface_impl;
        }
#else
        g_tpSingleton = new null_thinking_particles_interface_impl;
#endif
    }

    return *g_tpSingleton;
}
#endif

void thinking_particles_interface::disable_version_check() {
    if( !g_tpAllowNewVersions ) {
        // Delete the existing singleton so that the next access will not enforce version limits.
        if( g_tpSingleton ) {
            delete g_tpSingleton;
            g_tpSingleton = NULL;
        }

        g_tpAllowNewVersions = true;
    }
}

} // namespace particles
} // namespace max3d
} // namespace frantic
#endif
