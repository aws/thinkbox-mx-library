// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/channels/channel_map_adaptor.hpp>
#include <frantic/max3d/shaders/map_query.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

namespace frantic {
namespace max3d {
namespace particles {
namespace streams {

using frantic::particles::streams::particle_range;

namespace color_effects {
enum color_effect { none, replace };
}

namespace density_effects {
enum density_effect {
    none,
    replace_with_opacity,
    multiply_with_opacity,
};
}

class material_colored_particle_istream_data;

class material_colored_particle_istream : public frantic::particles::streams::delegated_particle_istream {
    material_colored_particle_istream_data* m_pData;
    shaders::multimapping_shadecontext m_shadeContext;

    frantic::channels::channel_map_adaptor m_adaptor;
    frantic::channels::channel_map m_outPcm, m_nativePcm;

  public:
    material_colored_particle_istream(
        boost::shared_ptr<frantic::particles::streams::particle_istream> pDelegate, Mtl* pMtl, TimeValue t,
        frantic::graphics::transform4f worldToObjectTM, bool doShading, bool doDensityShading,
        frantic::max3d::shaders::renderInformation renderInfo = frantic::max3d::shaders::defaultRenderInfo );

    virtual ~material_colored_particle_istream();

    void set_channel_map( const frantic::channels::channel_map& pcm );
    void set_default_particle( char* buffer );

    std::size_t particle_size() const { return m_outPcm.structure_size(); }

    const frantic::channels::channel_map& get_channel_map() const { return m_outPcm; }
    const frantic::channels::channel_map& get_native_channel_map() const { return m_nativePcm; }

    bool get_particle( char* outBuffer );
    bool get_particles( char* outBuffer, std::size_t& numParticles );
};

} // namespace streams
} // namespace particles
} // namespace max3d
} // namespace frantic
