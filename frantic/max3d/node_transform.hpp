// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include <control.h>
#include <inode.h>

namespace frantic {
namespace max3d {

inline Matrix3 get_node_transform( INode* pNode, TimeValue t, Interval& outValidity ) {
    Matrix3 result = pNode->GetNodeTM( t, &outValidity );

    result.PreTranslate( pNode->GetObjOffsetPos() );
    PreRotateMatrix( result, pNode->GetObjOffsetRot() );
    ApplyScaling( result, pNode->GetObjOffsetScale() );

    return result;
}

} // namespace max3d
} // namespace frantic
