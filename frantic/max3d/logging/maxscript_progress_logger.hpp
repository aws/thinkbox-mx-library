// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/logging/progress_logger.hpp>
#include <frantic/strings/tstring.hpp>

namespace frantic {
namespace max3d {
namespace logging {

class maxscript_progress_logger : public frantic::logging::progress_logger {
  private:
    frantic::tstring m_loggerMXSObj;

  public:
    // set maxscript function
    void set_logger_mxs_object( const frantic::tstring& obj ) { m_loggerMXSObj = obj; }

    // for displaying canceled and completion messages
    void initialize();
    void complete();
    void error( const frantic::tstring& errorMessage );

    // normal progress_logger functions
    void update_progress( float progressPercent );
    void update_progress( long long completed, long long maximum );
    void set_title( const frantic::tstring& title );
};

} // namespace logging
} // namespace max3d
} // namespace frantic
