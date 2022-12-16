// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/strings/tstring.hpp>

#include <ParticleFlow/PFClassIDs.h>
#include <plugapi.h>

namespace frantic {
namespace max3d {
namespace geopipe {

#pragma warning( push, 3 )

inline std::string RendTypeToString( RendType r ) {
    switch( r ) {
    case RENDTYPE_NORMAL:
        return "RENDTYPE_NORMAL";
    case RENDTYPE_REGION:
        return "RENDTYPE_REGION";
    case RENDTYPE_BLOWUP:
        return "RENDTYPE_BLOWUP";
    case RENDTYPE_SELECT:
        return "RENDTYPE_SELECT";
    case RENDTYPE_REGIONCROP:
        return "RENDTYPE_REGIONCROP";
    case RENDTYPE_REGION_SEL:
        return "RENDTYPE_REGION_SEL";
    case RENDTYPE_CROP_SEL:
        return "RENDTYPE_CROP_SEL";
    case RENDTYPE_BAKE_SEL:
        return "RENDTYPE_BAKE_SEL";
    case RENDTYPE_BAKE_ALL:
        return "RENDTYPE_BAKE_ALL";
#if MAX_RELEASE >= 8000 // RENDTYPE_BAKE_SEL_CROP is not in 3ds max 6 or earlier
    case RENDTYPE_BAKE_SEL_CROP:
        return "RENDTYPE_BAKE_SEL_CROP";
#endif
    default:
        return "RENDTYPE_UNKNOWN";
    }
}

inline frantic::tstring ParamTypeToString( ParamType2 paramType ) {
    if( ( paramType & TYPE_BY_REF ) != 0 )
        return ParamTypeToString( ParamType2( paramType & ~TYPE_BY_REF ) ) + _T("_BR");
    if( ( paramType & TYPE_BY_VAL ) != 0 )
        return ParamTypeToString( ParamType2( paramType & ~TYPE_BY_VAL ) ) + _T("_BV");
    if( ( paramType & TYPE_BY_PTR ) != 0 )
        return ParamTypeToString( ParamType2( paramType & ~TYPE_BY_PTR ) ) + _T("_BP");
    if( ( paramType & TYPE_TAB ) != 0 )
        return ParamTypeToString( ParamType2( paramType & ~TYPE_TAB ) ) + _T("_TAB");

    switch( paramType ) {
    case TYPE_FLOAT:
        return _T("TYPE_FLOAT");
    case TYPE_INT:
        return _T("TYPE_INT");
    case TYPE_RGBA:
        return _T("TYPE_RGBA");
    case TYPE_POINT3:
        return _T("TYPE_POINT3");
    case TYPE_BOOL:
        return _T("TYPE_BOOL");

    case TYPE_ANGLE:
        return _T("TYPE_ANGLE");
    case TYPE_PCNT_FRAC:
        return _T("TYPE_PCNT_FRAC");
    case TYPE_WORLD:
        return _T("TYPE_WORLD");
    case TYPE_STRING:
        return _T("TYPE_STRING");
    case TYPE_FILENAME:
        return _T("TYPE_FILENAME");
    case TYPE_HSV:
        return _T("TYPE_HSV");
    case TYPE_COLOR_CHANNEL:
        return _T("TYPE_COLOR_CHANNEL");
    case TYPE_TIMEVALUE:
        return _T("TYPE_TIMEVALUE");
    case TYPE_RADIOBTN_INDEX:
        return _T("TYPE_RADIOBTN_INDEX");
    case TYPE_MTL:
        return _T("TYPE_MTL");
    case TYPE_TEXMAP:
        return _T("TYPE_TEXMAP");
    case TYPE_BITMAP:
        return _T("TYPE_BITMAP");
    case TYPE_INODE:
        return _T("TYPE_INODE");
    case TYPE_REFTARG:
        return _T("TYPE_REFTARG");
    case TYPE_INDEX:
        return _T("TYPE_INDEX");
    case TYPE_MATRIX3:
        return _T("TYPE_MATRIX3");
    case TYPE_PBLOCK2:
        return _T("TYPE_PBLOCK2");
    case TYPE_POINT4:
        return _T("TYPE_POINT4");
    case TYPE_FRGBA:
        return _T("TYPE_FRGBA");
    case TYPE_ENUM:
        return _T("TYPE_ENUM");
    case TYPE_VOID:
        return _T("TYPE_VOID");
    case TYPE_INTERVAL:
        return _T("TYPE_INTERVAL");
    case TYPE_ANGAXIS:
        return _T("TYPE_ANGAXIS");
    case TYPE_QUAT:
        return _T("TYPE_QUAT");
    case TYPE_RAY:
        return _T("TYPE_RAY");
    case TYPE_POINT2:
        return _T("TYPE_POINT2");
    case TYPE_BITARRAY:
        return _T("TYPE_BITARRAY");
    case TYPE_CLASS:
        return _T("TYPE_CLASS");
    case TYPE_MESH:
        return _T("TYPE_MESH");
    case TYPE_OBJECT:
        return _T("TYPE_OBJECT");
    case TYPE_CONTROL:
        return _T("TYPE_CONTROL");
    case TYPE_POINT:
        return _T("TYPE_POINT");
    case TYPE_TSTR:
        return _T("TYPE_TSTR");
    case TYPE_IOBJECT:
        return _T("TYPE_IOBJECT");
    case TYPE_INTERFACE:
        return _T("TYPE_INTERFACE");
    case TYPE_HWND:
        return _T("TYPE_HWND");
    case TYPE_NAME:
        return _T("TYPE_NAME");
    case TYPE_COLOR:
        return _T("TYPE_COLOR");
    case TYPE_FPVALUE:
        return _T("TYPE_FPVALUE");
    case TYPE_VALUE:
        return _T("TYPE_VALUE");
    case TYPE_DWORD:
        return _T("TYPE_DWORD");
    case TYPE_bool:
        return _T("TYPE_bool");

    case TYPE_KEYARG_MARKER:
        return _T("TYPE_KEYARG_MARKER");
    case TYPE_MSFLOAT:
        return _T("TYPE_MSFLOAT");
    case TYPE_UNSPECIFIED:
        return _T("TYPE_UNSPECIFIED");

    default:
        return _T("ParamType2(") + boost::lexical_cast<frantic::tstring>( paramType ) + _T(")");
    }
}

inline std::string SuperClassIDToString( SClass_ID sclassID ) {
    switch( sclassID ) {
    // Internal super-class-ids
    case GEN_MODAPP_CLASS_ID:
        return "GEN_MODAPP_CLASS_ID";
    case MODAPP_CLASS_ID:
        return "MODAPP_CLASS_ID";
    case OBREF_MODAPP_CLASS_ID:
        return "OBREF_MODAPP_CLASS_ID";
    case BASENODE_CLASS_ID:
        return "BASENODE_CLASS_ID";
    case GEN_DERIVOB_CLASS_ID:
        return "GEN_DERIVOB_CLASS_ID";
    case DERIVOB_CLASS_ID:
        return "DERIVOB_CLASS_ID";
    case WSM_DERIVOB_CLASS_ID:
        return "WSM_DERIVOB_CLASS_ID";
    case PARAMETER_BLOCK_CLASS_ID:
        return "PARAMETER_BLOCK_CLASS_ID";
    case PARAMETER_BLOCK2_CLASS_ID:
        return "PARAMETER_BLOCK2_CLASS_ID";
    case EASE_LIST_CLASS_ID:
        return "EASE_LIST_CLASS_ID";
    case AXIS_DISPLAY_CLASS_ID:
        return "AXIS_DISPLAY_CLASS_ID";
    case MULT_LIST_CLASS_ID:
        return "MULT_LIST_CLASS_ID";
    case NOTETRACK_CLASS_ID:
        return "NOTETRACK_CLASS_ID";
    case TREE_VIEW_CLASS_ID:
        return "TREE_VIEW_CLASS_ID";
    case SCENE_CLASS_ID:
        return "SCENE_CLASS_ID";
    case THE_GRIDREF_CLASS_ID:
        return "THE_GRIDREF_CLASS_ID";
    case VIEWREF_CLASS_ID:
        return "VIEWREF_CLASS_ID";
    case BITMAPDAD_CLASS_ID:
        return "BITMAPDAD_CLASS_ID";
#if MAX_VERSION_MAJOR < 24
    case PARTICLE_SYS_CLASS_ID:
        return "PARTICLE_SYS_CLASS_ID";
#endif
    case AGGMAN_CLASS_ID:
        return "AGGMAN_CLASS_ID";
    case MAXSCRIPT_WRAPPER_CLASS_ID:
        return "MAXSCRIPT_WRAPPER_CLASS_ID";

    // Pseudo-super-class-ids
    case DEFORM_OBJ_CLASS_ID:
        return "DEFORM_OBJ_CLASS_ID";
    case MAPPABLE_OBJ_CLASS_ID:
        return "MAPPABLE_OBJ_CLASS_ID";
    case GENERIC_SHAPE_CLASS_ID:
        return "GENERIC_SHAPE_CLASS_ID";

    // Pluggable super-class-ids
    case GEOMOBJECT_CLASS_ID:
        return "GEOMOBJECT_CLASS_ID";
    case CAMERA_CLASS_ID:
        return "CAMERA_CLASS_ID";
    case LIGHT_CLASS_ID:
        return "LIGHT_CLASS_ID";
    case SHAPE_CLASS_ID:
        return "SHAPE_CLASS_ID";
    case HELPER_CLASS_ID:
        return "HELPER_CLASS_ID";
    case SYSTEM_CLASS_ID:
        return "SYSTEM_CLASS_ID";
    case REF_MAKER_CLASS_ID:
        return "REF_MAKER_CLASS_ID";
    case REF_TARGET_CLASS_ID:
        return "REF_TARGET_CLASS_ID";
    case OSM_CLASS_ID:
        return "OSM_CLASS_ID";
    case WSM_CLASS_ID:
        return "WSM_CLASS_ID";
    case WSM_OBJECT_CLASS_ID:
        return "WSM_OBJECT_CLASS_ID";
    case SCENE_IMPORT_CLASS_ID:
        return "SCENE_IMPORT_CLASS_ID";
    case SCENE_EXPORT_CLASS_ID:
        return "SCENE_EXPORT_CLASS_ID";
    case BMM_STORAGE_CLASS_ID:
        return "BMM_STORAGE_CLASS_ID";
    case BMM_FILTER_CLASS_ID:
        return "BMM_FILTER_CLASS_ID";
    case BMM_IO_CLASS_ID:
        return "BMM_IO_CLASS_ID";
    case BMM_DITHER_CLASS_ID:
        return "BMM_DITHER_CLASS_ID";
    case BMM_COLORCUT_CLASS_ID:
        return "BMM_COLORCUT_CLASS_ID";
    case USERDATATYPE_CLASS_ID:
        return "USERDATATYPE_CLASS_ID";
    case MATERIAL_CLASS_ID:
        return "MATERIAL_CLASS_ID";
    case TEXMAP_CLASS_ID:
        return "TEXMAP_CLASS_ID";
    case UVGEN_CLASS_ID:
        return "UVGEN_CLASS_ID";
    case XYZGEN_CLASS_ID:
        return "XYZGEN_CLASS_ID";
    case TEXOUTPUT_CLASS_ID:
        return "TEXOUTPUT_CLASS_ID";
    case SOUNDOBJ_CLASS_ID:
        return "SOUNDOBJ_CLASS_ID";
    case FLT_CLASS_ID:
        return "FLT_CLASS_ID";
    case RENDERER_CLASS_ID:
        return "RENDERER_CLASS_ID";
    case BEZFONT_LOADER_CLASS_ID:
        return "BEZFONT_LOADER_CLASS_ID";
    case ATMOSPHERIC_CLASS_ID:
        return "ATMOSPHERIC_CLASS_ID";
    case UTILITY_CLASS_ID:
        return "UTILITY_CLASS_ID";
    case TRACKVIEW_UTILITY_CLASS_ID:
        return "TRACKVIEW_UTILITY_CLASS_ID";
#if MAX_VERSION_MAJOR < 15
    case FRONTEND_CONTROL_CLASS_ID:
        return "FRONTEND_CONTROL_CLASS_ID";
#endif
    case MOT_CAP_DEV_CLASS_ID:
        return "MOT_CAP_DEV_CLASS_ID";
    case MOT_CAP_DEVBINDING_CLASS_ID:
        return "MOT_CAP_DEVBINDING_CLASS_ID";
    case OSNAP_CLASS_ID:
        return "OSNAP_CLASS_ID";
    case TEXMAP_CONTAINER_CLASS_ID:
        return "TEXMAP_CONTAINER_CLASS_ID";
    case RENDER_EFFECT_CLASS_ID:
        return "RENDER_EFFECT_CLASS_ID";
    case FILTER_KERNEL_CLASS_ID:
        return "FILTER_KERNEL_CLASS_ID";
    case SHADER_CLASS_ID:
        return "SHADER_CLASS_ID";
    case COLPICK_CLASS_ID:
        return "COLPICK_CLASS_ID";
    case SHADOW_TYPE_CLASS_ID:
        return "SHADOW_TYPE_CLASS_ID";
    case GUP_CLASS_ID:
        return "GUP_CLASS_ID";
    case LAYER_CLASS_ID:
        return "LAYER_CLASS_ID";
    case SCHEMATICVIEW_UTILITY_CLASS_ID:
        return "SCHEMATICVIEW_UTILITY_CLASS_ID";
    case SAMPLER_CLASS_ID:
        return "SAMPLER_CLASS_ID";
#if MAX_VERSION_MAJOR < 14
    case ASSOC_CLASS_ID:
        return "ASSOC_CLASS_ID";
    case GLOBAL_ASSOC_CLASS_ID:
        return "GLOBAL_ASSOC_CLASS_ID";
#endif
    case IK_SOLVER_CLASS_ID:
        return "IK_SOLVER_CLASS_ID";
    case RENDER_ELEMENT_CLASS_ID:
        return "RENDER_ELEMENT_CLASS_ID";
    case BAKE_ELEMENT_CLASS_ID:
        return "BAKE_ELEMENT_CLASS_ID";
    case CUST_ATTRIB_CLASS_ID:
        return "CUST_ATTRIB_CLASS_ID";
    case RADIOSITY_CLASS_ID:
        return "RADIOSITY_CLASS_ID";
    case TONE_OPERATOR_CLASS_ID:
        return "TONE_OPERATOR_CLASS_ID";
    case MPASS_CAM_EFFECT_CLASS_ID:
        return "MPASS_CAM_EFFECT_CLASS_ID";
    case MR_SHADER_CLASS_ID_DEFUNCT:
        return "MR_SHADER_CLASS_ID_DEFUNCT";

    // control super-class-ids
    case CTRL_SHORT_CLASS_ID:
        return "CTRL_SHORT_CLASS_ID";
    case CTRL_INTEGER_CLASS_ID:
        return "CTRL_INTEGER_CLASS_ID";
    case CTRL_FLOAT_CLASS_ID:
        return "CTRL_FLOAT_CLASS_ID";
    case CTRL_POINT2_CLASS_ID:
        return "CTRL_POINT2_CLASS_ID";
    case CTRL_POINT3_CLASS_ID:
        return "CTRL_POINT3_CLASS_ID";
#if MAX_VERSION_MAJOR < 19
    case CTRL_POS_CLASS_ID:
        return "CTRL_POS_CLASS_ID";
    case CTRL_QUAT_CLASS_ID:
        return "CTRL_QUAT_CLASS_ID";
#endif
    case CTRL_MATRIX3_CLASS_ID:
        return "CTRL_MATRIX3_CLASS_ID";
    case CTRL_COLOR_CLASS_ID:
        return "CTRL_COLOR_CLASS_ID";
    case CTRL_COLOR24_CLASS_ID:
        return "CTRL_COLOR24_CLASS_ID";
    case CTRL_POSITION_CLASS_ID:
        return "CTRL_POSITION_CLASS_ID";
    case CTRL_ROTATION_CLASS_ID:
        return "CTRL_ROTATION_CLASS_ID";
    case CTRL_SCALE_CLASS_ID:
        return "CTRL_SCALE_CLASS_ID";
    case CTRL_MORPH_CLASS_ID:
        return "CTRL_MORPH_CLASS_ID";
    case CTRL_USERTYPE_CLASS_ID:
        return "CTRL_USERTYPE_CLASS_ID";
#if MAX_VERSION_MAJOR < 25
    case CTRL_MASTERPOINT_CLASS_ID:
        return "CTRL_MASTERPOINT_CLASS_ID";
    case MASTERBLOCK_SUPER_CLASS_ID:
        return "MASTERBLOCK_SUPER_CLASS_ID";
#endif
    case CTRL_POINT4_CLASS_ID:
        return "CTRL_POINT4_CLASS_ID";
    case CTRL_FRGBA_CLASS_ID:
        return "CTRL_FRGBA_CLASS_ID";

    case PATH_CONTROL_CLASS_ID:
        return "PATH_CONTROL_CLASS_ID";
    case EULER_CONTROL_CLASS_ID:
        return "EULER_CONTROL_CLASS_ID";
    case EXPR_POS_CONTROL_CLASS_ID:
        return "EXPR_POS_CONTROL_CLASS_ID";
    case EXPR_P3_CONTROL_CLASS_ID:
        return "EXPR_P3_CONTROL_CLASS_ID";
    case EXPR_FLOAT_CONTROL_CLASS_ID:
        return "EXPR_FLOAT_CONTROL_CLASS_ID";
    case EXPR_SCALE_CONTROL_CLASS_ID:
        return "EXPR_SCALE_CONTROL_CLASS_ID";
    case EXPR_ROT_CONTROL_CLASS_ID:
        return "EXPR_ROT_CONTROL_CLASS_ID";
    case LOCAL_EULER_CONTROL_CLASS_ID:
        return "LOCAL_EULER_CONTROL_CLASS_ID";
    case POSITION_CONSTRAINT_CLASS_ID:
        return "POSITION_CONSTRAINT_CLASS_ID";
    case ORIENTATION_CONSTRAINT_CLASS_ID:
        return "ORIENTATION_CONSTRAINT_CLASS_ID";
    case LOOKAT_CONSTRAINT_CLASS_ID:
        return "LOOKAT_CONSTRAINT_CLASS_ID";
    case ADDITIVE_EULER_CONTROL_CLASS_ID:
        return "ADDITIVE_EULER_CONTROL_CLASS_ID";

    case FLOATNOISE_CONTROL_CLASS_ID:
        return "FLOATNOISE_CONTROL_CLASS_ID";
    case POSITIONNOISE_CONTROL_CLASS_ID:
        return "POSITIONNOISE_CONTROL_CLASS_ID";
    case POINT3NOISE_CONTROL_CLASS_ID:
        return "POINT3NOISE_CONTROL_CLASS_ID";
    case ROTATIONNOISE_CONTROL_CLASS_ID:
        return "ROTATIONNOISE_CONTROL_CLASS_ID";
    case SCALENOISE_CONTROL_CLASS_ID:
        return "SCALENOISE_CONTROL_CLASS_ID";

    case DUMMYCHANNEL_CLASS_ID:
        return "DUMMYCHANNEL_CLASS_ID";

    default:
        return "SuperClassID(" + boost::lexical_cast<std::string>( sclassID ) + ")";
    }
}

inline std::string ClassIDToSimpleString( Class_ID classID ) {
    // Note, the cast results in negative numbers which match the classid format returned to maxscript
    return boost::lexical_cast<std::string>( static_cast<long>( classID.PartA() ) ) + "," +
           boost::lexical_cast<std::string>( static_cast<long>( classID.PartB() ) );
}

inline std::string ClassIDToString( Class_ID classID ) {
    if( classID == BOOLCNTRL_CLASS_ID )
        return "BOOLCNTRL_CLASS_ID";
    if( classID == SURF_CONTROL_CLASSID )
        return "SURF_CONTROL_CLASSID";
    if( classID == LINKCTRL_CLASSID )
        return "LINKCTRL_CLASSID";

    if( classID == Class_ID( TRIOBJ_CLASS_ID, 0 ) )
        return "TRIOBJ_CLASS_ID";
    if( classID == Class_ID( EDITTRIOBJ_CLASS_ID, 0 ) )
        return "EDITTRIOBJ_CLASS_ID";
    if( classID == Class_ID( POLYOBJ_CLASS_ID, 0 ) )
        return "POLYOBJ_CLASS_ID";
    if( classID == Class_ID( PATCHOBJ_CLASS_ID, 0 ) )
        return "PATCHOBJ_CLASS_ID";
    if( classID == Class_ID( NURBSOBJ_CLASS_ID, 0 ) )
        return "NURBSOBJ_CLASS_ID";

    if( classID == EPOLYOBJ_CLASS_ID )
        return "EPOLYOBJ_CLASS_ID";

    if( classID == Class_ID( BOXOBJ_CLASS_ID, 0 ) )
        return "BOXOBJ_CLASS_ID";
    if( classID == Class_ID( SPHERE_CLASS_ID, 0 ) )
        return "SPHERE_CLASS_ID";
    if( classID == Class_ID( CYLINDER_CLASS_ID, 0 ) )
        return "CYLINDER_CLASS_ID";

    if( classID == Class_ID( CONE_CLASS_ID, 0 ) )
        return "CONE_CLASS_ID";
    if( classID == Class_ID( TORUS_CLASS_ID, 0 ) )
        return "TORUS_CLASS_ID";
    if( classID == Class_ID( TUBE_CLASS_ID, 0 ) )
        return "TUBE_CLASS_ID";
    if( classID == Class_ID( HEDRA_CLASS_ID, 0 ) )
        return "HEDRA_CLASS_ID";
/* Boolean class name changed in Max 2015 */
#if MAX_VERSION_MAJOR < 17
    if( classID == Class_ID( BOOLOBJ_CLASS_ID, 0 ) )
        return "BOOLOBJ_CLASS_ID";
#endif
    if( classID == NEWBOOL_CLASS_ID )
        return "NEWBOOL_CLASS_ID";

    if( classID == GRID_OSNAP_CLASS_ID )
        return "GRID_OSNAP_CLASS_ID";

    if( classID == Class_ID( TEAPOT_CLASS_ID1, TEAPOT_CLASS_ID2 ) )
        return "TEAPOT_CLASS_ID";

    if( classID == Class_ID( PATCHGRID_CLASS_ID, 0 ) )
        return "PATCHGRID_CLASS_ID";

    if( classID == BONE_OBJ_CLASSID )
        return "BONE_OBJ_CLASSID";

    if( classID == Class_ID( RAIN_CLASS_ID, 0 ) )
        return "RAIN_CLASS_ID";
    if( classID == Class_ID( SNOW_CLASS_ID, 0 ) )
        return "SNOW_CLASS_ID";

    if( classID == Class_ID( WAVEOBJ_CLASS_ID, 0 ) )
        return "WAVEOBJ_CLASS_ID";

    if( classID == Class_ID( LOFTOBJ_CLASS_ID, 0 ) )
        return "LOFTOBJ_CLASS_ID";
    if( classID == Class_ID( LOFT_DEFCURVE_CLASS_ID, 0 ) )
        return "LOFT_DEFCURVE_CLASS_ID";
    if( classID == Class_ID( LOFT_GENERIC_CLASS_ID, 0 ) )
        return "LOFT_GENERIC_CLASS_ID";

    if( classID == Class_ID( TARGET_CLASS_ID, 0 ) )
        return "TARGET_CLASS_ID";
    if( classID == Class_ID( MORPHOBJ_CLASS_ID, 0 ) )
        return "MORPHOBJ_CLASS_ID";

    if( classID == Class_ID( SPLINESHAPE_CLASS_ID, 0 ) )
        return "SPLINESHAPE_CLASS_ID";
    if( classID == Class_ID( LINEARSHAPE_CLASS_ID, 0 ) )
        return "LINEARSHAPE_CLASS_ID";
    if( classID == Class_ID( SPLINE3D_CLASS_ID, 0 ) )
        return "SPLINE3D_CLASS_ID";
    if( classID == Class_ID( NGON_CLASS_ID, 0 ) )
        return "NGON_CLASS_ID";
    if( classID == Class_ID( DONUT_CLASS_ID, 0 ) )
        return "DONUT_CLASS_ID";
    if( classID == Class_ID( STAR_CLASS_ID, 0 ) )
        return "STAR_CLASS_ID";
    if( classID == Class_ID( RECTANGLE_CLASS_ID, 0 ) )
        return "RECTANGLE_CLASS_ID";
    if( classID == Class_ID( HELIX_CLASS_ID, 0 ) )
        return "HELIX_CLASS_ID";
    if( classID == Class_ID( ELLIPSE_CLASS_ID, 0 ) )
        return "ELLIPSE_CLASS_ID";
    if( classID == Class_ID( CIRCLE_CLASS_ID, 0 ) )
        return "CIRCLE_CLASS_ID";
    if( classID == Class_ID( TEXT_CLASS_ID, 0 ) )
        return "TEXT_CLASS_ID";
    if( classID == Class_ID( ARC_CLASS_ID, 0 ) )
        return "ARC_CLASS_ID";

    if( classID == Class_ID( SIMPLE_CAM_CLASS_ID, 0 ) )
        return "SIMPLE_CAM_CLASS_ID";
    if( classID == Class_ID( LOOKAT_CAM_CLASS_ID, 0 ) )
        return "LOOKAT_CAM_CLASS_ID";

    if( classID == Class_ID( OMNI_LIGHT_CLASS_ID, 0 ) )
        return "OMNI_LIGHT_CLASS_ID";
    if( classID == Class_ID( SPOT_LIGHT_CLASS_ID, 0 ) )
        return "SPOT_LIGHT_CLASS_ID";
    if( classID == Class_ID( DIR_LIGHT_CLASS_ID, 0 ) )
        return "DIR_LIGHT_CLASS_ID";
    if( classID == Class_ID( FSPOT_LIGHT_CLASS_ID, 0 ) )
        return "FSPOT_LIGHT_CLASS_ID";
    if( classID == Class_ID( TDIR_LIGHT_CLASS_ID, 0 ) )
        return "TDIR_LIGHT_CLASS_ID";

    if( classID == Class_ID( DUMMY_CLASS_ID, 0 ) )
        return "DUMMY_CLASS_ID";
    if( classID == Class_ID( BONE_CLASS_ID, 0 ) )
        return "BONE_CLASS_ID";
    if( classID == Class_ID( TAPEHELP_CLASS_ID, 0 ) )
        return "TAPEHELP_CLASS_ID";
    if( classID == Class_ID( GRIDHELP_CLASS_ID, 0 ) )
        return "GRIDHELP_CLASS_ID";
    if( classID == Class_ID( POINTHELP_CLASS_ID, 0 ) )
        return "POINTHELP_CLASS_ID";
    if( classID == Class_ID( PROTHELP_CLASS_ID, 0 ) )
        return "PROTHELP_CLASS_ID";

    if( classID == Class_ID( DMTL_CLASS_ID, 0 ) )
        return "DMTL_CLASS_ID";
    if( classID == Class_ID( DMTL2_CLASS_ID, 0 ) )
        return "DMTL2_CLASS_ID";
    if( classID == Class_ID( MULTI_CLASS_ID, 0 ) )
        return "MULTI_CLASS_ID";
    if( classID == Class_ID( DOUBLESIDED_CLASS_ID, 0 ) )
        return "DOUBLESIDED_CLASS_ID";
    if( classID == Class_ID( MIXMAT_CLASS_ID, 0 ) )
        return "MIXMAT_CLASS_ID";
    if( classID == Class_ID( BAKE_SHELL_CLASS_ID, 0 ) )
        return "BAKE_SHELL_CLASS_ID";

    if( classID == Class_ID( CHECKER_CLASS_ID, 0 ) )
        return "CHECKER_CLASS_ID";
    if( classID == Class_ID( MARBLE_CLASS_ID, 0 ) )
        return "MARBLE_CLASS_ID";
    if( classID == Class_ID( MASK_CLASS_ID, 0 ) )
        return "MASK_CLASS_ID";
    if( classID == Class_ID( MIX_CLASS_ID, 0 ) )
        return "MIX_CLASS_ID";
    if( classID == Class_ID( NOISE_CLASS_ID, 0 ) )
        return "NOISE_CLASS_ID";

    if( classID == Class_ID( BMTEX_CLASS_ID, 0 ) )
        return "BMTEX_CLASS_ID";
    if( classID == Class_ID( COMPOSITE_CLASS_ID, 0 ) )
        return "COMPOSITE_CLASS_ID";
    if( classID == Class_ID( FALLOFF_CLASS_ID, 0 ) )
        return "FALLOFF_CLASS_ID";
    if( classID == Class_ID( PLATET_CLASS_ID, 0 ) )
        return "PLATET_CLASS_ID";

    if( classID == Class_ID( SREND_CLASS_ID, 0 ) )
        return "SREND_CLASS_ID";

    if( classID == Class_ID( MTL_LIB_CLASS_ID, 0 ) )
        return "MTL_LIB_CLASS_ID";
    if( classID == Class_ID( MTLBASE_LIB_CLASS_ID, 0 ) )
        return "MTLBASE_LIB_CLASS_ID";
    if( classID == Class_ID( THE_SCENE_CLASS_ID, 0 ) )
        return "THE_SCENE_CLASS_ID";
    if( classID == Class_ID( MEDIT_CLASS_ID, 0 ) )
        return "MEDIT_CLASS_ID";

    // Particle flow class ids
    if( classID == ParticleChannelNew_Class_ID )
        return "ParticleChannelNew_Class_ID";
    if( classID == ParticleChannelID_Class_ID )
        return "ParticleChannelID_Class_ID";
    if( classID == ParticleChannelBool_Class_ID )
        return "ParticleChannelBool_Class_ID";
    if( classID == ParticleChannelInt_Class_ID )
        return "ParticleChannelInt_Class_ID";
    if( classID == ParticleChannelFloat_Class_ID )
        return "ParticleChannelFloat_Class_ID";
    if( classID == ParticleChannelPoint2_Class_ID )
        return "ParticleChannelPoint2_Class_ID";
    if( classID == ParticleChannelPoint3_Class_ID )
        return "ParticleChannelPoint3_Class_ID";
    if( classID == ParticleChannelPTV_Class_ID )
        return "ParticleChannelPTV_Class_ID";
    if( classID == ParticleChannelInterval_Class_ID )
        return "ParticleChannelInterval_Class_ID";
    if( classID == ParticleChannelAngAxis_Class_ID )
        return "ParticleChannelAngAxis_Class_ID";
    if( classID == ParticleChannelQuat_Class_ID )
        return "ParticleChannelQuat_Class_ID";
    if( classID == ParticleChannelMatrix3_Class_ID )
        return "ParticleChannelMatrix3_Class_ID";
    if( classID == ParticleChannelMesh_Class_ID )
        return "ParticleChannelMesh_Class_ID";
    if( classID == ParticleChannelMeshMap_Class_ID )
        return "ParticleChannelMeshMap_Class_ID";
    if( classID == ParticleChannelINode_Class_ID )
        return "ParticleChannelINode_Class_ID";
    if( classID == ParticleChannelTabPoint3_Class_ID )
        return "ParticleChannelTabPoint3_Class_ID";
    if( classID == ParticleChannelTabFace_Class_ID )
        return "ParticleChannelTabFace_Class_ID";
    if( classID == ParticleChannelTabUVVert_Class_ID )
        return "ParticleChannelTabUVVert_Class_ID";
    if( classID == ParticleChannelTabTVFace_Class_ID )
        return "ParticleChannelTabTVFace_Class_ID";
    if( classID == ParticleChannelMap_Class_ID )
        return "ParticleChannelMap_Class_ID";
    if( classID == ParticleChannelVoid_Class_ID )
        return "ParticleChannelVoid_Class_ID";

    if( classID == PFOperatorViewportRender_Class_ID )
        return "PFOperatorViewportRender_Class_ID";
    if( classID == PFOperatorDisplay_Class_ID )
        return "PFOperatorDisplay_Class_ID";
    if( classID == PFOperatorRender_Class_ID )
        return "PFOperatorRender_Class_ID";
    if( classID == PFOperatorViewportMetaball_Class_ID )
        return "PFOperatorViewportMetaball_Class_ID";
    if( classID == PFOperatorRenderMetaball_Class_ID )
        return "PFOperatorRenderMetaball_Class_ID";
    if( classID == PFOperatorSimpleBirth_Class_ID )
        return "PFOperatorSimpleBirth_Class_ID";
    if( classID == PFOperatorSimplePosition_Class_ID )
        return "PFOperatorSimplePosition_Class_ID";
    if( classID == PFOperatorSimpleSpeed_Class_ID )
        return "PFOperatorSimpleSpeed_Class_ID";
    if( classID == PFOperatorSimpleOrientation_Class_ID )
        return "PFOperatorSimpleOrientation_Class_ID";
    if( classID == PFOperatorSimpleSpin_Class_ID )
        return "PFOperatorSimpleSpin_Class_ID";
    if( classID == PFOperatorSimpleShape_Class_ID )
        return "PFOperatorSimpleShape_Class_ID";
    if( classID == PFOperatorSimpleScale_Class_ID )
        return "PFOperatorSimpleScale_Class_ID";
    if( classID == PFOperatorSimpleMapping_Class_ID )
        return "PFOperatorSimpleMapping_Class_ID";
    if( classID == PFOperatorMaterial_Class_ID )
        return "PFOperatorMaterial_Class_ID";
    if( classID == PFOperatorInstanceShape_Class_ID )
        return "PFOperatorInstanceShape_Class_ID";
    if( classID == PFOperatorMarkShape_Class_ID )
        return "PFOperatorMarkShape_Class_ID";
    if( classID == PFOperatorFacingShape_Class_ID )
        return "PFOperatorFacingShape_Class_ID";
    if( classID == PFOperatorMetaballShape_Class_ID )
        return "PFOperatorMetaballShape_Class_ID";
    if( classID == PFOperatorFragmentShape_Class_ID )
        return "PFOperatorFragmentShape_Class_ID";
    if( classID == PFOperatorLongShape_Class_ID )
        return "PFOperatorLongShape_Class_ID";
    if( classID == PFOperatorExit_Class_ID )
        return "PFOperatorExit_Class_ID";
    if( classID == PFOperatorForceSpaceWarp_Class_ID )
        return "PFOperatorForceSpaceWarp_Class_ID";
    if( classID == PFOperatorPositionOnObject_Class_ID )
        return "PFOperatorPositionOnObject_Class_ID";
    if( classID == PFOperatorPositionAgglomeration_Class_ID )
        return "PFOperatorPositionAgglomeration_Class_ID";
    if( classID == PFOperatorSpeedAvoidCollisions_Class_ID )
        return "PFOperatorSpeedAvoidCollisions_Class_ID";
    if( classID == PFOperatorSpeedCopy_Class_ID )
        return "PFOperatorSpeedCopy_Class_ID";
    if( classID == PFOperatorSpeedFollowLeader_Class_ID )
        return "PFOperatorSpeedFollowLeader_Class_ID";
    if( classID == PFOperatorSpeedKeepApart_Class_ID )
        return "PFOperatorSpeedKeepApart_Class_ID";
    if( classID == PFOperatorSpeedSurfaceNormals_Class_ID )
        return "PFOperatorSpeedSurfaceNormals_Class_ID";
    if( classID == PFOperatorOrientationFollowPath_Class_ID )
        return "PFOperatorOrientationFollowPath_Class_ID";
    if( classID == PFOperatorOrientationFacing_Class_ID )
        return "PFOperatorOrientationFacing_Class_ID";
    if( classID == PFOperatorSpinBySpeed_Class_ID )
        return "PFOperatorSpinBySpeed_Class_ID";
    if( classID == PFOperatorBirthByObjectGroup_Class_ID )
        return "PFOperatorBirthByObjectGroup_Class_ID";
    if( classID == PFOperatorScriptBirth_Class_ID )
        return "PFOperatorScriptBirth_Class_ID";
    if( classID == PFOperatorScript_Class_ID )
        return "PFOperatorScript_Class_ID";
    if( classID == PFOperatorComments_Class_ID )
        return "PFOperatorComments_Class_ID";
    if( classID == PFOperatorCache_Class_ID )
        return "PFOperatorCache_Class_ID";
    if( classID == PFOperatorMaterialStatic_Class_ID )
        return "PFOperatorMaterialStatic_Class_ID";
    if( classID == PFOperatorMaterialDynamic_Class_ID )
        return "PFOperatorMaterialDynamic_Class_ID";
    if( classID == PFOperatorMaterialFrequency_Class_ID )
        return "PFOperatorMaterialFrequency_Class_ID";

    if( classID == PFTestDuration_Class_ID )
        return "PFTestDuration_Class_ID";
    if( classID == PFTestSpawn_Class_ID )
        return "PFTestSpawn_Class_ID";
    if( classID == PFTestCollisionSpaceWarp_Class_ID )
        return "PFTestCollisionSpaceWarp_Class_ID";
    if( classID == PFTestSpawnCollisionSW_Class_ID )
        return "PFTestSpawnCollisionSW_Class_ID";
    if( classID == PFTestSpeed_Class_ID )
        return "PFTestSpeed_Class_ID";
    if( classID == PFTestSpeedGoToTarget_Class_ID )
        return "PFTestSpeedGoToTarget_Class_ID";
    if( classID == PFTestScale_Class_ID )
        return "PFTestScale_Class_ID";
    if( classID == PFTestProximity_Class_ID )
        return "PFTestProximity_Class_ID";
    if( classID == PFTestScript_Class_ID )
        return "PFTestScript_Class_ID";
    if( classID == PFTestGoToNextEvent_Class_ID )
        return "PFTestGoToNextEvent_Class_ID";
    if( classID == PFTestSplitByAmount_Class_ID )
        return "PFTestSplitByAmount_Class_ID";
    if( classID == PFTestSplitBySource_Class_ID )
        return "PFTestSplitBySource_Class_ID";
    if( classID == PFTestSplitSelected_Class_ID )
        return "PFTestSplitSelected_Class_ID";
    if( classID == PFTestGoToRotation_Class_ID )
        return "PFTestGoToRotation_Class_ID";

    if( classID == PFEngine_Class_ID )
        return "PFEngine_Class_ID";
    if( classID == ParticleGroup_Class_ID )
        return "ParticleGroup_Class_ID";
    if( classID == PFActionList_Class_ID )
        return "PFActionList_Class_ID";
    if( classID == PFArrow_Class_ID )
        return "PFArrow_Class_ID";
    if( classID == PFIntegrator_Class_ID )
        return "PFIntegrator_Class_ID";
    if( classID == PViewManager_Class_ID )
        return "PViewManager_Class_ID";
    if( classID == ParticleView_Class_ID )
        return "ParticleView_Class_ID";
    if( classID == PFActionListPool_Class_ID )
        return "PFActionListPool_Class_ID";
    if( classID == PFSystemPool_Class_ID )
        return "PFSystemPool_Class_ID";
    if( classID == PFSimpleActionState_Class_ID )
        return "PFSimpleActionState_Class_ID";
    if( classID == ParticleContainer_Class_ID )
        return "ParticleContainer_Class_ID";
    if( classID == PFNotifyDepCatcher_Class_ID )
        return "PFNotifyDepCatcher_Class_ID";

    if( classID == ParticleBitmap_Class_ID )
        return "ParticleBitmap_Class_ID";

    return "Class_ID(" + boost::lexical_cast<std::string>( classID.PartA() ) + "," +
           boost::lexical_cast<std::string>( classID.PartB() ) + ")";
}

// Searches through the class directory to find the ClassDesc for the given class id.
inline ClassDesc* get_class_desc( SClass_ID scid, Class_ID cid ) {
    DllDir& dllDir = GetCOREInterface()->GetDllDir();
    ClassDirectory& classDir = dllDir.ClassDir();
    return classDir.FindClass( scid, cid );
}

inline std::string MtlReqToString( ULONG req_flags ) {
    std::string result;
    std::string delimiter = "";

    if( req_flags & MTLREQ_2SIDE ) {
        result = result + delimiter + "MTLREQ_2SIDE";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_WIRE ) {
        result = result + delimiter + "MTLREQ_WIRE";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_WIRE_ABS ) {
        result = result + delimiter + "MTLREQ_WIRE_ABS";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_TRANSP ) {
        result = result + delimiter + "MTLREQ_TRANSP";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_UV ) {
        result = result + delimiter + "MTLREQ_UV";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_FACEMAP ) {
        result = result + delimiter + "MTLREQ_FACEMAP";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_XYZ ) {
        result = result + delimiter + "MTLREQ_XYZ";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_OXYZ ) {
        result = result + delimiter + "MTLREQ_OXYZ";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_BUMPUV ) {
        result = result + delimiter + "MTLREQ_BUMPUV";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_BGCOL ) {
        result = result + delimiter + "MTLREQ_BGCOL";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_PHONG ) {
        result = result + delimiter + "MTLREQ_PHONG";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_AUTOREFLECT ) {
        result = result + delimiter + "MTLREQ_AUTOREFLECT";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_AUTOMIRROR ) {
        result = result + delimiter + "MTLREQ_AUTOMIRROR";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_NOATMOS ) {
        result = result + delimiter + "MTLREQ_NOATMOS";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_ADDITIVE_TRANSP ) {
        result = result + delimiter + "MTLREQ_ADDITIVE_TRANSP";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_VIEW_DEP ) {
        result = result + delimiter + "MTLREQ_VIEW_DEP";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_UV2 ) {
        result = result + delimiter + "MTLREQ_UV2";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_BUMPUV2 ) {
        result = result + delimiter + "MTLREQ_BUMPUV2";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_PREPRO ) {
        result = result + delimiter + "MTLREQ_PREPRO";
        delimiter = " | ";
    }
    if( req_flags & MTLREQ_DONTMERGE_FRAGMENTS ) {
        result = result + delimiter + "MTLREQ_DONTMERGE_FRAGMENTS";
        delimiter = " | ";
    }
    if( req_flags & MTLREQ_DISPLACEMAP ) {
        result = result + delimiter + "MTLREQ_DISPLACEMAP";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_SUPERSAMPLE ) {
        result = result + delimiter + "MTLREQ_SUPERSAMPLE";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_WORLDCOORDS ) {
        result = result + delimiter + "MTLREQ_WORLDCOORDS";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_TRANSP_IN_VP ) {
        result = result + delimiter + "MTLREQ_TRANSP_IN_VP";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_FACETED ) {
        result = result + delimiter + "MTLREQ_FACETED";
        delimiter = " | ";
    }

    if( req_flags & MTLREQ_NOEXPOSURE ) {
        result = result + delimiter + "MTLREQ_NOEXPOSURE";
        delimiter = " | ";
    }
    if( req_flags & MTLREQ_SS_GLOBAL ) {
        result = result + delimiter + "MTLREQ_SS_GLOBAL";
        delimiter = " | ";
    }
    if( req_flags & MTLREQ_REND1 ) {
        result = result + delimiter + "MTLREQ_REND1";
        delimiter = " | ";
    }
    if( req_flags & MTLREQ_REND2 ) {
        result = result + delimiter + "MTLREQ_REND2";
        delimiter = " | ";
    }
    if( req_flags & MTLREQ_REND3 ) {
        result = result + delimiter + "MTLREQ_REND3";
        delimiter = " | ";
    }
    if( req_flags & MTLREQ_REND4 ) {
        result = result + delimiter + "MTLREQ_REND4";
        delimiter = " | ";
    }

    return result;
}

inline frantic::tstring RefTargetClassName( ReferenceTarget* ref ) {
    TSTR name;
    ref->GetClassName( name );
    frantic::tstring ref_name = static_cast<const TCHAR*>( name );
    return ref_name;
}

#pragma warning( pop )

} // namespace geopipe
} // namespace max3d
} // namespace frantic
