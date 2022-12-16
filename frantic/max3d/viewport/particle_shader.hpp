// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic\logging\logging_level.hpp>

#include <boost\filesystem\path.hpp>
#include <frantic\strings\tstring.hpp>
#include <maxapi.h>
#include <maxversion.h>

#if MAX_VERSION_MAJOR >= 17

#include <Graphics\DeviceCaps.h>
#include <Graphics\DrawContext.h>
#include <Graphics\HLSLMaterialHandle.h>
#include <Graphics\IDisplayManager.h>
#include <Graphics\IVirtualDevice.h>
#include <Graphics\MaterialRequiredStreams.h>
#include <Graphics\RenderStates.h>

#define TECH_SMALL_PARTICLE _M( "ParticleSmallShader" )
#define TECH_LARGE_PARTICLE_PERSPECTIVE _M( "ParticleLargePerspectiveShader" )
#define TECH_LARGE_PARTICLE_ORTHOGRAPHIC _M( "ParticleLargeOrthographicShader" )

extern const char* particle_shader_src;

class particle_shader {
  private:
    MaxSDK::Graphics::HLSLMaterialHandle m_shader;
    MaxSDK::Graphics::MaterialRequiredStreams m_format;
    MSTR m_techName;

  public:
    particle_shader( const frantic::tstring& shaderPath ) {
        m_format.Clear();

        MaxSDK::Graphics::DeviceCaps caps;
        MaxSDK::Graphics::GetIDisplayManager()->GetDeviceCaps( caps );
        if( MaxSDK::Graphics::Level5_0 <= caps.FeatureLevel ) {
            m_shader.InitializeWithFile( shaderPath.c_str() );
        }

        MaxSDK::Graphics::MaterialRequiredStreamElement positionChannel;
        positionChannel.SetType( MaxSDK::Graphics::VertexFieldFloat3 );
        positionChannel.SetChannelCategory( MaxSDK::Graphics::MeshChannelPosition );
        positionChannel.SetUsageIndex( 0 );
        positionChannel.SetStreamIndex( 0 );
        m_format.AddStream( positionChannel );

        MaxSDK::Graphics::MaterialRequiredStreamElement colorChannel;
        colorChannel.SetType( MaxSDK::Graphics::VertexFieldFloat4 );
        colorChannel.SetChannelCategory( MaxSDK::Graphics::MeshChannelVertexColor );
        colorChannel.SetUsageIndex( 0 );
        colorChannel.SetStreamIndex( 1 );
        m_format.AddStream( colorChannel );

        m_techName = TECH_LARGE_PARTICLE_PERSPECTIVE;
    }

    ~particle_shader() {}

    /**
     *	activate - activate the shader before drawing
     *	@param drawContext	The draw context to draw to
     */
    void activate( MaxSDK::Graphics::DrawContext& drawContext ) {
        /*MaxSDK::Graphics::IVirtualDevice &vd = */ drawContext.GetVirtualDevice();

        const float viewPortFOV = const_cast<ViewExp*>( drawContext.GetViewExp() )->GetFOV();
        m_shader.SetFloatParameter( _M( "VIEWPORT_FOV" ), viewPortFOV );

        m_shader.SetActiveTechniqueName( m_techName );
        m_shader.Activate( drawContext );
        m_shader.ActivatePass( drawContext, 0 );
    }

    /**
     *	terminate - terminate the shader after drawing
     *	@param drawContext	the draw context that was drawn to
     */
    void terminate( MaxSDK::Graphics::DrawContext& drawContext ) {
        /*MaxSDK::Graphics::IVirtualDevice &vd = */ drawContext.GetVirtualDevice();
        m_shader.PassesFinished( drawContext );
        m_shader.Terminate();
    }

    /**
     *	get_stream_format - get the format that the vertex buffers should be in for the shader
     *	@return	the format
     */
    MaxSDK::Graphics::MaterialRequiredStreams& get_stream_format() { return m_format; }

    /**
     *	GetEffect - get the shader itself
     *	@return the shader
     */
    const MaxSDK::Graphics::HLSLMaterialHandle& GetEffect() { return m_shader; }

    /**
     *	set_tech_name - set wich technique (set of shaders) the effect should use to draw
     *		this will be different for large vs small particles
     *	@param name	 the technique name
     */
    void set_tech_name( MSTR name ) { m_techName = name; }
};

#endif