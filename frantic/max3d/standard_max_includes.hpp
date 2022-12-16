// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

// This header includes all the standard 3ds Max headers we normally use, and also includes
// the 3ds Max auto-link header.

#include <frantic/max3d/autolink/link_max3d.hpp>

// Include the Windows headers with minimal fuss, and without the min and max macros
#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// Bring the std::min and std::max algorithms into the global namespace so the 3ds Max SDK shuts
// up about the macros missing and uses these instead.
#pragma warning( push )
#pragma warning( disable : 4267 ) // Bizarre that MS's standard headers have such warnings...
#include <algorithm>
#pragma warning( pop )
using std::max;
using std::min;

// We want the MSVC math.h to define various constants. Max.h includes math.h, so this is defined here in case
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

// the 3ds Max headers are really noisy at level 4
#pragma warning( push, 3 )
#pragma warning( disable : 4100 )

// 3ds max headers
#if !defined(NO_INIUTIL_USING)
#define NO_INIUTIL_USING
#endif
#include <Max.h>

#if MAX_VERSION_MAJOR < 24
#pragma comment( lib, "zlibdll.lib" )
#endif

#if MAX_RELEASE < 8900 // Only do this for the max 8 and earlier builds
#include <max_mem.h>
#define _INC_CRTDBG // Pretend to already have included crtdbg.h
#endif

#include <Notify.h>
#include <Simpobj.h>
#include <iparamm2.h>
#include <modstack.h>
#include <ref.h>
#include <simpmod.h>
#if _MSC_VER < 1910 || MAX_RELEASE > 19000
// Don't include this header if we're building for 3ds Max 2016 or earlier
// with Visual Studio 2017 or later.
// The Max 2016 and earlier SDK defines some functions that are named
// the same as standard library functions starting in Visual Studio 2017.
// The only time we will be building with this combination is for Frost MX V-Ray
// which doesn't actually need this header.
#include <texutil.h>
#endif
#include <icurvctl.h>
#include <imtl.h>
#include <shaders.h>
#include <splshape.h>
#if MAX_RELEASE >= 25000
#include <geom/point3.h>
#else
#include <point3.h>
#endif
#include <strclass.h>

// The renderer capability system was added in 3ds Max 6.
#if MAX_RELEASE >= 6000
#include <IMtlRender_Compatibility.h>
#endif

// 3ds max maxscript headers
#if MAX_RELEASE >= 14000
#include <maxscript/compiler/parser.h>
#include <maxscript/foundation/3dmath.h>
#include <maxscript/foundation/arrays.h>
#include <maxscript/foundation/colors.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/foundation/strings.h>
#include <maxscript/kernel/value.h>
#include <maxscript/maxscript.h>
#include <maxscript/maxwrapper/bitmaps.h>
#include <maxscript/maxwrapper/mxsmaterial.h>
#include <maxscript/maxwrapper/mxsobjects.h>
#else
#include <MAXScrpt/3dmath.h>
#include <MAXScrpt/MAXScrpt.h>
#include <MAXScrpt/arrays.h>
#include <MAXScrpt/bitmaps.h>
#include <MAXScrpt/maxmats.h>
#include <MAXScrpt/maxobj.h>
#include <MAXScrpt/numbers.h>
#include <MAXScrpt/strings.h>
#include <MAXScrpt/value.h>
#include <Maxscrpt/colorval.h>
#include <Maxscrpt/parser.h>
#endif

// Particle Flow headers
// TODO: We may want to isolate these to a different file.
#include <IParticleObjectExt.h>
#include <ParticleFlow/IPFActionList.h>
#include <ParticleFlow/IPFActionState.h>
#include <ParticleFlow/IPFOperator.h>
#include <ParticleFlow/IPViewManager.h>
#include <ParticleFlow/IParticleChannels.h>
#include <ParticleFlow/IParticleGroup.h>
#include <ParticleFlow/PFClassIDs.h>
#include <ParticleFlow/PFSimpleOperator.h>
//#include <ParticleFlow/IPFSystem.h>
#include <ParticleFlow/IChannelContainer.h>
#include <ParticleFlow/PFMessages.h>

#pragma warning( pop )
