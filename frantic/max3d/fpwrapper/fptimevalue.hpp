// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#ifndef FP_TIME_VALUE
#define FP_TIME_VALUE

// warning 4244
#pragma warning( push )
#pragma warning( disable : 4244 )
#include <maxtypes.h>
#pragma warning( pop )

// Wraps a TimeValue so it's a different type than int, for use by the templates
class FPTimeValue {
    TimeValue m_t;

  public:
    FPTimeValue() { m_t = 0; }
    FPTimeValue( const FPTimeValue& rhs ) { m_t = rhs.m_t; }
    FPTimeValue( TimeValue t ) { m_t = t; }

    operator TimeValue() const { return m_t; }
};

#endif
