// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <iparamb2.h>
//#include "assetmanagement/AssetUser.h"

// The base_type macro is fully malicious and a real dick move on Autodesk's part. I'm replacing it with an inline
// function.
#ifdef base_type
#undef base_type
inline ParamType2 base_type( int paramType ) { return ( (ParamType2)( ( paramType ) & ~( TYPE_TAB ) ) ); }
#endif

#include <boost/call_traits.hpp>

namespace frantic {
namespace max3d {

namespace detail {
// Get the value of a parameter & its validity interval.
template <class T>
struct ParamBlockGet {
    inline static bool apply( IParamBlock2* pblock, ParamID paramID, TimeValue t, T& outValue, Interval& outValid,
                              int tabIndex ) {
        return pblock->GetValue( paramID, t, outValue, outValid, tabIndex ) != FALSE;
    }
};

template <>
struct ParamBlockGet<bool> {
    inline static bool apply( IParamBlock2* pblock, ParamID paramID, TimeValue t, bool& outValue, Interval& outValid,
                              int tabIndex ) {
        BOOL val;
        if( pblock->GetValue( paramID, t, val, outValid, tabIndex ) ) {
            outValue = val != FALSE;
            return true;
        }
        return false;
    }
};

template <class T>
struct ParamBlockSet {
    inline static void apply( IParamBlock2* pblock, ParamID paramID, TimeValue t, T value, int tabIndex ) {
        pblock->SetValue( paramID, t, value, tabIndex );
    }
};

template <>
struct ParamBlockSet<bool> {
    inline static void apply( IParamBlock2* pblock, ParamID paramID, TimeValue t, bool value, int tabIndex ) {
        BOOL realValue = value ? TRUE : FALSE;
        pblock->SetValue( paramID, t, realValue, tabIndex );
    }
};
} // namespace detail

template <class T>
T get( IParamBlock2* pblock, ParamID paramID, TimeValue t = 0, int tabIndex = 0 ) {
    T result;
    Interval iv;
    detail::ParamBlockGet<T>::apply( pblock, paramID, t, result, iv, tabIndex );
    return result;
}

template <class T>
T get( IParamBlock2* pblock, ParamID paramID, TimeValue t, Interval& outValid, int tabIndex = 0 ) {
    T result;
    detail::ParamBlockGet<T>::apply( pblock, paramID, t, result, outValid, tabIndex );
    return result;
}

template <class T>
void set( IParamBlock2* pblock, ParamID paramID, TimeValue t, T value, int tabIndex = 0 ) {
    detail::ParamBlockSet<T>::apply( pblock, paramID, t, value, tabIndex );
}

} // namespace max3d
} // namespace frantic
