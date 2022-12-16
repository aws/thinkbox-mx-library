// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/particles/streams/material_affected_particle_istream.hpp>
#include <frantic/particles/streams/density_scale_particle_istream.hpp>
#include <frantic/particles/streams/empty_particle_istream.hpp>
#include <frantic/particles/streams/set_channel_particle_istream.hpp>
#include <frantic/particles/streams/transformed_particle_istream.hpp>

#define FF_PARTICLE_FLOW_CLASS_ID Class_ID( 1962490626, 515064576 )
#define FF_THINKING_PARTICLES_CLASS_ID Class_ID( 1225677363, 1171929551 )
#define FF_PFSOURCE_CLASS_ID Class_ID( 1345457306, 0 )

namespace frantic {
namespace max3d {
namespace particles {

inline boost::shared_ptr<frantic::particles::streams::particle_istream> material_shade_stream_with_inode(
    INode* pNode, TimeValue t, bool shadeColor, bool shadeDensity,
    boost::shared_ptr<frantic::particles::streams::particle_istream> pin,
    frantic::max3d::shaders::renderInformation renderInfo = frantic::max3d::shaders::defaultRenderInfo ) {
    using frantic::graphics::color3f;
    using frantic::graphics::transform4f;
    using namespace frantic::particles;
    using namespace frantic::particles::streams;
    using namespace frantic::max3d::particles::streams;

    if( pNode->GetMtl() ) {
        if( shadeColor || shadeDensity ) {
            transform4f objTM = pNode->GetNodeTM( t );
            objTM.to_inverse();

            pin.reset( new material_colored_particle_istream( pin, pNode->GetMtl(), t, objTM, shadeColor, shadeDensity,
                                                              renderInfo ) );
        }
    } else if( shadeColor && !pin->get_native_channel_map().has_channel( _T("Color") ) ) {
        // If there is no material or default color already, apply the node's wireframe color
        pin.reset( new set_channel_particle_istream<color3f>( pin, _T("Color"),
                                                              color3f::from_RGBA( pNode->GetWireColor() ) ) );
    }

    return pin;
}

inline boost::shared_ptr<frantic::particles::streams::particle_istream>
transform_stream_with_inode( INode* node, TimeValue time, TimeValue timeStep,
                             const boost::shared_ptr<frantic::particles::streams::particle_istream>& pin ) {
    using namespace frantic::particles::streams;
    using frantic::graphics::transform4f;

    if( node != 0 ) {
        transform4f nodeTransform = node->GetObjTMAfterWSM( time );
        transform4f nodeTransformDerivative = node->GetObjTMAfterWSM( time + timeStep );

        nodeTransformDerivative -= nodeTransform;
        nodeTransformDerivative *= 1.f / TicksToSec( timeStep );

        if( !( nodeTransform.is_identity() && nodeTransformDerivative.is_zero() ) )
            return boost::shared_ptr<particle_istream>(
                new transformed_particle_istream<float>( pin, nodeTransform, nodeTransformDerivative ) );
    }

    return pin;
}

inline boost::shared_ptr<frantic::particles::streams::particle_istream> visibility_density_scale_stream_with_inode(
    INode* node, TimeValue time, const boost::shared_ptr<frantic::particles::streams::particle_istream>& pin ) {
    if( node != 0 ) {
        // We want to use the value held in the controller at time t, not the value returned from
        // node->GetVisibility() because we do not want to clamp at 1.0;
        float visibility = 1.f;
        Control* visController = node->GetVisController();

        if( visController ) {
            Interval ivalid = FOREVER;
            visController->GetValue( time, &visibility, ivalid );
        } else {
            visibility = node->GetVisibility( time );
        }

        // Try to avoid making a new particle_istream if we can.
        if( visibility <= 0 )
            return boost::shared_ptr<frantic::particles::streams::particle_istream>(
                new frantic::particles::streams::empty_particle_istream( pin->get_channel_map() ) );
        else if( visibility != 1.f )
            return boost::shared_ptr<frantic::particles::streams::particle_istream>(
                new frantic::particles::streams::density_scale_particle_istream( pin, visibility ) );
    }

    return pin;
}

} // namespace particles
} // namespace max3d
} // namespace frantic
