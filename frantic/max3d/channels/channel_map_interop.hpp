// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/channels/channel_map.hpp>
#include <frantic/logging/logging_level.hpp>

#include <tab.h>

namespace frantic {
namespace max3d {
namespace channels {

void set_channel_map( frantic::channels::channel_map& outMap, const Tab<const MCHAR*>& channels ) {
    outMap.reset();

    for( int i = 0, iEnd = channels.Count(); i < iEnd; ++i ) {
        frantic::tstring chStr( channels[i] );

        std::pair<frantic::channels::data_type_t, std::size_t> dt( frantic::channels::data_type_invalid, 0 );

        std::size_t nameEnd = chStr.find( _T( ' ' ) );
        std::size_t typeStart = frantic::tstring::npos;
        if( nameEnd != 0 && nameEnd != frantic::tstring::npos )
            typeStart = chStr.find_first_not_of( _T( ' ' ), nameEnd );

        if( typeStart != frantic::tstring::npos )
            dt = frantic::channels::channel_data_type_and_arity_from_string( chStr.substr( typeStart ) );

        if( dt.first == frantic::channels::data_type_invalid || dt.second == 0 ) {
            FF_LOG( error ) << _T("Malformed channel: \"") << chStr << _T("\"") << std::endl;
            continue;
        }

        frantic::tstring chName = chStr.substr( 0, nameEnd );
        if( outMap.has_channel( chName ) ) {
            FF_LOG( warning ) << _T("Duplicate channel: \"") << chName << _T("\"") << std::endl;
        } else {
            outMap.define_channel( chName, dt.second, dt.first );
        }
    }

    outMap.end_channel_definition();
}

} // namespace channels
} // namespace max3d
} // namespace frantic
