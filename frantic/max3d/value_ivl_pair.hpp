// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <ctime>
#include <map>
#include <string>

#include <frantic/max3d/units.hpp>

namespace frantic {
namespace max3d {

typedef std::pair<frantic::tstring, Interval> value_ivl_t;

typedef std::map<frantic::tstring, value_ivl_t> value_ivl_map_t;

#ifdef FRANTIC_USING_DOTNET
inline System::Collections::Hashtable* to_Hashtable( const frantic::max3d::value_ivl_map_t& aMap ) {
    frantic::max3d::value_ivl_map_t::const_iterator i;

    System::Collections::Hashtable* result = __gc new System::Collections::Hashtable();

    for( i = aMap.begin(); i != aMap.end(); ++i ) {
        System::String* str = __gc new System::String( i->second.first.c_str() );
        Exocortex::Time::IntegerTimeInterval* interval = new Exocortex::Time::IntegerTimeInterval(
            Exocortex::Time::IntegerTime::FromTicks( i->second.second.Start() ),
            Exocortex::Time::IntegerTime::FromTicks( i->second.second.End() ) );
        System::Object* thePair __gc[] = new System::Object*[2];
        thePair[0] = str;
        thePair[1] = interval;
        result->Add( __gc new System::String( i->first.c_str() ), thePair );
    }
    return result;
}
#endif

namespace detail {
template <class T, class S>
class value_ivl_ctr {
  public:
    inline static value_ivl_t get_value_ivl( S* theObject, int paramId, TimeValue time ) {
        T result;
        Interval ivalid;
        if( !theObject->pblock2->GetValue( paramId, time, result, ivalid ) )
            throw runtime_error( "Unable to get parameter " + boost::lexical_cast<string>( paramId ) +
                                 " from 3dsmax object" );

        return value_ivl_t( boost::lexical_cast<string>( result ), ivalid );
    }

    inline static value_ivl_t get_value_ivl_scale( S* theObject, int paramId, TimeValue time, T scale ) {
        T result;
        Interval ivalid;
        if( !theObject->pblock2->GetValue( paramId, time, result, ivalid ) )
            throw runtime_error( "Unable to get parameter " + boost::lexical_cast<string>( paramId ) +
                                 " from 3dsmax object" );

        result *= scale;

        return value_ivl_t( boost::lexical_cast<string>( result ), ivalid );
    }
};

template <class S>
class value_ivl_ctr<bool, S> {
  public:
    inline static value_ivl_t get_value_ivl( S* theObject, int paramId, TimeValue time ) {
        int result;
        Interval ivalid;
        if( theObject == 0 )
            throw runtime_error( "Unable to get parameter " + boost::lexical_cast<string>( paramId ) +
                                 " from 3dsmax object because object was null" );
        if( theObject->pblock2 == 0 )
            throw runtime_error( "Unable to get parameter " + boost::lexical_cast<string>( paramId ) +
                                 " from 3dsmax object because the object's pblock2 was null" );
        if( !theObject->pblock2->GetValue( paramId, time, result, ivalid ) )
            throw runtime_error( "Unable to get parameter " + boost::lexical_cast<string>( paramId ) +
                                 " from 3dsmax object" );

        return value_ivl_t( boost::lexical_cast<string>( result != 0 ), ivalid );
    }
};

template <class S>
class value_ivl_ctr<std::string, S> {
  public:
    inline static value_ivl_t get_value_ivl( S* theObject, int paramId, TimeValue time ) {
        std::string result;
        TCHAR* ret = theObject->pblock2->GetStr( paramId );
        if( ret == NULL )
            throw runtime_error( "Unable to get parameter " + boost::lexical_cast<string>( paramId ) +
                                 " from 3dsmax object" );

        result = ret;

        return value_ivl_t( result, FOREVER );
    }
};
} // namespace detail

template <class T, class S>
value_ivl_t get_value_ivl( S* theObject, int paramId, TimeValue time ) {
    return detail::value_ivl_ctr<T, S>::get_value_ivl( theObject, paramId, time );
}

template <class T, class S>
value_ivl_t get_value_ivl_scale( S* theObject, int paramId, TimeValue time, T scale ) {
    return detail::value_ivl_ctr<T, S>::get_value_ivl_scale( theObject, paramId, time, scale );
}

template <class T>
value_ivl_t make_value_ivl( const T& value, const Interval ivalid = FOREVER ) {
    return value_ivl_t( boost::lexical_cast<frantic::tstring>( value ), ivalid );
}

inline void add_key_value_pairs( value_ivl_map_t& theMap, const frantic::tstring& text ) {
    using namespace std;

    string::size_type pos = 0;
    while( pos != string::npos ) {
        string::size_type curPos = pos;
        pos = text.find( '\n', curPos );

        frantic::tstring line;
        if( pos == string::npos )
            line = text.substr( curPos );
        else {
            line = text.substr( curPos, pos - curPos );
            ++pos;
        }

        if( line.size() > 0 && line[line.size() - 1] == '\r' )
            line.resize( line.size() - 1 );

        curPos = line.find( '=' );
        if( curPos != string::npos ) {
            frantic::tstring key = line.substr( 0, curPos );
            frantic::tstring value = line.substr( curPos + 1 );

            theMap[key] = make_value_ivl( value );
        }
    }
}

static frantic::tstring pb2_callbackReturnValue;
// Retrieves all the paramblock parameters from an object
inline void add_pblock2_parameters( value_ivl_map_t& theMap, ReferenceMaker* r, TimeValue t, bool convertToMeters,
                                    const int& callbackInputValue = 0 ) {
#ifdef TraceFn
//	TraceFn( "add_pblock2_parameters" );
#else
#define TraceAssert assert
#define TraceVar
#endif

    TraceAssert( r != 0 );

    using namespace std;
    using namespace boost;

    for( int i = 0; i < r->NumRefs(); ++i ) {
        //		TraceVar(i);

        ParamID callbackInputID = -1;

        if( r->GetReference( i ) == 0 )
            continue;

        if( r->GetReference( i )->ClassID() == Class_ID( PARAMETER_BLOCK2_CLASS_ID, 0 ) ) {
            IParamBlock2* p = static_cast<IParamBlock2*>( r->GetReference( i ) );

            const TCHAR* localName = p->GetLocalName();
            if( localName == 0 ) {
                // TraceAssert( localName != 0 );
                localName = _T("");
                // TraceLine( "WARNING!!!! GetLocalName() returned null on a paramblock2" );
                continue;
            }
            //			TraceVar( localName );

            //			TraceVar(p->NumParams());
            for( int j = 0; j < p->NumParams(); ++j ) {
                //				TraceVar(j);
                ParamID id = p->IndextoID( j );

                //				TCHAR* anotherLocalName = p->GetLocalName( id );
                //				if( anotherLocalName == 0 ) {
                //					TraceAssert( anotherLocalName != 0 );
                //					anotherLocalName = "";
                //				}
                //				TraceVar( anotherLocalName );

                ParamDef& def = p->GetParamDef( id );

                // Check if stuff is null
                if( &def == 0 || def.int_name == 0 ) {
                    TraceAssert( &def != 0 );
                    TraceAssert( def.int_name != 0 );
                    continue;
                }

                frantic::tstring paramName = def.int_name;
                frantic::tstring paramValue = _T("");

                //				TraceVar( paramName );

                // Save the callback parameter and return id's if they are of type INTEGER
                if( paramName == _T("inputCallbackValue") && def.type == TYPE_INT ) {
                    callbackInputID = id;
                    continue;
                }

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
                    paramValue = lexical_cast<frantic::tstring>( floatValue );
                    break;
                case TYPE_INT:
                case TYPE_BOOL:
                    p->GetValue( id, t, intValue, ivl );
                    paramValue = lexical_cast<frantic::tstring>( intValue );
                    break;
                case TYPE_RGBA:
                    p->GetValue( id, t, colorValue, ivl );
                    paramValue = lexical_cast<frantic::tstring>( colorValue );
                    break;
                case TYPE_POINT3:
                    p->GetValue( id, t, point3Value, ivl );
                    paramValue = lexical_cast<frantic::tstring>( frantic::max3d::from_max_t( point3Value ) );
                    break;
                case TYPE_WORLD:
                    p->GetValue( id, t, floatValue, ivl );
                    paramValue = lexical_cast<frantic::tstring>(
                        ( convertToMeters ? frantic::max3d::get_scale_to_meters() : 1 ) * floatValue );
                    break;
                case TYPE_TIMEVALUE:
                    p->GetValue( id, t, timeValueValue, ivl );
                    paramValue = lexical_cast<frantic::tstring>( timeValueValue );
                    break;
                case TYPE_STRING:
                case TYPE_FILENAME:
                    stringValue = p->GetStr( id, t );
                    if( stringValue == 0 )
                        paramValue = _T("");
                    else
                        paramValue = stringValue;
                    break;
                case TYPE_INODE:
                    p->GetValue( id, t, inodeValue, ivl );
                    if( inodeValue == 0 )
                        paramValue = _T("0");
                    else
                        paramValue =
                            lexical_cast<frantic::tstring>( inodeValue->GetHandle() ) + _T("/") + inodeValue->GetName();
                    break;
                }

                theMap[paramName] = max3d::make_value_ivl( paramValue, ivl );
            }

            // Now get the additional key/value pairs by calling the script callback
            pb2_callbackReturnValue = _T("");
            if( callbackInputID >= 0 ) {
                // CALLBACK MAGIC EXPLANATION:
                // Setting the value triggers the "on inputCallbackValue set val" event within the scripted
                // plugin.  That plugin sets the "FloodUtil.CallbackReturnValue" or
                // the "FloodSprayUtil.CallbackReturnValue", which then sets the pb2_callbackReturnValue variable,
                // a global static variable.  That's how its value gets set, for the add_key_value_pairs
                // call immediately after.
                p->SetValue( callbackInputID, t, callbackInputValue );
                add_key_value_pairs( theMap, pb2_callbackReturnValue );
            }
        }
    }
}

} // namespace max3d
} // namespace frantic
