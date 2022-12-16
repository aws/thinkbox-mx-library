// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <maxversion.h>
#if MAX_VERSION_MAJOR >= 19
#include <interval.h>
#else

inline bool operator==( const Interval& lhs, const Interval& rhs ) {
    return ( lhs.Start() == rhs.Start() ) && ( lhs.End() == rhs.End() );
}

inline bool operator!=( const Interval& lhs, const Interval& rhs ) { return !( lhs == rhs ); }

#endif
