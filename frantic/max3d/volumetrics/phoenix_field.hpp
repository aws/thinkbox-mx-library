// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/volumetrics/field_interface.hpp>

#include <inode.h>

#include <memory>

namespace frantic {
namespace max3d {
namespace volumetrics {

bool is_phoenix_node( INode* pNode, TimeValue t );

/**
 * Returns an object that adapts a PhoenixFD voxel field into the frantic::volumetrics::field_interface interface.
 */
std::unique_ptr<frantic::volumetrics::field_interface> get_phoenix_field( INode* pNode, TimeValue t );

} // namespace volumetrics
} // namespace max3d
} // namespace frantic
