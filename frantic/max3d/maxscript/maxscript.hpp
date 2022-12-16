// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/maxscript/includes.hpp>

#include <frantic/diagnostics/assert_macros.hpp>
#include <frantic/files/files.hpp>
#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/fpwrapper/max_typetraits.hpp>
#include <frantic/max3d/windows.hpp>
#include <frantic/misc/string_functions.hpp>

#include <boost/static_assert.hpp>
#include <boost/tuple/tuple.hpp>
#pragma warning( push, 3 )
#pragma warning( disable : 4701 4702 4267 )
#include <boost/lexical_cast.hpp>
#pragma warning( pop )

#include <boost/mpl/int.hpp>

#if MAX_VERSION_MAJOR >= 19
#include <maxscript/macros/value_locals.h>
#endif

#include <type_traits>

namespace frantic {
namespace max3d {
namespace mxs {

/**
 * This class is intended to replace the MXS macros NNN_type_value_locals() and friends with a modern, template base,
 * type-safe, exception safe version that does exactly the same thing. This is also intended to replace the older
 * frame<N> & local<T> classes.
 * @note I arbitrarily chose 6 as the number of supported types. We can always extend this.
 */
template <class T1, class T2 = void, class T3 = void, class T4 = void, class T5 = void, class T6 = void>
class mxs_local_tuple {
    struct {
        int count;
        Value** link;
        Value* vals[6];
    } m_impl; // From six_typed_value_locals() macro

  public:
    template <int N>
    struct value_type {};
    template <>
    struct value_type<0> {
        typedef T1 type;
    };
    template <>
    struct value_type<1> {
        typedef T2 type;
    };
    template <>
    struct value_type<2> {
        typedef T3 type;
    };
    template <>
    struct value_type<3> {
        typedef T4 type;
    };
    template <>
    struct value_type<4> {
        typedef T5 type;
    };
    template <>
    struct value_type<5> {
        typedef T6 type;
    };

    template <int N>
    struct ptr_type {
        typedef typename value_type<N>::type* type;
    };

    enum { COUNT = 6 };

  public:
    mxs_local_tuple() {
        init_thread_locals(); // Saw this in helium ... Not sure why I need to do it.
        push_alloc_frame();

        // From six_typed_value_locals() macro
        m_impl.count = COUNT;
        m_impl.link = thread_local( current_locals_frame );
        for( int i = 0; i < m_impl.count; ++i )
            m_impl.vals[i] = NULL;

        thread_local( current_locals_frame ) = (Value**)&m_impl;
    }

    ~mxs_local_tuple() {
        // I'm not sure how exceptions and alloc frames are supposed to work together ...
        // if( !std::uncaught_exception() ){
        thread_local( current_locals_frame ) = m_impl.link; // From return_value() macro
        pop_alloc_frame();
        //}
    }

    template <int N>
    typename ptr_type<N>::type get() {
        return static_cast<typename ptr_type<N>::type>( m_impl.vals[N] );
    }

    template <int N>
    typename ptr_type<N>::type move_to_heap() {
        if( m_impl.vals[N] != NULL )
            m_impl.vals[N] = m_impl.vals[N]->get_heap_ptr();
        return get<N>();
    }

    template <int N>
    void set( typename ptr_type<N>::type val ) {
        m_impl.vals[N] = val;
    }

    template <int N>
    typename ptr_type<N>::type get_as_return_value() {
        // From return_protected() macro
        typename ptr_type<N>::type result = move_to_heap<N>();
        thread_local( current_result ) = result;
        return result;
    }
};

// TODO: Allow this frame to act like a vector.
template <int N>
class frame {
    int _count;
#if MAX_VERSION_MAJOR < 19
    Value** _impl;
#else
    ScopedValueTempArray _impl;
#endif

  public:
    frame()
        :
#if MAX_VERSION_MAJOR < 19
        _impl( NULL )
        ,
#else
        _impl( N )
        ,
#endif
        _count( 0 ) {
        init_thread_locals();
        push_alloc_frame();
#if MAX_VERSION_MAJOR < 19
        value_temp_array( _impl, N );
#endif
    }

    ~frame() {
#if MAX_VERSION_MAJOR < 19
        pop_value_temp_array( _impl );
#endif
        pop_alloc_frame();
    }

    // Do NOT call this directly! This is used by local's constructor.
    Value*& next_local() {
        if( _count == N )
            throw std::runtime_error( "mxs.frame.link: Index out of bounds [" + boost::lexical_cast<std::string>( N ) +
                                      "]" );
        return _impl[_count++];
    }
};

template <class T>
class local {
    Value*& _pptr;

#if MAX_VERSION_MAJOR < 15
    local( const local& rhs );
#endif
    local& operator=( const local& rhs );

  public:
    template <int N>
    local( frame<N>& f, Value* initialValue = NULL )
        : _pptr( f.next_local() ) {
        _pptr = initialValue;
    }
#if MAX_VERSION_MAJOR >= 15
    local( const local& rhs )
        : _pptr( rhs._pptr ) {}
#endif
    local& operator=( T* v ) {
        _pptr = v;
        return *this;
    }

    T* ptr() { return static_cast<T*>( _pptr ); }

    operator bool() { return ptr() != NULL; }
    operator void*() { return ptr(); }
    operator T*() { return ptr(); }
    T* operator->() { return ptr(); }
};

inline frantic::tstring to_string( const MAXScriptException& e ) {
    frame<1> f;
    local<StringStream> stream( f );

    stream = new StringStream;
    const_cast<MAXScriptException&>( e ).sprin1( stream );

    return stream->to_string();
}

inline frantic::tstring to_string( Value* v ) {
    frame<1> f;
    local<StringStream> stream( f );

    stream = new StringStream;
    v->sprin1( stream );

    return stream->to_string();
}

namespace detail {
class scoped_stream {
    CharStream* _stream;

  public:
    scoped_stream( CharStream* stream )
        : _stream( stream ) {}
    ~scoped_stream() { _stream->close(); }
};

inline Value* evaluate( CharStream* stream ) {
    mxs::frame<3> f;
    mxs::local<Parser> parser( f );
    mxs::local<Value> code( f );
    mxs::local<Value> result( f );

    // DEBUG version of max was hitting breakpoints all over the place if I didn't push an alloc frame.
    init_thread_locals();
    push_alloc_frame();

    try {
        parser = new Parser( thread_local( current_stdout ) );
#if MAX_VERSION_MAJOR < 24
        code = parser->compile_all( stream );
#else
        code = parser->compile_all( stream, MAXScript::ScriptSource::NotSpecified );
#endif
        result = code->eval();

        if( result == NULL )
            result = &undefined;
        else {
            // Migrate to the heap if on the stack since we are about to pop the alloc frame.
            result = result->get_heap_ptr();
        }
    } catch( MAXScriptException& e ) {
        throw std::runtime_error( frantic::strings::to_string( to_string( e ) ) );
    }

    pop_alloc_frame();
#if MAX_VERSION_MAJOR >= 19
    return_value( result );
#else
    return_protected( result );
#endif
}

class scoped_global {
    frame<2> _frame;
    local<Value> _name;
    local<Value> _prev;

  public:
    scoped_global( Value* name, Value* value )
        : _name( _frame )
        , _prev( _frame ) {
        _name = name;
        _prev = globals->get( _name );
        if( _prev )
            globals->set( _name, value );
        else
            globals->put_new( _name, value );
    }

    ~scoped_global() {
        if( _prev )
            globals->set( _name, _prev );
        else
            globals->remove( _name );
    }
};
} // namespace detail

// If you want the raw Max Value* object, use file_in<Value*>(...);
template <class T>
inline T file_in( const frantic::tstring& path ) {
    if( !files::file_exists( path ) )
        throw std::runtime_error( "mxs::file_in() - Tried to execute a maxscript file which doesn't exist: \"" +
                                  frantic::strings::to_string( path ) + "\"" );

    mxs::frame<2> f;
    mxs::local<FileStream> stream( f );
    mxs::local<Value> result( f );

    stream = new FileStream;
    stream->open( const_cast<TCHAR*>( path.c_str() ), _T("r") );

    detail::scoped_stream ss( stream );

    result = detail::evaluate( stream );
    return fpwrapper::MaxTypeTraits<T>::FromValue( result );
}

template <>
inline void file_in( const frantic::tstring& path ) {
    file_in<Value*>( path );
}

inline Value* get_struct_value( Struct* theStruct, TCHAR* varName ) {
#if MAX_VERSION_MAJOR < 12
    Value* index = theStruct->definition->members->get( Name::intern( varName ) );
    if( index && is_int( index ) )
        return theStruct->member_data[index->to_int()];
#else
    return theStruct->definition->get_member_value( Name::intern( varName ) );
#endif

    return 0;
}

namespace detail {
/**
 * This will verify that a MXS Value* is an Array object, and has the specified number of elements.
 * @param pMXSvalue The Value* that we expect to be an Array
 * @param expectedCount the expected number of items in the Array
 * @return The passed pMXSValue cast to a MXS Array*
 */
inline Array* get_mxs_array( Value* pMXSValue, int expectedCount ) {
    if( !pMXSValue || !is_array( pMXSValue ) )
        throw std::runtime_error( "get_mxs_array() The MXS Value* passed was not an array" );

    Array* pMXSArray = static_cast<Array*>( pMXSValue );
    if( pMXSArray->size != expectedCount )
        throw std::runtime_error( "get_mxs_array() The MXS Array* had incorrect dimensions" );

    return pMXSArray;
}

/**
 * This struct is used in the template meta-program that visits each element of the Array, and converts
 * it to the I'th type specified in the TupleType
 * @tparam TupleType The boost::tuple that specifes that types of the Array, as well as storage locations for the
 * converted objects.
 * @tparam pMXSArray The MXS Array* that holds elements to convert to C++ types.
 */
template <typename TupleType, int I>
struct extract_tuple_impl {
    // Template meta-program for traversing the TupleType's list of types and extracting the associated Array* element
    // for each.
    static void eval( typename TupleType& outTuple, Array* pMXSArray ) {
        // Convert I into an index from 0 to length-1, instead of length to 1
        typedef std::integral_constant<int, boost::tuples::length<TupleType>::value - I> tuple_index_t; 

        if( !pMXSArray->data[tuple_index_t::value] )
            throw std::runtime_error( "extract_tuple() The MXS Array* had a NULL entry" );

        typedef boost::tuples::element<tuple_index_t::value, TupleType>::type T;  // Get the type for this tuple element
        typedef frantic::max3d::fpwrapper::MaxTypeTraits<T> TypeTraits; // Grab the MaxTypeTraits for the underlying
                                                                        // type

        outTuple.get<tuple_index_t::value>() = TypeTraits::FromValue( pMXSArray->data[tuple_index_t::value] );

        extract_tuple_impl<TupleType, I - 1>::eval( outTuple, pMXSArray );
    }
};

/**
 * @overload The base case to stop the recursive array iteration.
 */
template <class TupleType>
struct extract_tuple_impl<TupleType, 0> {
    // Base case to end recursion
    static void eval( TupleType& /*outTuple*/, Array* /*pMXSArray*/ ) {}
};
} // namespace detail

/**
 * This function will convert a MXS Array* into a boost::tuple with the specified types. This is useful since MXS
 * often uses an Array to pass around groups of heterogenous data.
 * @tparam T1 The expected type of the first element of the MXS Array*
 * @param pMXSValue Pointer to the MXS Value* that is an array of values with a single element.
 */
template <class T1>
inline boost::tuple<T1> extract_tuple( Value* pMXSValue ) {
    boost::tuple<T1> resultTuple;
    detail::extract_tuple_impl<boost::tuple<T1>, 1>::eval( resultTuple, detail::get_mxs_array( pMXSValue, 1 ) );
    return resultTuple;
}

/**
 * @overload for 2 element array
 */
template <class T1, class T2>
inline boost::tuple<T1, T2> extract_tuple( Value* pMXSValue ) {
    boost::tuple<T1, T2> resultTuple;
    detail::extract_tuple_impl<boost::tuple<T1, T2>, 2>::eval( resultTuple, detail::get_mxs_array( pMXSValue, 2 ) );
    return resultTuple;
}

/**
 * @overload for 3 element array
 */
template <class T1, class T2, class T3>
inline boost::tuple<T1, T2, T3> extract_tuple( Value* pMXSValue ) {
    boost::tuple<T1, T2, T3> resultTuple;
    detail::extract_tuple_impl<boost::tuple<T1, T2, T3>, 3>::eval( resultTuple, detail::get_mxs_array( pMXSValue, 3 ) );
    return resultTuple;
}

/**
 * @overload for 4 element array
 */
template <class T1, class T2, class T3, class T4>
inline boost::tuple<T1, T2, T3, T4> extract_tuple( Value* pMXSValue ) {
    boost::tuple<T1, T2, T3, T4> resultTuple;
    detail::extract_tuple_impl<boost::tuple<T1, T2, T3, T4>, 4>::eval( resultTuple,
                                                                       detail::get_mxs_array( pMXSValue, 4 ) );
    return resultTuple;
}

/**
 * @overload for 5 element array
 */
template <class T1, class T2, class T3, class T4, class T5>
inline boost::tuple<T1, T2, T3, T4, T5> extract_tuple( Value* pMXSValue ) {
    boost::tuple<T1, T2, T3, T4, T5> resultTuple;
    detail::extract_tuple_impl<boost::tuple<T1, T2, T3, T4, T5>, 5>::eval( resultTuple,
                                                                           detail::get_mxs_array( pMXSValue, 5 ) );
    return resultTuple;
}

#define FF_MXS_LOCALS_STRING _T("__franticMXSlocals__")

class expression {
    frame<2> _frame;
    local<CharStream> _stream;
    local<Array> _locals;

    TimeValue _savedTime;
    BOOL _savedUseTimeContext;
    CharStream* _savedCurrentStdout;

    Value* _outStream;
    frantic::tstring _script;

    // Disable copying. Perhaps re-enable this.
    expression( const expression& );
    expression& operator=( const expression& );

  public:
    expression( const frantic::tstring& script )
        : _stream( _frame )
        , _locals( _frame )
        , _script( script ) {
        _stream = new StringStream;
        _locals = new Array( 0 );

        _savedTime = thread_local( current_time );
        _savedUseTimeContext = thread_local( use_time_context );
        _savedCurrentStdout = thread_local( current_stdout );

        _stream->putch( _T( '(' ) );
    }

    ~expression() {
        _stream->close();
        thread_local( current_time ) = _savedTime;
        thread_local( use_time_context ) = _savedUseTimeContext;
        thread_local( current_stdout ) = _savedCurrentStdout;
    }

    expression& bind( const frantic::tstring& name, Value* val ) {
        _locals->append( val );
        _stream->printf( _T("local %s = ") FF_MXS_LOCALS_STRING _T("[%d];"), name.c_str(), _locals->size );
        return *this;
    }

    expression& bind( const frantic::tstring& name, ReferenceTarget* val ) {
        return bind( name, MAXClass::make_wrapper_for( val ) );
    }

    expression& at_time( TimeValue t ) {
        thread_local( current_time ) = t;
        thread_local( use_time_context ) = true;
        return *this;
    }

    expression& redirect_stdout( CharStream* out ) {
        thread_local( current_stdout ) = out;
        return *this;
    }

    /**
     * This function finishes construction of the expression, and runs the maxscript.
     * If all you want is the Value* object, use evaluate<Value*>().  If you
     * don't want any return value, use evaluate<void>(), which calls the void
     * specialized function.
     */
    template <class T>
    T evaluate() {
        frame<2> f;
        local<Value> name( f );
        local<Value> result( f );

        _stream->puts( const_cast<MCHAR*>( _script.c_str() ) );
        _stream->putch( _T( ')' ) );

        name = Name::intern( FF_MXS_LOCALS_STRING );
        detail::scoped_global sg( name, _locals );

        result = detail::evaluate( _stream.ptr() );

        try {
            return fpwrapper::MaxTypeTraits<T>::FromValue( result );
        } catch( const MAXScriptException& e ) {
            throw std::runtime_error( frantic::strings::to_string( to_string( e ) ) );
        }
    }

    template <>
    void evaluate() {
        evaluate<Value*>();
    }
};

#undef FF_MXS_LOCALS_STRING

} // namespace mxs
} // namespace max3d
} // namespace frantic
