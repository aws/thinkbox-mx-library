// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"
#include <frantic/max3d/geometry/max_mesh_interface.hpp>

//#pragma warning( push, 3 )
//#include <meshadj.h>
//#pragma warning( pop )

#undef NDEBUG
#include <cassert>

using namespace frantic::geometry;

namespace frantic {
namespace max3d {
namespace geometry {

void MaxMeshInterface::get_predefined_channels( std::vector<MaxMeshInterface::channel_info>& outChannels,
                                                mesh_channel::channel_type iterationType, bool forOutput ) {
    using namespace frantic::channels;

    switch( iterationType ) {
    case mesh_channel::vertex:
        outChannels.push_back( channel_info( _T("Position"), data_type_float32, 3, _T("Vertex Position") ) );
        if( !forOutput )
            outChannels.push_back( channel_info( _T("Normal"), data_type_float32, 3, _T("Vertex Average Normal") ) );
        outChannels.push_back(
            channel_info( _T("Selection"), data_type_float32, 1, _T("Vertex Soft-Selection Weight") ) );
        /* Moved these out since magma can do loops, so they shouldn't be built-in channels (in my opinion).
        if( !forOutput ){
                outChannels.push_back( channel_info( _T("SelectionFromFaceAvg"), data_type_float32, 1, _T("Vertex
        Selection Weight, from average of FaceSelection") ) ); outChannels.push_back( channel_info(
        _T("SelectionFromFaceUnion"), data_type_int8, 1, _T("Vertex Selection Weight, from union of FaceSelection") ) );
                outChannels.push_back( channel_info( _T("SelectionFromFaceIntersect"), data_type_int8, 1, _T("Vertex
        Selection Weight, from intersection of FaceSelection") ) );
        }*/
        break;
    case mesh_channel::face:
        if( !forOutput ) {
            outChannels.push_back( channel_info( _T("FaceNormal"), data_type_float32, 3, _T("Face Normal") ) );
            outChannels.push_back( channel_info( _T("FaceTangent"), data_type_float32, 3, _T("Face Tangent") ) );
            outChannels.push_back( channel_info( _T("FaceCenter"), data_type_float32, 3, _T("Face Center") ) );
            outChannels.push_back(
                channel_info( _T("FaceArea"), data_type_float32, 1, _T("Face Area in generic units squared") ) );
            outChannels.push_back(
                channel_info( _T("FaceMaxEdgeLength"), data_type_float32, 1, _T("Maximum length of face's edges") ) );
            outChannels.push_back(
                channel_info( _T("FaceElement"), data_type_int32, 1, _T("Index of element face is a member of") ) );
        }

        if( !forOutput )
            get_predefined_channels( outChannels, mesh_channel::element, false );

        outChannels.push_back( channel_info( _T("FaceSelection"), data_type_int8, 1, _T("Face Selection State") ) );
        outChannels.push_back( channel_info( _T("MtlIndex"), data_type_int32, 1, _T("Face Material ID") ) );
        outChannels.push_back( channel_info( _T("SmoothingGroup"), data_type_int32, 1, _T("Face Smoothing Groups") ) );
        break;
    case mesh_channel::face_vertex:
        if( !forOutput )
            get_predefined_channels( outChannels, mesh_channel::vertex, false );

        outChannels.push_back( channel_info( _T("Color"), data_type_float32, 3, _T("Vertex Color (Map Channel 0)") ) );
        outChannels.push_back(
            channel_info( _T("TextureCoord"), data_type_float32, 3, _T("Texture Coordinate (Map Channel 1)") ) );
        if( !forOutput )
            outChannels.push_back( channel_info( _T("SmoothNormal"), data_type_float32, 3, _T("Smoothed Normal") ) );

        if( !forOutput )
            get_predefined_channels( outChannels, mesh_channel::face, false );
        break;
    case mesh_channel::element:
        if( !forOutput ) {
            outChannels.push_back( channel_info( _T("FaceElementArea"), data_type_float32, 1,
                                                 _T("Area of element that face is a member of") ) );
            outChannels.push_back( channel_info(
                _T("FaceElementVolume"), data_type_float32, 1,
                _T("Volume of element that face is a member of. Only valid if the element is closed.") ) );
            outChannels.push_back( channel_info(
                _T("FaceElementCentroid"), data_type_float32, 3,
                _T("Center of mass of element that face is a member of. Only valid if the element is closed.") ) );
        }
        break;
    };
}

// Access texture mapping information per-face-per-vertex.
class MappingAccessor : public frantic::geometry::mesh_channel {
    // NOTE: MeshMaps can be deleted when adding new channels to the mesh, so we can't store a ptr/ref directly.
    Mesh* m_mesh;
    int m_mapChan;

    inline static frantic::tstring make_name( int mapChan ) {
        if( mapChan == 0 )
            return _T("Color");
        else if( mapChan == 1 )
            return _T("TextureCoord");
        else
            return _T("Mapping") + boost::lexical_cast<frantic::tstring>( mapChan );
    }

  public:
    MappingAccessor( Mesh* mesh, int mapChan )
        : frantic::geometry::mesh_channel( make_name( mapChan ), mesh_channel::face_vertex,
                                           frantic::channels::data_type_float32, 3, mesh->Map( mapChan ).getNumVerts(),
                                           mesh->Map( mapChan ).getNumFaces(), false )
        , m_mesh( mesh )
        , m_mapChan( mapChan ) {
        assert( m_mesh != NULL );
        assert( m_mapChan < MAX_MESHMAPS && m_mapChan >= -NUM_HIDDENMAPS );
        assert( m_mesh->mapSupport( m_mapChan ) );
        assert( &m_mesh->Map( m_mapChan ) != NULL );
        assert( m_mesh->Map( m_mapChan ).IsUsed() );
        assert( m_mesh->Map( m_mapChan ).tf != NULL || m_mesh->Map( m_mapChan ).getNumFaces() == 0 );
        assert( m_mesh->Map( m_mapChan ).tv != NULL || m_mesh->Map( m_mapChan ).getNumVerts() == 0 );
    }

    void make_writeable() {
        MeshMap& m = m_mesh->Map( m_mapChan );

        // They need to have one element per face-vertex in order to write to them without clobbering writes from
        // previous iterations.
        if( m.getNumVerts() != 3 * m_mesh->getNumFaces() || m.getNumFaces() != m_mesh->getNumFaces() ) {
            MeshMap newMap;
            newMap.flags = m.flags; // Keep the flags
            newMap.setNumFaces( m_mesh->getNumFaces() );
            newMap.setNumVerts( 3 * m_mesh->getNumFaces() );

            // We can only initialize the new layout if the old one made sense (ie. There was a correlation between
            // geometry faces and the map faces).
            if( m.getNumFaces() == m_mesh->getNumFaces() ) {
                for( int i = 0, iEnd = m_mesh->getNumFaces(); i < iEnd; ++i ) {
                    newMap.tf[i].setTVerts( 3 * i, 3 * i + 1, 3 * i + 2 );
                    newMap.tv[3 * i] = m.tv[m.tf[i].getTVert( 0 )];
                    newMap.tv[3 * i + 1] = m.tv[m.tf[i].getTVert( 1 )];
                    newMap.tv[3 * i + 2] = m.tv[m.tf[i].getTVert( 2 )];
                }
            }

            m.SwapContents( newMap );

            // We changed the number of valid verts in this channel, so update the base object which tracks the count.
            this->set_num_elements( static_cast<std::size_t>( m.getNumVerts() ) );
        } else {
            // TODO: Actually confirm that the faces map to unique indices ... We might miss some invalid map layouts
            // currently.
        }
    }

    virtual void get_value( std::size_t index, void* outValue ) const {
        assert( index < static_cast<DWORD>( m_mesh->Map( m_mapChan ).getNumVerts() ) );

        *reinterpret_cast<Point3*>( outValue ) = m_mesh->Map( m_mapChan ).tv[index];
    }

    virtual void set_value( std::size_t index, const void* value ) const {
        assert( index < static_cast<DWORD>( m_mesh->Map( m_mapChan ).getNumVerts() ) );

        m_mesh->Map( m_mapChan ).tv[index] = *reinterpret_cast<const Point3*>( value );
    }

    virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t fvertIndex ) const {
        assert( faceIndex < static_cast<DWORD>( m_mesh->Map( m_mapChan ).getNumFaces() ) );
        assert( fvertIndex < 3 );
        assert( m_mesh->Map( m_mapChan ).tf[faceIndex].getTVert( (int)fvertIndex ) <
                static_cast<DWORD>( m_mesh->Map( m_mapChan ).getNumVerts() ) );

        return m_mesh->Map( m_mapChan ).tf[faceIndex].getTVert( (int)fvertIndex );
    }

    virtual std::size_t get_num_face_verts( std::size_t /*faceIndex*/ ) const { return 3; }
};

void MaxMeshInterface::set_mesh( Mesh* theMesh, bool takeOwnership ) {
    using frantic::geometry::mesh_channel;

    if( m_ownedTriObject )
        m_triObject->MaybeAutoDelete();

    m_triObject = NULL;
    m_ownedTriObject = false;

    if( theMesh != m_mesh ) {
        if( m_mesh && m_ownedMesh )
            m_mesh->DeleteThis();
        m_mesh = theMesh;
        m_tempData.SetMesh( m_mesh );
        m_ownedMesh = takeOwnership;

        this->reset();

        if( m_mesh ) {
            std::unique_ptr<VertexPositionAccessor> pos( new VertexPositionAccessor( m_mesh ) );
            this->append_vertex_channel( std::move( pos ) );

            std::unique_ptr<FaceMtlIndexAccessor> mtlIndex( new FaceMtlIndexAccessor( m_mesh ) );
            std::unique_ptr<FaceSmoothingGroupAccessor> smGroup( new FaceSmoothingGroupAccessor( m_mesh ) );
            std::unique_ptr<FaceNormalAccessor> faceNormal( new FaceNormalAccessor( m_mesh ) );
            std::unique_ptr<FaceCenterAccessor> faceCenter( new FaceCenterAccessor( m_mesh ) );
            std::unique_ptr<FaceAreaAccessor> faceArea( new FaceAreaAccessor( m_mesh ) );
            std::unique_ptr<FaceMaxEdgeLengthAccessor> faceMaxEdgeLength( new FaceMaxEdgeLengthAccessor( m_mesh ) );
            std::unique_ptr<FaceSelectionAccessor> faceSelection( new FaceSelectionAccessor( m_mesh ) );

            this->append_face_channel( std::move( mtlIndex ) );
            this->append_face_channel( std::move( smGroup ) );
            this->append_face_channel( std::move( faceNormal ) );
            this->append_face_channel( std::move( faceCenter ) );
            this->append_face_channel( std::move( faceArea ) );
            this->append_face_channel( std::move( faceMaxEdgeLength ) );
            this->append_face_channel( std::move( faceSelection ) );

            if( m_mesh->mapSupport( 0 ) ) {
                std::unique_ptr<MappingAccessor> map( new MappingAccessor( m_mesh, 0 ) );

                this->append_vertex_channel( std::move( map ) );
            }

            if( m_mesh->mapSupport( 1 ) ) {
                std::unique_ptr<MappingAccessor> map( new MappingAccessor( m_mesh, 1 ) );

                this->append_vertex_channel( std::move( map ) );
            }

            // For the face tangent, we use the "natural" texture coordinates which more or less end up being Map
            // channel 1.
            if( m_mesh->tvFace && m_mesh->tVerts && m_mesh->numTVerts > 0 ) {
                std::unique_ptr<FaceTangentAccessor> faceTangent( new FaceTangentAccessor( m_mesh ) );

                this->append_face_channel( std::move( faceTangent ) );
            }

            for( int i = 2; i < MAX_MESHMAPS; ++i ) {
                if( m_mesh->mapSupport( i ) ) {
                    std::unique_ptr<MappingAccessor> map( new MappingAccessor( m_mesh, i ) );

                    this->append_vertex_channel( std::move( map ) );
                }
            }
        } // if( m_mesh )
    }     // if( m_mesh != theMesh )
}

void MaxMeshInterface::set_tri_object( TriObject* pTriObject, bool takeOwnership ) {
    if( pTriObject ) {
        this->set_mesh( &pTriObject->GetMesh(), false );

        m_triObject = pTriObject;
        m_ownedTriObject = takeOwnership;
    } else {
        this->set_mesh( NULL, false );
    }
}

void MaxMeshInterface::commit_writes() {}

bool MaxMeshInterface::is_valid() const { return m_mesh != NULL; }

bool MaxMeshInterface::request_channel( const frantic::tstring& channelName, bool vertexChannel, bool forOutput,
                                        bool throwOnError ) {
    using frantic::geometry::mesh_channel;

    bool result = false;

    if( vertexChannel ) {
        const mesh_channel* ch = this->get_vertex_channels().get_channel( channelName );

        // See if we have already populated this channel.
        if( ch != NULL ) {
            // Some channels do not exist for output purposes, but we can always read from them if its already
            // populated.
            if( !forOutput )
                result = true;
            else if( ch->is_writeable() ) {
                if( ch->get_channel_type() == mesh_channel::vertex ) {
                    result = true;
                } else { // ch->get_channel_type() == mesh_channel::face_vertex

                    if( const MappingAccessor* chMap = dynamic_cast<const MappingAccessor*>( ch ) ) {
                        // A little sketchy ...
                        const_cast<MappingAccessor*>( chMap )->make_writeable();

                        result = true;
                    }
                }
            }
        } else {
            // There are some channels that are only populated when we go to use them.
            if( channelName == _T("Selection") ) {
                init_selection();

                std::unique_ptr<VertexSelectionAccessor> sel( new VertexSelectionAccessor( m_mesh ) );

                this->append_vertex_channel( std::move( sel ) );

                result = true;
            } else if( channelName == _T("SelectionFromFaceAvg") ) {
                if( !forOutput ) {
                    std::unique_ptr<FromFacesAccessor<accumulate_average>> sel(
                        new FromFacesAccessor<accumulate_average>( m_mesh, m_tempData.AdjEList(),
                                                                   _T("SelectionFromFaceAvg") ) );

                    this->append_vertex_channel( std::move( sel ) );

                    result = true;
                }
            } else if( channelName == _T("SelectionFromFaceUnion") ) {
                if( !forOutput ) {
                    std::unique_ptr<FromFacesAccessor<accumulate_union>> sel( new FromFacesAccessor<accumulate_union>(
                        m_mesh, m_tempData.AdjEList(), _T("SelectionFromFaceUnion") ) );

                    this->append_vertex_channel( std::move( sel ) );

                    result = true;
                }
            } else if( channelName == _T("SelectionFromFaceIntersect") ) {
                if( !forOutput ) {
                    std::unique_ptr<FromFacesAccessor<accumulate_intersection>> sel(
                        new FromFacesAccessor<accumulate_intersection>( m_mesh, m_tempData.AdjEList(),
                                                                        _T("SelectionFromFaceIntersect") ) );

                    this->append_vertex_channel( std::move( sel ) );

                    result = true;
                }
            } else if( channelName == _T("VertexEdgeCount") ) {
                if( !forOutput ) {
                    std::unique_ptr<VertexEdgeCountAccessor> acc(
                        new VertexEdgeCountAccessor( m_mesh, m_tempData.AdjEList() ) );

                    this->append_vertex_channel( std::move( acc ) );

                    result = true;
                }
            } else if( channelName == _T("VertexFaceCount") ) {
                if( !forOutput ) {
                    std::unique_ptr<VertexFaceCountAccessor> acc(
                        new VertexFaceCountAccessor( m_mesh, m_tempData.AdjEList() ) );

                    this->append_vertex_channel( std::move( acc ) );

                    result = true;
                }
            } else if( channelName == _T("Normal") ) {
                // Only available for input
                if( !forOutput ) {
                    std::unique_ptr<VertexNormalAccessor> norm(
                        new VertexNormalAccessor( m_mesh, *m_tempData.VertexNormals() ) );

                    this->append_vertex_channel( std::move( norm ) );

                    result = true;
                }
            } else if( channelName == _T("SmoothNormal") ) {
                // Only available for input
                if( !forOutput ) {
                    m_mesh->buildNormals();

                    std::unique_ptr<SmoothNormalAccessor> smNorm( new SmoothNormalAccessor( m_mesh ) );

                    this->append_vertex_channel( std::move( smNorm ) );

                    result = true;
                }
            } else {
                int mapChan = MAX_MESHMAPS;
                if( channelName == _T("Color") )
                    mapChan = 0;
                else if( channelName == _T("TextureCoord") )
                    mapChan = 1;
                else if( channelName.substr( 0, 7 ) == _T("Mapping") ) {
                    mapChan = boost::lexical_cast<int>( channelName.substr( 7 ) );
                }

                if( mapChan >= 0 && mapChan < MAX_MESHMAPS ) {
                    if( forOutput ) {
                        m_mesh->setMapSupport( mapChan, TRUE );

                        MeshMap& m = m_mesh->Map( mapChan );
                        m.setNumFaces( m_mesh->getNumFaces() );
                        m.setNumVerts( 3 * m_mesh->getNumFaces() );

                        for( int i = 0, iEnd = m.getNumFaces(); i < iEnd; ++i )
                            m.tf[i].setTVerts( 3 * i, 3 * i + 1, 3 * i + 2 );

                        std::unique_ptr<MappingAccessor> map( new MappingAccessor( m_mesh, mapChan ) );

                        this->append_vertex_channel( std::move( map ) );

                        result = true;
                    }
                }
            }
        }
    } else {
        if( const mesh_channel* ch = this->get_face_channels().get_channel( channelName ) ) {
            if( !forOutput || ch->is_writeable() )
                result = true;
        } else {
            if( channelName == _T("FaceElement") ) {
                if( !forOutput ) {
                    init_elements();

                    std::unique_ptr<FaceElementAccessor> sel( new FaceElementAccessor( m_mesh, m_faceElems.get() ) );

                    this->append_face_channel( std::move( sel ) );

                    result = true;
                }
            } else if( channelName == _T("FaceElementArea") ) {
                if( !forOutput ) {
                    init_elements();

                    std::unique_ptr<FaceElementAreaAccessor> sel(
                        new FaceElementAreaAccessor( m_mesh, m_faceElems.get(), m_numElems, m_elemData.get() ) );

                    this->append_face_channel( std::move( sel ) );

                    result = true;
                }
            } else if( channelName == _T("FaceElementVolume") ) {
                if( !forOutput ) {
                    init_elements();

                    std::unique_ptr<FaceElementVolumeAccessor> sel(
                        new FaceElementVolumeAccessor( m_mesh, m_faceElems.get(), m_numElems, m_elemData.get() ) );

                    this->append_face_channel( std::move( sel ) );

                    result = true;
                }
            } else if( channelName == _T("FaceElementCentroid") ) {
                if( !forOutput ) {
                    init_elements();

                    std::unique_ptr<FaceElementCentroidAccessor> sel(
                        new FaceElementCentroidAccessor( m_mesh, m_faceElems.get(), m_numElems, m_elemData.get() ) );

                    this->append_face_channel( std::move( sel ) );

                    result = true;
                }
                /*}else if( channelName == _T("FaceNeighbors") ){
                        if( !forOutput ){
                                std::unique_ptr< FaceNeighborAccessor > acc( new FaceNeighborAccessor( m_mesh,
                   m_tempData.AdjFList(), _T("FaceNeighbors") ) );

                                this->append_face_channel( acc );

                                result = true;
                        }*/
            } else if( channelName == _T("FaceEdgeCount") ) {
                if( !forOutput ) {
                    std::unique_ptr<FaceEdgeCountAccessor> acc( new FaceEdgeCountAccessor( m_mesh ) );

                    this->append_face_channel( std::move( acc ) );

                    result = true;
                }
            }
        }
    }

    if( throwOnError && !result )
        throw std::runtime_error( "MaxMeshInterface::request_channel() Failed to add channel: \"" +
                                  frantic::strings::to_string( channelName ) + "\"" );

    return result;
}

struct vertex_iterator_impl {
    std::size_t vertexIndex, iterIndex;
};

struct face_iterator_impl {
    std::size_t faceIndex, iterIndex;
};

BOOST_STATIC_ASSERT( sizeof( vertex_iterator_impl ) <= frantic::geometry::detail::iterator_storage_size );
BOOST_STATIC_ASSERT( sizeof( face_iterator_impl ) <= frantic::geometry::detail::iterator_storage_size );

void MaxMeshInterface::init_adjacency() {
    if( !m_adjAllocated ) {
        m_tempData.AdjEList(); //->OrderAllEdges( m_mesh->faces ); //Edges will be in counter-clockwise order. Will the
                               //faces be ordered left-right?
        m_tempData.AdjFList();
        m_adjAllocated = true;
    }
}

bool MaxMeshInterface::has_adjacency() const { return m_adjAllocated; }

bool MaxMeshInterface::init_vertex_iterator( vertex_iterator& vIt, std::size_t vertexIndex ) const {
    assert( m_adjAllocated && "Adjacency data must be allocated via init_iterator_data() before being used" );
    vertex_iterator_impl& it = *static_cast<vertex_iterator_impl*>( vIt.m_data.address() );
    it.vertexIndex = vertexIndex;
    it.iterIndex = 0;
    return m_tempData.AdjEList()->list[it.vertexIndex].Count() > 0;
}

bool MaxMeshInterface::advance_vertex_iterator( vertex_iterator& vIt ) const {
    vertex_iterator_impl& it = *static_cast<vertex_iterator_impl*>( vIt.m_data.address() );
    it.iterIndex = ( it.iterIndex + 1 ) % m_tempData.AdjEList()->list[it.vertexIndex].Count();
    return it.iterIndex > 0; // When it becomes 0 again, we have completed the full loop.
}

std::size_t MaxMeshInterface::get_edge_endpoint( vertex_iterator& vIt ) const {
    vertex_iterator_impl& it = *static_cast<vertex_iterator_impl*>( vIt.m_data.address() );
    return m_tempData.AdjEList()->edges[m_tempData.AdjEList()->list[it.vertexIndex][it.iterIndex]].OtherVert(
        static_cast<DWORD>( it.vertexIndex ) );
}

// Hopefully the faces are consistently ordered!
std::size_t MaxMeshInterface::get_edge_left_face( vertex_iterator& vIt ) const {
    vertex_iterator_impl& it = *static_cast<vertex_iterator_impl*>( vIt.m_data.address() );

    const MEdge& edge = m_tempData.AdjEList()->edges[m_tempData.AdjEList()->list[it.vertexIndex][it.iterIndex]];

    // The edge vertices are not ordered in any particular way so we need to check both sides (ie faces f[0], f[1]) to
    // find the directed edge that originates from the vertex we are interested in. The faces are defined in order (ie.
    // v[0] -> v[1], v[1] -> v[2], v[2] -> v[0], so the left face has our vertex at v[eidx].
    if( edge.f[0] != UNDEFINED ) {
        int eidx = const_cast<MEdge&>( edge ).EdgeIndex( m_mesh->faces, 0 );
        if( m_mesh->faces[edge.f[0]].v[eidx] ==
            it.vertexIndex ) // Does the edge originate on the vertex we are interested in?
            return static_cast<std::size_t>( edge.f[0] );
    }
    if( edge.f[1] != UNDEFINED ) {
        int eidx = const_cast<MEdge&>( edge ).EdgeIndex( m_mesh->faces, 1 );
        if( m_mesh->faces[edge.f[1]].v[eidx] ==
            it.vertexIndex ) // Does the edge originate on the vertex we are interested in?
            return static_cast<std::size_t>( edge.f[1] );
    }
    return static_cast<std::size_t>( -1 );
}

std::size_t MaxMeshInterface::get_edge_right_face( vertex_iterator& vIt ) const {
    vertex_iterator_impl& it = *static_cast<vertex_iterator_impl*>( vIt.m_data.address() );

    const MEdge& edge = m_tempData.AdjEList()->edges[m_tempData.AdjEList()->list[it.vertexIndex][it.iterIndex]];

    // The edge vertices are not ordered in any particular way so we need to check both sides (ie faces f[0], f[1]) to
    // find the directed edge that originates from the vertex we are interested in. The faces are defined in order (ie.
    // v[0] -> v[1], v[1] -> v[2], v[2] -> v[0], so the right face has our vertex at v[(eidx+1)%3].
    if( edge.f[0] != UNDEFINED ) {
        int eidx = const_cast<MEdge&>( edge ).EdgeIndex( m_mesh->faces, 0 );
        if( m_mesh->faces[edge.f[0]].v[( eidx + 1 ) % 3] ==
            it.vertexIndex ) // Does the edge originate on the vertex we are interested in?
            return static_cast<std::size_t>( edge.f[0] );
    }
    if( edge.f[1] != UNDEFINED ) {
        int eidx = const_cast<MEdge&>( edge ).EdgeIndex( m_mesh->faces, 1 );
        if( m_mesh->faces[edge.f[1]].v[( eidx + 1 ) % 3] ==
            it.vertexIndex ) // Does the edge originate on the vertex we are interested in?
            return static_cast<std::size_t>( edge.f[1] );
    }
    return static_cast<std::size_t>( -1 );
}

bool MaxMeshInterface::is_edge_visible( vertex_iterator& vIt ) const {
    vertex_iterator_impl& it = *static_cast<vertex_iterator_impl*>( vIt.m_data.address() );
    return m_tempData.AdjEList()->edges[m_tempData.AdjEList()->list[it.vertexIndex][it.iterIndex]].Visible(
               m_mesh->faces ) != FALSE;
}

bool MaxMeshInterface::is_edge_boundary( frantic::geometry::vertex_iterator& vIt ) const {
    vertex_iterator_impl& it = *static_cast<vertex_iterator_impl*>( vIt.m_data.address() );

    const MEdge& edge = m_tempData.AdjEList()->edges[m_tempData.AdjEList()->list[it.vertexIndex][it.iterIndex]];

    return edge.f[0] == UNDEFINED || edge.f[1] == UNDEFINED;
}

void MaxMeshInterface::init_face_iterator( face_iterator& fIt, std::size_t faceIndex ) const {
    assert( m_adjAllocated && "Adjacency data must be allocated via init_iterator_data() before being used" );
    face_iterator_impl& it = *static_cast<face_iterator_impl*>( fIt.m_data.address() );
    it.faceIndex = faceIndex;
    it.iterIndex = 0;
}

bool MaxMeshInterface::advance_face_iterator( face_iterator& fIt ) const {
    face_iterator_impl& it = *static_cast<face_iterator_impl*>( fIt.m_data.address() );
    it.iterIndex = ( it.iterIndex + 1 ) % 3;
    return it.iterIndex > 0;
}

std::size_t MaxMeshInterface::get_face_neighbor( face_iterator& fIt ) const {
    face_iterator_impl& it = *static_cast<face_iterator_impl*>( fIt.m_data.address() );
    DWORD result = m_tempData.AdjFList()->list[it.faceIndex].f[it.iterIndex];
    if( result == UNDEFINED )
        return static_cast<std::size_t>( -1 );
    return static_cast<std::size_t>( result );
}

std::size_t MaxMeshInterface::get_face_next_vertex( frantic::geometry::face_iterator& /*fIt*/ ) const {
    assert( m_adjAllocated );

    throw std::runtime_error(
        "MaxMeshInterface::get_face_next_vertex : Error, not implemented for 3dsMax mesh interface." );

    // This seems like a reasonable guess, but I have yet to verify this correspondance is actually true
    // face_iterator_impl& it = *static_cast<face_iterator_impl*>( fIt.m_data.address() );
    // m_mesh->faces[it.faceIndex].getVert( (int)(it.iterIndex + 1) % 3 );
}

std::size_t MaxMeshInterface::get_face_prev_vertex( frantic::geometry::face_iterator& /*fIt*/ ) const {
    assert( m_adjAllocated );

    throw std::runtime_error(
        "MaxMeshInterface::get_face_next_vertex : Error, not implemented for 3dsMax mesh interface." );

    // This seems like a reasonable guess, but I have yet to verify this correspondance is actually true
    // face_iterator_impl& it = *static_cast<face_iterator_impl*>( fIt.m_data.address() );
    // m_mesh->faces[it.faceIndex].getVert( (int)it.iterIndex );
}

} // namespace geometry
} // namespace max3d
} // namespace frantic
