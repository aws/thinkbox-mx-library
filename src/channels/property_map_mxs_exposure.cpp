// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <maxversion.h>

#include <frantic/max3d/channels/property_map_mxs_exposure.hpp>
#include <frantic/max3d/maxscript/maxscript.hpp> //for the hack to get around sdk linker errors
#include <frantic/misc/string_functions.hpp>
#include <frantic/strings/tstring.hpp>

using namespace std;
using namespace frantic::strings;
using namespace frantic::channels;
using namespace frantic::graphics;

namespace frantic {
namespace max3d {
namespace channels {

frantic::max3d::channels::property_map_mxs_exposure_meta_class property_map_mxs_exposure_class;

ValueMetaClass* property_map_mxs_exposure::local_base_class() { return &property_map_mxs_exposure_class; }

void property_map_mxs_exposure::sprin1( CharStream* s ) {
    stringstream st;
    st << "Property Map:\n";
    // TODO: write property_map::dump( std::basic_ostream<frantic::tchar>& ) and use it here
    m_props.dump( st );
#ifdef _UNICODE
    s->puts( frantic::strings::to_wstring( st.str() ).c_str() );
#else
    s->puts( const_cast<char*>( st.str().c_str() ) );
#endif
}

void property_map_mxs_exposure::build_lower_to_prop_case() {
    m_lowerToPropCase.clear();
    const channel_map& cm = m_props.get_channel_map();
    for( size_t i = 0; i < cm.channel_count(); ++i ) {
        m_lowerToPropCase[to_lower( cm[i].name() )] = cm[i].name();
    }
}

Value* channel_to_value( const char* data, data_type_t dataType ) {
    switch( dataType ) {
    case data_type_int8:
        return new( GC_IN_HEAP ) Integer( *data );
    case data_type_int16:
        return new( GC_IN_HEAP ) Integer( *reinterpret_cast<const boost::int16_t*>( data ) );
    case data_type_int32:
        return new( GC_IN_HEAP ) Integer( *reinterpret_cast<const boost::int32_t*>( data ) );
    case data_type_int64:
        return new( GC_IN_HEAP ) Integer64( *reinterpret_cast<const boost::int64_t*>( data ) );
    case data_type_uint8:
        return new( GC_IN_HEAP ) Integer( *reinterpret_cast<const boost::uint8_t*>( data ) );
    case data_type_uint16:
        return new( GC_IN_HEAP ) Integer( *reinterpret_cast<const boost::uint16_t*>( data ) );
    case data_type_uint32:
        return new( GC_IN_HEAP ) Integer64( *reinterpret_cast<const boost::uint32_t*>( data ) );
    case data_type_uint64:
        return new( GC_IN_HEAP ) Integer64( *reinterpret_cast<const boost::uint64_t*>( data ) );
    case data_type_float16:
        return new( GC_IN_HEAP ) Float( *reinterpret_cast<const half*>( data ) );
    case data_type_float32:
        return new( GC_IN_HEAP ) Float( *reinterpret_cast<const float*>( data ) );
    case data_type_float64:
        return new( GC_IN_HEAP ) Double( *reinterpret_cast<const double*>( data ) );
    case data_type_string:
        return new( GC_IN_HEAP ) String( frantic::channels::cstring_from_channel_string( data ) );
    default: {
        frantic::tstring msg = frantic::tstring() + _T( "Tried to convert a value of unexpected type " ) +
                               frantic::channels::channel_data_type_str( dataType ) + _T( " to a maxscript type." );
        throw RuntimeError( const_cast<TCHAR*>( msg.c_str() ) );
    }
    }
}

Value* channel_to_value( const char* data, std::size_t arity, data_type_t dataType ) {
    frantic::max3d::mxs::frame<1> f;
    frantic::max3d::mxs::local<Array> result( f );

    std::size_t dataSize = frantic::channels::sizeof_channel_data_type( dataType );

    result = new( GC_IN_HEAP ) Array( (int)arity );
    for( std::size_t i = 0; i < arity; ++i )
        result->append( channel_to_value( data + i * dataSize, dataType ) );
#if MAX_VERSION_MAJOR < 19
    return_protected( result.ptr() );
#else
    return_value( result.ptr() );
#endif
}

/**
 * Overriding the internal get_property that maxscript calls.
 * Do not directly call this function.
 *
 * @param  arg_list  set by maxscript
 * @param  count     set by maxscript
 * @return           maxscript protected return value
 */
Value* property_map_mxs_exposure::get_property( Value** arg_list, int count ) {
    Value* retVal = NULL;
    try {
        if( count >= 1 && arg_list[0] ) {
            frantic::tstring inputParam = to_lower( arg_list[0]->to_string() );
            ::map<frantic::tstring, frantic::tstring>::const_iterator i = m_lowerToPropCase.find( inputParam );
            if( i != m_lowerToPropCase.end() ) {
                const frantic::channels::channel& c = m_props.get_channel_map()[i->second];
                const char* data = c.get_channel_data_pointer( m_props.get_raw_buffer() );
                data_type_t dataType = c.data_type();
                size_t arity = c.arity();

                retVal = channel_to_value( data, dataType );
                if( arity > 1 ) {
                    size_t incrementSize = frantic::channels::sizeof_channel_data_type( dataType );
                    Array* array = new( GC_IN_HEAP ) Array( (int)arity );
                    ( *array )[0] = retVal;
                    for( size_t j = 1; j < arity; ++j ) {
                        data += incrementSize;
                        ( *array )[(int)j] = channel_to_value( data, dataType );
                    }
                    retVal = array;
                }
            }
        }
    } catch( std::exception& e ) {
        frantic::tstring msg = frantic::strings::to_tstring( e.what() );
        throw RuntimeError( const_cast<TCHAR*>( msg.c_str() ) );
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
#pragma optimize( "", on )
Value* property_map_mxs_exposure::set_property( Value** arg_list, int count ) {
    bool success = false;
    if( count >= 2 && arg_list[0] && arg_list[1] ) {
        frantic::tstring inputParam = arg_list[1]->to_string();
        // success = m_accessor->set_maxscript_property( inputParam, arg_list[0], MAXScript_time() );
        //  TODO: Not implemented yet!
        success = false;
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
Value* property_map_mxs_exposure::show_props_vf( Value** arg_list, int count ) {
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

    const channel_map& channelMap = m_props.get_channel_map();
    for( unsigned int i = 0; i < channelMap.channel_count(); ++i ) {

        if( hasPropertyCalled ) {
            // hasProperty

#if MAX_RELEASE == 8000
            // problem: i can't link to the provided max_name_match function in the max 8 sdk!! i'm just going to do
            // string compare instead
            if( frantic::strings::to_lower( channelMap[i].name() ) == frantic::strings::to_lower( inputProp ) )
                return &true_value;
#else
            // use the proper max's regular expression matcher
            if( max_name_match( const_cast<TCHAR*>( frantic::strings::to_tstring( channelMap[i].name() ).c_str() ),
                                const_cast<TCHAR*>( inputProp.c_str() ) ) )
                return &true_value;
#endif

        } else if( out ) {
            // print property name
            out->puts( _T("  .") );
            out->puts( const_cast<TCHAR*>( frantic::strings::to_tstring( channelMap[i].name() ).c_str() ) );
            // print type
            out->puts( _T(" : ") );
            out->puts( const_cast<TCHAR*>(
                frantic::strings::to_tstring( channel_data_type_str( channelMap[i].data_type() ) ).c_str() ) );
            out->puts( _T("\n") );
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
Value* property_map_mxs_exposure::get_props_vf( Value** /*arg_list*/, int /*count*/ ) {

    const channel_map& channelMap = m_props.get_channel_map();

    // create an max array that will hold our property names
    one_typed_value_local( Array * result );
    vl.result = new Array( (int)channelMap.channel_count() );

    for( unsigned int i = 0; i < channelMap.channel_count(); ++i )
        vl.result->append(
            Name::intern( const_cast<TCHAR*>( frantic::strings::to_tstring( channelMap[i].name() ).c_str() ) ) );

    return_value( vl.result );
}

} // namespace channels
} // namespace max3d
} // namespace frantic
