// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#pragma warning( push, 3 )
#pragma warning( disable : 4701 4702 4267 )
#include <boost/lexical_cast.hpp>
#pragma warning( pop )

#include <frantic/max3d/iostream.hpp>

#include <frantic/logging/logging_level.hpp>
#include <frantic/max3d/fpwrapper/max_typetraits.hpp>
#include <frantic/max3d/geopipe/get_inodes.hpp>
#include <frantic/max3d/geopipe/object_dumping_help.hpp>
#include <frantic/max3d/maxscript/maxscript.hpp>

// Evil max sdk has a macro called base_type!  In a public header! arg!
#undef base_type
#include <boost/regex.hpp>

// Some functions and classes to help extract parameters from max objects
namespace frantic {
namespace max3d {

/**
 * Gets the parameter name and ID of a parameter in a paramblock
 */
inline bool get_parameter_name_and_id( IParamBlock* p, int parameterIndex, frantic::tstring& outName, int& outParamID,
                                       bool debugPrints = false ) {
    outName = strings::to_lower( (const TCHAR*)p->SubAnimName( p->GetAnimNum( parameterIndex ) ) );
    outParamID = parameterIndex;
    if( debugPrints && outName.empty() ) {
        mprintf( _T("Got empty parameter name from IParamBlock\n") );
    }

    return !outName.empty();
}

/**
 * Gets the parameter name and ID of a parameter in a paramblock2
 */
inline bool get_parameter_name_and_id( IParamBlock2* p, int parameterIndex, frantic::tstring& outName, int& outParamID,
                                       bool debugPrints = false ) {
    ParamID id = p->IndextoID( parameterIndex );

    ParamDef& def = p->GetParamDef( id );

    if( IsBadReadPtr( &def, sizeof( ParamDef ) ) ) {
        if( debugPrints )
            mprintf( _T("%s\n"), ( _T("Got an invalid ParamDef pointer from IParamBlock2: ") +
                                   boost::lexical_cast<frantic::tstring>( (void*)&def ) + _T("\n") )
                                     .c_str() );
        return false;
    } else if( IsBadReadPtr( def.int_name, 1 ) ) {
        if( debugPrints )
            mprintf( _T("%s\n"), ( _T("Got an invalid int_name pointer in ParamDef from IParamBlock2: ") +
                                   boost::lexical_cast<frantic::tstring>( (void*)def.int_name ) + _T("\n") )
                                     .c_str() );
        return false;
    } else {
        // make the outName lowercase for the comparison
        outName = strings::to_lower( def.int_name );
        outParamID = id;

        return true;
    }
}

namespace detail {

template <class ParameterType>
ParameterType get_tab_parameter_from_paramblock( IParamBlock2* p, TimeValue t, const frantic::tstring& parameterName,
                                                 int tabIdx, bool& outSuccess ) {
    frantic::tstring lowerParameterName = frantic::strings::to_lower( parameterName );

    frantic::tstring testParameterName;
    int testParamID;

    for( int parameterIndex = 0; parameterIndex < p->NumParams(); ++parameterIndex ) {
        if( get_parameter_name_and_id( p, parameterIndex, testParameterName, testParamID ) ) {
            testParameterName = frantic::strings::to_lower( testParameterName );
            if( testParameterName == lowerParameterName ) {
                // mprintf("found a matching named parameter\n");
                ParamType2 paramType = (ParamType2)p->GetParameterType( (ParamID)testParamID );

                if( ( paramType & TYPE_TAB ) &&
                    fpwrapper::MaxTypeTraits<ParameterType>::is_compatible_type( paramType & ( ~TYPE_TAB ) ) &&
                    tabIdx < p->Count( (ParamID)testParamID ) ) {
                    // mprintf("the parameter type is compatible\n");
                    ParameterType result;
                    if( fpwrapper::MaxTypeTraits<ParameterType>::FromParamBlock( p, (ParamID)testParamID, t, result,
                                                                                 tabIdx ) ) {
                        // mprintf("Parameter search and extract successful\n");
                        outSuccess = true;
                        return result;
                    } else {
                        // mprintf("Parameter extract failed\n");
                        return ParameterType();
                    }
                }
                // else
                //{
                //
                //	ParamType2 wantedParamType = fpwrapper::MaxTypeTraits<ParameterType>::type_enum();
                //	std::string type = ParamTypeToString(paramType);
                //	std::string wantedType = ParamTypeToString(wantedParamType);

                //	mprintf("Generic param type mismatch.  Wanted: %s, Got: %s\n", wantedType.c_str(),
                //type.c_str());
                //}
            }
        }
    }

    outSuccess = false;
    return ParameterType();
}

// the Generic case...
// Works with either IParamBlock or IParamBlock2 passed in for p
template <class IParamBlockType, class ParameterType>
ParameterType get_parameter_from_paramblock( IParamBlockType* p, TimeValue t, const frantic::tstring& parameterName,
                                             bool& outSuccess ) {
    frantic::tstring lowerParameterName = frantic::strings::to_lower( parameterName );

    frantic::tstring testParameterName;
    int testParamID;

    for( int parameterIndex = 0; parameterIndex < p->NumParams(); ++parameterIndex ) {
        if( get_parameter_name_and_id( p, parameterIndex, testParameterName, testParamID ) ) {
            testParameterName = frantic::strings::to_lower( testParameterName );
            if( testParameterName == lowerParameterName ) {
                // mprintf("found a matching named parameter\n");
                ParamType2 paramType = (ParamType2)p->GetParameterType( (ParamID)testParamID );
                if( fpwrapper::MaxTypeTraits<ParameterType>::is_compatible_type( paramType ) ) {
                    // mprintf("the parameter type is compatible\n");
                    ParameterType result;
                    if( fpwrapper::MaxTypeTraits<ParameterType>::FromParamBlock( p, (ParamID)testParamID, t,
                                                                                 result ) ) {
                        // mprintf("Parameter search and extract successful\n");
                        outSuccess = true;
                        return result;
                    } else {
                        // mprintf("Parameter extract failed\n");
                        return ParameterType();
                    }
                }
                // else
                //{
                //
                //	ParamType2 wantedParamType = fpwrapper::MaxTypeTraits<ParameterType>::type_enum();
                //	std::string type = ParamTypeToString(paramType);
                //	std::string wantedType = ParamTypeToString(wantedParamType);

                //	mprintf("Generic param type mismatch.  Wanted: %s, Got: %s\n", wantedType.c_str(),
                //type.c_str());
                //}
            }
        }
    }

    outSuccess = false;
    return ParameterType();
}

// a class to hold the specializations of the functions
template <class T>
struct parameter_extraction_helper {
    static T get_max_parameter( IParamBlock* p, TimeValue t, const std::string& parameterName, bool& outSuccess ) {
        // return T::unimplemented_parameter_type();
        // mprintf("Attempting to get a generic param from paramblock: %s %s\n", typeid(T()).name(),
        // parameterName.c_str());
        T result = get_parameter_from_paramblock<IParamBlock, T>( p, t, parameterName, outSuccess );
        return result;
    }
    static T get_max_parameter( IParamBlock2* p, TimeValue t, const std::string& parameterName, bool& outSuccess ) {
        // return T::unimplemented_parameter_type();
        // mprintf("Attempting to get a generic param from paramblock2: %s %s\n", typeid(T()).name(),
        // parameterName.c_str());
        return get_parameter_from_paramblock<IParamBlock2, T>( p, t, parameterName, outSuccess );
    }

    static T get_max_tab_parameter( IParamBlock2* p, TimeValue t, const std::string& parameterName, int tabIdx,
                                    bool& outSuccess ) {

        return get_tab_parameter_from_paramblock<T>( p, t, parameterName, tabIdx, outSuccess );
    }

    static T return_parameter_if_valid( Animatable*, bool& outSuccess ) {
        outSuccess = false;
        return T();
    }
};

// Specialized for ReferenceTarget*
template <>
struct parameter_extraction_helper<ReferenceTarget*> {
    static ReferenceTarget* get_max_parameter( IParamBlock* /*p*/, TimeValue /*t*/,
                                               const std::string& /*parameterName*/, bool& outSuccess ) {
        outSuccess = false;
        return 0;
    }
    static ReferenceTarget* get_max_parameter( IParamBlock2* p, TimeValue t, const frantic::tstring& parameterName,
                                               bool& outSuccess ) {
        return get_parameter_from_paramblock<IParamBlock2, ReferenceTarget*>( p, t, parameterName, outSuccess );
    }

    static ReferenceTarget* get_max_tab_parameter( IParamBlock2* p, TimeValue t, const frantic::tstring& parameterName,
                                                   int tabIdx, bool& outSuccess ) {
        return get_tab_parameter_from_paramblock<ReferenceTarget*>( p, t, parameterName, tabIdx, outSuccess );
    }

    static ReferenceTarget* return_parameter_if_valid( Animatable* a, bool& outSuccess ) {
        if( a == 0 ) {
            outSuccess = false;
            return 0;
        }
        // TODO: How do you tell whether an Animatable* is a ReferenceMaker?  This cast is suspect...
        ReferenceMaker* rm = static_cast<ReferenceMaker*>( a );
        if( rm->IsRefTarget() ) {
            outSuccess = true;
            return static_cast<ReferenceTarget*>( rm );
        } else {
            outSuccess = false;
            return 0;
        }
    }
};

template <class T>
inline T get_parameter( ReferenceMaker* r, TimeValue t, const frantic::tstring& parameterName, bool& outSuccess ) {
    // If the object being passed in is a parameter block 2 or a parameter block, take care of that first
    if( r->ClassID() == Class_ID( PARAMETER_BLOCK2_CLASS_ID, 0 ) ) {
        IParamBlock2* p = static_cast<IParamBlock2*>( r );
        return detail::parameter_extraction_helper<T>::get_max_parameter( p, t, parameterName, outSuccess );
    } else if( r->ClassID() == Class_ID( PARAMETER_BLOCK_CLASS_ID, 0 ) ) {
        IParamBlock* p = static_cast<IParamBlock*>( r );
        return detail::parameter_extraction_helper<T>::get_max_parameter( p, t, parameterName, outSuccess );
    }

    T result;

    // First make a map which can convert selected Animatable* to ReferenceTarget* for when we look at the subanims
    std::map<Animatable*, ReferenceTarget*> references;
    for( int index = 0; index < r->NumRefs(); ++index ) {
        ReferenceTarget* refTarg = r->GetReference( index );
        if( refTarg != 0 ) {
            Class_ID cid = refTarg->ClassID();
            if( cid == Class_ID( PARAMETER_BLOCK2_CLASS_ID, 0 ) || cid == Class_ID( PARAMETER_BLOCK_CLASS_ID, 0 ) ) {
                // Treat properties of parameter blocks as if they were properties of the object itself
                result = get_max_parameter<T>( refTarg, t, parameterName, outSuccess );
                if( outSuccess )
                    return result;
            } else
                references[refTarg] = refTarg;
        }
    }

    for( int index = 0; index < r->NumSubs(); ++index ) {
        Animatable* anim = r->SubAnim( index );

        // Convert the Animatable to a ReferenceTarget based on the object's References list
        std::map<Animatable*, ReferenceTarget*>::iterator i = references.find( anim );
        if( i == references.end() )
            continue;

        ReferenceTarget* refTarg = i->second;

        TSTR name = r->SubAnimName( index );

        if( frantic::logging::is_logging_debug() ) {
            TSTR cn;
            refTarg->GetClassName( cn );
            std::cerr << "SubAnim # " << index << " has class id " << refTarg->ClassID() << ", ref name " << (char*)name
                      << std::endl;
        }

        // If the name matches this subanim, return its value
        if( frantic::strings::to_lower( (char*)name ) == frantic::strings::to_lower( parameterName ) ) {
            result = detail::parameter_extraction_helper<T>::return_parameter_if_valid( refTarg, outSuccess );
            if( outSuccess )
                return result;
        }
    }

    outSuccess = false;
    return T();
}

template <class T>
inline T get_tab_parameter( ReferenceMaker* r, TimeValue t, const std::string& parameterName, int tabIdx,
                            bool& outSuccess ) {
    // If the object being passed in is a parameter block 2 or a parameter block, take care of that first
    if( r->ClassID() == Class_ID( PARAMETER_BLOCK2_CLASS_ID, 0 ) ) {
        IParamBlock2* p = static_cast<IParamBlock2*>( r );
        return detail::parameter_extraction_helper<T>::get_max_tab_parameter( p, t, parameterName, tabIdx, outSuccess );
    }
    T result;

    // First make a map which can convert selected Animatable* to ReferenceTarget* for when we look at the subanims
    for( int index = 0; index < r->NumRefs(); ++index ) {
        ReferenceTarget* refTarg = r->GetReference( index );
        if( refTarg != 0 ) {
            Class_ID cid = refTarg->ClassID();
            if( cid == Class_ID( PARAMETER_BLOCK2_CLASS_ID, 0 ) ) {
                // Treat properties of parameter blocks as if they were properties of the object itself
                result = get_tab_parameter<T>( refTarg, t, parameterName, tabIdx, outSuccess );
                if( outSuccess )
                    return result;
            }
        }
    }

    outSuccess = false;
    return T();
}

} // namespace detail

template <class T>
inline T get_max_parameter( ReferenceMaker* r, TimeValue t, const std::string& parameterName, bool& outSuccess ) {
    // Break apart subproperty access via the '.' operator.  This way, the caller of get_max_parameter can pass in
    // "lens.fov" and it will look for the parameter as a subproperty of something called lens.
    std::string::size_type dotPos = parameterName.find( '.' );
    if( dotPos != std::string::npos ) {
        if( frantic::logging::is_logging_debug() )
            std::cerr << "Getting the reference target named " << parameterName.substr( 0, dotPos ) << std::endl;

        // Get an object which could have subproperties (just a reference target for now, could add iobjects later)
        ReferenceTarget* subRefTarget =
            get_max_parameter<ReferenceTarget*>( r, t, parameterName.substr( 0, dotPos ), outSuccess );
        if( !outSuccess )
            return T();

        if( frantic::logging::is_logging_debug() )
            std::cerr << "Now getting the subproperty named " << parameterName.substr( dotPos + 1 ) << std::endl;

        // Recursively get the subproperty of this object
        return get_max_parameter<T>( subRefTarget, t, parameterName.substr( dotPos + 1 ), outSuccess );
    }

    // Don't expose an INode's properties, instead just expose its object properties.  The INode properties are easy
    // to get directly from the INode*, anyway.
    if( r->SuperClassID() == BASENODE_CLASS_ID ) {
        INode* node = static_cast<INode*>( r );
        Object* obj = node->GetObjectRef();

        if( obj == 0 ) {
            outSuccess = false;
            return T();
        }

        // Substitute the object's properties for the INode's properties.
        r = obj;
    }

    // check for an array subscript signifying this is an array parameter we're looking for.
    static boost::regex re( "\\[(\\d+)\\]$" );

    boost::smatch what;
    if( boost::regex_search( parameterName, what, re ) ) {
        std::string newParameterName = parameterName.substr( 0, parameterName.size() - what.str( 0 ).size() );
        int tabIdx = boost::lexical_cast<int>( what.str( 1 ) );
        return detail::get_tab_parameter<T>( r, t, newParameterName, tabIdx, outSuccess );
    } else
        return detail::get_parameter<T>( r, t, parameterName, outSuccess );
}

/**
 * Looks through an object and its parameter blocks for references to inodes whose objects have the given class id
 */
inline void get_inode_references_of_class_id( ReferenceTarget* containerObject, Class_ID cid,
                                              std::set<INode*>& outReferences ) {
    if( containerObject == 0 )
        return;

    // Go through all the references
    for( int i = 0; i < containerObject->NumRefs(); ++i ) {
        ReferenceTarget* ref = containerObject->GetReference( i );
        if( ref->ClassID() == Class_ID( PARAMETER_BLOCK2_CLASS_ID, 0 ) ) {
            // If it's a parameter block 2, recursively get its references
            get_inode_references_of_class_id( ref, cid, outReferences );
        } else if( ref->SuperClassID() == BASENODE_CLASS_ID ) {
            // If it's an INode, check whether its object has the appropriate class id.
            // If it does have the right class id, add the node to the output set.
            INode* node = static_cast<INode*>( ref );
            if( node->GetObjectRef() != 0 && node->GetObjectRef()->ClassID() == cid )
                outReferences.insert( node );
        }
    }
}

/**
 * Recursively searches a reftarget enumerating <type, name> parameter pairs.
 */
inline void collect_all_parameters_recursive( ReferenceTarget* refTarget,
                                              std::vector<std::pair<frantic::tstring, frantic::tstring>>& params,
                                              TimeValue t, const frantic::tstring& prefix = frantic::tstring(),
                                              bool stopAtINode = false ) {
    std::map<ReferenceTarget*, int> processed;
    if( refTarget != 0 && !( stopAtINode && refTarget->SuperClassID() == BASENODE_CLASS_ID ) ) {
        // build a map from animatable to reference targets.
        for( int i = 0; i < refTarget->NumRefs(); ++i ) {
            // mprintf( "%sReference # %d\n", indent.c_str(), i );
            ReferenceTarget* childTarget = refTarget->GetReference( i );
            // Record which targets we got in the processed map
            if( childTarget != 0 ) {
                Class_ID cid = childTarget->ClassID();
                if( cid == Class_ID( PARAMETER_BLOCK2_CLASS_ID, 0 ) ||
                    cid == Class_ID( PARAMETER_BLOCK_CLASS_ID, 0 ) ) {
                    collect_all_parameters_recursive( childTarget, params, t, prefix, true );
                } else
                    processed[childTarget] = i;
            }
        }
        //	 The subanims
        for( int i = 0; i < refTarget->NumSubs(); ++i ) {
            Animatable* childAnim = refTarget->SubAnim( i );
            std::map<ReferenceTarget*, int>::iterator ref_it = processed.find( (ReferenceTarget*)childAnim );
            if( ref_it != processed.end() ) {
                collect_all_parameters_recursive( (ReferenceTarget*)childAnim, params, t,
                                                  prefix + ( (const TCHAR*)refTarget->SubAnimName( i ) ) + _T("."),
                                                  true );
            }
        }
        // Special case the parameters of a parameter block 2
        if( refTarget->ClassID() == Class_ID( PARAMETER_BLOCK2_CLASS_ID, 0 ) ) {
            IParamBlock2* p = static_cast<IParamBlock2*>( refTarget );
            // mprintf( "%sParameter block 2 parameters:\n", indent.c_str() );
            frantic::tstring parameterName;
            int paramID;
            for( int parameterIndex = 0; parameterIndex < p->NumParams(); ++parameterIndex ) {
                if( max3d::get_parameter_name_and_id( p, parameterIndex, parameterName, paramID ) ) {
                    params.push_back(
                        std::make_pair( geopipe::ParamTypeToString( p->GetParameterType( (ParamID)paramID ) ),
                                        prefix + parameterName ) );
                }
            }
        }

        // Special case the parameters of a parameter block 1
        if( refTarget->ClassID() == Class_ID( PARAMETER_BLOCK_CLASS_ID, 0 ) ) {
            IParamBlock* p = static_cast<IParamBlock*>( refTarget );
            // mprintf( "%sParameter block 1 parameters:\n", indent.c_str() );
            frantic::tstring parameterName;
            int paramID;
            for( int parameterIndex = 0; parameterIndex < p->NumParams(); ++parameterIndex ) {
                if( max3d::get_parameter_name_and_id( p, parameterIndex, parameterName, paramID ) ) {
                    params.push_back(
                        std::make_pair( geopipe::ParamTypeToString( (ParamType2)p->GetParameterType( paramID ) ),
                                        prefix + parameterName ) );
                }
            }
        }
    }
}

/**
 * Now, to extract parameters from Maxscript!
 * the script can be any executable maxscript.  The ref name is the name used in the script
 * to refer to the reference. So, to get the parameter "soften" from a material we can call:
 * val = get_mxs_parameter(ref, t, "theMat.soften", "theMat");
 * Cool? cool.
 */
inline Value* get_mxs_parameter_value( ReferenceTarget* ref, TimeValue t, const frantic::tstring& script,
                                       const frantic::tstring& refName, bool& success,
                                       bool /*propagate_all_exceptions*/ = false, CharStream* /*logStream*/ = 0 )

{
    mxs::frame<2> f;
    mxs::local<Value> theRef( f ), result( f );

    theRef = MAXClass::make_wrapper_for( ref );
    result = mxs::expression( script ).bind( refName, ref ).at_time( t ).evaluate<Value*>();

    success = true;
    return result;
}

/**
 * Try to get a typed version of the
 * parameter from the maxscript.
 */
template <typename T>
inline T get_mxs_parameter( ReferenceTarget* ref, TimeValue t, const std::string& script, const std::string& refName,
                            bool& success, bool propagate_all_exceptions = false, CharStream* logStream = 0 ) {
    Value* val = get_mxs_parameter_value( ref, t, script, refName, success, propagate_all_exceptions, logStream );

    // ToDo: This doesn't perform any type-compatibility checking...That might be a good idea.
    if( val != &undefined && success )
        return fpwrapper::MaxTypeTraits<T>::FromValue( val );
    else
        return T();
}

/**
 * This function is intended the intended function for retrieving arbitrary named parameters from a Max object.
 * This functionality should be complimented with maxscript::maxscript_evaluate() for evaluating more complex scripts.
 * My intention for this was to supersede get_mxs_parameter() which is too verbose.
 */
template <class T>
inline T get_parameter( ReferenceTarget* ref, TimeValue t, const frantic::tstring& param ) {
    Value* v = mxs::expression( _T("obj.") + param ).bind( _T("obj"), ref ).at_time( t ).evaluate<Value*>();
    if( v == &undefined )
        throw std::runtime_error( "get_parameter<" + std::string( typeid( T ).name() ) +
                                  ">() failed to get parameter \"" + frantic::strings::to_string( param ) +
                                  "\" because it was undefined" );

    return max3d::fpwrapper::MaxTypeTraits<T>::FromValue( v );
}

template <class T>
inline T try_get_parameter( ReferenceTarget* ref, TimeValue t, const frantic::tstring& param, T _default = T() ) {
    Value* v = mxs::expression( _T("try(obj.") + param + _T(")catch(undefined)") )
                   .bind( _T("obj"), ref )
                   .at_time( t )
                   .evaluate<Value*>();
    if( v == &undefined )
        return _default;

    return max3d::fpwrapper::MaxTypeTraits<T>::FromValue( v );
}

} // namespace max3d
} // namespace frantic
