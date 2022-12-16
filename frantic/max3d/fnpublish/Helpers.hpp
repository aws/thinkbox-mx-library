// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/fnpublish/Traits.hpp>

#pragma warning( push, 3 )
#include <ifnpub.h>
#pragma warning( pop )

namespace frantic {
namespace max3d {
namespace fnpublish {

/**
 * Object used to declare parameter names and default values. Returned from FPInterfaceDesc<T>::function().
 */
class FunctionDesc {
  public:
    FunctionDesc( FPFunctionDef* pDesc );

    /**
     * Assigns the name (and optional string table localized description) to the next parameter.
     * \param szName The name of the parameter.
     * \param i18nDesc The localized description of the parameter.
     * \return *this so that you can string multiple param() calls together.
     */
    FunctionDesc& param( const MCHAR* szName, StringResID i18nDesc = 0 );

    /**
     * Marks the parameter as a optional keyword parameter and assigns the name and default value. Throws
     * std::logic_error if the default value is not the correct type.
     *
     * \note All following parameters must be optional keyword parameters or a std::logic_error is thrown.
     *
     * \param szName The name of the parameter.
     * \param defaultValue The default value if this keyword parameter is not supplied.
     * \param i18nDesc The localized description of the parameter.
     * \return *this so that you can string multiple param() calls together.
     */
    template <class T>
    FunctionDesc& param( const MCHAR* szName, const T& defaultValue, StringResID i18nDesc = 0 );

  private:
    FPParamDef* get_param( std::size_t i );

  private:
    FPFunctionDef* m_pDesc;
    std::size_t m_counter;
};

/**
 * Object used to declare the options of an enumeration, and the associated name used in MAXScript
 */
template <class EnumType>
class EnumDesc {
  public:
    EnumDesc( FPEnum* pDesc );

    /**
     * Adds a new option to the enumeration with the given name and value
     * \param szName The name of the option.
     * \param value The value of the enumeration option.
     */
    EnumDesc& option( const MCHAR* szName, EnumType value );

  private:
    FPEnum* m_pDesc;
};

inline FunctionDesc::FunctionDesc( FPFunctionDef* pDesc )
    : m_pDesc( pDesc )
    , m_counter( 0 ) {}

inline FPParamDef* FunctionDesc::get_param( std::size_t i ) {
    if( i < static_cast<std::size_t>( m_pDesc->params.Count() ) )
        return m_pDesc->params[static_cast<int>( i )];
    throw std::out_of_range( "Invalid parameter index" );
}

inline FunctionDesc& FunctionDesc::param( const MCHAR* szName, StringResID i18nDesc ) {
    FPParamDef* pParam = this->get_param( m_counter++ );

    if( m_pDesc->flags & FP_HAS_KEYARGS )
        throw std::logic_error( "Invalid positional argument after keyword argument" );

    pParam->internal_name = const_cast<MCHAR*>( szName ); // Max 2010 expects a non-const static char*
    pParam->description = i18nDesc;

    return *this;
}

namespace detail {
template <class T>
struct ArrayToPtr {
    typedef T type;
};

template <class T, int N>
struct ArrayToPtr<T[N]> {
    typedef T* type;
};
} // namespace detail

template <class T>
inline FunctionDesc& FunctionDesc::param( const MCHAR* szName, const T& defaultValue, StringResID i18nDesc ) {
    FPParamDef* pParam = this->get_param( m_counter++ );

    // Passing a string constant was mucking things up due to its type being 'const char[N]' instead of 'const char*',
    // so I fix that here.

#if MAX_VERSION_MAJOR < 19
    typedef typename detail::ArrayToPtr<T>::type TModified;
    if( Traits<typename RemoveConstRef<TModified>::type>::fp_param_type() != pParam->type )
#else
    // There were some breaking changes to template specialization in Visual Studio 2015.
    // This caused the old code for dealing with string literals to fail to find the corret Traits<> specialization.
    // This explicitly deals with them.
    typedef typename std::remove_extent<T>::type noextent_t;
    typedef typename std::conditional<std::is_same<MCHAR, noextent_t>::value, const MCHAR*, T>::type fixedstr_t;
    if( Traits<fixedstr_t>::fp_param_type() != pParam->type )
#endif
        throw std::logic_error( "Invalid default parameter value type" );

    m_pDesc->flags |= FP_HAS_KEYARGS;
    m_pDesc->keyparam_count++;

    pParam->internal_name = const_cast<MCHAR*>( szName ); // Max 2010 expects a non-const static char*
    pParam->description = i18nDesc;
    pParam->flags |= FPP_KEYARG;

    if( !pParam->options )
        pParam->options = new FPParamOptions;

#if MAX_VERSION_MAJOR < 19
    Traits<typename RemoveConstRef<TModified>::type>::get_return_value( pParam->options->keyarg_default, defaultValue );
#else
    Traits<fixedstr_t>::get_return_value( pParam->options->keyarg_default, defaultValue );
#endif

    return *this;
}

template <class EnumType>
inline EnumDesc<EnumType>::EnumDesc( FPEnum* pDesc )
    : m_pDesc( pDesc ) {}

template <class EnumType>
inline EnumDesc<EnumType>& EnumDesc<EnumType>::option( const MCHAR* szName, EnumType value ) {
    FPEnum::enum_code newCode;
    newCode.name = const_cast<MCHAR*>( szName ); // Max 2010 expects a non-const static char*
    newCode.code = static_cast<int>( value );

    m_pDesc->enumeration.Append( 1, &newCode );

    return *this;
}

} // namespace fnpublish
} // namespace max3d
} // namespace frantic
