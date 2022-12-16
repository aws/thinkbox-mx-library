// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/max3d/channels/property_map_interop.hpp>
#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/maxscript/maxscript.hpp>
#include <frantic/max3d/units.hpp>

using namespace std;
using namespace boost;
using namespace frantic::channels;
using namespace frantic::graphics;

namespace frantic {
namespace max3d {
namespace channels {

void get_mxs_parameters( Value* v, TimeValue /*t*/, bool /*convertToMeters*/,
                         frantic::channels::property_map& outProps ) {
    if( !is_array( v ) )
        throw std::runtime_error( "Expected an array in get_array_parameters()" );

    frantic::channels::channel_map map;

    Array* arr = static_cast<Array*>( v );
    for( int i = 0; i < arr->size; ++i ) {
        if( !is_array( arr->data[i] ) )
            throw std::runtime_error( "Expected an array of size 2 as element " +
                                      boost::lexical_cast<std::string>( i ) );

        Array* pair = static_cast<Array*>( arr->data[i] );
        if( pair->size != 2 )
            throw std::runtime_error( "Expected an array of size 2 as element " +
                                      boost::lexical_cast<std::string>( i ) );

        if( !is_string( pair->data[0] ) )
            throw std::runtime_error( "Expected a string as element 1 of pair " +
                                      boost::lexical_cast<std::string>( i ) );

        frantic::tstring propName = pair->data[0]->to_string();
        if( is_integer( pair->data[1] ) )
            map.define_channel<boost::int32_t>( propName );
        else if( is_integer64( pair->data[1] ) )
            map.define_channel<boost::int64_t>( propName );
        else if( is_float( pair->data[1] ) )
            map.define_channel<float>( propName );
        else if( is_double( pair->data[1] ) )
            map.define_channel<double>( propName );
        else if( is_point2( pair->data[1] ) )
            map.define_channel( propName, 2, frantic::channels::data_type_float32 );
        else if( is_point3( pair->data[1] ) || is_color( pair->data[1] ) )
            map.define_channel( propName, 3, frantic::channels::data_type_float32 );
        else if( is_point4( pair->data[1] ) || is_quat( pair->data[1] ) )
            map.define_channel( propName, 4, frantic::channels::data_type_float32 );
        else
            throw std::runtime_error( "Unknown value type in property " + frantic::strings::to_string( propName ) );
    }
    map.end_channel_definition();

    // Create a temporary property_map using the newly created channel_map.
    frantic::channels::property_map tempProps;
    tempProps.set_channel_map_with_swap( map );

    for( int i = 0; i < arr->size; ++i ) {
        Array* pair = static_cast<Array*>( arr->data[i] );

        frantic::tstring propName = pair->data[0]->to_string();
        if( is_integer( pair->data[1] ) )
            tempProps.get<boost::int32_t>( propName ) = pair->data[1]->to_int();
        else if( is_integer64( pair->data[1] ) )
            tempProps.get<boost::int64_t>( propName ) = pair->data[1]->to_int64();
        else if( is_float( pair->data[1] ) )
            tempProps.get<float>( propName ) = pair->data[1]->to_float();
        else if( is_double( pair->data[1] ) )
            tempProps.get<double>( propName ) = pair->data[1]->to_double();
        else if( is_point2( pair->data[1] ) )
            tempProps.get<frantic::graphics2d::vector2f>( propName ) = from_max_t( pair->data[1]->to_point2() );
        else if( is_point3( pair->data[1] ) || is_color( pair->data[1] ) )
            tempProps.get<frantic::graphics::vector3f>( propName ) = from_max_t( pair->data[1]->to_point3() );
        else if( is_point4( pair->data[1] ) ) {
            Point4 p = pair->data[1]->to_point4();
            tempProps.get<frantic::graphics::vector4f>( propName ) = frantic::graphics::vector4f( p.x, p.y, p.z, p.w );
        } else if( is_quat( pair->data[1] ) ) {
            Quat q = pair->data[1]->to_quat();
            tempProps.get<frantic::graphics::vector4f>( propName ) = frantic::graphics::vector4f( q.x, q.y, q.z, q.w );
        } else
            throw std::runtime_error( "Unknown value type in property " + frantic::strings::to_string( propName ) );
    }

    outProps.swap( tempProps );
}

void get_pblock2_parameters( ReferenceTarget* r, TimeValue t, bool convertToMeters,
                             frantic::channels::property_map& outProps ) {
    // First go through all the parameters, and make the channel map.
    channel_map cm;
    for( int i = 0; i < r->NumRefs(); ++i ) {
        if( r->GetReference( i ) == 0 )
            continue;

        if( r->GetReference( i )->ClassID() == Class_ID( PARAMETER_BLOCK2_CLASS_ID, 0 ) ) {
            IParamBlock2* p = static_cast<IParamBlock2*>( r->GetReference( i ) );

            // Get the paramblock local name, which oddly can sometimes be NULL.
            // TODO: Maybe this check isn't necessary?
            const TCHAR* localName = p->GetLocalName();
            if( localName == 0 ) {
                localName = _T("");
                continue;
            }

            for( int j = 0; j < p->NumParams(); ++j ) {
                ParamID id = p->IndextoID( j );

                ParamDef& def = p->GetParamDef( id );

                // If the ParamDef is NULL, or the parameter name is NULL, then there's something wrong with the
                // ParamBlock2. But we still continue merrily along...
                if( &def == 0 || def.int_name == 0 ) {
                    continue;
                }
                frantic::tstring paramName = def.int_name;

                switch( (int)def.type ) {
                case TYPE_FLOAT:
                case TYPE_WORLD: // TODO: World types needs special unit-based treatment
                    cm.define_channel<float>( paramName );
                    break;
                case TYPE_INT:
                case TYPE_BOOL:
                    cm.define_channel<int>( paramName );
                    break;
                case TYPE_RGBA:
                    cm.define_channel<color3f>( paramName );
                    break;
                case TYPE_POINT3:
                    cm.define_channel<vector3f>( paramName );
                    break;
                case TYPE_STRING:
                case TYPE_FILENAME:
                    cm.define_channel<frantic::tstring>( paramName );
                    break;
                case TYPE_INODE:
                    // In the scene source abstraction, node handles are strings.  That's
                    // why we treat the INode as a string type here.
                    cm.define_channel<frantic::tstring>( paramName );
                    break;
                }
            }
        }
    }
    cm.end_channel_definition();

    // Create a temporary property_map using the newly created channel_map.
    property_map tempProps;
    tempProps.set_channel_map_with_swap( cm );

    // Loop through all the paramblock2's again, this time retrieving the property values.
    for( int i = 0; i < r->NumRefs(); ++i ) {
        if( r->GetReference( i ) == 0 )
            continue;

        if( r->GetReference( i )->ClassID() == Class_ID( PARAMETER_BLOCK2_CLASS_ID, 0 ) ) {
            IParamBlock2* p = static_cast<IParamBlock2*>( r->GetReference( i ) );

            // Get the paramblock local name, which oddly can sometimes be NULL.
            // TODO: Maybe this check isn't necessary?
            const TCHAR* localName = p->GetLocalName();
            if( localName == 0 ) {
                localName = _T("");
                continue;
            }

            for( int j = 0; j < p->NumParams(); ++j ) {
                ParamID id = p->IndextoID( j );

                ParamDef& def = p->GetParamDef( id );

                // If the ParamDef is NULL, or the parameter name is NULL, then there's something wrong with the
                // ParamBlock2. But we still continue merrily along...
                if( &def == 0 || def.int_name == 0 ) {
                    continue;
                }
                frantic::tstring paramName = def.int_name;

                Interval ivl = FOREVER;
                float floatValue;
                int intValue;
                Color colorValue;
                Point3 point3Value;
                TimeValue timeValueValue;
                INode* inodeValue;
                const TCHAR* stringValue;
                switch( (int)def.type ) {
                case TYPE_FLOAT:
                    p->GetValue( id, t, floatValue, ivl );
                    tempProps.get<float>( paramName ) = floatValue;
                    break;
                case TYPE_WORLD: // TODO: World types needs special unit-based treatment beyond meters or scene units.
                    p->GetValue( id, t, floatValue, ivl );
                    tempProps.get<float>( paramName ) =
                        float( ( convertToMeters ? frantic::max3d::get_scale_to_meters() : 1 ) * floatValue );
                    break;
                case TYPE_INT:
                case TYPE_BOOL:
                    p->GetValue( id, t, intValue, ivl );
                    tempProps.get<int>( paramName ) = intValue;
                    break;

                case TYPE_RGBA:
                    p->GetValue( id, t, colorValue, ivl );
                    tempProps.get<color3f>( paramName ) = color3f( colorValue.r, colorValue.g, colorValue.b );
                    break;
                case TYPE_POINT3:
                    p->GetValue( id, t, point3Value, ivl );
                    tempProps.get<vector3f>( paramName ) = vector3f( point3Value.x, point3Value.y, point3Value.z );
                    break;
                case TYPE_STRING:
                case TYPE_FILENAME:
                    stringValue = p->GetStr( id, t );
                    if( stringValue == 0 )
                        tempProps.get<frantic::tstring>( paramName ) = _T("");
                    else
                        tempProps.get<frantic::tstring>( paramName ) = stringValue;
                    break;
                case TYPE_INODE:
                    // In the scene source abstraction, node handles are strings.  That's
                    // why we treat the INode as a string type here.
                    p->GetValue( id, t, inodeValue, ivl );
                    if( inodeValue == 0 )
                        tempProps.get<frantic::tstring>( paramName ) = _T("0");
                    else
                        tempProps.get<frantic::tstring>( paramName ) =
                            boost::lexical_cast<frantic::tstring>( inodeValue->GetHandle() );
                    break;
                }
            }
        }
    }

    // Swap the fully created property_map into outProps
    outProps.swap( tempProps );
}

void get_callback_parameters( ReferenceTarget* r, TimeValue t, bool convertToMeters,
                              frantic::channels::property_map& outProps ) {
    mxs::frame<3> f;

    // Create the script to get the callback properties.  This is structured to allow any errors in the
    // script to propagate as an exception as much as possible.
    frantic::tstring script = frantic::strings::to_tstring(
        std::string() +
        "(local hasProps = false\n"
        "try( hasProps = ((classof CallbackObject.PropertyCallback) == MAXScriptFunction) ) catch()\n"
        "if hasProps then (\n"
        "  CallBackObject.PropertyCallback " +
        ( convertToMeters ? "true" : "false" ) +
        "\n"
        ") else (\n"
        "  #()\n"
        "))\n" );

    mxs::local<Value> v( f );
    mxs::local<Array> a( f ), suba( f );

    v = mxs::expression( script ).bind( _T("CallbackObject"), r ).at_time( t ).evaluate<Value*>();

    // The returned array should be an array of #(string, value) pairs for the properties.
    if( !v->is_kind_of( class_tag( Array ) ) )
        throw runtime_error(
            "max3d::get_callback_parameters() - The PropertyCallback function returned a non-array:\n" +
            frantic::strings::to_string( mxs::to_string( v ) ) );
    a = static_cast<Array*>( v.ptr() );

    vector<frantic::tstring> propNames( a->size );

    // Now do a first pass through the array of properties, building up the channel_map.
    channel_map cm;
    for( int i = 0; i < a->size; ++i ) {
        // Get the array element, and confirm that it's a pair of two elements
        v = a->get( i + 1 );
        if( !v->is_kind_of( class_tag( Array ) ) )
            throw runtime_error( "max3d::get_callback_parameters() - The PropertyCallback function returned a non "
                                 "property-pair in its array:\n" +
                                 frantic::strings::to_string( mxs::to_string( a->get( i + 1 ) ) ) );
        suba = static_cast<Array*>( v.ptr() );
        if( suba->size != 2 )
            throw runtime_error( "max3d::get_callback_parameters() - The PropertyCallback function returned a non "
                                 "property-pair in its array:\n" +
                                 frantic::strings::to_string( mxs::to_string( a->get( i + 1 ) ) ) );

        // Get the name of the property
        v = suba->get( 1 );
        if( !v->is_kind_of( class_tag( String ) ) )
            throw runtime_error( "max3d::get_callback_parameters() - The PropertyCallback function returned a non "
                                 "property-pair in its array:\n" +
                                 frantic::strings::to_string( mxs::to_string( a->get( i + 1 ) ) ) );
        propNames[i] = v->to_string();

        // Get type type of the property value
        v = suba->get( 2 );
        if( v->is_kind_of( class_tag( Float ) ) ) {
            cm.define_channel<float>( propNames[i] );
        } else if( v->is_kind_of( class_tag( Double ) ) ) {
            cm.define_channel<double>( propNames[i] );
        } else if( v->is_kind_of( class_tag( Integer ) ) ) {
            cm.define_channel<int>( propNames[i] );
        } else if( v->is_kind_of( class_tag( Integer64 ) ) ) {
            cm.define_channel<int64_t>( propNames[i] );
        } else if( v->is_kind_of( class_tag( Point3Value ) ) ) {
            cm.define_channel<vector3f>( propNames[i] );
        } else if( v->is_kind_of( class_tag( ColorValue ) ) ) {
            cm.define_channel<color3f>( propNames[i] );
        } else if( v->is_kind_of( class_tag( Matrix3Value ) ) ) {
            cm.define_channel<transform4f>( propNames[i] );
        } else if( v->is_kind_of( class_tag( MAXNode ) ) ) {
            // In the Flood scene source abstraction, nodes handles are strings, which is why this is converted to a
            // string.
            cm.define_channel<frantic::tstring>( propNames[i] );
        } else if( v->is_kind_of( class_tag( String ) ) ) {
            cm.define_channel<frantic::tstring>( propNames[i] );
        } else {
            throw runtime_error( "max3d::get_callback_parameters() - The PropertyCallback function returned an "
                                 "unrecognized value type in its array:\n" +
                                 frantic::strings::to_string( mxs::to_string( a->get( i + 1 ) ) ) );
        }
    }
    cm.end_channel_definition();

    // Create a temporary property_map using the newly created channel_map.
    property_map tempProps;
    tempProps.set_channel_map_with_swap( cm );

    // Now do a second pass through the array of properties, populating the property_map with values.
    for( int i = 0; i < a->size; ++i ) {
        // Get the array element, no need to check that it's array again, because the previous loop did that
        suba = static_cast<Array*>( a->get( i + 1 ) );

        // Get type type of the property value
        v = suba->get( 2 );
        if( v->is_kind_of( class_tag( Float ) ) ) {
            tempProps.get<float>( propNames[i] ) = v->to_float();
        } else if( v->is_kind_of( class_tag( Double ) ) ) {
            tempProps.get<double>( propNames[i] ) = v->to_double();
        } else if( v->is_kind_of( class_tag( Integer ) ) ) {
            tempProps.get<int>( propNames[i] ) = v->to_int();
        } else if( v->is_kind_of( class_tag( Integer64 ) ) ) {
            tempProps.get<int64_t>( propNames[i] ) = v->to_int64();
        } else if( v->is_kind_of( class_tag( Point3Value ) ) ) {
            tempProps.get<vector3f>( propNames[i] ) = from_max_t( v->to_point3() );
        } else if( v->is_kind_of( class_tag( ColorValue ) ) ) {
            AColor c = v->to_acolor();
            tempProps.get<color3f>( propNames[i] ) = color3f( c.r, c.g, c.b );
        } else if( v->is_kind_of( class_tag( Matrix3Value ) ) ) {
            tempProps.get<transform4f>( propNames[i] ) = frantic::max3d::from_max_t( v->to_matrix3() );
        } else if( v->is_kind_of( class_tag( MAXNode ) ) ) {
            // In the Flood scene source abstraction, nodes handles are strings, which is why this is converted to a
            // string.
            INode* node = v->to_node();
            if( node == 0 )
                tempProps.get<frantic::tstring>( propNames[i] ) = _T("");
            else
                tempProps.get<frantic::tstring>( propNames[i] ) = lexical_cast<frantic::tstring>( node->GetHandle() );
        } else if( v->is_kind_of( class_tag( String ) ) ) {
            tempProps.get<frantic::tstring>( propNames[i] ) = v->to_string();
        }
    }

    // Swap the fully created property_map into outProps
    outProps.swap( tempProps );
}

void get_object_parameters( ReferenceTarget* r, TimeValue t, bool convertToMeters,
                            frantic::channels::property_map& outProps ) {
    // Get the pblock2 and callback parameters
    property_map propsA, propsB;
    get_pblock2_parameters( r, t, convertToMeters, propsA );
    get_callback_parameters( r, t, convertToMeters, propsB );
    // If the callback parameters are non-empty, merge them with the pblock2 parameters.
    if( !propsB.empty() )
        propsA.merge_property_map( propsB );
    // Swap the resulting parameters into outProps
    outProps.swap( propsA );
}

} // namespace channels
} // namespace max3d
} // namespace frantic
