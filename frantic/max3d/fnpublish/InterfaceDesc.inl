// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
/**
 * Contains the inline implementations of the classes and functions defined in InterfaceDesc.hpp. This file is included at the end of InterfaceDesc.hpp and should not be included directly by users. 
 * Don't be alarmed by this style, it is commonly used when implementations must be in headers (ie. templates and inline stuff) but you want to separate declaration and implementation files. Boost
 * uses .ipp extensions, while MSVC recognizes .inl extensions and assigns an appropriate icon in the Solution Explorer window.
 */
#pragma once

#include <frantic/max3d/fnpublish/InterfaceDesc.hpp>

#pragma warning( push, 3 )
#include <ifnpub.h>
#pragma warning( pop )

#include <boost/bind.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_enum.hpp>

#include <memory>

#if MAX_VERSION_MAJOR < 15
#define p_end end
#endif

namespace frantic{ namespace max3d{ namespace fnpublish{

/**
 * This partial specialization is enabled when an enumeration type is used, since each declared enumeration is a distinct type but they are all published via TYPE_ENUM to 3ds Max.
 */
template<>
struct Traits< TimeParameter >{
	typedef TimeParameter param_type;
	typedef TimeParameter return_type;

	static int fp_param_type(){ return TYPE_TIMEVALUE; }
	static int fp_return_type(){ return TYPE_TIMEVALUE; }

	static param_type get_parameter( FPValue& fpValue ){ return TimeParameter( TYPE_TIMEVALUE_FIELD( fpValue ) ); }
	static void get_return_value( FPValue& fpOutValue, const TimeParameter& val ){ return fpOutValue.LoadPtr( TYPE_TIMEVALUE, TYPE_TIMEVALUE_RSLT static_cast<TimeValue>(val) ); }
};

template <class T>
inline InterfaceDesc<T>::InterfaceDesc( Interface_ID id, const MCHAR* szName, StringResID i18nDesc, ClassDesc* cd, ULONG flags )
	: ::FPInterfaceDesc( id, const_cast<MCHAR*>(szName), i18nDesc, cd, flags, p_end )
{}

template <class T>
inline bool InterfaceDesc<T>::empty() const {
	return this->functions.Count() == 0 && this->props.Count() == 0;
}

template <class T>
inline FPStatus InterfaceDesc<T>::invoke_on(FunctionID fid, TimeValue t, T* pSelf, FPValue& result, FPParams* p){
	if( static_cast<std::size_t>(fid) < m_dispatchFns.size() ){
		try{
			m_dispatchFns[fid]( t, pSelf, result, p );
			
			return FPS_OK;
		}catch( ... ){
			frantic::max3d::rethrow_current_exception_as_max_t();
		}
	}
	return FPS_NO_SUCH_FUNCTION;
}

namespace detail{
	template <class T>
	inline void Append( Tab<T>& tab, const T& val ){
		tab.Append( 1, const_cast<T*>(&val) );
	}

	template <class T>
	inline void AppendAndRelease( Tab<T*>& tab, std::auto_ptr<T>& p ){
		T* pVal = p.get();
		tab.Append( 1, &pVal );
		p.release();
	}

	struct make_fptimevalue : public std::unary_function<TimeValue,TimeWrapper>{
		inline TimeWrapper operator()( TimeValue t ) const {
			return TimeWrapper( t );
		}
	};

	template <class T, int Index>
	struct get_parameter : public std::unary_function< FPParams*, typename Traits< typename RemoveConstRef<T>::type >::param_type >{
		inline result_type operator()( FPParams* p ) const {
			return Traits< typename RemoveConstRef<T>::type >::get_parameter( p->params[Index] );
		}
	};

	template <class T>
	struct set_result : public std::binary_function<FPValue&,const T&,void>{
		inline void operator()( FPValue& result, const T& val ) const {
			Traits<T>::get_return_value( result, val );
		}
	};

	template <class T>
	struct set_result<const T&> : public std::binary_function<FPValue&,const T&,void>{
		inline void operator()( FPValue& result, const T& val ) const {
			Traits<T>::get_return_value( result, val );
		}
	};

	// This overload is used when returning heap allocated objects to Max. For example, functions or properties that specify TYPE_STRING_TAB are expected to
	// return a new heap allocated Tab<> with the data. In order to make sure developers don't accidentally return a pointer to an existing Tab<> I require
	// an auto_ptr< Tab<> >. Presumably the same logic is applied to any objects that return by pointer (I only tested Tab<>).
	template <class T>
	struct set_result< std::auto_ptr<T> > : public std::binary_function<FPValue&,std::auto_ptr<T>,void>{
		inline void operator()( FPValue& result, std::auto_ptr<T> val ) const {
			Traits< std::auto_ptr<T> >::get_return_value( result, val );
		}
	};

	template <class T, class R>
	struct bind_result{
		template <class FunctionType>
		inline static typename invoke_type<T>::type apply( FunctionType fn ){
			return boost::bind( set_result<R>(), _3, fn );
		}
	};

	template <class T>
	struct bind_result<T, void>{
		template <class FunctionType>
		inline static FunctionType apply( FunctionType fn ){
			return fn;
		}
	};

	template <class R, class Enable = void>
	struct set_return_impl{
		inline static void apply( FPFunctionDef* pDesc ){
			pDesc->result_type = static_cast<ParamType2>( Traits< R >::fp_return_type() );
		}
	};

	template <class EnumType>
	struct set_return_impl< EnumType, typename boost::enable_if< boost::is_enum<EnumType> >::type >{
		inline static void apply( FPFunctionDef* pDesc ){
			pDesc->result_type = TYPE_ENUM;
			pDesc->enumID = enum_id< EnumType >::get_id();
		}
	};

	template <class R>
	inline void set_return( FPFunctionDef* pDesc ){
		set_return_impl<R>::apply( pDesc );
	}

	template <class P, class Enable = void>
	struct set_parameter_impl{
		inline static void apply( FPParamDef* pDesc ){
			pDesc->type = static_cast<ParamType2>( Traits< typename RemoveConstRef<P>::type >::fp_param_type() );
		}
	};

	template <class EnumType>
	struct set_parameter_impl< EnumType, typename boost::enable_if< boost::is_enum<EnumType> >::type >{
		inline static void apply( FPParamDef* pDesc ){
			pDesc->type = TYPE_ENUM;
			pDesc->enumID = enum_id< EnumType >::get_id();
		}
	};

	template <class P>
	inline void set_parameter( FPParamDef* pDesc ){
		set_parameter_impl<P>::apply( pDesc );
	}

	template <class P, class Enable = void>
	struct set_property_impl{
		inline static void apply( FPPropDef* pDesc ){
			pDesc->prop_type = static_cast<ParamType2>( Traits<P>::fp_return_type() );
		}
	};

	template <class EnumType>
	struct set_property_impl< EnumType, typename boost::enable_if< boost::is_enum<EnumType> >::type >{
		inline static void apply( FPPropDef* pDesc ){
			pDesc->prop_type = TYPE_ENUM;
			pDesc->enumID = enum_id< EnumType >::get_id();
		}
	};

	template <class T>
	inline void set_property( FPPropDef* pDesc ){
		set_property_impl<T>::apply( pDesc );
	}

	inline std::auto_ptr< FPFunctionDef > CreateFunctionDef( FunctionID id, const MCHAR* szName, StringResID i18nDesc, int numParams ){
		std::auto_ptr< FPFunctionDef > pResult( new FPFunctionDef );

		pResult->ID = id;
		pResult->internal_name = szName;
		pResult->description = i18nDesc;
		pResult->result_type = TYPE_VOID;
		pResult->params.SetCount( numParams );
		
		for( int i = 0; i < numParams; ++i ){
			pResult->params[i] = new FPParamDef;
			pResult->params[i]->internal_name.printf( _T("Arg%d"), i+1 );
			pResult->params[i]->type = TYPE_VOID;
		}
		
		return pResult;
	}
	
	template <class MemFnPtrType>
	struct function_impl;

	template <class T, class R>
	struct function_impl< R(T::*)() >{
		inline static typename invoke_type<T>::type apply( const MCHAR* szName, R(T::*pFn)(), ::FPInterfaceDesc& desc, FunctionID fid ){
			std::auto_ptr< FPFunctionDef > pFnDesc = CreateFunctionDef( fid, szName, 0, 0 );

			set_return<R>( pFnDesc.get() );

			AppendAndRelease( desc.functions, pFnDesc );

			return bind_result<T,R>::apply( boost::bind( boost::mem_fn( pFn ), _2 ) );
		}
	};

	template <class T, class R>
	struct function_impl< R(T::*)(TimeWrapper) >{
		inline static typename invoke_type<T>::type apply( const MCHAR* szName, R(T::*pFn)(TimeWrapper), ::FPInterfaceDesc& desc, FunctionID fid ){
			std::auto_ptr< FPFunctionDef > pFnDesc = CreateFunctionDef( fid, szName, 0, 0 );

			set_return<R>( pFnDesc.get() );

			AppendAndRelease( desc.functions, pFnDesc );

			return bind_result<T,R>::apply( boost::bind( pFn, _2, boost::bind( make_fptimevalue(), _1 ) ) );
		}
	};

	template <class T, class R, class P1>
	struct function_impl< R(T::*)(P1) >{
		inline static typename invoke_type<T>::type apply( const MCHAR* szName, R(T::*pFn)(P1), ::FPInterfaceDesc& desc, FunctionID fid ){
			std::auto_ptr< FPFunctionDef > pFnDesc = CreateFunctionDef( fid, szName, 0, 1 );

			set_return<R>( pFnDesc.get() );
			set_parameter<P1>( pFnDesc->params[0] );

			AppendAndRelease( desc.functions, pFnDesc );

			return bind_result<T,R>::apply( boost::bind( pFn, _2, 
				boost::bind( get_parameter<P1,0>(), _4 )
			) );
		}
	};

	template <class T, class R, class P1>
	struct function_impl< R(T::*)(P1, TimeWrapper) >{
		inline static typename invoke_type<T>::type apply( const MCHAR* szName, R(T::*pFn)(P1, TimeWrapper), ::FPInterfaceDesc& desc, FunctionID fid ){
			std::auto_ptr< FPFunctionDef > pFnDesc = CreateFunctionDef( fid, szName, 0, 1 );

			set_return<R>( pFnDesc.get() );
			set_parameter<P1>( pFnDesc->params[0] );

			AppendAndRelease( desc.functions, pFnDesc );

			return bind_result<T,R>::apply( boost::bind( pFn, _2, 
				boost::bind( get_parameter<P1,0>(), _4 ),
				boost::bind( make_fptimevalue(), _1 )
			) );
		}
	};

	template <class T, class R, class P1, class P2>
	struct function_impl< R(T::*)(P1, P2) >{
		inline static typename invoke_type<T>::type apply( const MCHAR* szName, R(T::*pFn)(P1, P2), ::FPInterfaceDesc& desc, FunctionID fid ){
			std::auto_ptr< FPFunctionDef > pFnDesc = CreateFunctionDef( fid, szName, 0, 2 );

			set_return<R>( pFnDesc.get() );
			set_parameter<P1>( pFnDesc->params[0] );
			set_parameter<P2>( pFnDesc->params[1] );

			AppendAndRelease( desc.functions, pFnDesc );

			return bind_result<T,R>::apply( boost::bind( pFn, _2, 
				boost::bind( get_parameter<P1,0>(), _4 ),
				boost::bind( get_parameter<P2,1>(), _4 )
			) );
		}
	};

	template <class T, class R, class P1, class P2>
	struct function_impl< R(T::*)(P1, P2, TimeWrapper) >{
		inline static typename invoke_type<T>::type apply( const MCHAR* szName, R(T::*pFn)(P1, P2, TimeWrapper), ::FPInterfaceDesc& desc, FunctionID fid ){
			std::auto_ptr< FPFunctionDef > pFnDesc = CreateFunctionDef( fid, szName, 0, 2 );

			set_return<R>( pFnDesc.get() );
			set_parameter<P1>( pFnDesc->params[0] );
			set_parameter<P2>( pFnDesc->params[1] );

			AppendAndRelease( desc.functions, pFnDesc );

			return bind_result<T,R>::apply( boost::bind( pFn, _2, 
				boost::bind( get_parameter<P1,0>(), _4 ),
				boost::bind( get_parameter<P2,1>(), _4 ),
				boost::bind( make_fptimevalue(), _1 )
			) );
		}
	};

	template <class T, class R, class P1, class P2, class P3>
	struct function_impl< R(T::*)(P1, P2, P3) >{
		inline static typename invoke_type<T>::type apply( const MCHAR* szName, R(T::*pFn)(P1, P2, P3), ::FPInterfaceDesc& desc, FunctionID fid ){
			std::auto_ptr< FPFunctionDef > pFnDesc = CreateFunctionDef( fid, szName, 0, 3 );

			set_return<R>( pFnDesc.get() );
			set_parameter<P1>( pFnDesc->params[0] );
			set_parameter<P2>( pFnDesc->params[1] );
			set_parameter<P3>( pFnDesc->params[2] );

			AppendAndRelease( desc.functions, pFnDesc );

			return bind_result<T,R>::apply( boost::bind( pFn, _2, 
				boost::bind( get_parameter<P1,0>(), _4 ),
				boost::bind( get_parameter<P2,1>(), _4 ),
				boost::bind( get_parameter<P3,2>(), _4 )
			) );
		}
	};

	template <class T, class R, class P1, class P2, class P3>
	struct function_impl< R(T::*)(P1, P2, P3, TimeWrapper) >{
		inline static typename invoke_type<T>::type apply( const MCHAR* szName, R(T::*pFn)(P1, P2, P3, TimeWrapper), ::FPInterfaceDesc& desc, FunctionID fid ){
			std::auto_ptr< FPFunctionDef > pFnDesc = CreateFunctionDef( fid, szName, 0, 3 );

			set_return<R>( pFnDesc.get() );
			set_parameter<P1>( pFnDesc->params[0] );
			set_parameter<P2>( pFnDesc->params[1] );
			set_parameter<P3>( pFnDesc->params[2] );

			AppendAndRelease( desc.functions, pFnDesc );

			return bind_result<T,R>::apply( boost::bind( pFn, _2, 
				boost::bind( get_parameter<P1,0>(), _4 ),
				boost::bind( get_parameter<P2,1>(), _4 ),
				boost::bind( get_parameter<P3,2>(), _4 ),
				boost::bind( make_fptimevalue(), _1 )
			) );
		}
	};

	template <class T, class R, class P1, class P2, class P3, class P4>
	struct function_impl< R(T::*)(P1, P2, P3, P4) >{
		inline static typename invoke_type<T>::type apply( const MCHAR* szName, R(T::*pFn)(P1, P2, P3, P4), ::FPInterfaceDesc& desc, FunctionID fid ){
			std::auto_ptr< FPFunctionDef > pFnDesc = CreateFunctionDef( fid, szName, 0, 4 );

			set_return<R>( pFnDesc.get() );
			set_parameter<P1>( pFnDesc->params[0] );
			set_parameter<P2>( pFnDesc->params[1] );
			set_parameter<P3>( pFnDesc->params[2] );
			set_parameter<P4>( pFnDesc->params[3] );

			AppendAndRelease( desc.functions, pFnDesc );

			return bind_result<T,R>::apply( boost::bind( pFn, _2, 
				boost::bind( get_parameter<P1,0>(), _4 ),
				boost::bind( get_parameter<P2,1>(), _4 ),
				boost::bind( get_parameter<P3,2>(), _4 ),
				boost::bind( get_parameter<P4,3>(), _4 )
			) );
		}
	};

	template <class T, class R, class P1, class P2, class P3, class P4>
	struct function_impl< R(T::*)(P1, P2, P3, P4, TimeWrapper) >{
		inline static typename invoke_type<T>::type apply( const MCHAR* szName, R(T::*pFn)(P1, P2, P3, P4, TimeWrapper), ::FPInterfaceDesc& desc, FunctionID fid ){
			std::auto_ptr< FPFunctionDef > pFnDesc = CreateFunctionDef( fid, szName, 0, 4 );

			set_return<R>( pFnDesc.get() );
			set_parameter<P1>( pFnDesc->params[0] );
			set_parameter<P2>( pFnDesc->params[1] );
			set_parameter<P3>( pFnDesc->params[2] );
			set_parameter<P4>( pFnDesc->params[3] );

			AppendAndRelease( desc.functions, pFnDesc );

			return bind_result<T,R>::apply( boost::bind( pFn, _2, 
				boost::bind( get_parameter<P1,0>(), _4 ),
				boost::bind( get_parameter<P2,1>(), _4 ),
				boost::bind( get_parameter<P3,2>(), _4 ),
				boost::bind( get_parameter<P4,3>(), _4 ),
				boost::bind( make_fptimevalue(), _1 )
			) );
		}
	};

	template <class T, class R, class P1, class P2, class P3, class P4, class P5>
	struct function_impl< R(T::*)(P1, P2, P3, P4, P5) >{
		inline static typename invoke_type<T>::type apply( const MCHAR* szName, R(T::*pFn)(P1, P2, P3, P4, P5), ::FPInterfaceDesc& desc, FunctionID fid ){
			std::auto_ptr< FPFunctionDef > pFnDesc = CreateFunctionDef( fid, szName, 0, 5 );

			set_return<R>( pFnDesc.get() );
			set_parameter<P1>( pFnDesc->params[0] );
			set_parameter<P2>( pFnDesc->params[1] );
			set_parameter<P3>( pFnDesc->params[2] );
			set_parameter<P4>( pFnDesc->params[3] );
			set_parameter<P5>( pFnDesc->params[4] );

			AppendAndRelease( desc.functions, pFnDesc );

			return bind_result<T,R>::apply( boost::bind( pFn, _2, 
				boost::bind( get_parameter<P1,0>(), _4 ),
				boost::bind( get_parameter<P2,1>(), _4 ),
				boost::bind( get_parameter<P3,2>(), _4 ),
				boost::bind( get_parameter<P4,3>(), _4 ),
				boost::bind( get_parameter<P5,4>(), _4 )
			) );
		}
	};

	template <class T, class R, class P1, class P2, class P3, class P4, class P5>
	struct function_impl< R(T::*)(P1, P2, P3, P4, P5, TimeWrapper) >{
		inline static typename invoke_type<T>::type apply( const MCHAR* szName, R(T::*pFn)(P1, P2, P3, P4, P5, TimeWrapper), ::FPInterfaceDesc& desc, FunctionID fid ){
			std::auto_ptr< FPFunctionDef > pFnDesc = CreateFunctionDef( fid, szName, 0, 5 );

			set_return<R>( pFnDesc.get() );
			set_parameter<P1>( pFnDesc->params[0] );
			set_parameter<P2>( pFnDesc->params[1] );
			set_parameter<P3>( pFnDesc->params[2] );
			set_parameter<P4>( pFnDesc->params[3] );
			set_parameter<P5>( pFnDesc->params[4] );

			AppendAndRelease( desc.functions, pFnDesc );

			return bind_result<T,R>::apply( boost::bind( pFn, _2, 
				boost::bind( get_parameter<P1,0>(), _4 ),
				boost::bind( get_parameter<P2,1>(), _4 ),
				boost::bind( get_parameter<P3,2>(), _4 ),
				boost::bind( get_parameter<P4,3>(), _4 ),
				boost::bind( get_parameter<P5,4>(), _4 ),
				boost::bind( make_fptimevalue(), _1 )
			) );
		}
	};

	template <class T, class R, class P1, class P2, class P3, class P4, class P5, class P6>
	struct function_impl< R(T::*)(P1, P2, P3, P4, P5, P6) >{
		inline static typename invoke_type<T>::type apply( const MCHAR* szName, R(T::*pFn)(P1, P2, P3, P4, P5, P6), ::FPInterfaceDesc& desc, FunctionID fid ){
			std::auto_ptr< FPFunctionDef > pFnDesc = CreateFunctionDef( fid, szName, 0, 6 );

			set_return<R>( pFnDesc.get() );
			set_parameter<P1>( pFnDesc->params[0] );
			set_parameter<P2>( pFnDesc->params[1] );
			set_parameter<P3>( pFnDesc->params[2] );
			set_parameter<P4>( pFnDesc->params[3] );
			set_parameter<P5>( pFnDesc->params[4] );
			set_parameter<P6>( pFnDesc->params[5] );

			AppendAndRelease( desc.functions, pFnDesc );

			return bind_result<T,R>::apply( boost::bind( pFn, _2, 
				boost::bind( get_parameter<P1,0>(), _4 ),
				boost::bind( get_parameter<P2,1>(), _4 ),
				boost::bind( get_parameter<P3,2>(), _4 ),
				boost::bind( get_parameter<P4,3>(), _4 ),
				boost::bind( get_parameter<P5,4>(), _4 ),
				boost::bind( get_parameter<P6,5>(), _4 )
			) );
		}
	};

	template <class T, class R, class P1, class P2, class P3, class P4, class P5, class P6>
	struct function_impl< R(T::*)(P1, P2, P3, P4, P5, P6, TimeWrapper) >{
		inline static typename invoke_type<T>::type apply( const MCHAR* szName, R(T::*pFn)(P1, P2, P3, P4, P5, P6, TimeWrapper), ::FPInterfaceDesc& desc, FunctionID fid ){
			std::auto_ptr< FPFunctionDef > pFnDesc = CreateFunctionDef( fid, szName, 0, 6 );

			set_return<R>( pFnDesc.get() );
			set_parameter<P1>( pFnDesc->params[0] );
			set_parameter<P2>( pFnDesc->params[1] );
			set_parameter<P3>( pFnDesc->params[2] );
			set_parameter<P4>( pFnDesc->params[3] );
			set_parameter<P5>( pFnDesc->params[4] );
			set_parameter<P6>( pFnDesc->params[5] );

			AppendAndRelease( desc.functions, pFnDesc );

			return bind_result<T,R>::apply( boost::bind( pFn, _2, 
				boost::bind( get_parameter<P1,0>(), _4 ),
				boost::bind( get_parameter<P2,1>(), _4 ),
				boost::bind( get_parameter<P3,2>(), _4 ),
				boost::bind( get_parameter<P4,3>(), _4 ),
				boost::bind( get_parameter<P5,4>(), _4 ),
				boost::bind( get_parameter<P6,5>(), _4 ),
				boost::bind( make_fptimevalue(), _1 )
			) );
		}
	};
}

template <class T>
template <class MemFnPtrType>
inline FunctionDesc InterfaceDesc<T>::function( const MCHAR* szName, MemFnPtrType pFn ){
	FunctionID fid = static_cast<FunctionID>( m_dispatchFns.size() );
	
	m_dispatchFns.push_back( detail::function_impl<MemFnPtrType>::apply( szName, pFn, *this, fid ) );

	return FunctionDesc( this->GetFnDef( fid ) );
}

template <class T>
template <class R>
inline void InterfaceDesc<T>::read_write_property( const MCHAR* szName, R(T::*pGetFn)(void), void(T::*pSetFn)(R) ){
	FunctionID fid = static_cast<FunctionID>( m_dispatchFns.size() );

	std::auto_ptr< FPPropDef > pDesc( new FPPropDef );
	pDesc->getter_ID = fid;
	pDesc->setter_ID = fid+1;
	pDesc->internal_name = szName;

	detail::set_property<R>( pDesc.get() );
	detail::AppendAndRelease( this->props, pDesc );

	m_dispatchFns.push_back( boost::bind( detail::set_result<R>(), _3, boost::bind( pGetFn, _2 ) ) );
	m_dispatchFns.push_back( boost::bind( pSetFn, _2, boost::bind( detail::get_parameter<R,0>(), _4 ) ) );
}

template <class T>
template <class R>
inline void InterfaceDesc<T>::read_only_property( const MCHAR* szName, R(T::*pGetFn)(void) ){
	FunctionID fid = static_cast<FunctionID>( m_dispatchFns.size() );

	std::auto_ptr< FPPropDef > pDesc( new FPPropDef );
	pDesc->getter_ID = fid;
	pDesc->setter_ID = FP_NO_FUNCTION;
	pDesc->internal_name = szName;

	detail::set_property<R>( pDesc.get() );
	detail::AppendAndRelease( this->props, pDesc );

	m_dispatchFns.push_back( boost::bind( detail::set_result<R>(), _3, boost::bind( pGetFn, _2 ) ) );
}

template <class T>
template <class EnumType>
inline EnumDesc<EnumType> InterfaceDesc<T>::enumeration(){
	std::auto_ptr<FPEnum> pEnumDesc( new FPEnum );

	pEnumDesc->ID = enum_id< EnumType >::get_id();

	detail::Append( this->enumerations, pEnumDesc.get() );

	EnumDesc<EnumType> result( pEnumDesc.get() );

	pEnumDesc.release();

	return result;
}

} } }
