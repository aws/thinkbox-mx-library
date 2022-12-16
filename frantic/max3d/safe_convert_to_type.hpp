// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/shared_ptr.hpp>

#include <object.h>

namespace frantic {
namespace max3d {

namespace detail {
struct optional_delete {
    bool doDelete;
    optional_delete( bool doDelete_ )
        : doDelete( doDelete_ ) {}
    void operator()( Object* pObj ) {
        if( pObj && this->doDelete )
            pObj->MaybeAutoDelete();
    }
};
} // namespace detail

template <class T>
boost::shared_ptr<T> safe_convert_to_type( Object* pObj, TimeValue t, Class_ID classId ) {
    if( !pObj || !pObj->CanConvertToType( classId ) )
        return boost::shared_ptr<T>();

    Object* pResult = pObj->ConvertToType( t, classId );

    return boost::shared_ptr<T>( static_cast<T*>( pResult ), detail::optional_delete( pResult != pObj ) );
}

} // namespace max3d
} // namespace frantic
