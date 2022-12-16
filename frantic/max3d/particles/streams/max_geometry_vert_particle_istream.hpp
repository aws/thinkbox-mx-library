// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/max3d/convert.hpp>
#include <frantic/max3d/particles/max3d_particle_streams.hpp>

#include <frantic/channels/channel_map_adaptor.hpp>
#include <frantic/particles/streams/particle_istream.hpp>

namespace frantic {
namespace max3d {
namespace particles {
namespace streams {

class max_geometry_vert_particle_istream : public frantic::particles::streams::particle_istream {
  private:
    typedef frantic::graphics::vector3f vector3f;

    PolyObject* m_poNow;
    PolyObject* m_poFuture;

    bool m_bDeletePONow;
    bool m_bDeletePOFuture;

    // A copy of the m_poNow mesh.
    // This is used if retrieving m_poFuture will destroy m_poNow, that is,
    // if m_poNow == os.obj.
    MNMesh m_meshCopyNow;
    bool m_useMeshCopyNow;

    bool m_hasInitializedNow; // set to true after first call to get_particle()
    bool m_hasInitializedFuture;

    INode* m_pNode;
    TimeValue m_time;
    TimeValue m_timeStep;

    // Used to store a mapping from vertex to a face that contains the vertex,
    // and the corresponding corner in that face.
    struct face_and_corner {
        int face;
        int corner;
        face_and_corner()
            : face( -1 )
            , corner( -1 ) {}
        void set( int face, int corner ) {
            this->face = face;
            this->corner = corner;
        }
        bool is_valid() { return face >= 0; }
    };
    std::vector<face_and_corner> m_vertexToFaceAndCorner;

    int m_index;
    boost::int64_t m_livingParticles;
    boost::int64_t m_particlesLeft;
    frantic::tstring m_name;
    float m_interval;

    frantic::channels::channel_map m_nativeMap;
    frantic::channels::channel_map m_outMap;

    std::vector<char> m_defaultParticleBuffer;

    struct {
        bool hasPosition;
        bool hasVelocity;
        bool hasNormal;
        bool hasID;

        frantic::channels::channel_cvt_accessor<vector3f> position;
        frantic::channels::channel_cvt_accessor<vector3f> velocity;
        frantic::channels::channel_cvt_accessor<vector3f> normal;
        frantic::channels::channel_cvt_accessor<int> id;
        std::vector<std::pair<int, frantic::channels::channel_cvt_accessor<vector3f>>> channels;
    } m_accessors;

    void init_accessors( const frantic::channels::channel_map& pcm ) {
        if( m_accessors.hasPosition = pcm.has_channel( _T("Position") ) )
            m_accessors.position = pcm.get_cvt_accessor<vector3f>( _T("Position") );
        if( m_accessors.hasVelocity = pcm.has_channel( _T("Velocity") ) )
            m_accessors.velocity = pcm.get_cvt_accessor<vector3f>( _T("Velocity") );
        if( m_accessors.hasNormal = pcm.has_channel( _T("Normal") ) )
            m_accessors.normal = pcm.get_cvt_accessor<vector3f>( _T("Normal") );
        if( m_accessors.hasID = pcm.has_channel( _T("ID") ) )
            m_accessors.id = pcm.get_cvt_accessor<int>( _T("ID") );

        m_accessors.channels.clear();

        if( pcm.has_channel( _T("Color") ) )
            m_accessors.channels.push_back( std::make_pair( 0, pcm.get_cvt_accessor<vector3f>( _T("Color") ) ) );
        if( pcm.has_channel( _T("TextureCoord") ) )
            m_accessors.channels.push_back( std::make_pair( 1, pcm.get_cvt_accessor<vector3f>( _T("TextureCoord") ) ) );

        for( std::size_t i = 0; i < pcm.channel_count(); ++i ) {
            if( pcm[i].name().substr( 0, 7 ) == _T("Mapping") ) {
                int channel = boost::lexical_cast<int>( pcm[i].name().substr( 7 ) );
                m_accessors.channels.push_back(
                    std::make_pair( channel, pcm.get_cvt_accessor<vector3f>( pcm[i].name() ) ) );
            }
        }
    }

    void init_stream( INode* pNode, TimeValue t, TimeValue timeStep ) {
        m_pNode = pNode;
        m_time = t;
        m_timeStep = timeStep;

        m_poNow = m_poFuture = NULL;
        m_bDeletePONow = m_bDeletePOFuture = false;

        m_useMeshCopyNow = false;

        m_hasInitializedNow = m_hasInitializedFuture = false;

        ObjectState os = pNode->EvalWorldState( t );
        if( !os.obj->CanConvertToType( polyObjectClassID ) )
            throw std::runtime_error( "max_geometry_vert_particle_istream::init_stream() - Cannot convert node: \"" +
                                      std::string( frantic::strings::to_string( pNode->GetName() ) ) +
                                      "\" to a poly object" );

        m_poNow = static_cast<PolyObject*>( os.obj->ConvertToType( t, polyObjectClassID ) );
        m_bDeletePONow = ( m_poNow != os.obj );
        int nLivingVerts = get_living_vert_count( m_poNow->GetMesh() );

        // delete poNow
        // we'll get it again on the first get_particle()
        if( m_bDeletePONow ) {
            m_poNow->MaybeAutoDelete();
            m_poNow = NULL;
            m_bDeletePONow = false;
        }

        m_index = -1;
        m_livingParticles = nLivingVerts;
        m_particlesLeft = nLivingVerts;
        m_name = pNode->GetName();
        m_interval = float( timeStep ) / TIME_TICKSPERSEC;

        m_nativeMap.define_channel<vector3f>( _T("Position") );
        m_nativeMap.define_channel<vector3f>( _T("Velocity") );
        m_nativeMap.define_channel<vector3f>( _T("Normal") );
        m_nativeMap.define_channel<int>( _T("ID") );

        if( os.obj->HasUVW( 0 ) )
            m_nativeMap.define_channel<vector3f>( _T("Color") );
        if( os.obj->HasUVW( 1 ) )
            m_nativeMap.define_channel<vector3f>( _T("TextureCoord") );
        for( int i = 2; i < os.obj->NumMapsUsed(); ++i ) {
            if( os.obj->HasUVW( i ) )
                m_nativeMap.define_channel<vector3f>( _T("Mapping") + boost::lexical_cast<frantic::tstring>( i ) );
        }
        m_nativeMap.end_channel_definition();
    }

    MNMesh& get_now_mesh_ref() {
        if( m_useMeshCopyNow ) {
            return m_meshCopyNow;
        }
        if( m_poNow ) {
            return m_poNow->GetMesh();
        } else {
            throw std::runtime_error( "max_geometry_vert_particle_istream::get_now_mesh_ref() - now mesh is NULL" );
        }
    }

    static bool is_consistent_topology( const MNMesh& a, const MNMesh& b ) {
        if( a.numv != b.numv ) {
            return false;
        }
        if( get_living_vert_count( a ) != get_living_vert_count( b ) ) {
            return false;
        }
        if( a.numf != b.numf ) {
            return false;
        }
        const int numFaces = a.numf;
        for( int faceIndex = 0; faceIndex < numFaces; ++faceIndex ) {
            const MNFace& faceA = a.f[faceIndex];
            const MNFace& faceB = b.f[faceIndex];
            if( faceA.deg != faceB.deg ) {
                return false;
            }
            const int cornerCount = faceA.deg;
            for( int corner = 0; corner < cornerCount; ++corner ) {
                const int vertIndexA = faceA.vtx[corner];
                const int vertIndexB = faceB.vtx[corner];
                const bool deadA = a.v[vertIndexA].GetFlag( MN_DEAD );
                const bool deadB = b.v[vertIndexB].GetFlag( MN_DEAD );
                if( deadA != deadB )
                    return false;
                if( !deadA && vertIndexA != vertIndexB )
                    return false;
            }
        }
        return true;
    }

    static int get_living_vert_count( const MNMesh& mesh ) {
        int vertCount = 0;
        for( int i = 0; i < mesh.numv; ++i ) {
            if( !mesh.v[i].GetFlag( MN_DEAD ) ) {
                ++vertCount;
            }
        }
        return vertCount;
    }

    void fill_vertex_to_face_and_corner_map( const MNMesh& mesh ) {
        m_vertexToFaceAndCorner.clear();
        m_vertexToFaceAndCorner.resize( mesh.numv );

        for( int faceIndex = 0; faceIndex < mesh.numf; ++faceIndex ) {
            const MNFace& face = mesh.f[faceIndex];
            const int cornerCount = face.deg;
            for( int corner = 0; corner < cornerCount; ++corner ) {
                const int vertIndex = face.vtx[corner];
                if( !mesh.v[vertIndex].GetFlag( MN_DEAD ) ) {
                    m_vertexToFaceAndCorner[vertIndex].set( faceIndex, corner );
                }
            }
        }
    }

  public:
    max_geometry_vert_particle_istream( INode* pNode, TimeValue t, TimeValue timeStep ) {
        init_stream( pNode, t, timeStep );
        set_channel_map( m_nativeMap );
    }

    max_geometry_vert_particle_istream( INode* pNode, TimeValue t, TimeValue timeStep,
                                        const frantic::channels::channel_map& pcm ) {
        init_stream( pNode, t, timeStep );
        set_channel_map( pcm );
    }

    ~max_geometry_vert_particle_istream() { close(); }

    void set_channel_map( const frantic::channels::channel_map& pcm ) {
        std::vector<char> newDefaultParticle( pcm.structure_size() );
        if( newDefaultParticle.size() ) {
            if( m_defaultParticleBuffer.size() > 0 ) {
                frantic::channels::channel_map_adaptor defaultAdaptor( pcm, m_outMap );
                defaultAdaptor.copy_structure( &newDefaultParticle[0], &m_defaultParticleBuffer[0] );
            } else
                memset( &newDefaultParticle[0], 0, pcm.structure_size() );
        }
        m_defaultParticleBuffer.swap( newDefaultParticle );

        m_outMap = pcm;
        init_accessors( pcm );
    }

    void set_default_particle( char* buffer ) {
        memcpy( &m_defaultParticleBuffer[0], buffer, m_outMap.structure_size() );
    }

    void close() {
        if( m_bDeletePONow ) {
            m_poNow->MaybeAutoDelete();
            m_poNow = NULL;
            m_bDeletePONow = false;
        }

        if( m_bDeletePOFuture ) {
            m_poFuture->MaybeAutoDelete();
            m_poFuture = NULL;
            m_bDeletePOFuture = false;
        }
    }

    const frantic::channels::channel_map& get_channel_map() const { return m_outMap; }
    const frantic::channels::channel_map& get_native_channel_map() const { return m_nativeMap; }
    frantic::tstring name() const { return m_name; }

    std::size_t particle_size() const { return m_outMap.structure_size(); }
    boost::int64_t particle_count() const { return m_livingParticles; }
    boost::int64_t particle_index() const { return m_index; }
    boost::int64_t particle_count_left() const { return m_particlesLeft; }
    boost::int64_t particle_progress_count() const { return m_livingParticles; }
    boost::int64_t particle_progress_index() const { return m_index; }

    bool get_particle( char* buffer ) {
        // If this is the first call to get_particle, then the meshes need to be re-initialized
        if( !m_hasInitializedNow ) {
            m_hasInitializedNow = true;

            ObjectState os = m_pNode->EvalWorldState( m_time );
            if( !os.obj->CanConvertToType( polyObjectClassID ) )
                throw std::runtime_error(
                    "max_geometry_vert_particle_istream::get_particle() - Cannot convert node: \"" +
                    std::string( frantic::strings::to_string( m_pNode->GetName() ) ) + "\" to a poly object" );
            m_poNow = static_cast<PolyObject*>( os.obj->ConvertToType( m_time, polyObjectClassID ) );
            m_bDeletePONow = ( m_poNow != os.obj );
            const int livingVertCountNow = get_living_vert_count( m_poNow->GetMesh() );
            if( livingVertCountNow != m_livingParticles ) {
                throw std::runtime_error( "max_geometry_vert_particle_istream::get_particle() - mismatch between "
                                          "vertex count in init_stream (" +
                                          boost::lexical_cast<std::string>( m_livingParticles ) +
                                          ") and get_particle (" +
                                          boost::lexical_cast<std::string>( livingVertCountNow ) + ")" );
            }

            fill_vertex_to_face_and_corner_map( m_poNow->GetMesh() );
        }

        assert( m_poNow );

        if( m_accessors.hasVelocity && !m_hasInitializedFuture ) {
            m_hasInitializedFuture = true;

            if( !m_bDeletePONow && !m_useMeshCopyNow ) {
                m_meshCopyNow = m_poNow->GetMesh();
                m_useMeshCopyNow = true;
            }

            ObjectState os = m_pNode->EvalWorldState( m_time + m_timeStep );
            m_poFuture = static_cast<PolyObject*>( os.obj->ConvertToType( m_time + m_timeStep, polyObjectClassID ) );
            m_bDeletePOFuture = ( m_poFuture != os.obj );

            if( !is_consistent_topology( get_now_mesh_ref(), m_poFuture->GetMesh() ) ) {
                // We have no use for the future mesh, so delete it now.
                if( m_bDeletePOFuture ) {
                    m_poFuture->MaybeAutoDelete();
                    m_bDeletePOFuture = false;
                }
                m_poFuture = NULL;
            }
        }

        MNMesh& mesh = get_now_mesh_ref();
        MNMesh* pMesh2 = m_poFuture ? &m_poFuture->GetMesh() : 0;

        const int totalParticles = mesh.numv;

        if( ++m_index >= totalParticles ) {
            return false;
        }

        // skip dead particles
        while( mesh.v[m_index].GetFlag( MN_DEAD ) ) {
            if( ++m_index >= totalParticles ) {
                return false;
            }
        }

        --m_particlesLeft;

        // copy particle
        memcpy( buffer, &m_defaultParticleBuffer[0], m_outMap.structure_size() ); // TODO: optimize

        if( m_accessors.hasPosition )
            m_accessors.position.set( buffer, frantic::max3d::from_max_t( mesh.P( m_index ) ) );
        if( m_accessors.hasVelocity && pMesh2 && !pMesh2->v[m_index].GetFlag( MN_DEAD ) ) {
            const frantic::graphics::vector3f velocity =
                m_interval != 0.0f
                    ? frantic::max3d::from_max_t( ( pMesh2->P( m_index ) - mesh.P( m_index ) ) / m_interval )
                    : frantic::graphics::vector3f( 0.0f, 0.0f, 0.0f );
            m_accessors.velocity.set( buffer, velocity );
        }
        if( m_accessors.hasNormal )
            m_accessors.normal.set( buffer, frantic::max3d::from_max_t( mesh.GetVertexNormal( m_index ) ) );
        if( m_accessors.hasID )
            m_accessors.id.set( buffer, m_index );

        if( m_vertexToFaceAndCorner[m_index].is_valid() ) {
            const int faceIndex = m_vertexToFaceAndCorner[m_index].face;
            const int cornerIndex = m_vertexToFaceAndCorner[m_index].corner;
            for( std::size_t i = 0; i < m_accessors.channels.size(); ++i ) {
                MNMap* pMap = mesh.M( m_accessors.channels[i].first );
                if( !pMap || pMap->VNum() == 0 )
                    continue;

                UVVert uv = pMap->V( pMap->F( faceIndex )->tv[cornerIndex] );
                m_accessors.channels[i].second.set( buffer, frantic::max3d::from_max_t( uv ) );
            }
        }
        return true;
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
