// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/fpwrapper/static_wrapper.hpp>
#include <frantic/strings/tstring.hpp>

// the purpose of this class is to expose the tual-logging setting to maxscript, s.t. they can be set up in a script

namespace frantic {
namespace max3d {
namespace logging {

class maxscript_tual_interface
    : public frantic::max3d::fpwrapper::FFStaticInterface<maxscript_tual_interface, FP_CORE> {
  private:
    static maxscript_tual_interface singletonInstance;

    maxscript_tual_interface();
    ~maxscript_tual_interface();

  public:
    // this is a horrible hack in order to ensure that this compilation unit is activated at dll load time
    // basically, if you want to be able to call this maxscript interface, this function must be called
    // at least once somewhere in your code to ensure that c++ doesn't exclude it from the linker
    // LibInitialize would be an ideal place.  Calling it more than once doesn't make any difference,
    // it just needs to be called!
    static void enable_maxscript_object();

    void set_logging_server_name( const frantic::tstring& serverName );
    const frantic::tstring get_logging_server_name();

    void set_logging_server_port( int port );
    int get_logging_server_port();

    void set_logging_enabled( bool enabled );
    bool get_logging_enabled();

    void set_logging_application_name( const frantic::tstring& name );
    const frantic::tstring get_logging_application_name();
};

} // namespace logging
} // namespace max3d
} // namespace frantic
