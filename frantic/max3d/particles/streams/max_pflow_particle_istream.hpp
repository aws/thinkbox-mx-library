// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/channels/channel_map.hpp>
#include <frantic/channels/channel_map_adaptor.hpp>
#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/time.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

#pragma warning( push, 3 )
#include <IParticleObjectExt.h>
#include <ParticleFlow/IChannelContainer.h>
#include <ParticleFlow/IPFSystem.h>
#include <ParticleFlow/IParticleChannels.h>
#include <ParticleFlow/IParticleGroup.h>
#pragma warning( pop )

namespace frantic {
namespace max3d {
namespace particles {
namespace streams {

class max_pflow_particle_istream : public frantic::particles::streams::particle_istream {
  private:
    frantic::tstring m_name;

    frantic::channels::channel_map m_nativeMap;
    frantic::channels::channel_map m_outMap;

    int m_index;
    boost::int64_t m_totalParticles;

    float m_density;
    TimeValue m_currentTime;
    IParticleGroup* m_particleGroup;

    bool m_forceRenderState;

    std::vector<char> m_defaultParticleBuffer;

    typedef frantic::graphics::vector4f vector4f;
    typedef frantic::graphics::vector3f vector3f;
    typedef frantic::graphics::color3f color3f;

    struct {
        IParticleChannelPoint3R* position;
        IParticleChannelPoint3R* velocity;
        IParticleChannelPoint3R* acceleration;
        IParticleChannelPoint3R* scale;
        IParticleChannelQuatR* orientation;
        IParticleChannelAngAxisR* spin;
        IParticleChannelIDR* index;
        IParticleChannelPTVR* birth;
        IParticleChannelPTVR* death;
        IParticleChannelPTVR* lifeSpan;
        IParticleChannelIntR* materialIndex;
        IParticleChannelIntR* mxsInt;
        IParticleChannelFloatR* mxsFloat;
        IParticleChannelPoint3R* mxsVector;
        IParticleChannelMeshMapR* meshMap;
        IParticleChannelMapR* color;
        IParticleChannelMapR* texCoord;
        IParticleChannelMeshR* mesh;
    } m_channels;

    struct channel_info {
        int channelNum;
        IParticleChannelMapR* src;
        frantic::channels::channel_cvt_accessor<vector3f> dest;
        channel_info( int num, IParticleChannelMapR* map,
                      const frantic::channels::channel_cvt_accessor<vector3f>& accessor ) {
            channelNum = num;
            src = map;
            dest = accessor;
        }
    };

    struct {
        bool hasPosition;
        bool hasDensity;
        bool hasColor;
        bool hasTexCoord;
        bool hasVelocity;
        bool hasAcceleration;
        bool hasNormal;
        bool hasTangent;
        bool hasOrientation;
        bool hasSpin;
        bool hasScale;
        bool hasID;
        bool hasMaterialIndex;
        bool hasAge;
        bool hasLifeSpan;
        bool hasMXSInt;
        bool hasMXSFloat;
        bool hasMXSVector;

        frantic::channels::channel_accessor<vector3f> position;
        frantic::channels::channel_cvt_accessor<vector3f> color;
        frantic::channels::channel_cvt_accessor<vector3f> texCoord;
        frantic::channels::channel_cvt_accessor<vector3f> velocity;
        frantic::channels::channel_cvt_accessor<vector3f> acceleration;
        frantic::channels::channel_cvt_accessor<vector3f> normal;
        frantic::channels::channel_cvt_accessor<vector3f> tangent;
        frantic::channels::channel_cvt_accessor<vector4f> orientation;
        frantic::channels::channel_cvt_accessor<vector4f> spin;
        frantic::channels::channel_cvt_accessor<vector3f> scale;
        frantic::channels::channel_cvt_accessor<float> density;
        frantic::channels::channel_cvt_accessor<int> id;
        frantic::channels::channel_cvt_accessor<double> age;
        frantic::channels::channel_cvt_accessor<double> lifeSpan;
        frantic::channels::channel_cvt_accessor<int> materialIndex;
        frantic::channels::channel_cvt_accessor<int> mxsInt;
        frantic::channels::channel_cvt_accessor<float> mxsFloat;
        frantic::channels::channel_cvt_accessor<vector3f> mxsVector;
        std::vector<channel_info> mappings; // Stores accessors for map channels 2 through 99
        frantic::channels::channel_cvt_accessor<float> radius;
        frantic::channels::channel_cvt_accessor<vector3f> radiusXYZ;
    } m_accessors;

    // Store mesh bound box sizes for Radius calculation,
    // indexed by valueIndex.
    // these are the standard shapes like box, 20-sided sphere, etc.
    std::vector<frantic::graphics::vector3f> m_bboxWidths;
    bool m_doneBuildBBoxWidths;
    bool m_doneShowBBoxWarning;

    /**
     * Determine the size of mesh's boundbox.
     *
     * @param mesh the mesh to get the boundbox dimensions from.
     * @return the edge lengths of mesh's bound box.
     */
    static frantic::graphics::vector3f compute_mesh_width( const Mesh* mesh ) {
        // tempted to const_cast and getBoundingBox...
        frantic::graphics::vector3f width( 0 );
        if( mesh ) {
            frantic::graphics::boundbox3f bbox;
            // TODO: handle hidden verts?
            if( mesh->verts ) {
                const int numVerts = mesh->getNumVerts();
                for( int i = 0; i < numVerts; ++i ) {
                    bbox += frantic::max3d::from_max_t( mesh->verts[i] );
                }
            }
            if( bbox.is_empty() ) {
                width = frantic::graphics::vector3f( 0 );
            } else {
                width = frantic::graphics::vector3f( bbox.xsize(), bbox.ysize(), bbox.zsize() );
            }
        } else {
            // TODO: what?
            // 1.f appears to match what getParticleScale returns when the mesh is NULL
            // but the mesh is NULL, no geometry will appear..
            width = frantic::graphics::vector3f( 0.f );
        }
        return width;
    }

    /**
     * Populate m_bboxWidths, which will hold the bound box dimensions
     * for standard shapes such as box and 20-sided sphere.
     *
     * @param meshChannel the particle mesh channel to get mesh sizes from.
     */
    void build_bbox_widths( IParticleChannelMeshR* meshChannel ) {
        m_bboxWidths.clear();
        if( meshChannel ) {
            const int valueCount = meshChannel->GetValueCount();
            m_bboxWidths.resize( std::max( 0, valueCount ) );
            for( int valueIndex = 0; valueIndex < valueCount; ++valueIndex ) {
                const Mesh* m = meshChannel->GetValueByIndex( valueIndex );
                m_bboxWidths[valueIndex] = compute_mesh_width( m );
            }
        }
    }

    /**
     *  Return the size of the mesh's boundbox.
     *
     * @param meshValueIndex the index returned by IParticleChannelMeshR->GetValueIndex()
     *     for the particle.
     * @return the edge lengths of the mesh's boundbox.
     */
    frantic::graphics::vector3f get_mesh_width( const int meshValueIndex ) {
        if( !m_doneBuildBBoxWidths ) {
            build_bbox_widths( m_channels.mesh );
            m_doneBuildBBoxWidths = true;
        }

        if( meshValueIndex >= 0 && meshValueIndex < static_cast<int>( m_bboxWidths.size() ) ) {
            return m_bboxWidths[meshValueIndex];
        } else {
            // This used to be:
            // return get_mesh_width( m_channels.mesh->GetValue( m_index ) );
            // But I changed it to a warning because I can't reproduce the case that requires it.
            if( !m_doneShowBBoxWarning ) {
                m_doneShowBBoxWarning = true;
                FF_LOG( warning ) << "max_pflow_particle_istream() Internal Error: index out of range for radius "
                                     "calculation.  Please contact Thinkbox support.  ("
                                  << boost::lexical_cast<frantic::tstring>( meshValueIndex )
                                  << " is not in the range [0, "
                                  << boost::lexical_cast<frantic::tstring>( m_bboxWidths.size() ) << "))" << std::endl;
            }
            return frantic::graphics::vector3f( 1.f );
        }
    }

  private:
    void init_stream( INode* pNode, TimeValue t ) {
        m_particleGroup = GetParticleGroupInterface( pNode->GetObjectRef() );
        if( !m_particleGroup )
            throw std::runtime_error( "max_pflow_particle_istream() - Could not get the pflow IParticleGroup interface "
                                      "from the supplied node" );

        IPFSystem* particleSystem = PFSystemInterface( m_particleGroup->GetParticleSystem() );
        if( !particleSystem )
            throw std::runtime_error(
                "max_pflow_particle_istream() - Could not get the IPFSystem from the IParticleGroup for node: " +
                std::string( frantic::strings::to_string( pNode->GetName() ) ) );

        IParticleObjectExt* particleSystemParticles = GetParticleObjectExtInterface( particleSystem );
        if( !particleSystemParticles )
            throw std::runtime_error(
                "max_pflow_particle_istream() - Could not get the IParticleObjectExt from the IPFSystem for node: " +
                std::string( frantic::strings::to_string( pNode->GetName() ) ) );

        if( m_forceRenderState )
            particleSystem->SetRenderState( true );

        particleSystemParticles->UpdateParticles( pNode, t );

        m_index = -1;
        m_name = pNode->GetName();
        m_totalParticles = 0;
        m_currentTime = t;
        m_density = 1.f / particleSystem->GetMultiplier( t );

        IObject* particleContainer = m_particleGroup->GetParticleContainer();
        if( !particleContainer ) {
            // Apparently PFlow has started making bunk particle event objects that don't have a particle container.
            // This allows them to silently slip away instead of stopping the render.
            FF_LOG( warning )
                << "max_pflow_particle_istream() - Could not GetParticleContainer() from the IParticleGroup for node: "
                << pNode->GetName() << std::endl;

            // This will leave the native map as empty, which might cause problems elsewhere.
            m_nativeMap.end_channel_definition();

            return;
        }

        // The amount channel is in the particle group it seems.
        IParticleChannelAmountR* amountChannel = GetParticleChannelAmountRInterface( particleContainer );
        if( !amountChannel )
            throw std::runtime_error(
                "max_pflow_particle_istream() - Could not get the pflow IParticleChannelAmountR" );

        m_totalParticles = amountChannel->Count();

        IChannelContainer* channelContainer = GetChannelContainerInterface( particleContainer );
        if( !channelContainer )
            throw std::runtime_error( "max_pflow_particle_istream() - Could not get the pflow IParticleContainer "
                                      "interface from the supplied node" );

        m_channels.position = GetParticleChannelPositionRInterface( channelContainer );
        // if(m_channels.position) m_nativeMap.define_channel<vector3f>(_T("Position"));

        // I was having trouble with an empty channel_map (which causes a divide by 0 when asked for its size) so I
        // forced the Position channel to exist.
        m_nativeMap.define_channel<vector3f>( _T("Position") );

        m_channels.velocity = GetParticleChannelSpeedRInterface( channelContainer );
        if( m_channels.velocity )
            m_nativeMap.define_channel<vector3f>( _T("Velocity") );

        m_channels.acceleration = GetParticleChannelAccelerationRInterface( channelContainer );
        if( m_channels.acceleration )
            m_nativeMap.define_channel<vector3f>( _T("Acceleration") );

        m_channels.orientation = GetParticleChannelOrientationRInterface( channelContainer );
        if( m_channels.orientation ) {
            m_nativeMap.define_channel<vector4f>( _T("Orientation") );
            m_nativeMap.define_channel<vector3f>( _T("Normal") );
            m_nativeMap.define_channel<vector3f>( _T("Tangent") );
        }

        m_channels.spin = GetParticleChannelSpinRInterface( channelContainer );
        if( m_channels.spin )
            m_nativeMap.define_channel<vector4f>( _T("Spin") );

        m_channels.scale = GetParticleChannelScaleRInterface( channelContainer );
        if( m_channels.scale )
            m_nativeMap.define_channel<vector3f>( _T("Scale") );

        m_channels.index = GetParticleChannelIDRInterface( channelContainer );
        if( m_channels.index )
            m_nativeMap.define_channel<int>( _T("ID") );

        m_channels.birth = GetParticleChannelBirthTimeRInterface( channelContainer );
        if( m_channels.birth )
            m_nativeMap.define_channel<double>( _T("Age") );

        m_channels.death = GetParticleChannelDeathTimeRInterface( channelContainer );
        m_channels.lifeSpan = GetParticleChannelLifespanRInterface( channelContainer );
        if( m_channels.lifeSpan || ( m_channels.death && m_channels.birth ) )
            m_nativeMap.define_channel<double>( _T("LifeSpan") );

        m_channels.materialIndex = GetParticleChannelMtlIndexRInterface( channelContainer );
        if( m_channels.materialIndex )
            m_nativeMap.define_channel<int>( _T("MtlIndex") );

        m_channels.mxsInt = GetParticleChannelMXSIntegerRInterface( channelContainer );
        if( m_channels.mxsInt )
            m_nativeMap.define_channel<int>( _T("MXSInteger") );

        m_channels.mxsFloat = GetParticleChannelMXSFloatRInterface( channelContainer );
        if( m_channels.mxsFloat )
            m_nativeMap.define_channel<float>( _T("MXSFloat") );

        m_channels.mxsVector = GetParticleChannelMXSVectorRInterface( channelContainer );
        if( m_channels.mxsVector )
            m_nativeMap.define_channel<vector3f>( _T("MXSVector") );

        m_channels.meshMap = GetParticleChannelShapeTextureRInterface( channelContainer );
        if( m_channels.meshMap ) {
            m_channels.color = m_channels.meshMap->GetMapReadChannel( 0 );
            if( m_channels.color )
                m_nativeMap.define_channel<vector3f>( _T("Color") );

            m_channels.texCoord = m_channels.meshMap->GetMapReadChannel( 1 );
            if( m_channels.texCoord )
                m_nativeMap.define_channel<vector3f>( _T("TextureCoord") );

            for( int i = 2; i < MAX_MESHMAPS; ++i ) {
                if( m_channels.meshMap->MapSupport( i ) )
                    m_nativeMap.define_channel<vector3f>( _T("Mapping") + boost::lexical_cast<frantic::tstring>( i ) );
            }
        } else {
            m_channels.color = NULL;
            m_channels.texCoord = NULL;
        }

        m_channels.mesh = GetParticleChannelShapeRInterface( channelContainer );
        if( m_channels.scale || m_channels.mesh ) {
            m_nativeMap.define_channel<float>( _T("Radius") );
            m_nativeMap.define_channel<vector3f>( _T("RadiusXYZ") );
        }
        m_doneBuildBBoxWidths = false;
        m_doneShowBBoxWarning = false;

        m_nativeMap.end_channel_definition();
    }

    void init_accessors() {
        if( m_accessors.hasPosition = m_outMap.has_channel( _T("Position") ) )
            m_accessors.position = m_outMap.get_accessor<vector3f>( _T("Position") );

        if( m_accessors.hasVelocity = m_outMap.has_channel( _T("Velocity") ) )
            m_accessors.velocity = m_outMap.get_cvt_accessor<vector3f>( _T("Velocity") );

        if( m_accessors.hasAcceleration = m_outMap.has_channel( _T("Acceleration") ) )
            m_accessors.acceleration = m_outMap.get_cvt_accessor<vector3f>( _T("Acceleration") );

        if( m_accessors.hasNormal = m_outMap.has_channel( _T("Normal") ) )
            m_accessors.normal = m_outMap.get_cvt_accessor<vector3f>( _T("Normal") );

        if( m_accessors.hasTangent = m_outMap.has_channel( _T("Tangent") ) )
            m_accessors.tangent = m_outMap.get_cvt_accessor<vector3f>( _T("Tangent") );

        if( m_accessors.hasOrientation = m_outMap.has_channel( _T("Orientation") ) )
            m_accessors.orientation = m_outMap.get_cvt_accessor<vector4f>( _T("Orientation") );

        if( m_accessors.hasSpin = m_outMap.has_channel( _T("Spin") ) )
            m_accessors.spin = m_outMap.get_cvt_accessor<vector4f>( _T("Spin") );

        if( m_accessors.hasScale = m_outMap.has_channel( _T("Scale") ) )
            m_accessors.scale = m_outMap.get_cvt_accessor<vector3f>( _T("Scale") );

        if( m_accessors.hasDensity = m_outMap.has_channel( _T("Density") ) )
            m_accessors.density = m_outMap.get_cvt_accessor<float>( _T("Density") );

        if( m_accessors.hasID = m_outMap.has_channel( _T("ID") ) )
            m_accessors.id = m_outMap.get_cvt_accessor<int>( _T("ID") );

        if( m_accessors.hasMaterialIndex = m_outMap.has_channel( _T("MtlIndex") ) )
            m_accessors.materialIndex = m_outMap.get_cvt_accessor<int>( _T("MtlIndex") );

        if( m_accessors.hasAge = m_outMap.has_channel( _T("Age") ) )
            m_accessors.age = m_outMap.get_cvt_accessor<double>( _T("Age") );

        if( m_accessors.hasLifeSpan = m_outMap.has_channel( _T("LifeSpan") ) )
            m_accessors.lifeSpan = m_outMap.get_cvt_accessor<double>( _T("LifeSpan") );

        if( m_accessors.hasMXSInt = m_outMap.has_channel( _T("MXSInteger") ) )
            m_accessors.mxsInt = m_outMap.get_cvt_accessor<int>( _T("MXSInteger") );

        if( m_accessors.hasMXSFloat = m_outMap.has_channel( _T("MXSFloat") ) )
            m_accessors.mxsFloat = m_outMap.get_cvt_accessor<float>( _T("MXSFloat") );

        if( m_accessors.hasMXSVector = m_outMap.has_channel( _T("MXSVector") ) )
            m_accessors.mxsVector = m_outMap.get_cvt_accessor<vector3f>( _T("MXSVector") );

        if( m_accessors.hasColor = m_outMap.has_channel( _T("Color") ) )
            m_accessors.color = m_outMap.get_cvt_accessor<vector3f>( _T("Color") );

        if( m_accessors.hasTexCoord = m_outMap.has_channel( _T("TextureCoord") ) )
            m_accessors.texCoord = m_outMap.get_cvt_accessor<vector3f>( _T("TextureCoord") );

        if( m_channels.meshMap ) {
            // We potentially have a bunch of map channels
            for( std::size_t i = 0; i < m_outMap.channel_count(); ++i ) {
                const frantic::channels::channel& ch = m_outMap[i];

                // Check the first 7 characters to see if it is a MappingXX channel.
                if( _tcsnccmp( _T("Mapping"), ch.name().c_str(), 7 ) == 0 ) {
                    int channel = boost::lexical_cast<int>( ch.name().substr( 7 ) );
                    if( m_channels.meshMap->MapSupport( channel ) )
                        m_accessors.mappings.push_back(
                            channel_info( channel, m_channels.meshMap->GetMapReadChannel( channel ),
                                          m_outMap.get_cvt_accessor<vector3f>( ch.name() ) ) );
                } // if name[0:7] = "Mapping"
            }     // for i = 0 to m_outMap.size()
        }

        m_accessors.radius.reset();
        if( m_outMap.has_channel( _T("Radius") ) ) {
            m_accessors.radius = m_outMap.get_cvt_accessor<float>( _T("Radius") );
        }

        m_accessors.radiusXYZ.reset();
        if( m_outMap.has_channel( _T("RadiusXYZ") ) ) {
            m_accessors.radiusXYZ = m_outMap.get_cvt_accessor<vector3f>( _T("RadiusXYZ") );
        }
    }

  public:
    max_pflow_particle_istream( INode* pNode, TimeValue t, bool forceRenderState = true )
        : m_forceRenderState( forceRenderState ) {
        init_stream( pNode, t );
        set_channel_map( m_nativeMap );
    }

    max_pflow_particle_istream( INode* pNode, TimeValue t, const frantic::channels::channel_map& pcm,
                                bool forceRenderState = true )
        : m_forceRenderState( forceRenderState ) {
        init_stream( pNode, t );
        set_channel_map( pcm );
    }

    virtual ~max_pflow_particle_istream() { close(); }

    void set_channel_map( const frantic::channels::channel_map& pcm ) {
        std::vector<char> newDefaultParticle( pcm.structure_size() );
        if( newDefaultParticle.size() > 0 ) {
            if( m_defaultParticleBuffer.size() > 0 ) {
                frantic::channels::channel_map_adaptor defaultAdaptor( pcm, m_outMap );
                defaultAdaptor.copy_structure( &newDefaultParticle[0], &m_defaultParticleBuffer[0] );
            } else
                memset( &newDefaultParticle[0], 0, pcm.structure_size() );
        }
        m_defaultParticleBuffer.swap( newDefaultParticle );

        m_outMap = pcm;
        init_accessors();
    }

    void set_default_particle( char* buffer ) {
        memcpy( &m_defaultParticleBuffer[0], buffer, m_outMap.structure_size() );
    }

    void close() {
        if( m_forceRenderState )
            PFSystemInterface( m_particleGroup->GetParticleSystem() )->SetRenderState( false );
    }

    frantic::tstring name() const { return m_name; }
    std::size_t particle_size() const { return m_outMap.structure_size(); }
    boost::int64_t particle_count() const { return m_totalParticles; }
    boost::int64_t particle_index() const { return m_index; }
    boost::int64_t particle_count_left() const { return m_totalParticles - m_index - 1; }
    boost::int64_t particle_progress_count() const { return m_totalParticles; }
    boost::int64_t particle_progress_index() const { return m_index; }

    const frantic::channels::channel_map& get_channel_map() const { return m_outMap; }
    const frantic::channels::channel_map& get_native_channel_map() const { return m_nativeMap; }

    bool get_particle( char* buffer ) {
        using frantic::graphics::vector3f;

        if( ++m_index < m_totalParticles ) {
            memcpy( buffer, &m_defaultParticleBuffer[0], m_outMap.structure_size() ); // TODO: optimize this

            if( m_channels.position && m_accessors.hasPosition )
                m_accessors.position.get( buffer ) =
                    frantic::max3d::from_max_t( m_channels.position->GetValue( m_index ) );

            if( m_channels.velocity && m_accessors.hasVelocity )
                m_accessors.velocity.set(
                    buffer, frantic::max3d::from_max_t( m_channels.velocity->GetValue( m_index ) ) * TIME_TICKSPERSEC );

            if( m_channels.acceleration && m_accessors.hasAcceleration )
                m_accessors.acceleration.set(
                    buffer, frantic::max3d::from_max_t( m_channels.acceleration->GetValue( m_index ) ) *
                                ( TIME_TICKSPERSEC * TIME_TICKSPERSEC ) );

            if( m_channels.scale && m_accessors.hasScale )
                m_accessors.scale.set( buffer, frantic::max3d::from_max_t( m_channels.scale->GetValue( m_index ) ) );

            if( m_channels.materialIndex && m_accessors.hasMaterialIndex )
                m_accessors.materialIndex.set( buffer, m_channels.materialIndex->GetValue( m_index ) );

            if( m_channels.mxsInt && m_accessors.hasMXSInt )
                m_accessors.mxsInt.set( buffer, m_channels.mxsInt->GetValue( m_index ) );

            if( m_channels.mxsFloat && m_accessors.hasMXSFloat )
                m_accessors.mxsFloat.set( buffer, m_channels.mxsFloat->GetValue( m_index ) );

            if( m_channels.mxsVector && m_accessors.hasMXSVector )
                m_accessors.mxsVector.set( buffer,
                                           frantic::max3d::from_max_t( m_channels.mxsVector->GetValue( m_index ) ) );

            if( m_channels.index && m_accessors.hasID )
                m_accessors.id.set( buffer, m_channels.index->GetParticleBorn( m_index ) );

            if( m_accessors.hasDensity )
                m_accessors.density.set( buffer, m_density );

            if( m_channels.birth && m_accessors.hasAge )
                m_accessors.age.set( buffer, frantic::max3d::to_seconds<double>(
                                                 m_currentTime - m_channels.birth->GetTick( m_index ) ) );

            if( m_accessors.hasLifeSpan ) {
                if( m_channels.lifeSpan )
                    m_accessors.lifeSpan.set(
                        buffer, frantic::max3d::to_seconds<double>( m_channels.lifeSpan->GetTick( m_index ) ) );
                else if( m_channels.birth && m_channels.death )
                    m_accessors.lifeSpan.set(
                        buffer, frantic::max3d::to_seconds<double>( m_channels.death->GetTick( m_index ) -
                                                                    m_channels.birth->GetTick( m_index ) ) );
            }

            if( m_channels.spin && m_accessors.hasSpin ) {
                const AngAxis& a( m_channels.spin->GetValue( m_index ) );
                m_accessors.spin.set( buffer, AngAxis( a.axis, a.angle * TIME_TICKSPERSEC ) );
            }

            if( m_channels.orientation ) {
                // vector4f orient = m_channels.orientation->GetValue(m_index);
                const Quat& q = m_channels.orientation->GetValue( m_index );
                Matrix3 m( 1 );
                q.MakeMatrix( m );

                if( m_accessors.hasOrientation )
                    m_accessors.orientation.set(
                        buffer, vector4f( q.x, q.y, q.z, -q.w ) ); // Flip the real part of the quat to handle the
                                                                   // matrix being transposed in our code.
                if( m_accessors.hasNormal )
                    m_accessors.normal.set( buffer, frantic::max3d::from_max_t( m.GetRow( 0 ) ) );
                // m_accessors.normal.set( buffer, orient.quaternion_basis_vector(0) );
                if( m_accessors.hasTangent )
                    m_accessors.tangent.set( buffer, frantic::max3d::from_max_t( m.GetRow( 1 ) ) );
                // m_accessors.tangent.set( buffer, orient.quaternion_basis_vector(1) );
            }

            if( m_channels.color && m_accessors.hasColor ) {
                const TabUVVert* uv = m_channels.color->GetUVVert( m_index );
                if( !uv || uv->Count() == 0 )
                    throw std::runtime_error( "max_pflow_particle_istream::get_particle() - the Vertex Color Channel "
                                              "was present but empty." );

                m_accessors.color.set( buffer, frantic::max3d::from_max_t( ( *uv )[0] ) );
            }

            if( m_channels.texCoord && m_accessors.hasTexCoord ) {
                const TabUVVert* uv = m_channels.texCoord->GetUVVert( m_index );
                if( !uv || uv->Count() == 0 )
                    throw std::runtime_error( "max_pflow_particle_istream::get_particle() - the Texture Coord channel "
                                              "was present but empty." );

                m_accessors.texCoord.set( buffer, frantic::max3d::from_max_t( ( *uv )[0] ) );
            }

            for( std::size_t i = 0; i < m_accessors.mappings.size(); ++i ) {
                const TabUVVert* uv = m_accessors.mappings[i].src->GetUVVert( m_index );
                if( !uv || uv->Count() == 0 )
                    throw std::runtime_error( "max_pflow_particle_istream::get_particle() - the Map Channel[" +
                                              boost::lexical_cast<std::string>( m_accessors.mappings[i].channelNum ) +
                                              "] was present but empty." );

                m_accessors.mappings[i].dest.set( buffer, frantic::max3d::from_max_t( ( *uv )[0] ) );
            }

            if( m_accessors.radius.is_valid() || m_accessors.radiusXYZ.is_valid() ) {
                vector3f scale( 1.f );
                if( m_channels.scale ) {
                    scale = frantic::max3d::from_max_t( m_channels.scale->GetValue( m_index ) );
                }

                vector3f bboxWidth( 1.f );
                if( m_channels.mesh ) {
                    const int valueIndex = m_channels.mesh->GetValueIndex( m_index );
                    bboxWidth = get_mesh_width( valueIndex );
                }

                const vector3f scaledWidth = vector3f::component_multiply( scale, bboxWidth );
                const vector3f radiusXYZ = 0.5f * scaledWidth;

                if( m_accessors.radius.is_valid() ) {
                    m_accessors.radius.set( buffer, radiusXYZ.max_abs_component() );
                }

                if( m_accessors.radiusXYZ.is_valid() ) {
                    m_accessors.radiusXYZ.set( buffer, radiusXYZ );
                }
            }

            return true;
        }

        return false;
    }

    bool get_particles( char* buffer, std::size_t& numParticles ) {
        for( std::size_t i = 0; i < numParticles; ++i ) {
            if( !this->get_particle( buffer ) ) {
                numParticles = i;
                return false;
            }
            buffer += m_outMap.structure_size();
        }

        return true;
    }
};

} // namespace streams
} // namespace particles
} // namespace max3d
} // namespace frantic
