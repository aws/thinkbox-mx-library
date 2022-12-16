// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <map>

#include <frantic/max3d/fpwrapper/make_varargs.hpp>
#include <frantic/max3d/fpwrapper/max_typetraits.hpp>

namespace frantic {
namespace max3d {
namespace fpwrapper {

// -------------- The main abstract base dispatcher class

// This class calls a function based on the function publishing inputs
template <class MainClass>
class FPDispatcher {
  protected:
    FunctionID m_fid;
    frantic::tstring m_name;
    std::vector<frantic::tstring> m_paramNames;

  public:
    virtual void dispatch( MainClass* obj, TimeValue t, FPValue& result, FPParams* p ) = 0;
    virtual void add_descriptor_varargs( make_varargs& va ) = 0;

    const FunctionID get_fid() { return m_fid; }
    const frantic::tstring& get_name() { return m_name; }
};

// -------------- The classes which handle actually calling the functions

// General dispatcher, does nothing
template <class FnT>
struct FPDispatcherDispatch {};

// dispatcher for - void function();
template <class MainClass>
struct FPDispatcherDispatch<void ( MainClass::* )()> {
    typedef void ( MainClass::*FnT )();

    static const int arity = 0;
    typedef void ReturnType;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue /*t*/, FPValue& /*result*/, FPParams* /*p*/ ) {
        ( obj->*function )();
    }

    inline static void add_descriptor_argument_varargs( make_varargs& /*va*/,
                                                        const std::vector<frantic::tstring>& /*paramNames*/ ) {}
};
// dispatcher for - void function(); with time
template <class MainClass>
struct FPDispatcherDispatch<void ( MainClass::* )( FPTimeValue )> {
    typedef void ( MainClass::*FnT )( FPTimeValue );

    static const int arity = 0;
    typedef void ReturnType;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& /*result*/, FPParams* /*p*/ ) {
        ( obj->*function )( t );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& /*va*/,
                                                        const std::vector<frantic::tstring>& /*paramNames*/ ) {}
};

// dispatcher for - RT function();
template <class MainClass, class RT>
struct FPDispatcherDispatch<RT ( MainClass::* )()> {
    typedef RT ( MainClass::*FnT )();

    static const int arity = 0;
    typedef typename RemoveConstRef<RT>::type ReturnType;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue /*t*/, FPValue& result, FPParams* /*p*/ ) {
        RT value = ( obj->*function )();
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& /*va*/,
                                                        const std::vector<frantic::tstring>& /*paramNames*/ ) {}
};
// dispatcher for - RT function(); with time
template <class MainClass, class RT>
struct FPDispatcherDispatch<RT ( MainClass::* )( FPTimeValue )> {
    typedef RT ( MainClass::*FnT )( FPTimeValue );

    static const int arity = 0;
    typedef typename RemoveConstRef<RT>::type ReturnType;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& result, FPParams* /*p*/ ) {
        RT value = ( obj->*function )( t );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& /*va*/,
                                                        const std::vector<frantic::tstring>& /*paramNames*/ ) {}
};

// dispatcher for - void function( T0 );
template <class MainClass, class T0>
struct FPDispatcherDispatch<void ( MainClass::* )( T0 )> {
    typedef void ( MainClass::*FnT )( T0 );

    static const int arity = 1;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue /*t*/, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )( MaxTypeTraits<Arg0Type>::to_type( p->params[0] ) );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
    }
};
// dispatcher for - void function( T0 ); with time
template <class MainClass, class T0>
struct FPDispatcherDispatch<void ( MainClass::* )( T0, FPTimeValue )> {
    typedef void ( MainClass::*FnT )( T0, FPTimeValue );

    static const int arity = 1;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )( MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), t );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
    }
};

// dispatcher for - RT function( T0 );
template <class MainClass, class RT, class T0>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0 )> {
    typedef RT ( MainClass::*FnT )( T0 );

    static const int arity = 1;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue /*t*/, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )( MaxTypeTraits<Arg0Type>::to_type( p->params[0] ) );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
    }
};
// dispatcher for - RT function( T0 ); with time
template <class MainClass, class RT, class T0>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0, FPTimeValue )> {
    typedef RT ( MainClass::*FnT )( T0, FPTimeValue );

    static const int arity = 1;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )( MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), t );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
    }
};

// dispatcher for - void function( T0, T1 );
template <class MainClass, class T0, class T1>
struct FPDispatcherDispatch<void ( MainClass::* )( T0, T1 )> {
    typedef void ( MainClass::*FnT )( T0, T1 );

    static const int arity = 2;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue /*t*/, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )( MaxTypeTraits<Arg0Type>::to_type( p->params[0] ),
                            MaxTypeTraits<Arg1Type>::to_type( p->params[1] ) );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
    }
};
// dispatcher for - void function( T0, T1 ); with time
template <class MainClass, class T0, class T1>
struct FPDispatcherDispatch<void ( MainClass::* )( T0, T1, FPTimeValue )> {
    typedef void ( MainClass::*FnT )( T0, T1, FPTimeValue );

    static const int arity = 2;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )( MaxTypeTraits<Arg0Type>::to_type( p->params[0] ),
                            MaxTypeTraits<Arg1Type>::to_type( p->params[1] ), t );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
    }
};

// dispatcher for - RT function( T0, T1 );
template <class MainClass, class RT, class T0, class T1>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0, T1 )> {
    typedef RT ( MainClass::*FnT )( T0, T1 );

    static const int arity = 2;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue /*t*/, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )( MaxTypeTraits<Arg0Type>::to_type( p->params[0] ),
                                       MaxTypeTraits<Arg1Type>::to_type( p->params[1] ) );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
    }
};
// dispatcher for - RT function( T0, T1 ); with time
template <class MainClass, class RT, class T0, class T1>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0, T1, FPTimeValue )> {
    typedef RT ( MainClass::*FnT )( T0, T1, FPTimeValue );

    static const int arity = 2;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;

// found a weird optimizer bug that this pragma fixed
#pragma optimize( "t", off )
    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )( MaxTypeTraits<Arg0Type>::to_type( p->params[0] ),
                                       MaxTypeTraits<Arg1Type>::to_type( p->params[1] ), t );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }
#pragma optimize( "", on )

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
    }
};

// dispatcher for - void function( T0, T1, T2 );
template <class MainClass, class T0, class T1, class T2>
struct FPDispatcherDispatch<void ( MainClass::* )( T0, T1, T2 )> {
    typedef void ( MainClass::*FnT )( T0, T1, T2 );

    static const int arity = 3;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue /*t*/, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )( MaxTypeTraits<Arg0Type>::to_type( p->params[0] ),
                            MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
                            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ) );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
    }
};
// dispatcher for - void function( T0, T1, T2 ); with time
template <class MainClass, class T0, class T1, class T2>
struct FPDispatcherDispatch<void ( MainClass::* )( T0, T1, T2, FPTimeValue )> {
    typedef void ( MainClass::*FnT )( T0, T1, T2, FPTimeValue );

    static const int arity = 3;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )( MaxTypeTraits<Arg0Type>::to_type( p->params[0] ),
                            MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
                            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), t );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
    }
};

// dispatcher for - RT function( T0, T1, T2 );
template <class MainClass, class RT, class T0, class T1, class T2>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0, T1, T2 )> {
    typedef RT ( MainClass::*FnT )( T0, T1, T2 );

    static const int arity = 3;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue /*t*/, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )( MaxTypeTraits<Arg0Type>::to_type( p->params[0] ),
                                       MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
                                       MaxTypeTraits<Arg2Type>::to_type( p->params[2] ) );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
    }
};
// dispatcher for - RT function( T0, T1, T2 ); with time
template <class MainClass, class RT, class T0, class T1, class T2>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0, T1, T2, FPTimeValue )> {
    typedef RT ( MainClass::*FnT )( T0, T1, T2, FPTimeValue );

    static const int arity = 3;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )( MaxTypeTraits<Arg0Type>::to_type( p->params[0] ),
                                       MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
                                       MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), t );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
    }
};

// dispatcher for - void function( T0, T1, T2, T3 );
template <class MainClass, class T0, class T1, class T2, class T3>
struct FPDispatcherDispatch<void ( MainClass::* )( T0, T1, T2, T3 )> {
    typedef void ( MainClass::*FnT )( T0, T1, T2, T3 );

    static const int arity = 4;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue /*t*/, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ) );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
    }
};
// dispatcher for - void function( T0, T1, T2, T3 ); with time
template <class MainClass, class T0, class T1, class T2, class T3>
struct FPDispatcherDispatch<void ( MainClass::* )( T0, T1, T2, T3, FPTimeValue )> {
    typedef void ( MainClass::*FnT )( T0, T1, T2, T3, FPTimeValue );

    static const int arity = 4;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ), t );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
    }
};

// dispatcher for - RT function( T0, T1, T2, T3 );
template <class MainClass, class RT, class T0, class T1, class T2, class T3>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0, T1, T2, T3 )> {
    typedef RT ( MainClass::*FnT )( T0, T1, T2, T3 );

    static const int arity = 4;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue /*t*/, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ) );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
    }
};
// dispatcher for - RT function( T0, T1, T2, T3 ); with time
template <class MainClass, class RT, class T0, class T1, class T2, class T3>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0, T1, T2, T3, FPTimeValue )> {
    typedef RT ( MainClass::*FnT )( T0, T1, T2, T3, FPTimeValue );

    static const int arity = 4;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ), t );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
    }
};

// dispatcher for - void function( T0, T1, T2, T3, T4 );
template <class MainClass, class T0, class T1, class T2, class T3, class T4>
struct FPDispatcherDispatch<void ( MainClass::* )( T0, T1, T2, T3, T4 )> {
    typedef void ( MainClass::*FnT )( T0, T1, T2, T3, T4 );

    static const int arity = 5;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue /*t*/, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ) );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
    }
};
// dispatcher for - void function( T0, T1, T2, T3, T4 ); with time
template <class MainClass, class T0, class T1, class T2, class T3, class T4>
struct FPDispatcherDispatch<void ( MainClass::* )( T0, T1, T2, T3, T4, FPTimeValue )> {
    typedef void ( MainClass::*FnT )( T0, T1, T2, T3, T4, FPTimeValue );

    static const int arity = 5;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), t );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
    }
};

// dispatcher for - RT function( T0, T1, T2, T3, T4 );
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0, T1, T2, T3, T4 )> {
    typedef RT ( MainClass::*FnT )( T0, T1, T2, T3, T4 );

    static const int arity = 5;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue /*t*/, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ) );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
    }
};
// dispatcher for - RT function( T0, T1, T2, T3, T4 ); with time
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0, T1, T2, T3, T4, FPTimeValue )> {
    typedef RT ( MainClass::*FnT )( T0, T1, T2, T3, T4, FPTimeValue );

    static const int arity = 5;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), t );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
    }
};

// HERE
// dispatcher for - void function( T0, T1, T2, T3, T4, T5 );
template <class MainClass, class T0, class T1, class T2, class T3, class T4, class T5>
struct FPDispatcherDispatch<void ( MainClass::* )( T0, T1, T2, T3, T4, T5 )> {
    typedef void ( MainClass::*FnT )( T0, T1, T2, T3, T4, T5 );

    static const int arity = 6;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;
    typedef typename RemoveConstRef<T5>::type Arg5Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue /*t*/, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), MaxTypeTraits<Arg5Type>::to_type( p->params[5] ) );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
        va.add_values( paramNames[5].c_str(), 0, MaxTypeTraits<Arg5Type>::type_enum() );
    }
};
// dispatcher for - void function( T0, T1, T2, T3, T4, T5 ); with time
template <class MainClass, class T0, class T1, class T2, class T3, class T4, class T5>
struct FPDispatcherDispatch<void ( MainClass::* )( T0, T1, T2, T3, T4, T5, FPTimeValue )> {
    typedef void ( MainClass::*FnT )( T0, T1, T2, T3, T4, T5, FPTimeValue );

    static const int arity = 6;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;
    typedef typename RemoveConstRef<T5>::type Arg5Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), MaxTypeTraits<Arg5Type>::to_type( p->params[5] ), t );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
        va.add_values( paramNames[5].c_str(), 0, MaxTypeTraits<Arg5Type>::type_enum() );
    }
};

// dispatcher for - RT function( T0, T1, T2, T3, T4, T5 );
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5 )> {
    typedef RT ( MainClass::*FnT )( T0, T1, T2, T3, T4, T5 );

    static const int arity = 6;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;
    typedef typename RemoveConstRef<T5>::type Arg5Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue /*t*/, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), MaxTypeTraits<Arg5Type>::to_type( p->params[5] ) );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
        va.add_values( paramNames[5].c_str(), 0, MaxTypeTraits<Arg5Type>::type_enum() );
    }
};

// dispatcher for - RT function( T0, T1, T2, T3, T4, T5 ); with time
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5, FPTimeValue )> {
    typedef RT ( MainClass::*FnT )( T0, T1, T2, T3, T4, T5, FPTimeValue );

    static const int arity = 6;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;
    typedef typename RemoveConstRef<T5>::type Arg5Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), MaxTypeTraits<Arg5Type>::to_type( p->params[5] ), t );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
        va.add_values( paramNames[5].c_str(), 0, MaxTypeTraits<Arg5Type>::type_enum() );
    }
};
// dispatcher for - void function( T0, T1, T2, T3, T4, T5, T6 ); without time
template <class MainClass, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct FPDispatcherDispatch<void ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6 )> {
    typedef void ( MainClass::*FnT )( T0, T1, T2, T3, T4, T5, T6 );

    static const int arity = 7;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;
    typedef typename RemoveConstRef<T5>::type Arg5Type;
    typedef typename RemoveConstRef<T6>::type Arg6Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), MaxTypeTraits<Arg5Type>::to_type( p->params[5] ),
            MaxTypeTraits<Arg6Type>::to_type( p->params[6] ), );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
        va.add_values( paramNames[5].c_str(), 0, MaxTypeTraits<Arg5Type>::type_enum() );
        va.add_values( paramNames[6].c_str(), 0, MaxTypeTraits<Arg6Type>::type_enum() );
    }
};
// dispatcher for - void function( T0, T1, T2, T3, T4, T5, T6 ); with time
template <class MainClass, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct FPDispatcherDispatch<void ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6, FPTimeValue )> {
    typedef void ( MainClass::*FnT )( T0, T1, T2, T3, T4, T5, T6, FPTimeValue );

    static const int arity = 7;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;
    typedef typename RemoveConstRef<T5>::type Arg5Type;
    typedef typename RemoveConstRef<T6>::type Arg6Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), MaxTypeTraits<Arg5Type>::to_type( p->params[5] ),
            MaxTypeTraits<Arg6Type>::to_type( p->params[6] ), t );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
        va.add_values( paramNames[5].c_str(), 0, MaxTypeTraits<Arg5Type>::type_enum() );
        va.add_values( paramNames[6].c_str(), 0, MaxTypeTraits<Arg6Type>::type_enum() );
    }
};
// dispatcher for - RT function( T0, T1, T2, T3, T4, T5, T6 ); without time
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6 )> {
    typedef RT ( MainClass::*FnT )( T0, T1, T2, T3, T4, T5, T6 );

    static const int arity = 7;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;
    typedef typename RemoveConstRef<T5>::type Arg5Type;
    typedef typename RemoveConstRef<T6>::type Arg6Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), MaxTypeTraits<Arg5Type>::to_type( p->params[5] ),
            MaxTypeTraits<Arg6Type>::to_type( p->params[6] ), );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
        va.add_values( paramNames[5].c_str(), 0, MaxTypeTraits<Arg5Type>::type_enum() );
        va.add_values( paramNames[6].c_str(), 0, MaxTypeTraits<Arg6Type>::type_enum() );
    }
};
// dispatcher for - RT function( T0, T1, T2, T3, T4, T5, T6 ); with time
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6, FPTimeValue )> {
    typedef RT ( MainClass::*FnT )( T0, T1, T2, T3, T4, T5, T6, FPTimeValue );

    static const int arity = 7;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;
    typedef typename RemoveConstRef<T5>::type Arg5Type;
    typedef typename RemoveConstRef<T6>::type Arg6Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), MaxTypeTraits<Arg5Type>::to_type( p->params[5] ),
            MaxTypeTraits<Arg6Type>::to_type( p->params[6] ), t );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
        va.add_values( paramNames[5].c_str(), 0, MaxTypeTraits<Arg5Type>::type_enum() );
        va.add_values( paramNames[6].c_str(), 0, MaxTypeTraits<Arg6Type>::type_enum() );
    }
};

// dispatcher for - void function( T0, T1, T2, T3, T4, T5, T6, T7 ); without time
template <class MainClass, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct FPDispatcherDispatch<void ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6, T7 )> {
    typedef void ( MainClass::*FnT )( T0, T1, T2, T3, T4, T5, T6, T7 );

    static const int arity = 8;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;
    typedef typename RemoveConstRef<T5>::type Arg5Type;
    typedef typename RemoveConstRef<T6>::type Arg6Type;
    typedef typename RemoveConstRef<T7>::type Arg7Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), MaxTypeTraits<Arg5Type>::to_type( p->params[5] ),
            MaxTypeTraits<Arg6Type>::to_type( p->params[6] ), MaxTypeTraits<Arg7Type>::to_type( p->params[7] ), );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
        va.add_values( paramNames[5].c_str(), 0, MaxTypeTraits<Arg5Type>::type_enum() );
        va.add_values( paramNames[6].c_str(), 0, MaxTypeTraits<Arg6Type>::type_enum() );
        va.add_values( paramNames[7].c_str(), 0, MaxTypeTraits<Arg7Type>::type_enum() );
    }
};
// dispatcher for - void function( T0, T1, T2, T3, T4, T5, T6, T7 ); with time
template <class MainClass, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct FPDispatcherDispatch<void ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6, T7, FPTimeValue )> {
    typedef void ( MainClass::*FnT )( T0, T1, T2, T3, T4, T5, T6, T7, FPTimeValue );

    static const int arity = 8;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;
    typedef typename RemoveConstRef<T5>::type Arg5Type;
    typedef typename RemoveConstRef<T6>::type Arg6Type;
    typedef typename RemoveConstRef<T7>::type Arg7Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), MaxTypeTraits<Arg5Type>::to_type( p->params[5] ),
            MaxTypeTraits<Arg6Type>::to_type( p->params[6] ), MaxTypeTraits<Arg7Type>::to_type( p->params[7] ), t );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
        va.add_values( paramNames[5].c_str(), 0, MaxTypeTraits<Arg5Type>::type_enum() );
        va.add_values( paramNames[6].c_str(), 0, MaxTypeTraits<Arg6Type>::type_enum() );
        va.add_values( paramNames[7].c_str(), 0, MaxTypeTraits<Arg7Type>::type_enum() );
    }
};
// dispatcher for - RT function( T0, T1, T2, T3, T4, T5, T6, T7 ); without time
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6, T7 )> {
    typedef RT ( MainClass::*FnT )( T0, T1, T2, T3, T4, T5, T6, T7 );

    static const int arity = 8;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;
    typedef typename RemoveConstRef<T5>::type Arg5Type;
    typedef typename RemoveConstRef<T6>::type Arg6Type;
    typedef typename RemoveConstRef<T7>::type Arg7Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), MaxTypeTraits<Arg5Type>::to_type( p->params[5] ),
            MaxTypeTraits<Arg6Type>::to_type( p->params[6] ), MaxTypeTraits<Arg7Type>::to_type( p->params[7] ) );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
        va.add_values( paramNames[5].c_str(), 0, MaxTypeTraits<Arg5Type>::type_enum() );
        va.add_values( paramNames[6].c_str(), 0, MaxTypeTraits<Arg6Type>::type_enum() );
        va.add_values( paramNames[7].c_str(), 0, MaxTypeTraits<Arg7Type>::type_enum() );
    }
};
// dispatcher for - RT function( T0, T1, T2, T3, T4, T5, T6, T7 ); with time
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6, T7, FPTimeValue )> {
    typedef RT ( MainClass::*FnT )( T0, T1, T2, T3, T4, T5, T6, T7, FPTimeValue );

    static const int arity = 8;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;
    typedef typename RemoveConstRef<T5>::type Arg5Type;
    typedef typename RemoveConstRef<T6>::type Arg6Type;
    typedef typename RemoveConstRef<T7>::type Arg7Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), MaxTypeTraits<Arg5Type>::to_type( p->params[5] ),
            MaxTypeTraits<Arg6Type>::to_type( p->params[6] ), MaxTypeTraits<Arg7Type>::to_type( p->params[7] ), t );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
        va.add_values( paramNames[5].c_str(), 0, MaxTypeTraits<Arg5Type>::type_enum() );
        va.add_values( paramNames[6].c_str(), 0, MaxTypeTraits<Arg6Type>::type_enum() );
        va.add_values( paramNames[7].c_str(), 0, MaxTypeTraits<Arg7Type>::type_enum() );
    }
};

// dispatcher for - void function( T0, T1, T2, T3, T4, T5, T6, T7, T8 ); without time
template <class MainClass, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct FPDispatcherDispatch<void ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6, T7, T8 )> {
    typedef void ( MainClass::*FnT )( T0, T1, T2, T3, T4, T5, T6, T7, T8 );

    static const int arity = 9;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;
    typedef typename RemoveConstRef<T5>::type Arg5Type;
    typedef typename RemoveConstRef<T6>::type Arg6Type;
    typedef typename RemoveConstRef<T7>::type Arg7Type;
    typedef typename RemoveConstRef<T8>::type Arg8Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), MaxTypeTraits<Arg5Type>::to_type( p->params[5] ),
            MaxTypeTraits<Arg6Type>::to_type( p->params[6] ), MaxTypeTraits<Arg7Type>::to_type( p->params[7] ),
            MaxTypeTraits<Arg8Type>::to_type( p->params[8] ) );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
        va.add_values( paramNames[5].c_str(), 0, MaxTypeTraits<Arg5Type>::type_enum() );
        va.add_values( paramNames[6].c_str(), 0, MaxTypeTraits<Arg6Type>::type_enum() );
        va.add_values( paramNames[7].c_str(), 0, MaxTypeTraits<Arg7Type>::type_enum() );
        va.add_values( paramNames[8].c_str(), 0, MaxTypeTraits<Arg8Type>::type_enum() );
    }
};
// dispatcher for - void function( T0, T1, T2, T3, T4, T5, T6, T7, T8 ); with time
template <class MainClass, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct FPDispatcherDispatch<void ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6, T7, T8, FPTimeValue )> {
    typedef void ( MainClass::*FnT )( T0, T1, T2, T3, T4, T5, T6, T7, T8, FPTimeValue );

    static const int arity = 9;
    typedef void ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;
    typedef typename RemoveConstRef<T5>::type Arg5Type;
    typedef typename RemoveConstRef<T6>::type Arg6Type;
    typedef typename RemoveConstRef<T7>::type Arg7Type;
    typedef typename RemoveConstRef<T8>::type Arg8Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& /*result*/, FPParams* p ) {
        ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), MaxTypeTraits<Arg5Type>::to_type( p->params[5] ),
            MaxTypeTraits<Arg6Type>::to_type( p->params[6] ), MaxTypeTraits<Arg7Type>::to_type( p->params[7] ),
            MaxTypeTraits<Arg8Type>::to_type( p->params[8] ), t );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
        va.add_values( paramNames[5].c_str(), 0, MaxTypeTraits<Arg5Type>::type_enum() );
        va.add_values( paramNames[6].c_str(), 0, MaxTypeTraits<Arg6Type>::type_enum() );
        va.add_values( paramNames[7].c_str(), 0, MaxTypeTraits<Arg7Type>::type_enum() );
        va.add_values( paramNames[8].c_str(), 0, MaxTypeTraits<Arg8Type>::type_enum() );
    }
};
// dispatcher for - RT function( T0, T1, T2, T3, T4, T5, T6, T7, T8 ); without time
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7,
          class T8>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6, T7, T8 )> {
    typedef RT ( MainClass::*FnT )( T0, T1, T2, T3, T4, T5, T6, T7, T8 );

    static const int arity = 9;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;
    typedef typename RemoveConstRef<T5>::type Arg5Type;
    typedef typename RemoveConstRef<T6>::type Arg6Type;
    typedef typename RemoveConstRef<T7>::type Arg7Type;
    typedef typename RemoveConstRef<T8>::type Arg8Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), MaxTypeTraits<Arg5Type>::to_type( p->params[5] ),
            MaxTypeTraits<Arg6Type>::to_type( p->params[6] ), MaxTypeTraits<Arg7Type>::to_type( p->params[7] ),
            MaxTypeTraits<Arg8Type>::to_type( p->params[8] ), );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
        va.add_values( paramNames[5].c_str(), 0, MaxTypeTraits<Arg5Type>::type_enum() );
        va.add_values( paramNames[6].c_str(), 0, MaxTypeTraits<Arg6Type>::type_enum() );
        va.add_values( paramNames[7].c_str(), 0, MaxTypeTraits<Arg7Type>::type_enum() );
        va.add_values( paramNames[8].c_str(), 0, MaxTypeTraits<Arg8Type>::type_enum() );
    }
};
// dispatcher for - RT function( T0, T1, T2, T3, T4, T5, T6, T7, T8 ); with time
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7,
          class T8>
struct FPDispatcherDispatch<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6, T7, T8, FPTimeValue )> {
    typedef RT ( MainClass::*FnT )( T0, T1, T2, T3, T4, T5, T6, T7, T8, FPTimeValue );

    static const int arity = 9;
    typedef typename RemoveConstRef<RT>::type ReturnType;
    typedef typename RemoveConstRef<T0>::type Arg0Type;
    typedef typename RemoveConstRef<T1>::type Arg1Type;
    typedef typename RemoveConstRef<T2>::type Arg2Type;
    typedef typename RemoveConstRef<T3>::type Arg3Type;
    typedef typename RemoveConstRef<T4>::type Arg4Type;
    typedef typename RemoveConstRef<T5>::type Arg5Type;
    typedef typename RemoveConstRef<T6>::type Arg6Type;
    typedef typename RemoveConstRef<T7>::type Arg7Type;
    typedef typename RemoveConstRef<T8>::type Arg8Type;

    inline static void dispatch( FnT function, MainClass* obj, TimeValue t, FPValue& result, FPParams* p ) {
        RT value = ( obj->*function )(
            MaxTypeTraits<Arg0Type>::to_type( p->params[0] ), MaxTypeTraits<Arg1Type>::to_type( p->params[1] ),
            MaxTypeTraits<Arg2Type>::to_type( p->params[2] ), MaxTypeTraits<Arg3Type>::to_type( p->params[3] ),
            MaxTypeTraits<Arg4Type>::to_type( p->params[4] ), MaxTypeTraits<Arg5Type>::to_type( p->params[5] ),
            MaxTypeTraits<Arg6Type>::to_type( p->params[6] ), MaxTypeTraits<Arg7Type>::to_type( p->params[7] ),
            MaxTypeTraits<Arg8Type>::to_type( p->params[8] ), t );
        MaxTypeTraits<ReturnType>::set_fpvalue( value, result );
    }

    inline static void add_descriptor_argument_varargs( make_varargs& va,
                                                        const std::vector<frantic::tstring>& paramNames ) {
        va.add_values( paramNames[0].c_str(), 0, MaxTypeTraits<Arg0Type>::type_enum() );
        va.add_values( paramNames[1].c_str(), 0, MaxTypeTraits<Arg1Type>::type_enum() );
        va.add_values( paramNames[2].c_str(), 0, MaxTypeTraits<Arg2Type>::type_enum() );
        va.add_values( paramNames[3].c_str(), 0, MaxTypeTraits<Arg3Type>::type_enum() );
        va.add_values( paramNames[4].c_str(), 0, MaxTypeTraits<Arg4Type>::type_enum() );
        va.add_values( paramNames[5].c_str(), 0, MaxTypeTraits<Arg5Type>::type_enum() );
        va.add_values( paramNames[6].c_str(), 0, MaxTypeTraits<Arg6Type>::type_enum() );
        va.add_values( paramNames[7].c_str(), 0, MaxTypeTraits<Arg7Type>::type_enum() );
        va.add_values( paramNames[8].c_str(), 0, MaxTypeTraits<Arg8Type>::type_enum() );
    }
};
// ------------- The container class which holds the function pointer and knows how to dispatch it

// dispatcher for - void function();
template <class MainClass, class FnT>
class FPDispatcherImpl : public FPDispatcher<MainClass> {
    FnT m_function;

  public:
    static const int arity = FPDispatcherDispatch<FnT>::arity;
    typedef typename FPDispatcherDispatch<FnT>::ReturnType ReturnType;

    FPDispatcherImpl( const FnT& function, FunctionID fid, const frantic::tstring& name,
                      const std::vector<frantic::tstring>& paramNames )
        : m_function( function ) {
        m_fid = fid;
        m_name = name;
        m_paramNames = paramNames;
    }

    void dispatch( MainClass* obj, TimeValue t, FPValue& result, FPParams* p ) {
        assert( arity == 0 || p != 0 );
#if MAX_VERSION_MAJOR < 15
        assert( ( arity == 0 && p == 0 ) || p->params.Count() == arity );
#else
        assert( ( arity == 0 && p == 0 ) || p->params.length() == arity );
#endif
        try {
            FPDispatcherDispatch<FnT>::dispatch( m_function, obj, t, result, p );
        } catch( std::exception& e ) {
            const frantic::tstring msg = frantic::strings::to_tstring( e.what() );
            throw MAXException( const_cast<TCHAR*>( msg.c_str() ) );
        }
    }

    virtual void add_descriptor_varargs( make_varargs& va ) {
        // add the varargs specifying the function
        va.add_values( m_fid, m_name.c_str(), 0, MaxTypeTraits<ReturnType>::type_enum(), 0, arity );
        // and then the varargs specifying each argument
        FPDispatcherDispatch<FnT>::add_descriptor_argument_varargs( va, m_paramNames );
    }
};

} // namespace fpwrapper
} // namespace max3d
} // namespace frantic
