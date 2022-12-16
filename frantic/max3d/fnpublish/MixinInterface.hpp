// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
/**
 * Contains a base class for mixin interfaces that publish functions, properties and enumertions to MAXScript. A mixin
 * interface is an abstract class that is "mixed in" to a concrete class via multiple inheritence. A concrete class in
 * 3ds Max (such as a subclass of GeomObject representing a triangle mesh ie. TriMesh) can inherit and implement one or
 * more mixin interfaces and then in turn expose the functionality to MAXScript.
 *
 * Ex.
 *
 * // This is the interface we want to "mix in" to our concrete class and publish functions from.
 * class ISomething : public frantic::max3d::fnpublish::MixinInterface<ISomething>{
 * public:
 *    virtual void DoSomething( const Point3& pos ) = 0;
 *
 *    static ThisInterfaceDesc* GetStaticDesc(){
 *       static ThisInterfaceDesc theDesc( SomethingInterface_ID, "Something", 0 );
 *
 *       if( theDesc.empty() ){ // ie. Check if we haven't initialized the descriptor yet
 *          theDesc.function( "DoSomething", &ISomething::DoSomething )
 *              .param( "Position" );
 *       }
 *
 *       return &theDesc;
 *    }
 *
 *    virtual ThisInterfaceDesc* GetDesc(){
 *       return ISomething::GetStaticDesc();
 *    }
 * };
 *
 * // This is the concrete class (shown implementing a geometry object but any Max base class works). It mixes
 * ISomething in, and provides
 * // implementations of the interface's functions.
 * class ConcreteSomething : public GeomObject, public ISomething{
 * public:
 *
 *    .
 *    .
 *    .
 *
 *    virtual void DoSomething( const Point3& pos ){ ... }
 *
 *    // NOTE: Correctly adding a mixin to a concrete class requires implementing Animatable::GetInterface(), and
 * calling ClassDesc2::AddInterface(). virtual BaseInterface* GetInterface( Interface_ID id ){ if( id ==
 * SomethingInterface_ID ) return static_cast<ISomething*>( this );
 *
 *       if( BaseInterface* fpInterface = ISomething::GetInterface( id ) ) // Query FPMixinInterface via ISomething for
 * the requested interface. return fpInterface; return GeomObject::GetInterface( id );                            //
 * Query the main base class for the requested interface.
 *    }
 * };
 *
 * // Each concrete 3ds Max class requires a ClassDesc object to expose it for creation, saving/loading, etc. All mixin
 * interfaces need to be exposed
 * // via the ClassDesc.
 * class ConcreteSomethingDesc : public ClassDesc2{
 * public:
 *    ConcreteSomethingDesc()
 *    {
 *       this->AddInterface( ISomething::GetStaticDesc() );
 *    }
 * };
 */
#pragma once

#include <frantic/max3d/fnpublish/InterfaceDesc.hpp>

namespace frantic {
namespace max3d {
namespace fnpublish {

/**
 * Classes can use multiple inheritence to 'mix-in' interfaces that inherit from this. This class adds the necessary
 * infrastructure for a subclass to expose/publish functions, properties and enumerations to 3ds Max & MAXScript.
 * \tparam The subclass that is inheriting from MixinInterface<>. This utilizes the curiously recursive template pattern
 * to create appropriate boilerplate code for inheriting classes.
 */
template <class T>
class MixinInterface : public ::FPMixinInterface {
  protected:
    /**
     * A singleton instance of this class should be created by subclass implementors. A pointer to the singleton is
     * retrieved via GetDesc()
     */
    class ThisInterfaceDesc : public InterfaceDesc<T> {
      public:
        /**
         * Constructor. Exposes the mixin interface's published functions and properties.
         * \param id The unique id of the interface.
         * \param szName The name of the interface.
         * \param i18nDesc The localized description of the interface.
         */
        ThisInterfaceDesc( Interface_ID id, const MCHAR* szName, StringResID i18nDesc = 0 )
            : InterfaceDesc<T>( id, szName, i18nDesc, NULL, FP_MIXIN ) {}
    };

  public:
    /**
     * This is redclared from ::FPMixinInterface::GetDesc() to narrow the type to be a subclass of ThisInterfaceDesc,
     * which is a specialize ::FPInterfaceDesc designed for this specific interface. The subclass implementor must
     * instantiate a singleton of 'ThisInterfaceDesc', publish fucntions, properties and enumerations, and return the
     * pointer via this function.
     * \return A pointer to a singleton descriptor object that contains the metadata needed to have MAXScript call
     * functions of the interface.
     */
    virtual ThisInterfaceDesc* GetDesc() = 0;

    /**
     * This is redclared pure virtual from ::FPMixinInterface::GetInterface() because concrete classes that utilize this
     * mixin interface MUST re-implement this function in order to expose the interface they are mixing in. The concrete
     * class must check the interface ID against each mixin interface and then finally against the main base class. Ex.
     * class ConcreteFoo : public GeomObject, public IFoo, public IBar{ virtual BaseInterface* GetInterface(
     * Interface_ID id ){ if( id == FooInterface_ID ) return static_cast<IFoo*>( this ); if( id == BarInterface_ID )
     * return static_cast<IBar*>( this ); if( BaseInterface* ifpbase = IFoo::GetInterface( id ) ) // Query
     * FPMixinInterface & FPInterface implementations via one of the mixed in interfaces too! Don't need to query both
     * though. return ifpbase; return GeomObject::GetInterface( id );
     *        }
     *     };
     *
     * \note Mixin interfaces that inherit from frantic::max3d::fnpublish::MixinInterface<T> can directly call the
     * baseclass implementation instead of additionally checking for the id directly because the default implementation
     * will do the right thing.
     */
    virtual BaseInterface* GetInterface( Interface_ID id ) = 0 {
        if( this->GetDesc()->GetID() == id )
            return static_cast<T*>( this );
        return ::FPMixinInterface::GetInterface( id );
    }

    /**
     * This method is called by 3ds Max when a function from this mixin interface needs to be called.
     */
    virtual FPStatus _dispatch_fn( FunctionID fid, TimeValue t, FPValue& result, FPParams* p ) {
        return this->GetDesc()->invoke_on( fid, t, static_cast<T*>( this ), result, p );
    }
};

} // namespace fnpublish
} // namespace max3d
} // namespace frantic
