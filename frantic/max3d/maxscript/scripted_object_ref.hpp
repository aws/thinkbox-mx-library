// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#pragma warning( push, 3 )
// Includes a little extra but ... meh works for both max 8 & 9 this way
#include <INodeTransformMonitor.h>
#pragma warning( pop )

#include <frantic/max3d/maxscript/scripted_object_accessor.hpp>

namespace frantic {
namespace max3d {
namespace maxscript {

class scripted_object_ref : public IRefTargMonitor {
  private:
    friend class scripted_object_accessor_base; // Needed so accessors can get at the param_info_t

    struct param_info_t {
        IParamBlock2* pblock;
        ParamID paramID;
        param_info_t( IParamBlock2* block, ParamID pid )
            : pblock( block )
            , paramID( pid ) {}
        bool operator<( const param_info_t& o ) const {
            return ( pblock < o.pblock ) || ( pblock == o.pblock && paramID < o.paramID );
        }
    };

    typedef std::map<frantic::tstring, param_info_t> param_map;

  private:
    ReferenceTarget* m_pWatcherTarg;
    RefTargMonitorRefMaker* m_pWatcher;
    unsigned long m_updateCounter;
    param_map m_params;

    inline void rebuild();
    inline const param_info_t& get_param_info( const frantic::tstring& param ) const;
    inline unsigned long get_update_counter() const { return m_updateCounter; }

  public:
    scripted_object_ref() { m_pWatcher = new RefTargMonitorRefMaker( *this ); }
    ~scripted_object_ref() {
        if( m_pWatcher ) {
            m_pWatcher->DeleteMe();
            m_pWatcher = NULL;
        }
    }

    template <class T>
    inline scripted_object_accessor<T> get_accessor( const frantic::tstring& paramName ) const;
    inline void attach_to( ReferenceTarget* rtarg );
    inline RefTargetHandle get_target() {
        RefTargetHandle targ = m_pWatcher->GetRef();
        if( targ ) {
            if( Object* obj = (Object*)targ->GetInterface( I_OBJECT ) ) {
                return obj->FindBaseObject();
            }
            return targ;
        }
        return NULL;
    }

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
};

} // namespace maxscript
} // namespace max3d
} // namespace frantic
