// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#pragma warning( push, 3 )
#include <XRef/iXrefObj.h>
#pragma warning( pop )

namespace frantic {
namespace max3d {
namespace geopipe {

inline Object* get_object_ref( INode* inode ) {
    Object* obj = inode->GetObjectRef();

    // check if the object is an xref
    if( obj->ClassID() == XREFOBJ_CLASS_ID && obj->SuperClassID() == SYSTEM_CLASS_ID ) {
        IXRefObject8* xref = (IXRefObject8*)obj;
        Object* sourceObj = xref->GetSourceObject(); // note: this call can also return to us the xref'd modifiers
        if( sourceObj ) {
            // return the xref source object if (how should we handle this being null?)
            return sourceObj;
        }
    }

    // this object doesn't have a xref'd object
    return obj;
}

} // namespace geopipe
} // namespace max3d
} // namespace frantic
