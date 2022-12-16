// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"
#include <frantic/max3d/geometry/mesh.hpp>

#include <MeshNormalSpec.h> // how does this not get included by mesh.hpp
#include <boost/current_function.hpp>
#include <frantic/geometry/raytraced_geometry_collection.hpp>
#include <frantic/geometry/trimesh3.hpp>
#include <frantic/logging/progress_logger.hpp>
#include <frantic/math/radian.hpp>
#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/geometry/auto_mesh.hpp>
#include <frantic/max3d/particles/particle_flow_access.hpp>

using namespace frantic::graphics;
using namespace frantic::channels;
using namespace frantic::geometry;

namespace frantic {
namespace max3d {
namespace geometry {

Point3 get_normal_from_face( Mesh& mesh, Face& face, int vertex ) {
    DWORD smGroup = face.getSmGroup();

    // get the normal for this vertex
    Point3 normal( 0, 0, 0 );
    RVertex& rvert( mesh.getRVert( face.v[vertex] ) );
    int normalCount = ( rvert.rFlags & NORCT_MASK );

    // if there are multiple normals at this vertex
    if( normalCount > 1 ) {

        // choose the normal that belongs to the smoothing group of this face
        for( int n = 0; n < normalCount; ++n ) {
            if( rvert.ern[n].getSmGroup() & smGroup ) {
                normal = rvert.ern[n].getNormal();
                break;
            }
        }
    } else {
        // there is just a single normal at this vertex
        normal = rvert.rn.getNormal();
    }

    return normal;
}

void clear_mesh( Mesh& mesh ) {
    mesh.setNumFaces( 0 );
    mesh.setNumVerts( 0 );

    mesh.setNumMaps( 0 );
    mesh.freeAllVData();
    mesh.FreeAll();

    mesh.ClearVSelectionWeights();

    mesh.InvalidateGeomCache();
    mesh.InvalidateTopologyCache();
}

void mesh_copy( Mesh& dest, const trimesh3& source ) {
    frantic::logging::null_progress_logger nullLogger;

    mesh_copy( dest, source, nullLogger );
}

void mesh_copy( Mesh& dest, const trimesh3& source, frantic::logging::progress_logger& progressLogger ) {
    clear_mesh( dest );

    { // scope for progress_logger_subinterval_tracker
        frantic::logging::progress_logger_subinterval_tracker tracker( progressLogger, 0, 25 );

        dest.setNumFaces( (int)source.face_count() );
        progressLogger.update_progress( 25.f );

        dest.setNumVerts( (int)source.vertex_count() );
        progressLogger.update_progress( 50.f );

        // Set the verts
        for( std::size_t v = 0; v < source.vertex_count(); ++v ) {
            dest.setVert( (int)v, to_max_t( source.get_vertex( v ) ) );
        }
        progressLogger.update_progress( 75.f );

        // Set the face indices
        for( std::size_t f = 0; f < source.face_count(); ++f ) {
            vector3 vface = source.get_face( f );
            dest.faces[f].setVerts( vface.x, vface.y, vface.z );
            dest.faces[f].setEdgeVisFlags( 1, 1, 1 );
            dest.faces[f].setSmGroup( 1 );
        }
        progressLogger.update_progress( 100.f );
    }

    // Copy the numbered vertex map channels and any other appropriate named vertex channels in the source mesh.
    std::vector<frantic::tstring> channelNames;
    source.get_vertex_channel_names( channelNames );
    bool hasSmoothingGroup = false, hasSmoothingGroups = false, hasMatID = false, hasVSelection = false,
         hasEdgeVisibility = false;
    {
        std::size_t total = channelNames.size();
        frantic::logging::progress_logger_subinterval_tracker tracker( progressLogger, 25, 50 );

        for( std::size_t i = 0; i < channelNames.size(); ++i ) {
            const frantic::tstring& name = channelNames[i];
            // First determine whether this is a channel we can copy to 3ds Max
            bool isChannel = false;
            int channelNumber = 0;
            if( name == _T("Color") ) {
                isChannel = true;
                channelNumber = 0;
            } else if( name == _T("TextureCoord") ) {
                isChannel = true;
                channelNumber = 1;
            } else if( name.substr( 0, 12 ) ==
                       _T("__mapChannel") ) { // This naming is deprecated ("__mapChannel42" is what it would look like)
                isChannel = true;
                channelNumber = _tstoi( name.substr( 12 ).c_str() );
            } else if( name.substr( 0, 7 ) ==
                       _T("Mapping") ) { // This is the new preferred naming ("Mapping42" is what it would look like)
                isChannel = true;
                channelNumber = _tstoi( name.substr( 7 ).c_str() );
            } else if( name == _T("Selection") ) {
                hasVSelection = true;
            } else if( name == _T("SmoothingGroup") ) {
                hasSmoothingGroup = true;
            } else if( name ==
                       _T("SmoothingGroups") ) { // TODO: this has to go, it was accidentally pluralized initially
                hasSmoothingGroups = true;
            }
            if( isChannel ) {
                const_trimesh3_vertex_channel_general_accessor ca = source.get_vertex_channel_general_accessor( name );
                if( ca.arity() != 3 )
                    throw std::runtime_error(
                        "max3d::mesh_copy: Cannot copy channel \"" + frantic::strings::to_string( name ) +
                        "\" to the 3ds max mesh, because the arity of this channel is " +
                        boost::lexical_cast<std::string>( ca.arity() ) + " instead of 3 in the source mesh." );

                // Initialize the map support
                dest.setMapSupport( channelNumber );
                dest.setNumMapVerts( channelNumber, (int)ca.size() );
                dest.setNumMapFaces( channelNumber, (int)ca.face_count() );

                // Copy the vertex data
                UVVert* mapVerts = dest.mapVerts( channelNumber );
                if( ca.data_type() == data_type_float32 ) {
                    for( unsigned i = 0; i < ca.size(); ++i ) {
                        memcpy( &( mapVerts[i] ), ca.data( i ), 12 );
                    }
                } else {
                    // Get a type conversion function, so we can transfer float16 or float64 inputs as well.
                    channel_type_convertor_function_t convertType = get_channel_type_convertor_function(
                        ca.data_type(), data_type_float32, frantic::strings::to_tstring( name ) );

                    for( unsigned i = 0; i < ca.size(); ++i ) {
                        convertType( reinterpret_cast<char*>( &( mapVerts[i] ) ), ca.data( i ), 3 );
                    }
                }

                // Copy the face data
                TVFace* mapFaces = dest.mapFaces( channelNumber );
                for( unsigned i = 0; i < ca.face_count(); ++i ) {
                    vector3 face = ca.face( i );
                    mapFaces[i].t[0] = face.x;
                    mapFaces[i].t[1] = face.y;
                    mapFaces[i].t[2] = face.z;
                }
            }
            progressLogger.update_progress( i + 1, total );
        }

        progressLogger.update_progress( 100.0 );
    }

    /*
            // I'm disabling this until we need it, as it is extra overhead.  Once we get a flexible channel interface
            // for saving loading, or we actually need it, I'll reactivate it.
            if ( hasNormals && !hasSmoothingGroups ) {
                    const_trimesh3_vertex_channel_accessor<vector3f> normals =
       source.get_vertex_channel_accessor<vector3f>("Normal"); if ( !normals.has_custom_faces() ) throw
       std::runtime_error("frantic::max3d::geometry::mesh_copy() - The mesh has a normals channel, but it does not have
       custom faces.");

                    MeshNormalSpec *mns = (MeshNormalSpec *) dest.GetInterface (MESH_NORMAL_SPEC_INTERFACE);
                    mns->SetParent(&dest);  // apparently i have to "parent" the normal spec
                    if ( !mns )
                            throw std::runtime_error("frantic::max3d::geometry::mesh_copy() - Could not retrieve the
       MeshNormalSpec from the destination mesh object."); mns->SetNumFaces((int)(normals.face_count()));
                    mns->SetNumNormals((int)(normals.size()));
                    mns->SetAllExplicit();

                    for( int f = 0; f < normals.face_count(); ++f ) {
                            for ( int v = 0; v < 3; ++v ) {
                                    int	vertIndex = normals.face(f)[v];
                                    Point3 n = normals[vertIndex];
                                    mns->SetNormal( f, v, n );
                            }
                    }
            }
    */
    if( hasSmoothingGroup ) {
        const_trimesh3_vertex_channel_accessor<int> sgAcc =
            source.get_vertex_channel_accessor<int>( _T("SmoothingGroup") );
        if( !sgAcc.has_custom_faces() )
            throw std::runtime_error(
                "frantic::max3d::geometry::mesh_copy() - The trimesh3 requested for copy into max has a "
                "'SmoothingGroups' channel, but does not have custom faces.  The current implementation of our support "
                "for smoothing groups requires that the channel have custom faces with a number of verts corresponding "
                "to the number of faces, and each vert data be an int indicating the smoothing group flags." );
        if( sgAcc.size() != source.face_count() )
            throw std::runtime_error(
                "frantic::max3d::geometry::mesh_copy() - The trimesh3 requested for copy into max has a "
                "'SmoothingGroups' channel, but the vertex count in that channel does not match the face count.  The "
                "current implementation of our support for smoothing groups requires that the channel have custom "
                "faces with a number of verts corresponding to the number of faces, and each vert data be an int "
                "indicating the smoothing group flags." );
        for( size_t f = 0; f < sgAcc.size(); ++f )
            dest.faces[f].smGroup = sgAcc[f];
    }

    // TODO: this has to go, it was accidentally pluralized initially
    if( hasSmoothingGroups ) {
        const_trimesh3_vertex_channel_accessor<int> sgAcc =
            source.get_vertex_channel_accessor<int>( _T("SmoothingGroups") );
        if( !sgAcc.has_custom_faces() )
            throw std::runtime_error(
                "frantic::max3d::geometry::mesh_copy() - The trimesh3 requested for copy into max has a "
                "'SmoothingGroups' channel, but does not have custom faces.  The current implementation of our support "
                "for smoothing groups requires that the channel have custom faces with a number of verts corresponding "
                "to the number of faces, and each vert data be an int indicating the smoothing group flags." );
        if( sgAcc.size() != source.face_count() )
            throw std::runtime_error(
                "frantic::max3d::geometry::mesh_copy() - The trimesh3 requested for copy into max has a "
                "'SmoothingGroups' channel, but the vertex count in that channel does not match the face count.  The "
                "current implementation of our support for smoothing groups requires that the channel have custom "
                "faces with a number of verts corresponding to the number of faces, and each vert data be an int "
                "indicating the smoothing group flags." );
        for( size_t f = 0; f < sgAcc.size(); ++f )
            dest.faces[f].smGroup = sgAcc[f];
    }

    if( hasVSelection ) {
        dest.SupportVSelectionWeights();
        const_trimesh3_vertex_channel_accessor<float> vsAcc =
            source.get_vertex_channel_accessor<float>( _T("Selection") );
        float* selectionWeights = dest.getVSelectionWeights();
        for( unsigned i = 0; i < vsAcc.size(); i++ ) {
            selectionWeights[i] = vsAcc[i];
        }
    }

    // Copy the any appropriate named face channels in the source mesh.
    channelNames.clear();
    source.get_face_channel_names( channelNames );
    hasSmoothingGroup = false;
    hasSmoothingGroups = false;
    hasMatID = false;
    for( std::size_t i = 0; i < channelNames.size(); ++i ) {
        const frantic::tstring& name = channelNames[i];
        if( name == _T("SmoothingGroup") )
            hasSmoothingGroup = true;
        else if( name == _T("SmoothingGroups") ) // TODO: this has to go, it was accidentally pluralized initially
            hasSmoothingGroups = true;
        else if( name == _T("MaterialID") )
            hasMatID = true;
        else if( name == _T("FaceEdgeVisibility") )
            hasEdgeVisibility = true;
    }

    if( hasSmoothingGroup ) {
        const_trimesh3_face_channel_accessor<int> sgAcc = source.get_face_channel_accessor<int>( _T("SmoothingGroup") );
        for( size_t f = 0; f < sgAcc.size(); ++f )
            dest.faces[f].smGroup = sgAcc[f];
    }

    // TODO: this has to go, it was accidentally pluralized initially
    if( hasSmoothingGroups ) {
        const_trimesh3_face_channel_accessor<int> sgAcc =
            source.get_face_channel_accessor<int>( _T("SmoothingGroups") );
        for( size_t f = 0; f < sgAcc.size(); ++f )
            dest.faces[f].smGroup = sgAcc[f];
    }

    if( hasMatID ) {
        const_trimesh3_face_channel_accessor<unsigned short> matIDAcc =
            source.get_face_channel_accessor<unsigned short>( _T("MaterialID") );
        for( int f = 0; f < (int)matIDAcc.size(); ++f )
            dest.setFaceMtlIndex( f, matIDAcc[f] );
    }

    if( hasEdgeVisibility ) {
        const_trimesh3_face_channel_accessor<boost::int8_t> visAcc =
            source.get_face_channel_accessor<boost::int8_t>( _T("FaceEdgeVisibility") );
        for( int f = 0; f < (int)visAcc.size(); ++f ) {
            char vis = visAcc[f];
            int va = ( vis & EDGE_A ) ? EDGE_VIS : EDGE_INVIS;
            int vb = ( vis & EDGE_B ) ? EDGE_VIS : EDGE_INVIS;
            int vc = ( vis & EDGE_C ) ? EDGE_VIS : EDGE_INVIS;
            dest.faces[f].setEdgeVisFlags( va, vb, vc );
        }
    }

    dest.InvalidateEdgeList();
    dest.InvalidateTopologyCache();
    dest.buildNormals();
    dest.buildBoundingBox();

    progressLogger.update_progress( 100.0 );
}

void mesh_copy_time_offset( Mesh& dest, const trimesh3& source, float timeOffset ) {
    frantic::logging::null_progress_logger nullLogger;
    mesh_copy_time_offset( dest, source, timeOffset, nullLogger );
}

void mesh_copy_time_offset( Mesh& dest, const trimesh3& source, float timeOffset,
                            frantic::logging::progress_logger& progressLogger ) {
    // First copy the mesh
    mesh_copy( dest, source, progressLogger );

    // Then move the vertices based on the velocity
    // TODO: Also use the acceleration channel if it exists, to get motion blur arcs instead of lines!
    if( timeOffset != 0 && source.has_vertex_channel( _T("Velocity") ) ) {
        const_trimesh3_vertex_channel_general_accessor ca =
            source.get_vertex_channel_general_accessor( _T("Velocity") );
        if( ca.arity() != 3 )
            throw std::runtime_error( "max3d::mesh_copy_time_offset: The velocity channel from the input mesh had an "
                                      "arity different from 3." );
        if( ca.has_custom_faces() )
            throw std::runtime_error( "max3d::mesh_copy_time_offset: The velocity channel of the input mesh has custom "
                                      "faces, which means it can't be applied for motion blur." );

        if( ca.data_type() == data_type_float32 ) {
            for( int i = 0; i < dest.getNumVerts(); ++i ) {
                dest.setVert( i,
                              dest.getVert( i ) + timeOffset * ( *reinterpret_cast<const Point3*>( ca.data( i ) ) ) );
            }
        } else {
            // Get a type conversion function, so we can use float16 or float64 inputs as well.
            channel_type_convertor_function_t convertType =
                get_channel_type_convertor_function( ca.data_type(), data_type_float32, _T("Velocity") );

            for( int i = 0; i < dest.getNumVerts(); ++i ) {
                Point3 velocity;
                convertType( reinterpret_cast<char*>( &velocity ), ca.data( i ), 3 );
                dest.setVert( i, dest.getVert( i ) + timeOffset * velocity );
            }
        }
    }
}

namespace detail {

void copy_smoothing_groups( trimesh3& dest, Mesh& source ) {
    dest.add_face_channel<int>( _T("SmoothingGroup") );
    trimesh3_face_channel_accessor<int> sgAcc = dest.get_face_channel_accessor<int>( _T("SmoothingGroup") );
    for( int f = 0; f < source.numFaces; ++f ) {
        sgAcc[f] = source.faces[f].smGroup;
    }
}

void copy_material_IDs( trimesh3& dest, Mesh& source ) {
    dest.add_face_channel<unsigned short>( _T("MaterialID") );
    trimesh3_face_channel_accessor<unsigned short> matIDAcc =
        dest.get_face_channel_accessor<unsigned short>( _T("MaterialID") );
    for( int f = 0; f < source.numFaces; ++f ) {
        matIDAcc[f] = source.getFaceMtlIndex( f );
    }
}

// TODO: Use an edge channel instead
void copy_edge_visibility( trimesh3& dest, Mesh& source ) {
    dest.add_face_channel<boost::int8_t>( _T("FaceEdgeVisibility") );
    trimesh3_face_channel_accessor<boost::int8_t> visAcc =
        dest.get_face_channel_accessor<boost::int8_t>( _T("FaceEdgeVisibility") );
    for( int f = 0; f < source.numFaces; ++f ) {
        visAcc[f] = static_cast<boost::int8_t>( source.faces[f].flags & EDGE_ALL );
    }
}

void copy_mesh_normals( trimesh3& dest, Mesh& source ) {

    // TODO:  Write our own trimesh normal construction code.  The docs on what this buildNormals call does
    // exactly are kind of sketchy.  The RNormal class reference also states that:
    //	Note:  This class is used internally by 3ds Max. Developers who need to compute face
    //	and vertex normals for a mesh should instead refer to the Advanced Topics section
    //	Computing Face and Vertex Normals.
    // Conrad insists it is ok though because he uses it all the time in Amaretto/Gelato.

    // build the mesh normals
    source.buildRenderNormals();

    MeshNormalSpec* meshNormalSpec =
        reinterpret_cast<MeshNormalSpec*>( source.GetInterface( MESH_NORMAL_SPEC_INTERFACE ) );

    // create a normals channel with custom faces
    dest.add_vertex_channel<vector3f>( _T("Normal"), 0, true );
    trimesh3_vertex_channel_accessor<vector3f> normals = dest.get_vertex_channel_accessor<vector3f>( _T("Normal") );

    if( meshNormalSpec && meshNormalSpec->GetNumNormals() ) {
        normals.set_vertex_count( meshNormalSpec->GetNumNormals() );
        for( int i = 0; i < meshNormalSpec->GetNumNormals(); ++i ) {
            normals[i] = from_max_t( meshNormalSpec->GetNormalArray()[i] );
        }

        for( int i = 0; i < source.getNumFaces(); ++i ) {
            for( int corner = 0; corner < 3; ++corner ) {
                const int normalIndex = meshNormalSpec->GetNormalIndex( i, corner );
                if( normalIndex < 0 ) {
                    throw std::runtime_error( "copy_mesh_normals Error: normal index is negative" );
                }
                if( normalIndex >= meshNormalSpec->GetNumNormals() ) {
                    throw std::runtime_error(
                        "copy_mesh_normals Error: normal index out of range (" +
                        boost::lexical_cast<std::string>( normalIndex ) +
                        " >= " + boost::lexical_cast<std::string>( meshNormalSpec->GetNumNormals() ) + ")" );
                }
                normals.face( i )[corner] = normalIndex;
            }
        }
    } else {
        // Build the index structure for adding them to the trimesh.  this will keep track of which normals
        // have already been added, and where.  we also need to count the normals so that we can allocate the
        // space in the trimesh.
        std::vector<std::map<DWORD, int>> normalIndices( source.getNumVerts() );

        // go through all the faces in the mesh and add them and their vertex normals to the channel
        Face* sourceFace = source.faces;
        DWORD sg; // smoothing group
        vector3 destFace;
        int numNormalsAdded = 0;
        for( int i = 0; i < source.getNumFaces(); ++i, ++sourceFace ) {
            sg = sourceFace->getSmGroup();

            if( sg == 0 ) {
                // Smoothing group 0 indicates "no smoothing"
                vector3f normalToAdd = from_max_t( source.getFaceNormal( i ) );
                normals.add_vertex( normalToAdd );
                for( int j = 0; j < 3; ++j ) {
                    destFace[j] = numNormalsAdded;
                }
                ++numNormalsAdded;
            } else {
                // find the verts that this face points to and check if we've already added the normals
                // for them to the trimesh.  if so, just use those indices, if not, add the normal and use
                // its new index.
                for( int j = 0; j < 3; ++j ) {

                    // check if we need this normal first or if it has been added already
                    int vert = sourceFace->v[j];
                    std::map<DWORD, int>::iterator it = normalIndices[vert].find( sg );
                    if( it == normalIndices[vert].end() ) {
                        // if the normal for this vert for this smoothing group isnt added yet, add it
                        // to the mesh and add its index to index structure
                        vector3f normalToAdd = from_max_t( get_normal_from_face( source, *sourceFace, j ) );
                        if( numNormalsAdded == (int)( normals.size() ) )
                            normals.add_vertex( normalToAdd );
                        else
                            normals[numNormalsAdded] = normalToAdd;
                        destFace[j] = numNormalsAdded;
                        normalIndices[vert][sg] = numNormalsAdded;
                        numNormalsAdded++;

                    }
                    // if it's already in there, just use the index you find in the index structure
                    else
                        destFace[j] = it->second;
                }
            }

            // add the face
            normals.face( i ) = destFace;
        }
    }
}

void copy_mesh_normals( trimesh3& dest, const transform4f& sourceXform, Mesh& source ) {
    copy_mesh_normals( dest, source );

    trimesh3_vertex_channel_accessor<vector3f> normals = dest.get_vertex_channel_accessor<vector3f>( _T("Normal") );

    const frantic::graphics::transform4f normalXform = sourceXform.to_inverse().to_transpose();

    for( std::size_t i = 0, ie = normals.size(); i < ie; ++i ) {
        normals[i] = normalXform.transform_no_translation( normals[i] );
    }
}

void copy_mesh_extrachannels( trimesh3& dest, Mesh& source, const frantic::channels::channel_propagation_policy& cpp ) {

    //	std::ofstream fout1("c:\\temp\\mesh_copy.log");
    //	fout1 << "*** Map Channel Count " << source.getNumMaps() << std::endl;
    for( int mapChannel = 0; mapChannel < source.getNumMaps(); ++mapChannel ) {
        MeshMap& mm = source.Map( mapChannel );
        if( mm.IsUsed() ) {

            //			fout1 << "*** Map Channel " << mapChannel << std::endl;
            // The __mapChannel# name corresponds to the convention used in Amaretto passing map channels to Gelato.
            frantic::tstring channelName;
            if( mapChannel > 1 )
                channelName = _T("Mapping") + boost::lexical_cast<frantic::tstring>( mapChannel );
            else if( mapChannel == 1 )
                channelName = _T("TextureCoord");
            else if( mapChannel == 0 )
                channelName = _T("Color");

            if( cpp.is_channel_included( channelName ) ) {
                //				fout1 << "*** Copying channel " << channelName << " to mesh ********" <<
                //std::endl;

                // Check whether the faces differ from the geometry faces
                bool hasCustomFaces = false;
                if( mm.fnum == (int)dest.face_count() && mm.tf != 0 ) {
                    // Could possibly just assume they're different in this case.
                    for( int i = 0; i < source.getNumFaces(); ++i ) {
                        if( source.faces[i].v[0] != mm.tf->t[0] || source.faces[i].v[0] != mm.tf->t[0] ||
                            source.faces[i].v[0] != mm.tf->t[0] ) {
                            hasCustomFaces = true;
                            break;
                        }
                    }
                }
                if( hasCustomFaces ) {
                    dest.add_vertex_channel<vector3f>( channelName, mm.vnum, true );
                    trimesh3_vertex_channel_accessor<vector3f> channel =
                        dest.get_vertex_channel_accessor<vector3f>( channelName );
                    if( !channel )
                        throw std::runtime_error( "copy_mesh_extrachannels: Error creating a channel named \"" +
                                                  frantic::strings::to_string( channelName ) +
                                                  "\" in the destination trimesh3." );

                    for( int i = 0; i < mm.vnum; ++i )
                        channel[i] = from_max_t( mm.tv[i] );
                    for( int i = 0; i < mm.fnum; ++i )
                        channel.face( i ) = vector3( (int*)mm.tf[i].t );
                } else {
                    dest.add_vertex_channel<vector3f>( channelName );
                    trimesh3_vertex_channel_accessor<vector3f> channel =
                        dest.get_vertex_channel_accessor<vector3f>( channelName );
                    if( !channel )
                        throw std::runtime_error( "copy_mesh_extrachannels: Error creating a channel named \"" +
                                                  frantic::strings::to_string( channelName ) +
                                                  "\" in the destination trimesh3." );

                    for( unsigned i = 0; i < channel.size(); ++i )
                        channel[i] = from_max_t( mm.tv[i] );
                }
            }
        }
    }

    // If there's a vertex selection copy that channel
    if( source.selLevel == MESH_VERTEX && cpp.is_channel_included( _T("Selection") ) ) {
        dest.add_vertex_channel<float>( _T("Selection") );
        trimesh3_vertex_channel_accessor<float> channel = dest.get_vertex_channel_accessor<float>( _T("Selection") );

        // try to get soft selection
        if( source.getVSelectionWeights() != 0 ) {
            float* selectionWeights = source.getVSelectionWeights();

            for( unsigned i = 0; i < channel.size(); ++i )
                channel[i] = selectionWeights[i];
        } else { // use the bit array if no soft selection data is present
            BitArray selected = source.VertSel();

            for( int i = 0; i < selected.GetSize(); ++i )
                channel[i] = (float)selected[i];
        }
    }

    // if theres a face selection copy that
    if( source.selLevel == MESH_FACE && cpp.is_channel_included( _T("FaceSelection") ) ) {
        dest.add_face_channel<int>( _T("FaceSelection") );
        trimesh3_face_channel_accessor<int> fAcc = dest.get_face_channel_accessor<int>( _T("FaceSelection") );
        BitArray fSel = source.FaceSel();

        for( int i = 0; i < fSel.GetSize(); ++i )
            fAcc[i] = fSel[i];
    }
}

} // namespace detail

namespace {

frantic::channels::channel_propagation_policy get_default_channel_propagation_policy( bool geometryOnly ) {
    frantic::channels::channel_propagation_policy cpp( geometryOnly );
    if( !geometryOnly ) {
        cpp.add_channel( _T("FaceEdgeVisibility") );
        cpp.add_channel( _T("Normal") );
    }
    return cpp;
}

} // anonymous namespace

void append_inode_to_mesh( INode* node, TimeValue t, Interval& outValidity, trimesh3& mesh ) {
    if( !node )
        throw std::runtime_error( "append_inode_to_mesh: Got a null node" );

    Object* obj = node->EvalWorldState( t ).obj;
    if( !obj )
        throw std::runtime_error( std::string( "append_inode_to_mesh: Node \"" ) +
                                  frantic::strings::to_string( node->GetName() ) +
                                  "\" does not evaluate to a valid object" );

    if( !obj->CanConvertToType( triObjectClassID ) ) // TODO: throw an exception?
        return;

    TriObject* pTriObj = static_cast<TriObject*>( obj->ConvertToType( t, triObjectClassID ) );

    transform4f xform( node->GetObjTMAfterWSM( t, &outValidity ) );
    outValidity &= obj->ObjectValidity( t );

    int vertexOffset = (int)mesh.vertex_count();
    for( int i = 0; i < pTriObj->mesh.getNumVerts(); ++i ) {
        vector3f pt( pTriObj->mesh.getVert( i ) );
        mesh.add_vertex( xform * pt );
    }

    for( int i = 0; i < pTriObj->mesh.getNumFaces(); ++i ) {
        mesh.add_face( pTriObj->mesh.faces[i].getVert( 0 ) + vertexOffset,
                       pTriObj->mesh.faces[i].getVert( 1 ) + vertexOffset,
                       pTriObj->mesh.faces[i].getVert( 2 ) + vertexOffset );
    }

    if( pTriObj != obj )
        pTriObj->MaybeAutoDelete();
}

void append_inode_to_mesh( INode* node, TimeValue t, trimesh3& mesh ) {
    Interval garbage = FOREVER;
    append_inode_to_mesh( node, t, garbage, mesh );
}

void mesh_copy( trimesh3& dest, Mesh& source, bool geometryOnly ) {
    mesh_copy( dest, source, get_default_channel_propagation_policy( geometryOnly ) );
}

void mesh_copy( trimesh3& dest, Mesh& source, const frantic::channels::channel_propagation_policy& cpp ) {
    dest.clear();

    dest.set_vertex_count( source.getNumVerts() );
    dest.set_face_count( source.getNumFaces() );

    for( std::size_t i = 0; i < dest.vertex_count(); ++i )
        dest.get_vertex( i ) = from_max_t( source.getVert( (int)i ) );

    for( std::size_t i = 0; i < dest.face_count(); ++i )
        dest.get_face( i ) = vector3( (int*)source.faces[i].v );

    detail::copy_mesh_extrachannels( dest, source, cpp );
    if( cpp.is_channel_included( _T("SmoothingGroup") ) )
        detail::copy_smoothing_groups( dest, source );
    if( cpp.is_channel_included( _T("MaterialID") ) )
        detail::copy_material_IDs( dest, source );
    if( cpp.is_channel_included( _T("FaceEdgeVisibility") ) )
        detail::copy_edge_visibility( dest, source );
    if( cpp.is_channel_included( _T("Normal") ) )
        detail::copy_mesh_normals( dest, source );
}

void mesh_copy( trimesh3& dest, const transform4f& sourceXform, Mesh& source, bool geometryOnly ) {
    mesh_copy( dest, sourceXform, source, get_default_channel_propagation_policy( geometryOnly ) );
}

void mesh_copy( trimesh3& dest, const transform4f& sourceXform, Mesh& source,
                const frantic::channels::channel_propagation_policy& cpp ) {
    dest.clear();

    dest.set_vertex_count( source.getNumVerts() );
    dest.set_face_count( source.getNumFaces() );

    for( std::size_t i = 0; i < dest.vertex_count(); ++i )
        dest.get_vertex( i ) = sourceXform * from_max_t( source.getVert( (int)i ) );

    for( std::size_t i = 0; i < dest.face_count(); ++i )
        dest.get_face( i ) = vector3( (int*)source.faces[i].v );

    detail::copy_mesh_extrachannels( dest, source, cpp );
    if( cpp.is_channel_included( _T("SmoothingGroup") ) )
        detail::copy_smoothing_groups( dest, source );
    if( cpp.is_channel_included( _T("MaterialID") ) )
        detail::copy_material_IDs( dest, source );
    if( cpp.is_channel_included( _T("FaceEdgeVisibility" ) ) )
        detail::copy_edge_visibility( dest, source );
    if( cpp.is_channel_included( _T("Normal") ) )
        detail::copy_mesh_normals( dest, sourceXform, source );
}

void mesh_copy( trimesh3& dest, const transform4f& sourceXformFirst, const transform4f& sourceXformSecond, Mesh& source,
                float timeStep, bool geometryOnly ) {
    mesh_copy( dest, sourceXformFirst, sourceXformSecond, source, timeStep,
               get_default_channel_propagation_policy( geometryOnly ) );
}

void mesh_copy( trimesh3& dest, const transform4f& sourceXformFirst, const transform4f& sourceXformSecond, Mesh& source,
                float timeStep, const frantic::channels::channel_propagation_policy& cpp ) {
    mesh_copy( dest, sourceXformFirst, source, cpp );

    if( sourceXformFirst != sourceXformSecond && cpp.is_channel_included( _T("Velocity") ) ) {
        dest.add_vertex_channel<vector3f>( _T("Velocity") );
        trimesh3_vertex_channel_accessor<vector3f> velocityChannel =
            dest.get_vertex_channel_accessor<vector3f>( _T("Velocity") );
        transform4f xformDerivative = ( sourceXformSecond - sourceXformFirst ) / timeStep;
        for( unsigned i = 0; i < velocityChannel.size(); ++i )
            velocityChannel[i] = xformDerivative * vector3f( source.getVert( i ) );
    }
}

namespace {
// A set which can hold up to three boost::int32_t's.
// We're using this instead of std::set to improve performance.
// equal_topology() had poor performance when using std::set
// because of the memory allocation in std::set::insert().
class set3i {
    int m_size;
    boost::int32_t m_values[3]; // stored in increasing order
  public:
    set3i()
        : m_size( 0 ) {}

    void clear() { m_size = 0; }

    void insert( boost::int32_t val ) {
        // search for val (or greater) in m_values
        for( int i = 0; i < m_size; ++i ) {
            if( m_values[i] > val ) {
                // keep m_values in increasing order
                // move subsequent values over
                for( int j = m_size; j > i; --j ) {
                    m_values[j] = m_values[j - 1];
                }
                // insert val in the current position
                m_values[i] = val;
                ++m_size;
                return;
            } else if( m_values[i] == val ) {
                // if val is already in the set, do nothing
                return;
            }
        }
        m_values[m_size] = val;
        ++m_size;
    }

    bool operator==( const set3i& other ) const {
        if( m_size != other.m_size ) {
            return false;
        }
        for( int i = 0; i < m_size; ++i ) {
            if( m_values[i] != other.m_values[i] ) {
                return false;
            }
        }
        return true;
    }

    bool operator!=( const set3i& other ) const { return !( *this == other ); }

    std::size_t size() const { return static_cast<std::size_t>( m_size ); }

    boost::int32_t operator[]( std::size_t i ) const { return m_values[i]; }
};
} // anonymous namespace

bool equal_topology( const Mesh& sourceFirst, const Mesh& sourceSecond ) {
    if( sourceFirst.getNumVerts() != sourceSecond.getNumVerts() )
        return false;

    if( sourceFirst.getNumFaces() != sourceSecond.getNumFaces() )
        return false;

    set3i set1, set2;
    for( int i = 0; i < sourceFirst.getNumFaces(); ++i ) {
        set1.clear();
        set2.clear();
        for( int j = 0; j < 3; j++ ) {
            set1.insert( sourceFirst.faces[i].v[j] );
            set2.insert( sourceSecond.faces[i].v[j] );
        }
        if( set1 != set2 ) {
            return false;
        }
    }
    return true;
}

bool equal_topology( const frantic::geometry::trimesh3& sourceFirst, const Mesh& sourceSecond ) {
    if( sourceFirst.vertex_count() != static_cast<std::size_t>( sourceSecond.getNumVerts() ) )
        return false;

    if( sourceFirst.face_count() != static_cast<std::size_t>( sourceSecond.getNumFaces() ) )
        return false;

    // I think we could accomplish the same tests by using a 6 element array and sorting it then verifying pairs match
    // at each step.
    set3i set1, set2;
    for( std::size_t i = 0; i < sourceFirst.face_count(); ++i ) {
        set1.clear();
        set2.clear();
        for( int j = 0; j < 3; j++ ) {
            set1.insert( sourceFirst.get_face( i )[j] );
            set2.insert( sourceSecond.faces[i].v[j] );
        }
        if( set1 != set2 ) {
            return false;
        }
    }
    return true;
}

// This is probably not the best solution, but it's better than what we were doing before
bool equal_topology( const MNMesh& sourceFirst, const MNMesh& sourceSecond ) {
    if( sourceFirst.VNum() != sourceSecond.VNum() )
        return false;

    if( sourceFirst.FNum() != sourceSecond.FNum() )
        return false;

    for( int i = 0; i < sourceFirst.FNum(); ++i ) {
        MNFace* face1 = sourceFirst.F( i );
        MNFace* face2 = sourceSecond.F( i );

        int deg1 = face1->deg;
        int deg2 = face2->deg;

        if( deg1 != deg2 )
            return false;

        for( int j = 0; j < deg1; ++j ) {
            if( face1->vtx[j] != face2->vtx[j] ) {
                return false;
            }
        }
    }
    return true;
}

void mesh_copy( trimesh3& dest, const transform4f& sourceXformFirst, const transform4f& sourceXformSecond,
                Mesh& sourceFirst, Mesh& sourceSecond, float timeStep, bool geometryOnly ) {
    mesh_copy( dest, sourceXformFirst, sourceXformSecond, sourceFirst, sourceSecond, timeStep,
               get_default_channel_propagation_policy( geometryOnly ) );
}

void mesh_copy( trimesh3& dest, const transform4f& sourceXformFirst, const transform4f& sourceXformSecond,
                Mesh& sourceFirst, Mesh& sourceSecond, float timeStep,
                const frantic::channels::channel_propagation_policy& cpp ) {
    if( !equal_topology( sourceFirst, sourceSecond ) )
        throw std::runtime_error( "mesh_copy: The two provided meshes have changing topology, so cannot be used to "
                                  "determine vertex velocities." );

    mesh_copy( dest, sourceXformFirst, sourceFirst, cpp );

    if( cpp.is_channel_included( _T("Velocity") ) ) {
        dest.add_vertex_channel<vector3f>( _T("Velocity") );
        trimesh3_vertex_channel_accessor<vector3f> velocityChannel =
            dest.get_vertex_channel_accessor<vector3f>( _T("Velocity") );
        for( unsigned i = 0; i < velocityChannel.size(); ++i )
            velocityChannel[i] = ( sourceXformSecond * from_max_t( sourceSecond.getVert( i ) ) -
                                   sourceXformFirst * from_max_t( sourceFirst.getVert( i ) ) ) /
                                 timeStep;
    }
}

void mesh_copy_mapchannel_to_mesh( trimesh3& dest, Mesh& source, int sourceMapChannel ) {
    if( !source.mapSupport( sourceMapChannel ) )
        throw std::runtime_error( "copy_mesh_mapchannel_to_mesh: Tried to copy a map channel, " +
                                  boost::lexical_cast<std::string>( sourceMapChannel ) +
                                  ", which the source mesh doesn't support." );

    dest.clear();

    dest.set_vertex_count( source.getNumMapVerts( sourceMapChannel ) );
    dest.set_face_count( source.getNumFaces() );

    for( std::size_t i = 0; i < dest.vertex_count(); ++i )
        dest.get_vertex( i ) = from_max_t( source.mapVerts( sourceMapChannel )[i] );

    for( std::size_t i = 0; i < dest.face_count(); ++i )
        dest.get_face( i ) = vector3( (int*)source.mapFaces( sourceMapChannel )[i].t );
}

bool mesh_copy_velocity_to_mesh( frantic::geometry::trimesh3& dest, const frantic::graphics::transform4f& sourceXform,
                                 Mesh& source, float timeStepSeconds ) {
    if( !equal_topology( dest, source ) )
        return false;

    if( !dest.has_vertex_channel( _T("Velocity") ) )
        dest.add_vertex_channel<vector3f>( _T("Velocity") );

    trimesh3_vertex_channel_accessor<vector3f> velocityChannel =
        dest.get_vertex_channel_accessor<vector3f>( _T("Velocity") );
    for( unsigned i = 0; i < velocityChannel.size(); ++i )
        velocityChannel[i] =
            ( sourceXform * from_max_t( source.getVert( i ) ) - dest.get_vertex( i ) ) / timeStepSeconds;

    return true;
}

void scale_mesh_verts( Mesh& mesh, float scale ) {
    for( int v = 0; v < mesh.getNumVerts(); ++v )
        mesh.setVert( v, scale * mesh.getVert( v ) );
}

void write_mesh( std::ostream& out, Mesh& m ) {
    out << "Mesh:" << std::endl;
    out << "FaceCount: " << m.getNumFaces() << " VertexCount: " << m.getNumVerts() << std::endl;

    out << "Faces:" << std::endl;
    for( int i = 0; i < m.getNumFaces(); i++ ) {
        out << m.faces[i].getVert( 0 ) << " " << m.faces[i].getVert( 1 ) << " " << m.faces[i].getVert( 2 ) << std::endl;
    }

    out << "Verts:" << std::endl;
    for( int i = 0; i < m.getNumVerts(); i++ ) {
        Point3 p = m.getVert( i );
        out << p.x << " " << p.y << " " << p.z << std::endl;
    }
}

void interpolate_mesh( Mesh& dest, Mesh& source1, Mesh& source2, float alpha ) {
    assert( source1.getNumVerts() == source2.getNumVerts() );

    dest.DeepCopy( &source1, (ChannelMask)ALL_CHANNELS );

    // Interpolate vertex locations
    for( int i = 0; i < source1.getNumVerts(); i++ ) {
        Point3 interp = ( 1 - alpha ) * source1.getVert( i ) + alpha * source2.getVert( i );
        dest.setVert( i, interp );
    }

    // TODO Interpolate map channels
}

void get_uvw( MNMesh& mesh, int vert, int mapChannel, float& outU, float& outV, float& outW ) {
    if( vert < 0 || vert >= mesh.VNum() )
        throw std::runtime_error( "get_uvw: The vert index provided (" + boost::lexical_cast<std::string>( vert ) +
                                  ") was outside of the valid range for the given mesh." );

    if( mesh.M( mapChannel ) == 0 || mesh.M( mapChannel )->VNum() == 0 || mesh.M( mapChannel )->FNum() == 0 )
        throw std::runtime_error( "get_uvw: The map channel requested (" +
                                  boost::lexical_cast<std::string>( mapChannel ) +
                                  ") doesn't exist within the given mesh." );

    if( !mesh.GetFlag( MN_MESH_FILLED_IN ) ) {
        mesh.FillInMesh();
    }

    Tab<int>& faces = mesh.vfac[vert];

    outU = 0;
    outV = 0;
    outW = 0;

    if( faces.Count() > 0 ) {
        MNFace& face = *mesh.F( faces[0] );
        MNMapFace& mapFace = *mesh.MF( mapChannel, faces[0] );

        // Find the vertex in the face
        int matchingVert = -1;
        for( int i = 0; i < face.deg; ++i ) {
            if( face.vtx[i] == vert )
                matchingVert = i;
        }
        if( matchingVert != -1 ) {
            // Get the map channel values
            vector3f vertMapChannelValue = from_max_t( mesh.MV( mapChannel, mapFace.tv[matchingVert] ) );
            outU = vertMapChannelValue.x;
            outV = vertMapChannelValue.y;
            outW = vertMapChannelValue.z;
        }
    }
}

void get_multimap_uvws(
    MNMesh& mesh, int vert, BitArray& mappingsRequired,
    UVVert* uvwArray ) // Assuming the array is large enough to fit all channels supplied by mappingsRequired
{
    if( vert < 0 || vert >= mesh.VNum() )
        throw std::runtime_error( "get_uvw: The vert index provided (" + boost::lexical_cast<std::string>( vert ) +
                                  ") was outside of the valid range for the given mesh." );

    if( !mesh.GetFlag( MN_MESH_FILLED_IN ) ) {
        mesh.FillInMesh();
    }

    Tab<int>& faces = mesh.vfac[vert]; // Collect the faces incident on this vertex
    if( faces.Count() == 0 )           // Don't do anything if there are no faces w/ this vertex
        return;

    for( int channel = 0; channel < mappingsRequired.GetSize(); ++channel ) {
        if( !mappingsRequired[channel] )
            continue;

        // Ensure this channel exists in the mesh
        if( mesh.M( channel ) == 0 || mesh.M( channel )->VNum() == 0 || mesh.M( channel )->FNum() == 0 )
            throw std::runtime_error( "get_multimap_uvws: The map channel requested (" +
                                      boost::lexical_cast<std::string>( channel ) +
                                      ") doesn't exist within the given mesh." );

        MNFace& face = *mesh.F( faces[0] );
        MNMapFace& mapFace = *mesh.MF( channel, faces[0] );

        // Find the vertex in the first face
        int matchingVert = -1;
        for( int i = 0; i < face.deg; ++i ) {
            if( face.vtx[i] == vert )
                matchingVert = i;
        }

        // Get the map channel values
        if( matchingVert != -1 )
            uvwArray[channel] = mesh.MV( channel, mapFace.tv[matchingVert] );
    }
}

void get_uvw_and_uv_derivatives( MNMesh& mesh, int vert, int mapChannel, float& outU, float& outV, float& outW,
                                 vector3f& outdPdu, vector3f& outdPdv ) {
    if( vert < 0 || vert >= mesh.VNum() )
        throw std::runtime_error( "get_uvw_and_uv_derivatives: The vert index provided (" +
                                  boost::lexical_cast<std::string>( vert ) +
                                  ") was outside of the valid range for the given mesh." );

    if( mesh.M( mapChannel ) == 0 || mesh.M( mapChannel )->VNum() == 0 || mesh.M( mapChannel )->FNum() == 0 )
        throw std::runtime_error( "get_uvw_and_uv_derivatives: The map channel requested (" +
                                  boost::lexical_cast<std::string>( mapChannel ) +
                                  ") doesn't exist within the given mesh." );

    if( !mesh.GetFlag( MN_MESH_FILLED_IN ) ) {
        mesh.FillInMesh();
    }

    vector3f vertPosition = from_max_t( mesh.P( vert ) );

    // Rather than storing the values and then compting the matrices (as described below), we compute them on the fly
    // as we run through the faces.
    float Atranspose_A[3] = { 0, 0, 0 };
    vector3f Atranspose_b[2];

    // Loop through all the faces that contain this vertex
    Tab<int>& faces = mesh.vfac[vert];
    for( int f = 0; f < faces.Count(); ++f ) {
        MNFace& face = *mesh.F( faces[f] );
        MNMapFace& mapFace = *mesh.MF( mapChannel, faces[f] );
        // Find the vertex in the face
        int matchingVert = -1;
        for( int i = 0; i < face.deg; ++i ) {
            if( face.vtx[i] == vert )
                matchingVert = i;
        }
        // Get all the corresponding map channel delta and position delta pairs based on the two edges adjacent to the
        // vert
        if( matchingVert != -1 ) {
            // Get the vertex positions and compute the angle of the face
            int firstVertIndex = ( matchingVert + face.deg - 1 ) % face.deg,
                secondVertIndex = ( matchingVert + 1 ) % face.deg;
            vector3f firstPositionDelta = vector3f( mesh.P( face.vtx[firstVertIndex] ) ) - vertPosition,
                     secondPositionDelta = vector3f( mesh.P( face.vtx[secondVertIndex] ) ) - vertPosition;

            // Get the map channel values
            vector3f vertMapChannelValue = from_max_t( mesh.MV( mapChannel, mapFace.tv[matchingVert] ) );
            if( f == 0 ) {
                outU = vertMapChannelValue.x;
                outV = vertMapChannelValue.y;
                outW = vertMapChannelValue.z;
            }
            vector3f firstMapChannelDelta =
                         vector3f( mesh.MV( mapChannel, mapFace.tv[firstVertIndex] ) ) - vertMapChannelValue,
                     secondMapChannelDelta =
                         vector3f( mesh.MV( mapChannel, mapFace.tv[secondVertIndex] ) ) - vertMapChannelValue;

            // Compute the Atranspose * A matrix and the 3 Atranspose * b vectors on the fly
            float x, y;
            x = firstMapChannelDelta.x;
            y = firstMapChannelDelta.y;
            Atranspose_A[0] += x * x;
            Atranspose_A[1] += x * y;
            Atranspose_A[2] += y * y;
            Atranspose_b[0] += x * firstPositionDelta;
            Atranspose_b[1] += y * firstPositionDelta;

            x = secondMapChannelDelta.x;
            y = secondMapChannelDelta.y;
            Atranspose_A[0] += x * x;
            Atranspose_A[1] += x * y;
            Atranspose_A[2] += y * y;
            Atranspose_b[0] += x * secondPositionDelta;
            Atranspose_b[1] += y * secondPositionDelta;
        } else {
            throw std::runtime_error(
                "get_uvw_and_uv_derivatives: The fedg list of an MNMesh provided inconsistent information." );
        }
    }

    float determinant = Atranspose_A[0] * Atranspose_A[2] - Atranspose_A[1] * Atranspose_A[1];
    if( determinant != 0 ) {
        // Compute the inverse of Atranspose * A
        float inverse_Atranspose_A[3];
        inverse_Atranspose_A[0] = Atranspose_A[2] / determinant;
        inverse_Atranspose_A[1] = -Atranspose_A[1] / determinant;
        inverse_Atranspose_A[2] = Atranspose_A[0] / determinant;

        // And complete the multiplication by the pseudo-inverse to get the desired vectors
        outdPdu = inverse_Atranspose_A[0] * Atranspose_b[0] + inverse_Atranspose_A[1] * Atranspose_b[1];
        outdPdv = inverse_Atranspose_A[1] * Atranspose_b[0] + inverse_Atranspose_A[2] * Atranspose_b[1];
    } else {
        outdPdu.set( 0 );
        outdPdv.set( 0 );
    }
}

void build_geometry_from_visible_inodes( raytraced_geometry_collection& geometry, std::vector<INode*>& renderNodes,
                                         TimeValue t, float motionBlurInterval, float shutterBias, bool geometryOnly,
                                         View& view, frantic::logging::progress_logger& progress ) {
    geometry.clear();
    for( unsigned i = 0; i < renderNodes.size(); i++ ) {
        // Skip the nodes whose (potentially animated) visibility value is not positive.
        if( !( renderNodes[i]->GetVisibility( t ) > 0 ) )
            continue;

        // first, get the transform from the inode.
        motion_blurred_transform<float> tempTransform =
            motion_blurred_transform<float>::from_objtmafterwsm( renderNodes[i], t, motionBlurInterval, shutterBias );
        // now get the mesh.
        trimesh3 tempMesh;

        // get the mesh from the inode...
        AutoMesh mesh = get_mesh_from_inode( renderNodes[i], t, view );

        // finally add the mesh and transform to the collection.
        if( mesh.get() != 0 ) {
            mesh_copy( tempMesh, *mesh.get(), geometryOnly );
            geometry.add_rigid_object_with_swap( tempTransform, tempMesh );
        }
        progress.update_progress( i + 1, renderNodes.size() );
    }
}

void filter_renderable_inodes( const std::vector<INode*>& inNodes, TimeValue t, std::vector<INode*>& outRenderNodes ) {
    // use a set to ensure unique renderable nodes.
    std::set<INode*> renderable;
    std::vector<INode*>::const_iterator node = inNodes.begin();

    for( ; node != inNodes.end(); node++ ) {
        // Only consider renderable nodes
        if( ( *node )->Renderable() ) {
            // try to get particle system groups from a particle system.
            std::set<INode*> groups;
            particles::extract_geometry_particle_groups( groups, ( *node ) );
            if( !groups.empty() ) // this node is a particle system. insert the geometry groups.
                renderable.insert( groups.begin(), groups.end() );
            else {
                // this node is not a particle system...
                // if it is an object that will give up a mesh, collect it.
                ObjectState os = ( *node )->EvalWorldState( t );
                Object* obj = os.obj;
                if( obj ) {
                    SClass_ID scid = obj->SuperClassID();

                    // If the object is a derived object, follow its references to the real object
                    // This is here because there were some biped objects not being saved when they should have been.
                    while( scid == GEN_DERIVOB_CLASS_ID ) {
                        obj = ( (IDerivedObject*)obj )->GetObjRef();
                        if( obj == 0 )
                            break;
                        else
                            scid = obj->SuperClassID();
                    }

                    // mprintf( "Node super class ID is %s\n", max3d::SuperClassIDToString(scid).c_str() );

                    // TODO: Does obj->IsRenderable() make the superclassid checks unnecessary?
                    if( ( scid == SHAPE_CLASS_ID || scid == GEOMOBJECT_CLASS_ID ) && obj->IsRenderable() )
                        renderable.insert( *node );
                }
            }
        }
    }

    // copy unique render nodes to the return vector.
    outRenderNodes.insert( outRenderNodes.end(), renderable.begin(), renderable.end() );
}

float get_face_corner_normal_and_angle( vector3f A, vector3f B, vector3f C, vector3f& Nout ) {
    vector3f U = C - B;
    vector3f V = B - A;

    Nout = vector3f::cross( V, U );

    float normalizationFactor = U.get_magnitude() * V.get_magnitude();
    if( normalizationFactor == 0 )
        normalizationFactor = 1;

    // Get the cosine of the angle
    float cosalpha = -vector3f::dot( U, V ) / normalizationFactor;
    // Just in case
    if( cosalpha > 1 )
        cosalpha = 1.0f;
    if( cosalpha < -1 )
        cosalpha = -1.0f;
    // Return the arc cosine
    return acos( cosalpha );
}

float get_mnmesh_face_corner_normal_and_angle( MNMesh* mesh, MNFace* mnface, int corner, vector3f& Nout ) {
    int cprev = ( corner + mnface->deg - 1 ) % mnface->deg;
    int cnext = ( corner + 1 ) % mnface->deg;
    float vertAngle = get_face_corner_normal_and_angle( from_max_t( mesh->V( mnface->vtx[cprev] )->p ),
                                                        from_max_t( mesh->V( mnface->vtx[corner] )->p ),
                                                        from_max_t( mesh->V( mnface->vtx[cnext] )->p ), Nout );
    bool oddOne = true;
    float smallerVertAngle = vertAngle;
    // Keep expanding the reach of this triangle we're getting the normal from, until
    // the angle is below 125 degrees
    while( smallerVertAngle > 125 * 3.1415926f / 180 ) {
        oddOne = !oddOne;
        if( oddOne )
            cnext = ( cnext + 1 ) % mnface->deg;
        else
            cprev = ( cprev + mnface->deg - 1 ) % mnface->deg;

        // If we reach all the way around, we have to stop and use this normal
        if( cprev == cnext )
            break;

        smallerVertAngle = get_face_corner_normal_and_angle( from_max_t( mesh->V( mnface->vtx[cprev] )->p ),
                                                             from_max_t( mesh->V( mnface->vtx[corner] )->p ),
                                                             from_max_t( mesh->V( mnface->vtx[cnext] )->p ), Nout );
    }
    // Return the original overall vert angle
    return vertAngle;
}

int get_convex_vertex_index( MNMesh* mesh, MNFace* mnface ) {
    int convexVertIndex = 0;
    Point3 convexVert = mesh->V( mnface->vtx[0] )->p;
    for( int corner = 1; corner < mnface->deg; ++corner ) {
        Point3 testVert = mesh->V( mnface->vtx[corner] )->p;
        if( testVert.x < convexVert.x ) {
            convexVertIndex = corner;
            convexVert = testVert;
        } else if( testVert.x == convexVert.x ) {
            if( testVert.y < convexVert.y ) {
                convexVertIndex = corner;
                convexVert = testVert;
            } else if( testVert.y == convexVert.y ) {
                if( testVert.z < convexVert.z ) {
                    convexVertIndex = corner;
                    convexVert = testVert;
                }
            }
        }
    }
    return convexVertIndex;
}

void get_mnmesh_smoothed_normals( MNMesh* mesh, std::vector<vector3f>& outNormals ) {
    if( !mesh )
        throw std::runtime_error( "ERROR: get_mnmesh_smoothed_normals() got a NULL mesh." );

    // Initialize the array to all zeros
    //	outNormals.resize(mesh->VNum());
    //	for( int i = 0; i < mesh->VNum(); ++i )
    //		outNormals[i] = vector3f();

    frantic::diagnostics::profiling_section p_init( _T("Init") ), p_mainLoop( _T("MainLoop") ),
        p_perFace( _T("PerFace") ), p_degThree( _T("DegThree") ), p_degHigher( _T("DegHigher") ),
        p_finalNormalize( _T("FinalNormalize") );

    p_init.enter();
    outNormals.assign( mesh->VNum(), vector3f() );
    p_init.exit();

    p_mainLoop.enter();
    int faceCount = mesh->FNum();
    for( int faceIndex = 0; faceIndex < faceCount; ++faceIndex ) {
        p_perFace.enter();
        MNFace* mnface = mesh->F( faceIndex );

        if( mnface->deg == 3 ) {
            p_degThree.enter();
            // Triangle case can be optimized
            vector3f normal;
            float vertexAngle = get_face_corner_normal_and_angle( from_max_t( mesh->V( mnface->vtx[0] )->p ),
                                                                  from_max_t( mesh->V( mnface->vtx[1] )->p ),
                                                                  from_max_t( mesh->V( mnface->vtx[2] )->p ), normal );

            normal *= vertexAngle;

            // Add to the weights and the weighted normal sum
            for( int corner = 0; corner < 3; ++corner ) {
                outNormals[mnface->vtx[corner]] += normal;
            }
            p_degThree.exit();
        } else {
            p_degHigher.enter();
            // First find a vertex that's guaranteed to be convex, and get its normal.  The normals of all the
            // corners should be point in roughly the same direction, so if a computed normal is pointing in the
            // opposite direction, the vertex is a concave one.
            int convexVertIndex = get_convex_vertex_index( mesh, mnface );
            vector3f convexVertNormal;
            get_mnmesh_face_corner_normal_and_angle( mesh, mnface, convexVertIndex, convexVertNormal );
            convexVertNormal.normalize();

            // Go through each vertex in the face, and compute the normal and angle to add to the contribution
            for( int corner = 0; corner < mnface->deg; ++corner ) {
                vector3f normal;
                float vertexAngle = get_mnmesh_face_corner_normal_and_angle( mesh, mnface, corner, normal );
                normal.normalize();

                // Check if the vertex is convex, and deal with it appropriately.
                float cosNormalAngle = vector3f::dot( convexVertNormal, normal );
                if( cosNormalAngle < 0 ) {
                    vertexAngle = (float)M_2PI - vertexAngle;
                    cosNormalAngle = -cosNormalAngle;
                }

                normal *= vertexAngle;

                outNormals[mnface->vtx[corner]] += normal;
            }
            p_degHigher.exit();
        }
        p_perFace.exit();
    }
    p_mainLoop.exit();

    p_finalNormalize.enter();
    // Normalize all the weighted normal sums we made.
    for( unsigned i = 0; i < outNormals.size(); ++i )
        outNormals[i].normalize();
    p_finalNormalize.exit();

    if( frantic::logging::is_logging_debug() ) {
        mprintf( _T("\nget_mnmesh_smoothed_normals timings:") );
        mprintf( _T("%s\n"), p_init.str().c_str() );
        mprintf( _T("%s\n"), p_mainLoop.str().c_str() );
        mprintf( _T("%s\n"), p_perFace.str().c_str() );
        mprintf( _T("%s\n"), p_degThree.str().c_str() );
        mprintf( _T("%s\n"), p_degHigher.str().c_str() );
        mprintf( _T("%s\n"), p_finalNormalize.str().c_str() );
    }
}

bool check_for_multiple_vertex_normals( Mesh& mesh ) {
    // go through each face in the scene
    for( int f = 0; f < mesh.numFaces; ++f ) {

        Face& face = mesh.faces[f];
        DWORD smGroup = face.getSmGroup();

        // check each of this face's vertices
        for( int i = 0; i < 3; ++i ) {
            RVertex& rvert( mesh.getRVert( face.v[i] ) );
            int normalCount = ( rvert.rFlags & NORCT_MASK );
            if( normalCount > 1 ) {

                // this vertex has multiple normals, but doesn't belong to a smoothing group
                if( smGroup == 0 )
                    return true;

                // check if there's a face, attached to this face, that doesn't share the same smoothing group
                for( int n = 0; n < normalCount; ++n ) {
                    if( !( rvert.ern[n].getSmGroup() & smGroup ) )
                        return true;
                }
            }
        }
    }

    // all faces that were attached to 1 or more other faces shared the same smoothing group
    return false;
}

bool check_for_differing_mapchannel_and_geom_verts( Mesh& mesh, int channel ) {
    TVFace* tvFaceArray = mesh.mapFaces( channel );
    assert( tvFaceArray );

    // go through each face and check if the verts of the geometry matches the verts of the map channel
    for( int f = 0; f < mesh.numFaces; ++f ) {

        Face& face = mesh.faces[f];
        TVFace& tvFace = tvFaceArray[f];

        for( int i = 0; i < 3; ++i ) {

            // check geom vert and map channel vert
            if( face.v[i] != tvFace.t[i] )
                return true;
        }
    }

    return false;
}

bool check_for_differing_mapchannel_and_geom_verts( MNMesh& mesh, int channel ) {
    using boost::lexical_cast;
    // go through each face and check if the verts of the geometry matches the verts of the map channel
    for( int fi = 0; fi < mesh.numf; ++fi ) {

        MNFace& face = mesh.f[fi];
        MNMapFace* mnMapFace = mesh.MF( channel, fi );
        FRANTIC_ASSERT_THROW( mnMapFace == NULL, "mnMapFace from channel " + lexical_cast<std::string>( channel ) +
                                                     " is null. idx: " + lexical_cast<std::string>( fi ) );
        FRANTIC_ASSERT_THROW( mnMapFace->deg == face.deg,
                              "mnMapFace from channel " + lexical_cast<std::string>( channel ) + " and face " +
                                  lexical_cast<std::string>( fi ) + " has " +
                                  lexical_cast<std::string>( mnMapFace->deg ) +
                                  " verts which is different from the number of verts than the mesh face which has " +
                                  lexical_cast<std::string>( face.deg ) );

        for( int vi = 0; vi < mnMapFace->deg; ++vi ) {

            // check geom vert and map channel vert
            if( face.vtx[vi] != mnMapFace->tv[vi] )
                return true;
        }
    }

    return false;
}

void build_mnmesh_normals( MNMesh& mesh, std::vector<std::map<DWORD, Point3>>& normals, bool& multiNormals,
                           bool& anySmoothing, bool alwaysExport ) {
    // It appears as if the header file defines this function, but it's not implemented
    anySmoothing = false;

    // set to true if there exists at least 1 vertex that has multiple normals of different smoothing groups
    multiNormals = false;

    // Warn about non-planar polygons only once
    bool warnedAboutNonPlanar = false;

    normals.clear();
    normals.resize( mesh.VNum() );
    // PASS 1: find all the smoothing groups that are there
    for( int f = 0; f < mesh.FNum(); ++f ) {
        MNFace* mnface = mesh.F( f );
        if( mnface->smGroup != 0 && mnface->deg >= 3 ) {
            for( int corner = 0; corner < mnface->deg; ++corner )
                normals[mnface->vtx[corner]][mnface->smGroup] = Point3( 0, 0, 0 );
        }
    }

    // PASS 2: add all the weighted normals together
    for( int f = 0; f < mesh.FNum(); ++f ) {

        MNFace* mnface = mesh.F( f );
        if( mnface->smGroup != 0 && mnface->deg >= 3 ) {

            // Optimize this when it's a triangle - the Normal N can't change
            if( mnface->deg == 3 ) {

                // Triangle case can be optimised
                // Weight by face area (won't work in concave case)
                // Point3 N = B ^ A;
                // Weight by angle
                vector3f N;
                float vertexAngle = get_face_corner_normal_and_angle( from_max_t( mesh.V( mnface->vtx[0] )->p ),
                                                                      from_max_t( mesh.V( mnface->vtx[1] )->p ),
                                                                      from_max_t( mesh.V( mnface->vtx[2] )->p ), N );

                N *= vertexAngle;

                for( int corner = 0; corner < 3; ++corner ) {

                    std::map<DWORD, Point3>& vertNormals = normals[mnface->vtx[corner]];

                    // check to see if there are multiple normals at this vertex of different smoothing groups
                    multiNormals = multiNormals || ( vertNormals.size() > 1 );

                    // Add this normal to any matching smoothing group normals
                    std::map<DWORD, Point3>::iterator iter;
                    for( iter = vertNormals.begin(); iter != vertNormals.end(); ++iter ) {
                        if( ( iter->first & mnface->smGroup ) != 0 ) {
                            vector3f currentN = from_max_t( iter->second );
                            iter->second = to_max_t( currentN + N );
                            // If we actually mixed two normals together, then smoothing has occurred
                            if( currentN.x != 0 || currentN.y != 0 || currentN.z != 0 )
                                anySmoothing = true;
                        }
                    }
                }
            } else {
                // First find a vertex that's guaranteed to be convex.
                int convexVertIndex = get_convex_vertex_index( &mesh, mnface );

                // Now get the normal of the convex vert, and then we
                // can assume that if the normal of a vertex is approximately
                // pointing in the same direction as the convex vert's normal,
                // that vert is also convex.
                vector3f convexVertNormal;
                get_mnmesh_face_corner_normal_and_angle( &mesh, mnface, convexVertIndex, convexVertNormal );
                convexVertNormal.normalize();

                for( int corner = 0; corner < mnface->deg; ++corner ) {

                    // Weight by angle
                    vector3f N;
                    float vertexAngle = get_mnmesh_face_corner_normal_and_angle( &mesh, mnface, corner, N );
                    N.normalize();

                    // If this is in a concave part, we have to invert the
                    // normal and adjust the vertex angle to the other arc.
                    float cosNormalAngle = vector3f::dot( convexVertNormal, N );
                    if( cosNormalAngle < 0 ) {
                        // TODO: This seems totally wrong, shouldn't it be 2PI - vertexAngle?
                        vertexAngle -= 2 * 3.1415926f;
                        cosNormalAngle = -cosNormalAngle;
                    }

                    // If the angle is more than 60 degrees
                    if( cosNormalAngle < 0.5f && !warnedAboutNonPlanar ) {

                        float angle = float( acos( cosNormalAngle ) ) * 180 / 3.1415926f;
                        // max3d::MsgBox( "WARNING: This object has a severely non-planar polygon (face " +
                        // boost::lexical_cast<std::string>(f+1) + ")\nThe normal angle difference is " +
                        // boost::lexical_cast<std::string>(angle) + "\nRecommendation: put a 'Turn to Poly' modifier on
                        // it, with Force Planar enabled", "Amaretto Warning" );
                        mprintf( _T("WARNING: The mesh has a severely non-planar polygon (face %d)\nThe normal angle ")
                                 _T("difference is %f\nRecommendation: put a 'Turn to Poly' modifier on it, with ")
                                 _T("Force Planar enabled\n"),
                                 f + 1, angle );
                        warnedAboutNonPlanar = true;
                    }

                    N *= vertexAngle;

                    std::map<DWORD, Point3>& vertNormals = normals[mnface->vtx[corner]];

                    // check to see if there are multiple normals at this vertex of different smoothing groups
                    multiNormals = multiNormals || ( vertNormals.size() > 1 );

                    // Add this normal to any matching smoothing group normals
                    std::map<DWORD, Point3>::iterator iter;
                    for( iter = vertNormals.begin(); iter != vertNormals.end(); ++iter ) {

                        if( ( iter->first & mnface->smGroup ) != 0 ) {
                            vector3f currentN = from_max_t( iter->second );
                            iter->second = to_max_t( currentN + N );
                            // If we actually mixed two normals together, then smoothing has occurred
                            if( currentN.x != 0 || currentN.y != 0 || currentN.z != 0 )
                                anySmoothing = true;
                        }
                    }
                }
            }
        }
    }

    if( anySmoothing || alwaysExport ) {
        // Normalize all the resulting normals
        for( unsigned v = 0; v < normals.size(); ++v ) {
            std::map<DWORD, Point3>& vertNormals = normals[v];
            std::map<DWORD, Point3>::iterator iter;
            for( iter = vertNormals.begin(); iter != vertNormals.end(); ++iter ) {
                iter->second = iter->second.Normalize();
            }
        }
    } else {
        // Signal that it's fully faceted by erasing all normal data
        normals.clear();
    }
}

bool is_vdata_crease_supported() {
#if MAX_VERSION_MAJOR < 17
    return false;
#elif MAX_VERSION_MAJOR == 17
    const std::wstring version = UtilityInterface::GetCurrentVersion();
    return boost::starts_with( version, L"18." );
#else
    return true;
#endif
}

int get_vdata_crease_channel() {
    if( is_vdata_crease_supported() ) {
#if MAX_VERSION_MAJOR < 17
        throw std::runtime_error( "get_vdata_crease_channel Internal Error: "
                                  "channel is not supported in this version of 3ds Max" );
#elif MAX_VERSION_MAJOR == 17
        // constant copied from VDATA_CREASE in
        // 3ds Max 2016 SDK's mesh.hpp
        return 4;
#else
        return VDATA_CREASE;
#endif
    } else {
        throw std::runtime_error( "get_vdata_crease_channel Error: "
                                  "channel is not supported in this version of 3ds Max" );
    }
}

frantic::tstring get_map_channel_name( int mapChannel ) {
    if( mapChannel == 0 ) {
        return _T("Color");
    } else if( mapChannel == 1 ) {
        return _T("TextureCoord");
    } else if( mapChannel > 1 && mapChannel <= 99 ) {
        return _T("Mapping") + boost::lexical_cast<frantic::tstring>( mapChannel );
    } else {
        throw std::runtime_error( "get_map_channel_name Error: unknown map channel number: " +
                                  boost::lexical_cast<std::string>( mapChannel ) );
    }
}

} // namespace geometry
} // namespace max3d
} // namespace frantic
