// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/geopipe/get_inodes.hpp>
#include <frantic/max3d/parameter_extraction.hpp>

#pragma warning( push, 3 )
#include <IParticleObjectExt.h>
#include <ParticleFlow/IPFActionList.h>
#include <ParticleFlow/IPFActionListSet.h>
#include <ParticleFlow/IPFRender.h>
#include <ParticleFlow/IParticleGroup.h>
#pragma warning( pop )

namespace frantic {
namespace max3d {
namespace particles {

//<-- Copied from ParticleFlow/PFOperatorRender_ParamBlock.h -->
enum { kPFRender_type };
enum { kPFRender_mainPBlockIndex };
enum {
    kPFRender_type_none,
    kPFRender_type_boundingBoxes,
    kPFRender_type_geometry,
    kPFRender_type_phantom,
    kPFRender_type_num = 4
};
//<-- End copy -->

inline Object* get_render_operator( IParticleGroup* pGroup ) {
    if( !pGroup )
        throw std::runtime_error( "get_render_operator() - called w/ a NULL IParticleGroup" );

    IPFActionList* pActions = NULL;
    Object* pRenderOp = NULL;

    // Get the local render operator
    pActions = PFActionListInterface( pGroup->GetActionList() );
    if( !pActions->IsActivated() || !pActions->HasUpStream() )
        return NULL; // Node cannot have particles if its deactivated or has no incoming particles

    for( int i = 0; !pRenderOp && i < pActions->NumActions(); ++i ) {
        if( PFRenderInterface( pActions->GetAction( i ) ) && pActions->IsActionActive( i ) )
            pRenderOp = GetPFObject( pActions->GetAction( i )->GetObjectRef() );
    }

    // Get the global render operator if this group didn't have one
    INode* pSysNode = pGroup->GetParticleSystem();
    if( !pSysNode )
        throw std::runtime_error( "get_render_operator() - called w/ a NULL IPFSystem" );

    // BUG: Getting the system node from a group is wrong in at least one Box 2 scene.
    // std::string name = pSysNode->GetName();

    pActions = PFActionListInterface( pSysNode );
    if( pActions->IsActivated() ) {
        for( int i = 0; !pRenderOp && i < pActions->NumActions(); ++i ) {
            if( PFRenderInterface( pActions->GetAction( i ) ) && pActions->IsActionActive( i ) )
                pRenderOp = GetPFObject( pActions->GetAction( i )->GetObjectRef() );
        }
    }

    return pRenderOp;
}

inline int get_render_operator_type( Object* pRenderOp ) {
    if( !PFRenderInterface( pRenderOp ) )
        throw std::runtime_error( "get_render_operator_type() - the Object* passed was not a PFRenderOperator" );

    IParamBlock2* pb = static_cast<Animatable*>( pRenderOp )->GetParamBlock( kPFRender_mainPBlockIndex );
    if( !pb )
        throw std::runtime_error( "get_render_operator_type() - Could not get pblock from render op" );
    return pb->GetInt( kPFRender_type );
}

inline int get_render_operator_type( IParticleGroup* pGroup ) {
    if( Object* pRenderOp = get_render_operator( pGroup ) )
        return get_render_operator_type( pRenderOp );
    return -1;
}

// This function used to be in the particles namespace
// extracts all particle groups of a particular type from a node.
inline void extract_geometry_particle_groups( std::set<INode*>& groups, INode* node ) {
    if( !node )
        throw std::runtime_error( "frantic::max3d::extract_geometry_particle_groups passed a null INode" );

    Object* node_obj = node->GetObjectRef();
    // first, make sure this inode has something to do with particles.
    if( node_obj && GetParticleObjectExtInterface( node_obj ) ) {
        // try to get the groups from the node.
        std::vector<INode*> inodes;
        frantic::max3d::get_referring_inodes( inodes, node );

        // mprintf("checking %d inodes\n", inodes.size());
        std::vector<INode*>::iterator i = inodes.begin();
        for( ; i != inodes.end(); i++ ) {
            if( *i ) {
                Object* obj = ( *i )->GetObjectRef();
                if( obj ) {
                    if( IParticleGroup* pGroup = ParticleGroupInterface( obj ) ) {
                        if( get_render_operator_type( pGroup ) == 2 )
                            groups.insert( *i );
                    }
                }
            }
        }
    }
}

} // namespace particles
} // namespace max3d
} // namespace frantic
