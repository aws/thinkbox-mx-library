// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
/**
 * Contains a base class for exposing a singleton object to MAXScript via the 3ds Max function publishing system.
 * Clients will subclass StaticInterface<> and use its member functions inherited from FPInterfaceDesc<> that expose
 * functions, enumerations, and properties.
 *
 * Ex.
 *
 * class SomeGlobalObject : public frantic::max3d::StaticInterface<SomeGlobalObject>{
 * public:
 *    SomeGlobalObject();
 *
 *    Point3 DoSomething();
 *    MSTR DoSomethingElse( const Tab<const MCHAR*>& stringList, float optionalFloat = 1.f );
 *    int GetSomeProperty();
 *    void SetSomeProperty( int newVal );
 * };
 *
 * SomeGlobalObject::SomeGlobalObject()
 *    : frantic::max3d::StaticInterface<SomeGlobalObject>( SomeGlobalObjectInterface_ID, _T("SomeGlobalObject"), 0 )
 * {
 *    this->function( _T("DoSomething"), &SomeGlobalObject::DoSomething );
 *
 *    this->function( _T("DoSomethingElse"), &SomeGlobalObject::DoSomethingElse )
 *       .param( "StringList" )
 *       .param( "FloatValue", 1.f );
 *
 *    this->read_write_property( _T("SomeProperty"), &SomeGlobalObject::GetSomeProperty,
 * &SomeGlobalObject::SetSomeProperty );
 * }
 *
 * namespace{
 *    SomeGlobalObject g_SomeGlobalObject; // The singleton instance.
 * }
 */
#pragma once

#include <frantic/max3d/fnpublish/InterfaceDesc.hpp>

namespace frantic {
namespace max3d {
namespace fnpublish {

/**
 * Used as a base class when publishing a singleton object in 3ds Max. Clients will inherit from this object and publish
 * member functions and properties in the constructor. \tparam T The class inheriting from StaticInterface and
 * publishing functions.
 */
template <class T>
class StaticInterface : public InterfaceDesc<T> {
  protected:
    /**
     * Constructor. Registers the instance as a static singleton interface in 3ds Max. Inheriting classes should publish
     * functions and properties in their constructor implementation. \param id The unique id of this singleton
     * interface. \param szName The name of this singleton interface. \param i18nDesc The localized description of the
     * interface.
     */
    StaticInterface( Interface_ID id, const MCHAR* szName, StringResID i18nDesc = 0 )
        : InterfaceDesc<T>( id, szName, i18nDesc, NULL, FP_CORE ) {}

  public:
    /**
     * Do not call directly. This is called automatically by 3ds Max.
     */
    virtual FPStatus _dispatch_fn( FunctionID fid, TimeValue t, FPValue& result, FPParams* p ) {
        return this->invoke_on( fid, t, static_cast<T*>( this ), result, p );
    }
};

} // namespace fnpublish
} // namespace max3d
} // namespace frantic
