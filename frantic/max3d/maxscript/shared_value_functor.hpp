// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/fpwrapper/max_typetraits.hpp>
#include <frantic/max3d/maxscript/shared_value_ptr.hpp>

namespace frantic {
namespace max3d {
namespace mxs {

template <class R, class P1 = void, class P2 = void, class P3 = void>
struct shared_value_functor;

template <>
struct shared_value_functor<void, void, void, void> {
    typedef void function_type( void );
    typedef void result_type;

    boost::shared_ptr<Value> m_MXSFun;

    shared_value_functor( boost::shared_ptr<Value> pMXSFun )
        : m_MXSFun( pMXSFun ) {}

    inline void operator()() const { m_MXSFun->apply( NULL, 0 ); }
};

template <class P1>
struct shared_value_functor<void, P1, void, void> {
    typedef void function_type( const P1& );
    typedef void result_type;

    boost::shared_ptr<Value> m_MXSFun;

    shared_value_functor( boost::shared_ptr<Value> pMXSFun )
        : m_MXSFun( pMXSFun ) {}

    inline void operator()( const P1& p1 ) const {
        Value** locals = NULL;
        value_local_array( locals, 1 );

        locals[0] = frantic::max3d::fpwrapper::MaxTypeTraits<P1>::to_value( p1 );

        m_MXSFun->apply( locals, 1 );
#if MAX_VERSION_MAJOR < 19
        pop_value_local_array( locals );
#endif
    }
};

template <class P1, class P2>
struct shared_value_functor<void, P1, P2, void> {
    typedef void function_type( const P1&, const P2& );
    typedef void result_type;

    boost::shared_ptr<Value> m_MXSFun;

    shared_value_functor( boost::shared_ptr<Value> pMXSFun )
        : m_MXSFun( pMXSFun ) {}

    inline void operator()( const P1& p1, const P2& p2 ) const {
        Value** locals = NULL;
        value_local_array( locals, 2 );

        locals[0] = frantic::max3d::fpwrapper::MaxTypeTraits<P1>::to_value( p1 );
        locals[1] = frantic::max3d::fpwrapper::MaxTypeTraits<P2>::to_value( p2 );

        m_MXSFun->apply( locals, 2 );

        pop_value_local_array( locals );
    }
};

template <class P1, class P2, class P3>
struct shared_value_functor<void, P1, P2, P3> {
    typedef void function_type( const P1&, const P2&, const P3& );
    typedef void result_type;

    boost::shared_ptr<Value> m_MXSFun;

    shared_value_functor( boost::shared_ptr<Value> pMXSFun )
        : m_MXSFun( pMXSFun ) {}

    inline void operator()( const P1& p1, const P2& p2, const P3& p3 ) const {
        Value** locals = NULL;
        value_local_array( locals, 3 );

        locals[0] = frantic::max3d::fpwrapper::MaxTypeTraits<P1>::to_value( p1 );
        locals[1] = frantic::max3d::fpwrapper::MaxTypeTraits<P2>::to_value( p2 );
        locals[2] = frantic::max3d::fpwrapper::MaxTypeTraits<P3>::to_value( p3 );

        m_MXSFun->apply( locals, 3 );

        pop_value_local_array( locals );
    }
};

template <class R>
struct shared_value_functor<R, void, void, void> {
    typedef R function_type( void );
    typedef R result_type;

    boost::shared_ptr<Value> m_MXSFun;

    shared_value_functor( boost::shared_ptr<Value> pMXSFun )
        : m_MXSFun( pMXSFun ) {}

    inline R operator()() const {
        Value* result = frantic::max3d::fpwrapper::MaxTypeTraits<R>::from_value( m_MXSFun->apply( NULL, 0 ) );

        return frantic::max3d::fpwrapper::MaxTypeTraits<R>::FromValue( result );
    }
};

template <class R, class P1>
struct shared_value_functor<R, P1, void, void> {
    typedef R function_type( const P1& );
    typedef R result_type;

    boost::shared_ptr<Value> m_MXSFun;

    shared_value_functor( boost::shared_ptr<Value> pMXSFun )
        : m_MXSFun( pMXSFun ) {}

    inline R operator()( const P1& p1 ) const {
        Value** locals = NULL;
        value_local_array( locals, 1 );

        locals[0] = frantic::max3d::fpwrapper::MaxTypeTraits<P1>::to_value( p1 );

        Value* result = m_MXSFun->apply( locals, 1 );

        pop_value_local_array( locals );

        return frantic::max3d::fpwrapper::MaxTypeTraits<R>::FromValue( result );
    }
};

template <class R, class P1, class P2>
struct shared_value_functor<R, P1, P2, void> {
    typedef R function_type( const P1&, const P2& );
    typedef R result_type;

    boost::shared_ptr<Value> m_MXSFun;

    shared_value_functor( boost::shared_ptr<Value> pMXSFun )
        : m_MXSFun( pMXSFun ) {}

    inline R operator()( const P1& p1, const P2& p2 ) const {
        Value** locals = NULL;
        value_local_array( locals, 2 );

        locals[0] = frantic::max3d::fpwrapper::MaxTypeTraits<P1>::to_value( p1 );
        locals[1] = frantic::max3d::fpwrapper::MaxTypeTraits<P2>::to_value( p2 );

        Value* result = m_MXSFun->apply( locals, 2 );

        pop_value_local_array( locals );

        return frantic::max3d::fpwrapper::MaxTypeTraits<R>::FromValue( result );
    }
};

template <class R, class P1, class P2, class P3>
struct shared_value_functor {
    typedef R function_type( const P1&, const P2&, const P3& );
    typedef R result_type;

    boost::shared_ptr<Value> m_MXSFun;

    shared_value_functor( boost::shared_ptr<Value> pMXSFun )
        : m_MXSFun( pMXSFun ) {}

    inline R operator()( const P1& p1, const P2& p2, const P3& p3 ) const {
        Value** locals = NULL;
        value_local_array( locals, 2 );

        locals[0] = frantic::max3d::fpwrapper::MaxTypeTraits<P1>::to_value( p1 );
        locals[1] = frantic::max3d::fpwrapper::MaxTypeTraits<P2>::to_value( p2 );
        locals[2] = frantic::max3d::fpwrapper::MaxTypeTraits<P2>::to_value( p3 );

        Value* result = m_MXSFun->apply( locals, 2 );

        pop_value_local_array( locals );

        return frantic::max3d::fpwrapper::MaxTypeTraits<R>::FromValue( result );
    }
};

} // namespace mxs
} // namespace max3d
} // namespace frantic
