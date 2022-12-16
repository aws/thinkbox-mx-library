// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/volumetrics/force_field_adapter.hpp>

#pragma warning( push, 3 )
#include <tbb/spin_mutex.h> //TODO: Evaluate different mutexes for optimal performance.
//#include <tbb/queuing_mutex.h>
//#include <tbb/mutex.h>
#pragma warning( pop )

#include <memory>

typedef tbb::spin_mutex mutex_type;
typedef tbb::spin_mutex::scoped_lock scoped_lock_type;

namespace frantic {
namespace max3d {
namespace volumetrics {

namespace {
ForceField* get_forcefield( INode* node ) {
    Object* obj = node ? node->GetObjectRef() : NULL;

    if( !obj )
        return NULL;

    obj = obj->FindBaseObject();
    if( !obj || obj->SuperClassID() != WSM_OBJECT_CLASS_ID )
        return NULL;

    return static_cast<WSMObject*>( obj )->GetForceField( node );
}
} // namespace

bool is_forcefield_node( INode* pNode, TimeValue /*t*/ ) { return get_forcefield( pNode ) != NULL; }

namespace {
class ForceFieldAdapter : public frantic::volumetrics::field_interface {
    ForceField* m_maxField;
    TimeValue m_time;

    static frantic::channels::channel_map s_channelMap;
    static mutex_type s_mutex; // This is required because Max ForceField objects are not required to be thread-safe.

    static const float s_toUnitsPerSecSquared;

  public:
    ForceFieldAdapter( ForceField* maxField, TimeValue t = 0 )
        : m_maxField( maxField )
        , m_time( t ) {
        if( s_channelMap.channel_count() == 0 ) {
            s_channelMap.define_channel<frantic::graphics::vector3f>( _T("Force") );
            s_channelMap.end_channel_definition();
        }
    }

    virtual ~ForceFieldAdapter() { m_maxField->DeleteThis(); }

    virtual const frantic::channels::channel_map& get_channel_map() const { return s_channelMap; }

    virtual bool evaluate_field( void* dest, const frantic::graphics::vector3f& pos ) const {
        Point3 result;

        { // Create a block to give minimal scope to the lock
            scoped_lock_type lock( s_mutex );

            // ForceField::Force() provides no thread safetly promises so we need a global lock. :(
            result = m_maxField->Force( m_time, frantic::max3d::to_max_t( pos ), Point3( 0, 0, 0 ), 0 );
        }

        *reinterpret_cast<frantic::graphics::vector3f*>( dest ) =
            s_toUnitsPerSecSquared * frantic::max3d::from_max_t( result );

        return true;
    }
};

frantic::channels::channel_map ForceFieldAdapter::s_channelMap;
mutex_type ForceFieldAdapter::s_mutex;

// This constant is used in all the ForceField samples in the MaxSDK. I have inverted the fraction, so that it undos the
// scaling.
// const float ForceFieldAdapter::s_toUnitsPerSecSquared = static_cast<float>( 1000.0 * (
// static_cast<double>(TIME_TICKSPERSEC*TIME_TICKSPERSEC) / (1200.0*1200.0) ) );

// The above number, while pulled from the Max SDK, is pretty arbitrary as far as I can tell. At least I understand what
// this number is doing (converting from units per tick^2 to units per sec^2). This also seems to give better results
// with the default values of the space warps.
const float ForceFieldAdapter::s_toUnitsPerSecSquared = static_cast<float>( TIME_TICKSPERSEC * TIME_TICKSPERSEC );
} // namespace

std::unique_ptr<frantic::volumetrics::field_interface> get_force_field_adapter( INode* pNode, TimeValue t ) {
    if( ForceField* field = get_forcefield( pNode ) )
        return std::unique_ptr<frantic::volumetrics::field_interface>( new ForceFieldAdapter( field, t ) );
    return std::unique_ptr<frantic::volumetrics::field_interface>();
}

} // namespace volumetrics
} // namespace max3d
} // namespace frantic