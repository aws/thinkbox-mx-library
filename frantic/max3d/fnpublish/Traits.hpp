// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#pragma warning( push, 3 )
#include <ifnpub.h>
#pragma warning( pop )

#include <boost/type_traits/is_enum.hpp>
#include <boost/utility/enable_if.hpp>

#include <memory>

namespace frantic {
namespace max3d {
namespace fnpublish {

/**
 * Template metaprogram that transforms constant reference types into their associated value type. Non-const references
 * and pointers are not modified
 * \tparam T The type to transform.
 * \typedef type The resulting transformed type.
 */
template <class T>
struct RemoveConstRef {
    typedef T type;
};

/**
 * This template partial specialization transforms a const reference to a non-const non-reference type.
 */
template <class T>
struct RemoveConstRef<const T&> {
    typedef T type;
};

/**
 * Template metaprogram that transforms a constant type into a non-const type.
 * \tparam T The type to remove const-ness from.
 * \typedef type The resulting type from removing constness.
 */
template <class T>
struct RemoveConst {
    typedef T type;
};

/**
 * This partial specialization removes const from const types.
 */
template <class T>
struct RemoveConst<const T> {
    typedef T type;
};

/**
 * Traits<> is a traits class that exposes functions for manipulating FPValue objects for the specified type.
 * \tparam T The type to work with when manipulating FPValue and ParamType objects.
 * \tparam Enable This parameter is an implementation detail used with boost::enable_if to allow partial specialization
 * \note For supported types, this class will expose:
 *        typedef param_type;       The type to use as a parameter when working with T
 *        typedef return_type;      The type to use as a return value when working with T
 *        int fp_param_type();      Returns the ParamType value that best represents T as a parameter to a function in
 * the context of 3ds Max fucntion publishing. int fp_return_type();     Returns the ParamType value that best
 * represents T as a return type in the context of 3ds Max fucntion publishing. int fp_ref_return_type(); Returns the
 * ParamType value that best represents const T& as a return type in the context of 3ds Max fucntion publishing.
 *        param_type get_parameter( FPValue& ); Extracts a T value from an FPValue that is a parameter to a published
 * function. void get_return_value( FPValue& fpOutValue, const return_type& val ); Loads a T into an FPValue as a return
 * value of a published function. void get_ref_return_value( FPValue& fpOutValue, const return_type& val ); Loads a
 * reference to a T into an FPValue as return value of a published function.
 */
template <class T, class Enable = void>
struct Traits;

#define FP_RSLT( _type, _v ) ( _type##_RSLT _v )
#define FP_BV_RSLT( _type, _v ) ( _type##_BV_RSLT _v )

#define FPTRAITS( _type, _paramType )                                                                                  \
    template <>                                                                                                        \
    struct Traits<_type> {                                                                                             \
        typedef const _type& param_type;                                                                               \
        typedef _type return_type;                                                                                     \
                                                                                                                       \
        static int fp_param_type() { return _paramType; }                                                              \
        static int fp_return_type() { return _paramType | TYPE_BY_VAL; }                                               \
                                                                                                                       \
        static param_type get_parameter( FPValue& fpValue ) { return FP_FIELD( _paramType, fpValue ); }                \
        static void get_return_value( FPValue& fpOutValue, const return_type& val ) {                                  \
            return fpOutValue.LoadPtr( _paramType | TYPE_BY_VAL, FP_BV_RSLT( _paramType, val ) );                      \
        }                                                                                                              \
    };

#define FPTRAITS_BASIC( _type, _paramType )                                                                            \
    template <>                                                                                                        \
    struct Traits<_type> {                                                                                             \
        typedef _type param_type;                                                                                      \
        typedef _type return_type;                                                                                     \
                                                                                                                       \
        static int fp_param_type() { return _paramType; }                                                              \
        static int fp_return_type() { return _paramType; }                                                             \
                                                                                                                       \
        static param_type get_parameter( FPValue& fpValue ) { return FP_FIELD( _paramType, fpValue ); }                \
        static void get_return_value( FPValue& fpOutValue, return_type val ) {                                         \
            return fpOutValue.LoadPtr( _paramType, FP_RSLT( _paramType, val ) );                                       \
        }                                                                                                              \
    };

#define FPTRAITS_TAB( _type, _paramType )                                                                              \
    template <>                                                                                                        \
    struct Traits<Tab<_type>> {                                                                                        \
        typedef const Tab<_type>& param_type;                                                                          \
        typedef Tab<_type> return_type;                                                                                \
                                                                                                                       \
        static int fp_param_type() { return _paramType; }                                                              \
        static int fp_return_type() { return _paramType | TYPE_BY_VAL; }                                               \
                                                                                                                       \
        static param_type get_parameter( FPValue& fpValue ) { return *FP_FIELD( _paramType, fpValue ); }               \
        static void get_return_value( FPValue& fpOutValue, const return_type& val ) {                                  \
            return fpOutValue.LoadPtr( _paramType | TYPE_BY_VAL, FP_BV_RSLT( _paramType, val ) );                      \
        }                                                                                                              \
    };                                                                                                                 \
    template <>                                                                                                        \
    struct Traits<std::unique_ptr<Tab<_type>>> {                                                                       \
        typedef std::unique_ptr<Tab<_type>> return_type;                                                               \
                                                                                                                       \
        static int fp_return_type() { return _paramType; }                                                             \
                                                                                                                       \
        static void get_return_value( FPValue& fpOutValue, std::unique_ptr<Tab<_type>>& val ) {                        \
            return fpOutValue.LoadPtr( _paramType, FP_RSLT( _paramType, val.release() ) );                             \
        }                                                                                                              \
    };

// Types that are passed by value
FPTRAITS_BASIC( int, TYPE_INT )
FPTRAITS_BASIC( float, TYPE_FLOAT )
FPTRAITS_BASIC( __int64, TYPE_INT64 )
FPTRAITS_BASIC( double, TYPE_DOUBLE )
FPTRAITS_BASIC( bool, TYPE_bool )
FPTRAITS_BASIC( const MCHAR*, TYPE_STRING )
FPTRAITS_BASIC( ReferenceTarget*, TYPE_REFTARG )
FPTRAITS_BASIC( INode*, TYPE_INODE )
FPTRAITS_BASIC( FPInterface*, TYPE_INTERFACE )
FPTRAITS_BASIC( IObject*, TYPE_IOBJECT )
FPTRAITS_BASIC( Value*, TYPE_VALUE )

// Types that are passed as const ref parameters and returned by value.
FPTRAITS( Point2, TYPE_POINT2 )
FPTRAITS( Point3, TYPE_POINT3 )
FPTRAITS( Point4, TYPE_POINT4 )
FPTRAITS( Quat, TYPE_QUAT )
FPTRAITS( MSTR, TYPE_TSTR )
FPTRAITS( Interval, TYPE_INTERVAL )

// Arrays of values are always passed as const ref and returned by value (with the contained types passed
FPTRAITS_TAB( int, TYPE_INT_TAB )
FPTRAITS_TAB( float, TYPE_FLOAT_TAB )
FPTRAITS_TAB( __int64, TYPE_INT64_TAB )
FPTRAITS_TAB( double, TYPE_DOUBLE_TAB )
FPTRAITS_TAB( bool, TYPE_bool_TAB )
FPTRAITS_TAB( Point2*, TYPE_POINT2_TAB )
FPTRAITS_TAB( Point3*, TYPE_POINT3_TAB )
FPTRAITS_TAB( Point4*, TYPE_POINT4_TAB )
FPTRAITS_TAB( Quat*, TYPE_QUAT_TAB )
#if MAX_VERSION_MAJOR >= 15
FPTRAITS_TAB( const MCHAR*, TYPE_STRING_TAB )
#else
FPTRAITS_TAB( MCHAR*, TYPE_STRING_TAB )
template <>
struct Traits<Tab<const MCHAR*>> {
    typedef const Tab<const MCHAR*>& param_type;
    typedef Tab<const MCHAR*> return_type;

    static int fp_param_type() { return TYPE_STRING_TAB; }
    static int fp_return_type() { return TYPE_STRING_TAB; }

    static param_type get_parameter( FPValue& fpValue ) {
        return *reinterpret_cast<Tab<const MCHAR*>*>( TYPE_STRING_TAB_FIELD( fpValue ) );
    }
    static void get_return_value( FPValue& fpOutValue, const return_type& val ) {
        return fpOutValue.LoadPtr( TYPE_STRING_TAB_BV, TYPE_STRING_TAB_BV_RSLT val );
    }
};
#endif
FPTRAITS_TAB( ReferenceTarget*, TYPE_REFTARG_TAB )
FPTRAITS_TAB( INode*, TYPE_INODE_TAB )
FPTRAITS_TAB( FPInterface*, TYPE_INTERFACE_TAB )
FPTRAITS_TAB( IObject*, TYPE_IOBJECT_TAB )
FPTRAITS_TAB( Value*, TYPE_VALUE_TAB )

#undef FPTRAITS_TAB
#undef FPTRAITS
#undef FPTRAITS_BASIC

#undef FP_RSLT
#undef FP_BV_RSLT

template <>
struct Traits<void> {
    static int fp_return_type() { return TYPE_VOID; }
};

/**
 * This partial specialization is enabled when an enumeration type is used, since each declared enumeration is a
 * distinct type but they are all published via TYPE_ENUM to 3ds Max.
 */
template <class EnumType>
struct Traits<EnumType, typename boost::enable_if<boost::is_enum<EnumType>>::type> {
    typedef EnumType param_type;
    typedef EnumType return_type;

    static int fp_param_type() { return TYPE_ENUM; }
    static int fp_return_type() { return TYPE_ENUM; }

    static param_type get_parameter( FPValue& fpValue ) { return static_cast<EnumType>( TYPE_ENUM_FIELD( fpValue ) ); }
    static void get_return_value( FPValue& fpOutValue, EnumType val ) {
        return fpOutValue.LoadPtr( TYPE_ENUM, TYPE_ENUM_RSLT val );
    }
};

template <>
struct Traits<FPValue> {
    typedef const FPValue& param_type;
    typedef FPValue return_type;

    static int fp_param_type() { return TYPE_FPVALUE; }
    static int fp_return_type() { return TYPE_FPVALUE_BV; }

    static param_type get_parameter( FPValue& fpValue ) { return *TYPE_FPVALUE_FIELD( fpValue ); }
    static void get_return_value( FPValue& fpOutValue, const FPValue& val ) {
        fpOutValue.LoadPtr( TYPE_FPVALUE_BV, TYPE_FPVALUE_BV_RSLT val );
    }
};

template <>
struct Traits<const FPValue&> {
    typedef const FPValue& return_type;

    static int fp_return_type() { return TYPE_FPVALUE; }

    static void get_return_value( FPValue& fpOutValue, const FPValue& val ) {
        fpOutValue.LoadPtr( TYPE_FPVALUE, TYPE_FPVALUE_BR_RSLT val );
    }
};

} // namespace fnpublish
} // namespace max3d
} // namespace frantic
