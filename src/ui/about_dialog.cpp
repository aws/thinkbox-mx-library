// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/max3d/ui/about_dialog.hpp>

#include <BaseTsd.h>
#include <WinDef.h>

#include <maxapi.h>

#include <boost/regex.hpp>

#include <frantic/max3d/ui/about_resource.h>
#include <frantic/win32/utility.hpp>

namespace {

struct about_dialog_params : private boost::noncopyable {
    about_dialog_params( const frantic::tstring& title, const frantic::tstring& productName,
                         const frantic::tstring& version, const frantic::tstring& notices )
        : title( title )
        , productName( productName )
        , version( version )
        , notices( notices ) {}

    const frantic::tstring& title;
    const frantic::tstring& productName;
    const frantic::tstring& version;
    const frantic::tstring& notices;
};

INT_PTR init_dialog( HWND hwndDlg, LPARAM lParam ) {
    about_dialog_params* params = reinterpret_cast<about_dialog_params*>( lParam );

    assert( params );

    SetWindowText( hwndDlg, params->title.c_str() );
    SetWindowText( GetDlgItem( hwndDlg, IDC_ABOUT_PRODUCT ), params->productName.c_str() );
    SetWindowText( GetDlgItem( hwndDlg, IDC_ABOUT_VERSION ), params->version.c_str() );
    SetWindowText( GetDlgItem( hwndDlg, IDC_ABOUT_NOTICES ), params->notices.c_str() );

    return TRUE;
}

INT_PTR CALLBACK about_dialog_proc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
    switch( uMsg ) {
    case WM_INITDIALOG:
        return init_dialog( hwndDlg, lParam );
    case WM_COMMAND: {
        int ctrlID = LOWORD( wParam );

        switch( ctrlID ) {
        case IDOK:
            EndDialog( hwndDlg, wParam );
            return TRUE;
        }
    } break;
    case WM_CLOSE:
        EndDialog( hwndDlg, IDOK );
        return TRUE;
    }

    return FALSE;
}

} // anonymous namespace

namespace frantic {
namespace max3d {
namespace ui {

void show_about_dialog( HINSTANCE hInstance, const frantic::tstring& title, const frantic::tstring& productName,
                        const frantic::tstring& version, const frantic::tstring& notices ) {
    try {
        using namespace frantic::win32;

        // Change all line endings to CRLF, because the text box
        // control seems to need it to make a line break.
        boost::basic_regex<frantic::tchar> eol( _T("\r\n|\n|\r") );
        const frantic::tstring noticesCRLF = boost::regex_replace( notices, eol, _T("\r\n") );

        about_dialog_params params( title, productName, version, noticesCRLF );

        INT_PTR result = DialogBoxParam( hInstance, _T("IDD_ABOUT"), GetCOREInterface()->GetMAXHWnd(),
                                         &about_dialog_proc, (LPARAM)&params );

        if( result == 0 ) {
            throw std::runtime_error( "show_about_dialog Error: invalid parent window handle" );
        } else if( result == -1 ) {
            throw std::runtime_error( "show_about_dialog Error: " + GetLastErrorMessageA() );
        }
    } catch( std::exception& e ) {
        MessageBoxA( GetCOREInterface()->GetMAXHWnd(), e.what(), "Error", MB_OK | MB_ICONWARNING );
    }
}

} // namespace ui
} // namespace max3d
} // namespace frantic
