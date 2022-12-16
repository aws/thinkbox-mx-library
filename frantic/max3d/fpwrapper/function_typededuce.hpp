// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

// An attempt to deduce the return / parameter types of member function calls

namespace frantic {
namespace max3d {
namespace fpwrapper {

struct no_deduced_type {};

// To deduce the function arity
template <class FnT>
struct DeduceFnArity {
    // No arity of general function
};
// To deduce argument types
template <class FnT, int ArgNumber>
struct DeduceFnArguments {
    typedef no_deduced_type type;
};

// 0 argument function
template <class MainClass, class RT>
struct DeduceFnArity<RT ( MainClass::* )()> {
    enum { arity = 0 };
};
template <class MainClass, class RT, int ArgNumber>
struct DeduceFnArguments<RT ( MainClass::* )(), ArgNumber> {
    typedef void type;
};
template <class MainClass, class RT>
struct DeduceFnArguments<RT ( MainClass::* )(), 0> {
    typedef RT type;
};

// 1 argument function
template <class MainClass, class RT, class T0>
struct DeduceFnArity<RT ( MainClass::* )( T0 )> {
    enum { arity = 1 };
};
template <class MainClass, class RT, class T0, int ArgNumber>
struct DeduceFnArguments<RT ( MainClass::* )( T0 ), ArgNumber> {
    typedef void type;
};
template <class MainClass, class RT, class T0>
struct DeduceFnArguments<RT ( MainClass::* )( T0 ), 0> {
    typedef RT type;
};
template <class MainClass, class RT, class T0>
struct DeduceFnArguments<RT ( MainClass::* )( T0 ), 1> {
    typedef T0 type;
};

// 2 argument function
template <class MainClass, class RT, class T0, class T1>
struct DeduceFnArity<RT ( MainClass::* )( T0, T1 )> {
    enum { arity = 2 };
};
template <class MainClass, class RT, class T0, class T1, int ArgNumber>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1 ), ArgNumber> {
    typedef void type;
};
template <class MainClass, class RT, class T0, class T1>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1 ), 0> {
    typedef RT type;
};
template <class MainClass, class RT, class T0, class T1>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1 ), 1> {
    typedef T0 type;
};
template <class MainClass, class RT, class T0, class T1>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1 ), 2> {
    typedef T1 type;
};

// 3 argument function
template <class MainClass, class RT, class T0, class T1, class T2>
struct DeduceFnArity<RT ( MainClass::* )( T0, T1, T2 )> {
    enum { arity = 3 };
};
template <class MainClass, class RT, class T0, class T1, class T2, int ArgNumber>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2 ), ArgNumber> {
    typedef void type;
};
template <class MainClass, class RT, class T0, class T1, class T2>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2 ), 0> {
    typedef RT type;
};
template <class MainClass, class RT, class T0, class T1, class T2>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2 ), 1> {
    typedef T0 type;
};
template <class MainClass, class RT, class T0, class T1, class T2>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2 ), 2> {
    typedef T1 type;
};
template <class MainClass, class RT, class T0, class T1, class T2>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2 ), 3> {
    typedef T2 type;
};

// 4 argument function
template <class MainClass, class RT, class T0, class T1, class T2, class T3>
struct DeduceFnArity<RT ( MainClass::* )( T0, T1, T2, T3 )> {
    enum { arity = 4 };
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, int ArgNumber>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3 ), ArgNumber> {
    typedef void type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3 ), 0> {
    typedef RT type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3 ), 1> {
    typedef T0 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3 ), 2> {
    typedef T1 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3 ), 3> {
    typedef T2 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3 ), 4> {
    typedef T3 type;
};

// 5 argument function
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4>
struct DeduceFnArity<RT ( MainClass::* )( T0, T1, T2, T3, T4 )> {
    enum { arity = 5 };
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, int ArgNumber>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4 ), ArgNumber> {
    typedef void type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4 ), 0> {
    typedef RT type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4 ), 1> {
    typedef T0 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4 ), 2> {
    typedef T1 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4 ), 3> {
    typedef T2 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4 ), 4> {
    typedef T3 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4 ), 5> {
    typedef T4 type;
};

// 6 argument function
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5>
struct DeduceFnArity<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5 )> {
    enum { arity = 6 };
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5, int ArgNumber>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5 ), ArgNumber> {
    typedef void type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5 ), 0> {
    typedef RT type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5 ), 1> {
    typedef T0 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5 ), 2> {
    typedef T1 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5 ), 3> {
    typedef T2 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5 ), 4> {
    typedef T3 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5 ), 5> {
    typedef T4 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5 ), 6> {
    typedef T5 type;
};

// 7 argument function
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct DeduceFnArity<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6 )> {
    enum { arity = 7 };
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5, class T6,
          int ArgNumber>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6 ), ArgNumber> {
    typedef void type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6 ), 0> {
    typedef RT type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6 ), 1> {
    typedef T0 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6 ), 2> {
    typedef T1 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6 ), 3> {
    typedef T2 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6 ), 4> {
    typedef T3 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6 ), 5> {
    typedef T4 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6 ), 6> {
    typedef T5 type;
};
template <class MainClass, class RT, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
struct DeduceFnArguments<RT ( MainClass::* )( T0, T1, T2, T3, T4, T5, T6 ), 7> {
    typedef T6 type;
};

} // namespace fpwrapper
} // namespace max3d
} // namespace frantic
