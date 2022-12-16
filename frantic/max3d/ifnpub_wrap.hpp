// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <ifnpub.h>
#include <maxversion.h>

/**
 * Max 2013 and later renamed ParamTags::end in order to avoid conflicting with std::end. We can use this define in the
 * mean time in order to have both Max 2012 and Max 2013 together.
 */
#if MAX_VERSION_MAJOR < 15
#define PARAMTYPE_END end
#else
#define PARAMTYPE_END p_end
#endif
