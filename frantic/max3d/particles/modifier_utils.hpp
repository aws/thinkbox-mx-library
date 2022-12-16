// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/channels/channel_map.hpp>
#include <frantic/max3d/geometry/mesh.hpp>

#include <modstack.h>
#include <object.h>

namespace frantic {
namespace max3d {
namespace particles {

typedef std::pair<Modifier*, ModContext*> modifier_info_t;

namespace detail {
template <class ForwardIterator>
inline void apply_modifiers_to_paticles( const frantic::channels::channel_map& pcm, ForwardIterator begin,
                                         ForwardIterator end, TriObject* pTriObj,
                                         const std::vector<modifier_info_t> modifiers, INode* pNode, TimeValue t,
                                         TimeValue timeStep );

template <class ForwardIterator>
inline void apply_modifiers_to_particles( const frantic::channels::channel_map& pcm, ForwardIterator begin,
                                          ForwardIterator end, SClass_ID modifierType, INode* pNode, TimeValue t,
                                          TimeValue timeStep );
} // namespace detail

template <class ForwardIterator>
inline void apply_osm_modifiers_to_particles( const frantic::channels::channel_map& pcm, ForwardIterator begin,
                                              ForwardIterator end, INode* pNode, TimeValue t, TimeValue timeStep ) {
    detail::apply_modifiers_to_particles( pcm, begin, end, OSM_CLASS_ID, pNode, t, timeStep );
}

template <class Iterator>
inline void apply_wsm_modifiers_to_particles( const frantic::channels::channel_map& pcm, Iterator begin, Iterator end,
                                              INode* pNode, TimeValue t, TimeValue timeStep ) {
    detail::apply_modifiers_to_particles( pcm, begin, end, WSM_CLASS_ID, pNode, t, timeStep );
}

namespace detail {
/**
 * This function will iterate through a collection of particles, applying all of the
 * deformations to each one. It will also apply selection modifiers. The affected channels
 * are "Position", "Velocity", and "Normal", and "Selection".
 *
 * @note This function is currently unable to handle modifiers which return a different
 *        number of vertices than we passed in.
 * @note This function is current unable to handle modifiers that affect the UVW channels
 *        of a mesh.
 * @note If you got a vector of modifiers from frantic::max3d::collect_node_modifiers(),
 *        it will be in reverse order. Use std::reverse() before passing the vector to this
 *        function.
 *
 * @tparam ForwardIterator A type modeling the STL concept ForwardIterator
 * @param pcm The channel map for the particles
 * @param begin An iterator marking the beginning of the particle range
 * @param end An iterator marking the end of the particle range
 * @param noDefaultSelection True if the selection channel's initial value should be assumed 1.0
 * @param pTriObj A temporary TriObject that will be filled with the particle data, then passed
 *                 into the modifiers.
 * @param modifiers The collection of modifiers to run on each particle. It is important to have
 *                   the order in this list match the order in which the modifiers should be run.
 * @param pNode The node that these modifiers were applied to.
 * @param t The time at which to apply the modifiers.
 * @param timeStep The time at which to apply the modifiers again to compute the time derivative
 *                  using a O(1) finite difference (forward or backward).
 */
template <class ForwardIterator>
inline void apply_modifiers_to_particles( const frantic::channels::channel_map& pcm, ForwardIterator begin,
                                          ForwardIterator end, bool noDefaultSelection, TriObject* pTriObj,
                                          const std::vector<modifier_info_t> modifiers, INode* pNode, TimeValue t,
                                          TimeValue timeStep ) {
    using namespace frantic::channels;
    using namespace frantic::graphics;

    static const float OFFSET = 0.1f;

    channel_accessor<vector3f> posAccessor = pcm.get_accessor<vector3f>( _T("Position") );
    channel_cvt_accessor<vector3f> velAccessor( vector3f( 0 ) );
    channel_cvt_accessor<vector3f> normalAccessor( vector3f( 0 ) );
    channel_cvt_accessor<vector3f> tangentAccessor( vector3f( 0 ) );
    channel_cvt_accessor<float> selectionAccessor( 0 );

    if( pcm.has_channel( _T("Velocity") ) )
        velAccessor = pcm.get_cvt_accessor<vector3f>( _T("Velocity") );
    if( pcm.has_channel( _T("Normal") ) )
        normalAccessor = pcm.get_cvt_accessor<vector3f>( _T("Normal") );
    if( pcm.has_channel( _T("Tangent") ) )
        tangentAccessor = pcm.get_cvt_accessor<vector3f>( _T("Tangent") );
    if( pcm.has_channel( _T("Selection") ) )
        selectionAccessor = pcm.get_cvt_accessor<float>( _T("Selection") );

    int nParts = int( end - begin );

    float timeStepSeconds = timeStep / float( TIME_TICKSPERSEC );

    int nVerts = nParts;
    if( !normalAccessor.is_default() || !tangentAccessor.is_default() )
        nVerts += 6 * nParts;

    Mesh& theMesh = pTriObj->GetMesh();
    theMesh.setNumVerts( nVerts, FALSE, TRUE );
    theMesh.selLevel = MESH_OBJECT;

    if( !selectionAccessor.is_default() ) {
        if( !noDefaultSelection )
            theMesh.selLevel = MESH_VERTEX;
        theMesh.SupportVSelectionWeights();
    }

    int selLevel = theMesh.selLevel;

    int i = 0;
    for( ForwardIterator it = begin; it != end; ++it, ++i ) {
        vector3f position = posAccessor.get( *it );
        float selectionWeight = selectionAccessor.get( *it );

        theMesh.setVert( i, frantic::max3d::to_max_t( position ) );
        if( !selectionAccessor.is_default() )
            theMesh.getVSelectionWeights()[i] = selectionWeight;

        if( !normalAccessor.is_default() || !tangentAccessor.is_default() ) {
            vector3f n, t, b;

            if( !normalAccessor.is_default() && !tangentAccessor.is_default() ) {
                n = vector3f::normalize( normalAccessor.get( *it ) );
                t = vector3f::normalize( tangentAccessor.get( *it ) );
                // TODO: What should we do if the Normal and Tangent aren't orthogonal?
            } else if( !normalAccessor.is_default() ) {
                n = vector3f::normalize( normalAccessor.get( *it ) );
                if( std::abs( n.z ) <= std::abs( n.x ) ) {
                    t = vector3f::normalize( vector3f::cross(
                        vector3f( 0, 0, 1 ), n ) ); // By right-hand rule assuming n = [1,0,0], t = [0,1,0]
                } else {
                    t = vector3f::normalize( vector3f::cross(
                        n, vector3f( 1, 0, 0 ) ) ); // By right-hand rule assuming n = [0,0,1], t = [0,1,0]
                }
            } else {
                t = vector3f::normalize( tangentAccessor.get( *it ) );
                if( std::abs( t.z ) <= std::abs( t.y ) ) {
                    n = vector3f::normalize( vector3f::cross(
                        t, vector3f( 0, 0, 1 ) ) ); // By right-hand rule assuming t = [0,1,0], n = [1,0,0]
                } else {
                    n = vector3f::normalize( vector3f::cross(
                        vector3f( 0, 1, 0 ), t ) ); // By right-hand rule assuming t = [0,0,1], n = [1,0,0]
                }
            }

            b = vector3f::cross( n, t );

            theMesh.setVert( i + nParts, to_max_t( position + OFFSET * t ) );
            theMesh.setVert( i + 2 * nParts, to_max_t( position + OFFSET * b ) );
            theMesh.setVert( i + 3 * nParts, to_max_t( position - OFFSET * t ) );
            theMesh.setVert( i + 4 * nParts, to_max_t( position - OFFSET * b ) );
            theMesh.setVert( i + 5 * nParts, to_max_t( position + OFFSET * n ) );
            theMesh.setVert( i + 6 * nParts, to_max_t( position - OFFSET * n ) );
            if( !selectionAccessor.is_default() ) {
                for( int j = 1; j <= 6; ++j )
                    theMesh.getVSelectionWeights()[i + j * nParts] = selectionWeight;
            }
        }
    }

    // IMPORTANT!
    // This is assuming that the modifiers are in the vector in order of application. This is the
    // reverse of what you get when using the function, frantic::max3d::collect_node_modifiers().
    ObjectState os1 = pTriObj->Eval( t );
    for( std::vector<modifier_info_t>::const_iterator it = modifiers.begin(); it != modifiers.end(); ++it )
        it->first->ModifyObject( t, *( it->second ), &os1, pNode );

    if( theMesh.numVerts != nVerts )
        throw std::runtime_error(
            "apply_modifiers_to_particles() - A modifier created or deleted particles which is unsupported." );

    i = 0;
    for( ForwardIterator it = begin; it != end; ++it, ++i ) {
        // Store the old pos and selection in case we need to update the mesh for velocity tracking.
        vector3f oldPos = posAccessor.get( *it );
        float oldSelection = selectionAccessor.get( *it );

        // Read the new modified position from the mesh.
        posAccessor.get( *it ) = frantic::max3d::from_max_t( theMesh.getVert( i ) );

        if( !selectionAccessor.is_default() && theMesh.selLevel == MESH_VERTEX ) {
            if( theMesh.vDataSupport( VDATA_SELECT ) )
                selectionAccessor.set( *it, theMesh.getVSelectionWeights()[i] );
            else if( theMesh.vertSel[i] )
                selectionAccessor.set( *it, 1.f );
            else
                selectionAccessor.set( *it, 0.f );
        }

        if( !normalAccessor.is_default() || !tangentAccessor.is_default() ) {
            // Set the normal using the surface-area wighted average of the normals of
            // four triangles surrounding the point using the binormal and tangent.
            float angles[4];
            vector3f verts[7];
            vector3f norms[4];

            verts[0] = frantic::max3d::from_max_t( theMesh.getVert( i ) );
            verts[1] = frantic::max3d::from_max_t( theMesh.getVert( i + nParts ) );
            verts[2] = frantic::max3d::from_max_t( theMesh.getVert( i + 2 * nParts ) );
            verts[3] = frantic::max3d::from_max_t( theMesh.getVert( i + 3 * nParts ) );
            verts[4] = frantic::max3d::from_max_t( theMesh.getVert( i + 4 * nParts ) );
            verts[5] = frantic::max3d::from_max_t( theMesh.getVert( i + 5 * nParts ) );
            verts[6] = frantic::max3d::from_max_t( theMesh.getVert( i + 6 * nParts ) );

            angles[0] =
                frantic::max3d::geometry::get_face_corner_normal_and_angle( verts[2], verts[0], verts[1], norms[0] );
            angles[1] =
                frantic::max3d::geometry::get_face_corner_normal_and_angle( verts[3], verts[0], verts[2], norms[1] );
            angles[2] =
                frantic::max3d::geometry::get_face_corner_normal_and_angle( verts[4], verts[0], verts[3], norms[2] );
            angles[3] =
                frantic::max3d::geometry::get_face_corner_normal_and_angle( verts[1], verts[0], verts[4], norms[3] );

            vector3f n = vector3f::normalize( angles[0] * norms[0] + angles[1] * norms[1] + angles[2] * norms[2] +
                                              angles[3] * norms[3] );

            if( !normalAccessor.is_default() )
                normalAccessor.set( *it, n );

            if( !tangentAccessor.is_default() ) {
                // We will derive a BiNormal vector from the weighted sum of the cross of the modified Tangent and
                // Normal vectors. A Tangent vector will be derived from crossing the computed Normal vector above with
                // the BiNormal.
                angles[0] = frantic::max3d::geometry::get_face_corner_normal_and_angle( verts[1], verts[0], verts[5],
                                                                                        norms[0] );
                angles[1] = frantic::max3d::geometry::get_face_corner_normal_and_angle( verts[6], verts[0], verts[1],
                                                                                        norms[1] );
                angles[2] = frantic::max3d::geometry::get_face_corner_normal_and_angle( verts[3], verts[0], verts[6],
                                                                                        norms[2] );
                angles[3] = frantic::max3d::geometry::get_face_corner_normal_and_angle( verts[5], verts[0], verts[3],
                                                                                        norms[3] );

                vector3f b = vector3f::normalize( angles[0] * norms[0] + angles[1] * norms[1] + angles[2] * norms[2] +
                                                  angles[3] * norms[3] );

                vector3f t = vector3f::normalize( vector3f::cross( b, n ) );
                tangentAccessor.set( *it, t );
            }
        }

        if( !velAccessor.is_default() ) {
            theMesh.setVert( i, frantic::max3d::to_max_t( oldPos + timeStepSeconds * velAccessor.get( *it ) ) );

            // If a modifier sets the selection we should reset the selection to the original value so that the velocity
            // won't be affected by the selection caused by the current modifiers.
            if( !selectionAccessor.is_default() && theMesh.selLevel == MESH_VERTEX &&
                theMesh.vDataSupport( VDATA_SELECT ) )
                theMesh.getVSelectionWeights()[i] = oldSelection;
        }
    }

    if( !velAccessor.is_default() ) {
        theMesh.selLevel = selLevel;
        if( selLevel != MESH_VERTEX )
            theMesh.freeVSelectionWeights();

        theMesh.setNumVerts( nParts, TRUE );

        // IMPORTANT!
        // This is assuming that the modifiers are in the vector in order of application. This is the
        // reverse of what you get when using the function, frantic::max3d::collect_node_modifiers().
        ObjectState os2 = pTriObj->Eval( t );
        for( std::vector<modifier_info_t>::const_iterator it = modifiers.begin(); it != modifiers.end(); ++it )
            it->first->ModifyObject( t + timeStep, *( it->second ), &os2, pNode );

        if( theMesh.numVerts != nParts )
            throw std::runtime_error(
                "apply_modifiers_to_particles() - A modifier created or deleted particles which is unsupported." );

        int i = 0;
        for( ForwardIterator it = begin; it != end; ++it, ++i )
            velAccessor.set( *it, ( frantic::max3d::from_max_t( theMesh.getVert( i ) ) - posAccessor.get( *it ) ) /
                                      timeStepSeconds );
    }
}

template <class Iterator>
inline void apply_modifiers_to_particles( const frantic::channels::channel_map& pcm, Iterator begin, Iterator end,
                                          SClass_ID modifierType, INode* pNode, TimeValue t, TimeValue timeStep ) {
    Object* obj = pNode->GetObjOrWSMRef();

    // only objects with modifiers will have GEN_DERIVOB_CLASS_ID
    if( obj->SuperClassID() != GEN_DERIVOB_CLASS_ID )
        return;

    std::vector<frantic::max3d::modifier_info_t> modifiers;
    frantic::max3d::collect_node_modifiers( pNode, modifiers, modifierType );

    TriObject* pTriObj = (TriObject*)CreateInstance( GEOMOBJECT_CLASS_ID, triObjectClassID );
    detail::apply_modifiers_to_particles( pcm, begin, end, pTriObj, modifiers, pNode, t, timeStep );
    pTriObj->MaybeAutoDelete();
}
} // namespace detail

} // namespace particles
} // namespace max3d
} // namespace frantic
