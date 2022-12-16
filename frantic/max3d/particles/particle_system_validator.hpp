// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <vector>

#include <iparamm2.h>

#include <frantic/max3d/particles/particle_stream_factory.hpp>

namespace frantic {
namespace max3d {
namespace particles {

/**
 * A generic PBValidator for particle systems
 * Various PRTObject plugins, Particle Flow operators, etc. store references to particle systems in their parameter
 * blocks. eg. PRTSource stores a reference to the particle system it gets its particles from. The parameters that store
 * these references need a validator on them to control which types of nodes the user is allowed to select. For
 * instance, we don't allow users to select other Particle Flow systems as sources for our Particle Flow operators. This
 * class is a valid PBValidator which can be passed a list of particle_system_type, and told if said list is an allowlist
 * or a denylist.
 */
class list_particle_system_validator : public PBValidator {
  protected:
    const std::vector<particle_system_type> m_systemTypes;
    const bool m_denylist;

  public:
    list_particle_system_validator( std::vector<particle_system_type>& systemTypes, bool denylist = false )
        : m_systemTypes( systemTypes )
        , m_denylist( denylist ) {}

    virtual BOOL Validate( PB2Value& v ) {
        INode* nodeObj = dynamic_cast<INode*>( v.r );
        if( nodeObj ) {
            particle_system_type type = get_particle_system_type( nodeObj, GetCOREInterface()->GetTime() );
            if( type == kParticleTypeInvalid ) {
                return false;
            }

            for( std::vector<particle_system_type>::const_iterator it = m_systemTypes.begin();
                 it != m_systemTypes.end(); ++it ) {
                if( m_denylist ) {
                    if( type == *it ) {
                        return false;
                    }
                } else {
                    if( type == *it ) {
                        return true;
                    }
                }
            }

            if( m_denylist ) {
                return true;
            }
        } else {
            return true;
        }

        return false;
    }
};

/**
 * Like the normal list_particle_system_validator except it takes a template parameter which defines a
 *	static BOOL operator()( PB2Value& ) const
 * which must return true for the validator to validate the PB2Value&
 */
template <typename F>
class list_and_particle_system_validator : public list_particle_system_validator {
  public:
    list_and_particle_system_validator( std::vector<particle_system_type>& systemTypes, bool denylist = false )
        : list_particle_system_validator( systemTypes, denylist ) {}

    virtual BOOL Validate( PB2Value& v ) { return list_particle_system_validator::Validate( v ) && F::Validate( v ); }
};

/**
 * Like the normal list_particle_system_validator except it takes a template parameter which defines a
 *	static BOOL operator()( PB2Value& ) const
 * which either must return true, or the list condition must be satisfied for the validator to validate the PB2Value&
 */
template <typename F>
class list_or_particle_system_validator : public list_particle_system_validator {
  public:
    list_or_particle_system_validator( std::vector<particle_system_type>& systemTypes, bool denylist = false )
        : list_particle_system_validator( systemTypes, denylist ) {}

    virtual BOOL Validate( PB2Value& v ) { return list_particle_system_validator::Validate( v ) || F::Validate( v ); }
};

/**
 * A more efficient validator for cases where we want all the types of particle systems supported by
 * max_particle_istream_factory
 */
class all_particle_system_validator : public PBValidator {
  public:
    virtual BOOL Validate( PB2Value& v ) {
        INode* nodeObj = dynamic_cast<INode*>( v.r );
        if( nodeObj ) {
            particle_system_type type = get_particle_system_type( nodeObj, GetCOREInterface()->GetTime() );
            return type != kParticleTypeInvalid;
        }

        return true;
    }
};

} // namespace particles
} // namespace max3d
} // namespace frantic
