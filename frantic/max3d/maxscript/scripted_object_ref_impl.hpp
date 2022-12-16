// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/maxscript/scripted_object_ref.hpp>
#include <frantic/misc/string_functions.hpp>

// Stop Max SDK's base_type macro from messing with bind
#ifdef base_type
#undef base_type
#endif

#pragma warning( push, 3 )
#include <boost/bind.hpp>

#pragma warning( pop )

namespace frantic {
namespace max3d {
namespace maxscript {

/* This definition was deprecated in Max 2015 */
#if MAX_VERSION_MAJOR < 17
RefResult scripted_object_ref::ProcessRefTargMonitorMsg( Interval, RefTargetHandle rtarg, PartID& /*p*/, RefMessage msg,
                                                         bool fromTarget )
#else
RefResult scripted_object_ref::ProcessRefTargMonitorMsg( const Interval&, RefTargetHandle rtarg, PartID& /*p*/,
                                                         RefMessage msg, bool fromTarget, bool /*propagate*/,
                                                         RefTargMonitorRefMaker* /*caller*/ )
#endif
{

#if MAX_VERSION < 9000
    // This hack fixes a crash in max 8 when an OS modifier is added.
    if( rtarg != m_pWatcherTarg )
        m_pWatcher->SetRef( m_pWatcherTarg );
#endif

    if( fromTarget /*&& m_pWatcher->GetRef() == rtarg*/ && msg == REFMSG_SUBANIM_STRUCTURE_CHANGED )
        rebuild();

    return REF_SUCCEED;
}

const scripted_object_ref::param_info_t& scripted_object_ref::get_param_info( const frantic::tstring& param ) const {
    param_map::const_iterator it = m_params.find( frantic::strings::to_lower( param ) );
    if( it == m_params.end() ) {
        throw std::runtime_error(
            "scripted_object_ref::get_param_info() - Could not find parameter: \"" +
            frantic::strings::to_string( param ) +
            "\".  Check that all the correct events (on create/load/clone) are implemented in the scripted plugin." );
    }

    return it->second;
}

void scripted_object_ref::rebuild() {
    Animatable* pAnim = m_pWatcherTarg;
    if( !pAnim )
        throw std::runtime_error( "scripted_object_ref::rebuild() - not attached to a scripted object" );

    m_params.clear();
    for( int i = 0; i < pAnim->NumParamBlocks(); ++i ) {
        IParamBlock2* pblock = pAnim->GetParamBlock( i );
        if( !pblock )
            continue;

        for( int j = 0; j < pblock->NumParams(); ++j ) {
            ParamID id = pblock->IndextoID( j );
            frantic::tstring paramName = frantic::strings::to_lower( pblock->GetLocalName( id ).data() );
            m_params.insert( std::make_pair( paramName, param_info_t( pblock, id ) ) );
        }
    }

    ++m_updateCounter;
}

void scripted_object_ref::attach_to( ReferenceTarget* rtarg ) {
    if( !rtarg )
        throw std::runtime_error( "scripted_object_ref::attach_to() - Cannot attach to a NULL object" );

    m_pWatcherTarg = rtarg;
    m_pWatcher->SetRef( rtarg );
    rebuild();
}

int scripted_object_ref::ProcessEnumDependents( DependentEnumProc* /*dep*/ ) { return DEP_ENUM_CONTINUE; }

template <class T>
scripted_object_accessor<T> scripted_object_ref::get_accessor( const frantic::tstring& paramName ) const {
    return scripted_object_accessor<T>( *this, paramName );
}

scripted_object_accessor_base::scripted_object_accessor_base( const scripted_object_ref* owner,
                                                              const frantic::tstring& paramName )
    : m_owner( owner )
    , m_paramName( paramName )
    , m_pblock( NULL )
    , m_paramID( -1 )
    , m_updateCounter( (unsigned long)-1 ) {
    if( owner )
        m_updateCounter = m_owner->get_update_counter() - 1; // Subtract one to casue an update on first access
}

void scripted_object_accessor_base::validate() {
    if( !m_owner )
        throw std::runtime_error( "scripted_object_accessor_base::validate() - Accessor for \"" +
                                  frantic::strings::to_string( m_paramName ) +
                                  "\" was not linked to a scripted_object_ref" );

    if( m_updateCounter != m_owner->get_update_counter() ) {
        const scripted_object_ref::param_info_t& param = m_owner->get_param_info( m_paramName );
        m_pblock = param.pblock;
        m_paramID = param.paramID;
        m_updateCounter = m_owner->get_update_counter();
    }
}

} // namespace maxscript
} // namespace max3d
} // namespace frantic
