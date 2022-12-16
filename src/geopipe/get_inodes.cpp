// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <algorithm>
#include <iterator>

using std::max;
using std::min;

#include "frantic/max3d/geopipe/get_inodes.hpp"

using namespace std;

#pragma warning( disable : 4512 )

namespace frantic {
namespace max3d {

namespace detail {
class get_refmakers_proc : public DependentEnumProc {
    std::vector<ReferenceMaker*>& m_refmakers;
    ReferenceTarget* m_refTarget;

  public:
    get_refmakers_proc( std::vector<ReferenceMaker*>& refmakers, ReferenceTarget* refTarget )
        : m_refmakers( refmakers )
        , m_refTarget( refTarget ) {}

    virtual int proc( ReferenceMaker* rmaker ) {
        m_refmakers.push_back( rmaker );
        return 0;
    }
};
} // namespace detail
// This gets every ReferenceMaker object referring to refTarget
void get_refmakers( std::vector<ReferenceMaker*>& refmakers, ReferenceTarget* refTarget ) {
    assert( refTarget != 0 );

    refmakers.clear();

    detail::get_refmakers_proc ep( refmakers, refTarget );
#if MAX_RELEASE >= 9000
    refTarget->DoEnumDependents( &ep );
#else
    refTarget->EnumDependents( &ep );
#endif
}

void get_object_inodes( std::vector<INode*>& inodes, ReferenceTarget* refTarget ) {
    assert( refTarget != 0 );

    vector<ReferenceMaker*> refmakers;
    get_refmakers( refmakers, refTarget );

    // Copy all the refmakers which are also inodes of refTarget to the inodes vector
    inodes.clear();
    for( vector<ReferenceMaker*>::iterator i = refmakers.begin(); i != refmakers.end(); ++i ) {
        if( ( *i )->SuperClassID() == BASENODE_CLASS_ID ) {
            INode* inode = (INode*)( *i );

            if( inode->GetObjectRef() == refTarget ) {
                inodes.push_back( inode );
            }
        }
    }
}

void get_referring_inodes( std::vector<INode*>& inodes, ReferenceTarget* refTarget ) {
    assert( refTarget != 0 );

    vector<ReferenceMaker*> refmakers;
    get_refmakers( refmakers, refTarget );

    // Copy all the refmakers which are also inodes to the inodes vector
    inodes.clear();
    for( vector<ReferenceMaker*>::iterator i = refmakers.begin(); i != refmakers.end(); ++i ) {
        if( ( *i )->SuperClassID() == BASENODE_CLASS_ID ) {
            INode* inode = (INode*)( *i );
            inodes.push_back( inode );
        }
    }
}

#pragma optimize( "t", off )
void get_referring_inodes_recursive( std::vector<INode*>& inodes, ReferenceTarget* refTarget ) {
    if( refTarget ) {
        vector<ReferenceMaker*> refmakers;
        get_refmakers( refmakers, refTarget );

        // Copy all the refmakers which are also inodes to the inodes vector
        for( vector<ReferenceMaker*>::iterator i = refmakers.begin(); i != refmakers.end(); ++i ) {
            ReferenceMaker* refmaker = *i;

            if( refmaker->SuperClassID() == BASENODE_CLASS_ID ) {

                INode* inode = (INode*)( refmaker );
                inodes.push_back( inode );

            } else if( refmaker->SuperClassID() == REF_TARGET_CLASS_ID ) {

                ReferenceTarget* owner = (ReferenceTarget*)( refmaker );
                if( refTarget != owner )
                    get_referring_inodes_recursive( inodes, owner );
            }
        }
    }
}
#pragma optimize( "", on )

// This gets every object of the given class_id which refers to the target.
void get_referring_objects( std::vector<ReferenceMaker*>& objects, ReferenceTarget* refTarget, Class_ID cid ) {
    assert( refTarget != 0 );

    vector<ReferenceMaker*> refmakers;
    get_refmakers( refmakers, refTarget );

    // Copy all the refmakers which are also inodes to the inodes vector
    objects.clear();
    for( vector<ReferenceMaker*>::iterator i = refmakers.begin(); i != refmakers.end(); ++i ) {
        if( ( *i )->ClassID() == cid || cid == Class_ID( 0, 0 ) )
            objects.push_back( *i );
    }
}

// This gets every inode of every object of the given class_id which refers to the target.
void get_referring_objects_inodes( std::vector<INode*>& inodes, ReferenceTarget* refTarget, Class_ID cid ) {
    assert( refTarget != 0 );

    vector<ReferenceMaker*> objects;
    get_referring_objects( objects, refTarget, cid );

    vector<INode*> inodes_tmp;

    // Get all the inodes of each object
    inodes.clear();
    for( vector<ReferenceMaker*>::iterator i = objects.begin(); i != objects.end(); ++i ) {
        get_object_inodes( inodes_tmp, static_cast<ReferenceTarget*>( *i ) );
        copy( inodes_tmp.begin(), inodes_tmp.end(), std::back_inserter( inodes ) );
    }
}

// This gets every object with the given modifier class id applied to it, referencing the
// given refTarget. NOTE: this is a fairly specialized function designed to enumerate all the emitters of a
// fluid object.  It only accepts object space modifiers.
// WARNING: It might be possible for something incorrect to slip through, as I haven't figured out how to verify that
// the modifier
//          is in the modifier stack of the object.  It hasn't happened in practice, though.
void get_referring_osmodifier_inodes( std::vector<std::pair<INode*, Modifier*>>& mods, ReferenceTarget* refTarget,
                                      Class_ID cid ) {
    assert( refTarget != 0 );

    std::vector<ReferenceMaker*> modifiers;

    get_referring_objects( modifiers, refTarget, cid );

    vector<INode*> inodes_tmp;

    mods.clear();
    for( vector<ReferenceMaker*>::iterator i = modifiers.begin(); i != modifiers.end(); ++i ) {
        assert( cid == Class_ID( 0, 0 ) ||
                ( *i )->SuperClassID() == OSM_CLASS_ID ); // probably possible to generalize this away
        if( ( *i )->SuperClassID() == OSM_CLASS_ID ) {
            Modifier* mod = static_cast<Modifier*>( *i );
            // Only get modifiers which are enabled
            if( mod->IsEnabled() ) {
                get_referring_inodes( inodes_tmp, static_cast<ReferenceTarget*>( *i ) );
                for( vector<INode*>::iterator j = inodes_tmp.begin(); j != inodes_tmp.end(); ++j ) {
                    // Mask out all the refering INodes which do not have the modifier in their modifier stack
                    if( is_in_modifier_stack( mod, *j ) )
                        mods.push_back( make_pair( *j, mod ) );
                }
            }
        }
    }
}

namespace get_modifier_stack_detail {

// for getting all the modifiers of a node
class enum_modifiers_proc : public GeomPipelineEnumProc {
    vector<Modifier*>& m_mods;

  public:
    enum_modifiers_proc( vector<Modifier*>& mods )
        : m_mods( mods ) {}

    // object is a node, object, or modifier.  If derObj != NULL, object is a modifier
    PipeEnumResult proc( ReferenceTarget* object, IDerivedObject* derObj, int /*index*/ ) {
        if( derObj != NULL ) {
            Modifier* mod = static_cast<Modifier*>( object );

            // Only add the enabled modifiers to the stack
            if( mod->IsEnabled() )
                m_mods.push_back( mod );
        }

        return PIPE_ENUM_CONTINUE;
    }
};

// for getting the node that a modifier belongs to
class find_modifier_inode_proc : public GeomPipelineEnumProc {
    ModContext& m_modContext;
    bool m_found;

  public:
    find_modifier_inode_proc( ModContext& mc )
        : m_found( false )
        , m_modContext( mc ) {}
    PipeEnumResult proc( ReferenceTarget* /*object*/, IDerivedObject* derObj, int index ) {
        ModContext* curModContext = NULL;
        if( ( derObj != NULL ) && ( curModContext = derObj->GetModContext( index ) ) == &m_modContext ) {
            m_found = true;
            return PIPE_ENUM_STOP;
        }
        return PIPE_ENUM_CONTINUE;
    }
    bool found_inode() { return m_found; }
};
} // namespace get_modifier_stack_detail

// Gets all the modifiers in the stack of the given inode.  Given in order from the bottom to the top
void get_modifier_stack( std::vector<Modifier*>& mods, INode* inode ) {
    mods.clear();

    // Create the callback to add all the modifiers
    get_modifier_stack_detail::enum_modifiers_proc mod_proc( mods );

    // run through the stack
    EnumGeomPipeline( &mod_proc, inode, false );
}

bool is_in_modifier_stack( Modifier* mod, INode* inode ) {
    vector<Modifier*> mods;
    get_modifier_stack( mods, inode );
    return std::find( mods.begin(), mods.end(), mod ) != mods.end();
}

INode* find_modifier_inode( Modifier* mod, ModContext& mc ) {
    std::vector<INode*> inodes;
    get_referring_inodes( inodes, mod );

    for( size_t i = 0; i < inodes.size(); ++i ) {
        Object* obj = inodes[i]->GetObjectRef();
        get_modifier_stack_detail::find_modifier_inode_proc pipeEnumProc( mc );
        EnumGeomPipeline( &pipeEnumProc, obj );
        if( pipeEnumProc.found_inode() )
            return inodes[i];
    }
    return NULL;
}

} // namespace max3d
} // namespace frantic