// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <frantic/strings/tstring.hpp>
#include <string>

#ifndef MAX_RELEASE
#error "You must include 3ds Max before including frantic/max3d/windows.hpp"
#endif

namespace frantic {
namespace max3d {

// Message box function
inline int MsgBox( const frantic::tstring& lpText, const frantic::tstring& lpCaption, UINT uType = MB_OK,
                   DWORD logType = SYSLOG_ERROR, UINT modeResponse = IDOK ) {
    if( GetCOREInterface()->IsNetworkRenderServer() ) {
        LogSys* log = GetCOREInterface()->Log();

        log->LogEntry( logType, NO_DIALOG, NULL, _T("%s - %s\n"), lpCaption.c_str(), lpText.c_str() );

        // SYSLOG_ERROR does not abort a render, so we need to throw an exception to force it to die
        if( logType == SYSLOG_ERROR ) {
            throw MAXException( const_cast<TCHAR*>( ( lpCaption + _T(" - ") + lpText ).c_str() ) );
        }

        return modeResponse;
    } else {
        return ::MessageBox( GetCOREInterface()->GetMAXHWnd(), lpText.c_str(), lpCaption.c_str(), uType );
    }
}

} // namespace max3d
} // namespace frantic
