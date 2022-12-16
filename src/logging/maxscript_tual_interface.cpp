// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

// #include <frantic/logging/tual_log.hpp>

#include <frantic/max3d/logging/maxscript_tual_interface.hpp>

#define TUAL_LOGGING_INTERFACE_ID Interface_ID( 0x71af2c7a, 0x12871142 )

namespace frantic {
namespace max3d {
namespace logging {

// using namespace frantic::logging;

maxscript_tual_interface maxscript_tual_interface::singletonInstance;

maxscript_tual_interface::maxscript_tual_interface() {
    FFCreateDescriptor desc( this, TUAL_LOGGING_INTERFACE_ID, _T("tual_logging"), 0 );

    desc.add_property( &maxscript_tual_interface::get_logging_server_name,
                       &maxscript_tual_interface::set_logging_server_name, _T("serverName") );
    desc.add_property( &maxscript_tual_interface::get_logging_server_port,
                       &maxscript_tual_interface::set_logging_server_port, _T("serverPort") );
    desc.add_property( &maxscript_tual_interface::get_logging_enabled, &maxscript_tual_interface::set_logging_enabled,
                       _T("enabled") );
    desc.add_property( &maxscript_tual_interface::get_logging_application_name,
                       &maxscript_tual_interface::set_logging_application_name, _T("applicationName") );
}

maxscript_tual_interface::~maxscript_tual_interface() {}

// see the header file for details on why this function exists
void maxscript_tual_interface::enable_maxscript_object() {}

void maxscript_tual_interface::set_logging_server_name( const frantic::tstring& serverName ) {
    // tual_message::set_server_name( frantic::strings::to_string( serverName ) );
}

const frantic::tstring maxscript_tual_interface::get_logging_server_name() {
    return _T("");
    // return frantic::strings::to_tstring( tual_message::server_name() );
}

void maxscript_tual_interface::set_logging_server_port( int port ) {
    // tual_message::set_server_port( port );
}

int maxscript_tual_interface::get_logging_server_port() {
    return 0;
    // return tual_message::server_port();
}

void maxscript_tual_interface::set_logging_enabled( bool enabled ) {
    // tual_message::set_tual_enabled( enabled );
}

bool maxscript_tual_interface::get_logging_enabled() {
    return false;
    // return tual_message::tual_enabled();
}

void maxscript_tual_interface::set_logging_application_name( const frantic::tstring& name ) {
    // tual_message::set_application_name( frantic::strings::to_string( name ) );
}

const frantic::tstring maxscript_tual_interface::get_logging_application_name() {
    return _T("");
    // return frantic::strings::to_tstring( tual_message::application_name() );
}

} // namespace logging
} // namespace max3d
} // namespace frantic
