// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <maxversion.h>

#if MAX_VERSION_MAJOR >= 14
#include <maxscript/compiler/parser.h>
#include <maxscript/foundation/name.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/foundation/strings.h>
#include <maxscript/foundation/structs.h>
#include <maxscript/maxscript.h>
#else
#pragma warning( push, 3 )
#include <Maxscrpt/parser.h>
#include <Maxscrpt/strings.h>
#include <maxscrpt/funcs.h>
#include <maxscrpt/maxscrpt.h> //This is required to be included first due to Max 2010 having stupid headers.
#include <maxscrpt/numbers.h>
#include <maxscrpt/structs.h>
#pragma warning( pop )
#endif
