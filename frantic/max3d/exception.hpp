// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/exception/diagnostic_information.hpp>

#include <string>

#include <ifnpub.h>
#include <strbasic.h>

namespace frantic {
namespace max3d {

/**
 * In a manner similar to boost::current_exception(), or boost::current_exception_diagnostic_information() this function
 * will rethrow the current exception as a MAXException instance.
 *
 * The expected use of this function is in a generic catch handler since it internally handles all types of exceptions.
 * Ex: try{ some_function_that_throws(); }catch( ... ){
 *    // This is a valid, type-safe use of catch( ... ) because it is guaranteed to rethrow the original exception, or a
 * translated version. rethrow_current_exception_as_max_t();
 * }
 *
 * \note This must only be called in a catch handler.
 * \note The current exception must be a MAXException, a boost::exception subclass, or a std::exception subclass.
 * Anything else will be propogated without translation. \note While quite slow, the performance of this function is
 * trivial in the context of exceptional circumstances.
 */
inline void rethrow_current_exception_as_max_t() {
    try {
        throw;
    } catch( const MAXException& ) {
        throw;
    } catch( const boost::exception& e ) {
#ifdef UNICODE
        throw MAXException( MSTR::FromACP( boost::diagnostic_information( e ).c_str() ).data() );
#else
        throw MAXException( const_cast<MCHAR*>( boost::diagnostic_information( e ).c_str() ) );
#endif
    } catch( const std::exception& e ) {
#ifdef UNICODE
        throw MAXException( MSTR::FromACP( e.what() ).data() );
#else
        throw MAXException( const_cast<MCHAR*>( e.what() ) );
#endif
    } catch( ... ) {
        // If we couldn't determine that it was a member of our usual exception heirarchies, we just let it propogate.
        throw;
    }
}

} // namespace max3d
} // namespace frantic
