// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/exception.hpp>
#include <frantic/max3d/fnpublish/Helpers.hpp>
#include <frantic/max3d/fnpublish/Traits.hpp>

#pragma warning( push, 3 )
#include <ifnpub.h>
#pragma warning( pop )

#include <boost/function.hpp>

#include <vector>

namespace frantic {
namespace max3d {
namespace fnpublish {

/**
 * This struct wraps a TimeValue parameter in order to differentiate a time parameter from an int (since TimeValue is a
 * typedef of int). Use this type as the last parameter to a published function to request the 3ds Max time that the
 * function was called at. This can be different from the scene time obtained via Interface::GetTime() for several
 * reasons (ex. using MAXScript 'at time T( functionCall() )').
 */
struct TimeWrapper {
    TimeValue ticks;

    explicit TimeWrapper( TimeValue t_ )
        : ticks( t_ ) {}

    operator TimeValue() const { return ticks; }
};

/**
 * This struct is used to differentiate explicit time parameters from integers. Its use is similar to TimeWrapper,
 * except it connects to an argument instead of an implicit value.
 */
struct TimeParameter {
    TimeValue ticks;

    explicit TimeParameter( TimeValue t_ )
        : ticks( t_ ) {}

    operator TimeValue() const { return ticks; }
};

/**
 * This template class associates a numeric runtime-ID with an enumeration type. Implementors should expose a unique
 * (per publishing interface), constant EnumID. Ex. template<> struct enum_id<MyEnumType>{ static const EnumID ID = -1;
 * };
 */
template <class EnumType>
struct enum_id {
    inline static EnumID get_id() {
        // Use argument dependent lookup to find a get_id() function in the enclosing namespace of EnumType.
        return get_enum_id( EnumType() );

        // NOTE:
        // If you are getting C3861: 'get_enum_id' identifier not found, this means you need to correctly implement
        // either a specialization of frantic::max3d::fnpublish::enum_id<EnumType> or a function in the namespace of
        // EnumType like so: EnumID get_enum_id( EnumType ){ return THE_ID; } which will be found via ADL.
    }
};

namespace detail {
/**
 * Since template typedefs aren't a thing, this class acts similarly. The contained 'type' typedef is the type of
 * callable object that published functions are stored as in InterfaceDesc<T>.
 */
template <class T>
struct invoke_type {
    typedef boost::function<void( TimeValue, typename RemoveConst<T>::type*, FPValue&, FPParams* )> type;
};
} // namespace detail

/**
 * Exposes metadata to 3ds Max that allows other plugins and MAXScript to call published functions. An instance of this
 * class should be held statically for each object that publishes functions. \tparam T The class exposing fucntions.
 */
template <class T>
class InterfaceDesc : public ::FPInterfaceDesc {
  public:
    typedef typename detail::invoke_type<T>::type invoke_type;

  protected:
    /**
     * Constructor. Protected so this class cannot be directly instantiated.
     * \param id The id of the interface publishing functions. Must be globally unique within 3ds Max.
     * \param szName The name of interface publishing functions.
     * \param i18nDesc The localized description of the interface.
     * \param cd The ClassDesc* of the 3ds Max object that exposes this interface. Should be NULL if this is not used
     * with a FPMixinInterface. \param flags Indicates the type of interface being described. Only FP_MIXIN and FP_CORE
     * (for static, singleton objects) are supported.
     */
    InterfaceDesc( Interface_ID id, const MCHAR* szName, StringResID i18nDesc = 0, ClassDesc* cd = NULL,
                   ULONG flags = FP_MIXIN );

  public:
    /**
     * Returns true if no functions are published by the described interface.
     */
    bool empty() const;

    /**
     * Invokes a function on the provided object instance. Any exception thrown by the published function will be
     * captured and rethrown as a MAXException. \param fid The id of the published function to invoke. \param t The time
     * to invoke the function at. Ignored if the function doesn't take a TimeWrapper final parameter. \param pSelf The
     * object to invoke the function on. \param result The return value of the function (if non-void) will be stored
     * here. \param p The collection of parameters passed to the invoked function. \return FPS_NO_SUCH_FUNCTION if the
     * fid doesn't refer to a real function, or FPS_OK otherwise.
     */
    FPStatus invoke_on( FunctionID fid, TimeValue t, T* pSelf, FPValue& result, FPParams* p );

    /**
     * Publishes a property (ie. accessed via Object.PropertyName syntax in MAXScript) with read and write accessor
     * functions. \param szName The name of the property to expose. \param pGetFn The member function ptr to invoke in
     * order to retrieve the property's value. \param pSetFn The member function ptr to invoke in order to assign the
     * proepery a new value.
     */
    template <class R>
    void read_write_property( const MCHAR* szName, R ( T::*pGetFn )( void ), void ( T::*pSetFn )( R ) );

    /**
     * Publishes a property (ie. accessed via Object.PropertyName syntax in MAXScript) that cannot be modified.
     * \param szName The name of the property to expose.
     * \param pGetFn The member function ptr to invoke in order to retrieve the property's value.
     */
    template <class R>
    void read_only_property( const MCHAR* szName, R ( T::*pGetFn )( void ) );

    /**
     * Publishes a function.
     * \tparam MemFnPtrType Any member function pointer type for class T that consists of supported return and parameter
     * types. There is an undefined maximum number of parameters that the function may have.
     *
     * \note If the signature of MemFnPtrType includes a TimeWrapper as the last parameter, it is automatically
     * connected to the 3ds Max time that the function is evaluated at. It will not be published as a formal parameter.
     *
     * \param szName The name of the published function.
     * \param pFn A pointer to a member function of T to expose.
     * \return An object that allows sequential specification of parameter names and default values.
     */
    template <class MemFnPtrType>
    FunctionDesc function( const MCHAR* szName, MemFnPtrType pFn );

    /**
     * Declares a new enumeration. In order to use this you MUST provide a specialization for enum_id<T> that exposes a
     * const EnumID called ID that uniquely identifies the enumeration type for the publishing interface. The returned
     * object allows enum name/value pairs to be added. \return An object that allows sequential specification of
     * enumeration values.
     */
    template <class EnumType>
    EnumDesc<EnumType> enumeration();

  private:
    std::vector<invoke_type> m_dispatchFns;
};

} // namespace fnpublish
} // namespace max3d
} // namespace frantic

// Include the implementation file here. Don't be alarmed, this is not bizarre when it comes to templates and inline
// stuff.
#include <frantic/max3d/fnpublish/InterfaceDesc.inl>

namespace frantic {
namespace max3d {

using frantic::max3d::fnpublish::TimeWrapper;

}
} // namespace frantic
