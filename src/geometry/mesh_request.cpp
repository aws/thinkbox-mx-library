// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/max3d/geometry/auto_mesh.hpp>
#include <frantic/max3d/geometry/mesh.hpp>
#include <frantic/max3d/geometry/mesh_request.hpp>
#include <frantic/max3d/geometry/null_view.hpp>
#include <frantic/max3d/geometry/polymesh.hpp>

#include <frantic/files/filename_sequence.hpp>
#include <frantic/files/files.hpp>
#include <frantic/geometry/trimesh3_file_io.hpp>
#include <frantic/geometry/trimesh3_utils.hpp>
#include <frantic/geometry/xmesh_sequence_saver.hpp>
#include <frantic/logging/logging_level.hpp>
#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/parameter_extraction.hpp>
#include <frantic/max3d/rendering/renderplugin_utils.hpp>

using namespace frantic::geometry;
using namespace frantic::graphics;
using namespace frantic::max3d;
using namespace frantic::max3d::rendering;

namespace frantic {
namespace max3d {
namespace geometry {

void get_node_trimesh3( INode* meshNode, TimeValue startTime, TimeValue endTime, frantic::geometry::trimesh3& outMesh,
                        max_interval_t& outValidityInterval, float timeStepScale, bool ignoreEmptyMeshes,
                        bool ignoreTopologyWarnings, bool useObjectSpace,
                        const frantic::channels::channel_propagation_policy& cpp ) {

    if( !( timeStepScale > 0.f && timeStepScale < 1.f ) )
        throw std::runtime_error( std::string() +
                                  "get_node_trimesh3() - The provided scale factor, when getting mesh for node \"" +
                                  frantic::strings::to_string( meshNode->GetName() ) + "\", for the time step (" +
                                  boost::lexical_cast<std::string>( timeStepScale ) + ") must be between 0 and 1." );

    // This used to be "t_next = endTime-1", but that caused problems when the situation (startTime+1 == endTime)
    // occurred
    int t_next = endTime;
    float timeStepInSecs = float( t_next - startTime ) / 4800.0f;

    max_interval_t firstXfrmValidity, secondXfrmValidity;
    transform4f firstXfrm, secondXfrm;
    Matrix3 firstMaxXfrm, secondMaxXfrm;
    Interval xfrmValidity;

    if( useObjectSpace ) {
        // if we're using object space, there will be no node tranformation, so set it to the identity
        // and make it's interval forever so that the second transform doesn't need to be fetched
        firstXfrm.set_to_identity();
        xfrmValidity = FOREVER;
    } else {
        Matrix3 firstMaxXfrm = meshNode->GetObjTMAfterWSM( startTime, &xfrmValidity );
        firstXfrm = transform4f( firstMaxXfrm );
    }
    firstXfrmValidity.first = xfrmValidity.Start();
    firstXfrmValidity.second = xfrmValidity.End();

    bool changingTransform = false;
    if( firstXfrmValidity.second < t_next ) {

        Matrix3 secondMaxXfrm = meshNode->GetObjTMAfterWSM( t_next, &xfrmValidity );
        secondXfrm = transform4f( secondMaxXfrm );
        secondXfrmValidity.first = xfrmValidity.Start();
        secondXfrmValidity.second = xfrmValidity.End();

        changingTransform = true;
    } else {
        secondXfrm = firstXfrm;
    }

    // Get the mesh from the inode
    ObjectState obj = meshNode->EvalWorldState( startTime );
    Interval firstMeshValidity = obj.Validity( startTime );
    null_view view;

    AutoMesh firstMesh = get_mesh_from_inode( meshNode, startTime, view );

    if( firstMesh->getNumVerts() == 0 ) {
        if( ignoreEmptyMeshes ) {
            outMesh.clear();
            return;
        } else
            throw std::runtime_error( std::string() + "get_node_trimesh3() - The sampled mesh for node \"" +
                                      frantic::strings::to_string( meshNode->GetName() ) +
                                      "\" doesn't have any vertices" );
    }

    if( IParticleObjectExt* pParticleObj = GetParticleObjectExtInterface( obj.obj ) ) {
        // TODO: Add support for multiple render meshes. This is likely to be supported by particle systems.

        Tab<Point3> vertexVelocity;

        // If we can get at the (world-space) velocity of the mesh through IParticleObjectExt, do so.
        if( pParticleObj->GetRenderMeshVertexSpeed( startTime, meshNode, view, vertexVelocity ) ) {
            // Copy the mesh at the start time.
            // It appears that such objects are not already in worldspace, so they are
            // affected by node transforms.
            // Thinking Particles definitely are.
            // Particle Flow groups are too, but they are usually at the origin.
            // For now I will apply the world transform until we find a counter-example.
            // (However the Velocity is already in world space.)
            mesh_copy( outMesh, firstXfrm, *firstMesh.get(), cpp );

            if( cpp.is_channel_included( _T("Velocity") ) ) {
                outMesh.add_vertex_channel<vector3f>( _T("Velocity") );

                trimesh3_vertex_channel_accessor<vector3f> velocityChannel =
                    outMesh.get_vertex_channel_accessor<vector3f>( _T("Velocity") );
                for( unsigned i = 0; i < velocityChannel.size(); ++i )
                    velocityChannel[i] = (float)TIME_TICKSPERSEC * frantic::max3d::from_max_t( vertexVelocity[i] );
            }

            // the resulting mesh is only valid for this instant
            outValidityInterval.first = startTime;
            outValidityInterval.second = startTime;

            return;
        }
    }

    if( firstMeshValidity.InInterval( t_next ) ) {
        // do a simple one mesh generation
        if( changingTransform ) {
            mesh_copy( outMesh, firstXfrm, secondXfrm, *firstMesh.get(), timeStepInSecs, cpp );
            // the resulting mesh is only valid for this instant
            outValidityInterval.first = startTime;
            outValidityInterval.second = startTime;
        }
        // the object isn't moving and so it has no velocities
        else {
            mesh_copy( outMesh, firstXfrm, *firstMesh.get(), cpp );
            // we need to take the intersection of the xfrm validity and the mesh validity intervals
            outValidityInterval.first = ( firstMeshValidity.Start() > firstXfrmValidity.first )
                                            ? firstMeshValidity.Start()
                                            : firstXfrmValidity.first;
            outValidityInterval.second = ( firstMeshValidity.End() < firstXfrmValidity.second )
                                             ? firstMeshValidity.End()
                                             : firstXfrmValidity.second;
        }
    } else {
        // The mesh changed.
        // Check for consistent mesh topology and scale back the interval until you find a consistent mesh.

        if( firstMesh->getNumVerts() == 0 ) {
            if( ignoreEmptyMeshes ) {
                outMesh.clear();
                return;
            } else
                throw std::runtime_error( std::string() + "get_node_trimesh3() - The sampled mesh for node \"" +
                                          frantic::strings::to_string( meshNode->GetName() ) +
                                          "\" doesn't have any vertices" );
        }

        // Make a copy of the first mesh, because it'll get invalidated.
        Mesh firstMeshCopy = *firstMesh.get();
        firstMesh = AutoMesh();

        // Why is this being checked twice?  Was there some mysterious dropping of this mesh after the copy
        // was made?
        if( firstMeshCopy.getNumVerts() == 0 ) {
            if( ignoreEmptyMeshes ) {
                outMesh.clear();
                return;
            } else
                throw std::runtime_error( std::string() + "get_node_trimesh3() - The sampled mesh for node \"" +
                                          frantic::strings::to_string( meshNode->GetName() ) +
                                          "\" doesn't have any vertices" );
        }

        if( changingTransform ) {
            secondXfrm = transform4f( meshNode->GetObjTMAfterWSM( t_next, &xfrmValidity ) );
        }

        AutoMesh secondMesh = get_mesh_from_inode( meshNode, t_next, view );

        std::size_t secondMeshSampleCount = 1;

        while( !equal_topology( firstMeshCopy, *secondMesh.get() ) &&
               t_next !=
                   startTime ) { // this last bit is to make sure we stop if we cant find a mesh of consistent topology

            timeStepInSecs *= timeStepScale;
            t_next = startTime + (int)( 4800.0f * timeStepInSecs );

            if( changingTransform ) {
                secondXfrm = transform4f( meshNode->GetObjTMAfterWSM( t_next, &xfrmValidity ) );
            }
            secondMesh = get_mesh_from_inode( meshNode, t_next, view );
            ++secondMeshSampleCount;
        }
        // If the next step time is equal to the start time, then no mesh could be found on a
        // subsequent tick with consistent topology.
        if( t_next == startTime ) {
            // We'll go backwards a tick instead, find an appropriate mesh, and then reverse
            // the velocities.
            t_next = startTime - 1;
            timeStepInSecs = 1.0f / 4800.0f;
            if( changingTransform ) {
                secondXfrm = transform4f( meshNode->GetObjTMAfterWSM( t_next, &xfrmValidity ) );
            }
            secondMesh = get_mesh_from_inode( meshNode, t_next, view );
            ++secondMeshSampleCount;
            if( !equal_topology( firstMeshCopy, *secondMesh.get() ) ) {

                // if we're ignoring topological issues, just copy the mesh with no velocity and return
                if( ignoreTopologyWarnings ) {
                    mesh_copy( outMesh, firstXfrm, firstMeshCopy, cpp );
                    outValidityInterval.first = startTime;
                    outValidityInterval.second = startTime;
                    return;
                }

                // ok, now you really are screwed because this mesh has this topology for only one tick.
                std::stringstream ss;
                ss << "max3d::get_node_trimesh3() - Could not find a mesh for node "
                   << frantic::strings::to_string( meshNode->GetName() ) << " with consistent topology.\n";
                ss << "\t at time " << startTime << "\n";
                throw std::runtime_error( ss.str() );
            }

            // do a backwards mesh copy to get the reverse velocities
            mesh_copy( outMesh, secondXfrm, firstXfrm, *secondMesh.get(), firstMeshCopy, timeStepInSecs, cpp );
        } else {
            // do a forwards mesh copy to get the vertex velocities
            mesh_copy( outMesh, firstXfrm, secondXfrm, firstMeshCopy, *secondMesh.get(), timeStepInSecs, cpp );
        }
        // the resulting mesh is only valid for this instant
        outValidityInterval.first = startTime;
        outValidityInterval.second = startTime;
        FF_LOG( debug ) << "Found velocity in " << boost::lexical_cast<frantic::tstring>( secondMeshSampleCount )
                        << ( secondMeshSampleCount == 1 ? " step" : " steps" ) << "." << std::endl;
    }
}

namespace {

class auto_mnmesh {
  public:
    auto_mnmesh()
        : m_result( 0 ) {}

    auto_mnmesh( INode* inode, TimeValue time, View& view )
        : m_result( 0 ) {
        reset( inode, time, view );
    }

    void reset() {
        m_result = 0;
        m_polyobj = AutoPolyObject();
        m_polymesh.ClearAndFree();
    }

    void reset( INode* inode, TimeValue t, View& view ) {
        reset();

        if( !inode ) {
            throw std::runtime_error( "auto_mnmesh Error: INode passed to function is NULL" );
        }

        ObjectState os = inode->EvalWorldState( t );

        if( os.obj->CanConvertToType( polyObjectClassID ) ) {
            PolyObject* p = (PolyObject*)os.obj->ConvertToType( t, polyObjectClassID );
            bool needsDelete = ( p != os.obj );
            if( p ) {
                m_polyobj = AutoPolyObject( p, needsDelete );
                m_result = &m_polyobj->GetMesh();
            } else {
                throw( "auto_mnmesh Error: INode \"" + frantic::strings::to_string( inode->GetName() ) +
                       "\" returned null object" );
            }
        } else {
            AutoMesh m = get_mesh_from_inode( inode, t, view );
            m_polymesh.SetFromTri( *m );
            make_polymesh( m_polymesh );
            m_result = &m_polymesh;
        }
    }

    MNMesh* get() { return m_result; }

    const MNMesh* get() const { return m_result; }

    MNMesh* operator->() { return m_result; }

    const MNMesh* operator->() const { return m_result; }

    MNMesh& operator*() {
        assert( m_result );
        return *m_result;
    }

    const MNMesh& operator*() const {
        assert( m_result );
        return *m_result;
    }

  private:
    MNMesh* m_result;
    AutoPolyObject m_polyobj;
    MNMesh m_polymesh;
};

} // anonymous namespace

polymesh3_ptr get_node_polymesh3( INode* meshNode, TimeValue startTime, TimeValue endTime,
                                  max_interval_t& outValidityInterval, float timeStepScale, bool ignoreEmptyMeshes,
                                  bool ignoreTopologyWarnings, bool useObjectSpace,
                                  const frantic::channels::channel_propagation_policy& cpp ) {
    if( timeStepScale < 0.f || timeStepScale > 1.f ) {
        throw std::runtime_error( std::string() +
                                  "get_node_polymesh3() - The provided scale factor, when getting mesh for node \"" +
                                  frantic::strings::to_string( meshNode->GetName() ) + "\", for the time step (" +
                                  boost::lexical_cast<std::string>( timeStepScale ) + ") must be between 0 and 1." );
    }

    // This used to be "t_next = endTime-1", but that caused problems when the situation (startTime+1 == endTime)
    // occurred
    TimeValue t_next = endTime;
    float timeStepInSecs = float( t_next - startTime ) / 4800.0f;

    max_interval_t firstXfrmValidity;
    transform4f firstXfrm, secondXfrm;
    Interval xfrmValidity;

    if( useObjectSpace ) {
        // if we're using object space, there will be no node tranformation, so set it to the identity
        // and make it's interval forever so that the second transform doesn't need to be fetched
        firstXfrm.set_to_identity();
        xfrmValidity = FOREVER;
    } else {
        Matrix3 firstMaxXfrm = meshNode->GetObjTMAfterWSM( startTime, &xfrmValidity );
        firstXfrm = transform4f( firstMaxXfrm );
    }
    firstXfrmValidity.first = xfrmValidity.Start();
    firstXfrmValidity.second = xfrmValidity.End();

    bool changingTransform = false;
    if( firstXfrmValidity.second < t_next ) {
        Matrix3 secondMaxXfrm = meshNode->GetObjTMAfterWSM( t_next, &xfrmValidity );
        secondXfrm = transform4f( secondMaxXfrm );

        changingTransform = true;
    } else {
        secondXfrm = firstXfrm;
    }

    // Get the mesh from the inode
    ObjectState obj = meshNode->EvalWorldState( startTime );
    Interval firstMeshValidity = obj.Validity( startTime );
    null_view view;

    polymesh3_ptr outPtr;

    if( IParticleObjectExt* pParticleObj = GetParticleObjectExtInterface( obj.obj ) ) {
        // TODO: Add support for multiple render meshes. This is likely to be supported by particle systems.
        AutoMesh firstMesh = get_mesh_from_inode( meshNode, startTime, view );

        if( firstMesh->getNumVerts() == 0 ) {
            if( ignoreEmptyMeshes ) {
                outValidityInterval.first = startTime;
                outValidityInterval.second = startTime;
                polymesh3_ptr outPtr = polymesh3_builder().finalize();
                return outPtr;
            } else {
                throw std::runtime_error( std::string( "get_node_polymesh3() - The sampled mesh for node \"" ) +
                                          frantic::strings::to_string( meshNode->GetName() ) +
                                          "\" doesn't have any vertices" );
            }
        }

        Tab<Point3> vertexVelocity;

        // If we can get at the (world-space) velocity of the mesh through IParticleObjectExt, do so.
        if( pParticleObj->GetRenderMeshVertexSpeed( startTime, meshNode, view, vertexVelocity ) ) {
            outPtr = frantic::max3d::geometry::polymesh_copy( *firstMesh, firstXfrm, vertexVelocity, cpp );

            // the resulting mesh is only valid for this instant
            outValidityInterval.first = startTime;
            outValidityInterval.second = startTime;
            return outPtr;
        }
    }

    auto_mnmesh firstMesh( meshNode, startTime, view );

    if( firstMesh->VNum() == 0 ) {
        if( ignoreEmptyMeshes ) {
            outValidityInterval.first = startTime;
            outValidityInterval.second = startTime;
            polymesh3_ptr outPtr = polymesh3_builder().finalize();
            return outPtr;
        } else {
            throw std::runtime_error( std::string( "get_node_polymesh3() - The sampled mesh for node \"" ) +
                                      frantic::strings::to_string( meshNode->GetName() ) +
                                      "\" doesn't have any vertices after conversion to polymesh" );
        }
    }

    if( firstMeshValidity.InInterval( t_next ) ) {
        if( changingTransform ) {
            outPtr = polymesh_copy( *firstMesh, firstXfrm, secondXfrm, cpp, timeStepInSecs );

            // the resulting mesh is only valid for this instant
            outValidityInterval.first = startTime;
            outValidityInterval.second = startTime;
        } else {
            outPtr = frantic::max3d::geometry::from_max_t( *firstMesh, cpp );
            transform( outPtr, firstXfrm );

            // we need to take the intersection of the xfrm validity and the mesh validity intervals
            outValidityInterval.first = ( firstMeshValidity.Start() > firstXfrmValidity.first )
                                            ? firstMeshValidity.Start()
                                            : firstXfrmValidity.first;
            outValidityInterval.second = ( firstMeshValidity.End() < firstXfrmValidity.second )
                                             ? firstMeshValidity.End()
                                             : firstXfrmValidity.second;
        }
    } else {
        // Make a copy of the first mesh, because it'll get invalidated.
        MNMesh firstMeshCopy = *firstMesh;
        firstMesh.reset();

        // Why is this being checked twice?  Was there some mysterious dropping of this mesh after the copy
        // was made?
        if( firstMeshCopy.VNum() == 0 ) {
            if( ignoreEmptyMeshes ) {
                outValidityInterval.first = startTime;
                outValidityInterval.second = startTime;
                polymesh3_ptr outPtr = polymesh3_builder().finalize();
                return outPtr;
            } else {
                throw std::runtime_error( std::string( "get_node_polymesh3() - The sampled mesh for node \"" ) +
                                          frantic::strings::to_string( meshNode->GetName() ) +
                                          "\" doesn't have any vertices" );
            }
        }

        if( changingTransform ) {
            secondXfrm = transform4f( meshNode->GetObjTMAfterWSM( t_next, &xfrmValidity ) );
        }

        auto_mnmesh secondMesh( meshNode, t_next, view );

        std::size_t secondMeshSampleCount = 1;

        while( !equal_topology( firstMeshCopy, *secondMesh ) &&
               t_next !=
                   startTime ) { // this last bit is to make sure we stop if we cant find a mesh of consistent topology
            timeStepInSecs *= timeStepScale;
            t_next = startTime + (int)( 4800.0f * timeStepInSecs );

            if( changingTransform ) {
                secondXfrm = transform4f( meshNode->GetObjTMAfterWSM( t_next, &xfrmValidity ) );
            }
            secondMesh.reset( meshNode, t_next, view );
            ++secondMeshSampleCount;
        }

        // If the next step time is equal to the start time, then no mesh could be found on a
        // subsequent tick with consistent topology.
        if( t_next == startTime ) {
            // We'll go backwards a tick instead, find an appropriate mesh, and then reverse
            // the velocities.
            t_next = startTime - 1;
            timeStepInSecs = 1.0f / 4800.0f;
            if( changingTransform ) {
                secondXfrm = transform4f( meshNode->GetObjTMAfterWSM( t_next, &xfrmValidity ) );
            }
            secondMesh.reset( meshNode, t_next, view );
            ++secondMeshSampleCount;
            if( !equal_topology( firstMeshCopy, *secondMesh ) ) {
                if( !ignoreTopologyWarnings ) {
                    // ok, now you really are screwed because this mesh has this topology for only one tick.
                    std::stringstream ss;
                    ss << "max3d::get_node_polymesh3() - Could not find a mesh for node "
                       << frantic::strings::to_string( meshNode->GetName() ) << " with consistent topology.\n";
                    ss << "\t at time " << startTime << "\n";
                    throw std::runtime_error( ss.str() );
                } else {
                    // if we're ignoring topological issues, just copy the mesh with no velocity and return
                    outPtr = frantic::max3d::geometry::from_max_t( firstMeshCopy, cpp );
                    transform( outPtr, firstXfrm );
                }
            } else {
                // do a backwards mesh copy to get the reverse velocities
                outPtr = polymesh_copy( *secondMesh, firstMeshCopy, secondXfrm, firstXfrm, cpp, timeStepInSecs );
            }
        } else {
            // do a forwards mesh copy to get the vertex velocities
            outPtr = polymesh_copy( firstMeshCopy, *secondMesh, firstXfrm, secondXfrm, cpp, timeStepInSecs );
        }
        // the resulting mesh is only valid for this instant
        outValidityInterval.first = startTime;
        outValidityInterval.second = startTime;
        FF_LOG( debug ) << "Found velocity in " << boost::lexical_cast<frantic::tstring>( secondMeshSampleCount )
                        << ( secondMeshSampleCount == 1 ? " step" : " steps" ) << "." << std::endl;
    }

    if( !outPtr ) {
        throw std::runtime_error( "get_node_polymesh3() - No valid pointer obtained for given mesh" );
    }

    return outPtr;
}

void get_trimeshes_for_max_displacement( const std::vector<INode*>& meshNodes, TimeValue t, float maxDisplacement,
                                         float& frameOffset, std::vector<frantic::geometry::trimesh3>& outTrimeshes,
                                         float timeStepScale, bool ignoreEmptyMeshes, bool ignoreTopologyWarnings ) {

    if( !( timeStepScale > 0.f && timeStepScale < 1.f ) )
        throw std::runtime_error( "get_node_trimesh3() - The provided scale factor for the time step (" +
                                  boost::lexical_cast<std::string>( timeStepScale ) + ") must be between 0 and 1." );

    // TODO: Use validity intervals to determine which meshes need to be refreshed
    int ticksPerFrame = GetTicksPerFrame();
    float framesPerSecond = (float)( 4800.0f / ticksPerFrame );
    float timeStep = 1.0f - frameOffset; // starting fraction of a frame to try for a time step

    max_interval_t meshInterval;

    // TODO:  Remove this, it should be handled elsewhere.  this is a function for fetching meshes
    // given a max displacement, not in the absence of one.
    if( maxDisplacement < 0.f ) {

        // Grab new meshes and step the rest of the whole frame
        for( unsigned i = 0; i < meshNodes.size(); i++ ) {
            timeStep = 1.0f - frameOffset; // set the timeStep to go the rest of the frame
            get_node_trimesh3( meshNodes[i], t + int( frameOffset * ticksPerFrame ),
                               t + int( ( frameOffset + timeStep ) * ticksPerFrame ), outTrimeshes[i], meshInterval,
                               timeStepScale, ignoreEmptyMeshes, ignoreTopologyWarnings );
        }
    } else {

        // Grab meshes and check the velocity.  Scale back the time step by a half any time the
        // mesh velocity will move the mesh more than a voxel over the time step.
        timeStep = 1.f - frameOffset; // set the timeStep to go the rest of the frame if possible
        bool done = false;

        do {
            // Grab Meshes
            for( unsigned i = 0; i < meshNodes.size(); i++ ) {
                // get the mesh for the node
                get_node_trimesh3( meshNodes[i], t + int( frameOffset * ticksPerFrame ),
                                   t + int( ( frameOffset + timeStep ) * ticksPerFrame ), outTrimeshes[i], meshInterval,
                                   timeStepScale, ignoreEmptyMeshes, ignoreTopologyWarnings );
            }

            // Find the largest velocity across all the meshes
            std::vector<float> maxVelocityMagnitudes( outTrimeshes.size(), 0.0f );
            float maxVelocityMagnitude = 0.0f;
            for( unsigned i = 0; i < outTrimeshes.size(); i++ ) {
                maxVelocityMagnitudes[i] = get_mesh_max_velocity_magnitude( outTrimeshes[i] );
                if( maxVelocityMagnitudes[i] > maxVelocityMagnitude )
                    maxVelocityMagnitude = maxVelocityMagnitudes[i];
            }
            // If that velocity causes you to skip over a voxel, scale the timeStep back by the timeStepScale,
            // grab new meshes, and try again.
            if( maxVelocityMagnitude * timeStep / framesPerSecond > maxDisplacement ) {
                timeStep *= timeStepScale;
            } else {
                done = true;
            }
        } while( !done );
    }
    frameOffset += timeStep;
}

void get_trimeshes_for_max_displacement( const std::vector<INode*>& meshNodes, TimeValue tStart, TimeValue tEnd,
                                         float maxDisplacement, float timeStepScale, bool ignoreEmptyMeshes,
                                         bool ignoreTopologyWarnings,
                                         std::vector<frantic::geometry::trimesh3>& outTrimeshes, TimeValue& tOut ) {

    if( !( timeStepScale > 0.f && timeStepScale < 1.f ) )
        throw std::runtime_error( "get_node_trimesh3() - The provided scale factor for the time step (" +
                                  boost::lexical_cast<std::string>( timeStepScale ) + ") must be between 0 and 1." );

    // TODO: Use validity intervals to determine which meshes need to be refreshed
    max_interval_t meshInterval;

    // TODO:  Remove this, it should be handled elsewhere.  this is a function for fetching meshes
    // given a max displacement, not in the absence of one.
    if( maxDisplacement < 0.f ) {

        // Grab new meshes and step the rest of the whole frame
        for( unsigned i = 0; i < meshNodes.size(); i++ ) {
            get_node_trimesh3( meshNodes[i], tStart, tEnd, outTrimeshes[i], meshInterval, timeStepScale,
                               ignoreEmptyMeshes, ignoreTopologyWarnings );
        }
    } else {

        // Grab meshes and check the velocity.  Scale back the time step by a half any time the
        // mesh velocity will move the mesh more than a voxel over the time step.
        bool done = false;

        do {
            // Grab Meshes
            for( unsigned i = 0; i < meshNodes.size(); i++ ) {
                // get the mesh for the node
                get_node_trimesh3( meshNodes[i], tStart, tEnd, outTrimeshes[i], meshInterval, timeStepScale,
                                   ignoreEmptyMeshes, ignoreTopologyWarnings );
            }

            // Find the largest velocity across all the meshes
            std::vector<float> maxVelocityMagnitudes( outTrimeshes.size(), 0.0f );
            float maxVelocityMagnitude = 0.0f;
            for( unsigned i = 0; i < outTrimeshes.size(); i++ ) {
                maxVelocityMagnitudes[i] = get_mesh_max_velocity_magnitude( outTrimeshes[i] );
                if( maxVelocityMagnitudes[i] > maxVelocityMagnitude )
                    maxVelocityMagnitude = maxVelocityMagnitudes[i];
            }
            // If that velocity causes you to skip over a voxel, scale the timeStep back by the timeStepScale,
            // grab new meshes, and try again.
            int tDelta = tEnd - tStart;
            if( maxVelocityMagnitude * tDelta / 4800.f > maxDisplacement )
                tEnd = tStart + int( tDelta * timeStepScale );
            else
                done = true;
        } while( !done );
    }
    tOut = tEnd;
}

// TODO:  another similar function that uses the topology channel to check validity intervals of
//		  consistent topology would also be cool.  however, that would require that every object that
//		  you try to cache has the channel-wise validity correctly supported, and that's probably not
//		  an entirely safe assumption...
void cache_node_trimeshes_in_interval( INode* meshNode, TimeValue startTime, TimeValue endTime, int numSamples,
                                       int numRetries, frantic::geometry::xmesh_sequence_saver& xss,
                                       const frantic::files::filename_sequence& fsq,
                                       const std::map<std::string, bool>& options ) {

    double frame = startTime / double( GetTicksPerFrame() );
    frantic::tstring ext = frantic::files::extension_from_path( fsq[frame] );

    std::map<std::string, bool>::const_iterator iter;

    bool saveVelocity = false;
    iter = options.find( "SaveVelocity" );
    if( iter != options.end() ) {
        saveVelocity = iter->second;
    }

    bool ignoreEmpty = false;
    iter = options.find( "IgnoreEmpty" );
    if( iter != options.end() ) {
        ignoreEmpty = iter->second;
    }

    bool ignoreTopology = false;
    iter = options.find( "IgnoreTopology" );
    if( iter != options.end() ) {
        ignoreTopology = iter->second;
    }

    bool useObjectSpace = false;
    iter = options.find( "UseObjectSpace" );
    if( iter != options.end() ) {
        useObjectSpace = iter->second;
    }

    // Divide the interval equally into sample times
    std::vector<int> sampleTimes;
    int intervalLength = endTime - startTime;

    if( intervalLength < 0 )
        throw std::runtime_error( "cache_node_trimeshes_in_interval() - The given interval is invalid (" +
                                  boost::lexical_cast<std::string>( startTime ) + "," +
                                  boost::lexical_cast<std::string>( endTime ) + ")" );

    float stepSize = intervalLength / float( numSamples );
    for( int i = 0; i <= numSamples; ++i ) {
        sampleTimes.push_back( startTime + int( i * stepSize ) );
    }

    // For each sample time, try and extract and cache a mesh
    max_interval_t outValidityInterval;
    for( int i = 0; i < numSamples; ++i ) {

        frantic::tstring outFile = fsq[double( startTime ) / GetTicksPerFrame()];

        int sampleStart = sampleTimes[i];
        int sampleEnd = sampleTimes[i + 1];

        trimesh3 outMesh;

        int retries = 0;

        // mprintf("caching mesh for sample time %d (with end at %d)\n", sampleStart, sampleEnd);

        // If we dont want to save velocity, just set the start and end times to be equal and grab a mesh
        if( !saveVelocity ) {
            get_node_trimesh3( meshNode, sampleStart, sampleEnd, outMesh, outValidityInterval, 0.5f, ignoreEmpty, true,
                               useObjectSpace );
        }
        // Otherwise, we have to do some fancier sampling
        else {
            get_node_trimesh3( meshNode, sampleStart, sampleEnd, outMesh, outValidityInterval, 0.5f, ignoreEmpty, true,
                               useObjectSpace );

            // If the validitiy interval is instantaneous, there was a topology problem, and we need to try for
            // a sample elsewhere, but still close.
            int step = 1, newStartSample = sampleStart;
            while( !ignoreTopology && outValidityInterval.first == outValidityInterval.second &&
                   retries < numRetries ) {
                newStartSample = sampleStart + step * int( intervalLength / 4 );
                get_node_trimesh3( meshNode, sampleStart, newStartSample + intervalLength / numSamples, outMesh,
                                   outValidityInterval, 0.5f, ignoreEmpty, true, useObjectSpace );
                ++retries;
                ( step > 0 )
                    ? step = -step
                    : step = -step + 1; // i could do this differently, but i'm trying to avoid accumulation errors
            }
            sampleStart = newStartSample;
        }

        // Only save a sample if we didnt run out of retries before we found a good sample
        if( retries != numRetries ) {
            frame = sampleStart / double( GetTicksPerFrame() );
            if( ext == _T(".obj") )
                write_obj_mesh_file( fsq[frame], outMesh );
            else if( ext == _T(".xmesh") )
                xss.write_xmesh( outMesh, fsq[frame] );
            else
                throw std::runtime_error( "cache_node_trimeshes_in_interval() - Unrecognized extension: " +
                                          frantic::strings::to_string( ext ) + "\n" );
        }
        sampleStart = sampleEnd;
    }
}

} // namespace geometry
} // namespace max3d
} // namespace frantic
