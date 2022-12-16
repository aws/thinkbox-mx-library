// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef MAX_RELEASE
#error "You must include 3ds Max before including frantic/max3d/max_utility.hpp"
#endif

//#include <inode.h>
//#include <modstack.h>

#include <frantic/channels/property_map.hpp>
#include <frantic/max3d/channels/property_map_interop.hpp>
#include <frantic/max3d/fpwrapper/max_typetraits.hpp>
#include <frantic/max3d/geopipe/get_inodes.hpp>
#include <frantic/max3d/value_ivl_pair.hpp>

// This should never be done, importing a namespace into another is a bad idea.
// using namespace std;

namespace frantic {
namespace max3d {
namespace geopipe {

// A modifier property system defines a pattern between a primary object and a group of modified objects that link
// to the primary. These scripted modifiers must declare two params in their param block definition that match the
// first and second tag of the modifier property system. Tt is these tags that are used by the class to recognize
// the modifiers as belonging to the system.
class modifier_property_system {
  private:
    frantic::tstring m_tagOne;
    frantic::tstring m_tagTwo;
    bool m_convertToMeters;

    modifier_property_system() {}

  public:
    modifier_property_system( const frantic::tstring& firstTag, const frantic::tstring& secondTag,
                              bool convertToMeters ) {
        m_tagOne = firstTag;
        m_tagTwo = secondTag;
        m_convertToMeters = convertToMeters;
    }

    // Returns the tags for the property system
    frantic::tstring get_tag_one() { return m_tagOne; }
    frantic::tstring get_tag_two() { return m_tagTwo; }

    bool get_meters_conversion() const { return m_convertToMeters; }

    void set_meters_conversion( bool convertToMeters ) { m_convertToMeters = convertToMeters; }

    // Provides all the modifier object types that link to the primary node
    void get_object_types( INode* primaryNode, std::set<frantic::tstring>& outObjectTypes ) {
        if( primaryNode == 0 ) {
            throw std::runtime_error( "get_object_types(): param primaryNode is null" );
        }

        // Get all of the objects with scripted flood modifiers on them
        std::vector<std::pair<INode*, Modifier*>> mods;
        get_modifier_inodes( primaryNode, mods );
        max3d::value_ivl_map_t params;
        for( unsigned i = 0; i < mods.size(); ++i ) {
            params.clear();
            max3d::add_pblock2_parameters( params, mods[i].second, 0, m_convertToMeters );
            if( (unsigned)_ttoi( params[m_tagOne].first.c_str() ) == primaryNode->GetHandle() ) {
                outObjectTypes.insert( params[m_tagTwo].first );
            }
        }
    }

    // Provides all the objects with modifiers that match the given object type and link to the primary node
    void get_inodes( INode* primaryNode, const frantic::tstring& objectType, std::vector<INode*>& out_inodes ) {
        if( primaryNode == 0 ) {
            throw std::runtime_error( "get_inodes(): param primaryNode is null" );
        }

        // Get all of the objects with scripted modifiers on them with params that match the modifier tags
        std::vector<std::pair<INode*, Modifier*>> mods;
        max3d::value_ivl_map_t params;
        get_modifier_inodes( primaryNode, mods );

        for( unsigned i = 0; i < mods.size(); ++i ) {
            params.clear();
            max3d::add_pblock2_parameters( params, mods[i].second, 0,
                                           m_convertToMeters ); // get the params of the currend modifier
            if( params[m_tagTwo].first == objectType &&
                (unsigned)_ttoi( params[m_tagOne].first.c_str() ) == primaryNode->GetHandle() )
                out_inodes.push_back( mods[i].first );
            // else {
            //	mprintf( boost::lexical_cast<char*,string()+ "Object " + mods[i].first->GetName() + " was not added"> );
            // }
        }
    }

    /**
     * Provides the properties of the modifier object which links to the primary node and which modifies the provided
     * scene node
     *
     * @return True if the required modifier was found on the object, false if not.
     */
    bool get_inode_mod_properties( INode* refNode, INode* primaryNode, const frantic::tstring& objectType,
                                   TimeValue time, frantic::channels::property_map& outProps ) {
        if( primaryNode == 0 ) {
            throw std::runtime_error( "get_inode_mod_properties(): param primaryNode is null" );
        }
        if( refNode == 0 ) {
            throw std::runtime_error( "get_inode_mod_properties(): param refNode is null" );
        }

        std::vector<Modifier*> mods;
        max3d::get_modifier_stack(
            mods, refNode ); // get the modifier stack, but it returns stack ( incl. disabled modifiers )

        for( size_t i = 0; i < mods.size(); ++i ) {
            frantic::channels::property_map props;
            // we need to get check if the mod is enabled or not
            if( mods[i]->IsEnabled() ) {
                // Since we're actually getting the parameters, pass in the inode handle as its parameter
                max3d::channels::get_object_parameters( mods[i], time, m_convertToMeters, props );
                if( props.has_property( m_tagOne ) && props.has_property( m_tagTwo ) ) {
                    // if the modifier points to the provided Simulation INode than the copy the params into the output
                    // result
                    if( props.get<frantic::tstring>( m_tagTwo ) == objectType &&
                        props.get<frantic::tstring>( m_tagOne ) ==
                            boost::lexical_cast<frantic::tstring>( primaryNode->GetHandle() ) ) {
                        props.swap( outProps );
                        return true;
                    }
                }
            }
        }
        outProps.clear();
        return false;
    }

    // Provides the collection of all <INode,Modifier> pairs that link to the primary node
    void get_modifier_inodes( INode* primaryNode, std::vector<std::pair<INode*, Modifier*>>& outNodeModPairs ) const {
        if( primaryNode == 0 ) {
            throw std::runtime_error( "get_modifier_inodes(): param primaryNode is null" );
        }

        std::vector<std::pair<INode*, Modifier*>> widgetsTemp;
        Interval ivalid = FOREVER;

        // Get all the OS modifiers which have references pointing to the reference object
        max3d::get_referring_osmodifier_inodes( widgetsTemp, primaryNode );

        // Find all of the widgets which are fluid modifiers
        // These are those which have a parameter called "floodObjPick"
        // and a parameter called "floodNodeType"
        max3d::value_ivl_map_t params; // OH THE BADNESS

        for( unsigned i = 0; i < widgetsTemp.size(); ++i ) {
            // TraceLine( string() + "Node: " + widgetsTemp[i].first->GetName() );
            params.clear();
            max3d::add_pblock2_parameters( params, widgetsTemp[i].second, 0, m_convertToMeters );
            if( params.find( m_tagOne ) != params.end() && params.find( m_tagTwo ) != params.end() ) {
                outNodeModPairs.push_back( widgetsTemp[i] );
            }
        }
    }
};
} // namespace geopipe
} // namespace max3d
} // namespace frantic
