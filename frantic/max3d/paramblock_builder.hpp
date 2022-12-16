// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <iparamb2.h>

// The base_type macro is fully malicious and a real dick move on Autodesk's part. I'm replacing it with an inline
// function.
#ifdef base_type
#undef base_type
inline ParamType2 base_type( int paramType ) { return ( (ParamType2)( ( paramType ) & ~( TYPE_TAB ) ) ); }
#endif

#include <boost/aligned_storage.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/optional.hpp>
#include <boost/type_traits/alignment_of.hpp>

#if MAX_VERSION_MAJOR < 15
#define p_end end
#endif

// TODO: Remove this (or make it dependent on Hybrid builds).
#ifdef NDEBUG
#define PARAMBLOCKBUILDER_HPP_NDEBUG_REMOVED
#undef NDEBUG
#endif

#include <cassert>

namespace frantic {
namespace max3d {

namespace detail {
template <class T>
struct RemoveTab {
    typedef T type;
};

template <class T>
struct RemoveTab<T[]> {
    typedef T type;
};

template <class T, int Length>
struct RemoveTab<T[Length]> {
    typedef T type;
};
} // namespace detail

namespace parameter_units {
enum option {
    generic,   // The default option
    world,     // Reflects the choice of units in the 3ds Max configuration (ie. meters, feet, inches, etc.)
    percentage // 0-100%, converted to 0-1 in C++.
};
}

class ParamBlockBuilder {
  public:
    ParamBlockBuilder( BlockID blockID, const MCHAR szBlockName[], StringResID localBlockName, int version );
    ParamBlockBuilder& OwnerClassDesc( ClassDesc2* cd );
    ParamBlockBuilder& OwnerRefNum( int ownerRefNum );
    ParamBlockBuilder& RolloutTemplate( MapID rolloutID, int dlgTemplateID, StringResID localTitle,
                                        ParamMap2UserDlgProc* dlgCallback = NULL, int category = ROLLUP_CAT_STANDARD,
                                        int beginEditParamsFlags = 0, int addRollupPageFlags = 0 );

    // TODO: We could use template specialization to expose only the relevent options depending on the parameter type.
    template <class T>
    struct ParamBuilder {
        ParamBlockBuilder* m_owner;
        ParamID m_paramID;

        ParamBuilder& EnableAnimation( bool enabled, StringResID animTrackLocalName );
        ParamBuilder& EnableStickyDefault( bool enabled = true );
        ParamBuilder& ExtraFlags( unsigned flags );

        ParamBuilder& DefaultValue( const typename detail::RemoveTab<T>::type& value );
        ParamBuilder& ScriptDefault( const typename detail::RemoveTab<T>::type& value );
        ParamBuilder& Range( const typename detail::RemoveTab<T>::type& min,
                             const typename detail::RemoveTab<T>::type& max );
        ParamBuilder& Accessor( PBAccessor* accessor );

        ParamBuilder& Units( parameter_units::option unitType );
        ParamBuilder& AssetTypeID( MaxSDK::AssetManagement::AssetType assetTypeID );

        // Used with picker-supported parameters to filter which objects are selectable.
        ParamBuilder& PickableClassID( Class_ID classID, bool allowConvert );
        ParamBuilder& PickableSuperClassID( SClass_ID superClassID );
        ParamBuilder& Validator( PBValidator* validator );

        ParamBuilder& SpinnerUI( MapID rolloutID, EditSpinnerType spinnerType, int editControlID, int spinnerControlID,
                                 boost::optional<T> scale = boost::optional<T>() );
        ParamBuilder& EditBoxUI( MapID rolloutID, int editBoxControlID );
        ParamBuilder& CheckBoxUI( MapID rolloutID, int checkBoxControlID );
        template <int Length>
        ParamBuilder& CheckBoxUI( MapID rolloutID, int checkBoxControlID, int ( &linkedControlIDs )[Length] );
        template <int Length>
        ParamBuilder& RadioButtonsUI( MapID rolloutID, int ( &radioControlIDs )[Length] );
        template <int Length>
        ParamBuilder& RadioButtonsUI( MapID rolloutID, int ( &radioControlIDs )[Length], int ( &radioValues )[Length] );
        ParamBuilder& PickNodeButtonUI( MapID rolloutID, int controlID, StringResID localPrompt );
        template <int Length>
        ParamBuilder& ComboBoxUI( MapID rolloutID, int controlID, int ( &itemStringIDs )[Length] );

        // A list of INodes (works with INode[] parameters). Any of the buttons can be ignored by specifying 0 for the
        // control ID.
        ParamBuilder& PickNodeListBoxUI( MapID rolloutID, int listControlID, int addButtonControlID,
                                         int replaceButtonControlID, int removeButtonControlID,
                                         StringResID localPrompt );

        ParamBuilder& EnableUI( bool enabled );
        ParamBuilder& TooltipUI( StringResID localMessage );
        ParamBuilder& AdditionalRolloutUI( MapID additionalRolloutID );

        ParamBuilder& FilePickerDialogTitle( StringResID localTitle );
        ParamBuilder& FilePickerDialogDefault( const MCHAR szDefaultPath[] );
        ParamBuilder& FilePickerDialogFilter(
            const MCHAR szFileTypeFilters[] ); // Ex. "Data(*.dat)|*.dat|Excel(*.csv)|*.csv|All|*.*|"
    };

    template <class T>
    ParamBuilder<T> Parameter( ParamID paramID, const MCHAR szName[] );

    template <class T>
    __declspec( deprecated ) ParamBuilder<T> Parameter( ParamID paramID, const MCHAR szName[], StringResID localName,
                                                        bool animated = false, unsigned flags = 0 );

    const ParamBlockDesc2& GetDesc() const;

  private:
    ParamBlockDesc2&
    GetDescImpl(); // Name must be different from public version. After construction, all access should go through this.

  private:
    ParamBlockDesc2 m_desc;
};

inline ParamBlockBuilder::ParamBlockBuilder( BlockID blockID, const MCHAR szBlockName[], StringResID localBlockName,
                                             int version )
    : m_desc( blockID, const_cast<MCHAR*>( szBlockName ), localBlockName, NULL, P_VERSION, version, p_end ) {}

inline ParamBlockBuilder& ParamBlockBuilder::OwnerClassDesc( ClassDesc2* cd ) {
    m_desc.SetClassDesc( cd );
    return *this;
}

inline ParamBlockBuilder& ParamBlockBuilder::OwnerRefNum( int ownerRefNum ) {
    m_desc.flags |= P_AUTO_CONSTRUCT;
    m_desc.ref_no = ownerRefNum;
    return *this;
}

inline ParamBlockBuilder& ParamBlockBuilder::RolloutTemplate( MapID rolloutID, int dlgTemplateID,
                                                              StringResID localTitle, ParamMap2UserDlgProc* dlgCallback,
                                                              int category, int beginEditParamsFlags,
                                                              int addRollupPageFlags ) {
    m_desc.flags |= P_AUTO_UI | P_MULTIMAP | P_HASCATEGORY;

    ParamBlockDesc2::map_spec rolloutSpec;
    rolloutSpec.map_id = rolloutID;
    rolloutSpec.dlg_template = dlgTemplateID;
    rolloutSpec.title = static_cast<int>( localTitle );
    rolloutSpec.dlgProc = dlgCallback;
    rolloutSpec.category = category;
    rolloutSpec.test_flags = beginEditParamsFlags;
    rolloutSpec.rollup_flags = addRollupPageFlags;

    m_desc.map_specs.Append( 1, &rolloutSpec );
    return *this;
}

namespace detail {
template <class T>
struct ParamType;
template <>
struct ParamType<float> {
    enum { value = TYPE_FLOAT };
};
template <>
struct ParamType<int> {
    enum { value = TYPE_INT };
};
template <>
struct ParamType<Point3> {
    enum { value = TYPE_POINT3 };
};
template <>
struct ParamType<bool> {
    enum { value = TYPE_BOOL };
};
template <>
struct ParamType<const MCHAR*> {
    enum { value = TYPE_STRING };
};
template <>
struct ParamType<boost::filesystem::path> {
    enum { value = TYPE_FILENAME };
};
#if MAX_VERSION_MAJOR < 15
template <>
struct ParamType<MCHAR*> {
    enum { value = TYPE_STRING };
};
#endif
template <>
struct ParamType<Mtl*> {
    enum { value = TYPE_MTL };
};
template <>
struct ParamType<Texmap*> {
    enum { value = TYPE_TEXMAP };
};
template <>
struct ParamType<INode*> {
    enum { value = TYPE_INODE };
};
template <>
struct ParamType<ReferenceTarget*> {
    enum { value = TYPE_REFTARG };
};

template <class T>
struct AddParameter {
    inline static void apply( ParamBlockDesc2& desc, ParamID paramID, const MCHAR szName[], StringResID localName,
                              unsigned flags ) {
        desc.AddParam( paramID, szName, ParamType<T>::value, flags, localName, p_end );
    }
};

template <class T>
struct AddParameter<T[]> {
    inline static void apply( ParamBlockDesc2& desc, ParamID paramID, const MCHAR szName[], StringResID localName,
                              unsigned flags ) {
        desc.AddParam( paramID, szName, ParamType<typename detail::RemoveTab<T>::type>::value | TYPE_TAB, 0,
                       flags | P_VARIABLE_SIZE, localName, p_end );
    }
};

template <class T, int Length>
struct AddParameter<T[Length]> {
    inline static void apply( ParamBlockDesc2& desc, ParamID paramID, const MCHAR szName[], StringResID localName,
                              unsigned flags ) {
        desc.AddParam( paramID, szName, ParamType<typename detail::RemoveTab<T>::type>::value | TYPE_TAB, Length, flags,
                       localName, p_end );
    }
};
} // namespace detail

#if MAX_VERSION_MAJOR < 14
inline int base_type( int t ) { return ( (ParamType2)( ( t ) & ~( TYPE_TAB ) ) ); }
inline bool IsParamTypeAnimatable( int type ) { return animatable_type( type ); }
#endif

template <class T>
inline ParamBlockBuilder::ParamBuilder<T> ParamBlockBuilder::Parameter( ParamID paramID, const MCHAR szName[],
                                                                        StringResID localName, bool animated,
                                                                        unsigned flags ) {
    assert( !animated || IsParamTypeAnimatable( static_cast<ParamType2>(
                             detail::ParamType<typename detail::RemoveTab<T>::type>::value ) ) );

    flags &= ( ~P_ANIMATABLE );
    if( animated )
        flags |= P_ANIMATABLE;

    detail::AddParameter<T>::apply( m_desc, paramID, szName, localName, flags );

    ParamBuilder<T> result;
    result.m_owner = this;
    result.m_paramID = paramID;

    return result;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T> ParamBlockBuilder::Parameter( ParamID paramID, const MCHAR szName[] ) {
    detail::AddParameter<T>::apply( m_desc, paramID, szName, 0, 0 );

    ParamBuilder<T> result;
    result.m_owner = this;
    result.m_paramID = paramID;

    return result;
}

inline const ParamBlockDesc2& ParamBlockBuilder::GetDesc() const { return m_desc; }

inline ParamBlockDesc2& ParamBlockBuilder::GetDescImpl() { return m_desc; }

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::EnableAnimation( bool enabled, StringResID animTrackLocalName ) {
    ParamDef& pd = m_owner->GetDescImpl().GetParamDef( m_paramID );

    assert( !enabled || IsParamTypeAnimatable( pd.type ) );

    // TODO: If I can't actually change the flags here, we should use ReplaceParam to rebuild the param. Might get
    // tricky though ...
    pd.local_name = animTrackLocalName;
    pd.flags = enabled ? pd.flags | P_ANIMATABLE : pd.flags & ( ~P_ANIMATABLE );

    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>& ParamBlockBuilder::ParamBuilder<T>::EnableStickyDefault( bool enabled ) {
    ParamDef& pd = m_owner->GetDescImpl().GetParamDef( m_paramID );

    // TODO: If I can't actually change the flags here, we should use ReplaceParam to rebuild the param. Might get
    // tricky though ...
    pd.flags = enabled ? pd.flags | P_RESET_DEFAULT : pd.flags & ( ~P_RESET_DEFAULT );

    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>& ParamBlockBuilder::ParamBuilder<T>::ExtraFlags( unsigned flags ) {
    ParamDef& pd = m_owner->GetDescImpl().GetParamDef( m_paramID );

    // TODO: If I can't actually change the flags here, we should use ReplaceParam to rebuild the param. Might get
    // tricky though ...
    pd.flags = ( flags & ( ~P_ANIMATABLE ) ) | ( pd.flags & P_ANIMATABLE );

    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::DefaultValue( const typename detail::RemoveTab<T>::type& value ) {
    m_owner->GetDescImpl().ParamOption( m_paramID, p_default, value );
    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::ScriptDefault( const typename detail::RemoveTab<T>::type& value ) {
    m_owner->GetDescImpl().ParamOption( m_paramID, p_ms_default, value );
    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::Range( const typename detail::RemoveTab<T>::type& min,
                                           const typename detail::RemoveTab<T>::type& max ) {
    m_owner->GetDescImpl().ParamOption( m_paramID, p_range, min, max );
    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>& ParamBlockBuilder::ParamBuilder<T>::Accessor( PBAccessor* accessor ) {
    m_owner->GetDescImpl().ParamOption( m_paramID, p_accessor, accessor );
    return *this;
}

namespace detail {
template <class T>
struct set_units_impl;

template <>
struct set_units_impl<float> {
    inline static void apply( ParamBlockDesc2& pbDesc, ParamID paramID, parameter_units::option unitType ) {
        switch( unitType ) {
        case parameter_units::world:
            pbDesc.ParamOption( paramID, p_dim, stdWorldDim, p_end );
            break;
        case parameter_units::percentage:
            pbDesc.ParamOption( paramID, p_dim, stdPercentDim, p_end );
            break;
        default:
            pbDesc.ParamOption( paramID, p_dim, defaultDim, p_end );
            break;
        }
    }
};
} // namespace detail

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::Units( parameter_units::option unitType ) {
    detail::set_units_impl<T>::apply( m_owner->GetDescImpl(), m_paramID, unitType );
    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>& ParamBlockBuilder::ParamBuilder<T>::PickableClassID( Class_ID classID,
                                                                                                bool allowConvert ) {
    m_owner->GetDescImpl().ParamOption( m_paramID, p_classID, classID );
    m_owner->GetDescImpl().flags = allowConvert ? ( m_owner->GetDescImpl().flags | P_CAN_CONVERT )
                                                : ( m_owner->GetDescImpl().flags & ( ~P_CAN_CONVERT ) );
    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::PickableSuperClassID( SClass_ID superClassID ) {
    m_owner->GetDescImpl().ParamOption( m_paramID, p_sclassID, superClassID );
    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::AssetTypeID( MaxSDK::AssetManagement::AssetType assetTypeID ) {
    m_owner->GetDescImpl().ParamOption( m_paramID, p_assetTypeID, assetTypeID );
    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>& ParamBlockBuilder::ParamBuilder<T>::Validator( PBValidator* validator ) {
    m_owner->GetDescImpl().ParamOption( m_paramID, p_validator, validator );
    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::SpinnerUI( MapID rolloutID, EditSpinnerType spinnerType, int editControlID,
                                               int spinnerControlID, boost::optional<T> scale ) {
    assert( m_owner->GetDescImpl().GetParamDef( m_paramID ).type == TYPE_INT ||
            m_owner->GetDescImpl().GetParamDef( m_paramID ).type == TYPE_FLOAT );

    if( scale ) {
        m_owner->GetDescImpl().ParamOption( m_paramID, p_ui, rolloutID, TYPE_SPINNER, spinnerType, editControlID,
                                            spinnerControlID, scale.get() );
    } else {
        m_owner->GetDescImpl().ParamOption( m_paramID, p_ui, rolloutID, TYPE_SPINNER, spinnerType, editControlID,
                                            spinnerControlID, SPIN_AUTOSCALE );
    }
    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>& ParamBlockBuilder::ParamBuilder<T>::EditBoxUI( MapID rolloutID,
                                                                                          int editBoxControlID ) {
    assert( m_owner->GetDescImpl().GetParamDef( m_paramID ).type == TYPE_STRING ||
            m_owner->GetDescImpl().GetParamDef( m_paramID ).type == TYPE_FILENAME );

    m_owner->GetDescImpl().ParamOption( m_paramID, p_ui, rolloutID, TYPE_EDITBOX, editBoxControlID );
    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>& ParamBlockBuilder::ParamBuilder<T>::CheckBoxUI( MapID rolloutID,
                                                                                           int checkBoxControlID ) {
    assert( m_owner->GetDescImpl().GetParamDef( m_paramID ).type == TYPE_BOOL );

    m_owner->GetDescImpl().ParamOption( m_paramID, p_ui, rolloutID, TYPE_SINGLECHEKBOX, checkBoxControlID );
    return *this;
}

template <class T>
template <int Length>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::CheckBoxUI( MapID rolloutID, int checkBoxControlID,
                                                int ( &linkedControlIDs )[Length] ) {
    assert( m_owner->GetDescImpl().GetParamDef( m_paramID ).type == TYPE_BOOL );

    Tab<MapID> enableControls;
    enableControls.SetCount( Length );

    std::copy( linkedControlIDs, linkedControlIDs + Length, enableControls.Addr( 0 ) );

    m_owner->GetDescImpl().ParamOption( m_paramID, p_ui, rolloutID, TYPE_SINGLECHEKBOX, checkBoxControlID );
    m_owner->GetDescImpl().ParamOptionEnableCtrls( m_paramID, enableControls );
    return *this;
}

template <class T>
template <int Length>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::RadioButtonsUI( MapID rolloutID, int ( &radioControlIDs )[Length] ) {
    m_owner->GetDescImpl().ParamOption( m_paramID, p_ui, rolloutID, TYPE_RADIO, 0 );

    ParamDef& def = m_owner->GetDescImpl().GetParamDef( m_paramID );

    assert( def.type == TYPE_INT || def.type == TYPE_RADIOBTN_INDEX );

    MaxHeapOperators::operator delete( def.ctrl_IDs );

    def.ctrl_IDs = static_cast<int*>( MaxHeapOperators::operator new( sizeof( int[Length] ) ) );
    def.ctrl_count = Length;

    memcpy( def.ctrl_IDs, radioControlIDs, sizeof( int[Length] ) );

    return *this;
}

template <class T>
template <int Length>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::RadioButtonsUI( MapID rolloutID, int ( &radioControlIDs )[Length],
                                                    int ( &radioValues )[Length] ) {
    m_owner->GetDescImpl().ParamOption( m_paramID, p_ui, rolloutID, TYPE_RADIO, 0 );

    ParamDef& def = m_owner->GetDescImpl().GetParamDef( m_paramID );

    assert( def.type == TYPE_INT || def.type == TYPE_RADIOBTN_INDEX );

    MaxHeapOperators::operator delete( def.ctrl_IDs );
    MaxHeapOperators::operator delete( def.val_bits );

    def.ctrl_IDs = static_cast<int*>( MaxHeapOperators::operator new( sizeof( int[Length] ) ) );
    def.val_bits = static_cast<int*>( MaxHeapOperators::operator new( sizeof( int[Length] ) ) );
    def.ctrl_count = Length;

    memcpy( def.ctrl_IDs, radioControlIDs, sizeof( int[Length] ) );
    memcpy( def.val_bits, radioValues, sizeof( int[Length] ) );

    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::PickNodeButtonUI( MapID rolloutID, int controlID, StringResID localPrompt ) {
    assert( m_owner->GetDescImpl().GetParamDef( m_paramID ).type == TYPE_INODE );

    m_owner->GetDescImpl().ParamOption( m_paramID, p_ui, rolloutID, TYPE_PICKNODEBUTTON, controlID );
    m_owner->GetDescImpl().ParamOption( m_paramID, p_prompt, localPrompt );
    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::PickNodeListBoxUI( MapID rolloutID, int listControlID, int addButtonControlID,
                                                       int replaceButtonControlID, int removeButtonControlID,
                                                       StringResID localPrompt ) {
    assert( m_owner->GetDescImpl().GetParamDef( m_paramID ).type == TYPE_INODE_TAB );

    m_owner->GetDescImpl().ParamOption( m_paramID, p_ui, rolloutId, TYPE_NODELISTBOX, listControlID, addButtonControlID,
                                        replaceButtonControlID, removeButtonControlID );
    m_owner->GetDescImpl().ParamOption( m_paramID, p_prompt, localPrompt );
    return *this;
}

template <class T>
template <int Length>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::ComboBoxUI( MapID rolloutID, int controlID, int ( &itemStringIDs )[Length] ) {
    Tab<int> stringIDs;
    stringIDs.Append( Length, itemStringIDs );

    m_owner->GetDescImpl().ParamOption( m_paramID, p_ui, rolloutID, TYPE_INT_COMBOBOX, controlID );
    m_owner->GetDescImpl().ParamOptionContentValues( m_paramID, stringIDs );

    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>& ParamBlockBuilder::ParamBuilder<T>::EnableUI( bool enabled ) {
    m_owner->GetDescImpl().ParamOption( m_paramID, p_enabled, enabled ? TRUE : FALSE );
    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>& ParamBlockBuilder::ParamBuilder<T>::TooltipUI( StringResID localMessage ) {
    m_owner->GetDescImpl().ParamOption( m_paramID, p_tooltip, localMessage );
    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::AdditionalRolloutUI( MapID additionalRolloutID ) {
    m_owner->GetDescImpl().ParamOption( m_paramID, p_uix, additionalRolloutID );
    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::FilePickerDialogTitle( StringResID localTitle ) {
    m_owner->GetDescImpl().ParamOption( m_paramID, p_caption, localTitle );
    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::FilePickerDialogDefault( const MCHAR szDefaultPath[] ) {
    m_owner->GetDescImpl().ParamOption( m_paramID, p_init_file, szDefaultPath );
    return *this;
}

template <class T>
inline ParamBlockBuilder::ParamBuilder<T>&
ParamBlockBuilder::ParamBuilder<T>::FilePickerDialogFilter( const MCHAR szFileTypeFilters[] ) {
    m_owner->GetDescImpl().ParamOption( m_paramID, p_file_types, szFileTypeFilters );
    return *this;
}

} // namespace max3d
} // namespace frantic

#ifdef PARAMBLOCKBUILDER_HPP_NDEBUG_REMOVED
#undef PARAMBLOCKBUILDER_HPP_NDEBUG_REMOVED
#define NDEBUG
// Re-include <cassert> so we reset the behavior of assert for other files included after this.
#include <cassert>
#endif
