// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// experimental wrapping of max UI elements in macros

// These are used inside the paramblock2 declaration, and should be used without semicolonss or commas between them

/* ---- EXAMPLES ----
  // ...
        // # of rollouts - for P_MULTIMAP
        3,
        // rollouts - for P_AUTO_UI
        emitterrollout_liquidobject,	IDD_EMITTER_FLUIDOBJECT,	IDS_FLUIDOBJECT,	0, 0, NULL,
        emitterrollout_size,			IDD_EMITTER_SIZE,			IDS_EMITTERSIZE,	0, 0,
  NULL, emitterrollout_density,			IDD_EMITTER_DENSITY,		IDS_DENSITY, 0, 0, &ep_density_dlgproc,

        MAXUI_SPINNER_UNIVERSE(	emitterrollout_size,	emitterhelper_size_radius, IDC_EMITTER_RADIUS,
                                                        "EmitterRadius", P_ANIMATABLE + P_RESET_DEFAULT,
                                                        0, 1, 0, 1000 )

        MAXUI_LISTBOX_INT(		emitterrollout_density,	emitterhelper_density_type, IDC_EMITTER_DENSITY_TYPE,
                                                        "DensityType", P_RESET_DEFAULT,
                                                        0, &ep_accessor )

        MAXUI_INODEBUTTON_CLASSID(	emitterrollout_liquidobject,	emitterhelper_liquidobject,
  IDC_EMITTERPICKFLUIDBUTTON, "FluidObject", 0, "Choose the fluid simulator you would like to affect.",
  LIQUIDOBJECT_CLASS_ID )

        MAXUI_CHECKBOX(			emitterrollout_size,	emitterhelper_size_isuniform, IDC_EMITTER_ISUNIFORM,
                                                        "IsUniformVelocity", 0,
                                                        false )

         ...
*/

#if MAX_VERSION_MAJOR >= 15
#define PD_END p_end
#else
#define PD_END end
#endif

// A spinner without units
#define MAXUI_SPINNER_FLOAT( rolloutid, id, dlgid, name, flags, stringtablevalue, defaultv, ms_defaultv, minv, maxv )  \
    id, _T( name ), TYPE_FLOAT, flags, stringtablevalue, p_default, double( defaultv ), p_ms_default,                  \
        double( ms_defaultv ), p_range, double( minv ), double( maxv ), p_ui, rolloutid, TYPE_SPINNER, EDITTYPE_FLOAT, \
        dlgid, dlgid##_SPIN, SPIN_AUTOSCALE, PD_END

#define MAXUI_SPINNER_FLOAT_ACCESSOR( rolloutid, id, dlgid, name, flags, defaultv, ms_defaultv, minv, maxv, accessor ) \
    id, _T( name ), TYPE_FLOAT, flags, IDS_GENERICSTRING, p_default, double( defaultv ), p_ms_default,                 \
        double( ms_defaultv ), p_range, double( minv ), double( maxv ), p_ui, rolloutid, TYPE_SPINNER, EDITTYPE_FLOAT, \
        dlgid, dlgid##_SPIN, SPIN_AUTOSCALE, p_accessor, accessor, PD_END

// A spinner with max's units
#define MAXUI_SPINNER_UNIVERSE( rolloutid, id, dlgid, name, flags, defaultv, ms_defaultv, minv, maxv )                 \
    id, _T( name ), TYPE_FLOAT, flags, IDS_GENERICSTRING, p_default, double( defaultv ), p_ms_default,                 \
        double( ms_defaultv ), p_range, double( minv ), double( maxv ), p_ui, rolloutid, TYPE_SPINNER,                 \
        EDITTYPE_UNIVERSE, dlgid, dlgid##_SPIN, SPIN_AUTOSCALE, PD_END

#define MAXUI_SPINNER_UNIVERSE_ACCESSOR( rolloutid, id, dlgid, name, flags, defaultv, ms_defaultv, minv, maxv,         \
                                         accessor )                                                                    \
    id, _T( name ), TYPE_FLOAT, flags, IDS_GENERICSTRING, p_default, double( defaultv ), p_ms_default,                 \
        double( ms_defaultv ), p_range, double( minv ), double( maxv ), p_ui, rolloutid, TYPE_SPINNER,                 \
        EDITTYPE_UNIVERSE, dlgid, dlgid##_SPIN, SPIN_AUTOSCALE, p_accessor, accessor, PD_END

// A spinner with time units (displays frames)
#define MAXUI_SPINNER_TIME( rolloutid, id, dlgid, name, flags, defaultv, ms_defaultv, minv, maxv )                     \
    id, _T( name ), TYPE_TIMEVALUE, flags, IDS_GENERICSTRING, p_default, int( defaultv ), p_ms_default,                \
        int( ms_defaultv ), p_range, int( minv ), int( maxv ), p_ui, rolloutid, TYPE_SPINNER, EDITTYPE_INT, dlgid,     \
        dlgid##_SPIN, SPIN_AUTOSCALE, PD_END

#define MAXUI_SPINNER_TIME_ACCESSOR( rolloutid, id, dlgid, name, flags, defaultv, ms_defaultv, minv, maxv, accessor )  \
    id, _T( name ), TYPE_TIMEVALUE, flags, IDS_GENERICSTRING, p_default, int( defaultv ), p_ms_default,                \
        int( ms_defaultv ), p_range, int( minv ), int( maxv ), p_ui, rolloutid, TYPE_SPINNER, EDITTYPE_INT, dlgid,     \
        dlgid##_SPIN, SPIN_AUTOSCALE, p_accessor, accessor, PD_END

// A spinner with integer units
#define MAXUI_SPINNER_INT( rolloutid, id, dlgid, name, flags, defaultv, ms_defaultv, minv, maxv )                      \
    id, _T( name ), TYPE_INT, flags, IDS_GENERICSTRING, p_default, int( defaultv ), p_ms_default, int( ms_defaultv ),  \
        p_range, int( minv ), int( maxv ), p_ui, rolloutid, TYPE_SPINNER, EDITTYPE_INT, dlgid, dlgid##_SPIN,           \
        SPIN_AUTOSCALE, PD_END

#define MAXUI_SPINNER_INT_ACCESSOR( rolloutid, id, dlgid, name, flags, defaultv, ms_defaultv, minv, maxv, accessor )   \
    id, _T( name ), TYPE_INT, flags, IDS_GENERICSTRING, p_default, int( defaultv ), p_ms_default, int( ms_defaultv ),  \
        p_range, int( minv ), int( maxv ), p_ui, rolloutid, TYPE_SPINNER, EDITTYPE_INT, dlgid, dlgid##_SPIN,           \
        SPIN_AUTOSCALE, p_accessor, accessor, PD_END

// A textbox control

#define MAXUI_TEXTBOX( rolloutid, id, dlgid, name, flags, defaultv, ms_defaultv )                                      \
    id, _T( name ), TYPE_STRING, flags, IDS_GENERICSTRING, p_default, _T( defaultv ), p_ms_default, _T( ms_defaultv ), \
        p_ui, rolloutid, TYPE_EDITBOX, dlgid, PD_END

// A checkbox
#define MAXUI_CHECKBOX( rolloutid, id, dlgid, name, flags, defaultv )                                                  \
    id, _T( name ), TYPE_BOOL, flags, IDS_GENERICSTRING, p_default, BOOL( defaultv ), p_ui, rolloutid,                 \
        TYPE_SINGLECHEKBOX, dlgid, PD_END
#define MAXUI_CHECKBOX_ACCESSOR( rolloutid, id, dlgid, name, flags, defaultv, accessor )                               \
    id, _T( name ), TYPE_BOOL, flags, IDS_GENERICSTRING, p_default, BOOL( defaultv ), p_ui, rolloutid,                 \
        TYPE_SINGLECHEKBOX, dlgid, p_accessor, accessor, PD_END

// An inode pickbutton for a specified classid
#define MAXUI_INODEBUTTON_CLASSID( rolloutid, id, dlgid, name, flags, prompt, classid )                                \
    id, _T( name ), TYPE_INODE, flags, IDS_GENERICSTRING, p_prompt, _T( prompt ), p_ui, rolloutid,                     \
        TYPE_PICKNODEBUTTON, dlgid, p_classID, classid, PD_END

// A listbox with an int value
#define MAXUI_LISTBOX_INT( rolloutid, id, dlgid, name, flags, defaultv )                                               \
    id, _T( name ), TYPE_INT, flags, IDS_GENERICSTRING, p_default, int( defaultv ), p_ui, rolloutid, TYPE_INTLISTBOX,  \
        dlgid, 0, PD_END

// A listbox with an int value
#define MAXUI_LISTBOX_INT_ACCESSOR( rolloutid, id, dlgid, name, flags, defaultv, accessor )                            \
    id, _T( name ), TYPE_INT, flags, IDS_GENERICSTRING, p_default, int( defaultv ), p_ui, rolloutid, TYPE_INTLISTBOX,  \
        dlgid, 0, p_accessor, accessor, PD_END
