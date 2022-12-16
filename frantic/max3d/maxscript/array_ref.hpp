// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace frantic {
namespace max3d {
namespace fpwrapper {

class ArrayRefClass : public ValueMetaClass {
  public:
    ArrayRefClass( MCHAR* name )
        : ValueMetaClass( name ) {}
    void collect() { delete this; }
};

extern ArrayRefClass ArrayRef_class;

class ArrayRef : public Value {
    void* m_tabPtr;
    int m_tabCount;
    Value* ( *m_get )( void*, int );       // The get function knows the correct type of the data
    void ( *m_set )( void*, int, Value* ); // The set function knows the correct type of the data

  public:
    template <class T>
    ArrayRef( Tab<T>& t );

    template <class T>
    ArrayRef( std::vector<T>& t );

    template <class T>
    ArrayRef( T* pData, int count );

#pragma warning( push ) // mute unreferenced formal param warnings from the MAX macro
#pragma warning( disable : 4100 )
    classof_methods( ArrayRef, Value );
#pragma warning( pop )

    ValueMetaClass* local_base_class() { return &ArrayRef_class; }

    void collect() { delete this; }
    void sprin1( CharStream* s );

    Value* get_count( Value** arg_list, int count );
    Value* get_vf( Value** arg_list, int count );
    Value* put_vf( Value** arg_list, int count );
};

template <class T>
struct ArrayRefImpl {
    static Value* get( void* pData, int index ) {
        return max_traits<T>::to_value( *( static_cast<T*>( pData ) + index ) );
    }
    static void set( void* pData, int index, Value* v ) {
        *( static_cast<T*>( pData ) + index ) = max_traits<T>::from_value( v );
    }
};

template <class T>
ArrayRef::ArrayRef( Tab<T>& t ) {
    m_tabPtr = t.Addr( 0 );
    m_tabCount = t.Count();
    m_get = &ArrayRefImpl<T>::get;
    m_set = &ArrayRefImpl<T>::set;
}

template <class T>
ArrayRef::ArrayRef( std::vector<T>& v ) {
    m_tabPtr = &v[0];
    m_tabCount = static_cast<int>( v.size() );
    m_get = &ArrayRefImpl<T>::get;
    m_set = &ArrayRefImpl<T>::set;
}

template <class T>
ArrayRef::ArrayRef( T* pData, int count ) {
    m_tabPtr = pData;
    m_tabCount = count;
    m_get = &ArrayRefImpl<T>::get;
    m_set = &ArrayRefImpl<T>::set;
}

} // namespace fpwrapper
} // namespace max3d
} // namespace frantic
