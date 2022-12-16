// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <deque>
#include <list>
#include <map>
#include <set>
#include <vector>

#pragma warning( push, 3 )
#if MAX_VERSION_MAJOR >= 14
#include <maxscript/foundation/3dmath.h>
#include <maxscript/foundation/arrays.h>
#include <maxscript/foundation/colors.h>
#include <maxscript/foundation/numbers.h>
#include <maxscript/foundation/strings.h>
#include <maxscript/kernel/value.h>
#include <maxscript/maxscript.h>
#include <maxscript/maxwrapper/bitmaps.h>
#include <maxscript/maxwrapper/mxsmaterial.h>
#include <maxscript/maxwrapper/mxsobjects.h>
#else
#include "MAXScrpt/3dmath.h"
#include "MAXScrpt/arrays.h"
#include "MAXScrpt/maxobj.h"
#include "MAXScrpt/numbers.h"
#include "MAXScrpt/strings.h"
#include "MAXScrpt/value.h"
#include "Maxscrpt/maxmats.h"
#include "maxscrpt/bitmaps.h"
#include "maxscrpt/colorval.h"
#include <MAXScrpt/MAXScrpt.h>
#endif
#include "fpnodehandle.hpp"
#include "fptimevalue.hpp"
#pragma warning( pop )

#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/maxscript/maxscript.hpp>

#include <boost/ref.hpp>

namespace frantic {
namespace max3d {
namespace fpwrapper {

// empty struct for the default type traits error
struct unknown_max_type {};
// empty struct to indicate converting from the max type to this type is not implemented
struct unimplemented_parameter_type {};

// Quick little meta-program to remove the const and the reference from a type
// Its purpose is to convert parameter types like 'const string&' into just 'string'
template <class T>
struct RemoveConstRef {
    typedef T type;
};
template <class T>
struct RemoveConstRef<T&> {
    typedef T type;
};
template <class T>
struct RemoveConstRef<const T&> {
    typedef T type;
};
template <class T>
struct RemoveConstRef<const T> {
    typedef T type;
};

// This class wraps all the #define's for 3dsmax types into a template system
// The default type trait returns unknown_max_type for everything, so a "cannot convert to unknown_max_type" error
// will occur if the type hasn't been specialized yet.
template <class T>
class MaxTypeTraits {
  public:
    // The actual type being passed in
    typedef unknown_max_type type;
    // The type for the FPParam or FPValue
    typedef unknown_max_type max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_UNSPECIFIED; }
    inline static unknown_max_type to_type( const unknown_max_type& ) { return unknown_max_type(); }
    inline static unknown_max_type to_type( const FPValue& ) { return unknown_max_type(); }
    inline static unknown_max_type to_max_type( const unknown_max_type& ) { return unknown_max_type(); }
    inline static unknown_max_type to_value( const unknown_max_type& ) { return unknown_max_type(); }
    inline static void set_fpvalue( const unknown_max_type&, FPValue& ) {}
    // inline static bool FromParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t){ return type();}
    // inline static bool FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, int tabIndex = 0){return
    // type();}
    inline static bool FromParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, type& returnVal ) { return false; }
    inline static bool FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, type& returnVal,
                                       int tabIndex = 0 ) {
        return false;
    }

    inline static bool ToParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, const type& inputVal ) {
        return false;
    }
    inline static bool ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,
                                     int tabIndex = 0 ) {
        return false;
    }

    inline static type FromValue( Value* value ) { return type(); }
    inline static bool is_compatible_type( int type ) { return false; }
};

// Do the few primitive types with this macro
#define ff_make_MaxTypeTraitPrimitive( maxt )                                                                          \
    template <>                                                                                                        \
    class MaxTypeTraits<maxt##_TYPE> {                                                                                 \
      public:                                                                                                          \
        typedef maxt##_TYPE type;                                                                                      \
        typedef maxt##_TYPE max_type;                                                                                  \
        inline static ParamType2 type_enum() { return (ParamType2)maxt; }                                              \
        inline static type to_type( const max_type& in ) { return in; }                                                \
        inline static type to_type( const FPValue& in ) {                                                              \
            assert( in.type == type_enum() );                                                                          \
            return to_type( maxt##_FIELD( in ) );                                                                      \
        }                                                                                                              \
        inline static max_type to_max_type( const type& in ) { return in; }                                            \
        inline static Value* to_value( const type& in );                                                               \
        inline static void set_fpvalue( const type& in, FPValue& out ) { out.Load( type_enum(), to_max_type( in ) ); } \
        inline static bool FromParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, type& returnVal );             \
        inline static bool FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, type& returnVal,           \
                                           int tabIndex = 0 );                                                         \
        inline static bool ToParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, const type& inputVal );          \
        inline static bool ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,        \
                                         int tabIndex = 0 );                                                           \
        inline static type FromValue( Value* );                                                                        \
        inline static bool is_compatible_type( int type );                                                             \
    };

ff_make_MaxTypeTraitPrimitive( TYPE_INT );
inline Value* MaxTypeTraits<int>::to_value( const int& in ) {
    Value* v = new( GC_IN_HEAP ) Integer( in );
#if MAX_VERSION_MAJOR >= 19
    return_value( v );
#else
    return_protected( v );
#endif
}
inline bool MaxTypeTraits<int>::FromParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, int& returnVal ) {
    Interval ivalid = FOREVER;
    return frantic::max3d::to_bool( p->GetValue( paramIdx, t, returnVal, ivalid ) );
}

inline bool MaxTypeTraits<int>::FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, int& returnVal,
                                                int tabIndex ) {
    Interval ivalid = FOREVER;
    return frantic::max3d::to_bool( p->GetValue( paramId, t, returnVal, ivalid, tabIndex ) );
}

inline bool MaxTypeTraits<int>::ToParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, const type& inputVal ) {
    return frantic::max3d::to_bool( p->SetValue( paramIdx, t, inputVal ) );
}

inline bool MaxTypeTraits<int>::ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,
                                              int tabIndex ) {
    return frantic::max3d::to_bool( p->SetValue( paramId, t, inputVal, tabIndex ) );
}

inline int MaxTypeTraits<int>::FromValue( Value* value ) {
    if( value->is_kind_of( &Boolean_class ) )
        return (int)value->to_bool();
    else
        return value->to_int();
}

inline bool MaxTypeTraits<int>::is_compatible_type( int type ) {
    return type == TYPE_INT || type == TYPE_BOOL || type == TYPE_bool || type == TYPE_DWORD ||
           type == TYPE_RADIOBTN_INDEX || type == TYPE_TIMEVALUE;
}

ff_make_MaxTypeTraitPrimitive( TYPE_FLOAT );
inline Value* MaxTypeTraits<float>::to_value( const float& in ) {
    Value* v = new( GC_IN_HEAP ) Float( in );
#if MAX_VERSION_MAJOR >= 19
    return_value( v );
#else
    return_protected( v );
#endif
}
inline bool MaxTypeTraits<float>::FromParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, float& returnVal ) {
    Interval ivalid = FOREVER;
    bool success = false;
    if( MaxTypeTraits<int>::is_compatible_type( p->GetParameterType( paramIdx ) ) ) {
        int retProxy;
        success = MaxTypeTraits<int>::FromParamBlock( p, paramIdx, t, retProxy );
        if( success )
            returnVal = (type)retProxy;
    } else
        success = frantic::max3d::to_bool( p->GetValue( paramIdx, t, returnVal, ivalid ) );
    return success;
}

inline bool MaxTypeTraits<float>::FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, float& returnVal,
                                                  int tabIndex ) {
    Interval ivalid = FOREVER;
    bool success = false;
    if( MaxTypeTraits<int>::is_compatible_type( p->GetParameterType( paramId ) ) ) {
        int retProxy;
        success = MaxTypeTraits<int>::FromParamBlock( p, paramId, t, retProxy, tabIndex );
        if( success )
            returnVal = (type)retProxy;
    } else
        success = frantic::max3d::to_bool( p->GetValue( paramId, t, returnVal, ivalid, tabIndex ) );
    return success;
}

inline bool MaxTypeTraits<float>::ToParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, const type& inputVal ) {
    return frantic::max3d::to_bool( p->SetValue( paramIdx, t, inputVal ) );
}

inline bool MaxTypeTraits<float>::ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,
                                                int tabIndex ) {
    return frantic::max3d::to_bool( p->SetValue( paramId, t, inputVal, tabIndex ) );
}

inline float MaxTypeTraits<float>::FromValue( Value* value ) {
    if( value->is_kind_of( &Boolean_class ) )
        return (float)value->to_bool();
    else
        return value->to_float();
}

inline bool MaxTypeTraits<float>::is_compatible_type( int type ) {
    return type == TYPE_FLOAT || type == TYPE_PCNT_FRAC || type == TYPE_WORLD ||
           MaxTypeTraits<int>::is_compatible_type( type );
}

ff_make_MaxTypeTraitPrimitive( TYPE_DWORD );
inline Value* MaxTypeTraits<DWORD>::to_value( const DWORD& in ) {
    // TODO: Is there an actual DWORD type in maxscript?
    Value* v = new( GC_IN_HEAP ) Integer( in );
#if MAX_VERSION_MAJOR >= 19
    return_value( v );
#else
    return_protected( v );
#endif
}
inline bool MaxTypeTraits<DWORD>::FromParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, DWORD& returnVal ) {
    Interval ivalid = FOREVER;
    int retProxy;
    bool success = frantic::max3d::to_bool( p->GetValue( paramIdx, t, retProxy, ivalid ) );
    if( success )
        returnVal = retProxy;

    return success;
}

inline bool MaxTypeTraits<DWORD>::FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, DWORD& returnVal,
                                                  int tabIndex ) {
    Interval ivalid = FOREVER;
    int retProxy;
    bool success = frantic::max3d::to_bool( p->GetValue( paramId, t, retProxy, ivalid, tabIndex ) );
    if( success )
        returnVal = retProxy;

    return success;
}

inline bool MaxTypeTraits<DWORD>::ToParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, const type& inputVal ) {
    return frantic::max3d::to_bool( p->SetValue( paramIdx, t, (int)inputVal ) );
}

inline bool MaxTypeTraits<DWORD>::ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,
                                                int tabIndex ) {
    return frantic::max3d::to_bool( p->SetValue( paramId, t, (int)inputVal, tabIndex ) );
}

inline DWORD MaxTypeTraits<DWORD>::FromValue( Value* value ) { return value->to_int(); }

inline bool MaxTypeTraits<DWORD>::is_compatible_type( int type ) {
    return type == TYPE_INT || type == TYPE_BOOL || type == TYPE_bool || type == TYPE_DWORD ||
           type == TYPE_RADIOBTN_INDEX || type == TYPE_TIMEVALUE;
}

ff_make_MaxTypeTraitPrimitive( TYPE_INT64 );
inline Value* MaxTypeTraits<INT64>::to_value( const INT64& in ) {
    // TODO: Is there an actual DWORD type in maxscript?
    Value* v = new( GC_IN_HEAP ) Integer64( in );
#if MAX_VERSION_MAJOR >= 19
    return_value( v );
#else
    return_protected( v );
#endif
}
inline bool MaxTypeTraits<INT64>::FromParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, INT64& returnVal ) {
    Interval ivalid = FOREVER;
    int retProxy;
    bool success = frantic::max3d::to_bool( p->GetValue( paramIdx, t, retProxy, ivalid ) );
    if( success )
        returnVal = retProxy;

    return success;
}

inline bool MaxTypeTraits<INT64>::FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, INT64& returnVal,
                                                  int tabIndex ) {
    Interval ivalid = FOREVER;
    int retProxy;
    bool success = frantic::max3d::to_bool( p->GetValue( paramId, t, retProxy, ivalid, tabIndex ) );
    if( success )
        returnVal = retProxy;

    return success;
}

inline bool MaxTypeTraits<INT64>::ToParamBlock( IParamBlock*, int, FPTimeValue, const type& ) {
    throw std::runtime_error( "MaxTypeTraits<INT64> error: IParamBlock is unable to store an INT64\n" );
}

inline bool MaxTypeTraits<INT64>::ToParamBlock( IParamBlock2*, ParamID, FPTimeValue, const type&, int ) {
    throw std::runtime_error( "MaxTypeTraits<INT64> error: IParamBlock2 is unable to store an INT64\n" );
}

inline INT64 MaxTypeTraits<INT64>::FromValue( Value* value ) { return value->to_int64(); }

inline bool MaxTypeTraits<INT64>::is_compatible_type( int type ) {
    return type == TYPE_INT || type == TYPE_INT64 || type == TYPE_BOOL || type == TYPE_bool || type == TYPE_DWORD ||
           type == TYPE_RADIOBTN_INDEX || type == TYPE_TIMEVALUE;
}

ff_make_MaxTypeTraitPrimitive( TYPE_bool );
inline Value* MaxTypeTraits<bool>::to_value( const bool& in ) { return in ? &true_value : &false_value; }

inline bool MaxTypeTraits<bool>::FromParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, bool& returnVal ) {
    int value;
    Interval ivalid = FOREVER;
    if( p->GetValue( paramIdx, t, value, ivalid ) ) {
        returnVal = ( value != 0 );
        return true;
    } else
        return false;
}

inline bool MaxTypeTraits<bool>::FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, bool& returnVal,
                                                 int tabIndex ) {
    int value;
    Interval ivalid = FOREVER;
    if( p->GetValue( paramId, t, value, ivalid, tabIndex ) ) {
        returnVal = ( value != 0 );
        return true;
    } else
        return false;
}

inline bool MaxTypeTraits<bool>::ToParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, const type& inputVal ) {
    return frantic::max3d::to_bool( p->SetValue( paramIdx, t, inputVal ? 1 : 0 ) );
}

inline bool MaxTypeTraits<bool>::ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,
                                               int tabIndex ) {
    return frantic::max3d::to_bool( p->SetValue( paramId, t, inputVal ? 1 : 0, tabIndex ) );
}

inline bool MaxTypeTraits<bool>::FromValue( Value* value ) {
    if( value->is_kind_of( &Float_class ) )
        return value->to_float() != 0.f;
    else if( value->is_kind_of( &Integer_class ) )
        return value->to_int() != 0;
    else
        return value->to_bool() != 0;
}

inline bool MaxTypeTraits<bool>::is_compatible_type( int type ) { return type == TYPE_BOOL || type == TYPE_bool; }

// Bug in ifnpub.h?  It claims that TYPE_MATRIX3_TYPE is Matrix, not Matrix3
// ff_make_MaxTypeTrait( TYPE_MATRIX3 );

// for void type
template <>
class MaxTypeTraits<void> {
  public:
    inline static ParamType2 type_enum() { return TYPE_VOID; }
};

// Type traits for FPTimeValue
template <>
class MaxTypeTraits<FPTimeValue> {
  public:
    // The actual type being passed in
    typedef FPTimeValue type;
    // The type for the FPParam or FPValue
    typedef TimeValue max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return (ParamType2)TYPE_TIMEVALUE; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return TYPE_TIMEVALUE_FIELD( in );
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    inline static Value* to_value( const type& in ) {
        Value* v = MSTime::intern( in );
#if MAX_VERSION_MAJOR >= 19
        return_value( v );
#else
        return_protected( v );
#endif
    }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.Load( type_enum(), to_max_type( in ) ); }

    inline static bool FromParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, type& returnVal ) {
        Interval ivalid = FOREVER;
        bool success = false;
        int retProxy;
        success = frantic::max3d::to_bool( p->GetValue( paramIdx, t, retProxy, ivalid ) );
        if( success )
            returnVal = retProxy;
        return success;
    }

    inline static bool FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, type& returnVal,
                                       int tabIndex = 0 ) {
        Interval ivalid = FOREVER;
        bool success = false;
        int retProxy;
        success = frantic::max3d::to_bool( p->GetValue( paramId, t, retProxy, ivalid, tabIndex ) );
        if( success )
            returnVal = retProxy;
        return success;
    }

    inline static bool ToParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, const type& inputVal ) {
        return frantic::max3d::to_bool( p->SetValue( paramIdx, t, (int)inputVal ) );
    }

    inline static bool ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,
                                     int tabIndex = 0 ) {
        return frantic::max3d::to_bool( p->SetValue( paramId, t, (int)inputVal, tabIndex ) );
    }

    inline static type FromValue( Value* value ) { return FPTimeValue( value->to_timevalue() ); }
};

// Type traits for Point3
template <>
class MaxTypeTraits<Point3> {
  public:
    // The actual type being passed in
    typedef Point3 type;
    // The type for the FPParam or FPValue
    typedef Point3 max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return (ParamType2)TYPE_POINT3; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return TYPE_POINT3_FIELD( in );
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    inline static Value* to_value( const type& in ) {
        Value* v = new( GC_IN_HEAP ) Point3Value( in );
#if MAX_VERSION_MAJOR >= 19
        return_value( v );
#else
        return_protected( v );
#endif
    }
    inline static void set_fpvalue( const type& in, FPValue& out ) {
        out.LoadPtr( TYPE_POINT3_BV, TYPE_POINT3_BV_RSLT in );
    }
    inline static bool FromParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, type& returnVal ) {
        Interval ivalid = FOREVER;
        return frantic::max3d::to_bool( p->GetValue( paramIdx, t, returnVal, ivalid ) );
    }

    inline static bool FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, type& returnVal,
                                       int tabIndex = 0 ) {
        Interval ivalid = FOREVER;
        return frantic::max3d::to_bool( p->GetValue( paramId, t, returnVal, ivalid, tabIndex ) );
    }

    inline static bool ToParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, const type& inputVal ) {
        return frantic::max3d::to_bool( p->SetValue( paramIdx, t, const_cast<type&>( inputVal ) ) );
    }

    inline static bool ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,
                                     int tabIndex = 0 ) {
        return frantic::max3d::to_bool( p->SetValue( paramId, t, const_cast<type&>( inputVal ), tabIndex ) );
    }

    inline static type FromValue( Value* value ) { return value->to_point3(); }
    inline static bool is_compatible_type( int type ) { return type == type_enum(); }
};

// The Point4 is valid in max 6 and newer
#if MAX_RELEASE >= 6000
// Type traits for Point4
template <>
class MaxTypeTraits<Point4> {
  public:
    // The actual type being passed in
    typedef Point4 type;
    // The type for the FPParam or FPValue
    typedef Point4 max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return (ParamType2)TYPE_POINT4; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return TYPE_POINT4_FIELD( in );
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    inline static Value* to_value( const type& in ) {
        Value* v = new( GC_IN_HEAP ) Point4Value( in );
#if MAX_VERSION_MAJOR >= 19
        return_value( v );
#else
        return_protected( v );
#endif
    }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), &in ); }

    inline static bool FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, type& returnVal,
                                       int tabIndex = 0 ) {
        Interval ivalid = FOREVER;
        return frantic::max3d::to_bool( p->GetValue( paramId, t, returnVal, ivalid, tabIndex ) );
    }

    inline static bool ToParamBlock( IParamBlock* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/,
                                     const type& /*inputVal*/ ) {
        throw std::runtime_error( "MaxTypeTraits<Point4> error: IParamBlock is unable to store a Point4\n" );
    }

    inline static bool ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,
                                     int tabIndex = 0 ) {
        return frantic::max3d::to_bool( p->SetValue( paramId, t, const_cast<type&>( inputVal ), tabIndex ) );
    }

    inline static type FromValue( Value* value ) { return value->to_point4(); }
    inline static bool is_compatible_type( int type ) { return type == type_enum(); }
};
#endif

// Type traits for Color
template <>
class MaxTypeTraits<Color> {
  public:
    // The actual type being passed in
    typedef Color type;
    // The type for the FPParam or FPValue
    typedef Color max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return (ParamType2)TYPE_COLOR; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return *in.clr;
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    inline static Value* to_value( const type& in ) {
        return ColorValue::intern( AColor( in ) );
        // return new (GC_IN_HEAP) Point3Value( Point3(in.r, in.g, in.b) );
    }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), &in ); }

    inline static bool FromParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, type& returnVal ) {
        Interval ivalid = FOREVER;
        return frantic::max3d::to_bool( p->GetValue( paramIdx, t, returnVal, ivalid ) );
    }

    inline static bool FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, type& returnVal,
                                       int tabIndex = 0 ) {
        Interval ivalid = FOREVER;
        return frantic::max3d::to_bool( p->GetValue( paramId, t, returnVal, ivalid, tabIndex ) );
    }

    inline static bool ToParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, const type& inputVal ) {
        return frantic::max3d::to_bool( p->SetValue( paramIdx, t, const_cast<type&>( inputVal ) ) );
    }

    inline static bool ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,
                                     int tabIndex = 0 ) {
        return frantic::max3d::to_bool( p->SetValue( paramId, t, const_cast<type&>( inputVal ), tabIndex ) );
    }

    inline static type FromValue( Value* value ) { return value->to_acolor(); }
    inline static bool is_compatible_type( int type ) {
        return type == TYPE_COLOR || type == TYPE_RGBA
#if MAX_RELEASE >= 6000
               || type == TYPE_FRGBA
#endif
            ;
    }
};

// Type traits for AColor
#if MAX_RELEASE >= 6000 // only compile this in v6+
template <>
class MaxTypeTraits<AColor> {
  public:
    // The actual type being passed in
    typedef AColor type;
    // The type for the FPParam or FPValue
    typedef AColor max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return (ParamType2)TYPE_FRGBA; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return *in.clr;
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    inline static Value* to_value( const type& in ) {
        return ColorValue::intern( AColor( in ) );
        // return new (GC_IN_HEAP) Point3Value( Point3(in.r, in.g, in.b) );
    }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), &in ); }

    inline static bool FromParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t, type& returnVal ) {
        Color c;
        Interval ivalid = FOREVER;
        bool result = frantic::max3d::to_bool( p->GetValue( paramIdx, t, c, ivalid ) );
        returnVal = c;
        return result;
    }

    inline static bool FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, type& returnVal,
                                       int tabIndex = 0 ) {
        Interval ivalid = FOREVER;
        return frantic::max3d::to_bool( p->GetValue( paramId, t, returnVal, ivalid, tabIndex ) );
    }

    inline static bool ToParamBlock( IParamBlock* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/,
                                     const type& /*inputVal*/ ) {
        throw std::runtime_error( "MaxTypeTraits<AColor> error: IParamBlock is unable to store an AColor\n" );
    }

    inline static bool ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,
                                     int tabIndex = 0 ) {
        return frantic::max3d::to_bool( p->SetValue( paramId, t, const_cast<type&>( inputVal ), tabIndex ) );
    }

    inline static type FromValue( Value* value ) { return value->to_acolor(); }
    inline static bool is_compatible_type( int type ) {
        return type == TYPE_COLOR || type == TYPE_RGBA || type == TYPE_FRGBA;
    }
};
#endif // max6+

// Type traits for Matrix3
template <>
class MaxTypeTraits<Matrix3> {
  public:
    // The actual type being passed in
    typedef Matrix3 type;
    // The type for the FPParam or FPValue
    typedef Matrix3 max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return (ParamType2)TYPE_MATRIX3; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return TYPE_MATRIX3_FIELD( in );
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), &in ); }
    inline static Value* to_value( const type& in ) {
        Value* v = new( GC_IN_HEAP ) Matrix3Value( in );
#if MAX_VERSION_MAJOR >= 19
        return_value( v );
#else
        return_protected( v );
#endif
    }
    inline static bool FromParamBlock( IParamBlock* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/, type& /*returnVal*/ ) {
        throw std::runtime_error( "MaxTypeTraits<Matrix3> error: IParamBlock is unable to store a matrix\n" );
    }

    inline static bool FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, type& returnVal,
                                       int tabIndex = 0 ) {
        Interval ivalid = FOREVER;
        return frantic::max3d::to_bool( p->GetValue( paramId, t, returnVal, ivalid, tabIndex ) );
    }

    inline static bool ToParamBlock( IParamBlock* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/,
                                     const type& /*inputVal*/ ) {
        throw std::runtime_error( "MaxTypeTraits<Matrix3> error: IParamBlock is unable to store a matrix\n" );
    }

    inline static bool ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,
                                     int tabIndex = 0 ) {
        return frantic::max3d::to_bool( p->SetValue( paramId, t, const_cast<type&>( inputVal ), tabIndex ) );
    }

    inline static type FromValue( Value* value ) { return value->to_matrix3(); }
    inline static bool is_compatible_type( int type ) { return type == type_enum(); }
};

// specialized for Object*
template <>
class MaxTypeTraits<Object*> {
  public:
    // The actual type being passed in
    typedef Object* type;
    // The type for the FPParam or FPValue
    typedef Object* max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_OBJECT; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return TYPE_OBJECT_FIELD( in );
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    inline static Value* to_value( const type& in ) {
        return MAXClass::make_wrapper_for( in );
        // return new (GC_IN_HEAP) MAXNode( in );
    }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), in ); }
};

// specialized for INode*
template <>
class MaxTypeTraits<INode*> {
  public:
    // The actual type being passed in
    typedef INode* type;
    // The type for the FPParam or FPValue
    typedef INode* max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_INODE; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return TYPE_INODE_FIELD( in );
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    inline static Value* to_value( const type& in ) {
        return MAXClass::make_wrapper_for( in );
        // return new (GC_IN_HEAP) MAXNode( in );
    }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), in ); }
    inline static bool FromParamBlock( IParamBlock* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/, type& /*returnVal*/ ) {
        throw std::runtime_error( "MaxTypeTraits<INode *> error: IParamBlock is unable to store an INode\n" );
    }

    inline static bool FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, type& returnVal,
                                       int tabIndex = 0 ) {
        Interval ivalid = FOREVER;
        return frantic::max3d::to_bool( p->GetValue( paramId, t, returnVal, ivalid, tabIndex ) );
    }

    inline static bool ToParamBlock( IParamBlock* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/,
                                     const type& /*inputVal*/ ) {
        throw std::runtime_error( "MaxTypeTraits<INode *> error: IParamBlock is unable to store an INode\n" );
    }

    inline static bool ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,
                                     int tabIndex = 0 ) {
        return frantic::max3d::to_bool( p->SetValue( paramId, t, inputVal, tabIndex ) );
    }

    inline static type FromValue( Value* value ) { return value->to_node(); }

    inline static bool is_compatible_type( int type ) { return type == type_enum(); }
};

template <>
class MaxTypeTraits<Control*> {
  public:
    // The actual type being passed in
    typedef Control* type;
    // The type for the FPParam or FPValue
    typedef Control* max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_CONTROL; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return TYPE_CONTROL_FIELD( in );
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    inline static Value* to_value( const type& in ) {
        Value* v = new( GC_IN_HEAP ) MAXControl( in );
#if MAX_VERSION_MAJOR >= 19
        return_value( v );
#else
        return_protected( v );
#endif
    }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), in ); }
    inline static bool FromParamBlock( IParamBlock* p, int paramIdx, FPTimeValue /*t*/, type& returnVal ) {
        returnVal = FromParamBlock( p, paramIdx );
        return true;
    }
    inline static type FromParamBlock( IParamBlock* p, int paramIdx ) { return p->GetController( paramIdx ); }

    inline static bool FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue /*t*/, type& returnVal,
                                       int tabIndex = 0 ) {
#if MAX_VERSION_MAJOR >= 14
        returnVal = p->GetControllerByID( paramId, tabIndex );
#else
        returnVal = p->GetController( paramId, tabIndex );
#endif
        return true;
    }

    inline static type FromParamBlock( IParamBlock2* p, ParamID paramId ) {
#if MAX_VERSION_MAJOR >= 14
        return p->GetControllerByID( paramId );
#else
        return p->GetController( paramId );
#endif
    }

    inline static bool ToParamBlock( IParamBlock* p, int paramIdx, FPTimeValue /*t*/, const type& inputVal ) {
        ToParamBlock( p, paramIdx, inputVal );
        return true;
    }

    inline static void ToParamBlock( IParamBlock* p, int paramIdx, const type& inputVal ) {
        p->SetController( paramIdx, inputVal );
    }

    inline static bool ToParamBlock( IParamBlock2* p, ParamID paramIdx, FPTimeValue /*t*/, const type& inputVal,
                                     int tabIndex = 0 ) {
        ToParamBlock( p, paramIdx, inputVal, tabIndex );
        return true;
    }

    inline static void ToParamBlock( IParamBlock2* p, ParamID paramIdx, const type& inputVal, int tabIndex = 0 ) {
#if MAX_VERSION_MAJOR >= 14
        p->SetControllerByID( paramIdx, tabIndex, inputVal );
#else
        p->SetController( paramIdx, tabIndex, inputVal );
#endif
    }

    inline static type FromValue( Value* value ) { return value->to_controller(); }
    inline static bool is_compatible_type( int type ) { return type == type_enum(); }
};

// specialized for INode* stored as a node handle
template <>
class MaxTypeTraits<FPNodeHandle> {
  public:
    // The actual type being passed in
    typedef FPNodeHandle type;
    // The type for the FPParam or FPValue
    typedef INode* max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_INODE; }
    inline static type to_type( const max_type& in ) { return FPNodeHandle( in ); }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return FPNodeHandle( TYPE_INODE_FIELD( in ) );
    }
    inline static max_type to_max_type( const type& in ) { return (INode*)in; }
    inline static Value* to_value( const type& in ) {
        INode* node = in;
        if( node ) {
            Value* v = new( GC_IN_HEAP ) MAXNode( node );
#if MAX_VERSION_MAJOR >= 19
            return_value( v );
#else
            return_protected( v );
#endif
        } else
            return &undefined;
    }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), in ); }
    inline static type FromValue( Value* value ) { return FPNodeHandle( value->to_node() ); }
    inline static bool is_compatible_type( int type ) { return type == type_enum(); }
};

// specialized for Mtl*
template <>
class MaxTypeTraits<Mtl*> {
  public:
    // The actual type being passed in
    typedef Mtl* type;
    // The type for the FPParam or FPValue
    typedef Mtl* max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_MTL; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return TYPE_MTL_FIELD( in );
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    // error LNK2019: unresolved external symbol "public: __thiscall MAXMaterial::MAXMaterial(class Mtl *)"
    // (??0MAXMaterial@@QAE@PAVMtl@@@Z) referenced in function "public: static class Value * __cdecl
    // frantic::fpwrapper::MaxTypeTraits<class Mtl *>::to_value(class Mtl * const &)"
    // (?to_value@?$MaxTypeTraits@PAVMtl@@@fpwrapper@frantic@@SAPAVValue@@ABQAVMtl@@@Z)
    //	inline static Value* to_value( const type& in ) {
    //		MAXMaterial* result = new (GC_IN_HEAP) MAXMaterial();
    //		result->mat = in;
    //		return result;
    //	}
    inline static Value* to_value( const type& in ) { return MAXClass::make_wrapper_for( in ); }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), in ); }

    inline static bool FromParamBlock( IParamBlock* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/, type& /*returnVal*/ ) {
        throw std::runtime_error( "MaxTypeTraits<Mtl *> error: IParamBlock is unable to store a Material\n" );
    }

    inline static bool FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, type& returnVal,
                                       int tabIndex = 0 ) {
        Interval ivalid = FOREVER;
        return frantic::max3d::to_bool( p->GetValue( paramId, t, returnVal, ivalid, tabIndex ) );
    }

    inline static bool ToParamBlock( IParamBlock* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/,
                                     const type& /*inputVal*/ ) {
        throw std::runtime_error( "MaxTypeTraits<Mtl *> error: IParamBlock is unable to store a Material\n" );
    }

    inline static bool ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,
                                     int tabIndex = 0 ) {
        return frantic::max3d::to_bool( p->SetValue( paramId, t, inputVal, tabIndex ) );
    }

    inline static type FromValue( Value* value ) { return value->to_mtl(); }
    inline static bool is_compatible_type( int type ) { return type == type_enum(); }
};

// specialized for Texmap*
template <>
class MaxTypeTraits<Texmap*> {
  public:
    // The actual type being passed in
    typedef Texmap* type;
    // The type for the FPParam or FPValue
    typedef Texmap* max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_TEXMAP; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return TYPE_TEXMAP_FIELD( in );
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    //	inline static Value* to_value( const type& in ) {
    //		MAXTexture* result = new (GC_IN_HEAP) MAXTexture(in);
    //		return result;
    //	}
    inline static Value* to_value( const type& in ) { return MAXClass::make_wrapper_for( in ); }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), in ); }

    inline static bool FromParamBlock( IParamBlock* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/, type& /*returnVal*/ ) {
        throw std::runtime_error( "MaxTypeTraits<Texmap *> error: IParamBlock is unable to store a Material\n" );
    }

    inline static bool FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, type& returnVal,
                                       int tabIndex = 0 ) {
        Interval ivalid = FOREVER;
        return frantic::max3d::to_bool( p->GetValue( paramId, t, returnVal, ivalid, tabIndex ) );
    }

    inline static bool ToParamBlock( IParamBlock* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/,
                                     const type& /*inputVal*/ ) {
        throw std::runtime_error( "MaxTypeTraits<Texmap *> error: IParamBlock is unable to store a Material\n" );
    }

    inline static bool ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,
                                     int tabIndex = 0 ) {
        return frantic::max3d::to_bool( p->SetValue( paramId, t, inputVal, tabIndex ) );
    }

    inline static type FromValue( Value* value ) { return value->to_texmap(); }
    inline static bool is_compatible_type( int type ) { return type == type_enum(); }
};

// specialized for Bitmap *
template <>
class MaxTypeTraits<PBBitmap*> {
  public:
    // The actual type being passed in
    typedef PBBitmap* type;
    // The type for the FPParam or FPValue
    typedef PBBitmap* max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_BITMAP; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return TYPE_BITMAP_FIELD( in );
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    inline static Value* to_value( const type& in ) {
        MAXBitMap* result = new( GC_IN_HEAP ) MAXBitMap( in->bi, in->bm );
#if MAX_VERSION_MAJOR >= 19
        return_value( result );
#else
        return_protected( result );
#endif
    }

    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), in ); }

    inline static bool FromParamBlock( IParamBlock* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/, type& /*returnVal*/ ) {
        throw std::runtime_error( "MaxTypeTraits<Bitmap *> error: IParamBlock is unable to store a Bitmap\n" );
    }

    inline static bool FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, type& returnVal,
                                       int tabIndex = 0 ) {
        Interval ivalid = FOREVER;
        return frantic::max3d::to_bool( p->GetValue( paramId, t, returnVal, ivalid, tabIndex ) );
    }

    inline static bool ToParamBlock( IParamBlock* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/,
                                     const type& /*inputVal*/ ) {
        throw std::runtime_error( "MaxTypeTraits<Bitmap *> error: IParamBlock is unable to store a Bitmap\n" );
    }

    inline static bool ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,
                                     int tabIndex = 0 ) {
        return frantic::max3d::to_bool( p->SetValue( paramId, t, inputVal, tabIndex ) );
    }

    // Can't really figure out how to get one of these guys from a value without leaking memory.
    // inline static type FromValue(Value * value)
    //{
    //	if(!is_bitmap(value))
    //		throw std::runtime_error("MaxTypeTraits<PBBitmap *> unable to cast Value * \"" +
    //frantic::max3d::mxs::to_string(value) + "\" to a bitmap"); 	else 		return (static_cast<MAXBitMap *>(value))->bm;
    //}
    inline static bool is_compatible_type( int type ) { return type == type_enum(); }
};

// specialized for ReferenceMaker*
template <>
class MaxTypeTraits<ReferenceMaker*> {
  public:
    // The actual type being passed in
    typedef ReferenceMaker* type;
    // The type for the FPParam or FPValue
    typedef ReferenceTarget* max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_REFTARG; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return TYPE_REFTARG_FIELD( in );
    }
    inline static max_type to_max_type( const type& in ) { return static_cast<ReferenceTarget*>( in ); }
    inline static Value* to_value( const type& in ) {
        // TODO: This seems like an unsafe cast, but will work most of the time because usually within max
        // ReferenceMakers are also ReferenceTargets
        return MAXClass::make_wrapper_for( static_cast<ReferenceTarget*>( in ) );
    }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), in ); }
    inline static type FromValue( Value* value ) { return static_cast<ReferenceMaker*>( value->to_reftarg() ); }
    inline static bool is_compatible_type( int type ) { return type == type_enum(); }
};

// specialized for ReferenceTarget*
template <>
class MaxTypeTraits<ReferenceTarget*> {
  public:
    // The actual type being passed in
    typedef ReferenceTarget* type;
    // The type for the FPParam or FPValue
    typedef ReferenceTarget* max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_REFTARG; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return TYPE_REFTARG_FIELD( in );
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    inline static Value* to_value( const type& in ) { return MAXClass::make_wrapper_for( in ); }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), in ); }

    inline static bool FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, type& returnVal,
                                       int tabIndex = 0 ) {
        Interval ivalid = FOREVER;
        return frantic::max3d::to_bool( p->GetValue( paramId, t, returnVal, ivalid, tabIndex ) );
    }

    inline static bool ToParamBlock( IParamBlock* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/,
                                     const type& /*inputVal*/ ) {
        throw std::runtime_error(
            "MaxTypeTraits<ReferenceTarget*> error: IParamBlock is unable to store a ReferenceTarget*\n" );
    }

    inline static bool ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,
                                     int tabIndex = 0 ) {
        return frantic::max3d::to_bool( p->SetValue( paramId, t, inputVal, tabIndex ) );
    }

    inline static type FromValue( Value* value ) { return value->to_reftarg(); }

    inline static bool is_compatible_type( int type ) {
        return type == TYPE_MTL || type == TYPE_TEXMAP || type == TYPE_INODE || type == TYPE_REFTARG ||
               type == TYPE_PBLOCK2 || type == TYPE_OBJECT || type == TYPE_CONTROL;
    }
};

// specialized for Mesh*
template <>
class MaxTypeTraits<Mesh*> {
  public:
    // The actual type being passed in
    typedef Mesh* type;
    // The type for the FPParam or FPValue
    typedef Mesh* max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_MESH; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return TYPE_MESH_FIELD( in );
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    //	inline static Value* to_value( const type& in ) {
    //		return new (GC_IN_HEAP) MAXNode( in );
    //	}
    // How to do this without a memory leak?
    //	inline static void set_fpvalue( const type& in, FPValue& out ) {
    //		out.LoadPtr( type_enum(), in );
    //	}
    inline static bool is_compatible_type( int type ) { return type == type_enum(); }
};

// specialized for Bitmap*
/*
// The Bitmap type was really screwy connecting with maxscript
template<>
class MaxTypeTraits<PBBitmap*> {
public:
        // The actual type being passed in
        typedef PBBitmap* type;
        // The type for the FPParam or FPValue
        typedef PBBitmap* max_type;
        // The enum value from paramtype.h
        inline static ParamType2 type_enum() { return TYPE_BITMAP; }
        inline static type to_type( const max_type& in ) { return in; }
        inline static type to_type( const FPValue& in ) { assert( in.type == type_enum() ); return
TYPE_BITMAP_FIELD(in); } inline static max_type to_max_type( const type& in ) { return in; }
//	inline static Value* to_value( const type& in ) {
//		return new (GC_IN_HEAP) MAXNode( in );
//	}
        // How to do this without a memory leak?
//	inline static void set_fpvalue( const type& in, FPValue& out ) {
//		out.LoadPtr( type_enum(), in );
//	}
        inline static bool is_compatible_type(int type)
        {
                return	type == type_enum();
        }
};
*/

// Specialized for Interval
template <>
class MaxTypeTraits<Interval> {
  public:
    // The actual type being passed in
    typedef Interval type;
    // The type for the FPParam or FPValue
    typedef Interval max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_INTERVAL; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return TYPE_INTERVAL_FIELD( in );
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    inline static Value* to_value( const type& in ) {
        Value* v = new( GC_IN_HEAP ) MSInterval( in );
#if MAX_VERSION_MAJOR >= 19
        return_value( v );
#else
        return_protected( v );
#endif
    }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), &in ); }
    inline static bool is_compatible_type( int type ) { return type == type_enum(); }
};

// specialized for Value*
template <>
class MaxTypeTraits<Value*> {
  public:
    // The actual type being passed in
    typedef Value* type;
    // The type for the FPParam or FPValue
    typedef Value* max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_VALUE; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return TYPE_VALUE_FIELD( in );
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    inline static Value* to_value( const type& in ) { return to_max_type( in ); }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), in ); }
    inline static bool is_compatible_type( int type ) { return type == type_enum(); }
    inline static type FromValue( Value* v ) { return v; }
};

// specialized for IObject*
template <>
class MaxTypeTraits<IObject*> {
  public:
    // The actual type being passed in
    typedef IObject* type;
    // The type for the FPParam or FPValue
    typedef IObject* max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_IOBJECT; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return TYPE_IOBJECT_FIELD( in );
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    //	inline static Value* to_value( const type& in ) { return new (GC_IN_HEAP) IObjectValue( in ); } // MISSING
    //FUNCTIONS IN MAX .LIB'S!!!
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), in ); }
    inline static bool is_compatible_type( int type ) { return type == type_enum(); }
};

// specialized for FPInterface*
template <>
class MaxTypeTraits<FPInterface*> {
  public:
    // The actual type being passed in
    typedef FPInterface* type;
    // The type for the FPParam or FPValue
    typedef FPInterface* max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_INTERFACE; }
    inline static type to_type( const max_type& in ) { return in; }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        return TYPE_INTERFACE_FIELD( in );
    }
    inline static max_type to_max_type( const type& in ) { return in; }
    inline static Value* to_value( const type& in ) { return new FPInterfaceValue( in ); }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), in ); }
    inline static bool is_compatible_type( int type ) { return type == type_enum(); }
};

// specialized for std::vector<IObject*>
template <>
class MaxTypeTraits<std::vector<IObject*>> {
  public:
    // The actual type being passed in
    typedef std::vector<IObject*> type;
    // The type for the FPParam or FPValue
    typedef TYPE_IOBJECT_TAB_TYPE max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_IOBJECT_TAB; }
    inline static type to_type( const max_type& in ) {
        std::vector<IObject*> result;
        for( int i = 0; i < in->Count(); ++i ) {
            result.push_back( ( *in )[i] );
        }
        return result;
    }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        max_type temp = TYPE_IOBJECT_TAB_FIELD( in );
        std::vector<IObject*> result;
        for( int i = 0; i < temp->Count(); ++i ) {
            result.push_back( ( *temp )[i] );
        }
        return result;
    }
    //	inline static max_type to_max_type( const type& in ) { return in; }
    //	inline static Value* to_value( const type& in ) { return new (GC_IN_HEAP) IObjectValue( in ); }
    inline static void set_fpvalue( const type& in, FPValue& out ) {
        Tab<IObject*>* tab = new Tab<IObject*>();
        tab->SetCount( (int)in.size() );
        for( unsigned i = 0; i < in.size(); ++i ) {
            ( *tab )[i] = in[i];
        }
        out.LoadPtr( type_enum(), tab );
    }
    inline static bool is_compatible_type( int type ) { return type == type_enum(); }
};

// specialized for std::vector<INode*>
template <>
class MaxTypeTraits<std::vector<INode*>> {
  public:
    // The actual type being passed in
    typedef std::vector<INode*> type;
    // The type for the FPParam or FPValue
    typedef TYPE_INODE_TAB_TYPE max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_INODE_TAB; }
    inline static type to_type( const max_type& in ) {
        std::vector<INode*> result;
        for( int i = 0; i < in->Count(); ++i ) {
            result.push_back( ( *in )[i] );
        }
        return result;
    }
    inline static type to_type( const FPValue& in ) {
        assert( in.type == type_enum() );
        max_type temp = TYPE_INODE_TAB_FIELD( in );
        std::vector<INode*> result;
        for( int i = 0; i < temp->Count(); ++i ) {
            result.push_back( ( *temp )[i] );
        }
        return result;
    }
    //	inline static max_type to_max_type( const type& in ) { return in; }
    //	inline static Value* to_value( const type& in ) { return new (GC_IN_HEAP) IObjectValue( in ); }
    inline static void set_fpvalue( const type& in, FPValue& out ) {
        Tab<INode*>* tab = new Tab<INode*>();
        tab->SetCount( (int)in.size() );
        for( unsigned i = 0; i < in.size(); ++i ) {
            ( *tab )[i] = in[i];
        }
        out.LoadPtr( type_enum(), tab );
    }
    inline static bool is_compatible_type( int type ) { return type == type_enum(); }
};

// specialized for STL containers
template <class STLContainer>
class MaxTypeTraitsSTL {
  public:
    // The actual type being passed in
    typedef STLContainer type;
    // The type inside the container
    typedef typename STLContainer::value_type value_type;
    // The type for the FPParam or FPValue
    // typedef Tab< typename MaxTypeTraits< value_type >::max_type >* max_type;
    typedef Value* max_type;

    // The enum value from paramtype.h
    // inline static ParamType2 type_enum() { return TYPE_VALUE; }
    // inline static ParamType2 type_enum(){ return (ParamType2)(MaxTypeTraits<value_type>::type_enum() | TYPE_TAB);}
    inline static ParamType2 type_enum() { return (ParamType2)( MaxTypeTraits<Value*>::type_enum() ); }
    inline static type to_type( const max_type& in ) {

        if( !is_array( in ) )
            throw std::runtime_error( "MaxTypeTraitsSTL::to_type() - Cannot convert type to Array." );

        type result;
        Array* arr = static_cast<Array*>( in );

        for( int i = 0; i < arr->size; ++i ) {
            Value*& v = ( *arr )[i];
            if( v ) {
                result.push_back( MaxTypeTraits<STLContainer::value_type>::FromValue( v ) );
            } else {
                throw std::runtime_error( "MaxTypeTraitsSTL::to_type() - Array entry is NULL." );
            }
        }
        return result;
    }

    inline static type to_type( const FPValue& in ) { return to_type( in.v ); }
    inline static max_type to_max_type( const type& in ) {
        mxs::frame<2> f;
        mxs::local<Array> result( f );
        mxs::local<Value> element( f );

        result = new Array( 0 );

        for( typename STLContainer::const_iterator i = in.begin(); i != in.end(); ++i ) {
            element = MaxTypeTraits<value_type>::to_value( *i );
            result->append( element );
        }
#if MAX_VERSION_MAJOR < 19
        return_protected( result );
#else
        return_value( result );
#endif
    }
    inline static Value* to_value( const type& in ) { return to_max_type( in ); }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( TYPE_VALUE, to_max_type( in ) ); }

    inline static bool FromParamBlock( IParamBlock* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/, type& /*returnVal*/ ) {
        throw std::runtime_error( "MaxTypeTraits<STLContainer> error: IParamBlock is unable to store an array\n" );
    }

    inline static bool FromParamBlock( IParamBlock2* /*p*/, ParamID /*paramId*/, FPTimeValue /*t*/, type& /*returnVal*/,
                                       int /*tabIdx*/ ) {
        throw std::runtime_error( "MaxTypeTraits<STLContainer> error: IParamBlock2 table of tables unimplemented.\n" );
    }

    inline static bool FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, type& returnVal ) {
        bool success = true;
        value_type v;
        for( int i = 0; success && i < p->Count( paramId ); i++ ) {
            success = MaxTypeTraits<value_type>::FromParamBlock( p, paramId, t, v, i );
            if( success )
                returnVal.push_back( v );
        }

        return success;
    }

    inline static bool ToParamBlock( IParamBlock* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/,
                                     const type& /*inputVal*/ ) {
        throw std::runtime_error( "MaxTypeTraits<STLContainer> error: IParamBlock is unable to store an array\n" );
    }

    inline static bool ToParamBlock( IParamBlock2* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/, const type& /*inputVal*/,
                                     int /*tabIndex*/ = 0 ) {
        throw std::runtime_error( "MaxTypeTraits<STLContainer> error: IParamBlock2 table of tables unimplemented.\n" );
    }

    inline static type FromValue( Value* v ) {
        if( !v->is_kind_of( class_tag( Array ) ) )
            throw std::runtime_error( "build_inode_list: Value * is not an array" );

        // No need to protect this value since the caller should have.
        Array* array = static_cast<Array*>( v );

        STLContainer c;
        for( int i = 0; i < array->size; ++i ) {
            c.push_back( MaxTypeTraits<value_type>::FromValue( array->get( i + 1 ) ) );
        }
        return c;
    }

    inline static bool is_compatible_type( int type ) {
        return MaxTypeTraits<value_type>::is_compatible_type( type & ( ~TYPE_TAB ) );
    }
};

// specialized for list<T>
template <class T>
class MaxTypeTraits<std::list<T>> : public MaxTypeTraitsSTL<std::list<T>> {};
// specialized for vector<T>
template <class T>
class MaxTypeTraits<std::vector<T>> : public MaxTypeTraitsSTL<std::vector<T>> {};
// specialized for deque<T>
template <class T>
class MaxTypeTraits<std::deque<T>> : public MaxTypeTraitsSTL<std::deque<T>> {};
// specialized for set<T>
template <class T>
class MaxTypeTraits<std::set<T>> : public MaxTypeTraitsSTL<std::set<T>> {};
// specialized for map<T>
template <class S, class T>
class MaxTypeTraits<std::map<S, T>> : public MaxTypeTraitsSTL<std::map<S, T>> {};

// specialized for pair<S,T> -> returns a 2 element array
template <class S, class T>
class MaxTypeTraits<std::pair<S, T>> {
  public:
    // The actual type being passed in
    typedef std::pair<S, T> type;
    // The type for the FPParam or FPValue
    typedef Value* max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return TYPE_VALUE; }
    inline static unimplemented_parameter_type to_type( const max_type& in ) { return unimplemented_parameter_type(); }
    inline static unimplemented_parameter_type to_type( const FPValue& in ) { return unimplemented_parameter_type(); }
    inline static max_type to_max_type( const type& in ) {
        mxs::frame<2> f;
        mxs::local<Array> result( f );
        mxs::local<Value> element( f );

        result = new Array( 0 );

        element = MaxTypeTraits<RemoveConstRef<S>::type>::to_value( in.first );
        result->append( element );

        element = MaxTypeTraits<RemoveConstRef<T>::type>::to_value( in.second );
        result->append( element );
#if MAX_VERSION_MAJOR < 19
        return_protected( result );
#else
        return_value( result );
#endif
    }
    inline static Value* to_value( const type& in ) { return to_max_type( in ); }
    inline static void set_fpvalue( const type& in, FPValue& out ) { out.LoadPtr( type_enum(), to_max_type( in ) ); }
};

#define MaxTypeTraits_ParamType2_IMPL( typeenum )                                                                      \
    template <>                                                                                                        \
    class MaxTypeTraits<typeenum##_TYPE> {                                                                             \
      public:                                                                                                          \
        typedef typeenum##_TYPE type;                                                                                  \
        inline static ParamType2 type_enum() { return typeenum; }                                                      \
        inline static type to_type( const FPValue& in ) { return typeenum##_FIELD( in ); }                             \
        inline static void set_fpvalue( type in, FPValue& out ) { out.LoadPtr( typeenum, typeenum##_RSLT in ); }       \
    };

MaxTypeTraits_ParamType2_IMPL( TYPE_TSTR_BV );
MaxTypeTraits_ParamType2_IMPL( TYPE_STRING );

// Allow const TCHAR* to be passed around. FP doesn't support the const idea.
// There's no need for this after MAX_VERSION_MAJOR 15, because it works fine
// with const strings.
#if MAX_VERSION_MAJOR < 15
template <>
class MaxTypeTraits<const TYPE_STRING_TYPE> {
  public:
    typedef const TYPE_STRING_TYPE type;
    inline static ParamType2 type_enum() { return MaxTypeTraits<TYPE_STRING_TYPE>::type_enum(); }
    inline static type to_type( const FPValue& in ) { return MaxTypeTraits<TYPE_STRING_TYPE>::to_type( in ); }
    inline static void set_fpvalue( type in, FPValue& out ) {
        MaxTypeTraits<TYPE_STRING_TYPE>::set_fpvalue( const_cast<TYPE_STRING_TYPE>( in ), out );
    }
};
#endif

// Passing by reference is accomplished with a boost::ref<T>
#define MaxTypeTraits_ParamType2_REF_IMPL( typeenum )                                                                  \
    template <>                                                                                                        \
    class MaxTypeTraits<boost::reference_wrapper<typeenum##_TYPE>> {                                                   \
      public:                                                                                                          \
        typedef boost::reference_wrapper<typeenum##_TYPE> type;                                                        \
        inline static ParamType2 type_enum() { return typeenum##_BR; }                                                 \
        inline static type to_type( const FPValue& in ) { return boost::ref( typeenum##_BR_FIELD( in ) ); }            \
        inline static void set_fpvalue( boost::reference_wrapper<typeenum##_TYPE> in, FPValue& out ) {                 \
            out.LoadPtr( typeenum, typeenum##_BR_RSLT in.get() );                                                      \
        }                                                                                                              \
    };

MaxTypeTraits_ParamType2_REF_IMPL( TYPE_INT );

struct FPIndex {
    TYPE_INDEX_TYPE value;
    explicit FPIndex( int _value )
        : value( _value ) {}
    operator TYPE_INDEX_TYPE() { return value; }
};

template <>
class MaxTypeTraits<FPIndex> {
  public:
    typedef const FPIndex type;
    inline static ParamType2 type_enum() { return TYPE_INDEX; }
    inline static type to_type( const FPValue& in ) { return FPIndex( TYPE_INDEX_FIELD( in ) ); }
    inline static void set_fpvalue( type in, FPValue& out ) { out.LoadPtr( TYPE_INDEX, TYPE_INDEX_RSLT in.value ); }
};

// MaxTypeTraits_CONST_ParamType2_IMPL(TYPE_STRING);

// Specialization for string
// Should we maybe use a TSTR? not sure, but we want to avoid the potential memory leak John Burnett warned about
// Trying TYPE_STRING didn't seem to work very well, and neither did TYPE_TSTR; sticking with TYPE_VALUE
template <>
class MaxTypeTraits<frantic::tstring> {
  public:
    typedef frantic::tstring type;
    typedef TYPE_VALUE_TYPE max_type;
    inline static ParamType2 type_enum() { return TYPE_VALUE; }
    inline static type to_type( const max_type& in ) {
        // std::cerr << ( "MaxTypeTraits<std::string>::to_type( const max_type& in )\n" );
        const TCHAR* result = in->to_string();
        if( result == 0 ) {
            // std::cerr << ( "recovered NULL\n" );
            return _T("");
        } else {
            // std::cerr << ( "recovered TCHAR*: %s\n", result );
            return result;
        }
        /*
        TSTR result = in->to_filename();
        return (char*)result;
        */
    }
    inline static type to_type( const TCHAR* in ) {
        // std::cerr << ( "MaxTypeTraits<std::string>::to_type(const TCHAR * in)\n" );
        if( in == 0 ) {
            return _T("");
        } else {
            return in;
        }
    }
    inline static type to_type( const FPValue& in ) {
        // std::cerr << ( "MaxTypeTraits<std::string>::to_type( const FPValue& in )\n" );
        assert( in.type == type_enum() );
        return to_type( TYPE_VALUE_FIELD( in ) );
    }
    inline static max_type to_max_type( const type& in ) {
        // std::cerr << ( "MaxTypeTraits<std::string>::to_max_type( const type& in )\n" );
        return to_value( in );
    }
    inline static Value* to_value( const type& in ) {
#if MAX_VERSION_MAJOR >= 19
        return_value( new( GC_IN_HEAP ) String( const_cast<TCHAR*>( in.c_str() ) ) );
#else
        return_protected( new( GC_IN_HEAP ) String( const_cast<TCHAR*>( in.c_str() ) ) );
#endif
    }
    inline static void set_fpvalue( const type& in, FPValue& out ) {
        // std::cerr << ( "MaxTypeTraits<std::string>::set_fpvalue( const type& in, FPValue& out )\n" );
        out.LoadPtr( type_enum(), to_max_type( in ) );
    }

    inline static bool FromParamBlock( IParamBlock* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/, type& /*returnVal*/ ) {
        // std::cerr << ( "MaxTypeTraits<std::string>::FromParamBlock( IParamBlock* p, int paramIdx, FPTimeValue t)\n"
        // );
        throw std::runtime_error( "MaxTypeTraits<std::string> error: IParamBlock is unable to store a string\n" );
    }

    inline static bool FromParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, type& returnVal,
                                       int tabIndex = 0 ) {
        // std::cerr << ( "MaxTypeTraits<std::string>::FromParamBlock( IParamBlock2* p, int paramId, FPTimeValue t, int
        // tabIndex = 0)\n" );
        bool success = false;
        const TCHAR* stringValue = p->GetStr( paramId, t, tabIndex );

        if( !IsBadReadPtr( stringValue, 1 ) ) {
            success = true;
            returnVal = stringValue;
        }

        return success;
    }

    inline static bool ToParamBlock( IParamBlock* /*p*/, int /*paramIdx*/, FPTimeValue /*t*/,
                                     const type& /*inputVal*/ ) {
        throw std::runtime_error( "MaxTypeTraits<std::string> error: IParamBlock is unable to store a string\n" );
    }

    inline static bool ToParamBlock( IParamBlock2* p, ParamID paramId, FPTimeValue t, const type& inputVal,
                                     int tabIndex = 0 ) {
        return frantic::max3d::to_bool( p->SetValue( paramId, t, (TCHAR*)inputVal.c_str(), tabIndex ) );
    }

    inline static type FromValue( Value* value ) { return value->to_string(); }

    inline static bool is_compatible_type( int type ) { return type == TYPE_STRING || type == TYPE_FILENAME; }
};

////////////////////////////
////////////////////////////
// Type traits for FPTimeValue

// For people who want the raw deal.
template <>
class MaxTypeTraits<FPValue> {
  public:
    // The actual type being passed in
    typedef FPValue type;
    // The type for the FPParam or FPValue
    typedef FPValue max_type;
    // The enum value from paramtype.h
    inline static ParamType2 type_enum() { return (ParamType2)TYPE_FPVALUE; }
    inline static const type& to_type( const max_type& in ) { return in; }
    inline static const max_type& to_max_type( const type& in ) { return in; }
    inline static void set_fpvalue( const FPValue& in, FPValue& out ) { out = in; }
};
////////////////////////////////
} // namespace fpwrapper
} // namespace max3d
} // namespace frantic
