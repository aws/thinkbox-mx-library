// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/channels/channel_map_adaptor.hpp>
#include <frantic/graphics/color3f.hpp>
#include <frantic/graphics/quat4f.hpp>
#include <frantic/graphics/vector3f.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/maxscript/maxscript.hpp>
#include <frantic/max3d/time.hpp>

using namespace frantic::channels;
using namespace frantic::graphics;

// There are various data types not exposed (or just plain screwed up) through IParticleObjectExt. In TP4 and later we
// have access to the TP_MasterSystemInterface which seems like the canonical way to access data, so we use it if its
// available.

#ifdef IID_TP_MASTERSYSTEM
#define TP_GetMasterSystem( particleMat )                                                                              \
    static_cast<TP_MasterSystemInterface*>( particleMat->GetInterface( IID_TP_MASTERSYSTEM ) )
#define TP_GetUniqueID( masterSys, index ) ( masterSys )->UniqueID( index )
#define TP_GetMass( masterSys, index ) ( masterSys )->Mass( index )
#else
typedef void TP_MasterSystemInterface;
#define TP_GetMasterSystem( particleMat ) NULL
#define TP_GetUniqueID( masterSys, index ) ( index )
#define TP_GetMass( masterSys, index ) 0.f
#endif

namespace frantic {
namespace max3d {
namespace particles {

/**
 * TODO: Figure out what changes btw. versions and use a template to select the functionality for each version.
 *       for now it doesn't matter because TP3 and TP4 seem to have the same interface.
 */
class tp_particle_istream_template : public frantic::particles::streams::particle_istream {
    INode* m_pNode;

    ParticleMat* m_pMat;
    PGroup* m_pGroup;
    IParticleObjectExt* m_pParticles;
    TP_MasterSystemInterface* m_pMasterSys;

    frantic::tstring m_name;
    TimeValue m_time;

    int m_currentParticle;
    int m_currentIndex;
    int m_totalIndex;

    std::map<frantic::tstring, int> m_customTPChannels;

    std::vector<std::pair<int, frantic::channels::channel_cvt_accessor<int>>> m_intAccessors;
    std::vector<std::pair<int, frantic::channels::channel_cvt_accessor<float>>> m_floatAccessors;
    std::vector<std::pair<int, frantic::channels::channel_cvt_accessor<color3f>>> m_colorAccessors;
    std::vector<std::pair<int, frantic::channels::channel_cvt_accessor<vector3f>>> m_vectorAccessors;

    channel_map m_outMap;
    channel_map m_nativeMap;
    boost::scoped_array<char> m_defaultParticle;

    frantic::graphics::color3f m_defaultColor;

    struct {
        channel_cvt_accessor<vector4f> orientation;
        channel_cvt_accessor<vector4f> spin;
        channel_cvt_accessor<vector3f> position;
        channel_cvt_accessor<vector3f> velocity;
        channel_cvt_accessor<vector3f> scale;
        channel_cvt_accessor<vector3f> normal;
        channel_cvt_accessor<vector3f> tangent;
        channel_cvt_accessor<color3f> color;
        channel_cvt_accessor<float> density;
        channel_cvt_accessor<float> mass;
        channel_cvt_accessor<float> size;
        channel_cvt_accessor<double> age;
        channel_cvt_accessor<double> lifeSpan;
        channel_cvt_accessor<int> id;
        channel_cvt_accessor<float> radius; // used only if Radius is not a custom data channel
        channel_cvt_accessor<vector3f> radiusXYZ;

    } m_accessors;

    void init_accessors( const channel_map& pcm ) {
        m_accessors.orientation.reset();
        m_accessors.spin.reset();
        m_accessors.position.reset();
        m_accessors.velocity.reset();
        m_accessors.scale.reset();
        m_accessors.normal.reset();
        m_accessors.tangent.reset();
        m_accessors.color.reset();
        m_accessors.density.reset();
        m_accessors.mass.reset();
        m_accessors.size.reset();
        m_accessors.age.reset();
        m_accessors.lifeSpan.reset();
        m_accessors.id.reset();
        m_accessors.radius.reset();
        m_accessors.radiusXYZ.reset();

        m_intAccessors.clear();
        m_floatAccessors.clear();
        m_colorAccessors.clear();
        m_vectorAccessors.clear();

        for( std::size_t i = 0; i < pcm.channel_count(); ++i ) {
            const frantic::channels::channel& ch = pcm[i];

            std::map<frantic::tstring, int>::iterator it = m_customTPChannels.find( ch.name() );
            if( it == m_customTPChannels.end() )
                continue;
            if( it->second >= 0 ) {
                int tpChannelIndex = it->second;
                int type = m_pMat->DataChannelType( m_pGroup, tpChannelIndex );

                if( type == PORT_TYPE_INT )
                    m_intAccessors.push_back(
                        std::make_pair( tpChannelIndex, pcm.get_cvt_accessor<int>( ch.name() ) ) );
                else if( type == PORT_TYPE_FLOAT )
                    m_floatAccessors.push_back(
                        std::make_pair( tpChannelIndex, pcm.get_cvt_accessor<float>( ch.name() ) ) );
                else if( type == PORT_TYPE_COLOR )
                    m_colorAccessors.push_back(
                        std::make_pair( tpChannelIndex, pcm.get_cvt_accessor<color3f>( ch.name() ) ) );
                else if( type == PORT_TYPE_POINT3 )
                    m_vectorAccessors.push_back(
                        std::make_pair( tpChannelIndex, pcm.get_cvt_accessor<vector3f>( ch.name() ) ) );
                else
                    throw std::runtime_error( "The channel \"" + frantic::strings::to_string( ch.name() ) +
                                              "\" has an unsupported type" );
            } else {
                if( ch.name() == _T("Orientation") )
                    m_accessors.orientation = pcm.get_cvt_accessor<vector4f>( _T("Orientation") );
                // else if( ch.name() == _T("Spin") )
                //	m_accessors.spin = pcm.get_cvt_accessor<vector4f>(_T("Spin"));
                else if( ch.name() == _T("Position") )
                    m_accessors.position = pcm.get_cvt_accessor<vector3f>( _T("Position") );
                else if( ch.name() == _T("Velocity") )
                    m_accessors.velocity = pcm.get_cvt_accessor<vector3f>( _T("Velocity") );
                else if( ch.name() == _T("Scale") )
                    m_accessors.scale = pcm.get_cvt_accessor<vector3f>( _T("Scale") );
                else if( ch.name() == _T("Normal") )
                    m_accessors.normal = pcm.get_cvt_accessor<vector3f>( _T("Normal") );
                else if( ch.name() == _T("Tangent") )
                    m_accessors.tangent = pcm.get_cvt_accessor<vector3f>( _T("Tangent") );
                else if( ch.name() == _T("Color") )
                    m_accessors.color = pcm.get_cvt_accessor<color3f>( _T("Color") );
                else if( ch.name() == _T("Mass") )
                    m_accessors.mass = pcm.get_cvt_accessor<float>( _T("Mass") );
                else if( ch.name() == _T("Size") )
                    m_accessors.size = pcm.get_cvt_accessor<float>( _T("Size") );
                else if( ch.name() == _T("Age") )
                    m_accessors.age = pcm.get_cvt_accessor<double>( _T("Age") );
                else if( ch.name() == _T("LifeSpan") )
                    m_accessors.lifeSpan = pcm.get_cvt_accessor<double>( _T("LifeSpan") );
                else if( ch.name() == _T("ID") )
                    m_accessors.id = pcm.get_cvt_accessor<int>( _T("ID") );
                else if( ch.name() == _T("Radius") )
                    m_accessors.radius = pcm.get_cvt_accessor<float>( _T("Radius") );
                else if( ch.name() == _T("RadiusXYZ") )
                    m_accessors.radiusXYZ = pcm.get_cvt_accessor<vector3f>( _T("RadiusXYZ") );
            }
        }
    }

  public:
    tp_particle_istream_template( const channel_map& pcm, INode* pNode, PGroup* group, TimeValue t )
        : m_time( t )
        , m_name( frantic::tstring() + _T("Thinking Particles particle istream for group: ") + group->GetName() ) {
        Object* pObj = pNode->GetObjectRef();
        if( !pObj || !( pObj = pObj->FindBaseObject() ) || pObj->ClassID() != MATTERWAVES_CLASS_ID )
            throw std::runtime_error( std::string() + "thinking_particle_istream() - Node: " +
                                      frantic::strings::to_string( pNode->GetName() ) +
                                      " is not a Thinking Particles object" );

        m_pMat = static_cast<ParticleMat*>( pObj );
        m_pNode = pNode;
        m_pGroup = group;
        m_pMasterSys = NULL; // Don't need this until later.

        m_pParticles = GetParticleObjectExtInterface( m_pMat );
        if( !m_pParticles )
            throw std::runtime_error( std::string() + "thinking_particle_istream() - Node: " +
                                      frantic::strings::to_string( pNode->GetName() ) +
                                      " had a TP object, but did not implement IParticleObjectExt" );

        MaxParticleInterface* pTPParticles = dynamic_cast<MaxParticleInterface*>( m_pParticles );
        if( pTPParticles )
            // Ok to call MaxParticleInterface::SetMaster because it is inline in the header, so we don't link to
            // anything.
            pTPParticles->SetMaster( m_pMat, m_pGroup );

        m_pParticles->UpdateParticles( pNode, t );

        m_currentParticle = -1;
        m_currentIndex = -1;
        m_totalIndex = m_pParticles->NumParticles();

        // This is the wireframe color of the particular group as opposed to pNode->GetWireColor()
        // Ok to call PGroup::GetColor() because it is inline, so no linking happens.

        // BUG: Apparently not! This causes an Access Violation semi-regularly. I drove myself nuts trying to make it
        // work. m_defaultColor = frantic::max3d::from_max_t( m_pGroup->GetColor() );
        m_defaultColor =
            frantic::max3d::from_max_t( m_pGroup->GetParamBlockByID( PB_PGROUP )
                                            ->GetColor( GR_COLOR ) ); // Don't ask why this works better. It just does.

        // Add all the default channels into the map, in order to catch custom channels
        // with conflicting names.
        m_customTPChannels.insert( std::make_pair( _T("Orientation"), -1 ) );
        // m_customTPChannels.insert( std::make_pair(_T("Spin"),-1) );
        m_customTPChannels.insert( std::make_pair( _T("Position"), -1 ) );
        m_customTPChannels.insert( std::make_pair( _T("Velocity"), -1 ) );
        m_customTPChannels.insert( std::make_pair( _T("Scale"), -1 ) );
        m_customTPChannels.insert( std::make_pair( _T("Normal"), -1 ) );
        m_customTPChannels.insert( std::make_pair( _T("Tangent"), -1 ) );
        m_customTPChannels.insert( std::make_pair( _T("Mass"), -1 ) );
        m_customTPChannels.insert( std::make_pair( _T("Size"), -1 ) );
        m_customTPChannels.insert( std::make_pair( _T("Age"), -1 ) );
        m_customTPChannels.insert( std::make_pair( _T("LifeSpan"), -1 ) );
        m_customTPChannels.insert( std::make_pair( _T("ID"), -1 ) );

        m_nativeMap.define_channel<vector4f>( _T("Orientation") );
        // m_nativeMap.define_channel<vector4f>(_T("Spin"));
        m_nativeMap.define_channel<vector3f>( _T("Position") );
        m_nativeMap.define_channel<vector3f>( _T("Velocity") );
        m_nativeMap.define_channel<vector3f>( _T("Scale") );
        m_nativeMap.define_channel<vector3f>( _T("Normal") );
        m_nativeMap.define_channel<vector3f>( _T("Tangent") );
        m_nativeMap.define_channel<float>( _T("Mass") );
        m_nativeMap.define_channel<float>( _T("Size") );
        m_nativeMap.define_channel<double>( _T("Age") );
        m_nativeMap.define_channel<double>( _T("LifeSpan") );
        m_nativeMap.define_channel<int>( _T("ID") );

        // TP supports custom data channels with type Float, Int, Vector, Alignment, and Color.
        // These will be passed through, as long as the names do not interfere with other channels.
        // NOTE: Ok to call ParticleMat::NumDataChannels because it is declared virtual, so we don't link to anything.
        int numDataChannels = m_pMat->NumDataChannels( m_pGroup );
        for( int i = 0; i < numDataChannels; ++i ) {
            // Ok to call ParticleMat::DataChannelName because it is declared virtual, so we don't link to anything.
            frantic::tstring channelName = m_pMat->DataChannelName( m_pGroup, i );
            if( m_customTPChannels.find( channelName ) != m_customTPChannels.end() )
                throw std::runtime_error( "The channel \"" + frantic::strings::to_string( channelName ) +
                                          "\" already exists in TP object " + frantic::strings::to_string( m_name ) );

            // Ok to call ParticleMat::DataChannelType because it is declared virtual, so we don't link to anything.
            int type = m_pMat->DataChannelType( m_pGroup, i );
            if( type == PORT_TYPE_FLOAT )
                m_nativeMap.define_channel<float>( channelName );
            else if( type == PORT_TYPE_INT )
                m_nativeMap.define_channel<int>( channelName );
            else if( type == PORT_TYPE_COLOR || type == PORT_TYPE_POINT3 )
                m_nativeMap.define_channel<vector3f>( channelName );
            else {
                FF_LOG( warning ) << "The channel \"" << channelName << "\" in object: " << m_name
                                  << " has an unsupported type" << std::endl;
                continue;
            }
            m_customTPChannels.insert( std::make_pair( channelName, i ) );
        }

        if( m_customTPChannels.find( _T("Color") ) == m_customTPChannels.end() ) {
            m_customTPChannels.insert( std::make_pair( _T("Color"), -1 ) );
            m_nativeMap.define_channel<color3f>( _T("Color") );
        }

        if( m_customTPChannels.find( _T("Radius") ) == m_customTPChannels.end() ) {
            m_customTPChannels.insert( std::make_pair( _T("Radius"), -1 ) );
            m_nativeMap.define_channel<float>( _T("Radius") );
        }

        if( m_customTPChannels.find( _T("RadiusXYZ") ) == m_customTPChannels.end() ) {
            m_customTPChannels.insert( std::make_pair( _T("RadiusXYZ"), -1 ) );
            m_nativeMap.define_channel<vector3f>( _T("RadiusXYZ") );
        }

        m_nativeMap.end_channel_definition();

        set_channel_map( pcm );
    }

    virtual ~tp_particle_istream_template() { close(); }
    void close() {}

    void set_channel_map( const channel_map& pcm ) {
        boost::scoped_array<char> newDefaultParticle( new char[pcm.structure_size()] );

        // Initialize the new particle.
        pcm.construct_structure( newDefaultParticle.get() );

        // If there already was a default particle, adapt it to the new channels.
        if( m_defaultParticle ) {
            frantic::channels::channel_map_adaptor defaultAdaptor( pcm, m_outMap );
            defaultAdaptor.copy_structure( newDefaultParticle.get(), m_defaultParticle.get() );
        }

        m_defaultParticle.swap( newDefaultParticle );

        m_outMap = pcm;
        init_accessors( m_outMap );
    }

    const channel_map& get_channel_map() const { return m_outMap; }
    const channel_map& get_native_channel_map() const { return m_nativeMap; }

    virtual frantic::tstring name() const { return m_name; }

    virtual std::size_t particle_size() const { return m_outMap.structure_size(); }

    virtual boost::int64_t particle_index() const { return m_currentParticle; }
    virtual boost::int64_t particle_count() const { return -1; }
    virtual boost::int64_t particle_count_left() const { return -1; }
    virtual boost::int64_t particle_progress_count() const { return m_totalIndex; }
    virtual boost::int64_t particle_progress_index() const { return m_currentIndex; }

    virtual void set_default_particle( char* rawParticleBuffer ) {
        m_outMap.copy_structure( m_defaultParticle.get(), rawParticleBuffer );
    }

    virtual bool get_particle( char* rawParticleBuffer ) {
        //"Re-open" the stream if this is the first particle
        if( m_currentIndex < 0 ) {
            m_pParticles = GetParticleObjectExtInterface( m_pMat );

            MaxParticleInterface* pTPParticles = dynamic_cast<MaxParticleInterface*>( m_pParticles );
            if( pTPParticles )
                pTPParticles->SetMaster( m_pMat, m_pGroup );

            m_pParticles->UpdateParticles( m_pNode, m_time );
            m_totalIndex = m_pParticles->NumParticles();

            m_pMasterSys = TP_GetMasterSystem( m_pMat );
        }

        if( ++m_currentIndex >= m_totalIndex )
            return false;

        // Some particles might be "dead". These are flagged with negative age. They should be skipped.
        while( m_pParticles->GetParticleAgeByIndex( m_currentIndex ) < 0 ) {
            if( ++m_currentIndex >= m_totalIndex )
                return false;
        }

        ++m_currentParticle;

        m_outMap.copy_structure( rawParticleBuffer, m_defaultParticle.get() );

        if( m_accessors.position.is_valid() )
            m_accessors.position.set(
                rawParticleBuffer,
                frantic::max3d::from_max_t( *m_pParticles->GetParticlePositionByIndex( m_currentIndex ) ) );

        // Docs say this is in units/frame but it is actually in units/ticks.
        if( m_accessors.velocity.is_valid() )
            m_accessors.velocity.set( rawParticleBuffer, frantic::max3d::from_max_t(
                                                             *m_pParticles->GetParticleSpeedByIndex( m_currentIndex ) *
                                                             static_cast<float>( TIME_TICKSPERSEC ) ) );

        if( m_accessors.scale.is_valid() )
            m_accessors.scale.set(
                rawParticleBuffer,
                frantic::max3d::from_max_t( *m_pParticles->GetParticleScaleXYZByIndex( m_currentIndex ) ) );

        if( m_accessors.age.is_valid() )
            m_accessors.age.set( rawParticleBuffer, frantic::max3d::to_seconds<double>(
                                                        m_pParticles->GetParticleAgeByIndex( m_currentIndex ) ) );

        if( m_accessors.lifeSpan.is_valid() )
            m_accessors.lifeSpan.set(
                rawParticleBuffer,
                frantic::max3d::to_seconds<double>( m_pParticles->GetParticleLifeSpanByIndex( m_currentIndex ) ) );

        if( m_accessors.orientation.is_valid() ) {
            Matrix3& m = *m_pParticles->GetParticleTMByIndex( m_currentIndex );

            // Normalize each row since the size/scale is multiplied in. TODO: I don't think it can be sheared, but we
            // may need to do a decomposition to accurately get the rotation out of this.
            frantic::graphics::quat4f q = frantic::graphics::quat4f::from_coord_sys(
                frantic::graphics::vector3f::normalize( frantic::max3d::from_max_t( m.GetRow( 0 ) ) ),
                frantic::graphics::vector3f::normalize( frantic::max3d::from_max_t( m.GetRow( 1 ) ) ),
                frantic::graphics::vector3f::normalize( frantic::max3d::from_max_t( m.GetRow( 2 ) ) ) );

            m_accessors.orientation.set( rawParticleBuffer, vector4f( q.x, q.y, q.z, q.w ) );
        }

        if( m_accessors.normal.is_valid() || m_accessors.tangent.is_valid() ) {
            Matrix3& m = *m_pParticles->GetParticleTMByIndex( m_currentIndex );

            if( m_accessors.normal.is_valid() )
                m_accessors.normal.set( rawParticleBuffer, frantic::graphics::vector3f::normalize(
                                                               frantic::max3d::from_max_t( m.GetRow( 0 ) ) ) );
            if( m_accessors.tangent.is_valid() )
                m_accessors.tangent.set( rawParticleBuffer, frantic::graphics::vector3f::normalize(
                                                                frantic::max3d::from_max_t( m.GetRow( 1 ) ) ) );
        }

        if( m_accessors.color.is_valid() )
            m_accessors.color.set( rawParticleBuffer, m_defaultColor );

        if( m_accessors.size.is_valid() )
            m_accessors.size.set( rawParticleBuffer, m_pParticles->GetParticleScaleByIndex( m_currentIndex ) );

        union {
            float f[4];
            int i[1];
        } tempData;

        // Ok to access PGroup::pids because it is a public data member, so we don't link to anything.
        int realIndex = m_pGroup->pids[m_currentIndex];

        if( m_accessors.id.is_valid() )
            m_accessors.id.set( rawParticleBuffer, TP_GetUniqueID( m_pMasterSys, realIndex ) );

        if( m_accessors.radius.is_valid() || m_accessors.radiusXYZ.is_valid() ) {
            const float scale = m_pParticles->GetParticleScaleByIndex( m_currentIndex );

            const Point3* pScaleXYZ = m_pParticles->GetParticleScaleXYZByIndex( m_currentIndex );
            const vector3f scaleXYZ = pScaleXYZ ? frantic::max3d::from_max_t( *pScaleXYZ ) : vector3f( 1 );

            const vector3f radiusXYZ = 0.5f * scale * scaleXYZ;

            if( m_accessors.radius.is_valid() ) {
                m_accessors.radius.set( rawParticleBuffer, radiusXYZ.max_abs_component() );
            }

            if( m_accessors.radiusXYZ.is_valid() ) {
                m_accessors.radiusXYZ.set( rawParticleBuffer, radiusXYZ );
            }
        }

        if( m_accessors.mass.is_valid() )
            m_accessors.mass.set( rawParticleBuffer, TP_GetMass( m_pMasterSys, realIndex ) );

        { // Copy the custom integer channels
            std::vector<std::pair<int, frantic::channels::channel_cvt_accessor<int>>>::iterator it, itEnd;
            for( it = m_intAccessors.begin(), itEnd = m_intAccessors.end(); it != itEnd; ++it ) {
                // Ok to call ParticleMat::GetValue because it is declared virtual, so we don't link to anything.
                BOOL result = m_pMat->GetValue( realIndex, it->first, tempData.i, PORT_TYPE_INT );
                if( !result )
                    throw std::runtime_error( "Failed to read from integer TP channel " +
                                              boost::lexical_cast<std::string>( it->first ) + " in TP object " +
                                              frantic::strings::to_string( m_name ) );
                it->second.set( rawParticleBuffer, tempData.i[0] );
            }
        }

        { // Copy the custom float channels
            std::vector<std::pair<int, frantic::channels::channel_cvt_accessor<float>>>::iterator it, itEnd;
            for( it = m_floatAccessors.begin(), itEnd = m_floatAccessors.end(); it != itEnd; ++it ) {
                // Ok to call ParticleMat::GetValue because it is declared virtual, so we don't link to anything.
                BOOL result = m_pMat->GetValue( realIndex, it->first, tempData.f, PORT_TYPE_FLOAT );
                if( !result )
                    throw std::runtime_error( "Failed to read from float TP channel " +
                                              boost::lexical_cast<std::string>( it->first ) + " in TP object " +
                                              frantic::strings::to_string( m_name ) );
                it->second.set( rawParticleBuffer, tempData.f[0] );
            }
        }

        { // Copy the custom Point3 channels
            std::vector<std::pair<int, frantic::channels::channel_cvt_accessor<vector3f>>>::iterator it, itEnd;
            for( it = m_vectorAccessors.begin(), itEnd = m_vectorAccessors.end(); it != itEnd; ++it ) {
                // Ok to call ParticleMat::GetValue because it is declared virtual, so we don't link to anything.
                BOOL result = m_pMat->GetValue( realIndex, it->first, tempData.f, PORT_TYPE_POINT3 );
                if( !result )
                    throw std::runtime_error( "Failed to read from Point3 TP channel " +
                                              boost::lexical_cast<std::string>( it->first ) + " in TP object " +
                                              frantic::strings::to_string( m_name ) );
                it->second.set( rawParticleBuffer, vector3f( tempData.f[0], tempData.f[1], tempData.f[2] ) );
            }
        }

        { // Copy the custom Color channels
            std::vector<std::pair<int, frantic::channels::channel_cvt_accessor<color3f>>>::iterator it, itEnd;
            for( it = m_colorAccessors.begin(), itEnd = m_colorAccessors.end(); it != itEnd; ++it ) {
                // Ok to call ParticleMat::GetValue because it is declared virtual, so we don't link to anything.
                BOOL result = m_pMat->GetValue( realIndex, it->first, tempData.f, PORT_TYPE_COLOR );
                if( !result )
                    throw std::runtime_error( "Failed to read from Point3 TP channel " +
                                              boost::lexical_cast<std::string>( it->first ) + " in TP object " +
                                              frantic::strings::to_string( m_name ) );
                it->second.set( rawParticleBuffer, color3f( tempData.f[0], tempData.f[1], tempData.f[2] ) );
            }
        }

        return true;
    }

    bool get_particles( char* buffer, std::size_t& numParticles ) {
        for( std::size_t i = 0; i < numParticles; ++i, buffer += m_outMap.structure_size() ) {
            if( !get_particle( buffer ) ) {
                numParticles = i;
                return false;
            }
        }

        return true;
    }
};

} // namespace particles
} // namespace max3d
} // namespace frantic
