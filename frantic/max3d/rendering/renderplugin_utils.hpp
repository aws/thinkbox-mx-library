// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <frantic/rendering/lights/light_list.hpp>
namespace frantic {
namespace max3d {
namespace rendering {

// This initializes all the variables an a RenderGlobalContext to some
// reasonable starting values.
inline void initialize_renderglobalcontext( RenderGlobalContext& rgc ) {
    rgc.renderer = NULL;
    rgc.projType = PROJ_PERSPECTIVE;
    rgc.devWidth = 0;
    rgc.devHeight = 0;
    rgc.xscale = 1.f;
    rgc.yscale = 1.f;
    rgc.xc = 0;
    rgc.yc = 0;
    rgc.antialias = FALSE;
    rgc.nearRange = 0;
    rgc.farRange = 0;
    rgc.devAspect = 0;
    rgc.frameDur = 1.f;
    rgc.envMap = NULL;
    rgc.atmos = NULL;
    rgc.time = 0;
    rgc.wireMode = FALSE;
    rgc.force2Side = FALSE;
    rgc.inMtlEdit = FALSE;
    rgc.fieldRender = FALSE;
    rgc.first_field = FALSE;
    rgc.field_order = FALSE;
    rgc.objMotBlur = FALSE;
    rgc.nBlurFrames = 0;
}

namespace detail {
inline void add_lights_from_scene_recursive( INode* node, TimeValue t, std::set<INode*> doneNodes,
                                             frantic::rendering::lights::light_list& outLights, float mblurInterval,
                                             float mblurBias ) {
    if( node == 0 )
        return;

    // Check whether this node was already processed, and if not, add it
    // to the set of processed nodes.
    if( doneNodes.find( node ) != doneNodes.end() )
        return;
    doneNodes.insert( node );

    outLights.add_light( node, t, mblurInterval, mblurBias );

    // Recursively process all the children
    int numChildren = node->NumberOfChildren();
    for( int childIndex = 0; childIndex < numChildren; ++childIndex ) {
        add_lights_from_scene_recursive( node->GetChildNode( childIndex ), t, doneNodes, outLights, mblurInterval,
                                         mblurBias );
    }
}
} // namespace detail

inline void add_lights_from_scene( INode* scene, TimeValue t, frantic::rendering::lights::light_list& outLights,
                                   float mblurInterval = 0.5f, float mblurBias = 0.f ) {
    std::set<INode*> doneNodes;
    detail::add_lights_from_scene_recursive( scene, t, doneNodes, outLights, mblurInterval, mblurBias );
}

// This recursively calls LoadMapFiles on all the sub texmaps of this material
inline void load_map_files_recursive( MtlBase* m, TimeValue t ) {
    if( m == 0 )
        return;

    if( IsTex( m ) )
        static_cast<Texmap*>( m )->LoadMapFiles( t );

    if( IsMtl( m ) ) {
        Mtl* mtl = static_cast<Mtl*>( m );
        for( int i = 0; i < mtl->NumSubMtls(); ++i )
            load_map_files_recursive( mtl->GetSubMtl( i ), t );
    }

    for( int i = 0; i < m->NumSubTexmaps(); ++i )
        load_map_files_recursive( m->GetSubTexmap( i ), t );
}

// This function object can be used with refmaker_call_recursive to call RenderBegin
// on all the objects.
class render_begin_function {
    TimeValue m_t;
    unsigned long m_flags;

  public:
    render_begin_function( TimeValue t, unsigned long flags ) {
        m_t = t;
        m_flags = flags;
    }

    void operator()( ReferenceMaker* rm ) const { rm->RenderBegin( m_t, m_flags ); }
};

// This function object can be used with refmaker_call_recursive to call RenderEnd
// on all the objects.
class render_end_function {
    TimeValue m_t;

  public:
    render_end_function( TimeValue t ) { m_t = t; }

    void operator()( ReferenceMaker* rm ) const { rm->RenderEnd( m_t ); }
};

template <class FunctionObject>
inline void refmaker_call_recursive( ReferenceMaker* rm, std::set<ReferenceMaker*>& doneNodes,
                                     const FunctionObject& func ) {
    if( rm == 0 )
        return;

    // Use the set<> to ensure we don't process anything twice
    if( doneNodes.find( rm ) == doneNodes.end() ) {
        doneNodes.insert( rm );

        func( rm );

        // Go through all the references
        int numRefs = rm->NumRefs();
        for( int i = 0; i < numRefs; i++ ) {
            refmaker_call_recursive( rm->GetReference( i ), doneNodes, func );
        }

        // In the case of an INode, also go through all of its children
        if( rm->SuperClassID() == BASENODE_CLASS_ID ) {
            INode* node = static_cast<INode*>( rm );
            for( int i = 0; i < node->NumberOfChildren(); ++i ) {
                refmaker_call_recursive( node->GetChildNode( i ), doneNodes, func );
            }
        }
    }
}

// This function object can be used with inode_call_recursive to get all the renderable
// nodes from a scene.  This also has the side effect of calling EvalWorldState on
// the nodes.
class renderable_node_retrieval_function {
    std::vector<INode*>& m_nodes;
    TimeValue m_t;
    bool m_renderHidden;

    renderable_node_retrieval_function& operator=( const renderable_node_retrieval_function& ) {}

  public:
    renderable_node_retrieval_function( std::vector<INode*>& nodes, TimeValue t, bool renderHidden )
        : m_nodes( nodes )
        , m_t( t )
        , m_renderHidden( renderHidden ) {}

    void operator()( INode* node ) const {
        if( node->Renderable() && ( m_renderHidden || !node->IsNodeHidden( true ) ) ) {
            ObjectState os = node->EvalWorldState( m_t );
            if( os.obj != 0 ) {
                Object* obj = os.obj;
                SClass_ID scid = obj->SuperClassID();

                // Shape and Geometry objects can be rendered
                if( ( scid == SHAPE_CLASS_ID || scid == GEOMOBJECT_CLASS_ID ) && obj->IsRenderable() ) {
                    m_nodes.push_back( node );
                }
            }
        }
    }
};

// This recursively calls a function on an inode and all its children.
template <class FunctionObject>
inline void inode_call_recursive( INode* node, std::set<INode*>& doneNodes, const FunctionObject& func ) {
    if( node == 0 )
        return;

    // Use the set<> to ensure we don't process anything twice
    if( doneNodes.find( node ) == doneNodes.end() ) {
        doneNodes.insert( node );

        func( node );

        for( int i = 0; i < node->NumberOfChildren(); ++i ) {
            inode_call_recursive( node->GetChildNode( i ), doneNodes, func );
        }
    }
}

// This recursively calls a function on all the inodes in an array and all their children.
template <class FunctionObject>
inline void inode_call_recursive( const std::vector<INode*>& nodes, std::set<INode*>& doneNodes,
                                  const FunctionObject& func ) {
    for( unsigned i = 0; i < nodes.size(); ++i )
        inode_call_recursive( nodes[i], doneNodes, func );
}

} // namespace rendering
} // namespace max3d
} // namespace frantic
