// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/channels/channel_propagation_policy.hpp>
#include <frantic/files/filename_sequence.hpp>
#include <frantic/geometry/polymesh3.hpp>
#include <frantic/geometry/trimesh3.hpp>
#include <frantic/geometry/xmesh_sequence_saver.hpp>

namespace frantic {
namespace max3d {
/**
 * Standard interval pair which can be used in frantic library in place of the 3dsMax defined Interval class
 */
typedef std::pair<int, int> max_interval_t;

namespace geometry {

/**
 * This function takes a meshNode, a start time/end time, and returns a trimesh with velocities at each vertex
 * computed by taking the difference in positions of the mesh vertices at start and end time.  The mesh returned
 * is in world space.
 *
 * If mesh topology changes over the interval given, the interval is scaled back by a factor of timeStepScale
 * until the topology is consistent, throwing an exception if no consistent topology mesh can be found.
 * If the ignore topology flag is true, then instead the mesh is assigned a velocity of 0 at all verts
 * when inconsistent topology is encountered.
 *
 * @param  meshNode		the node for which the time step should be determined
 * @param  startTime	the startTime of the interval at which to compute the velocity
 * @param  endTime		the endTime of the interval at which to compute the velocity
 * @param  outMesh		the output trimesh with velocity channel
 * @param  outValidity	the validity of the returned trimesh
 * @param  timeStepScale    the factor to scale back the interval by when the mesh topology is inconsistent, defaulted
 * to 0.5f
 * @param  ignoreEmptyMeshes	ignore when the mesh is empty, otherwise throw an exception
 * @param  ignoreTopologyWarnings	ignore inconsistent topology and return a mesh with no velocities if the case
 * arises
 * @param  useObjectSpace			use object space transforms rather than worldspace transforms to calculate
 * velocity, defaults to false
 * @param  cpp		specify the channels to copy, defaults to copy all channels
 */
void get_node_trimesh3(
    INode* meshNode, TimeValue startTime, TimeValue endTime, frantic::geometry::trimesh3& outMesh,
    max_interval_t& outValidityInterval, float timeStepScale = 0.5f, bool ignoreEmptyMeshes = false,
    bool ignoreTopologyWarnings = false, bool useObjectSpace = false,
    const frantic::channels::channel_propagation_policy& cpp = frantic::channels::channel_propagation_policy() );

/**
 * This function is similar to get_node_trimesh3, but returns a pointer to a polymesh3 instead.
 *
 * @param  meshNode		the node for which the time step should be determined
 * @param  startTime	the startTime of the interval at which to compute the velocity
 * @param  endTime		the endTime of the interval at which to compute the velocity
 * @param  outValidity	the validity of the returned trimesh
 * @param  timeStepScale    the factor to scale back the interval by when the mesh topology is inconsistent
 * @param  ignoreEmptyMeshes	ignore when the mesh is empty, otherwise throw an exception
 * @param  ignoreTopologyWarnings	ignore inconsistent topology and return a mesh with no velocities if the case
 * arises
 * @param  useObjectSpace			use object space transforms rather than worldspace transforms to calculate
 * velocity
 * @param  cpp		specify the channels to copy
 *
 * @return a pointer to the output polymesh
 */
frantic::geometry::polymesh3_ptr get_node_polymesh3( INode* meshNode, TimeValue startTime, TimeValue endTime,
                                                     max_interval_t& outValidityInterval, float timeStepScale,
                                                     bool ignoreEmptyMeshes, bool ignoreTopologyWarnings,
                                                     bool useObjectSpace,
                                                     const frantic::channels::channel_propagation_policy& cpp );

/**
 * This function takes a vector of meshNodes and returns a vector of trimeshes corresponding to a
 * sequence of meshes around the given time.  Each returned mesh will not have any verts that are
 * displaced from the previous mesh in the sequence by more than maxDisplacement.  The function
 * tries to take a step of the size frameOffset, and scales back appropriately until it can find a
 * step where none of the meshes move more than maxdisplacement.
 *
 * @param  meshNodes	a vector of INodes with the meshes you want to fetch
 * @param  t			the TimeValue to get the meshes at
 * @param  maxDisplacement	the max displacement that a vert in the mesh can move before the timestep has to be
 * scaled back in scene units
 * @param  frameOffset	the desired time step to take (in a fraction of a frame), gets overwritten with the actual time
 * step taken
 * @param  outTrimeshes	a vector of trimeshes that will be populated with the fetched meshes
 * @param  timeStepScale    the factor to scale back the interval by when the mesh violates the mas displacement,
 * defaulted to 0.5f
 * @param  ignoreEmptyMeshes	ignore when the mesh is empty, otherwise throw an exception
 * @param  ignoreTopologyWarnings	ignore inconsistent topology and return a mesh with no velocities if the case
 * arises
 */
void get_trimeshes_for_max_displacement( const std::vector<INode*>& meshNodes, TimeValue t, float maxDisplacement,
                                         float& frameOffset, std::vector<frantic::geometry::trimesh3>& outTrimeshes,
                                         float timeStepScale = 0.5f, bool ignoreEmptyMeshes = false,
                                         bool ignoreTopologyWarnings = false );

/**
 * This function takes a vector of meshNodes and returns a vector of trimeshes corresponding to a
 * sequence of meshes around the given time.  Each returned mesh will not have any verts that are
 * displaced from the previous mesh in the sequence by more than maxDisplacement.  The function
 * tries to take a step to time tEnd from tStart, and scales back appropriately until it can find a
 * step where none of the meshes move more than maxdisplacement.
 *
 * @param  meshNodes	a vector of INodes with the meshes you want to fetch
 * @param  tStart		the TimeValue to get the meshes at
 * @param  tEnd			the max TimeValue to consider
 * @param  maxDisplacement	the max displacement that a vert in the mesh can move before the timestep has to be
 * scaled back in scene units
 * @param  timeStepScale    the factor to scale back the interval by when the mesh violates the mas displacement,
 * defaulted to 0.5f
 * @param  ignoreEmptyMeshes	ignore when the mesh is empty, otherwise throw an exception
 * @param  ignoreTopologyWarnings	ignore inconsistent topology and return a mesh with no velocities if the case
 * arises
 * @param  outTrimeshes	a vector of trimeshes that will be populated with the fetched meshes
 * @param  tOut			the time at which the scaling back resulted in meshes which moved less than the max
 * displacement
 */
void get_trimeshes_for_max_displacement( const std::vector<INode*>& meshNodes, TimeValue tEnd, TimeValue tStart,
                                         float maxDisplacement, float timeStepScale, bool ignoreEmptyMeshes,
                                         bool ignoreTopologyWarningsS,
                                         std::vector<frantic::geometry::trimesh3>& outTrimeshes, TimeValue& tOut );

/**
 * This function takes a meshNode, a start time/end time, and caches out series of trimeshes.  How many
 * and what information is included in the trimeshes depends on the options given in the accompanying
 * map and how many samples were requested.
 *
 * @param  meshNode		the node for which the time step should be determined
 * @param  startTime	the startTime of the interval at which to compute the velocity
 * @param  endTime		the endTime of the interval at which to compute the velocity
 * @param  numSamples	the number of samples requested for the interval
 * @param  xss			an xmesh sequence saving object used to save the output meshes
 * @param  filename		the filename sequence object used to generate sequence files
 * @param  options		a map of options affecting how meshes get saved
 */
void cache_node_trimeshes_in_interval( INode* meshNode, TimeValue startTime, TimeValue endTime, int numSamples,
                                       int numRetries, frantic::geometry::xmesh_sequence_saver& xss,
                                       const frantic::files::filename_sequence& fsq,
                                       const std::map<std::string, bool>& options );

} // namespace geometry
} // namespace max3d
} // namespace frantic
