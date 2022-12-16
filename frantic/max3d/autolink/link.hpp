// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef FRANTIC_BUILDING_MAX_LIBRARY

#if _MSC_VER >= 1400
// For VS 2005
#ifdef _WIN64
#define FRANTIC_MAX_LIB_PLATFORM "x64/"
#else
#define FRANTIC_MAX_LIB_PLATFORM "Win32/"
#endif
#else
// For VS 2003
#define FRANTIC_MAX_LIB_PLATFORM ""
#endif

/*#ifdef _DEBUG
#  error DEBUG mode is not supported by FranticMaxLibrary
#endif*/

#if MAX_RELEASE < 8000
#error This version of 3dsMax is not supported!
#elif MAX_RELEASE < 9000
#define FRANTIC_MAX_LIB_MAXVER "ReleaseMax8"
#elif MAX_RELEASE < 10000
#define FRANTIC_MAX_LIB_MAXVER "ReleaseMax9"
#elif MAX_RELEASE < 12000
#define FRANTIC_MAX_LIB_MAXVER "ReleaseMax2009"
#elif MAX_RELEASE < 13000
#define FRANTIC_MAX_LIB_MAXVER "ReleaseMax2010"
#else
#error This version of 3dsMax is not supported!
#endif

#ifndef FRANTIC_LIB_BASE
#error FRANTIC_LIB_BASE must be defined in the Makefile or .sln to point to the Libraries folder path. (i.e. FRANTIC_LIB_BASE="\"../../Libraries/\"")
#endif

#pragma message( "Linking with " FRANTIC_LIB_BASE "FranticMaxLibrary/" FRANTIC_MAX_LIB_PLATFORM FRANTIC_MAX_LIB_MAXVER \
                 "/FranticMaxLibrary"                                                                                  \
                 ".lib" )
#pragma comment( lib, FRANTIC_LIB_BASE "FranticMaxLibrary/" FRANTIC_MAX_LIB_PLATFORM FRANTIC_MAX_LIB_MAXVER            \
                                       "/FranticMaxLibrary"                                                            \
                                       ".lib" )

#endif
