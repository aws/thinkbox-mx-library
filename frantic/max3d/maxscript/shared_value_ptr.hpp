// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/maxscript/includes.hpp>

#include <boost/shared_ptr.hpp>

namespace frantic {
namespace max3d {
namespace mxs {

namespace detail {
inline void make_collectable( Value* pVal ) { pVal->make_collectable(); }
} // namespace detail

inline boost::shared_ptr<Value> make_shared_value( Value* pVal ) {
    // From observations, make_heap_static() marks a value as permanent and also allows it to live across 3ds Max scene
    // reset.
    return boost::shared_ptr<Value>( pVal->make_heap_static(), &detail::make_collectable );
}

} // namespace mxs
} // namespace max3d
} // namespace frantic
