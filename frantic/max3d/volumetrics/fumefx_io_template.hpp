// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/strings/tstring.hpp>

class VoxelFlowBase;

namespace frantic {
namespace max3d {
namespace volumetrics {

/**
 * \tparam FumeFXTraits A traits class implementing:
 *                       VoxelFlowBase* CreateVoxelFlow();
 *                       void DeleteVoxelFlow( VoxelFlowBase* );
 * \param fxdPath Path to the .fxd file to load the data from
 * \return A shared_ptr for a VoxelFlowBase that has been initialized via LoadHeader() (ie. You still need to call
 * LoadOutput to get the real data).
 */
template <class FumeFXTraits>
boost::shared_ptr<VoxelFlowBase> GetVoxelFlow( const frantic::tstring& fxdPath, bool throwIfNotFound = true ) {
    boost::shared_ptr<VoxelFlowBase> pResult;

    pResult.reset( FumeFXTraits::CreateVoxelFlow(), &FumeFXTraits::DeleteVoxelFlow );

    int loadResult = pResult->LoadHeader( const_cast<frantic::tchar*>( fxdPath.c_str() ) );
    if( loadResult != LOAD_OK ) {
        if( loadResult == LOAD_USERCANCEL )
            throw std::runtime_error( "FumeFX->LoadHeader() - User cancelled during load" );
        if( loadResult == LOAD_FILEOPENERROR )
            throw std::runtime_error( "FumeFX->LoadHeader() - Error opening file \"" +
                                      frantic::strings::to_string( fxdPath ) + "\"" );
        if( loadResult == LOAD_FILELOADERROR )
            throw std::runtime_error( "FumeFX->LoadHeader() - Error loading from file \"" +
                                      frantic::strings::to_string( fxdPath ) + "\"" );

        // This one always seems to come back. My guess is that LoadHeader doesn't actually use these return codes, and
        // just returns FALSE when there is an error.
        if( loadResult == LOAD_RAMERR ) {
            if( boost::filesystem::exists( fxdPath ) || throwIfNotFound )
                throw std::runtime_error( "FumeFX->LoadHeader() - Error during load of file \"" +
                                          frantic::strings::to_string( fxdPath ) +
                                          "\". Check if this file actually exists." );

            // This file didn't exist, so return null.
            pResult.reset();
        }
    }

    return pResult;
}

template <class FumeFXTraits>
boost::shared_ptr<VoxelFlowBase> CreateEmptyVoxelFlow() {
    boost::shared_ptr<VoxelFlowBase> pResult;

    pResult.reset( FumeFXTraits::CreateVoxelFlow(), &FumeFXTraits::DeleteVoxelFlow );

    return pResult;
}

} // namespace volumetrics
} // namespace max3d
} // namespace frantic
