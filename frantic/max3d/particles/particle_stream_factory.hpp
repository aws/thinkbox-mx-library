// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/particles/streams/max_geometry_vert_particle_istream.hpp>
#include <frantic/max3d/particles/streams/max_iparticleobjext_particle_istream.hpp>
#include <frantic/max3d/particles/streams/max_legacy_particle_istream.hpp>
#include <frantic/max3d/particles/streams/max_pflow_particle_istream.hpp>
#include <frantic/max3d/particles/streams/phoenix_particle_istream.hpp>

#include <frantic/max3d/particles/IMaxKrakatoaPRTObject.hpp>
#include <frantic/max3d/particles/max3d_particle_utils.hpp>
#include <frantic/max3d/particles/tp_interface.hpp>

#include <frantic/graphics/camera.hpp>
#include <frantic/particles/streams/empty_particle_istream.hpp>

namespace frantic {
namespace max3d {
namespace particles {

/**
 * Returns true if max_particle_istream_factory() would return a valid particle_istream using this node.
 * @param pNode The node to evaluate
 * @param t The time to evaluate the node's result at
 * @param strict If true, only particle sources are considered. If false, objects that are not otherwise particle
 * sources will have their geometry's vertices interpreted as particles. return True if this node can be passed to
 * max_particle_istream_factory() and result in a valid particle_istream.
 */
inline bool is_particle_istream_source( INode* pNode, TimeValue t, bool strict = false ) {
    if( pNode != NULL ) {
        ObjectState os = pNode->EvalWorldState( t );
        if( os.obj ) {
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
            tp_interface& tpInterface = tp_interface::get_instance();
#endif

            if( IMaxKrakatoaPRTObjectPtr pPRTObject = GetIMaxKrakatoaPRTObject( os.obj ) ) {
                return true;
            } else if( ParticleGroupInterface( os.obj ) ) {
                return true;
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
            } else if( tpInterface.is_available() && tpInterface.is_node_thinking_particles( pNode ) ) {
                return true;
#endif
#if defined( PHOENIX_SDK_AVAILABLE )
            } else if( streams::IsPhoenixObject( pNode, t ).first ) {
                return true;
#endif
            } else if( GetParticleObjectExtInterface( os.obj ) ) {
                return true;
            } else if( os.obj->GetInterface( I_SIMPLEPARTICLEOBJ ) ) {
                return true;
            } else if( !strict && os.obj->CanConvertToType( polyObjectClassID ) ) {
                return true;
            }
        }
    }

    return false;
}

enum particle_system_type {
    kParticleTypeInvalid,
    kParticleTypeKrakatoa,
    kParticleTypeParticleGroup,
    kParticleTypeThinkingParticles,
    kParticleTypePhoenixFD,
    kParticleTypeParticleObjectExt,
    kParticleTypeSimpleParticleObject,
    kParticleTypePolyObject
};

/**
 * Returns the source max_particle_istream_factory() would aquire particles from.
 * @param pNode The node to evaluate
 * @param t The time to evaluate the node's result at
 * @param strict If true, only particle sources are considered. If false, objects that are not otherwise particle
 * sources will have their geometry's vertices interpreted as particles. return a particle_system_type specifying which
 * source type max_particle_istream_factory() would aquire particles from or kParticleTypeInvalid if the node is not a
 * valid particle system. Note that this will always return kParticleTypeInvalid if is_particle_istream_source() would
 * return false, and never returns kParticleTypeInvalid if is_particle_istream_source() would return true.
 */
inline particle_system_type get_particle_system_type( INode* pNode, TimeValue t, bool strict = false ) {
    if( pNode != NULL ) {
        ObjectState os = pNode->EvalWorldState( t );
        if( os.obj ) {
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
            tp_interface& tpInterface = tp_interface::get_instance();
#endif

            if( IMaxKrakatoaPRTObjectPtr pPRTObject = GetIMaxKrakatoaPRTObject( os.obj ) ) {
                return kParticleTypeKrakatoa;
            } else if( ParticleGroupInterface( os.obj ) ) {
                return kParticleTypeParticleGroup;
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
            } else if( tpInterface.is_available() && tpInterface.is_node_thinking_particles( pNode ) ) {
                return kParticleTypeThinkingParticles;
#endif
#if defined( PHONEIX_SDK_AVAILABLE )
            } else if( streams::IsPhoenixObject( pNode, t ).first ) {
                return kParticleTypePhoenixFD;
#endif
            } else if( GetParticleObjectExtInterface( os.obj ) ) {
                return kParticleTypeParticleObjectExt;
            } else if( os.obj->GetInterface( I_SIMPLEPARTICLEOBJ ) ) {
                return kParticleTypeSimpleParticleObject;
            } else if( !strict && os.obj->CanConvertToType( polyObjectClassID ) ) {
                return kParticleTypePolyObject;
            }
        }
    }

    return kParticleTypeInvalid;
}

/**
 * Produces a particle_istream from an INode by inspecting the result of INode::EvalWorldState().
 * @param node The scene node that produces particle data.
 * @param particleChannelMap The channels requested from the particle stream.
 * @param time The scene time to evaluate the particles source.
 * @param timeStep The offset from 'time' to evaluate the object at when using finite differences (ex. for velocity).
 * @param strict If true, only particle sources are considered. If false, objects that are not otherwise particle
 * sources will have their geometry's vertices interpreted as particles.
 * @return A shared_ptr to the new particle_istream subclass instance.
 */
inline boost::shared_ptr<frantic::particles::streams::particle_istream>
max_particle_istream_factory( INode* node, const frantic::channels::channel_map& particleChannelMap, TimeValue time,
                              TimeValue timeStep, bool strict = false ) {
    using namespace frantic::particles::streams;
    using namespace frantic::particles;

    if( node != NULL ) {
        ObjectState os = node->EvalWorldState( time );
        if( os.obj ) {
            boost::shared_ptr<particle_istream> result;

#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
            tp_interface& tpInterface = tp_interface::get_instance();
#endif

            if( IMaxKrakatoaPRTObjectPtr pPRTObject = GetIMaxKrakatoaPRTObject( os.obj ) ) {
                Interval validInterval = FOREVER;
                result = pPRTObject->CreateStream( node, time, validInterval, Class_ID( 0, 0 ) );
                result->set_channel_map( particleChannelMap );
            } else if( ParticleGroupInterface( os.obj ) ) {
                result.reset( new streams::max_pflow_particle_istream( node, time, particleChannelMap ) );
#if defined( THINKING_PARTICLES_SDK_AVAILABLE )
            } else if( tpInterface.is_available() && tpInterface.is_node_thinking_particles( node ) ) {
                result = tpInterface.get_particle_stream( particleChannelMap, node, NULL, time );
#endif
#if defined( PHONEIX_SDK_AVAILABLE )
            } else if( streams::IsPhoenixObject( node, time ).first ) {
                result = streams::GetPhoenixParticleIstream( node, time, particleChannelMap );
                if( !result || result->particle_count() == 0 ) {
                    result.reset( new frantic::particles::streams::empty_particle_istream( particleChannelMap ) );
                }
#endif
            } else if( GetParticleObjectExtInterface( os.obj ) ) {
                result.reset( new streams::max_particleobjext_particle_istream( node, time, particleChannelMap ) );
            } else if( os.obj->GetInterface( I_SIMPLEPARTICLEOBJ ) ) {
                result.reset( new streams::max_legacy_particle_istream( node, time, particleChannelMap ) );
            } else if( !strict && os.obj->CanConvertToType( polyObjectClassID ) ) {
                result.reset(
                    new streams::max_geometry_vert_particle_istream( node, time, timeStep, particleChannelMap ) );
                result = transform_stream_with_inode( node, time, timeStep, result );
            } else
                throw std::runtime_error(
                    "max_particle_istream_factory() - Could not determine the correct factory type for node: " +
                    frantic::strings::to_string( node->GetName() ) );

            // Avoid operating on NULL streams.  Such NULL streams may be returned by GetPhoenixParticleIstream().
            // TODO: Should this be fixed in GetPhoenixParticleIstream() instead?
            if( result ) {
                result = visibility_density_scale_stream_with_inode( node, time, result );
            }
            return result;
        }
    }

    throw std::runtime_error( std::string() +
                              "max_particle_istream_factory: Unable to create a particle_istream from node \"" +
                              frantic::strings::to_string( node ? node->GetName() : _T("null") ) + "\"." );
}

/**
 * @overload
 */
inline boost::shared_ptr<frantic::particles::streams::particle_istream>
max_particle_istream_factory( INode* pNode, IMaxKrakatoaPRTEvalContextPtr pEvalContext, Interval& valid ) {
    using namespace frantic::particles::streams;
    using namespace frantic::particles;

    TimeValue t = pEvalContext->GetTime();
    TimeValue tStep = 10; // Hardcoding 10 ticks for finite differences.

    boost::shared_ptr<particle_istream> result;

    Interval validInterval = FOREVER;

    if( pNode != NULL ) {
        ObjectState os = pNode->EvalWorldState( t );
        if( os.obj ) {
            if( IMaxKrakatoaPRTObjectPtr pPRTObject = GetIMaxKrakatoaPRTObject( os.obj ) )
                result = pPRTObject->CreateStream( pNode, validInterval, pEvalContext );

            valid &= os.obj->ObjectValidity( t );
        }
    }

    if( !result )
        result = max_particle_istream_factory( pNode, pEvalContext->GetDefaultChannels(), t, tStep, true );

    return result;
}

inline boost::shared_ptr<frantic::particles::streams::particle_istream>
max_particle_istream_factory( INode* pNode, IMaxKrakatoaPRTEvalContextPtr pEvalContext ) {
    Interval garbage = FOREVER;
    return max_particle_istream_factory( pNode, pEvalContext, garbage );
}

} // namespace particles
} // namespace max3d
} // namespace frantic
