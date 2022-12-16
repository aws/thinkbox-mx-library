// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/graphics/units.hpp>

#pragma warning( push, 3 )
#include <units.h> // 3DS Max units declaration
#pragma warning( pop )

namespace frantic {
namespace max3d {

/**
 * 3DS Max has a system setting for the interpretation of a generic unit. It is typically 1 generic unit = 1 inch, but
 * it can be set to whatever a user desires. This function retrieves the setting as a scale factor from generic units to
 * meters. \return The scale value to convert from generic units to meters.
 */
inline double get_scale_to_meters() {
#if MAX_VERSION_MAJOR >= 24
    return GetSystemUnitScale( UNITS_METERS );
#else
    return GetMasterScale( UNITS_METERS );
#endif
}

/**
 * 3DS Max has a system setting for the interpretation of a generic unit. It is typically 1 generic unit = 1 inch, but
 * it can be set to whatever a user desires. This function retrieves the setting as a scale factor from generic units to
 * millimeters. \return The scale value to convert from generic units to millimeters.
 */
inline double get_scale_to_millimeters() {
#if MAX_VERSION_MAJOR >= 24
    return GetSystemUnitScale( UNITS_MILLIMETERS );
#else
    return GetMasterScale( UNITS_MILLIMETERS );
#endif
}

/**
 * 3DS Max has a system setting for the interpretation of a generic unit. It is typically 1 generic unit = 1 inch, but
 * it can be set to whatever a user desires. This function retrieves the setting as a scale factor from generic units to
 * the specified type. Valid range is 0-6. See MaxSDK units.h. \return The scale value to convert from generic units to
 * the specified type.
 */
inline double get_scale( int type ) {
#if MAX_VERSION_MAJOR >= 24
    return GetSystemUnitScale( type );
#else
    return GetMasterScale( type );
#endif
}

/**
 * 3DS Max has a system setting for the interpretation of a generic unit. It is typically 1 generic unit = 1 inch, but
 * it can be set to whatever a user desires. This function retrieves the setting and converts it to our internal
 * enumeration. \return The currently configured system unit, and the number of units in a single generic unit. Ex.
 * <inches, 2.0> means that 1 generic unit = 2 inches.
 */
inline std::pair<frantic::graphics::length_unit::option, float> get_system_unit_and_scale() {
    int maxType;
    float scale;
#if MAX_VERSION_MAJOR >= 24
    GetSystemUnitInfo( &maxType, &scale );
#else
    GetMasterUnitInfo( &maxType, &scale );
#endif

    frantic::graphics::length_unit::option lengthUnit = frantic::graphics::length_unit::invalid;

    switch( maxType ) {
    case UNITS_INCHES:
        lengthUnit = frantic::graphics::length_unit::inches;
        break;
    case UNITS_FEET:
        lengthUnit = frantic::graphics::length_unit::feet;
        break;
    case UNITS_MILES:
        lengthUnit = frantic::graphics::length_unit::miles;
        break;
    case UNITS_MILLIMETERS:
        lengthUnit = frantic::graphics::length_unit::millimeters;
        break;
    case UNITS_CENTIMETERS:
        lengthUnit = frantic::graphics::length_unit::centimeters;
        break;
    case UNITS_METERS:
        lengthUnit = frantic::graphics::length_unit::meters;
        break;
    case UNITS_KILOMETERS:
        lengthUnit = frantic::graphics::length_unit::kilometers;
        break;
    default:
        break;
    }

    return std::make_pair( lengthUnit, scale );
}

} // namespace max3d
} // namespace frantic
