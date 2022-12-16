// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <frantic/max3d/operators.hpp>
#include <frantic/strings/tstring.hpp>

#include <boost/shared_array.hpp>

#include <ifnpub.h>
#if MAX_RELEASE >= 25000
#include <geom/ipoint3.h>
#else
#include <ipoint3.h>
#endif

// The operators get defined in the global namespace

// ---- Output operators

inline std::ostream& operator<<( std::ostream& out, const Point3& pt ) {
    out << "[" << pt.x << "," << pt.y << "," << pt.z << "]";
    return out;
}

// TODO: In max 6, calling this operator causes a crash.  A confusing crash, because it never actually
//       enters this function.
inline std::ostream& operator<<( std::ostream& out, const IPoint3& pt ) {
    /*
    TraceFn();
    TraceVar( &out );
    TraceVar( &pt );
    TraceVar( pt.x );
    TraceVar( pt.y );
    TraceVar( pt.z );
    */
    out << "[" << pt.x << "," << pt.y << "," << pt.z << "]";
    // out << "!!!???!!!";
    return out;
}

// inline std::string toString(const Matrix3 & mat)
//{
//	ostringstream ostr;
//	ostr << mat;
//	return ostr.str();
// }

inline std::ostream& operator<<( std::ostream& out, const Box3& box ) {
    Point3 min = box.Min();
    Point3 max = box.Max();
    out << "[" << min.x << "," << min.y << "," << min.z << "]-";
    out << "[" << max.x << "," << max.y << "," << max.z << "]";

    return out;
}

inline std::ostream& operator<<( std::ostream& out, const Matrix3& mat ) {
    out << "matrix [ ";
    for( int r = 0; r < 4; ++r ) {
        Point3 row = mat.GetRow( r );
        out << row.x << " " << row.y << " " << row.z << "  ";
    }
    out << "]";
    return out;
}

inline std::string toMaxscriptFormat( const Matrix3& mat ) {
    std::stringstream ss;
    ss << "Matrix3";
    for( int r = 0; r < 4; ++r ) {
        Point3 row = mat.GetRow( r );
        ss << " [" << row.x << ", " << row.y << ", " << row.z << "]";
    }
    return ss.str();
}

inline void write_TimeValue( std::ostream& out, TimeValue t ) {
    if( t == TIME_NegInfinity )
        out << "-inf";
    else if( t == TIME_PosInfinity )
        out << "+inf";
    else
        out << float( t ) / TIME_TICKSPERSEC << 's';
}

inline std::ostream& operator<<( std::ostream& out, const Interval& ivl ) {
    if( ivl == FOREVER )
        out << "[ FOREVER ]";
    else if( ivl == NEVER )
        out << "[ NEVER ]";
    else {
        out << "[ ";
        write_TimeValue( out, ivl.Start() );
        out << ", ";
        write_TimeValue( out, ivl.End() );
        out << " ]";
    }

    return out;
}

inline std::ostream& operator<<( std::ostream& out, const Class_ID& cid ) {
    std::stringstream stmp;
    stmp << std::hex;
    stmp << "Class_ID(0x" << const_cast<Class_ID*>( &cid )->PartA();
    stmp << ", 0x" << const_cast<Class_ID*>( &cid )->PartB() << ")";

    out << stmp.str();

    return out;
}

// ----- Input operators

inline std::istream& operator>>( std::istream& in, Point3& pt ) {
    char ch;

    in >> ch;

    if( ch == '[' || ch == '(' ) {
        in >> pt.x;
        if( !in.good() )
            return in;

        in >> ch;
        if( in.good() && ch != ',' )
            in.putback( ch );

        in >> pt.y;
        if( !in.good() )
            return in;

        in >> ch;
        if( in.good() && ch != ',' )
            in.putback( ch );

        in >> pt.z;
        if( !in.good() )
            return in;

        in >> ch;
        // if( in != ')' && in != ']' ) {}

    } else {
        in.putback( ch );
        in.setstate( ::std::ios_base::failbit );
    }

    return in;
}

inline std::istream& operator>>( std::istream& in, IPoint3& pt ) {
    char ch;

    in >> ch;

    if( ch == '[' || ch == '(' ) {
        in >> pt.x;
        if( !in.good() )
            return in;

        in >> ch;
        if( in.good() && ch != ',' )
            in.putback( ch );

        in >> pt.y;
        if( !in.good() )
            return in;

        in >> ch;
        if( in.good() && ch != ',' )
            in.putback( ch );

        in >> pt.z;
        if( !in.good() )
            return in;

        in >> ch;
        // if( in != ')' && in != ']' ) {}

    } else {
        in.putback( ch );
        in.setstate( ::std::ios_base::failbit );
    }

    return in;
}

namespace frantic {
namespace max3d {
namespace detail {

enum {
    kNameChunk = 5, // This is arbitrary
    kValueChunk,
};

#if MAX_VERSION_MAJOR >= 15

// Starting in 3ds Max 2013 (MAX_VERSION_MAJOR 15), ISave and ILoad have a
// CodePage() function that indicates which code page should be used to
// save or load Unicode strings.
// The following to_string() and to_tstring() functions are used to convert
// to and from the CodePage().
inline std::string to_max_file_string( const std::wstring& s, ISave* isave ) {
    return TSTR::FromUTF16( s.c_str() ).ToCP( isave->CodePage() ).data();
}
inline std::wstring tstring_from_max_file_string( const std::string& s, ILoad* iload ) {
    return TSTR::FromCP( iload->CodePage(), s.c_str() ).ToUTF16().data();
}

#else

// In earlier versions of 3ds Max there is no CodePage() member, so we
// pass the string through unchanged.
inline const std::string& to_max_file_string( const std::string& s, ISave* /*isave*/ ) { return s; }
inline const std::string& tstring_from_max_file_string( const std::string& s, ILoad* /*load*/ ) { return s; }

#endif

inline IOResult isave_write_prop( ISave* isave, const std::map<frantic::tstring, FPValue>::value_type& val ) {
    IOResult result;

    isave->BeginChunk( kNameChunk );
    result = isave->WriteCString( val.first.c_str() );
    if( result != IO_OK )
        return result;
    isave->EndChunk();

    isave->BeginChunk( kValueChunk );
    result = const_cast<FPValue&>( val.second ).Save( isave );
    if( result != IO_OK )
        return result;
    isave->EndChunk();

    return IO_OK;
}

inline IOResult isave_write_prop( ISave* isave, const FPValue& val ) {
    IOResult result;

    result = const_cast<FPValue&>( val ).Save( isave );
    if( result != IO_OK )
        return result;

    return IO_OK;
}

template <class Container>
struct iload_read_propmap_impl {
    inline static IOResult apply( ILoad* iload, Container& outContainer ) {
        IOResult result;

        FPValue val;

        result = iload->OpenChunk();
        while( result == IO_OK ) {
            result = val.Load( iload );
            if( result != IO_OK )
                return IO_ERROR;

            outContainer.push_back( val );

            iload->CloseChunk();
            result = iload->OpenChunk();
        }

        if( result != IO_END )
            return IO_ERROR;

        return IO_OK;
    }
};

template <template <class, class> class Container, class Allocator>
struct iload_read_propmap_impl<Container<std::pair<frantic::tstring, FPValue>, Allocator>> {
    inline static IOResult apply( ILoad* iload,
                                  Container<std::pair<frantic::tstring, FPValue>, Allocator>& outContainer ) {
        return iload_read_propmap_keyvalue( iload, outContainer );
    }
};

template <class Container>
inline IOResult iload_read_propmap_keyvalue( ILoad* iload, Container& outContainer ) {
    IOResult result;

    TCHAR* szName;
    typename Container::value_type prop;

    result = iload->OpenChunk();
    while( result == IO_OK ) {
        // For each property

        result = iload->OpenChunk();
        while( result == IO_OK ) {
            // Will loop through detail::kNameChunk, and detail::kValueChunk.

            switch( iload->CurChunkID() ) {
            case detail::kNameChunk:
                result = iload->ReadCStringChunk( &szName );
                if( result != IO_OK )
                    return IO_ERROR;
                const_cast<frantic::tstring&>( prop.first ) =
                    szName; // Need a const cast for std::map<std::string,FPValue>::value_type
                break;
            case detail::kValueChunk:
                result = prop.second.Load( iload );
                if( result != IO_OK )
                    return IO_ERROR;
                break;
            default:
                return IO_ERROR;
            }

            iload->CloseChunk();
            result = iload->OpenChunk();
        }

        if( result != IO_END )
            return IO_ERROR;

        outContainer.insert( outContainer.end(), prop );

        iload->CloseChunk();
        result = iload->OpenChunk();
    }

    if( result != IO_END )
        return IO_ERROR;

    return IO_OK;
}

} // end of namespace detail

template <class ForwardIterator>
inline IOResult isave_write_propmap( ISave* isave, ForwardIterator itStart, ForwardIterator itEnd ) {
    IOResult result;
    USHORT counter = 0;

    for( ForwardIterator it = itStart; it != itEnd; ++it, ++counter ) {
        isave->BeginChunk( counter );
        result = detail::isave_write_prop( isave, *it );
        if( result != IO_OK )
            return result;
        isave->EndChunk();
    }

    return IO_OK;
}

template <class Container>
inline IOResult iload_read_propmap( ILoad* iload, Container& outContainer ) {
    return detail::iload_read_propmap_impl<Container>::apply( iload, outContainer );
}

/**
 * Overload for reading key-value pairs into a map, without explicitly calling iload_read_propmap() instead of
 * iload_read_propmap().
 */
inline IOResult iload_read_propmap( ILoad* iload, std::map<frantic::tstring, FPValue>& outContainer ) {
    return detail::iload_read_propmap_keyvalue( iload, outContainer );
}

inline IOResult isave_write_strmap( ISave* isave, const std::map<frantic::tstring, frantic::tstring>& tstringMap ) {
    ULONG nb = 0;
    // Compute the # of bytes needed to save this
    // unsigned - totalSize
    // int / string / int / string  - size / i->first / size / i->second
#if MAX_VERSION_MAJOR >= 15
    // Starting in 3ds Max 2013, ISave and ILoad have a CodePage() function that indicates
    // which code page should be used to save or load Unicode strings.
    // Here we build a new map m that holds strings encoded using the isave->CodePage().
    std::map<std::string, std::string> m;
    for( std::map<frantic::tstring, frantic::tstring>::const_iterator i = tstringMap.begin(); i != tstringMap.end();
         ++i ) {
        m[detail::to_max_file_string( i->first, isave )] = detail::to_max_file_string( i->second, isave );
    }
#else
    const std::map<std::string, std::string>& m = tstringMap;
#endif

    std::size_t totalSize = sizeof( unsigned );
    std::map<std::string, std::string>::const_iterator i;
    for( i = m.begin(); i != m.end(); ++i ) {
        totalSize += 2 * sizeof( int );
        totalSize += i->first.size();
        totalSize += i->second.size();
    }
    boost::shared_array<char> buf( new char[totalSize] );

    char* bufPtr = buf.get();

    *( (int*)bufPtr ) = (int)totalSize;
    bufPtr += sizeof( unsigned );

    for( i = m.begin(); i != m.end(); ++i ) {
        std::size_t strSize = i->first.size();
        *( (int*)bufPtr ) = (int)strSize;
        bufPtr += sizeof( int );
        memcpy( bufPtr, i->first.data(), strSize );
        bufPtr += strSize;

        strSize = i->second.size();
        *( (int*)bufPtr ) = (int)strSize;
        bufPtr += sizeof( int );
        memcpy( bufPtr, i->second.data(), strSize );
        bufPtr += strSize;
    }

    if( bufPtr - buf.get() != (int)totalSize )
        throw std::runtime_error( "isave_write_strmap: Consistency error, wrote an unexpected number of bytes." );

    return isave->Write( buf.get(), (unsigned int)totalSize, &nb );
}

inline IOResult iload_read_strmap( ILoad* iload, std::map<frantic::tstring, frantic::tstring>& m ) {
    // For Amaretto, it is incorrect to clear the map here as the map is initialized with
    // default values when the constructor is called.  I assume this will be the same for
    // any other plugin that utilizes this function. -PTF
    // m.clear();
    std::size_t totalSize = (std::size_t)iload->CurChunkLength();
    ULONG nb = 0;

    boost::shared_array<char> buf( new char[totalSize] );
    IOResult res = iload->Read( buf.get(), (ULONG)totalSize, &nb );
    if( res != IO_OK )
        return res;

    char* bufPtr = buf.get();
    char* bufPtrEnd = buf.get() + totalSize;

    assert( *( (unsigned*)bufPtr ) == totalSize );
    bufPtr += sizeof( unsigned );

    while( bufPtr < bufPtrEnd ) {
        int firstLen = *( (int*)bufPtr );
        bufPtr += sizeof( int );
        assert( bufPtr + firstLen <= bufPtrEnd );
        std::string first( bufPtr, firstLen );
        bufPtr += firstLen;

        int secondLen = *( (int*)bufPtr );
        bufPtr += sizeof( int );
        assert( bufPtr + secondLen <= bufPtrEnd );
        std::string second( bufPtr, secondLen );
        bufPtr += secondLen;

        m[detail::tstring_from_max_file_string( first, iload )] = detail::tstring_from_max_file_string( second, iload );
    }

    return res;
}

inline IOResult isave_write_string( ISave* isave, const std::string& str ) {
    ULONG nb = 0;
    // Compute the # of bytes needed to save this
    // int / string
    std::size_t totalSize = sizeof( str.size() );
    totalSize += str.size();

    boost::shared_array<char> buf( new char[totalSize] );

    char* bufPtr = buf.get();

    std::size_t strSize = str.size();
    *( (int*)bufPtr ) = (int)strSize;
    bufPtr += sizeof( int );
    memcpy( bufPtr, str.data(), strSize );
    bufPtr += strSize;

    if( bufPtr - buf.get() != (int)totalSize )
        throw std::runtime_error( "isave_write_string: Consistency error, wrote wrote an unexpected number of bytes." );

    return isave->Write( buf.get(), (unsigned long)totalSize, &nb );
}

inline IOResult iload_read_string( ILoad* iload, std::string& str ) {
    str.clear();
    std::size_t totalSize = (std::size_t)iload->CurChunkLength();
    ULONG nb = 0;

    boost::shared_array<char> buf( new char[totalSize] );
    IOResult res = iload->Read( buf.get(), (unsigned long)totalSize, &nb );
    if( res != IO_OK )
        return res;

    char* bufPtr = buf.get();
    //	char* bufPtrEnd = buf.get() + totalSize;

    int len = *( (int*)bufPtr );
    bufPtr += sizeof( int );
    //	assert( bufPtr + len <= bufPtrEnd );
    str = std::string( bufPtr, len );
    bufPtr += len;

    //	assert( bufPtr == bufPtrEnd );

    return res;
}

#ifdef FRANTIC_USING_DOTNET

typedef char dotnet_bytearray __gc[];

inline IOResult isave_write_bytearray( ISave* isave, dotnet_bytearray data ) {
    TraceFn();
    DWORD nb;
    char __pin* pinnedData = &( data[0] );

    IOResult res = isave->Write( pinnedData, data->Length, &nb );
    if( nb != data->Length ) {
        //		TraceLine( "isave->Write failed" );
        //		TraceVar( data->Length );
        //		TraceVar( nb );
        return IO_ERROR;
    }

    return res;
}

inline IOResult iload_read_bytearray( ILoad* iload, dotnet_bytearray* m ) {
    TraceFn();
    DWORD nb;
    ULONG totalSize = iload->CurChunkLength();
    *m = new char __gc[totalSize];
    char __pin* pinnedM = &( ( *m )[0] );
    IOResult res = iload->Read( pinnedM, totalSize, &nb );

    if( nb != totalSize ) {
        //		TraceLine( "iload->Read failed" );
        //		TraceVar( totalSize );
        //		TraceVar( nb );
        return IO_ERROR;
    }

    return res;
}

#endif

} // namespace max3d
} // namespace frantic
