// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/safe_convert_to_type.hpp>

#include <frantic/volumetrics/field_interface.hpp>

#include <boost/shared_ptr.hpp>

#pragma warning( push, 3 )
#include <inode.h>
#include <maxtypes.h>
#pragma warning( pop )

namespace frantic {
namespace volumetrics {
class field_interface;
}
} // namespace frantic

#define EmberField_INTERFACE Interface_ID( 0x24ee1160, 0x5dd71f7e )
#define EmberPipeObject_CLASSID Class_ID( 0x69c25b86, 0x2da73ce8 )

namespace frantic {
namespace max3d {
namespace volumetrics {

/**
 * This is the preferred, cross DLL method for acquiring the field data from a Stoke field object.
 * \param pNode The node to evaluate
 * \param t The time to evaluate at
 * \param[in,out] valid This interval will be intersected with the time interval over which the resulting field is
 * unchanged.+
 */
boost::shared_ptr<frantic::volumetrics::field_interface> create_field( INode* pNode, TimeValue t, Interval& valid );

/**
 * This interface is the main way for non-Stoke objects to access field data. The preferred method of extracting a field
 * from a node is via ember::max3d::create_field(), which wraps this interface.
 */
class IEmberField : public BaseInterface {
  public:
    virtual ~IEmberField() {}

    virtual BaseInterface* GetInterface( Interface_ID id ) {
        if( id == EmberField_INTERFACE )
            return this;
        return BaseInterface::GetInterface( id );
    }

    /**
     * This is the main function for accessing the stored field. The result will be in world-space.
     * \param pNode The node associated with *this that we are evaluating. There can be many nodes associated with a
     * single field object. \param t The time to evaluate the field at. \param[in,out] valid Will be intersected with
     * the time interval over which the resulting field is unchanged.
     */
    virtual boost::shared_ptr<frantic::volumetrics::field_interface> create_field( INode* pNode, TimeValue t,
                                                                                   Interval& valid ) = 0;

    // TODO: Add an async version
    // TODO: Allow access to the potentially non-worldspace version.
};

inline IEmberField* GetEmberFieldInterface( Animatable* anim ) {
    return anim ? static_cast<IEmberField*>( anim->GetInterface( EmberField_INTERFACE ) ) : NULL;
}
inline IEmberField* GetEmberFieldInterface( INode* node, TimeValue t ) {
    return node ? GetEmberFieldInterface( node->EvalWorldState( t ).obj ) : NULL;
}

/**
 * Returns the evaluated field in worldspace. This overload allows a specific ObjectState from within the stack to be
 * specified. Usually you don't want this.
 */
inline boost::shared_ptr<frantic::volumetrics::field_interface> create_field( INode* pNode, const ObjectState& os,
                                                                              TimeValue t, Interval& valid ) {
    assert( valid.InInterval( t ) );
    assert( os.obj->FindBaseObject() == pNode->GetObjectRef()->FindBaseObject() );

    if( IEmberField* ember = GetEmberFieldInterface( os.obj ) )
        return ember->create_field( pNode, t, valid );

    if( boost::shared_ptr<Object> pPipeObj =
            frantic::max3d::safe_convert_to_type<Object>( os.obj, t, EmberPipeObject_CLASSID ) ) {
        if( IEmberField* ember = GetEmberFieldInterface( pPipeObj.get() ) )
            return ember->create_field( pNode, t, valid );
    }

    return boost::shared_ptr<frantic::volumetrics::field_interface>();
}

/**
 * Returns the evaluated field in worldspace
 */
inline boost::shared_ptr<frantic::volumetrics::field_interface> create_field( INode* pNode, TimeValue t,
                                                                              Interval& valid ) {
    if( pNode != NULL )
        return create_field( pNode, pNode->EvalWorldState( t ), t, valid );

    return boost::shared_ptr<frantic::volumetrics::field_interface>();
}

inline bool is_field( INode* pNode, TimeValue t ) {
    if( !pNode )
        return false;

    ObjectState os = pNode->EvalWorldState( t );

    return GetEmberFieldInterface( os.obj ) || ( os.obj && os.obj->CanConvertToType( EmberPipeObject_CLASSID ) );
}

} // namespace volumetrics
} // namespace max3d
} // namespace frantic
