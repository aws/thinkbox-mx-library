// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#ifndef FP_NODE_HANDLE_H
#define FP_NODE_HANDLE_H

// A wrapper for node handle so it may be stored as an int.
class FPNodeHandle {
  public:
    inline FPNodeHandle( INode* node = NULL ) {
        if( node == NULL )
            m_handle = 0;
        else
            m_handle = node->GetHandle();
    }

    operator INode*() const {
        if( m_handle != 0 )
            return GetCOREInterface()->GetINodeByHandle( m_handle );
        else
            return NULL;
    }

  private:
    unsigned int m_handle;
};

#endif
