// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace frantic {
namespace max3d {
namespace particles {

// Enums are in namespaces in order to give scope to the values
// ie. "max3d_particle_color::default_color" instead of just "default_color"
namespace max3d_particle_color {
enum max3d_particle_color_enum { default_color, blended_z_depth, blended_camera_distance, medit_slot_1, none };

inline max3d_particle_color_enum from_string( const std::string& mode ) {
    if( mode == "Choose Color" )
        return default_color;
    else if( mode == "Blended Z Depth" )
        return blended_z_depth;
    else if( mode == "Blended Camera Distance" )
        return blended_camera_distance;
    else if( mode == "Medit1" )
        return medit_slot_1;
    else if( mode == "No Material" )
        return none;

    throw std::runtime_error( "max3d_particle_color: Invalid string \"" + mode + "\"" );
}
} // namespace max3d_particle_color

namespace max3d_density_scaling {
enum max3d_density_scaling_enum { no_scaling, material_opacity };

inline max3d_density_scaling_enum from_string( const std::string& mode ) {
    if( mode == "No Scaling" )
        return no_scaling;
    else if( mode == "Material Opacity" )
        return material_opacity;

    throw std::runtime_error( std::string() + "max3d_density_scaling: Invalid string \"" + mode + "\"" );
}
} // namespace max3d_density_scaling

} // namespace particles
} // namespace max3d
} // namespace frantic
