// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/max3d/maxscript/array_ref.hpp>

namespace frantic {
namespace max3d {
namespace fpwrapper {

ArrayRefClass ArrayRef_class( _T("ArrayRef") );

void ArrayRef::sprin1( CharStream* s ) {
    s->puts( _T("#(") );
    for( int i = 0; i < m_tabCount; ++i ) {
        m_get( m_tabPtr, i )->sprin1( s );
        if( i + 1 != m_tabCount )
            s->puts( _T(", ") );
    }
    s->puts( _T(")") );
}

Value* ArrayRef::get_count( Value** /*arg_list*/, int /*count*/ ) { return Integer::intern( m_tabCount ); }

Value* ArrayRef::get_vf( Value** arg_list, int /*count*/ ) {
    int index = arg_list[0]->to_int();
    if( index <= 0 )
        throw RuntimeError( _T("array index must be positive number") );
    if( index > m_tabCount )
        return &undefined;
    return m_get( m_tabPtr, index - 1 );
}

Value* ArrayRef::put_vf( Value** arg_list, int /*count*/ ) {
    int index = arg_list[0]->to_int();
    if( index <= 0 )
        throw RuntimeError( _T("array index must be positive number") );
    if( index > m_tabCount )
        throw RuntimeError( _T("array index out of bounds") );
    m_set( m_tabPtr, index - 1, arg_list[1] );
    return &ok;
}

} // namespace fpwrapper
} // namespace max3d
} // namespace frantic