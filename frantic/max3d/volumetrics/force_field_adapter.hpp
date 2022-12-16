// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/volumetrics/field_interface.hpp>

#include <object.h>

#include <memory>

namespace frantic {
namespace max3d {
namespace volumetrics {

bool is_forcefield_node( INode* pNode, TimeValue t );

/**
 * Returns an object that adapts a 3ds Max Force Field (WSM) into the frantic::volumetrics::field_interface interface.
 */
std::unique_ptr<frantic::volumetrics::field_interface> get_force_field_adapter( INode* pNode, TimeValue t );

} // namespace volumetrics
} // namespace max3d
} // namespace frantic
