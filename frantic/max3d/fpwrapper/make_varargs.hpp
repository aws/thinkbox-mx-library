// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

// #define MAKE_VARARGS_DEBUG "c:/varargs.txt"

#ifdef MAKE_VARARGS_DEBUG
#include <typeinfo>
#endif

#include <fstream>

// This class provides a way to construct a varargs list
namespace frantic {
namespace max3d {
namespace fpwrapper {

class make_varargs {
    char* m_data;
    int m_capacity;
    char* m_currentPos;
#ifdef MAKE_VARARGS_DEBUG
    std::ofstream m_debug_out;
#endif

    // makes sure there's room to add at least this much data
    void ensure_add_capacity( int addSize ) {
        if( m_capacity - size() < addSize ) {
            // Create the new array
            int newCapacity = m_capacity * 3 / 2;
            char* newData = new char[newCapacity];
            memcpy( newData, m_data, m_capacity );

            // Set the values
            m_currentPos = newData + ( m_currentPos - m_data );
            m_data = newData;
            m_capacity = newCapacity;
        }
    }

  public:
    make_varargs( int startCapacity = 1024 ) {
        m_capacity = startCapacity;
        m_data = new char[m_capacity];
        m_currentPos = m_data;
#ifdef MAKE_VARARGS_DEBUG
        m_debug_out.open( MAKE_VARARGS_DEBUG );
#endif
    }
    make_varargs( const make_varargs& rhs ) {
        m_capacity = rhs.m_capacity;
        m_data = new char[m_capacity];
        memcpy( m_data, rhs.m_data, m_capacity );
        m_currentPos = m_data + ( rhs.m_currentPos - rhs.m_data );
    }
    ~make_varargs() {
#ifdef MAKE_VARARGS_DEBUG
        dump( MAKE_VARARGS_DEBUG ".bin" );
#endif

        delete m_data;
        m_data = 0;
        m_currentPos = 0;
    }

    void dump( const char* file ) {
        std::ofstream fout( file, std::ios::binary );
        fout.write( get(), size() );
    }

    int size() const { return static_cast<int>( m_currentPos - m_data ); }

    // WARNING: non-POD types are dangerous here, and will cause silent weirdness.  Should add an is_pod<>
    // meta-programming check.
    template <class T>
    void add_values( T value ) {
        ensure_add_capacity( sizeof( value ) + 16 );
        char* savedCurrentPos = m_currentPos;
        // use the varargs stuff to figure it out
        va_arg( m_currentPos, T );
        // Zero the memory we're putting it at
        memset( savedCurrentPos, 0, m_currentPos - savedCurrentPos );
        // Copy the memory values
        memcpy( savedCurrentPos, &value, sizeof( value ) );
#ifdef MAKE_VARARGS_DEBUG
        m_debug_out << typeid( value ).name() << ": " << value << std::endl;
#endif
    }

    // the FP interface complains a lot about the short parameters, so I'm doing them as int's
    void add_values( short value ) { add_values( int( value ) ); }

    template <class T0, class T1>
    void add_values( T0 value0, T1 value1 ) {
        add_values( value0 );
        add_values( value1 );
    }

    template <class T0, class T1, class T2>
    void add_values( T0 value0, T1 value1, T2 value2 ) {
        add_values( value0 );
        add_values( value1 );
        add_values( value2 );
    }

    template <class T0, class T1, class T2, class T3>
    void add_values( T0 value0, T1 value1, T2 value2, T3 value3 ) {
        add_values( value0 );
        add_values( value1 );
        add_values( value2 );
        add_values( value3 );
    }

    template <class T0, class T1, class T2, class T3, class T4>
    void add_values( T0 value0, T1 value1, T2 value2, T3 value3, T4 value4 ) {
        add_values( value0 );
        add_values( value1 );
        add_values( value2 );
        add_values( value3 );
        add_values( value4 );
    }

    template <class T0, class T1, class T2, class T3, class T4, class T5>
    void add_values( T0 value0, T1 value1, T2 value2, T3 value3, T4 value4, T5 value5 ) {
        add_values( value0 );
        add_values( value1 );
        add_values( value2 );
        add_values( value3 );
        add_values( value4 );
        add_values( value5 );
    }

    template <class T0, class T1, class T2, class T3, class T4, class T5, class T6>
    void add_values( T0 value0, T1 value1, T2 value2, T3 value3, T4 value4, T5 value5, T6 value6 ) {
        add_values( value0 );
        add_values( value1 );
        add_values( value2 );
        add_values( value3 );
        add_values( value4 );
        add_values( value5 );
        add_values( value6 );
    }

    template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7>
    void add_values( T0 value0, T1 value1, T2 value2, T3 value3, T4 value4, T5 value5, T6 value6, T7 value7 ) {
        add_values( value0 );
        add_values( value1 );
        add_values( value2 );
        add_values( value3 );
        add_values( value4 );
        add_values( value5 );
        add_values( value6 );
        add_values( value7 );
    }

    template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
    void add_values( T0 value0, T1 value1, T2 value2, T3 value3, T4 value4, T5 value5, T6 value6, T7 value7,
                     T8 value8 ) {
        add_values( value0 );
        add_values( value1 );
        add_values( value2 );
        add_values( value3 );
        add_values( value4 );
        add_values( value5 );
        add_values( value6 );
        add_values( value7 );
        add_values( value8 );
    }

    template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
    void add_values( T0 value0, T1 value1, T2 value2, T3 value3, T4 value4, T5 value5, T6 value6, T7 value7, T8 value8,
                     T9 value9 ) {
        add_values( value0 );
        add_values( value1 );
        add_values( value2 );
        add_values( value3 );
        add_values( value4 );
        add_values( value5 );
        add_values( value6 );
        add_values( value7 );
        add_values( value8 );
        add_values( value9 );
    }

    char* get() const { return m_data; }
};

} // namespace fpwrapper
} // namespace max3d
} // namespace frantic
