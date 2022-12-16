// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/fnpublish/InterfaceDesc.hpp>

namespace frantic {
namespace max3d {
namespace fnpublish {

/**
 * A baseclass for publishing standalone objects to 3ds Max. Subclass this (and nothing else) when you wish to publish
 * functions, properties and enumerations on objects that will not be part of the normal 3ds Max heirarchy (rooted at
 * Animatable). If you want to support multiple interfaces on a single object, please use IObject instead. \tparam The
 * concrete class inheriting from StandaloneInterface. \note You cannot implement other mixin interfaces on this object
 * (or can you?). Use IObject for that.
 */
template <class T>
class StandaloneInterface : public FPInterface {
  public:
    virtual ~StandaloneInterface();

    /**
     * \return The ID of the standalone interface, retrieved from the descriptor object.
     */
    virtual Interface_ID GetID();

    /**
     * \return The lifetime management style, as an enum value. Should always be 'wantsRelease' for standalone
     * interfaces
     */
    virtual LifetimeType LifetimeControl();

    /**
     * Used when declaring shared ownership of the standalone interface. We use a reference count underneath to manage
     * the the lifetime of the object. \return this
     */
    virtual BaseInterface* AcquireInterface();

    /**
     * Drops a reference to this object, potentially deleting it if no other references exist.
     */
    virtual void ReleaseInterface();

    /**
     * Directly deletes the interface, regardless of the number of existing references
     */
    virtual void DeleteInterface();

    /**
     * Creates a copy of this object (via the copy constructor of T) and returns it.
     */
    virtual BaseInterface* CloneInterface( void* remapDir = NULL );

    /**
     * Standard 3ds Max function for accessing interfaces on objects.
     */
    virtual BaseInterface* GetInterface( Interface_ID id );

    /**
     * This method is called by 3ds Max when a function from this mixin interface needs to be called.
     */
    virtual FPStatus _dispatch_fn( FunctionID fid, TimeValue t, FPValue& result, FPParams* p );

  protected:
    StandaloneInterface();

    /**
     * A singleton instance of this class should be created by subclass implementors. A pointer to the singleton is
     * retrieved via GetDesc().
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
            : InterfaceDesc<T>( id, szName, i18nDesc, NULL, 0 ) {}
    };

  public:
    /**
     * This is redeclared from FPInterface::GetDesc() to narrow the type to be a subclass of ThisInterfaceDesc, which is
     * a specialize FPInterfaceDesc designed for this specific interface. The subclass implementor must instantiate a
     * singleton of 'ThisInterfaceDesc', publish functions, properties and enumerations, and return the pointer via this
     * function. \return A pointer to a singleton descriptor object that contains the metadata needed to have MAXScript
     * call functions of the interface.
     */
    virtual ThisInterfaceDesc* GetDesc() = 0;

  private:
    LONG m_refCount;
};

template <class T>
inline StandaloneInterface<T>::StandaloneInterface()
    : m_refCount( 0 ) {}

template <class T>
inline StandaloneInterface<T>::~StandaloneInterface() {}

template <class T>
inline Interface_ID StandaloneInterface<T>::GetID() {
    return this->GetDesc()->GetID();
}

template <class T>
inline BaseInterface::LifetimeType StandaloneInterface<T>::LifetimeControl() {
    return wantsRelease;
}

template <class T>
inline BaseInterface* StandaloneInterface<T>::AcquireInterface() {
    InterlockedIncrement( &m_refCount );
    return this;
}

template <class T>
inline void StandaloneInterface<T>::ReleaseInterface() {
    FPInterface::ReleaseInterface();

    if( 0 == InterlockedDecrement( &m_refCount ) )
        this->DeleteInterface();
}

template <class T>
inline void StandaloneInterface<T>::DeleteInterface() {
    FPInterface::DeleteInterface();

    delete this;
}

template <class T>
inline BaseInterface* StandaloneInterface<T>::CloneInterface( void* /*remapDir*/ ) {
    return new T( *static_cast<T*>( this ) );
}

template <class T>
inline BaseInterface* StandaloneInterface<T>::GetInterface( Interface_ID id ) {
    if( id == this->GetDesc()->GetID() )
        return static_cast<T*>( this );
    return FPInterface::GetInterface( id );
}

template <class T>
FPStatus StandaloneInterface<T>::_dispatch_fn( FunctionID fid, TimeValue t, FPValue& result, FPParams* p ) {
    return this->GetDesc()->invoke_on( fid, t, static_cast<T*>( this ), result, p );
}

} // namespace fnpublish
} // namespace max3d
} // namespace frantic
