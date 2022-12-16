// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/strings/tstring.hpp>

#include <boost/config.hpp>

#include <boost/unordered_set.hpp>

#if MAX_VERSION_MAJOR >= 14
#include <INamedSelectionSetManager.h>
#endif

namespace frantic {
namespace max3d {
namespace geopipe {

// I assume that no two named selection sets can be named the same
inline void get_named_selection_set_nodes( const frantic::tstring& selectionName, std::vector<INode*>& outNodes ) {
#if MAX_VERSION_MAJOR >= 14
    INamedSelectionSetManager* pI = INamedSelectionSetManager::GetInstance();
#else
    Interface* pI = GetCOREInterface();
#endif

    int nSets = pI->GetNumNamedSelSets();

    for( int i = 0; i < nSets; ++i ) {
        if( frantic::tstring( pI->GetNamedSelSetName( i ) ) == selectionName ) {
            int nNodes = pI->GetNamedSelSetItemCount( i );
            for( int j = 0; j < nNodes; ++j ) {
                INode* pNode = pI->GetNamedSelSetItem( i, j );
                if( pNode )
                    outNodes.push_back( pNode );
            }

            return;
        }
    }
}

namespace detail {

typedef boost::unordered_set<INode*> INodePtrSet;

} // namespace detail

inline void get_named_selection_set_union( const std::vector<frantic::tstring>& names, std::vector<INode*>& outNodes ) {
#if MAX_VERSION_MAJOR >= 14
    INamedSelectionSetManager* pI = INamedSelectionSetManager::GetInstance();
#else
    Interface* pI = GetCOREInterface();
#endif

    int nSets = pI->GetNumNamedSelSets();

    detail::INodePtrSet outSet;
    boost::unordered_set<frantic::tstring> nameSet;

    nameSet.insert( names.begin(), names.end() );
    for( int i = 0; i < nSets; ++i ) {
        if( nameSet.find( pI->GetNamedSelSetName( i ) ) != nameSet.end() ) {
            int nNodes = pI->GetNamedSelSetItemCount( i );
            for( int j = 0; j < nNodes; ++j ) {
                INode* pNode = pI->GetNamedSelSetItem( i, j );
                if( pNode )
                    outSet.insert( pNode );
            }
        }
    }

    for( detail::INodePtrSet::iterator it = outSet.begin(); it != outSet.end(); ++it )
        outNodes.push_back( *it );
}

} // namespace geopipe
} // namespace max3d
} // namespace frantic
