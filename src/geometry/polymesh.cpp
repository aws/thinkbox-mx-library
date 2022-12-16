// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/geometry/polymesh3_accessors.hpp>

#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/geometry/mesh.hpp>
#include <frantic/max3d/geometry/polymesh.hpp>

using frantic::geometry::polymesh3;
using frantic::geometry::polymesh3_builder;
using frantic::geometry::polymesh3_ptr;
using frantic::graphics::raw_byte_buffer;
using frantic::graphics::transform4f;
using frantic::graphics::vector3f;

namespace frantic {
namespace max3d {
namespace geometry {

polymesh3_ptr from_max_t( MNMesh& source ) {
    // Include all channels in the output mesh
    frantic::channels::channel_propagation_policy cpp( false );
    return from_max_t( source, cpp );
}

polymesh3_ptr from_max_t( MNMesh& source, const frantic::channels::channel_propagation_policy& cpp ) {
    using namespace frantic::channels;

    polymesh3_builder polyBuild;

    for( int i = 0, iEnd = source.VNum(); i < iEnd; ++i )
        polyBuild.add_vertex( frantic::max3d::from_max_t( source.P( i ) ) );

    for( int i = 0, iEnd = source.FNum(); i < iEnd; ++i ) {
        MNFace* pFace = source.F( i );
        polyBuild.add_polygon( pFace->vtx, (std::size_t)pFace->deg );
    }

    polymesh3_ptr pPolyMesh = polyBuild.finalize();

    for( int i = 0; i < MAX_MESHMAPS; ++i ) {
        MNMap* pMap = source.M( i );
        if( !pMap || pMap->VNum() == 0 || pMap->FNum() != source.FNum() )
            continue;

        frantic::tstring name;
        if( i == 0 )
            name = _T("Color");
        else if( i == 1 )
            name = _T("TextureCoord");
        else
            name = _T("Mapping") + boost::lexical_cast<frantic::tstring>( i );

        if( cpp.is_channel_included( name ) ) {
            pPolyMesh->add_empty_vertex_channel( name, data_type_float32, 3, (std::size_t)pMap->VNum() );

            frantic::geometry::polymesh3_vertex_accessor<vector3f> chAcc =
                pPolyMesh->get_vertex_accessor<vector3f>( name );
            for( std::size_t i = 0, iEnd = chAcc.vertex_count(); i < iEnd; ++i )
                chAcc.get_vertex( i ) = frantic::max3d::from_max_t( pMap->V( (int)i ) );
            for( std::size_t i = 0, iEnd = chAcc.face_count(); i < iEnd; ++i ) {
                MNMapFace* pFace = pMap->F( (int)i );
                memcpy( chAcc.get_face( i ).first, pFace->tv, sizeof( int ) * pFace->deg );
            }
        }
    }

    { // Add the face channels
        bool getSm = cpp.is_channel_included( _T("SmoothingGroup") );
        bool getMtl = cpp.is_channel_included( _T("MaterialID") );

        if( getSm || getMtl ) {
            frantic::graphics::raw_byte_buffer smBuffer, mtlBuffer;
            smBuffer.reserve( sizeof( int ) * source.FNum() );
            mtlBuffer.reserve( sizeof( MtlID ) * source.FNum() );

            for( int i = 0, iEnd = source.FNum(); i < iEnd; ++i ) {
                MNFace* pFace = source.F( (int)i );
                *(int*)smBuffer.add_element( sizeof( int ) ) = (int)pFace->smGroup;
                *(MtlID*)mtlBuffer.add_element( sizeof( MtlID ) ) = pFace->material;
            }

            if( getSm )
                pPolyMesh->add_face_channel( _T("SmoothingGroup"), data_type_int32, 1, smBuffer );
            if( getMtl )
                pPolyMesh->add_face_channel( _T("MaterialID"), data_type_uint16, 1, mtlBuffer );
        }
    }

    if( source.selLevel == MNM_SL_VERTEX && cpp.is_channel_included( _T("Selection") ) ) {
        frantic::graphics::raw_byte_buffer selBuffer;
        selBuffer.resize( sizeof( float ) * source.VNum() );

        if( source.vDataSupport( VDATA_SELECT ) ) {
            float* vsel = source.vertexFloat( VDATA_SELECT );
            if( vsel ) {
                for( int i = 0, iEnd = source.VNum(); i < iEnd; ++i ) {
                    *(float*)selBuffer.ptr_at( i * sizeof( float ) ) = vsel[i];
                }
            } else {
                if( selBuffer.size() > 0 ) {
                    memset( selBuffer.ptr_at( 0 ), 0, selBuffer.size() );
                }
            }
        } else {
            BitArray vsel;
            source.getVertexSel( vsel );
            for( int i = 0, iEnd = source.VNum(); i < iEnd; ++i ) {
                *(float*)selBuffer.ptr_at( i * sizeof( float ) ) = vsel[i] ? 1.f : 0.f;
            }
        }

        pPolyMesh->add_vertex_channel( _T("Selection"), data_type_float32, 1, selBuffer );
    }

    if( source.selLevel == MNM_SL_FACE && cpp.is_channel_included( _T("FaceSelection" ) ) ) {
        frantic::graphics::raw_byte_buffer faceSelBuffer;
        faceSelBuffer.resize( sizeof( boost::int32_t ) * source.FNum() );

        BitArray fsel;
        source.getFaceSel( fsel );
        for( int i = 0, iEnd = source.FNum(); i < iEnd; ++i ) {
            *(boost::int32_t*)faceSelBuffer.ptr_at( i * sizeof( boost::int32_t ) ) = fsel[i];
        }
        pPolyMesh->add_face_channel( _T("FaceSelection"), data_type_int32, 1, faceSelBuffer );
    }

    if( cpp.is_channel_included( _T("EdgeSharpness") ) ) {
        float* edgeData = source.edgeFloat( EDATA_CREASE );
        if( edgeData ) {
            size_t edgeCreaseCount = 0;
            for( int i = 0; i < source.nume; ++i ) {
                if( edgeData[i] > 0 ) {
                    ++edgeCreaseCount;
                }
            }

            const int noCreasePos = boost::numeric_cast<int>( edgeCreaseCount );

            frantic::graphics::raw_byte_buffer dataBuffer;
            dataBuffer.resize( ( edgeCreaseCount + 1 ) * sizeof( float ) );
            float* edgeCreaseChannel = reinterpret_cast<float*>( dataBuffer.begin() );
            edgeCreaseChannel[edgeCreaseCount] = 0;

            std::vector<int> faceBuffer;
            faceBuffer.reserve( source.FNum() );

            std::map<int, int> edgeToBufferPos;

            int edgeCreaseChannelPos = 0;
            for( int i = 0, iEnd = source.FNum(); i < iEnd; ++i ) {
                MNFace* face = source.F( (int)i );
                for( int edgeCount = 0, edgeCountEnd = face->deg; edgeCount < edgeCountEnd; ++edgeCount ) {
                    int edgeIndex = face->edg[edgeCount];
                    float magnitude = edgeData[edgeIndex];

                    int vertBufPos = noCreasePos;
                    if( magnitude > 0 ) {
                        std::map<int, int>::iterator it = edgeToBufferPos.find( edgeIndex );

                        if( it == edgeToBufferPos.end() ) {
                            edgeCreaseChannel[edgeCreaseChannelPos] = magnitude;

                            vertBufPos = edgeCreaseChannelPos;
                            edgeToBufferPos[edgeIndex] = vertBufPos;

                            ++edgeCreaseChannelPos;
                        } else {
                            vertBufPos = it->second;
                        }
                    }
                    faceBuffer.push_back( vertBufPos );
                }
            }

            pPolyMesh->add_vertex_channel( _T("EdgeSharpness"), frantic::channels::data_type_float32, 1, dataBuffer,
                                           &faceBuffer );
        }
    }

    if( is_vdata_crease_supported() && cpp.is_channel_included( _T("VertexSharpness") ) ) {
        float* vertexData = source.vertexFloat( get_vdata_crease_channel() );
        if( vertexData ) {
            std::size_t vertexCount = source.VNum();

            raw_byte_buffer buffer( vertexData, vertexCount * sizeof( float ) );

            pPolyMesh->add_vertex_channel( _T("VertexSharpness"), frantic::channels::data_type_float32, 1, buffer );
        }
    }

    return pPolyMesh;
}

polymesh3_ptr from_max_t( Mesh& source ) {
    using namespace frantic::channels;

    polymesh3_builder polyBuild;

    for( int i = 0, iEnd = source.getNumVerts(); i < iEnd; ++i )
        polyBuild.add_vertex( frantic::max3d::from_max_t( source.getVert( i ) ) );
    for( int i = 0, iEnd = source.getNumFaces(); i < iEnd; ++i )
        polyBuild.add_polygon( (int*)source.faces[i].v, 3 );

    polymesh3_ptr pPolyMesh = polyBuild.finalize();

    for( int i = 0; i < MAX_MESHMAPS; ++i ) {
        if( !source.mapSupport( i ) )
            continue;

        frantic::tstring name;
        if( i == 0 )
            name = _T("Color");
        else if( i == 1 )
            name = _T("TextureCoord");
        else
            name = _T("Mapping") + boost::lexical_cast<frantic::tstring>( i );

        pPolyMesh->add_empty_vertex_channel( name, data_type_float32, 3, (std::size_t)source.getNumMapVerts( i ) );

        UVVert* pMapVerts = source.mapVerts( i );
        TVFace* pMapFaces = source.mapFaces( i );

        frantic::geometry::polymesh3_vertex_accessor<vector3f> chAcc = pPolyMesh->get_vertex_accessor<vector3f>( name );
        for( std::size_t i = 0, iEnd = chAcc.vertex_count(); i < iEnd; ++i )
            chAcc.get_vertex( i ) = frantic::max3d::from_max_t( pMapVerts[i] );
        for( std::size_t i = 0, iEnd = chAcc.face_count(); i < iEnd; ++i )
            memcpy( chAcc.get_face( i ).first, pMapFaces[i].t, sizeof( int[3] ) );
    }

    { // Add the face channels
        frantic::graphics::raw_byte_buffer smBuffer, mtlBuffer;
        smBuffer.reserve( sizeof( int ) * source.getNumFaces() );
        mtlBuffer.reserve( sizeof( MtlID ) * source.getNumFaces() );

        for( int i = 0, iEnd = source.getNumFaces(); i < iEnd; ++i ) {
            *(int*)smBuffer.add_element( sizeof( int ) ) = (int)source.faces[i].getSmGroup();
            *(MtlID*)mtlBuffer.add_element( sizeof( MtlID ) ) = source.faces[i].getMatID();
        }

        pPolyMesh->add_face_channel( _T("SmoothingGroup"), data_type_int32, 1, smBuffer );
        pPolyMesh->add_face_channel( _T("MaterialID"), data_type_uint16, 1, mtlBuffer );
    }

    return pPolyMesh;
}

void clear_polymesh( MNMesh& mesh ) { mesh.ClearAndFree(); }

// return number of maps to pass to MNMesh.SetNumMaps
int get_num_maps( polymesh3_ptr polymesh ) {
    if( !polymesh ) {
        return 0;
    }

    int numMaps = 0;

    for( polymesh3::iterator it = polymesh->begin(), itEnd = polymesh->end(); it != itEnd; ++it ) {
        if( it->second.is_vertex_channel() ) {
            const frantic::tstring& channelName = it->first;
            if( channelName == _T("Color") ) {
                numMaps = std::max<int>( numMaps, 1 );
            } else if( channelName == _T("TextureCoord") ) {
                numMaps = std::max<int>( numMaps, 2 );
            } else if( _tcsnccmp( it->first.c_str(), _T("Mapping"), 7 ) == 0 ) {
                bool gotMapNum = false;
                int mapNum = 0;
                try {
                    mapNum = boost::lexical_cast<int>( it->first.substr( 7 ) );
                    gotMapNum = true;
                } catch( boost::bad_lexical_cast& ) {
                    gotMapNum = false;
                }
                if( gotMapNum && mapNum >= 0 && mapNum < MAX_MESHMAPS ) {
                    numMaps = std::max<int>( numMaps, 1 + mapNum );
                }
            }
        }
    }
    return numMaps;
}

void polymesh_copy( MNMesh& dest, polymesh3_ptr polymesh ) {
    dest.Clear();

    // Also reset the selection level -- it isn't reset by Clear()
    dest.selLevel = MNM_SL_OBJECT;

    frantic::geometry::polymesh3_const_vertex_accessor<vector3f> vertAcc =
        polymesh->get_const_vertex_accessor<vector3f>( _T("verts") );

    dest.setNumVerts( (int)vertAcc.vertex_count() );
    dest.setNumFaces( (int)vertAcc.face_count() );

    for( std::size_t i = 0, iEnd = vertAcc.vertex_count(); i < iEnd; ++i )
        dest.V( (int)i )->p = frantic::max3d::to_max_t( vertAcc.get_vertex( i ) );
    for( std::size_t i = 0, iEnd = vertAcc.face_count(); i < iEnd; ++i ) {
        frantic::geometry::polymesh3_const_face_range f = vertAcc.get_face( i );
        dest.F( (int)i )->MakePoly( (int)( f.second - f.first ), const_cast<int*>( f.first ) );
        dest.F( (int)i )->smGroup = 1;
        dest.F( (int)i )->material = 0;
    }

    if( polymesh->has_face_channel( _T("SmoothingGroup") ) ) {
        frantic::geometry::polymesh3_const_face_accessor<int> chAcc =
            polymesh->get_const_face_accessor<int>( _T("SmoothingGroup") );
        for( std::size_t i = 0, iEnd = vertAcc.face_count(); i < iEnd; ++i )
            dest.F( (int)i )->smGroup = chAcc.get_face( i );
    }

    if( polymesh->has_face_channel( _T("MaterialID") ) ) {
        frantic::geometry::polymesh3_const_face_accessor<MtlID> chAcc =
            polymesh->get_const_face_accessor<MtlID>( _T("MaterialID") );
        for( std::size_t i = 0, iEnd = vertAcc.face_count(); i < iEnd; ++i )
            dest.F( (int)i )->material = chAcc.get_face( i );
    }

    dest.SetMapNum( get_num_maps( polymesh ) );

    // Grab all the mapping channels
    for( polymesh3::iterator it = polymesh->begin(), itEnd = polymesh->end(); it != itEnd; ++it ) {
        if( it->second.is_vertex_channel() ) {
            bool isMap = false;
            int mapNum = 0;
            if( it->first == _T("Color") ) {
                isMap = true;
                mapNum = 0;
            } else if( it->first == _T("TextureCoord") ) {
                isMap = true;
                mapNum = 1;
            } else if( _tcsnccmp( it->first.c_str(), _T("Mapping"), 7 ) == 0 ) {
                bool gotMapNum = false;
                try {
                    mapNum = boost::lexical_cast<int>( it->first.substr( 7 ) );
                    gotMapNum = true;
                } catch( boost::bad_lexical_cast& ) {
                    gotMapNum = false;
                }
                isMap = gotMapNum && mapNum >= 0 && mapNum < MAX_MESHMAPS;
            }
            if( isMap ) {
                frantic::geometry::polymesh3_const_vertex_accessor<vector3f> chAcc =
                    polymesh->get_const_vertex_accessor<vector3f>( it->first );
                std::size_t numFaces = chAcc.has_custom_faces() ? chAcc.face_count() : vertAcc.face_count();
                std::size_t numVerts = chAcc.vertex_count();

                dest.InitMap( mapNum );

                MNMap* pMap = dest.M( mapNum );
                pMap->setNumVerts( (int)numVerts );
                pMap->setNumFaces( (int)numFaces );

                for( std::size_t i = 0; i < numVerts; ++i )
                    pMap->v[(int)i] = frantic::max3d::to_max_t( chAcc.get_vertex( i ) );
                if( chAcc.has_custom_faces() ) {
                    for( std::size_t i = 0; i < numFaces; ++i ) {
                        frantic::geometry::polymesh3_const_face_range r = chAcc.get_face( i );
                        pMap->F( (int)i )->MakePoly( (int)( r.second - r.first ), const_cast<int*>( r.first ) );
                    }
                } else {
                    for( std::size_t i = 0; i < numFaces; ++i ) {
                        frantic::geometry::polymesh3_const_face_range r = vertAcc.get_face( i );
                        pMap->F( (int)i )->MakePoly( (int)( r.second - r.first ), const_cast<int*>( r.first ) );
                    }
                }
            }
        }
    }

    if( polymesh->has_vertex_channel( _T("Selection") ) ) {
        frantic::geometry::polymesh3_const_vertex_accessor<float> chAcc =
            polymesh->get_const_vertex_accessor<float>( _T("Selection") );

        dest.SupportVSelectionWeights();
        float* vSelectionWeights = dest.getVSelectionWeights();
        for( std::size_t i = 0, ie = chAcc.vertex_count(); i < ie; ++i ) {
            vSelectionWeights[i] = chAcc.get_vertex( i );
        }
        dest.selLevel = MNM_SL_VERTEX;
    }

    dest.InvalidateGeomCache();
    dest.InvalidateTopoCache();
    dest.FillInMesh();
    dest.PrepForPipeline();
}

frantic::geometry::polymesh3_ptr polymesh_copy( Mesh& mesh, const frantic::graphics::transform4f& firstXfrm,
                                                const frantic::graphics::transform4f& secondXfrm,
                                                const frantic::channels::channel_propagation_policy& cpp,
                                                float timeStepInSecs ) {
    frantic::geometry::polymesh3_ptr outPtr = frantic::max3d::geometry::from_max_t( mesh );
    frantic::graphics::transform4f xformDerivative = transform4f::zero();

    if( cpp.is_channel_included( _T("Velocity") ) ) {
        xformDerivative = ( secondXfrm - firstXfrm ) / timeStepInSecs;

        raw_byte_buffer buffer;
        size_t size = mesh.numVerts * sizeof( float ) * 3;
        buffer.resize( size );
        memset( buffer.ptr_at( 0 ), 0, size );

        outPtr->add_vertex_channel( _T("Velocity"), frantic::channels::data_type_float32, 3, buffer );

        transform( outPtr, firstXfrm, xformDerivative );
    }

    return outPtr;
}

namespace {

bool find_unused_map_channel( MNMesh& mesh, int* pOutMapChannel ) {
    int mapChannel = 0;
    for( int mapChannelEnd = mesh.MNum(); mapChannel < mapChannelEnd; ++mapChannel ) {
        MNMap* map = mesh.M( mapChannel );
        if( map && map->GetFlag( MN_DEAD ) ) {
            break;
        }
    }
    if( mapChannel < MAX_MESHMAPS ) {
        if( pOutMapChannel ) {
            *pOutMapChannel = mapChannel;
        }
        return true;
    }
    return false;
}

void exclude_channel( frantic::channels::channel_propagation_policy& cpp, const frantic::tstring& channelName ) {
    if( cpp.is_include_list() ) {
        cpp.remove_channel( channelName );
    } else {
        cpp.add_channel( channelName );
    }
}

void copy_velocity_channel( MNMesh& mesh, int mapChannel, const Tab<Point3>& velocity ) {
    if( mapChannel < -NUM_HIDDENMAPS || mapChannel >= MAX_MESHMAPS ) {
        throw std::runtime_error( "copy_velocity_channel Error: map channel out of range: " +
                                  boost::lexical_cast<std::string>( mapChannel ) );
    }
    if( mesh.VNum() != velocity.Count() ) {
        throw std::runtime_error( "copy_velocity_channel Error: mismatch between vertex count and velocity count (" +
                                  boost::lexical_cast<std::string>( mesh.VNum() ) + " vs " +
                                  boost::lexical_cast<std::string>( velocity.Count() ) + ")" );
    }
    if( mapChannel >= mesh.MNum() ) {
        mesh.SetMapNum( mapChannel + 1 );
    }
    MNMap* map = mesh.M( mapChannel );
    if( !map ) {
        throw std::runtime_error( "copy_velocity_channel Error: added map channel is NULL" );
    }
    map->ClearFlag( MN_DEAD );
    map->setNumVerts( mesh.VNum() );
    map->setNumFaces( mesh.FNum() );
    for( int vertexIndex = 0, vertexIndexEnd = mesh.VNum(); vertexIndex < vertexIndexEnd; ++vertexIndex ) {
        map->v[vertexIndex] = velocity[vertexIndex];
    }
    for( int faceIndex = 0, faceIndexEnd = mesh.FNum(); faceIndex < faceIndexEnd; ++faceIndex ) {
        const MNFace& face = mesh.f[faceIndex];
        map->f[faceIndex].MakePoly( face.deg, face.vtx );
    }
}

/**
 * Copy velocity information acquired from GetRenderMeshVertexSpeed() into an output polymesh3.
 *
 * @todo I think there's a lot of redundant copying in here.  Test and optimize it.
 *
 * @param[out] outMesh the destination mesh for the Velocity.
 * @param velocity the velocity array acquired from GetRenderMeshVertexSpeed().
 * @param inMesh a mesh that holds velocity information in a map channel.
 * @param mapChannel the map channel in inMesh that holds velocity information.
 */
void copy_velocity_channel( frantic::geometry::polymesh3_ptr outMesh, const Tab<Point3>& velocity, MNMesh& inMesh,
                            int mapChannel ) {
    if( !outMesh ) {
        throw std::runtime_error( "copy_velocity_channel Error: output mesh is NULL" );
    }
    if( outMesh->has_channel( _T("Velocity") ) ) {
        throw std::runtime_error( "copy_velocity_channel Error: the output mesh already has a Velocity channel" );
    }

    if( mapChannel < -NUM_HIDDENMAPS || mapChannel >= inMesh.MNum() ) {
        throw std::runtime_error(
            "copy_velocity_channel Error: map channel is out of range of maps in the input mesh" );
    }
    MNMap* map = inMesh.M( mapChannel );
    if( !map || map->GetFlag( MN_DEAD ) ) {
        throw std::runtime_error(
            "copy_velocity_channel Error: the specified map channel is not present in the input mesh" );
    }
    if( map->FNum() != static_cast<int>( outMesh->face_count() ) ) {
        throw std::runtime_error(
            "copy_velocity_channel: Mismatch between face count in map channel and output mesh (" +
            boost::lexical_cast<std::string>( map->FNum() ) + " vs " +
            boost::lexical_cast<std::string>( outMesh->face_count() ) + ")" );
    }

    outMesh->add_empty_vertex_channel( _T("Velocity"), frantic::channels::data_type_float32, 3 );

    frantic::geometry::polymesh3_vertex_accessor<frantic::graphics::vector3f> velocityChannel =
        outMesh->get_vertex_accessor<vector3f>( _T("Velocity") );
    frantic::geometry::polymesh3_const_vertex_accessor<frantic::graphics::vector3f> geomChannel =
        outMesh->get_const_vertex_accessor<vector3f>( _T("verts") );

    // Some vertex velocities may be lost in the map channel conversion, for example,
    // if there are no faces in the mesh.  From what I've seen so far, the order of the
    // original vertices is maintained during the conversion, so I copy the original
    // vertex velocities into the output Velocity channel first, and then copy
    // velocities from the map channel.
    for( int vertexIndex = 0,
             vertexIndexEnd = std::min<int>( static_cast<int>( outMesh->vertex_count() ), velocity.Count() );
         vertexIndex < vertexIndexEnd; ++vertexIndex ) {
        velocityChannel.get_vertex( vertexIndex ) =
            (float)TIME_TICKSPERSEC * frantic::max3d::from_max_t( velocity[vertexIndex] );
    }

    for( int faceIndex = 0, faceIndexEnd = static_cast<int>( outMesh->face_count() ); faceIndex < faceIndexEnd;
         ++faceIndex ) {
        frantic::geometry::polymesh3_const_face_range face = geomChannel.get_face( faceIndex );
        MNMapFace& mapFace = map->f[faceIndex];
        if( ( face.second - face.first ) != mapFace.deg ) {
            throw std::runtime_error( "copy_velocity_channel Error: mismatch in degree of face " +
                                      boost::lexical_cast<std::string>( faceIndex ) +
                                      " between output mesh and map channel" );
        }
        for( int corner = 0, cornerEnd = mapFace.deg; corner < cornerEnd; ++corner ) {
            const int vertexIndex = face.first[corner];
            const int mapVertexIndex = mapFace.tv[corner];
            velocityChannel.get_vertex( vertexIndex ) =
                (float)TIME_TICKSPERSEC * frantic::max3d::from_max_t( map->v[mapVertexIndex] );
        }
    }
}

} // anonymous namespace

frantic::geometry::polymesh3_ptr polymesh_copy( Mesh& trimesh, const frantic::graphics::transform4f& xfrm,
                                                const Tab<Point3>& worldSpaceVertexVelocity,
                                                const frantic::channels::channel_propagation_policy& cpp ) {
    MNMesh polymesh, tempPolymesh;
    polymesh.SetFromTri( trimesh );

    MNMesh* pVelocityPolymesh = 0;
    int velocityMapChannel = -1;

    frantic::channels::channel_propagation_policy cppWithoutVelocityMapChannel( cpp );
    if( cpp.is_channel_included( _T("Velocity") ) ) {
        if( find_unused_map_channel( polymesh, &velocityMapChannel ) ) {
            exclude_channel( cppWithoutVelocityMapChannel, get_map_channel_name( velocityMapChannel ) );
            pVelocityPolymesh = &polymesh;
        } else {
            tempPolymesh = polymesh;
            tempPolymesh.MAlloc( 0, FALSE );
            velocityMapChannel = 0;
            pVelocityPolymesh = &tempPolymesh;
        }
        copy_velocity_channel( *pVelocityPolymesh, velocityMapChannel, worldSpaceVertexVelocity );
    }

    make_polymesh( polymesh );
    make_polymesh( tempPolymesh );

    frantic::geometry::polymesh3_ptr outPtr = polymesh_copy( polymesh, xfrm, cppWithoutVelocityMapChannel );
    if( pVelocityPolymesh && velocityMapChannel >= 0 ) {
        copy_velocity_channel( outPtr, worldSpaceVertexVelocity, *pVelocityPolymesh, velocityMapChannel );
    }

    return outPtr;
}

frantic::geometry::polymesh3_ptr polymesh_copy( Mesh& firstMesh, Mesh& secondMesh,
                                                const frantic::graphics::transform4f& firstXfrm,
                                                const frantic::graphics::transform4f& secondXfrm,
                                                const frantic::channels::channel_propagation_policy& cpp,
                                                float timeStepInSecs ) {
    if( firstMesh.numVerts != secondMesh.numVerts )
        throw std::runtime_error( "polymesh_copy() - meshes must have same number of vertices" );

    frantic::geometry::polymesh3_ptr outPtr = frantic::max3d::geometry::from_max_t( firstMesh );
    transform( outPtr, firstXfrm );

    if( cpp.is_channel_included( _T("Velocity") ) ) {
        outPtr->add_empty_vertex_channel( _T("Velocity"), frantic::channels::data_type_float32, 3 );
        frantic::geometry::polymesh3_vertex_accessor<frantic::graphics::vector3f> acc =
            outPtr->get_vertex_accessor<frantic::graphics::vector3f>( _T("Velocity") );
        const int numVerts = firstMesh.numVerts;
        for( int i = 0; i < numVerts; ++i ) {
            vector3f vec1 = frantic::max3d::from_max_t( firstMesh.getVert( i ) );
            vector3f vec2 = frantic::max3d::from_max_t( secondMesh.getVert( i ) );
            acc.get_vertex( static_cast<std::size_t>( i ) ) = ( secondXfrm * vec2 - firstXfrm * vec1 ) / timeStepInSecs;
        }
    }

    return outPtr;
}

frantic::geometry::polymesh3_ptr polymesh_copy( MNMesh& mesh, const frantic::graphics::transform4f& xform,
                                                const frantic::channels::channel_propagation_policy& cpp ) {
    frantic::geometry::polymesh3_ptr outPtr = frantic::max3d::geometry::from_max_t( mesh, cpp );
    frantic::geometry::transform( outPtr, xform );
    return outPtr;
}

frantic::geometry::polymesh3_ptr polymesh_copy( MNMesh& mesh, const frantic::graphics::transform4f& firstXfrm,
                                                const frantic::graphics::transform4f& secondXfrm,
                                                const frantic::channels::channel_propagation_policy& cpp,
                                                float timeStepInSecs ) {
    frantic::geometry::polymesh3_ptr outPtr = frantic::max3d::geometry::from_max_t( mesh, cpp );
    frantic::graphics::transform4f xformDerivative = transform4f::zero();

    if( cpp.is_channel_included( _T("Velocity") ) ) {
        xformDerivative = ( secondXfrm - firstXfrm ) / timeStepInSecs;

        raw_byte_buffer buffer;
        size_t size = mesh.numv * sizeof( float ) * 3;
        buffer.resize( size );
        memset( buffer.ptr_at( 0 ), 0, size );

        outPtr->add_vertex_channel( _T("Velocity"), frantic::channels::data_type_float32, 3, buffer );
    }

    transform( outPtr, firstXfrm, xformDerivative );

    return outPtr;
}

frantic::geometry::polymesh3_ptr polymesh_copy( MNMesh& firstMesh, MNMesh& secondMesh,
                                                const frantic::graphics::transform4f& firstXfrm,
                                                const frantic::graphics::transform4f& secondXfrm,
                                                const frantic::channels::channel_propagation_policy& cpp,
                                                float timeStepInSecs ) {
    if( firstMesh.numv != secondMesh.numv )
        throw std::runtime_error( "polymesh_copy() - meshes must have same number of vertices" );

    frantic::geometry::polymesh3_ptr outPtr = frantic::max3d::geometry::from_max_t( firstMesh, cpp );
    transform( outPtr, firstXfrm );

    if( cpp.is_channel_included( _T("Velocity") ) ) {
        const unsigned numVerts = firstMesh.numv;

        outPtr->add_empty_vertex_channel( _T("Velocity"), frantic::channels::data_type_float32, 3 );
        frantic::geometry::polymesh3_vertex_accessor<frantic::graphics::vector3f> acc =
            outPtr->get_vertex_accessor<frantic::graphics::vector3f>( _T("Velocity") );

        for( unsigned i = 0; i < numVerts; ++i ) {
            vector3f vec1 = frantic::max3d::from_max_t( firstMesh.P( i ) );
            vector3f vec2 = frantic::max3d::from_max_t( secondMesh.P( i ) );
            acc.get_vertex( static_cast<std::size_t>( i ) ) = ( secondXfrm * vec2 - firstXfrm * vec1 ) / timeStepInSecs;
        }
    }

    return outPtr;
}

void polymesh_copy_time_offset( MNMesh& dest, frantic::geometry::polymesh3_ptr polymesh, float timeOffset ) {
    polymesh_copy( dest, polymesh );

    if( polymesh && timeOffset != 0 && polymesh->has_vertex_channel( _T("Velocity") ) ) {
        frantic::geometry::polymesh3_const_vertex_accessor<frantic::graphics::vector3f> velAcc =
            polymesh->get_const_vertex_accessor<frantic::graphics::vector3f>( _T("Velocity") );
        if( velAcc.has_custom_faces() ) {
            throw std::runtime_error(
                "polymesh_copy_time_offset() The \'Velocity\' channel of the supplied polymesh3 has custom faces." );
        }
        if( velAcc.vertex_count() != static_cast<std::size_t>( dest.numv ) ) {
            throw std::runtime_error(
                "polymesh_copy_time_offset() Internal Error: Mismatch between size of Velocity channel (" +
                boost::lexical_cast<std::string>( velAcc.vertex_count() ) + ") and number of vertices (" +
                boost::lexical_cast<std::string>( dest.numv ) + ")." );
        }
        for( int i = 0, iEnd = dest.numv; i < iEnd; ++i ) {
            const frantic::graphics::vector3f velocity = velAcc.get_vertex( static_cast<std::size_t>( i ) );
            dest.V( i )->p = dest.V( i )->p + timeOffset * ( *reinterpret_cast<const Point3*>( &velocity ) );
        }
    }
}

void polymesh_copy( Mesh& dest, polymesh3_ptr polymesh, bool throwIfNotTriangles ) {
    frantic::max3d::geometry::clear_mesh( dest );

    if( !polymesh->is_triangle_mesh() ) {
        if( throwIfNotTriangles )
            throw std::runtime_error( "polymesh_copy() The supplied polymesh3 had at least one non-triangle face." );

        // Otherwise, triangulate the polymesh using Max's methods (for now).
        MNMesh temp;
        polymesh_copy( temp, polymesh );

        temp.OutToTri( dest );
    } else {
        frantic::geometry::polymesh3_const_vertex_accessor<vector3f> vertAcc =
            polymesh->get_const_vertex_accessor<vector3f>( _T("verts") );

        dest.setNumVerts( (int)vertAcc.vertex_count() );
        dest.setNumFaces( (int)vertAcc.face_count() );

        for( std::size_t i = 0, iEnd = vertAcc.vertex_count(); i < iEnd; ++i )
            dest.setVert( (int)i, frantic::max3d::to_max_t( vertAcc.get_vertex( i ) ) );
        for( std::size_t i = 0, iEnd = vertAcc.face_count(); i < iEnd; ++i ) {
            frantic::geometry::polymesh3_const_face_range f = vertAcc.get_face( i );
            dest.faces[(int)i].setVerts( f.first[0], f.first[1], f.first[2] );
        }

        if( polymesh->has_face_channel( _T("SmoothingGroup") ) ) {
            frantic::geometry::polymesh3_const_face_accessor<int> chAcc =
                polymesh->get_const_face_accessor<int>( _T("SmoothingGroup") );
            for( std::size_t i = 0, iEnd = vertAcc.face_count(); i < iEnd; ++i )
                dest.faces[(int)i].setSmGroup( chAcc.get_face( i ) );
        }

        if( polymesh->has_face_channel( _T("MaterialID") ) ) {
            frantic::geometry::polymesh3_const_face_accessor<MtlID> chAcc =
                polymesh->get_const_face_accessor<MtlID>( _T("MaterialID") );
            for( std::size_t i = 0, iEnd = vertAcc.face_count(); i < iEnd; ++i )
                dest.faces[(int)i].setMatID( chAcc.get_face( i ) );
        }

        for( polymesh3::iterator it = polymesh->begin(), itEnd = polymesh->end(); it != itEnd; ++it ) {
            if( it->second.is_vertex_channel() ) {
                bool isMap = false;
                int mapNum = 0;
                if( it->first == _T("Color") ) {
                    isMap = true;
                    mapNum = 0;
                } else if( it->first == _T("TextureCoord") ) {
                    isMap = true;
                    mapNum = 1;
                } else if( _tcsnccmp( it->first.c_str(), _T("Mapping"), 7 ) == 0 ) {
                    bool gotMapNum = false;
                    try {
                        mapNum = boost::lexical_cast<int>( it->first.substr( 7 ) );
                        gotMapNum = true;
                    } catch( boost::bad_lexical_cast& ) {
                        gotMapNum = false;
                    }
                    isMap = gotMapNum && mapNum >= 0 && mapNum < MAX_MESHMAPS;
                }
                if( isMap ) {
                    frantic::geometry::polymesh3_const_vertex_accessor<vector3f> chAcc =
                        polymesh->get_const_vertex_accessor<vector3f>( it->first );
                    std::size_t numFaces = chAcc.has_custom_faces() ? chAcc.face_count() : vertAcc.face_count();
                    std::size_t numVerts = chAcc.vertex_count();

                    dest.setMapSupport( mapNum );
                    dest.setNumMapVerts( mapNum, (int)numVerts );
                    dest.setNumMapFaces( mapNum, (int)numFaces );

                    UVVert* pMapVerts = dest.mapVerts( mapNum );
                    TVFace* pTVFaces = dest.mapFaces( mapNum );
                    for( std::size_t i = 0; i < numVerts; ++i )
                        pMapVerts[i] = frantic::max3d::to_max_t( chAcc.get_vertex( i ) );
                    if( chAcc.has_custom_faces() ) {
                        for( std::size_t i = 0; i < numFaces; ++i ) {
                            const DWORD* face = (DWORD*)chAcc.get_face( i ).first;
                            pTVFaces[i].setTVerts( face[0], face[1], face[2] );
                        }
                    } else {
                        for( std::size_t i = 0; i < numFaces; ++i ) {
                            const DWORD* face = (DWORD*)vertAcc.get_face( i ).first;
                            pTVFaces[i].setTVerts( face[0], face[1], face[2] );
                        }
                    }
                }
            }
        }

        dest.InvalidateGeomCache();
        dest.InvalidateTopologyCache();
    }
}

void polymesh_copy_time_offset( Mesh& dest, frantic::geometry::polymesh3_ptr polymesh, float timeOffset,
                                bool throwIfNotTriangles ) {
    polymesh_copy( dest, polymesh, throwIfNotTriangles );

    if( polymesh && timeOffset != 0 && polymesh->has_vertex_channel( _T("Velocity") ) ) {
        frantic::geometry::polymesh3_const_vertex_accessor<frantic::graphics::vector3f> velAcc =
            polymesh->get_const_vertex_accessor<frantic::graphics::vector3f>( _T("Velocity") );
        if( velAcc.has_custom_faces() ) {
            throw std::runtime_error(
                "polymesh_copy_time_offset() The \'Velocity\' channel of the supplied polymesh3 has custom faces." );
        }
        if( velAcc.vertex_count() != static_cast<std::size_t>( dest.numVerts ) ) {
            throw std::runtime_error(
                "polymesh_copy_time_offset() Internal Error: Mismatch between size of Velocity channel (" +
                boost::lexical_cast<std::string>( velAcc.vertex_count() ) + ") and number of vertices (" +
                boost::lexical_cast<std::string>( dest.numVerts ) + ")." );
        }
        for( int i = 0, iEnd = dest.numVerts; i < iEnd; ++i ) {
            const frantic::graphics::vector3f velocity = velAcc.get_vertex( static_cast<std::size_t>( i ) );
            dest.setVert( i, dest.getVert( i ) + timeOffset * ( *reinterpret_cast<const Point3*>( &velocity ) ) );
        }
        dest.InvalidateGeomCache();
    }
}

void make_polymesh( MNMesh& mesh ) {
    mesh.FillInMesh();
    mesh.EliminateBadVerts();
    mesh.MakePolyMesh();
}

} // namespace geometry
} // namespace max3d
} // namespace frantic
