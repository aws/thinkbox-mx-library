// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <map>
#include <string>

#include <frantic/max3d/fpwrapper/function_typededuce.hpp>
#include <frantic/max3d/fpwrapper/max_typetraits.hpp>
#include <frantic/max3d/fpwrapper/memfun_dispatcher.hpp>

// The purpose of this header is to wrap the 3dsmax mixin FP interface into a prettier package

namespace frantic {
namespace max3d {
namespace fpwrapper {

class PropertyVarArgsAdder {
  protected:
    FunctionID m_fnGetID, m_fnSetID;
    frantic::tstring m_name;
    ParamType2 m_paramType;

  public:
    PropertyVarArgsAdder( const PropertyVarArgsAdder& rhs ) {
        m_name = rhs.m_name;
        m_fnGetID = rhs.m_fnGetID;
        m_fnSetID = rhs.m_fnSetID;
        m_paramType = rhs.m_paramType;
    }

    PropertyVarArgsAdder( const frantic::tstring& name, FunctionID fnGetID, FunctionID fnSetID, ParamType2 paramType ) {
        m_name = name;
        m_fnGetID = fnGetID;
        m_fnSetID = fnSetID;
        m_paramType = paramType;
    }

    void add_descriptor_varargs( make_varargs& va ) {
        va.add_values( m_fnGetID, m_fnSetID, m_name.c_str(), 0, m_paramType );
    }

    const frantic::tstring& get_name() { return m_name; }
};

// Declare this class so FFInterfaceWrapper can be friends with it
template <class MainClass>
class FFCreateDescriptorImpl;

// The main class, deriving from FPMixinInterface or FPStaticInterface
template <class MainClass>
class FFInterfaceWrapper {
    // The types for dispatching functions
    typedef FPDispatcher<MainClass>* dispatcher_type;
    typedef std::map<FunctionID, dispatcher_type> fn_map_type;

    bool m_descriptorCreated; // Flag used to ensure the FFCreateDescriptorImpl<MainClass> sequence only happens once
    fn_map_type m_functions;

  public:
    friend class FFCreateDescriptorImpl<MainClass>;

    // Specify which class should be used to create the descriptor
    typedef FFCreateDescriptorImpl<MainClass> FFCreateDescriptor;

    FFInterfaceWrapper() { m_descriptorCreated = false; }
    ~FFInterfaceWrapper() {
        for( std::map<FunctionID, dispatcher_type>::iterator i = m_functions.begin(); i != m_functions.end(); ++i ) {
            delete i->second;
            i->second = 0;
        }
    }

    void make_descriptor_varargs( FFCreateDescriptor& ffcd, make_varargs& va ) {
        assert( !m_descriptorCreated );

        // Add all the function descriptor info
        for( fn_map_type::iterator i = m_functions.begin(); i != m_functions.end(); ++i ) {
            // Exclude the functions which are named "", because those belong to properties
            if( i->second->get_name() != _T("") )
                i->second->add_descriptor_varargs( va );
        }

        if( ffcd.m_properties.size() > 0 ) {
            va.add_values( ::properties );
            for( unsigned i = 0; i < ffcd.m_properties.size(); ++i ) {
                ffcd.m_properties[i].add_descriptor_varargs( va );
            }
        }
#if MAX_VERSION_MAJOR >= 15
        va.add_values( ::p_end );
#else
        va.add_values( ::end );
#endif
    }

    // This is the function which dispatches the call
    FPStatus _dispatch_fn( MainClass* mainObj, FunctionID fid, TimeValue t, FPValue& result, FPParams* p ) {
        fn_map_type::iterator i = m_functions.find( fid );
        if( i != m_functions.end() ) {
            // Call the FPDispatcher to generate the function call
            ( *i ).second->dispatch( mainObj, t, result, p );
            return FPS_OK;
        } else {
            return FPS_NO_SUCH_FUNCTION;
        }
    }

    // This is the function which gets the descriptor
    FPInterfaceDesc* GetDesc() { return this; }
};

template <class MainClass>
class FFCreateDescriptorImpl {
    make_varargs m_descriptorArgs;
    FunctionID m_nextFunctionID;

    // Checks the ID
    FunctionID check_functionid( FunctionID fnID = FP_NO_FUNCTION ) {
        if( fnID == FP_NO_FUNCTION )
            fnID = m_nextFunctionID++;
        assert( fnID != FP_NO_FUNCTION && m_functions->find( fnID ) == m_functions->end() );

        return fnID;
    }

    // The types for specifying properties
    typedef std::vector<PropertyVarArgsAdder> property_list_type;
    property_list_type m_properties;

    // A reference to the functions array in the FFInterfaceWrapper class
    typename FFInterfaceWrapper<MainClass>::fn_map_type* m_functions;
    typedef typename FFInterfaceWrapper<MainClass>::dispatcher_type dispatcher_type;

    MainClass* m_mainObj;
    Interface_ID m_interfaceID;
    frantic::tstring m_interfaceName;
    ClassDesc* m_classDesc;

    // This class should only be created once, and no copies of it should ever be made
    FFCreateDescriptorImpl( const FFCreateDescriptorImpl& ) {}

  public:
    // Let the interface wrapper see the m_properties variable
    friend class FFInterfaceWrapper<MainClass>;

    // The main class is derived from either FFMixinInterface< MainClass > or FFStaticInterface< MainClass >,
    // each of which is friends with FFCreateDescriptorImpl< MainClass >, and has a private member m_FFInterfaceWrapper
    // containing the FFInterfaceWrapper< MainClass >
    FFCreateDescriptorImpl( MainClass* maxObj, const Interface_ID& interfaceID, const frantic::tstring& interfaceName,
                            ClassDesc* classDesc )
        : m_mainObj( maxObj ) {
        //		bool descriptorAlreadyCreated = m_mainObj->m_FFInterfaceWrapper.m_descriptorCreated;
        //		assert( !descriptorAlreadyCreated );

        if( interfaceID == Interface_ID() ) {
            frantic::tstring message =
                _T("FFCreateDescriptor() - The interface ID for the function publishing descriptor \"") +
                interfaceName +
                _T("\" was set to the default value, its value must be set to a randomly selected constant value.");
            LogSys* log = GetCOREInterface()->Log();
            log->LogEntry( SYSLOG_ERROR, DISPLAY_DIALOG, _T("FF Function Publishing Wrapper"), _T("%s"),
                           message.c_str() );
            throw std::runtime_error( frantic::strings::to_string( message ) );
        }

        m_functions = &m_mainObj->m_FFInterfaceWrapper.m_functions;

        assert( m_functions != 0 );

        m_interfaceID = interfaceID;
        m_interfaceName = interfaceName;
        m_classDesc = classDesc;
        m_nextFunctionID = 0;
    }

    ~FFCreateDescriptorImpl() {
        // Call the function in either FFMixinInterface< MainClass > or FFStaticInterface< MainClass >, which creates
        // the descriptor in the appropriate place
        //		bool descriptorAlreadyCreated = m_mainObj->m_FFInterfaceWrapper.m_descriptorCreated;
        //		assert( !descriptorAlreadyCreated );
        m_mainObj->FinalizeFFInterfaceWrapper( *this );
        // Now the descriptor has been created
        m_mainObj->m_FFInterfaceWrapper.m_descriptorCreated = true;
    }

    Interface_ID GetInterfaceID() const { return m_interfaceID; }

    frantic::tstring GetInterfaceName() const { return m_interfaceName; }

    ClassDesc* GetClassDesc() const { return m_classDesc; }

    // Read|Write property
    // TODO: use some kind of static assertion to make sure that FnGet and FnSet are of the correct type
    template <class FnGet, class FnSet>
    void add_property( FnGet fnGet, FnSet fnSet, const frantic::tstring& name ) {
        // deduce the property type from the get function
        typedef RemoveConstRef<DeduceFnArguments<FnGet, 0>::type>::type PropertyType;

        FunctionID fnGetID = check_functionid();
        FunctionID fnSetID = check_functionid();
        // Add the get function, which may or may not be time varying, depending on whether it has a
        // FPTimeValue parameter
        dispatcher_type dispatcherGet(
            new FPDispatcherImpl<MainClass, FnGet>( fnGet, fnGetID, _T(""), std::vector<frantic::tstring>() ) );
        ( *m_functions )[fnGetID] = dispatcherGet;

        // Add the set function, which may or may not be time varying, depending on whether it has a
        // FPTimeValue parameter
        dispatcher_type dispatcherSet(
            new FPDispatcherImpl<MainClass, FnSet>( fnSet, fnSetID, _T(""), std::vector<frantic::tstring>() ) );
        ( *m_functions )[fnSetID] = dispatcherSet;

        m_properties.push_back(
            PropertyVarArgsAdder( name, fnGetID, fnSetID, MaxTypeTraits<PropertyType>::type_enum() ) );
    }

    // Read only property
    template <class FnGet>
    void add_property( FnGet fnGet, const frantic::tstring& name ) {
        // deduce the property type from the get function
        typedef RemoveConstRef<DeduceFnArguments<FnGet, 0>::type>::type PropertyType;

        FunctionID fnGetID = check_functionid();
        // add_function0<PropertyType>( fnGet, "", fnGetID );
        //  Add the get function, which may or may not be time varying, depending on whether it has a
        //  FPTimeValue parameter
        dispatcher_type dispatcherGet(
            new FPDispatcherImpl<MainClass, FnGet>( fnGet, fnGetID, _T(""), std::vector<frantic::tstring>() ) );
        ( *m_functions )[fnGetID] = dispatcherGet;

        m_properties.push_back(
            PropertyVarArgsAdder( name, fnGetID, FP_NO_FUNCTION, MaxTypeTraits<PropertyType>::type_enum() ) );
    }

    template <class FnT>
    void
    add_function( FnT fn, const frantic::tstring& name, const frantic::tstring& paramName0 = _T("Param1"),
                  const frantic::tstring& paramName1 = _T("Param2"), const frantic::tstring& paramName2 = _T("Param3"),
                  const frantic::tstring& paramName3 = _T("Param4"), const frantic::tstring& paramName4 = _T("Param5"),
                  const frantic::tstring& paramName5 = _T("Param6"), const frantic::tstring& paramName6 = _T("Param7"),
                  const frantic::tstring& paramName7 = _T("Param8"), FunctionID fnID = FP_NO_FUNCTION ) {
        fnID = check_functionid( fnID );

        std::vector<frantic::tstring> paramNames;
        paramNames.push_back( paramName0 );
        paramNames.push_back( paramName1 );
        paramNames.push_back( paramName2 );
        paramNames.push_back( paramName3 );
        paramNames.push_back( paramName4 );
        paramNames.push_back( paramName5 );
        paramNames.push_back( paramName6 );
        paramNames.push_back( paramName7 );
        dispatcher_type dispatcher( new FPDispatcherImpl<MainClass, FnT>( fn, fnID, name, paramNames ) );
        ( *m_functions )[fnID] = dispatcher;
    }

    // use this method when creating a maxscript function that takes more than 8 parameters
    template <class FnT>
    void add_function( FnT fn, const frantic::tstring& name, const std::vector<frantic::tstring>& paramNames,
                       FunctionID fnID = FP_NO_FUNCTION ) {
        fnID = check_functionid( fnID );
        dispatcher_type dispatcher( new FPDispatcherImpl<MainClass, FnT>( fn, fnID, name, paramNames ) );
        ( *m_functions )[fnID] = dispatcher;
    }
};

} // namespace fpwrapper
} // namespace max3d
} // namespace frantic
