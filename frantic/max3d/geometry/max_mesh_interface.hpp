// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/convert.hpp>

#include <frantic/channels/named_channel_data.hpp>
#include <frantic/geometry/mesh_interface.hpp>
#include <frantic/geometry/triangle_utils.hpp>
#include <frantic/shading/highlights.hpp>

#include <mesh.h>
#include <meshadj.h>
#if MAX_RELEASE >= 25000
#include <geom/gutil.h>
#else
#include <gutil.h>
#endif

#include <boost/cstdint.hpp>
#include <boost/scoped_array.hpp>
#include <boost/tuple/tuple.hpp>

#include <tbb/spin_mutex.h>

// Include last so assert() doesn't accidentally get redefined
#include <frantic/misc/hybrid_assert.hpp>

#pragma region Macros
#define DEFINE_VERTEX_ACCESSOR( name, type, arity, readOnly, transformType )                                           \
    class Vertex##name##Accessor : public frantic::geometry::mesh_channel {                                            \
        Mesh* m_mesh;                                                                                                  \
                                                                                                                       \
      public:                                                                                                          \
        virtual std::size_t get_num_face_verts( std::size_t /*faceIndex*/ ) const { return 3; }                        \
        Vertex##name##Accessor( Mesh* mesh )                                                                           \
            : frantic::geometry::mesh_channel( _T( #name ), mesh_channel::vertex, type, arity, mesh->getNumVerts(),    \
                                               mesh->getNumFaces(), readOnly )                                         \
            , m_mesh( mesh ) {                                                                                         \
            frantic::geometry::mesh_channel::set_transform_type( transformType );                                      \
        }

#define DEFINE_FACE_ACCESSOR( name, prefix, type, arity, readOnly, transformType )                                     \
    class Face##name##Accessor : public frantic::geometry::mesh_channel {                                              \
        Mesh* m_mesh;                                                                                                  \
                                                                                                                       \
      public:                                                                                                          \
        virtual std::size_t get_num_face_verts( std::size_t /*faceIndex*/ ) const { return 1; }                        \
        Face##name##Accessor( Mesh* mesh )                                                                             \
            : frantic::geometry::mesh_channel( prefix _T( #name ), mesh_channel::face, type, arity,                    \
                                               mesh->getNumFaces(), mesh->getNumFaces(), readOnly )                    \
            , m_mesh( mesh ) {                                                                                         \
            frantic::geometry::mesh_channel::set_transform_type( transformType );                                      \
        }
#pragma endregion

namespace frantic {
namespace max3d {
namespace geometry {

/**
 * This class implements the abstract mesh_interface from the Frantic Library. You can use it to do mesh manipulation
 * and querying without being aware you are working with a 3ds Max mesh.
 */
class MaxMeshInterface : public frantic::geometry::mesh_interface {
    TriObject* m_triObject; // Will be non-null if this object was initialized with a TriObject instead of directly with
                            // a mesh.
    bool m_ownedTriObject; // Means we need to call m_triObject->MaybeAutoDelete() when done.

    Mesh* m_mesh;
    mutable MeshTempData m_tempData;
    bool m_ownedMesh; // Means we need to delete the mesh when we are done.
    bool m_adjAllocated;

    boost::scoped_array<DWORD> m_faceElems; // One entry per-face, mapping from face index to element index.

    struct element_data {
        float m_area, m_volume;
        Point3 m_centroid;

        inline element_data()
            : m_area( 0.f )
            , m_volume( 0.f )
            , m_centroid( 0.f, 0.f, 0.f ) {}

        inline float get_area() const { return m_area; }
        inline float get_volume() const { return m_volume; }
        inline const Point3& get_centroid() const { return m_centroid; }

        inline void add_face( Point3 ( &tri )[3] ) {
            Point3 fN = CrossProd( tri[1] - tri[0], tri[2] - tri[0] );
            float dV = DotProd( tri[0], fN );

            m_area += Length( fN );
            m_volume += dV;
            m_centroid += dV * ( tri[0] + tri[1] + tri[2] );
        }

        inline void finish() {
            m_area *= 0.5f;
            m_volume /= 6.f;
            m_centroid = m_centroid / ( 24.f * m_volume );
        }
    };

    std::size_t m_numElems;
    boost::scoped_array<element_data> m_elemData; // One entry per-element providing the per-element information.

    // Vertex accessor that provides the position of each vertex in space.
    DEFINE_VERTEX_ACCESSOR( Position, frantic::channels::data_type_float32, 3, false,
                            frantic::geometry::mesh_channel::transform_type::point )
    virtual void get_value( std::size_t index, void* outValue ) const {
        *reinterpret_cast<Point3*>( outValue ) = m_mesh->getVert( (int)index );
    }
    virtual void set_value( std::size_t index, const void* value ) const {
        m_mesh->setVert( (int)index, *reinterpret_cast<const Point3*>( value ) );
    }
    virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t fvertIndex ) const {
        return m_mesh->faces[faceIndex].getVert( (int)fvertIndex );
    }
};

// Vertex accessor that provides the weighted selection of a vertex.
DEFINE_VERTEX_ACCESSOR( Selection, frantic::channels::data_type_float32, 1, false,
                        frantic::geometry::mesh_channel::transform_type::none )
virtual void get_value( std::size_t index, void* outValue ) const {
    *reinterpret_cast<float*>( outValue ) = m_mesh->getVSelectionWeights()[index];
}
virtual void set_value( std::size_t index, const void* value ) const {
    float floatVal = *reinterpret_cast<const float*>( value );

    m_mesh->getVSelectionWeights()[(int)index] = floatVal;

    // We can't set the BitArray directly here, due to the nature of the compression making it thread-unsafe. We gotta
    // fix it later. if( floatVal > 0.f ) 	m_mesh->VertSel().Set( (int)index ); else 	m_mesh->VertSel().Clear( (int)index
    //);
}
virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t fvertIndex ) const {
    return m_mesh->faces[faceIndex].getVert( (int)fvertIndex );
}
};

// Vertex accessor that provides the face-angle weighted average of the smooth normals at a specific vertex.
class VertexNormalAccessor : public frantic::geometry::mesh_channel {
    Mesh* m_mesh;
    Tab<Point3>* m_vertexNormals;

  public:
    VertexNormalAccessor( Mesh* mesh, Tab<Point3>& vertexNormals )
        : frantic::geometry::mesh_channel( _T("Normal"), mesh_channel::vertex, frantic::channels::data_type_float32, 3,
                                           mesh->getNumVerts(), mesh->getNumFaces(), true )
        , m_mesh( mesh )
        , m_vertexNormals( &vertexNormals ) {
        set_transform_type( transform_type::normal );
    }

    virtual void get_value( std::size_t index, void* outValue ) const {
        *reinterpret_cast<Point3*>( outValue ) = ( *m_vertexNormals )[index];
    }
    virtual void set_value( std::size_t /*index*/, const void* /*value*/ ) const {}
    virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t fvertIndex ) const {
        return m_mesh->faces[faceIndex].getVert( (int)fvertIndex );
    }
    virtual std::size_t get_num_face_verts( std::size_t /*faceIndex*/ ) const { return 3; }
};

// Face accessor that provides the material id of each face
DEFINE_FACE_ACCESSOR( MtlIndex, _T(""), frantic::channels::data_type_int32, 1, false,
                      frantic::geometry::mesh_channel::transform_type::none )
virtual void get_value( std::size_t index, void* outValue ) const {
    *reinterpret_cast<int*>( outValue ) = (int)m_mesh->getFaceMtlIndex( (int)index );
}
virtual void set_value( std::size_t index, const void* value ) const {
    m_mesh->setFaceMtlIndex( (int)index, ( MtlID ) * reinterpret_cast<const int*>( value ) );
}
virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t /*fvertIndex*/ ) const { return faceIndex; }
};

// Face accessor that provides the smoothing group membership of each face (represented as 32 bit integer).
DEFINE_FACE_ACCESSOR( SmoothingGroup, _T(""), frantic::channels::data_type_int32, 1, false,
                      frantic::geometry::mesh_channel::transform_type::none )
virtual void get_value( std::size_t index, void* outValue ) const {
    *reinterpret_cast<int*>( outValue ) = (int)m_mesh->faces[index].getSmGroup();
}
virtual void set_value( std::size_t index, const void* value ) const {
    m_mesh->faces[index].setSmGroup( ( DWORD ) * reinterpret_cast<const int*>( value ) );
}
virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t /*fvertIndex*/ ) const { return faceIndex; }
};

// Face accessor that provides readonly access to the geometric normal of a face
DEFINE_FACE_ACCESSOR( Normal, _T("Face"), frantic::channels::data_type_float32, 3, true,
                      frantic::geometry::mesh_channel::transform_type::normal )
virtual void get_value( std::size_t index, void* outValue ) const {
    *reinterpret_cast<Point3*>( outValue ) = m_mesh->FaceNormal( (DWORD)index, TRUE );
}
virtual void set_value( std::size_t /*index*/, const void* /*value*/ ) const {}
virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t /*fvertIndex*/ ) const { return faceIndex; }
}
;

// Face accessor that provides readonly access to the max edge length of a face
DEFINE_FACE_ACCESSOR( Tangent, _T("Face"), frantic::channels::data_type_float32, 3, true,
                      frantic::geometry::mesh_channel::transform_type::vector )
virtual void get_value( std::size_t index, void* outValue ) const {
    using frantic::graphics::vector3f;
    using frantic::max3d::from_max_t;

    Face& f = m_mesh->faces[index];
    vector3f tri[] = { from_max_t( m_mesh->getVert( f.getVert( 0 ) ) ), from_max_t( m_mesh->getVert( f.getVert( 1 ) ) ),
                       from_max_t( m_mesh->getVert( f.getVert( 2 ) ) ) };

    TVFace& tvf = m_mesh->tvFace[index];
    UVVert tvs[] = { m_mesh->tVerts[tvf.getTVert( 0 )], m_mesh->tVerts[tvf.getTVert( 1 )],
                     m_mesh->tVerts[tvf.getTVert( 2 )] };

    float uvs[][2] = { { tvs[0].x, tvs[0].y }, { tvs[1].x, tvs[1].y }, { tvs[2].x, tvs[2].y } };

    vector3f dPdu, dPdv;
    if( !frantic::geometry::compute_dPduv( tri, uvs, dPdu, dPdv ) ) {
        vector3f n = vector3f::normalize( vector3f::cross( tri[1] - tri[0], tri[2] - tri[0] ) );

        *reinterpret_cast<vector3f*>( outValue ) = frantic::shading::compute_tangent( n );
    } else {
        *reinterpret_cast<vector3f*>( outValue ) = vector3f::normalize( dPdu );
    }
}
virtual void set_value( std::size_t /*index*/, const void* /*value*/ ) const {}
virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t /*fvertIndex*/ ) const { return faceIndex; }
}
;

// Face accessor that provides readonly access to the geometric center of a face
DEFINE_FACE_ACCESSOR( Center, _T("Face"), frantic::channels::data_type_float32, 3, true,
                      frantic::geometry::mesh_channel::transform_type::point )
virtual void get_value( std::size_t index, void* outValue ) const {
    *reinterpret_cast<Point3*>( outValue ) = m_mesh->FaceCenter( (DWORD)index );
}
virtual void set_value( std::size_t /*index*/, const void* /*value*/ ) const {}
virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t /*fvertIndex*/ ) const { return faceIndex; }
}
;

// Face accessor that provides readonly access to the surface area of a face
DEFINE_FACE_ACCESSOR( Area, _T("Face"), frantic::channels::data_type_float32, 1, true,
                      frantic::geometry::mesh_channel::transform_type::none )
virtual void get_value( std::size_t index, void* outValue ) const {
    *reinterpret_cast<float*>( outValue ) = 0.5f * Length( m_mesh->FaceNormal( (DWORD)index, FALSE ) );
}
virtual void set_value( std::size_t /*index*/, const void* /*value*/ ) const {}
virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t /*fvertIndex*/ ) const { return faceIndex; }
}
;

// Face accessor that provides readonly access to the max edge length of a face
DEFINE_FACE_ACCESSOR( MaxEdgeLength, _T("Face"), frantic::channels::data_type_float32, 1, true,
                      frantic::geometry::mesh_channel::transform_type::none )
virtual void get_value( std::size_t index, void* outValue ) const {
    Face& f = m_mesh->faces[index];
    Point3 tri[] = { m_mesh->getVert( f.getVert( 0 ) ), m_mesh->getVert( f.getVert( 1 ) ),
                     m_mesh->getVert( f.getVert( 2 ) ) };

    *reinterpret_cast<float*>( outValue ) = std::max<float>(
        Length( tri[1] - tri[0] ), std::max<float>( Length( tri[2] - tri[0] ), Length( tri[2] - tri[1] ) ) );
}
virtual void set_value( std::size_t /*index*/, const void* /*value*/ ) const {}
virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t /*fvertIndex*/ ) const { return faceIndex; }
}
;

// Face accessor that provides a boolean value (represented as a int8) for the face's selection
// DEFINE_FACE_ACCESSOR( Selection, _T("Face"), frantic::channels::data_type_int8, 1, false,
// frantic::geometry::mesh_channel::transform_type::none ) 	virtual void get_value( std::size_t index, void* outValue )
//const { *reinterpret_cast<boost::int8_t*>( outValue ) = (boost::int8_t)m_mesh->FaceSel()[ (int)index ]; } 	virtual void
//set_value( std::size_t index, const void* value ) const { m_mesh->FaceSel().Set( (int)index, *reinterpret_cast<const
//boost::int8_t*>( value ) != 0 ); } 	virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t /*fvertIndex*/
//) const { return faceIndex; }
// };

// Face accessor that provides a boolean value (represented as a int8) for the face's selection
class FaceSelectionAccessor : public frantic::geometry::mesh_channel {
    Mesh* m_mesh;
    mutable tbb::spin_mutex m_writeMutex; // Only needed for writes since we can clobber other writes due to the nature
                                          // of byte packed bit arrays.

  public:
    FaceSelectionAccessor( Mesh* mesh )
        : frantic::geometry::mesh_channel( _T("FaceSelection"), mesh_channel::face, frantic::channels::data_type_int8,
                                           1, mesh->getNumFaces(), mesh->getNumFaces(), false )
        , m_mesh( mesh ) {
        int numBits = mesh->FaceSel().GetSize();
        if( numBits != mesh->getNumFaces() )
            mesh->FaceSel().SetSize( mesh->getNumFaces(), TRUE );
    }

    virtual void get_value( std::size_t index, void* outValue ) const {
        *reinterpret_cast<boost::int8_t*>( outValue ) = (boost::int8_t)m_mesh->FaceSel()[(int)index];
    }
    virtual void set_value( std::size_t index, const void* value ) const {
        BOOL val = *reinterpret_cast<const boost::int8_t*>( value ) != 0;

        tbb::spin_mutex::scoped_lock theLock(
            m_writeMutex ); // Use this lock to prevent threaded tomfoolery when writing selection values.
        m_mesh->FaceSel().Set( (int)index, val );
    }
    virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t /*fvertIndex*/ ) const { return faceIndex; }
    virtual std::size_t get_num_face_verts( std::size_t /*faceIndex*/ ) const { return 1; }
};

// Returns an index representing the "element" this face is part of. An element is a connected region of faces. If two
// faces are not connected through any other faces or directly they will have a different element id.
class FaceElementAccessor : public frantic::geometry::mesh_channel {
    Mesh* m_mesh;
    DWORD* m_faceElems;

  public:
    FaceElementAccessor( Mesh* mesh, DWORD* faceElems )
        : frantic::geometry::mesh_channel( _T("FaceElement"), mesh_channel::face, frantic::channels::data_type_int32, 1,
                                           mesh->getNumFaces(), mesh->getNumFaces(), true )
        , m_mesh( mesh )
        , m_faceElems( faceElems ) {}

    virtual void get_value( std::size_t index, void* outValue ) const {
        *reinterpret_cast<int*>( outValue ) = (int)m_faceElems[index];
    }
    virtual void set_value( std::size_t /*index*/, const void* /*value*/ ) const {}
    virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t /*fvertIndex*/ ) const { return faceIndex; }
    virtual std::size_t get_num_face_verts( std::size_t /*faceIndex*/ ) const { return 1; }
};

class FaceElementAreaAccessor : public frantic::geometry::mesh_channel {
    Mesh* m_mesh;
    DWORD* m_faceElems;
    element_data* m_elemData;

  public:
    FaceElementAreaAccessor( Mesh* mesh, DWORD* faceElems, std::size_t numElems, element_data* elemData )
        : frantic::geometry::mesh_channel( _T("FaceElementArea"), mesh_channel::element,
                                           frantic::channels::data_type_float32, 1, numElems, mesh->getNumFaces(),
                                           true )
        , m_mesh( mesh )
        , m_faceElems( faceElems )
        , m_elemData( elemData ) {}

    virtual void get_value( std::size_t index, void* outValue ) const {
        *reinterpret_cast<float*>( outValue ) = m_elemData[index].get_area();
    }
    virtual void set_value( std::size_t /*index*/, const void* /*value*/ ) const {}
    virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t /*fvertIndex*/ ) const {
        return m_faceElems[faceIndex];
    }
    virtual std::size_t get_num_face_verts( std::size_t /*faceIndex*/ ) const { return 1; }
};

class FaceElementVolumeAccessor : public frantic::geometry::mesh_channel {
    Mesh* m_mesh;
    DWORD* m_faceElems;
    element_data* m_elemData;

  public:
    FaceElementVolumeAccessor( Mesh* mesh, DWORD* faceElems, std::size_t numElems, element_data* elemData )
        : frantic::geometry::mesh_channel( _T("FaceElementVolume"), mesh_channel::element,
                                           frantic::channels::data_type_float32, 1, numElems, mesh->getNumFaces(),
                                           true )
        , m_mesh( mesh )
        , m_faceElems( faceElems )
        , m_elemData( elemData ) {}

    virtual void get_value( std::size_t index, void* outValue ) const {
        *reinterpret_cast<float*>( outValue ) = m_elemData[index].get_volume();
    }
    virtual void set_value( std::size_t /*index*/, const void* /*value*/ ) const {}
    virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t /*fvertIndex*/ ) const {
        return m_faceElems[faceIndex];
    }
    virtual std::size_t get_num_face_verts( std::size_t /*faceIndex*/ ) const { return 1; }
};

class FaceElementCentroidAccessor : public frantic::geometry::mesh_channel {
    Mesh* m_mesh;
    DWORD* m_faceElems;
    element_data* m_elemData;

  public:
    FaceElementCentroidAccessor( Mesh* mesh, DWORD* faceElems, std::size_t numElems, element_data* elemData )
        : frantic::geometry::mesh_channel( _T("FaceElementCentroid"), mesh_channel::element,
                                           frantic::channels::data_type_float32, 3, numElems, mesh->getNumFaces(),
                                           true )
        , m_mesh( mesh )
        , m_faceElems( faceElems )
        , m_elemData( elemData ) {
        this->set_transform_type( transform_type::point );
    }

    virtual void get_value( std::size_t index, void* outValue ) const {
        *reinterpret_cast<Point3*>( outValue ) = m_elemData[index].get_centroid();
    }
    virtual void set_value( std::size_t /*index*/, const void* /*value*/ ) const {}
    virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t /*fvertIndex*/ ) const {
        return m_faceElems[faceIndex];
    }
    virtual std::size_t get_num_face_verts( std::size_t /*faceIndex*/ ) const { return 1; }
};

// Access the normals stored per-face-per-vertex. They are computed using the face's geometric normal, face-angle
// weighted averaging and the face's smoothing group info.
class SmoothNormalAccessor : public frantic::geometry::mesh_channel {
    Mesh* m_mesh;

  public:
    SmoothNormalAccessor( Mesh* mesh )
        : frantic::geometry::mesh_channel( _T("SmoothNormal"), mesh_channel::face_vertex,
                                           frantic::channels::data_type_float32, 3, mesh->getNumVerts(),
                                           mesh->getNumFaces(), true )
        , m_mesh( mesh ) {
        set_transform_type( transform_type::normal );
    }

    inline Point3 get_smooth_normal( int faceNum, int vertNum ) const {
        Face& f = m_mesh->faces[faceNum];
        RVertex& rv = m_mesh->getRVert( f.getVert( vertNum ) );

        int nNormals = (int)( rv.rFlags & NORCT_MASK );
        if( f.getSmGroup() != 0 && nNormals > 0 ) {
            if( nNormals > 1 ) {
                for( int i = 0; i < nNormals; ++i ) {
                    if( rv.ern[i].getSmGroup() & f.getSmGroup() )
                        return rv.ern[i].getNormal();
                }
            } else {
                return rv.rn.getNormal();
            }
        }

        return m_mesh->getFaceNormal( faceNum );
    }

    virtual void get_value( std::size_t index, void* outValue ) const {
        *reinterpret_cast<Point3*>( outValue ) = get_smooth_normal( (int)( index / 3 ), (int)( index % 3 ) );
    }
    virtual void set_value( std::size_t /*index*/, const void* /*value*/ ) const {}
    virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t fvertIndex ) const {
        return 3 * faceIndex + fvertIndex;
    }
    virtual std::size_t get_num_face_verts( std::size_t /*faceIndex*/ ) const { return 3; }
};

// This template class will apply an accumulation function to the values stored for on all the faces a vertex is part
// of.
template <class AccumulatorType>
class FromFacesAccessor : public frantic::geometry::mesh_channel {
    Mesh* m_mesh;
    AdjEdgeList* m_adjList;

    typedef typename AccumulatorType::result_type result_type;
    typedef frantic::channels::channel_data_type_traits<result_type> traits_type;

  public:
    FromFacesAccessor( Mesh* mesh, AdjEdgeList* adjList, const frantic::tstring& name )
        : frantic::geometry::mesh_channel( name, mesh_channel::vertex, traits_type::data_type(), traits_type::arity(),
                                           mesh->getNumVerts(), mesh->getNumFaces(), true )
        , m_mesh( mesh )
        , m_adjList( adjList ) {}

    virtual void get_value( std::size_t index, void* outValue ) const {
        Tab<DWORD> faceList;
        m_adjList->GetFaceList( (DWORD)index, faceList );

        AccumulatorType fn;

        DWORD* it = faceList.Addr( 0 );
        DWORD* itEnd = it + faceList.Count();

        for( fn.init( faceList.Count() ); it != itEnd; ++it )
            fn( *m_mesh, *it );

        *reinterpret_cast<result_type*>( outValue ) = fn.result();
    }
    virtual void set_value( std::size_t /*index*/, const void* /*value*/ ) const {}
    virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t fvertIndex ) const {
        return m_mesh->faces[faceIndex].getVert( (int)fvertIndex );
    }
    virtual std::size_t get_num_face_verts( std::size_t /*faceIndex*/ ) const { return 3; }
};

template <class ResultType>
struct accumulator {
    typedef ResultType result_type;

    // Must have member functions:
    //  void init( int count );
    //  void operator()( Mesh& mesh, DWORD face );
    //  void result_type result();
};

// Selects a vertex if all the associated faces are also selected.
struct accumulate_intersection : accumulator<float> {
    bool m_result;
    accumulate_intersection()
        : m_result( false ) {}

    inline void init( int count ) {
        if( count > 0 )
            m_result = true;
    }

    inline void operator()( Mesh& mesh, DWORD face ) {
        if( mesh.FaceSel()[face] == FALSE )
            m_result = false;
    }

    inline result_type result() const { return m_result ? 1.f : 0.f; }
};

// Selects a vertex if any of the associated faces are selected.
struct accumulate_union : accumulator<float> {
    bool m_result;
    accumulate_union()
        : m_result( false ) {}

    inline void init( int /*count*/ ) {}

    inline void operator()( Mesh& mesh, DWORD face ) {
        if( mesh.FaceSel()[face] != FALSE )
            m_result = true;
    }

    inline result_type result() const { return m_result ? 1.f : 0.f; }
};

// Creates a soft-selection for a vertex based on the portion of associated faces
// that are selected.
struct accumulate_average : accumulator<float> {
    int count, sum;
    accumulate_average()
        : sum( 0 ) {}

    inline void init( int _count ) { count = _count; }

    inline void operator()( Mesh& mesh, DWORD face ) {
        if( mesh.FaceSel()[face] != FALSE )
            ++sum;
    }

    inline float result() const { return (float)sum / (float)count; }
};

class FaceEdgeCountAccessor : public frantic::geometry::mesh_channel {
  public:
    FaceEdgeCountAccessor( Mesh* mesh )
        : frantic::geometry::mesh_channel( _T("FaceEdgeCount"), mesh_channel::face, frantic::channels::data_type_int32,
                                           1, mesh->getNumFaces(), mesh->getNumFaces(), false ) {}

    virtual void get_value( std::size_t /*index*/, void* outValue ) const { *reinterpret_cast<int*>( outValue ) = 3; }
    virtual void set_value( std::size_t /*index*/, const void* /*value*/ ) const {}
    virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t /*fvertIndex*/ ) const { return faceIndex; }
    virtual std::size_t get_num_face_verts( std::size_t /*faceIndex*/ ) const { return 3; }
};

class VertexEdgeCountAccessor : public frantic::geometry::mesh_channel {
    Mesh* m_mesh;
    AdjEdgeList* m_adjList;

  public:
    VertexEdgeCountAccessor( Mesh* mesh, AdjEdgeList* adjList )
        : frantic::geometry::mesh_channel( _T("VertexEdgeCount"), mesh_channel::vertex,
                                           frantic::channels::data_type_int32, 1, mesh->getNumVerts(),
                                           mesh->getNumVerts(), false )
        , m_mesh( mesh )
        , m_adjList( adjList ) {}

    virtual void get_value( std::size_t index, void* outValue ) const {
        *reinterpret_cast<int*>( outValue ) = static_cast<int>( m_adjList->list[index].Count() );
    }
    virtual void set_value( std::size_t /*index*/, const void* /*value*/ ) const {}
    virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t fvertIndex ) const {
        return m_mesh->faces[faceIndex].v[fvertIndex];
    }
    virtual std::size_t get_num_face_verts( std::size_t /*faceIndex*/ ) const { return 3; }
};

class VertexFaceCountAccessor : public frantic::geometry::mesh_channel {
    Mesh* m_mesh;
    AdjEdgeList* m_adjList;

  public:
    VertexFaceCountAccessor( Mesh* mesh, AdjEdgeList* adjList )
        : frantic::geometry::mesh_channel( _T("VertexFaceCount"), mesh_channel::vertex,
                                           frantic::channels::data_type_int32, 1, mesh->getNumVerts(),
                                           mesh->getNumVerts(), false )
        , m_mesh( mesh )
        , m_adjList( adjList ) {}

    virtual void get_value( std::size_t index, void* outValue ) const {
        DWORDTab adjFaces;
        m_adjList->GetFaceList( static_cast<DWORD>( index ), adjFaces ); // HACK This is not efficient!

        *reinterpret_cast<int*>( outValue ) = static_cast<int>( adjFaces.Count() );
    }
    virtual void set_value( std::size_t /*index*/, const void* /*value*/ ) const {}
    virtual std::size_t get_fv_index( std::size_t faceIndex, std::size_t fvertIndex ) const {
        return m_mesh->faces[faceIndex].v[fvertIndex];
    }
    virtual std::size_t get_num_face_verts( std::size_t /*faceIndex*/ ) const { return 3; }
};

/**
 * Initialze the vertex soft-selection channel
 */
inline void init_selection() {
    if( !m_mesh->vDataSupport( VDATA_SELECT ) ) {
        m_mesh->SupportVSelectionWeights();

        float* sel = m_mesh->getVSelectionWeights();

        // If we have a binary vertex selection, convert that to selection weights.
        if( m_mesh->selLevel == MESH_VERTEX ) {
            for( int i = 0, iEnd = m_mesh->getNumVerts(); i < iEnd; ++i )
                sel[i] = m_mesh->VertSel()[i] ? 1.f : 0.f;
        } else {
            // Otherwise, everything is selected.
            for( int i = 0, iEnd = m_mesh->getNumVerts(); i < iEnd; ++i )
                sel[i] = 1.f;
        }
    }
}

inline void init_elements() {
    if( !m_faceElems ) {
        FaceElementList tempList( *m_mesh, *m_tempData.AdjFList() );

        m_numElems = tempList.count;
        m_elemData.reset( new element_data[m_numElems] );

        m_faceElems.reset( new DWORD[m_mesh->getNumFaces()] );
        for( int i = 0, iEnd = m_mesh->getNumFaces(); i < iEnd; ++i ) {
            DWORD elemIndex = tempList[i];

            m_faceElems[i] = elemIndex;

            Face& f = m_mesh->faces[i];

            Point3 tri[] = { m_mesh->getVert( f.getVert( 0 ) ), m_mesh->getVert( f.getVert( 1 ) ),
                             m_mesh->getVert( f.getVert( 2 ) ) };

            m_elemData[elemIndex].add_face( tri );
        }

        for( std::size_t i = 0; i < m_numElems; ++i )
            m_elemData[i].finish();
    }
}

public:
MaxMeshInterface()
    : m_mesh( NULL )
    , m_ownedMesh( false )
    , m_numElems( 0 )
    , m_triObject( NULL )
    , m_ownedTriObject( false )
    , m_adjAllocated( false ) {}

virtual ~MaxMeshInterface() {
    if( m_triObject && m_ownedTriObject )
        m_triObject->MaybeAutoDelete();

    m_triObject = NULL;
    m_ownedTriObject = false;

    if( m_mesh && m_ownedMesh )
        m_mesh->DeleteThis();
    m_mesh = NULL;
    m_ownedMesh = false;
}

typedef boost::tuple<frantic::tstring, frantic::channels::data_type_t, std::size_t, frantic::tstring> channel_info;
typedef frantic::geometry::mesh_channel mesh_channel;

static void get_predefined_channels( std::vector<channel_info>& outChannels, mesh_channel::channel_type iterationType,
                                     bool forOutput );

void set_mesh( Mesh* theMesh, bool takeOwnership = false );

void set_tri_object( TriObject* pTriObject, bool takeOwnership );

void commit_writes();

/**
 * This provides access to the underlying mesh object. Please don't change it.
 */
const Mesh* get_mesh() const { return m_mesh; }

// HACK: Terrible! This needs to be removed...
const AdjEdgeList* get_edge_list() { return m_tempData.AdjEList(); }

virtual bool is_valid() const;

// This will populate an extra channel if possible.
virtual bool request_channel( const frantic::tstring& channelName, bool vertexChannel, bool forOutput,
                              bool throwOnError = true );

virtual std::size_t get_num_verts() const { return m_mesh->getNumVerts(); }

virtual std::size_t get_num_faces() const { return m_mesh->getNumFaces(); }

virtual std::size_t get_num_face_verts( std::size_t /*faceIndex*/ ) const { return 3; }

virtual void get_vert( std::size_t index, float ( &outValues )[3] ) const {
    memcpy( outValues, (float*)m_mesh->getVertPtr( (int)index ), 3 * sizeof( float ) );
}

virtual std::size_t get_face_vert_index( std::size_t faceIndex, std::size_t fvertIndex ) const {
    return m_mesh->faces[faceIndex].getVert( (int)fvertIndex );
}

virtual void get_face_vert_indices( std::size_t faceIndex, std::size_t outValues[] ) const {
    Face& f = m_mesh->faces[faceIndex];
    outValues[0] = f.getVert( 0 );
    outValues[1] = f.getVert( 1 );
    outValues[2] = f.getVert( 2 );
}

virtual void get_face_verts( std::size_t faceIndex, float outValues[][3] ) const {
    Face& f = m_mesh->faces[faceIndex];
    memcpy( outValues[0], (float*)m_mesh->getVertPtr( f.getVert( 0 ) ), 3 * sizeof( float ) );
    memcpy( outValues[1], (float*)m_mesh->getVertPtr( f.getVert( 1 ) ), 3 * sizeof( float ) );
    memcpy( outValues[2], (float*)m_mesh->getVertPtr( f.getVert( 2 ) ), 3 * sizeof( float ) );
}

virtual std::size_t get_num_elements() const { return std::max<std::size_t>( 1, m_numElems ); }

virtual std::size_t get_face_element_index( std::size_t faceIndex ) const {
    if( m_faceElems )
        return m_faceElems[faceIndex];
    return 0u;
}

virtual void init_adjacency();

virtual bool has_adjacency() const;

virtual bool init_vertex_iterator( frantic::geometry::vertex_iterator& vIt, std::size_t vertexIndex ) const;

virtual bool advance_vertex_iterator( frantic::geometry::vertex_iterator& vIt ) const;

virtual std::size_t get_edge_endpoint( frantic::geometry::vertex_iterator& vIt ) const;

virtual std::size_t get_edge_left_face( frantic::geometry::vertex_iterator& vIt ) const;

virtual std::size_t get_edge_right_face( frantic::geometry::vertex_iterator& vIt ) const;

virtual bool is_edge_visible( frantic::geometry::vertex_iterator& vIt ) const;

virtual bool is_edge_boundary( frantic::geometry::vertex_iterator& vIt ) const;

virtual void init_face_iterator( frantic::geometry::face_iterator& fIt, std::size_t faceIndex ) const;

virtual bool advance_face_iterator( frantic::geometry::face_iterator& fIt ) const;

virtual std::size_t get_face_neighbor( frantic::geometry::face_iterator& fIt ) const;

virtual std::size_t get_face_next_vertex( frantic::geometry::face_iterator& fIt ) const;

virtual std::size_t get_face_prev_vertex( frantic::geometry::face_iterator& fIt ) const;
}
;
}
}
}

#undef DEFINE_FACE_ACCESSOR
#undef DEFINE_VERTEX_ACCESSOR
