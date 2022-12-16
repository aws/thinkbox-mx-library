// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/strings/tstring.hpp>
#include <map>
#include <string>

// forward declaration
namespace frantic {
namespace max3d {
namespace maxscript {
class property_container_meta_class;
}
} // namespace max3d
} // namespace frantic

// PLEASE NOTE: there needs to be one global instance of property_container_meta_class that registers the class
// property_container with maxscript. the creation of the object is not done here since not all projects will want this
// maxscript type. whatever project is including this file needs to declare this global variable the extern will attach
// to.
extern frantic::max3d::maxscript::property_container_meta_class property_container_class;

namespace frantic {
namespace max3d {
namespace maxscript {

/**
 * This is the ValueMetaClass associated with property_container.
 * Each object that descends from Value needs a ValueMetaClass.
 */
class property_container_meta_class : public ValueMetaClass {
  public:
    property_container_meta_class( const frantic::tstring& name )
        : ValueMetaClass( const_cast<TCHAR*>( name.c_str() ) ) {}
    void collect() { delete this; }
};

/**
 * The property_container class needs to be able to get, set, and list the
 * parameters it represents. Since the container does not actually  store
 * the parameters, an accessor object is needed to provide these functions.
 */
class property_container_accessor {
  public:
    virtual Value* get_maxscript_property( const frantic::tstring& name, TimeValue t ) = 0;
    virtual bool set_maxscript_property( const frantic::tstring& name, Value* v, TimeValue t ) = 0;
    virtual std::vector<frantic::tstring> list_maxscript_properties() = 0;
};

// the max classof_methods macro is warning-y
#pragma warning( push, 3 )
#pragma warning( disable : 4100 )

/**
 * The property_container class is a subclass of Value. It is used to
 * expose an arbitrary list of parameters to script.
 */
class property_container : public Value {
  private:
    property_container_accessor* m_accessor;

  public:
    property_container( property_container_accessor* accessor )
        : m_accessor( accessor ) {}

    ValueMetaClass* local_base_class() { return &property_container_class; }
    classof_methods( property_container, Value ); // a sdk macro that adds default classof type functions
    void collect() { delete this; }
    void sprin1( CharStream* s ) { s->puts( _T("PropertyContainer") ); }
    Value* get_property( Value** arg_list, int count );
    Value* set_property( Value** arg_list, int count );
    Value* show_props_vf( Value** arg_list, int count );
    Value* get_props_vf( Value** arg_list, int count );

    /**
     * Lets the object know that it is no longer associated with the
     * accessor it was once attached to. This also makes the object collectable
     * to maxscript if it wasn't already.
     */
    void free_from_accessor() {
        m_accessor = NULL;
        make_collectable();
    }
};

#pragma warning( pop )

} // namespace maxscript
} // namespace max3d
} // namespace frantic
