// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <map>
#include <string>

#include <frantic/channels/property_map.hpp>

namespace frantic {
namespace max3d {
namespace channels {

/**
 * This is the ValueMetaClass associated with property_map_mxs_exposure.
 * Each object that descends from Value needs a ValueMetaClass.
 */
class property_map_mxs_exposure_meta_class : public ValueMetaClass {
  public:
    property_map_mxs_exposure_meta_class()
        : ValueMetaClass( _T("PropertyMap") ) {}
    void collect() { delete this; }
};

extern property_map_mxs_exposure_meta_class property_map_mxs_exposure_class;

// the max classof_methods macro is warning-y
#pragma warning( push, 3 )
#pragma warning( disable : 4100 )

/**
 * The property_container class is a subclass of Value. It is used to
 * expose an arbitrary list of parameters to script.
 */
class property_map_mxs_exposure : public Value {
    frantic::channels::property_map m_props;
    std::map<frantic::tstring, frantic::tstring> m_lowerToPropCase;

    void build_lower_to_prop_case();

  public:
    property_map_mxs_exposure( const frantic::channels::property_map& props )
        : m_props( props ) {
        build_lower_to_prop_case();
    }

    ValueMetaClass* local_base_class();
    classof_methods( property_map_mxs_exposure, Value ); // an sdk macro that adds default classof type functions
    void collect() { delete this; }
    void sprin1( CharStream* s );
    Value* get_property( Value** arg_list, int count );
    Value* set_property( Value** arg_list, int count );
    Value* show_props_vf( Value** arg_list, int count );
    Value* get_props_vf( Value** arg_list, int count );
};

#pragma warning( pop )

} // namespace channels
} // namespace max3d
} // namespace frantic
