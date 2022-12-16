// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"
#ifdef DONTBUILDTHIS

//#include <maxscrpt/maxobj.h>
//#include <maxscrpt/value.h>
#include <frantic/max3d/animation/serialize.hpp>

#pragma warning( push )
#pragma warning( disable : 4512 )
#pragma warning( disable : 4100 )
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#pragma warning( pop )

namespace {
TCHAR* toTCHAR( const frantic::tstring& str ) {
    const TCHAR* nameStr = str.c_str();
    size_t len = wcslen( nameStr ) + 1;
    TCHAR* buffer = (TCHAR*)malloc( len );
    memcpy( buffer, nameStr, len );
    return buffer;
}

void insert_point3( Point3 point, std::map<std::string, float>& data, const std::string& name ) {
    data.insert( std::pair<std::string, float>( name + "_X", point.x ) );
    data.insert( std::pair<std::string, float>( name + "_Y", point.y ) );
    data.insert( std::pair<std::string, float>( name + "_Z", point.z ) );
}

void insert_point4( Point4 point, std::map<std::string, float>& data, const std::string& name ) {
    data.insert( std::pair<std::string, float>( name + "_X", point.x ) );
    data.insert( std::pair<std::string, float>( name + "_Y", point.y ) );
    data.insert( std::pair<std::string, float>( name + "_Z", point.z ) );
    data.insert( std::pair<std::string, float>( name + "_W", point.w ) );
}

void insert_quat( Quat quat, std::map<std::string, float>& data, const std::string& name ) {
    data.insert( std::pair<std::string, float>( name + "_W", quat.w ) );
    data.insert( std::pair<std::string, float>( name + "_X", quat.x ) );
    data.insert( std::pair<std::string, float>( name + "_Y", quat.y ) );
    data.insert( std::pair<std::string, float>( name + "_Z", quat.z ) );
}

Point3 get_point3( std::map<std::string, float>& data, const std::string& name ) {
    float x = data.find( name + "_X" )->second;
    float y = data.find( name + "_Y" )->second;
    float z = data.find( name + "_Z" )->second;

    return Point3( x, y, z );
}

Point4 get_point4( std::map<std::string, float>& data, const std::string& name ) {
    float x = data.find( name + "_X" )->second;
    float y = data.find( name + "_Y" )->second;
    float z = data.find( name + "_Z" )->second;
    float w = data.find( name + "_W" )->second;

    return Point4( x, y, z, w );
}

Quat get_quat( std::map<std::string, float>& data, const std::string& name ) {
    float w = data.find( name + "_W" )->second;
    float x = data.find( name + "_X" )->second;
    float y = data.find( name + "_Y" )->second;
    float z = data.find( name + "_Z" )->second;

    return Quat( x, y, z, w );
}

enum key_type {
    BEZ_FLOAT = 0,
    BEZ_POINT3,
    BEZ_SCALE,
    BEZ_POINT4,
    BEZ_QUAT,
    LIN_FLOAT,
    LIN_POINT3,
    LIN_ROT,
    LIN_SCALE,
    TCB_FLOAT,
    TCB_POINT3,
    TCB_ROT,
    TCB_SCALE,
    TCB_POINT4,
    UNKNOWN
};

class key_serializer {
    int m_time;
    unsigned long m_flags;

    std::map<std::string, float> m_data;
    key_type m_type;

  private:
    void handle_bez_float( IBezFloatKey* key ) {
        m_data.insert( std::pair<std::string, float>( "Value", key->val ) );
        m_data.insert( std::pair<std::string, float>( "InTan", key->intan ) );
        m_data.insert( std::pair<std::string, float>( "OutTan", key->outtan ) );
        m_data.insert( std::pair<std::string, float>( "InLength", key->inLength ) );
        m_data.insert( std::pair<std::string, float>( "OutLength", key->outLength ) );
    }

    void handle_bez_point3( IBezPoint3Key* key ) {
        insert_point3( key->val, m_data, "Value" );
        insert_point3( key->intan, m_data, "InTan" );
        insert_point3( key->outtan, m_data, "OutTan" );
        insert_point3( key->inLength, m_data, "InLength" );
        insert_point3( key->outLength, m_data, "OutLength" );
    }

    void handle_bez_scale( IBezScaleKey* key ) {
        insert_point3( key->val.s, m_data, "Point" );
        insert_quat( key->val.q, m_data, "Quat" );
        insert_point3( key->intan, m_data, "InTan" );
        insert_point3( key->outtan, m_data, "OutTan" );
        insert_point3( key->inLength, m_data, "InLength" );
        insert_point3( key->outLength, m_data, "OutLength" );
    }

    void handle_bez_point4( IBezPoint4Key* key ) {
        insert_point4( key->val, m_data, "Value" );
        insert_point4( key->intan, m_data, "InTan" );
        insert_point4( key->outtan, m_data, "OutTan" );
        insert_point4( key->inLength, m_data, "InLength" );
        insert_point4( key->outLength, m_data, "OutLength" );
    }

    void handle_bez_quat( IBezQuatKey* key ) { insert_quat( key->val, m_data, "Value" ); }

    void handle_lin_float( ILinFloatKey* key ) { m_data.insert( std::pair<std::string, float>( "Value", key->val ) ); }

    void handle_lin_point3( ILinPoint3Key* key ) { insert_point3( key->val, m_data, "Value" ); }

    void handle_lin_rot( ILinRotKey* key ) { insert_quat( key->val, m_data, "Value" ); }

    void handle_lin_scale( ILinScaleKey* key ) {
        insert_point3( key->val.s, m_data, "Point" );
        insert_quat( key->val.q, m_data, "Quat" );
    }

    void handle_tcb_base( ITCBKey* key ) {
        m_data.insert( std::pair<std::string, float>( "Tension", key->tens ) );
        m_data.insert( std::pair<std::string, float>( "Cont", key->cont ) );
        m_data.insert( std::pair<std::string, float>( "Bias", key->bias ) );
        m_data.insert( std::pair<std::string, float>( "EaseIn", key->easeIn ) );
        m_data.insert( std::pair<std::string, float>( "EaseOut", key->easeOut ) );
    }

    void handle_tcb_float( ITCBFloatKey* key ) {
        handle_tcb_base( key );
        m_data.insert( std::pair<std::string, float>( "Value", key->val ) );
    }

    void handle_tcb_point3( ITCBPoint3Key* key ) {
        handle_tcb_base( key );
        insert_point3( key->val, m_data, "Value" );
    }

    void handle_tcb_rot( ITCBRotKey* key ) {
        handle_tcb_base( key );
        insert_point3( key->val.axis, m_data, "Axis" );
        m_data.insert( std::pair<std::string, float>( "Angle", key->val.angle ) );
    }

    void handle_tcb_scale( ITCBScaleKey* key ) {
        handle_tcb_base( key );
        insert_point3( key->val.s, m_data, "Point" );
        insert_quat( key->val.q, m_data, "Quat" );
    }

    void handle_tcb_point4( ITCBPoint4Key* key ) {
        handle_tcb_base( key );
        insert_point4( key->val, m_data, "Value" );
    }

    IBezFloatKey* get_bez_float() {
        IBezFloatKey* key = new IBezFloatKey();
        key->val = m_data.find( "Value" )->second;
        key->intan = m_data.find( "InTan" )->second;
        key->outtan = m_data.find( "OutTan" )->second;
        key->inLength = m_data.find( "InLength" )->second;
        key->outLength = m_data.find( "OutLength" )->second;
        return key;
    }

    IBezPoint3Key* get_bez_point3() {
        IBezPoint3Key* key = new IBezPoint3Key();
        key->val = get_point3( m_data, "Value" );
        key->intan = get_point3( m_data, "InTan" );
        key->outtan = get_point3( m_data, "OutTan" );
        key->inLength = get_point3( m_data, "InLength" );
        key->outLength = get_point3( m_data, "OutLength" );

        return key;
    }

    IBezPoint4Key* get_bez_point4() {
        IBezPoint4Key* key = new IBezPoint4Key();
        key->val = get_point4( m_data, "Value" );
        key->intan = get_point4( m_data, "InTan" );
        key->outtan = get_point4( m_data, "OutTan" );
        key->inLength = get_point4( m_data, "InLength" );
        key->outLength = get_point4( m_data, "OutLength" );

        return key;
    }

    IBezScaleKey* get_bez_scale() {
        IBezScaleKey* key = new IBezScaleKey();
        key->val.s = get_point3( m_data, "Point" );
        key->val.q = get_quat( m_data, "Quat" );
        key->intan = get_point3( m_data, "InTan" );
        key->outtan = get_point3( m_data, "OutTan" );
        key->inLength = get_point3( m_data, "InLength" );
        key->outLength = get_point3( m_data, "OutLength" );
        return key;
    }

    IBezQuatKey* get_bez_quat() {
        IBezQuatKey* key = new IBezQuatKey();
        key->val = get_quat( m_data, "Value" );
        return key;
    }

    ILinFloatKey* get_lin_float() {
        ILinFloatKey* key = new ILinFloatKey();
        key->val = m_data.find( "Value" )->second;
        return key;
    }

    ILinPoint3Key* get_lin_point3() {
        ILinPoint3Key* key = new ILinPoint3Key();
        key->val = get_point3( m_data, "Value" );
        return key;
    }

    ILinRotKey* get_lin_rot() {
        ILinRotKey* key = new ILinRotKey();
        key->val = get_quat( m_data, "Value" );
        return key;
    }

    ILinScaleKey* get_lin_scale() {
        ILinScaleKey* key = new ILinScaleKey();
        key->val.s = get_point3( m_data, "Point" );
        key->val.q = get_quat( m_data, "Quat" );
        return key;
    }

    void get_tcb_base( ITCBKey* key ) {
        key->tens = m_data.find( "Tension" )->second;
        key->cont = m_data.find( "Cont" )->second;
        key->bias = m_data.find( "Bias" )->second;
        key->easeIn = m_data.find( "EaseIn" )->second;
        key->easeOut = m_data.find( "EaseOut" )->second;
    }

    ITCBFloatKey* get_tcb_float() {
        ITCBFloatKey* key = new ITCBFloatKey();
        get_tcb_base( key );
        key->val = m_data.find( "Value" )->second;
        return key;
    }

    ITCBPoint3Key* get_tcb_point3() {
        ITCBPoint3Key* key = new ITCBPoint3Key();
        get_tcb_base( key );
        key->val = get_point3( m_data, "Value" );
        return key;
    }

    ITCBRotKey* get_tcb_rot() {
        ITCBRotKey* key = new ITCBRotKey();
        get_tcb_base( key );
        key->val.axis = get_point3( m_data, "Axis" );
        key->val.angle = m_data.find( "Angle" )->second;
        return key;
    }

    ITCBScaleKey* get_tcb_scale() {
        ITCBScaleKey* key = new ITCBScaleKey();
        get_tcb_base( key );
        key->val.s = get_point3( m_data, "Point" );
        key->val.q = get_quat( m_data, "Quat" );
        return key;
    }

    ITCBPoint4Key* get_tcb_point4() {
        ITCBPoint4Key* key = new ITCBPoint4Key();
        get_tcb_base( key );
        key->val = get_point4( m_data, "Value" );
        return key;
    }

  public:
    key_serializer()
        : m_type( UNKNOWN ) {}

    key_serializer( IKey* key, unsigned long controller_class_id ) {
        m_time = key->time;
        m_flags = key->flags;

        switch( controller_class_id ) {
        case HYBRIDINTERP_FLOAT_CLASS_ID:
            handle_bez_float( (IBezFloatKey*)key );
            m_type = BEZ_FLOAT;
            break;
        case HYBRIDINTERP_POINT3_CLASS_ID:
        case HYBRIDINTERP_POSITION_CLASS_ID:
        case HYBRIDINTERP_COLOR_CLASS_ID:
            handle_bez_point3( (IBezPoint3Key*)key );
            m_type = BEZ_POINT3;
            break;
        case HYBRIDINTERP_ROTATION_CLASS_ID:
            handle_bez_quat( (IBezQuatKey*)key );
            m_type = BEZ_QUAT;
            break;
        case HYBRIDINTERP_SCALE_CLASS_ID:
            handle_bez_scale( (IBezScaleKey*)key );
            m_type = BEZ_SCALE;
            break;
        case HYBRIDINTERP_POINT4_CLASS_ID:
        case HYBRIDINTERP_FRGBA_CLASS_ID:
            handle_bez_point4( (IBezPoint4Key*)key );
            m_type = BEZ_POINT4;
            break;
        case LININTERP_FLOAT_CLASS_ID:
            handle_lin_float( (ILinFloatKey*)key );
            m_type = LIN_FLOAT;
            break;
        case LININTERP_POSITION_CLASS_ID:
            handle_lin_point3( (ILinPoint3Key*)key );
            m_type = LIN_POINT3;
            break;
        case LININTERP_ROTATION_CLASS_ID:
            handle_lin_rot( (ILinRotKey*)key );
            m_type = LIN_ROT;
            break;
        case LININTERP_SCALE_CLASS_ID:
            handle_lin_scale( (ILinScaleKey*)key );
            m_type = LIN_SCALE;
            break;
        case TCBINTERP_FLOAT_CLASS_ID:
            handle_tcb_float( (ITCBFloatKey*)key );
            m_type = TCB_FLOAT;
            break;
        case TCBINTERP_POINT3_CLASS_ID:
        case TCBINTERP_POSITION_CLASS_ID:
            handle_tcb_point3( (ITCBPoint3Key*)key );
            m_type = TCB_POINT3;
            break;
        case TCBINTERP_ROTATION_CLASS_ID:
            handle_tcb_rot( (ITCBRotKey*)key );
            m_type = TCB_ROT;
            break;
        case TCBINTERP_SCALE_CLASS_ID:
            handle_tcb_scale( (ITCBScaleKey*)key );
            m_type = TCB_SCALE;
            break;
        case TCBINTERP_POINT4_CLASS_ID:
            handle_tcb_point4( (ITCBPoint4Key*)key );
            m_type = TCB_POINT4;
            break;
        default:
            mprintf( _T( "Key Serializer Error : Unkown key type\n  Parent Controller Class ID : %x\n" ),
                     controller_class_id );
            m_type = UNKNOWN;
        }
    }

    IKey* get_key() {
        IKey* key = 0;
        switch( m_type ) {
        case BEZ_FLOAT:
            key = get_bez_float();
            break;
        case BEZ_POINT3:
            key = get_bez_point3();
            break;
        case BEZ_QUAT:
            key = get_bez_quat();
            break;
        case BEZ_SCALE:
            key = get_bez_scale();
            break;
        case BEZ_POINT4:
            key = get_bez_point4();
            break;
        case LIN_FLOAT:
            key = get_lin_float();
            break;
        case LIN_POINT3:
            key = get_lin_point3();
            break;
        case LIN_ROT:
            key = get_lin_rot();
            break;
        case LIN_SCALE:
            key = get_lin_scale();
            break;
        case TCB_FLOAT:
            key = get_tcb_float();
            break;
        case TCB_POINT3:
            key = get_tcb_point3();
            break;
        case TCB_ROT:
            key = get_tcb_rot();
            break;
        case TCB_SCALE:
            key = get_tcb_scale();
            break;
        case TCB_POINT4:
            key = get_tcb_point4();
            break;
        default:
            mprintf( _T("Key Deserializer Error : Unkown key type\n" ) );
        }

        if( key != 0 ) {
            key->time = m_time;
            key->flags = m_flags;
        }

        return key;
    }

    friend class boost::serialization::access;
    template <class archive>
    void serialize( archive& ar, const unsigned int /*version*/ ) {
        using boost::serialization::make_nvp;

        ar& make_nvp( "Type", m_type );
        ar& make_nvp( "Time", m_time );
        ar& make_nvp( "Flags", m_flags );

        ar& make_nvp( "Data", m_data );
    }
};

class controller_serializer {
    std::vector<key_serializer> m_keyValues;
    std::vector<controller_serializer> m_subControllers;
    std::vector<controller_serializer> m_easeCurves;
    std::vector<controller_serializer> m_multCurves;
    unsigned long m_classIDA;
    unsigned long m_classIDB;
    unsigned long m_superClassID;

  public:
    controller_serializer() {}

    // As of know this only handles keyable interfaces
    //(If a keyable interface fails it class ID needs to be added in the key_serializer serialize / get_key functions)
    // Other controllers/ such as noise/ may not use keys
    // A special case will need to be created for each of these nodes if needed
    controller_serializer( Control* controller ) {
        m_classIDA = controller->ClassID().PartA();
        m_classIDB = controller->ClassID().PartB();
        m_superClassID = controller->SuperClassID();

        // handle basic keyable controllers
        if( controller->IsKeyable() ) {
            IKeyControl* keyInterface = GetKeyControlInterface( controller );
            if( keyInterface ) {
                // we need to create the key pointer like this since it can be different sizes
                AnyKey ak( keyInterface->GetKeySize() );
                IKey* key = ak;

                for( int i = 0; i < keyInterface->GetNumKeys(); ++i ) {
                    keyInterface->GetKey( i, key );
                    m_keyValues.push_back( key_serializer( key, m_classIDA ) );
                }
            }
        } else {
            // put special cases here
            mprintf( _T( "Warning: Unhandled Unkeyable Controller. Class ID: %x_%x.\n" ), m_classIDA, m_classIDB );
        }

        // if its a leaf check if there are any ease or mult curves
        if( controller->IsLeaf() ) {
            // it seems like there should be a better way to get at the mult/ease curves but I can't find it
            // using the GetEaseListInterface and GetMultListInterface macros just return null
            int numSubs = ( controller->NumEaseCurves() != 0 ) + ( controller->NumMultCurves() != 0 );
            for( int i = 0; i < numSubs; ++i ) {
                Animatable* sub = controller->SubAnim( i );
                if( sub->ClassID().PartA() == EASE_LIST_CLASS_ID ) {
                    EaseCurveList* el = (EaseCurveList*)controller->SubAnim( i );
                    for( int j = 0; j < el->NumSubs(); ++j )
                        m_easeCurves.push_back( controller_serializer( (Control*)el->SubAnim( j ) ) );
                }

                if( sub->ClassID().PartA() == MULT_LIST_CLASS_ID ) {
                    MultCurveList* ml = (MultCurveList*)controller->SubAnim( i );
                    for( int j = 0; j < ml->NumSubs(); ++j )
                        m_multCurves.push_back( controller_serializer( (Control*)ml->SubAnim( j ) ) );
                }
            }
        }
        // get the sub controllers
        else {
            for( int i = 0; i < controller->NumSubs(); ++i ) {
                Animatable* sub = controller->SubAnim( i );
                if( sub ) {
                    // some controllers may use parameter blocks
                    // this currently is not handled
                    if( sub->ClassID().PartA() != PARAMETER_BLOCK2_CLASS_ID )
                        m_subControllers.push_back( controller_serializer( (Control*)controller->SubAnim( i ) ) );
                    else
                        mprintf( _T( "Warning: Cannot Serialize Parameter Block.\n" ) );
                }
            }
        }
    }

    friend class boost::serialization::access;
    template <class archive>
    void serialize( archive& ar, const unsigned int /*version*/ ) {
        using boost::serialization::make_nvp;
        ar& make_nvp( "KeyValues", m_keyValues );
        ar& make_nvp( "ClassIDA", m_classIDA );
        ar& make_nvp( "ClassIDB", m_classIDB );
        ar& make_nvp( "SuperClassID", m_superClassID );
        ar& make_nvp( "SubControllers", m_subControllers );
        ar& make_nvp( "EaseCurves", m_easeCurves );
        ar& make_nvp( "MultCurves", m_multCurves );
    }

    Control* get_controller() {
        Control* controller = 0;
        controller = (Control*)CreateInstance( m_superClassID, Class_ID( m_classIDA, m_classIDB ) );

        if( controller ) {

            IKeyControl* keyInterface = GetKeyControlInterface( controller );
            if( keyInterface ) {
                for( size_t i = 0; i < m_keyValues.size(); ++i ) {
                    IKey* key = m_keyValues[i].get_key();
                    if( key )
                        keyInterface->AppendKey( key );
                }
            }

            // go backwards to maintain order
            for( size_t i = m_easeCurves.size(); i > 0; --i ) {
                Control* c = m_easeCurves[i - 1].get_controller();
                if( c )
                    controller->AppendEaseCurve( c );
            }

            for( size_t i = m_multCurves.size(); i > 0; --i ) {
                Control* c = m_multCurves[i - 1].get_controller();
                if( c )
                    controller->AppendMultCurve( c );
            }

            for( size_t i = 0; i < m_subControllers.size(); ++i ) {
                Control* c = m_subControllers[i].get_controller();
                if( c )
                    controller->AssignController( c, (int)i );
                else
                    // if we can not create a sub controller it can cause the parent to be invalid and max to crash
                    return 0;
            }
        } else
            mprintf(
                _T( "Serialize Controller Error : Could not create controller with class id %x_%x and super class id %x\n" ),
                m_classIDA, m_classIDB, m_superClassID );

        return controller;
    }
};

class tvnode_serializer {
    std::vector<tvnode_serializer> m_subnodes;
    std::vector<controller_serializer> m_controllers;
    std::vector<frantic::tstring> m_controllerNames;
    std::vector<frantic::tstring> m_subnodeNames;

  public:
    tvnode_serializer() {}

    tvnode_serializer( ITrackViewNode* node ) {
        for( int i = 0; i < node->NumSubs(); ++i ) {
            Animatable* sub = node->SubAnim( i );

            if( sub->ClassID() == TVNODE_CLASS_ID ) {
                tvnode_serializer tvns( (ITrackViewNode*)sub );
                m_subnodes.push_back( tvns );
                m_subnodeNames.push_back( node->GetName( i ) );
            } else {
                Control* controller = (Control*)sub;
                controller_serializer cs( controller );
                m_controllers.push_back( cs );
                m_controllerNames.push_back( node->GetName( i ) );
            }
        }
    }

    friend class boost::serialization::access;
    template <class archive>
    void serialize( archive& ar, const unsigned int /*version*/ ) {
        using boost::serialization::make_nvp;
        ar& make_nvp( "SubNodes", m_subnodes );
        ar& make_nvp( "Controllers", m_controllers );
        ar& make_nvp( "SubnodeNames", m_subnodeNames );
        ar& make_nvp( "ControllerNames", m_controllerNames );
    }

    void create_tvnode( ITrackViewNode* parent ) {

        for( size_t i = 0; i < m_subnodes.size(); ++i ) {
            TCHAR* nameStr = toTCHAR( m_subnodeNames[i] );

            ITrackViewNode* newNode = CreateITrackViewNode();
            parent->AddNode( newNode, nameStr, Class_ID( 0, 0 ) );
            m_subnodes[i].create_tvnode( newNode );
        }

        for( size_t i = 0; i < m_controllers.size(); ++i ) {
            Control* newCtrl = m_controllers[i].get_controller();

            if( newCtrl ) {
                TCHAR* nameStr = toTCHAR( m_controllerNames[i] );
                parent->AddController( newCtrl, nameStr, Class_ID( 0, 0 ) );
            }
        }
    }
};

} // end anonymous namespace

serialization_interface::serialization_interface() {
    FFCreateDescriptor c( this, Interface_ID( 0x534847f5, 0xa5c298d ), _T( "FranticSerializer" ), 0 );
    c.add_function( &serialization_interface::test, _T( "Test" ), _T( "Node" ) );

    c.add_function( &serialization_interface::serialize_tvnode, _T( "Serialize_TVNode" ), _T( "TrackViewNode" ) );
    c.add_function( &serialization_interface::deserialize_tvnode, _T( "Deserialize_TVNode" ),
                    _T( "SerializedValueString" ), _T( "Parent" ) );

    c.add_function( &serialization_interface::serialize_controller, _T( "Serialize_Controller" ), _T( "Controller" ) );
    c.add_function( &serialization_interface::deserialize_controller, _T( "Deserialize_Controller" ),
                    _T( "SerializedValueString" ) );
}

std::string serialization_interface::serialize_tvnode( Value* tvnode ) {
    try {
        std::stringstream ss;
        tvnode_serializer tvns( tvnode->to_trackviewnode() );
        boost::archive::xml_oarchive xml( ss );
        xml << boost::serialization::make_nvp( "TVNode", tvns );
        return ss.str();

    } catch( const std::exception& e ) {
        mprintf( _T( "TVNode Serialization Error : %s\n" ), e.what() );
    }

    return "";
}

void serialization_interface::deserialize_tvnode( const std::string& serializedValueString, Value* parent ) {
    try {
        std::stringstream ss( serializedValueString );
        tvnode_serializer tvns;
        boost::archive::xml_iarchive xml( ss );
        xml >> boost::serialization::make_nvp( "TVNode", tvns );
        tvns.create_tvnode( parent->to_trackviewnode() );
    } catch( const std::exception& e ) {
        mprintf( _T( "TVNode Deserialization Error : %s\n" ), e.what() );
    }
}

std::string serialization_interface::serialize_controller( Control* controller ) {
    std::stringstream ss;
    controller_serializer cs( controller );
    boost::archive::xml_oarchive xml( ss );
    xml << boost::serialization::make_nvp( "Controller", cs );
    return ss.str();
}

Control* serialization_interface::deserialize_controller( const std::string& serializedValueString ) {
    std::stringstream buffer( serializedValueString );
    controller_serializer cs;
    boost::archive::xml_iarchive xml( buffer );
    xml >> boost::serialization::make_nvp( "Controller", cs );
    return cs.get_controller();
}

std::string serialization_interface::test( Value* node ) {
    ITrackViewNode* tvnode = node->to_trackviewnode();
    return frantic::strings::to_string( tvnode->GetName( 0 ) );
}

void serialization_interface::initialize() {}

static serialization_interface theSerializationInterface;

void initialize_serialization_interface() { theSerializationInterface.initialize(); }

#endif
