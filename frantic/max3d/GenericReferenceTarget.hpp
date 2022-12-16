// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#pragma warning( push, 3 )
#include <iparamb2.h>
#include <ref.h>
#pragma warning( pop )

namespace frantic {
namespace max3d {

/**
 * This template class provides the necessary bootstrapping for a new object to be inserted in the 3ds Max reference
 * target heirarchy at any point. Its first template parameter defines *where* in the heirarchy to insert the new
 * object, and its second template parameter is used in the "Curiously Recursive Template Pattern" in order to implement
 * functions (like Clone) that need to know the concrete type of the object.
 *
 * This class defines a single IParamBlock2* that should be defined and created in the implementing class. All of the
 * Animatable, ReferenceMaker, and ReferenceTarget class member functions are defined assuming this single parameter
 * block.
 *
 * The implementing class is responsible for implementing any methods from classes in the heirarchy below
 * ReferenceTarget (ex. If you use GenericReferenceTarget<Object,T> you need to implement methods from BaseObject &
 * Object), as well as ReferenceMaker::NotifyRefChanged().
 *
 * @tparam BaseClass The 3ds Max class you are inheriting from. Must be ReferenceTarget, or a subclass of
 * ReferenceTarget.
 * @tparam ChildClass The concrete type you are creating by inheriting from GenericReferenceTarget, for use in the
 * Curiously Recursive Template Pattern. This is class T such that you are doing: class T : public
 * GenericReferenceTarget<SomeReferenceTargetSubclass, T>{};
 */
template <class BaseClass, class ChildClass>
class GenericReferenceTarget : public BaseClass {
  protected:
    IParamBlock2* m_pblock;

  protected:
    virtual ClassDesc2* GetClassDesc() = 0;

  public:
    GenericReferenceTarget();

    virtual ~GenericReferenceTarget();

    // From Animatable

    /**
     * @return the Class_ID as defined in GetClassDesc()->ClassID()
     */
    virtual Class_ID ClassID();

    /**
     * @param s A string that will be assigned GetClassDesc()->ClassName()
     */
#if MAX_VERSION_MAJOR < 24
    virtual void GetClassName( MSTR& s );
#else
    virtual void GetClassName( MSTR& s, bool localized );
#endif

    /**
     * @return 1 Since the single reference held is the parameter block.
     */
    virtual int NumRefs();
    virtual ReferenceTarget* GetReference( int i );
    virtual void SetReference( int i, ReferenceTarget* r );

    /**
     * @return 1 Since the single subanim is also the parameter block.
     */
    virtual int NumSubs();
    virtual Animatable* SubAnim( int i );
#if MAX_VERSION_MAJOR >= 24
    virtual TSTR SubAnimName( int i, bool localized );
#else
    virtual TSTR SubAnimName( int i );
#endif
    /**
     * @return 1 Since we hold a single parameter block
     */
    virtual int NumParamBlocks();
    virtual IParamBlock2* GetParamBlock( int i );
    virtual IParamBlock2* GetParamBlockByID( BlockID i );

    /**
     * Directly forwards to GetClassDesc()->BeginEditParams()
     */
    virtual void BeginEditParams( IObjParam* ip, ULONG flags, Animatable* prev = NULL );

    /**
     * Directly forwards to GetClassDesc()->EndEditParams()
     */
    virtual void EndEditParams( IObjParam* ip, ULONG flags, Animatable* next = NULL );

    /**
     * Calls 'operator delete' on 'this'
     */
    virtual void DeleteThis();

    // From ReferenceMaker
    //  NOTE: This is the Max 2015 signature for NotifyRefChanged. All older versions should just forward to this
    //  signature so we keep
    //        the implementations consistent.
    virtual RefResult NotifyRefChanged( const Interval& changeInt, RefTargetHandle hTarget, PartID& partID,
                                        RefMessage message, BOOL propagate ) = 0;

#if MAX_VERSION_MAJOR < 17
    virtual RefResult NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget, PartID& partID,
                                        RefMessage message );
#endif

    // From ReferenceTarget

    /**
     * Copies the held parameter block from 'from' to 'to' if both objects are valid. This is generally a deep copy of
     * the parameter block, except for any INode objects referenced (I think but you should test this if it matters to
     * you).
     */
    virtual void BaseClone( ReferenceTarget* from, ReferenceTarget* to, RemapDir& remap );

    /**
     * Creates a new ChildClass instance and uses ChildClass::BaseClone() to copy 'this' into the new object.
     */
    virtual ReferenceTarget* Clone( RemapDir& remap );
};

#pragma region Implementation

template <class BaseClass, class ChildClass>
GenericReferenceTarget<BaseClass, ChildClass>::GenericReferenceTarget() {
    m_pblock = NULL;
}

template <class BaseClass, class ChildClass>
GenericReferenceTarget<BaseClass, ChildClass>::~GenericReferenceTarget() {}

template <class BaseClass, class ChildClass>
Class_ID GenericReferenceTarget<BaseClass, ChildClass>::ClassID() {
    return GetClassDesc()->ClassID();
}

template <class BaseClass, class ChildClass>
#if MAX_VERSION_MAJOR >= 24
void GenericReferenceTarget<BaseClass, ChildClass>::GetClassName( MSTR& s, bool localized ) {
#else
void GenericReferenceTarget<BaseClass, ChildClass>::GetClassName( MSTR& s ) {
#endif
    s = GetClassDesc()->ClassName();
}

template <class BaseClass, class ChildClass>
int GenericReferenceTarget<BaseClass, ChildClass>::NumRefs() {
    return 1;
}

template <class BaseClass, class ChildClass>
ReferenceTarget* GenericReferenceTarget<BaseClass, ChildClass>::GetReference( int i ) {
    return ( i == 0 ) ? m_pblock : NULL;
}

template <class BaseClass, class ChildClass>
void GenericReferenceTarget<BaseClass, ChildClass>::SetReference( int i, ReferenceTarget* r ) {
    if( i == 0 )
        m_pblock = static_cast<IParamBlock2*>( r );
}

template <class BaseClass, class ChildClass>
int GenericReferenceTarget<BaseClass, ChildClass>::NumSubs() {
    return 1;
}

template <class BaseClass, class ChildClass>
Animatable* GenericReferenceTarget<BaseClass, ChildClass>::SubAnim( int i ) {
    return ( i == 0 ) ? m_pblock : NULL;
}

template <class BaseClass, class ChildClass>
#if MAX_VERSION_MAJOR < 24
TSTR GenericReferenceTarget<BaseClass, ChildClass>::SubAnimName( int i ) {
#else
TSTR GenericReferenceTarget<BaseClass, ChildClass>::SubAnimName( int i, bool localized ) {
#endif
    return ( i == 0 && m_pblock ) ? m_pblock->GetLocalName() : _T("");
}

template <class BaseClass, class ChildClass>
int GenericReferenceTarget<BaseClass, ChildClass>::NumParamBlocks() {
    return 1;
}

template <class BaseClass, class ChildClass>
IParamBlock2* GenericReferenceTarget<BaseClass, ChildClass>::GetParamBlock( int i ) {
    return ( i == 0 && m_pblock ) ? m_pblock : NULL;
}

template <class BaseClass, class ChildClass>
IParamBlock2* GenericReferenceTarget<BaseClass, ChildClass>::GetParamBlockByID( BlockID i ) {
    return ( m_pblock && i == m_pblock->ID() ) ? m_pblock : NULL;
}

template <class BaseClass, class ChildClass>
void GenericReferenceTarget<BaseClass, ChildClass>::BeginEditParams( IObjParam* ip, ULONG flags, Animatable* prev ) {
    GetClassDesc()->BeginEditParams( ip, this, flags, prev );
}

template <class BaseClass, class ChildClass>
void GenericReferenceTarget<BaseClass, ChildClass>::EndEditParams( IObjParam* ip, ULONG flags, Animatable* next ) {
    GetClassDesc()->EndEditParams( ip, this, flags, next );
}

template <class BaseClass, class ChildClass>
void GenericReferenceTarget<BaseClass, ChildClass>::DeleteThis() {
    delete this;
}

#if MAX_VERSION_MAJOR < 17
template <class BaseClass, class ChildClass>
RefResult GenericReferenceTarget<BaseClass, ChildClass>::NotifyRefChanged( Interval changeInt, RefTargetHandle hTarget,
                                                                           PartID& partID, RefMessage message ) {
    return this->NotifyRefChanged( changeInt, hTarget, partID, message, TRUE );
}
#endif

template <class BaseClass, class ChildClass>
void GenericReferenceTarget<BaseClass, ChildClass>::BaseClone( ReferenceTarget* from, ReferenceTarget* to,
                                                               RemapDir& remap ) {
    if( !to || !from || to == from )
        return;

    BaseClass::BaseClone( from, to, remap );

    for( int i = 0, iEnd = from->NumRefs(); i < iEnd; ++i )
        to->ReplaceReference( i, remap.CloneRef( from->GetReference( i ) ) );
}

template <class BaseClass, class ChildClass>
ReferenceTarget* GenericReferenceTarget<BaseClass, ChildClass>::Clone( RemapDir& remap ) {
    ReferenceTarget* result = (ReferenceTarget*)GetClassDesc()->Create( FALSE );

    this->BaseClone( this, result, remap );

    return result;
}

#pragma endregion

} // namespace max3d
} // namespace frantic

// Add it to the global namespace for backwards compatibility. TODO: Update GenomeProject to make this irrelevent.
using frantic::max3d::GenericReferenceTarget;
