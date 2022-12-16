// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <maxversion.h>

#if MAX_VERSION_MAJOR >= 17

#define INIT_FRAME_STAMP -1
#define COLOR_ARGB( a, r, g, b )                                                                                       \
    ( (DWORD)( ( ( (a)&0xff ) << 24 ) | ( ( (r)&0xff ) << 16 ) | ( ( (g)&0xff ) << 8 ) | ( (b)&0xff ) ) )

#include <boost/optional.hpp>

#include <Graphics\BaseMaterialHandle.h>
#include <Graphics\CustomRenderItemHandle.h>
#include <Graphics\IVirtualDevice.h>
#include <Graphics\IndexBufferHandle.h>
#include <Graphics\MaterialRequiredStreams.h>
#include <Graphics\SolidColorMaterialHandle.h>
#include <Graphics\VertexBufferHandle.h>
#include <Graphics\VertexColorMaterialHandle.h>

#include <Graphics/IDisplayManager.h>
#include <Graphics\HLSLMaterialHandle.h>

#include <frantic\misc\hybrid_assert.hpp>
#include <frantic\particles\particle_array.hpp>
#include <frantic\particles\particle_array_iterator.hpp>

#include "particle_shader.hpp"

#define RED_OFFSET 0
#define GREEN_OFFSET 1
#define BLUE_OFFSET 2
#define ALPHA_OFFSET 3

#include <vector>

namespace frantic {
namespace max3d {
namespace viewport {

enum render_type { RT_POINT, RT_POINT_LARGE, RT_VELOCITY, RT_NORMAL, RT_TANGENT };

// since particle_t is a protected member of PRTObject, which is, at the time of writing, closed to modification,
// when using a vector of them to store particle data, it must be passed in through a template
// No template parameter is required when using a particle_array though, this must be passed in instead to make the
// compiler happy
struct fake_particle {
    Point3 m_position;
    Point3 m_vector;
    Color m_color;
};

template <typename Particle = fake_particle>
class ParticleRenderItem : public MaxSDK::Graphics::ICustomRenderItem {
  public:
    ParticleRenderItem( const frantic::tstring& shaderFilePath )
        : m_valid( false )
        , m_nPrimitives( 0 )
        , m_realized( false )
        , m_hasPrecomputedVelocityOffset( false )
        , m_inWorldSpace( false )
        , m_finalTM( MaxSDK::Graphics::Matrix44().MakeIdentity() )
        , m_renderType( RT_POINT_LARGE )
        , m_pointSize( boost::none )
        , m_skipInverseTransform( false )
        , m_particleShaderSmall( shaderFilePath )
        , m_particleShaderLargeOrtho( shaderFilePath )
        , m_particleShaderLargePerspec( shaderFilePath ) {}

    /**
     *	Initialize - Initialize the object using a vector of custom particle structs
     *	@param particles The actual particle data
     *	@param renderType The type of data to be rendered (position, velocity, normal, etc.)
     */
    void Initialize( const std::vector<Particle>& particles, render_type renderType ) {
        init_shaders();

        m_realized = false;

        m_nPrimitives = particles.size();

        if( m_nPrimitives > 0 ) {

            std::size_t bufferSize = m_nPrimitives;
            if( renderType != RT_POINT && renderType != RT_POINT_LARGE ) {
                bufferSize *= 2;
            }

            MaxSDK::Graphics::VertexBufferHandle posBuffer;
            MaxSDK::Graphics::VertexBufferHandle colorBuffer;
            MaxSDK::Graphics::VertexBufferHandle normalBuffer;

            init_buffers( posBuffer, colorBuffer, normalBuffer, bufferSize );

            float* pPosBuffer = (float*)posBuffer.Lock( 0, 0 );     // bufferSize );
            float* pColorBuffer = (float*)colorBuffer.Lock( 0, 0 ); // bufferSize );

            try {
                for( std::vector<Particle>::const_iterator it = particles.begin(); it != particles.end(); ++it ) {
                    Point3 pos = ( *it ).m_position;
                    pPosBuffer[0] = pos.x;
                    pPosBuffer[1] = pos.y;
                    pPosBuffer[2] = pos.z;
                    pPosBuffer += 3;

                    int colorIterations = 1;

                    if( renderType == RT_VELOCITY ) {
                        Point3 velocity = ( *it ).m_vector;
                        if( m_hasPrecomputedVelocityOffset ) {
                            pPosBuffer[0] = velocity.x;
                            pPosBuffer[1] = velocity.y;
                            pPosBuffer[2] = velocity.z;
                        } else {
                            pPosBuffer[0] = pos.x + velocity.x;
                            pPosBuffer[1] = pos.y + velocity.y;
                            pPosBuffer[2] = pos.z + velocity.z;
                        }
                        pPosBuffer += 3;

                        colorIterations = 2;
                    }

                    for( int i = 0; i < colorIterations; ++i ) {
                        Color color = ( *it ).m_color;
                        pColorBuffer[ALPHA_OFFSET] = 1.0f;
                        pColorBuffer[RED_OFFSET] = color.r;
                        pColorBuffer[GREEN_OFFSET] = color.g;
                        pColorBuffer[BLUE_OFFSET] = color.b;
                        pColorBuffer += 4;
                    }
                }
            } catch( ... ) {
                posBuffer.Unlock();
                colorBuffer.Unlock();
            }

            posBuffer.Unlock();
            colorBuffer.Unlock();

            m_buffers.append( posBuffer );
            m_buffers.append( colorBuffer );
            if( MaxSDK::Graphics::Level5_0 > m_caps.FeatureLevel ) {
                m_buffers.append( normalBuffer );
            }
        }
        m_renderType = renderType;

        m_valid = true;
    }

    /**
     *	Initialize - Initialize the object using a particle array
     *	@param primType The type of max primitive to draw to the display (eg. point vs line)
     *	@param particles The particle array
     *	@param renderType The type of data to be rendered (position, velocity, normal, etc.)
     */
    void Initialize( const frantic::particles::particle_array& particles, render_type renderType, float normalScale ) {

        m_realized = false;

        m_channelMap = particles.get_channel_map();

        m_posAccessor = m_channelMap.get_accessor<frantic::graphics::vector3f>( _T("Position") );

        if( m_channelMap.has_channel( _T("Velocity") ) ) {
            m_hasVelocityData = true;
            m_velocityAccessor = m_channelMap.get_cvt_accessor<frantic::graphics::vector3f>( _T("Velocity") );
        } else {
            m_hasVelocityData = false;
        }

        if( m_channelMap.has_channel( _T("Normal") ) ) {
            m_hasNormalData = true;
            m_normalAccessor = m_channelMap.get_cvt_accessor<frantic::graphics::vector3f>( _T("Normal") );
        } else {
            m_hasNormalData = false;
        }

        if( m_channelMap.has_channel( _T("Tangent") ) ) {
            m_hasTangentData = true;
            m_tangentAccessor = m_channelMap.get_cvt_accessor<frantic::graphics::vector3f>( _T("Tangent") );
        } else {
            m_hasTangentData = false;
        }

        if( m_channelMap.has_channel( _T("Color") ) ) {
            m_hasColorData = true;
            m_colorAccessor = m_channelMap.get_cvt_accessor<frantic::graphics::color3f>( _T("Color") );
        } else {
            m_hasColorData = false;
        }

        init_shaders();

        m_nPrimitives = particles.size();

        if( m_nPrimitives > 0 ) {

            std::size_t bufferSize = m_nPrimitives;
            if( renderType != RT_POINT && renderType != RT_POINT_LARGE ) {
                bufferSize *= 2;
            }

            MaxSDK::Graphics::VertexBufferHandle posBuffer;
            MaxSDK::Graphics::VertexBufferHandle colorBuffer;
            MaxSDK::Graphics::VertexBufferHandle normalBuffer;

            std::size_t posOffset;
            std::size_t colorOffset;

            init_buffers( posBuffer, colorBuffer, normalBuffer, bufferSize );

            float* pPosBuffer = (float*)posBuffer.Lock( 0, m_nPrimitives );
            float* pColorBuffer = (float*)colorBuffer.Lock( 0, m_nPrimitives );

            posOffset = 3;
            colorOffset = 4;

            try {
                for( frantic::particles::particle_array::const_iterator it = particles.begin(); it != particles.end();
                     ++it ) {
                    frantic::graphics::vector3f pos = m_posAccessor.get( *it );
                    pPosBuffer[0] = pos.x;
                    pPosBuffer[1] = pos.y;
                    pPosBuffer[2] = pos.z;
                    pPosBuffer += posOffset;

                    int colorIterations = 1;

                    if( renderType == RT_VELOCITY ) {
                        frantic::graphics::vector3f velocity;
                        if( m_hasVelocityData ) {
                            velocity = m_velocityAccessor.get( *it ) + pos;
                        } else {
                            velocity = pos;
                        }

                        pPosBuffer[0] = velocity.x;
                        pPosBuffer[1] = velocity.y;
                        pPosBuffer[2] = velocity.z;

                        pPosBuffer += posOffset;
                        colorIterations = 2;

                    } else if( renderType == RT_NORMAL ) {
                        frantic::graphics::vector3f normal;
                        if( m_hasNormalData ) {
                            normal = normalScale * m_normalAccessor.get( *it ) + pos;
                        } else {
                            normal = pos;
                        }

                        pPosBuffer[0] = normal.x;
                        pPosBuffer[1] = normal.y;
                        pPosBuffer[2] = normal.z;

                        pPosBuffer += posOffset;
                        colorIterations = 2;

                    } else if( renderType == RT_TANGENT ) {
                        frantic::graphics::vector3f tangent;
                        if( m_hasTangentData ) {
                            tangent = normalScale * m_tangentAccessor.get( *it ) + pos;
                        } else {
                            tangent = pos;
                        }

                        pPosBuffer[0] = tangent.x;
                        pPosBuffer[1] = tangent.y;
                        pPosBuffer[2] = tangent.z;

                        pPosBuffer += posOffset;
                        colorIterations = 2;
                    }

                    for( int i = 0; i < colorIterations; ++i ) {
                        pColorBuffer[ALPHA_OFFSET] = 1.0f;

                        if( m_hasColorData ) {
                            frantic::graphics::color3f color = m_colorAccessor.get( *it );
                            pColorBuffer[RED_OFFSET] = color.r;
                            pColorBuffer[GREEN_OFFSET] = color.g;
                            pColorBuffer[BLUE_OFFSET] = color.b;
                        } else {
                            pColorBuffer[RED_OFFSET] = 1.0f;
                            pColorBuffer[GREEN_OFFSET] = 1.0f;
                            pColorBuffer[BLUE_OFFSET] = 1.0f;
                        }

                        pColorBuffer += colorOffset;
                    }
                }
            } catch( ... ) {
                posBuffer.Unlock();
                colorBuffer.Unlock();
            }

            posBuffer.Unlock();
            colorBuffer.Unlock();

            m_buffers.append( posBuffer );
            m_buffers.append( colorBuffer );

            if( MaxSDK::Graphics::Level5_0 > m_caps.FeatureLevel ) {
                m_buffers.append( normalBuffer );
            }
        }
        m_renderType = renderType;
        m_valid = true;
    }

    /**
     *	SetMessage - sets a string message to be redered alongside the particles (eg. to display the particle count)
     *	@param location the location to render the string at
     *	@param msg the string to render
     */
    void SetMessage( const Point3& location, const M_STD_STRING& msg ) {
        m_msgLocation = location;
        m_msg = msg;
    }

    // From ICustomRenderItem

    /**
     *	Realize
     *	@param drawContext the current DrawContext
     */
    void Realize( MaxSDK::Graphics::DrawContext& drawContext ) {
        MaxSDK::Graphics::IVirtualDevice& dev = drawContext.GetVirtualDevice();
        if( !dev.IsValid() )
            return;

        if( !m_valid && m_callback )
            m_callback( drawContext.GetTime(), *this );

        if( m_inWorldSpace ) {
            m_finalTM = MaxSDK::Graphics::Matrix44().MakeIdentity();
        } else if( m_valid && m_nPrimitives > 0 ) {
            // Capture the node's object to world transformation
            INode* pNode = drawContext.GetCurrentNode();
            if( pNode ) {
                MaxSDK::Graphics::Matrix44 worldTM;
                MaxSDK::Graphics::MaxWorldMatrixToMatrix44( worldTM, pNode->GetNodeTM( drawContext.GetTime() ) );
                // SetWorldMatrix get's applied on top of the initial node TM causing the node TM to be applied twice
                // so we must apply the inverse of the init node TM to get it to display properly which we get the first
                // time realize is called
                if( !m_realized ) {
                    Matrix3 invertedNodeTM = pNode->GetNodeTM( drawContext.GetTime() );
                    invertedNodeTM.Invert();
                    MaxSDK::Graphics::MaxWorldMatrixToMatrix44( m_initTM, invertedNodeTM );
                    m_realized = true;
                }
                if( m_skipInverseTransform ) {
                    m_finalTM = worldTM;
                } else {
                    MaxSDK::Graphics::Matrix44::Multiply( m_finalTM, m_initTM, worldTM );
                }
            } else {
                m_finalTM = MaxSDK::Graphics::Matrix44().MakeIdentity();
            }
        }

        drawContext.SetWorldMatrix( m_finalTM );
    }

    /**
     *	Display - actually do the rendering
     *	@param drawContext the current DrawContext
     */

    void Display( MaxSDK::Graphics::DrawContext& drawContext ) {

        MaxSDK::Graphics::IVirtualDevice& dev = drawContext.GetVirtualDevice();
        if( !dev.IsValid() )
            return;

        drawContext.SetWorldMatrix( m_finalTM );

        if( m_valid && m_nPrimitives > 0 ) {
            MaxSDK::Graphics::RasterizerState state = dev.GetRasterizerState();
            if( m_renderType != RT_POINT_LARGE ) {
                state.SetPointSize( 1.0f );
            } else {
                state.SetPointSize( m_pointSize.get_value_or( 4.0f ) );
            }
            dev.SetRasterizerState( state );

            dev.SetVertexStreams( m_buffers );
            dev.SetStreamFormat( m_streamDesc );
#if MAX_VERSION_MAJOR < 19
            dev.SetIndexBuffer( nullptr );
#else
            dev.SetIndexBuffer( MaxSDK::Graphics::EmptyIndexBufferHandle );
#endif

            if( MaxSDK::Graphics::Level5_0 > m_caps.FeatureLevel ) {
                m_legacyShader.Activate( drawContext );
                unsigned passCount = m_legacyShader.GetPassCount( drawContext );
                for( unsigned pass = 0; pass < passCount; ++pass ) {
                    m_legacyShader.ActivatePass( drawContext, 0 );
                    if( m_renderType == RT_POINT || m_renderType == RT_POINT_LARGE ) {
                        dev.Draw( MaxSDK::Graphics::PrimitivePointList, 0, static_cast<int>( m_nPrimitives ) );
                    } else {
                        dev.Draw( MaxSDK::Graphics::PrimitiveLineList, 0, static_cast<int>( m_nPrimitives ) );
                    }
                }
                m_legacyShader.PassesFinished( drawContext );
                m_legacyShader.Terminate();
            } else {
                particle_shader* curParticleShader = &m_particleShaderSmall;
                if( m_renderType == RT_POINT_LARGE ) {
                    curParticleShader = &m_particleShaderLargeOrtho;
                    if( const ViewExp* view = drawContext.GetViewExp() ) {
                        if( const_cast<ViewExp*>( view )->IsPerspView() ) {
                            curParticleShader = &m_particleShaderLargePerspec;
                        }
                    }
                }
                curParticleShader->activate( drawContext );
                if( m_renderType == RT_POINT || m_renderType == RT_POINT_LARGE ) {
                    dev.Draw( MaxSDK::Graphics::PrimitivePointList, 0, static_cast<int>( m_nPrimitives ) );
                } else {
                    dev.Draw( MaxSDK::Graphics::PrimitiveLineList, 0, static_cast<int>( m_nPrimitives ) );
                }
                curParticleShader->terminate( drawContext );
            }
        }

        if( !m_msg.empty() && !drawContext.IsHitTest() ) {
            if( const ViewExp* view = drawContext.GetViewExp() ) {
                if( GraphicsWindow* gw = const_cast<ViewExp*>( view )->getGW() ) {
                    gw->setColor( TEXT_COLOR, Point3( 1, 1, 1 ) );
                    gw->text( &m_msgLocation, m_msg.c_str() );
                }
            }
        }
    }

    /**
     *	GetPrimitiveCount - get the current number of primitive objects (eg. points, lines) to draw
     *	@return said number
     */
    std::size_t GetPrimitiveCount() const { return m_nPrimitives; }

    void SetInWorldSpace( bool inWorldSpace ) { m_inWorldSpace = inWorldSpace; }

    void SetPointSize( float size ) { m_pointSize = size; }

    void SetPrecomputedVelocityOffset( bool precomputed ) { m_hasPrecomputedVelocityOffset = precomputed; }

    void SetSkipInverseTransform( bool skip ) { m_skipInverseTransform = skip; }

    template <class FnType>
    inline void SetCallback( FnType&& fn ) {
        m_callback = std::forward<FnType>( fn );
    }

  private:
    /**
     *	init_shaders - helper method for initialize
     */
    void init_shaders() {
        m_buffers.removeAll();
        m_streamDesc.Clear();
        m_valid = false;

        m_streamDesc = m_particleShaderSmall.get_stream_format();

        MaxSDK::Graphics::GetIDisplayManager()->GetDeviceCaps( m_caps );
        if( MaxSDK::Graphics::Level5_0 > m_caps.FeatureLevel ) {
            // DX9 logic
            m_legacyShader.Release();
            m_legacyShader.Initialize();

        } else {
            // DX11 logic
            m_particleShaderSmall.set_tech_name( TECH_SMALL_PARTICLE );
            m_particleShaderLargePerspec.set_tech_name( TECH_LARGE_PARTICLE_PERSPECTIVE );
            m_particleShaderLargeOrtho.set_tech_name( TECH_LARGE_PARTICLE_ORTHOGRAPHIC );
        }
    }

    /**
     *	init_buffers - helper method for initialize
     *	@param posBuffer the position buffer
     *	@param colorBuffer the color buffer
     *	@param normalBuffer the dummy normal buffer
     *	@param bufferSize the number of vertices the buffers should hold
     */
    void init_buffers( MaxSDK::Graphics::VertexBufferHandle& posBuffer,
                       MaxSDK::Graphics::VertexBufferHandle& colorBuffer,
                       MaxSDK::Graphics::VertexBufferHandle& normalBuffer, std::size_t bufferSize ) {

        std::size_t stride = MaxSDK::Graphics::GetVertexStride( MaxSDK::Graphics::VertexFieldFloat3 );
        std::size_t colorStride = MaxSDK::Graphics::GetVertexStride( MaxSDK::Graphics::VertexFieldFloat4 );

#if MAX_VERSION_MAJOR < 19
        posBuffer.Initialize( stride );
        colorBuffer.Initialize( colorStride );
        posBuffer.SetBufferUsageType( MaxSDK::Graphics::BufferUsageStatic );
        colorBuffer.SetBufferUsageType( MaxSDK::Graphics::BufferUsageStatic );
#elif MAX_VERSION_MAJOR < 24
        // posBuffer.Release();
        // colorBuffer.Release();
        posBuffer.Initialize( stride, bufferSize );
        colorBuffer.Initialize( colorStride, bufferSize );

        posBuffer.SetBufferUsageType( MaxSDK::Graphics::BufferUsageStatic );
        colorBuffer.SetBufferUsageType( MaxSDK::Graphics::BufferUsageStatic );
#else
        posBuffer.Initialize( stride, bufferSize, (void*)nullptr, MaxSDK::Graphics::BufferUsageStatic );
        colorBuffer.Initialize( colorStride, bufferSize, (void*)nullptr, MaxSDK::Graphics::BufferUsageStatic );
#endif

#if MAX_VERSION_MAJOR < 19
        posBuffer.SetCapacity( bufferSize );
        posBuffer.SetNumberOfVertices( bufferSize );
        colorBuffer.SetCapacity( bufferSize );
        colorBuffer.SetNumberOfVertices( bufferSize );
#endif

        if( MaxSDK::Graphics::Level5_0 > m_caps.FeatureLevel ) {
#if MAX_VERSION_MAJOR < 19
            normalBuffer.Initialize( stride );
            normalBuffer.SetBufferUsageType( MaxSDK::Graphics::BufferUsageStatic );
#elif MAX_VERSION_MAJOR < 24
            normalBuffer.Initialize( stride, bufferSize );
            normalBuffer.SetBufferUsageType( MaxSDK::Graphics::BufferUsageStatic );
#else
            normalBuffer.Initialize( stride, bufferSize, (void*)nullptr, MaxSDK::Graphics::BufferUsageStatic );
#endif

#if MAX_VERSION_MAJOR < 19
            normalBuffer.SetCapacity( bufferSize );
            normalBuffer.SetNumberOfVertices( bufferSize );
#endif

            MaxSDK::Graphics::MaterialRequiredStreamElement normalChannel;
            normalChannel.SetType( MaxSDK::Graphics::VertexFieldFloat3 );
            normalChannel.SetChannelCategory( MaxSDK::Graphics::MeshChannelVertexNormal );
            normalChannel.SetUsageIndex( 0 );
            normalChannel.SetStreamIndex( 2 );
            m_streamDesc.AddStream( normalChannel );
        }
    }

  private:
    bool m_valid;
    bool m_realized;
    MaxSDK::Graphics::Matrix44 m_finalTM, m_initTM;

    render_type m_renderType;

    std::size_t m_nPrimitives;

    MaxSDK::Graphics::DeviceCaps m_caps;

    // Custom shaders for DX10/11
    particle_shader m_particleShaderSmall;
    particle_shader m_particleShaderLargePerspec;
    particle_shader m_particleShaderLargeOrtho;

    // Generic Max shader for DX9
    MaxSDK::Graphics::VertexColorMaterialHandle m_legacyShader;

    // Array of vertex buffers
    MaxSDK::Graphics::VertexBufferHandleArray m_buffers;

    // Stream buffer format
    MaxSDK::Graphics::MaterialRequiredStreams m_streamDesc;

    Point3 m_msgLocation;
    M_STD_STRING m_msg;

    frantic::channels::channel_map m_channelMap;
    frantic::channels::channel_accessor<frantic::graphics::vector3f> m_posAccessor;
    frantic::channels::channel_cvt_accessor<frantic::graphics::vector3f> m_velocityAccessor;
    frantic::channels::channel_cvt_accessor<frantic::graphics::vector3f> m_normalAccessor;
    frantic::channels::channel_cvt_accessor<frantic::graphics::vector3f> m_tangentAccessor;
    frantic::channels::channel_cvt_accessor<frantic::graphics::color3f> m_colorAccessor;

    bool m_hasVelocityData;
    bool m_hasNormalData;
    bool m_hasTangentData;
    bool m_hasColorData;

    bool m_inWorldSpace;
    bool m_hasPrecomputedVelocityOffset;
    bool m_skipInverseTransform;

    boost::optional<float> m_pointSize;
    std::function<void( TimeValue, frantic::max3d::viewport::ParticleRenderItem<Particle>& )> m_callback;
};

} // namespace viewport
} // namespace max3d
} // namespace frantic

#endif
