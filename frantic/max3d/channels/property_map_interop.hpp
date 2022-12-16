// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/channels/property_map.hpp>

namespace frantic {
namespace max3d {
namespace channels {

/**
 * Returns the equivalent MaxScript Value* corresponding to a channel value
 * @param data A pointer to a channel
 * @param dataType The type of the data pointed at by data
 * @return A MaxScript Value* with the same value.
 */
extern Value* channel_to_value( const char* data, frantic::channels::data_type_t dataType );

/**
 * @overload
 * Returns a MaxScript Array representing the tuple of channel values.
 * @param data A pointer to a channel
 * @param arity The number of consecutive data values
 * @param dataType The type of the data pointed at by data
 * @return A MaxScript Array* with the same values.
 */
extern Value* channel_to_value( const char* data, std::size_t arity, frantic::channels::data_type_t dataType );

/**
 * @fn get_mxs_parameters
 * This function retrieves parameters from a MaxScript array of Name-Value pairs. The array should contain
 * elements which are themselves two element arrays. The first element of each sub-array should be
 * a string representing the name of the property. The second element should be a MaxScript value type
 * such as a Float, Point3, Color, String, etc.
 *
 * Ex. #(#("Position",[0,0,0]), ...)
 *
 * @param v The MaxScript Value* containing the array of pairs
 * @param t The time at which to get the parameters
 * @param convertToMeters NOT USED!!!!!
 * @param outProps The output property map.
 */
void get_mxs_parameters( Value* v, TimeValue t, bool convertToMeters, frantic::channels::property_map& outProps );

/**
 * This function retrieves all the parameters from the ParamBlock2's of a ReferenceTarget, placing them in
 * the provided property_map.
 *
 * @param  r  The ReferenceTarget (i.e. object plugin, modifier, etc.)
 * @param  t  The time at which to get the parameters
 * @param  convertToMeters  If true, converts any distance-based parameters to meters.
 * @param  outProps  The property_map into which all the parameters go.
 */
void get_pblock2_parameters( ReferenceTarget* r, TimeValue t, bool convertToMeters,
                             frantic::channels::property_map& outProps );

/**
 * If the provided ReferenceTarget has a maxscript-callable callback function, this will call it, and return the
 * properties specified by that callback function.  An example callback function is as follows:
 *
 * <pre>
 * 	fn PropertyCallback convertToMeters = (
 *    local result = #()
 *    append result #("IntProperty", 7)
 *    if convertToMeters then
 *      append result #("Distance", MetersConversionFactor * distanceToObject)
 *    else
 *      append result #("Distance", distanceToObject)
 *    result
 *  )
 * </pre>
 *
 * @param  r  The ReferenceTarget (i.e. object plugin, modifier, etc.)
 * @param  t  The time at which to get the parameters
 * @param  convertToMeters  If true, converts any distance-based parameters to meters.
 * @param  outProps  The property_map into which all the parameters go.
 */
void get_callback_parameters( ReferenceTarget* r, TimeValue t, bool convertToMeters,
                              frantic::channels::property_map& outProps );

/**
 * This function retrieves all the parameters from the ReferenceTarget, including the paramblock2 parameters and the
 * parameters created by a callback function.
 *
 * @param  r  The ReferenceTarget (i.e. object plugin, modifier, etc.)
 * @param  t  The time at which to get the parameters
 * @param  convertToMeters  If true, converts any distance-based parameters to meters.
 * @param  outProps  The property_map into which all the parameters go.
 */
void get_object_parameters( ReferenceTarget* r, TimeValue t, bool convertToMeters,
                            frantic::channels::property_map& outProps );

} // namespace channels
} // namespace max3d
} // namespace frantic
