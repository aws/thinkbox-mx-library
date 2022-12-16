// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/max3d/maxscript/property_container.hpp>

#if MAX_RELEASE == 8000
#include <frantic/misc/string_functions.hpp>
#endif

namespace frantic {
namespace max3d {
namespace maxscript {

/**
 * Overriding the internal get_property that maxscript calls.
 * Do not directly call this function.
 *
 * @param  arg_list  set by maxscript
 * @param  count     set by maxscript
 * @return           maxscript protected return value
 */
Value* property_container::get_property( Value** arg_list, int count ) {
    Value* retVal = NULL;
    if( m_accessor && count >= 1 && arg_list[0] ) {
        frantic::tstring inputParam = arg_list[0]->to_string();
        retVal = m_accessor->get_maxscript_property( inputParam, MAXScript_time() );
    }
    return retVal ? retVal : Value::get_property( arg_list, count );
}

/**
 * Overriding the internal set_property that maxscript calls.
 * Do not directly call this function.
 *
 * @param  arg_list  set by maxscript
 * @param  count     set by maxscript
 * @return           maxscript protected return value
 */
Value* property_container::set_property( Value** arg_list, int count ) {
    bool success = false;
    if( m_accessor && count >= 2 && arg_list[0] && arg_list[1] ) {
        frantic::tstring inputParam = arg_list[1]->to_string();
        success = m_accessor->set_maxscript_property( inputParam, arg_list[0], MAXScript_time() );
    }
    return success ? arg_list[0] : Value::set_property( arg_list, count );
}

/**
 * Overriding the internal show_props_vf that maxscript calls.
 * Do not directly call this function.
 *
 * @param  arg_list  set by maxscript
 * @param  count     set by maxscript
 * @return           maxscript protected return value
 */
Value* property_container::show_props_vf( Value** arg_list, int count ) {
    if( m_accessor ) {

        // get the proper output stream (from sample code)
        CharStream* out = (CharStream*)key_arg( to );
        if( out == (CharStream*)&unsupplied )
            out = thread_local( current_stdout );
        else if( out == (CharStream*)&undefined )
            out = NULL;
        else if( !is_charstream( out ) )
            throw TypeError( _T("showProperties to: argument must be a stream, got: "), out );

        // hasProperty gives us an input value, showProperties does not
        frantic::tstring inputProp = _T("");
        bool hasPropertyCalled = false;
        if( count > 0 && arg_list[0] != &keyarg_marker ) {
            inputProp = arg_list[0]->to_string();
            hasPropertyCalled = true;
        }

        std::vector<frantic::tstring> paramList = m_accessor->list_maxscript_properties();
        for( unsigned int i = 0; i < paramList.size(); ++i ) {

            if( hasPropertyCalled ) {
                // hasProperty

#if MAX_RELEASE == 8000
                // problem: i can't link to the provided max_name_match function in the max 8 sdk!! i'm just going to do
                // string compare instead
                if( frantic::strings::to_lower( paramList[i] ) == frantic::strings::to_lower( inputProp ) )
                    return &true_value;
#else
                // use the proper max's regular expression matcher
                if( max_name_match( const_cast<TCHAR*>( paramList[i].c_str() ),
                                    const_cast<TCHAR*>( inputProp.c_str() ) ) )
                    return &true_value;
#endif

            } else if( out ) {
                // print property name
                out->puts( _T("  .") );
                out->puts( const_cast<TCHAR*>( paramList[i].c_str() ) );
                out->puts( _T("\n") );
            }
        }
    }

    return &false_value;
}

/**
 * Overriding the internal get_props_vf that maxscript calls.
 * Do not directly call this function.
 *
 * @param  arg_list  set by maxscript
 * @param  count     set by maxscript
 * @return           maxscript protected return value
 */
Value* property_container::get_props_vf( Value** /*arg_list*/, int /*count*/ ) {
    if( m_accessor ) {
        std::vector<frantic::tstring> paramList = m_accessor->list_maxscript_properties();

        // create an max array that will hold our property names
        one_typed_value_local( Array * result );
        vl.result = new Array( (int)paramList.size() );

        for( unsigned int i = 0; i < paramList.size(); ++i )
            vl.result->append( Name::intern( const_cast<TCHAR*>( paramList[i].c_str() ) ) );

        return_value( vl.result );
    }
    return &undefined;
}

} // namespace maxscript
} // namespace max3d
} // namespace frantic
