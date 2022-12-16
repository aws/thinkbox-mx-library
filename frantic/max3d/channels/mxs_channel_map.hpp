// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/channels/channel_map.hpp>

namespace frantic {
namespace max3d {
namespace channels {

inline Value* get_mxs_channel_map( const frantic::channels::channel_map& pcm ) {
    two_typed_value_locals( Array * result, Array * tuple );

    vl.result = new( GC_IN_HEAP ) Array( (int)pcm.channel_count() );

    for( std::size_t i = 0; i < pcm.channel_count(); ++i ) {
        const frantic::channels::channel& ch = pcm[i];

        vl.tuple = new( GC_IN_HEAP ) Array( 3 );
        vl.tuple->append( new( GC_IN_HEAP ) String( const_cast<MCHAR*>( ch.name().c_str() ) ) );
        vl.tuple->append( new( GC_IN_HEAP ) String(
            const_cast<MCHAR*>( frantic::channels::channel_data_type_str( ch.data_type() ) ) ) );
        vl.tuple->append( new( GC_IN_HEAP ) Integer( (int)ch.arity() ) );

        vl.result->append( vl.tuple );
    }

    return_value( vl.result );
}

} // namespace channels
} // namespace max3d
} // namespace frantic
