// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/max3d/fpwrapper/max_typetraits.hpp>
#include <frantic/max3d/logging/maxscript_progress_logger.hpp>
#include <frantic/max3d/maxscript/maxscript.hpp>

#include <frantic/win32/utility.hpp>

namespace frantic {
namespace max3d {
namespace logging {

void maxscript_progress_logger::initialize() {
    mxs::expression( m_loggerMXSObj + _T(".initialize()") ).evaluate<void>();
}

void maxscript_progress_logger::complete() { mxs::expression( m_loggerMXSObj + _T(".complete()") ).evaluate<void>(); }

void maxscript_progress_logger::error( const frantic::tstring& errorMessage ) {
    frantic::tstring newErrorMessage =
        frantic::strings::string_replace( errorMessage, _T("\""), _T("\\\"") ); // ensure quotation marks remain
    mxs::expression( m_loggerMXSObj + _T(".error \"") + newErrorMessage + _T("\"") ).evaluate<void>();
}

void maxscript_progress_logger::update_progress( float progress ) {
    // call as maxscript updata with completed=0, maximum=0, and adjustedProgress parameters
    bool success = mxs::expression( m_loggerMXSObj + _T( ".updateProgress 0 0 " ) +
                                    boost::lexical_cast<frantic::tstring>( get_adjusted_progress( progress ) ) )
                       .evaluate<bool>();

    // throw a cancel exception if the sim is canceled
    if( !success )
        throw frantic::logging::progress_cancel_exception( "Received cancel message." );

    frantic::win32::PumpMessages();
}

void maxscript_progress_logger::update_progress( long long completed, long long maximum ) {
    float progress = ( maximum != 0 ) ? (float)completed * 100.f / maximum : 0.0f;

    // call the maxscript update with completed, maximum and adjustedProgress parameters
    bool success =
        mxs::expression( m_loggerMXSObj + _T(".updateProgress ") + boost::lexical_cast<frantic::tstring>( completed ) +
                         _T(" ") + boost::lexical_cast<frantic::tstring>( maximum ) + _T(" ") +
                         boost::lexical_cast<frantic::tstring>( get_adjusted_progress( progress ) ) )
            .evaluate<bool>();

    // throw a cancel exception if the sim is canceled
    if( !success )
        throw frantic::logging::progress_cancel_exception( "Received cancel message." );

    frantic::win32::PumpMessages();
}

void maxscript_progress_logger::set_title( const frantic::tstring& title ) {
    frantic::tstring newTitle =
        frantic::strings::string_replace( title, _T("\""), _T("\\\"") ); // ensure quotation marks remain
    mxs::expression( m_loggerMXSObj + _T(".setTitle \"") + newTitle + _T("\"") ).evaluate<void>();

    frantic::win32::PumpMessages();
}

} // namespace logging
} // namespace max3d
} // namespace frantic
