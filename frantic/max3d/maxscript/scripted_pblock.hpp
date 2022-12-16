// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#pragma warning( push, 3 )
#if MAX_RELEASE == 8000
#include <INodeTransformMonitor.h>
#else
#include <IRefTargMonitor.h>
#endif
#pragma warning( pop )

#include <boost/function.hpp>

namespace frantic {
namespace max3d {
namespace maxscript {

class scripted_pblock;

// WARNING THIS IS DEPRECATED

// This is the base functionality of an accessor. It is used by scripted_pblock to update
// accessors on the fly.
class scripted_pblock_accessor_base {
  private:
    scripted_pblock* m_pOwner;
    std::string m_paramName;

  protected:
    IParamBlock2* m_pblock;
    ParamID m_paramID;

  protected:
    inline scripted_pblock_accessor_base( scripted_pblock& owner, const std::string& name );
    inline virtual ~scripted_pblock_accessor_base();

  public:
    inline void invalidate() { m_pblock = NULL; }
    inline void validate();
};

// This class enables a plugin which has been extended by a scripted plugin to access the
// the parameters defined in the scripted plugins, by name.
//
// Suggested usage is for the C++ plugin to have an instance of scripted_pblock, and expose
// a function which sets the reftarget to the scripted plugin. The scripted plugin will call
// this function in the postLoad and postCreate callbacks, passing itself. scripted_pblock_accessor can
// be held as members in the C++ plugin in order to access the specific parameters.
class scripted_pblock : public IRefTargMonitor {
  private:
    struct parameter_info {
        IParamBlock2* pblock;
        ParamID paramID;

        parameter_info( IParamBlock2* block, ParamID pid )
            : pblock( block )
            , paramID( pid ) {}
        bool operator<( const parameter_info& o ) const {
            return ( pblock < o.pblock ) || ( pblock == o.pblock && paramID < o.paramID );
        }
    };

    struct callback_info {
        boost::function<void()> m_callback;
        std::string m_paramName;

        callback_info( const std::string& paramName, const boost::function<void()>& func )
            : m_paramName( paramName )
            , m_callback( func ) {}
    };

    typedef std::map<std::string, parameter_info> param_map;
    typedef std::map<parameter_info, callback_info> callback_map;

  private:
    friend class scripted_pblock_accessor_base;

    inline const parameter_info& get_parameter_info( const std::string& name );
    inline void register_accessor( scripted_pblock_accessor_base& accessor );
    inline void delete_accessor( scripted_pblock_accessor_base& accessor );

  private:
    param_map m_paramMap;
    callback_map m_callbackMap;
    RefTargMonitorRefMaker* m_pWatcher; // Cannot be a smart ptr because need to call DeleteThis

    std::vector<scripted_pblock_accessor_base*> m_accessors;

  public:
    inline scripted_pblock();
    inline virtual ~scripted_pblock();

/* This definition was deprecated in Max 2015 */
#if MAX_VERSION_MAJOR < 17
    /* From IRefTargMonitor */
    inline RefResult ProcessRefTargMonitorMsg( Interval changeInt, RefTargetHandle hTarget, PartID& partID,
                                               RefMessage message, bool fromMonitoredTarget );
#else
    inline RefResult ProcessRefTargMonitorMsg( const Interval& changeInt, RefTargetHandle hTarget, PartID& partID,
                                               RefMessage message, bool fromMonitoredTarget, bool propagate,
                                               RefTargMonitorRefMaker* caller );
#endif
    inline int ProcessEnumDependents( DependentEnumProc* dep );

    inline void rebuild();
    inline void attach_to( ReferenceTarget* pTarg );
    inline void set_parameter_callback( const std::string& paramName, const boost::function<void()>& func );
};

scripted_pblock_accessor_base::scripted_pblock_accessor_base( scripted_pblock& owner, const std::string& name )
    : m_paramName( name )
    , m_pOwner( &owner )
    , m_pblock( NULL ) {
    m_pOwner->register_accessor( *this );
}

scripted_pblock_accessor_base::~scripted_pblock_accessor_base() { m_pOwner->delete_accessor( *this ); }

void scripted_pblock_accessor_base::validate() {
    if( !m_pblock ) { // Check if we need to remap into the scripted_pblock
        const scripted_pblock::parameter_info& paramInfo = m_pOwner->get_parameter_info( m_paramName );

        m_pblock = paramInfo.pblock;
        m_paramID = paramInfo.paramID;
    }
}

// This class is the one users actually instantiate to access parameters. Be careful that the template
// parameter is correct or else the return value will be meaningless. Usage of these accessors is:
//
// scripted_pblock params;
// scripted_pblock_accessor<float> size(params, "size");	//Links to a parameter called size
//
// TimeValue t = GetCOREInterface()->GetTime();
// if( size.at_time(t) > 100.f )
//	size.at_time(t) = 100.f;

template <class T>
class scripted_pblock_accessor : public scripted_pblock_accessor_base {
  private:
    scripted_pblock_accessor( const scripted_pblock_accessor& other ); // Disable copying
    scripted_pblock_accessor& operator=( const scripted_pblock_accessor& other );

    class helper;     // Temporary return type for at_time(). Is private so users can't hold on to it.
    class tab_helper; // Temporary return type for operator[]. Is private so users can't hold on to it.

    inline T get_tab_value( TimeValue t, int index );
    inline void set_tab_value( TimeValue t, int index, const T& val );

  public:
    scripted_pblock_accessor( scripted_pblock& owner, const std::string& name )
        : scripted_pblock_accessor_base( owner, name ) {}

    inline size_t size();

    helper at_time( TimeValue t ) { return helper( *this, 0, t ); }
    const T at_time( TimeValue t ) const { return get_tab_value( t, 0 ); }

    tab_helper operator[]( int index ) { return tab_helper( *this, index ); }
    const tab_helper operator[]( int index ) const { return tab_helper( *this, index ); }
};

template <class T>
inline size_t scripted_pblock_accessor<T>::size() {
    validate();
    return static_cast<size_t>( m_pblock->Count( m_paramID ) );
}

template <class T>
void scripted_pblock_accessor<T>::set_tab_value( TimeValue t, int index, const T& val ) {
    validate();
    m_pblock->SetValue( m_paramID, t, val, index );
}

template <class T>
T scripted_pblock_accessor<T>::get_tab_value( TimeValue t, int index ) {
    T result;

    validate();
    m_pblock->GetValue( m_paramID, t, result, Interval(), index );

    return result;
}

// Temporary return type for scripted_pblock_accessor::at_time(). Is private so users can't hold on to it.
// Allows a user to assign to or use as a r-value. Does not correctly support the address-of operator.
template <class T>
class scripted_pblock_accessor<T>::helper {
  private:
    TimeValue m_time;
    int m_index;
    scripted_pblock_accessor& m_owner;

  private:
    friend class scripted_pblock_accessor;
    friend class tab_helper;
    helper( const helper& other );            // Disable copy constructor
    helper& operator=( const helper& other ); // Disable assignment
    helper( scripted_pblock_accessor& owner, TimeValue t, int index )
        : m_owner( owner )
        , m_time( t )
        , m_index( index ) {}

  public:
    const T& operator=( const T& val ) {
        m_owner.set_tab_value( m_time, m_index, val );
        return val;
    }
    operator const T() { return m_owner.get_tab_value( m_time, m_index ); }
};

// Temporary return type for scripted_pblock_accessor::operator[]. Is private so users can't hold on to it.
// Allows a user to access tab values within the scriped_pblock. Just call .at_time() on it.
template <class T>
class scripted_pblock_accessor<T>::tab_helper {
  private:
    int m_index;
    scripted_pblock_accessor& m_owner;

  private:
    friend class scripted_pblock_accessor;
    tab_helper( const tab_helper& other );
    tab_helper& operator=( const tab_helper& other );
    tab_helper( scripted_pblock_accessor& owner, int index )
        : m_owner( owner )
        , m_index( index ) {}

  public:
    helper at_time( TimeValue t ) { return helper( m_owner, t, m_index ); }
    const T at_time( TimeValue t ) const { return m_owner.get_tab_value( t, m_index ); }
};

scripted_pblock::scripted_pblock() { m_pWatcher = new RefTargMonitorRefMaker( *this ); }

scripted_pblock::~scripted_pblock() {
    if( m_pWatcher )
        m_pWatcher->MaybeAutoDelete();
}

RefResult scripted_pblock::ProcessRefTargMonitorMsg( Interval /*changeInt*/, RefTargetHandle /*hTarget*/,
                                                     PartID& /*partID*/, RefMessage message,
                                                     bool fromMonitoredTarget ) {
    // mprintf("scripted_pblock::ProcessRefTargMonitorMsg() - Got message 0x%x, partid 0x%x, hTarget 0x%p, was %s from
    // target\n", message, partID, hTarget, (fromMonitoredTarget) ? "" : "not" );

    if( !fromMonitoredTarget )
        return REF_SUCCEED;

    if( message == REFMSG_SUBANIM_STRUCTURE_CHANGED ) // Notify me if the structure of the paramblocks has changed
        rebuild();
    else if( message == REFMSG_CHANGE ) {
        // Check all the param blocks of the watched object. One of them will return with a
        // value not equal to -1, or else this was some other sort of change we can ignore.

        Animatable* pAnim = m_pWatcher->GetRef();
        for( int i = 0; i < pAnim->NumParamBlocks(); ++i ) {
            IParamBlock2* pblock = pAnim->GetParamBlock( i );
            if( !pblock )
                continue;

            ParamID pid = pblock->LastNotifyParamID();
            if( pid != -1 ) {
                // check the callback map now.
                callback_map::iterator it = m_callbackMap.find( parameter_info( pblock, pid ) );
                if( it != m_callbackMap.end() )
                    it->second.m_callback();

                break;
            }
        } // for i = 0 to pAnim->NumParamBlocks()
    }

    return REF_STOP;
}

int scripted_pblock::ProcessEnumDependents( DependentEnumProc* /*dep*/ ) { return 1; }

void scripted_pblock::attach_to( ReferenceTarget* pTarg ) {
    if( !pTarg )
        throw std::runtime_error( "scripted_pblock::set_scripted_owner() - the reference was to a NULL object" );

    m_pWatcher->SetRef( pTarg );
    rebuild();
}

void scripted_pblock::rebuild() {
    m_paramMap.clear();
    if( !m_pWatcher->GetRef() )
        throw std::runtime_error( "scripted_pblock::rebuild() - the scripted plugin reference was not set" );

    Animatable* pAnim = m_pWatcher->GetRef();
    for( int i = 0; i < pAnim->NumParamBlocks(); ++i ) {
        IParamBlock2* pblock = pAnim->GetParamBlock( i );

        for( int j = 0; j < pblock->NumParams(); ++j ) {
            ParamID pid = pblock->IndextoID( j );
            m_paramMap.insert(
                std::make_pair( std::string( pblock->GetLocalName( pid ) ), parameter_info( pblock, pid ) ) );
        }
    } // i = 0 to pAnim->NumParamBlocks()

    /*mprintf("Rebuilt the scripted param block.\n");
    for(param_map::iterator it = m_paramMap.begin(); it != m_paramMap.end(); ++it)
            mprintf("\t%s - %s,%x\n", it->first.c_str(), it->second.pblock->GetLocalName(), it->second.paramID);*/

    // Invalidate all outstanding accessors. They will remap themselves when next accessed.
    for( std::vector<scripted_pblock_accessor_base*>::iterator it = m_accessors.begin(); it != m_accessors.end(); ++it )
        ( *it )->invalidate();

    callback_map newCallbackMap;
    for( callback_map::iterator it = m_callbackMap.begin(); it != m_callbackMap.end(); ++it ) {
        // For each callback, find the parameter_info based on the rebuilt paramblocks.
        param_map::iterator paramIt = m_paramMap.find( it->second.m_paramName );
        if( paramIt != m_paramMap.end() )
            newCallbackMap.insert( std::make_pair( paramIt->second, it->second ) );
    }

    m_callbackMap.swap( newCallbackMap );
}

const scripted_pblock::parameter_info& scripted_pblock::get_parameter_info( const std::string& name ) {
    Animatable* pAnim = m_pWatcher->GetRef();
    if( !pAnim )
        throw std::runtime_error( "scripted_pblock::get_param_info() - Attempted to access param: \"" + name +
                                  "\" but the scripted plugin was not set." );

    param_map::iterator it = m_paramMap.find( name );
    if( it == m_paramMap.end() ) {
        rebuild();

        it = m_paramMap.find( name );
        if( it == m_paramMap.end() )
            throw std::runtime_error( "scripted_pblock::get_param_info() - Could not find param \"" + name + "\"" );
    }

    return it->second;
}

void scripted_pblock::set_parameter_callback( const std::string& paramName, const boost::function<void()>& func ) {
    const parameter_info& paramInfo = get_parameter_info( paramName );
    m_callbackMap.insert( std::make_pair( paramInfo, callback_info( paramName, func ) ) );
}

void scripted_pblock::register_accessor( scripted_pblock_accessor_base& accessor ) {
    m_accessors.push_back( &accessor );
}

void scripted_pblock::delete_accessor( scripted_pblock_accessor_base& accessor ) {
    std::vector<scripted_pblock_accessor_base*>::iterator it =
        std::find( m_accessors.begin(), m_accessors.end(), &accessor );
    if( it == m_accessors.end() )
        throw std::runtime_error( "scripted_pblock::delete_accessor() - Tried to delete an unregistered accessor" );

    *it = m_accessors.back();
    m_accessors.pop_back();
}

} // namespace maxscript
} // namespace max3d
} // namespace frantic
