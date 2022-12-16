// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "stdafx.h"

#include <frantic/max3d/particles/IMaxKrakatoaPRTObject.hpp>
#include <frantic/max3d/particles/streams/seconds_to_ticks_particle_istream.hpp>
#include <frantic/max3d/particles/streams/ticks_to_seconds_particle_istream.hpp>
#include <frantic/max3d/shaders/map_query.hpp>

#include <frantic/logging/progress_logger.hpp>
#include <frantic/particles/streams/empty_particle_istream.hpp>

#pragma warning( push, 3 )
#include <maxversion.h>
#pragma warning( pop )

#if MAX_VERSION_MAJOR >= 14

#pragma warning( push, 3 )
#include <maxscript/mxsplugin/mxsPlugin.h>
#pragma warning( pop )

#include <memory>

inline ReferenceTarget* get_msplugin_delegate( MSPlugin* pMSPlugin ) { return pMSPlugin->get_delegate(); }

#else
// This file (via #include "IDX9PixelShader.h") has an unnecessary use of #include <d3dx9.h> which is just terrible.
// My work around is to use SubAnim( 0 ) instead, which seems to work. See MSObjectXtnd in msplugin.h to confirm
// SubAnim(0) is the delegate.
//#include <maxscrpt/msplugin.h>
#endif

#include <boost/make_shared.hpp>

using frantic::channels::channel_map;
using frantic::graphics::camera;

namespace frantic {
namespace max3d {
namespace particles {

/**
 * This class provides the context needed by IMaxKrakatoaPRTObject_Legacy2 objects to fully utilize all shading features
 * of 3dsMax while evaluating their particles. At the very least, make sure get_render_context().time is correct.
 */
class IMaxKrakatoaPRTObject_Legacy2::IEvalContext_Legacy2 {
  public:
    virtual ~IEvalContext_Legacy2() {}

    /**
     * Returns the camera that view-dependent particles should use. Many 3dsMax TexMaps require the camera properties to
     * correct assign a color to a particle. Also make sure that the camera related properties in RenderGlobalContext
     * match this object or underfined behaviour will occur.
     * @return A reference to the camera to be used by view-dependent particle sources.
     */
    virtual const frantic::graphics::camera<float>& get_camera() const = 0;

    /**
     * Returns the channel_map that the particle_istream created by IMaxKrakatoaPRTObject::GetParticleStream() should
     * use.
     * @return A reference to the expected channel_map.
     */
    virtual const frantic::channels::channel_map& get_channel_map() const = 0;

    /**
     * Returns a RenderGlobalContext which contains rendering information about the context. The "time" member is VERY
     * IMPORTANT. The various camera related members should match the camera from get_camera().
     * @note This is non-const due to Max's poor const handling.
     * @return The stored RenderGlobalContext.
     */
    virtual RenderGlobalContext& get_max_context() = 0;

    /**
     * Returns the duration and bias of the evaluation's motion blur.
     * @return A pair, where first is the motion blur duration in frames, and second is the motion blur bias [-1,1]
     * relative to get_render_context().time where -1 means the blur interval ends on the frame, 1 means it starts on
     * the frame, and 0 means it is centered.
     */
    virtual std::pair<float, float> get_motion_blur_params() const = 0;

    /**
     * Return a progress_logger object to be used periodically updating the progress while an object is being evaluated.
     * @return A refernce to a progress_logger.
     */
    virtual frantic::logging::progress_logger& get_progress_logger() = 0;

    /**
     * A helper function for accessing the evaluation time.
     */
    inline TimeValue get_time() const { return const_cast<IEvalContext_Legacy2*>( this )->get_max_context().time; }
};

#pragma warning( push )
#pragma warning( disable : 4996 )

class IMaxKrakatoaPRTObjectWrapperBase : public IMaxKrakatoaPRTObject {
  public:
    IMaxKrakatoaPRTObjectWrapperBase();

    virtual LifetimeType LifetimeControl();

    virtual BaseInterface* AcquireInterface();

    virtual void ReleaseInterface();

    virtual void DeleteInterface();

  private:
    volatile LONG m_refCount;
};

IMaxKrakatoaPRTObjectWrapperBase::IMaxKrakatoaPRTObjectWrapperBase()
    : m_refCount( 0 ) {}

BaseInterface::LifetimeType IMaxKrakatoaPRTObjectWrapperBase::LifetimeControl() { return BaseInterface::wantsRelease; }

BaseInterface* IMaxKrakatoaPRTObjectWrapperBase::AcquireInterface() {
    InterlockedIncrement( &m_refCount );
    return this;
}

void IMaxKrakatoaPRTObjectWrapperBase::ReleaseInterface() {
    if( InterlockedDecrement( &m_refCount ) == 0 )
        delete this;
}

void IMaxKrakatoaPRTObjectWrapperBase::DeleteInterface() { delete this; }

/**
 * This class exists to wrap objects that only support IMaxKrakatoaPRTObject_Legacy1 so that they appear to support
 * the IMaxKrakatoaPRTObject interface. This is only expected to be needed when working with out-of-date versions of
 * Krakatoa.
 */
class IMaxKrakatoaPRTObjectWrapper1 : public IMaxKrakatoaPRTObjectWrapperBase {
    IMaxKrakatoaPRTObject_Legacy1* m_wrappedInterface;

  private:
    friend IMaxKrakatoaPRTObjectPtr GetIMaxKrakatoaPRTObject( ReferenceMaker* pObj );

    IMaxKrakatoaPRTObjectWrapper1( IMaxKrakatoaPRTObject_Legacy1* wrappedInterface )
        : m_wrappedInterface( wrappedInterface ) {}

  public:
    virtual boost::shared_ptr<frantic::particles::streams::particle_istream>
    GetRenderStream( const frantic::channels::channel_map& pcm,
                     const frantic::max3d::shaders::renderInformation& renderInfo, INode* pNode, TimeValue t,
                     TimeValue timeStep ) {
        particle_istream_ptr pResult = m_wrappedInterface->GetRenderStream( pcm, renderInfo, pNode, t, timeStep );

        if( !pResult )
            pResult.reset( new frantic::particles::streams::empty_particle_istream( pcm ) );

        return pResult;
    }

    virtual particle_istream_ptr GetParticleStream( IEvalContext_Legacy2* globContext, INode* pNode ) {
        frantic::max3d::shaders::renderInformation renderInfo;
        renderInfo.m_camera = globContext->get_camera();
        renderInfo.m_cameraPosition = frantic::max3d::to_max_t( renderInfo.m_camera.camera_position() );

        particle_istream_ptr pResult = m_wrappedInterface->GetRenderStream( globContext->get_channel_map(), renderInfo,
                                                                            pNode, globContext->get_time(), 20 );

        if( !pResult )
            pResult.reset( new frantic::particles::streams::empty_particle_istream( globContext->get_channel_map() ) );

        return pResult;
    }

    virtual particle_istream_ptr CreateStream( INode* pNode, Interval& outValidity,
                                               boost::shared_ptr<IMaxKrakatoaPRTEvalContext> pEvalContext ) {
        frantic::max3d::shaders::renderInformation renderInfo;
        renderInfo.m_camera = pEvalContext->GetCamera();
        renderInfo.m_cameraPosition = frantic::max3d::to_max_t( renderInfo.m_camera.camera_position() );

        // The client (ie. caller) is expecting time channels in seconds, but the implementation is probably going to
        // supply ticks so we need to adjust the requested channels here
        frantic::channels::channel_map modifiedChannels;
        frantic::max3d::particles::streams::convert_time_channels_to_ticks( pEvalContext->GetDefaultChannels(),
                                                                            modifiedChannels );

        TimeValue t = pEvalContext->GetTime();

        outValidity.SetInstant( t );

        particle_istream_ptr pResult =
            m_wrappedInterface->GetRenderStream( modifiedChannels, renderInfo, pNode, t, 20 );

        if( !pResult )
            pResult.reset(
                new frantic::particles::streams::empty_particle_istream( pEvalContext->GetDefaultChannels() ) );

        pResult = frantic::max3d::particles::streams::convert_time_channels_to_seconds( pResult );

        return pResult;
    }

    virtual void GetStreamNativeChannels( INode* pNode, TimeValue t, frantic::channels::channel_map& outChannelMap ) {
        frantic::max3d::shaders::renderInformation renderInfo;

        frantic::channels::channel_map requestMap;
        requestMap.define_channel( _T("Position"), 3, frantic::channels::data_type_float32 );
        requestMap.end_channel_definition();

        particle_istream_ptr pStream = m_wrappedInterface->GetRenderStream( requestMap, renderInfo, pNode, t, 20 );
        if( pStream != NULL ) {
            pStream = frantic::max3d::particles::streams::convert_time_channels_to_seconds( pStream );

            outChannelMap = pStream->get_native_channel_map();
        }
    }
};

namespace {
/**
 * This class manages the lifetime of an IEvalContext_Legacy2 object and ties it to the lifetime of a particle_istream.
 */
class ContextHolderParticleIStream : public frantic::particles::streams::delegated_particle_istream {
    boost::scoped_ptr<IMaxKrakatoaPRTObject_Legacy2::IEvalContext_Legacy2> m_evalContext;

  public:
    ContextHolderParticleIStream( IMaxKrakatoaPRTObject_Legacy2::IEvalContext_Legacy2* evalContext,
                                  boost::shared_ptr<frantic::particles::streams::particle_istream> delegateStream )
        : delegated_particle_istream( delegateStream )
        , m_evalContext( evalContext ) {}

    virtual bool get_particle( char* rawParticleBuffer ) { return m_delegate->get_particle( rawParticleBuffer ); }
    virtual bool get_particles( char* buffer, std::size_t& numParticles ) {
        return m_delegate->get_particles( buffer, numParticles );
    }
};
} // namespace

/**
 * This class exists to wrap objects that only support IMaxKrakatoaPRTObject_Legacy1 so that they appear to support
 * the IMaxKrakatoaPRTObject interface. This is only expected to be needed when working with out-of-date versions of
 * Krakatoa.
 */
class IMaxKrakatoaPRTObjectWrapper2 : public IMaxKrakatoaPRTObjectWrapperBase {
    IMaxKrakatoaPRTObject_Legacy2* m_wrappedInterface;

  private:
    friend IMaxKrakatoaPRTObjectPtr GetIMaxKrakatoaPRTObject( ReferenceMaker* pObj );

    IMaxKrakatoaPRTObjectWrapper2( IMaxKrakatoaPRTObject_Legacy2* wrappedInterface )
        : m_wrappedInterface( wrappedInterface ) {}

  public:
    virtual boost::shared_ptr<frantic::particles::streams::particle_istream>
    GetRenderStream( const frantic::channels::channel_map& pcm,
                     const frantic::max3d::shaders::renderInformation& renderInfo, INode* pNode, TimeValue t,
                     TimeValue /*timeStep*/ ) {
        std::unique_ptr<IEvalContext_Legacy2> tempContext(
            IMaxKrakatoaPRTObject_Legacy2::CreateDefaultEvalContext( pcm, renderInfo.m_camera, t ) );

        particle_istream_ptr result = m_wrappedInterface->GetParticleStream( tempContext.get(), pNode );

        if( !result )
            result.reset( new frantic::particles::streams::empty_particle_istream( pcm ) );

        // We need to tie the lifetime of the IEvalContext to the lifetime of the particle stream, so we do it here with
        // this particle_istream subclass.
        result.reset( new ContextHolderParticleIStream( tempContext.release(), result ) );

        return result;
    }

    virtual particle_istream_ptr GetParticleStream( IEvalContext_Legacy2* globContext, INode* pNode ) {
        particle_istream_ptr result = m_wrappedInterface->GetParticleStream( globContext, pNode );

        if( !result )
            result.reset( new frantic::particles::streams::empty_particle_istream( globContext->get_channel_map() ) );

        return result;
    }

    virtual particle_istream_ptr CreateStream( INode* pNode, Interval& outValidity,
                                               boost::shared_ptr<IMaxKrakatoaPRTEvalContext> pEvalContext ) {
        class ContextWrapper : public IEvalContext_Legacy2 {
            boost::shared_ptr<IMaxKrakatoaPRTEvalContext> m_pEvalContext;

            frantic::channels::channel_map m_defaultChannels;

          public:
            ContextWrapper( boost::shared_ptr<IMaxKrakatoaPRTEvalContext> pEvalContext )
                : m_pEvalContext( pEvalContext ) {
                // The client (ie. caller) is expecting time channels in seconds, but the implementation is probably
                // going to supply ticks so we need to adjust the requested channels here
                frantic::max3d::particles::streams::convert_time_channels_to_ticks( pEvalContext->GetDefaultChannels(),
                                                                                    m_defaultChannels );
            }

            virtual const frantic::graphics::camera<float>& get_camera() const { return m_pEvalContext->GetCamera(); }

            virtual const frantic::channels::channel_map& get_channel_map() const {
                return m_pEvalContext->GetDefaultChannels();
            }

            virtual RenderGlobalContext& get_max_context() { return m_pEvalContext->GetRenderGlobalContext(); }

            virtual std::pair<float, float> get_motion_blur_params() const { return std::make_pair( 0.f, 0.f ); }

            virtual frantic::logging::progress_logger& get_progress_logger() {
                return m_pEvalContext->GetProgressLogger();
            }
        };

        std::unique_ptr<IEvalContext_Legacy2> tempContext( new ContextWrapper( pEvalContext ) );

        particle_istream_ptr result = m_wrappedInterface->GetParticleStream( tempContext.get(), pNode );

        if( !result )
            result.reset(
                new frantic::particles::streams::empty_particle_istream( pEvalContext->GetDefaultChannels() ) );

        result = frantic::max3d::particles::streams::convert_time_channels_to_seconds( result );

        // We need to tie the lifetime of the IEvalContext to the lifetime of the particle stream, so we do it here with
        // this particle_istream subclass.
        result.reset( new ContextHolderParticleIStream( tempContext.release(), result ) );

        outValidity.SetInstant( pEvalContext->GetTime() );

        return result;
    }

    virtual void GetStreamNativeChannels( INode* pNode, TimeValue t, frantic::channels::channel_map& outChannelMap ) {
        frantic::channels::channel_map requestMap;
        requestMap.define_channel( _T("Position"), 3, frantic::channels::data_type_float32 );
        requestMap.end_channel_definition();

        frantic::graphics::camera<float> defaultCamera;

        std::unique_ptr<IEvalContext_Legacy2> tempContext(
            IMaxKrakatoaPRTObject_Legacy2::CreateDefaultEvalContext( requestMap, defaultCamera, t ) );

        particle_istream_ptr pStream = m_wrappedInterface->GetParticleStream( tempContext.get(), pNode );
        if( pStream != NULL )
            outChannelMap = pStream->get_native_channel_map();
    }
};
#pragma warning( pop )

#pragma warning( push )
#pragma warning( disable : 4706 )

IMaxKrakatoaPRTObjectPtr GetIMaxKrakatoaPRTObject( ReferenceMaker* pObj ) {
    if( !pObj )
        return IMaxKrakatoaPRTObjectPtr();

    // try to get the prt interface from this object directly
    BaseInterface* pInterface = pObj->GetInterface( MAXKRAKATOAPRTOBJECT_INTERFACE );

    // try to get the legacy2 interface if we are interfacing with an old version of Krakatoa
    if( !pInterface && ( pInterface = pObj->GetInterface( MAXKRAKATOAPRTOBJECT_LEGACY2_INTERFACE ) ) )
        pInterface = new IMaxKrakatoaPRTObjectWrapper2( (IMaxKrakatoaPRTObject_Legacy2*)pInterface );

    // try to get the legacy1 interface if we are interfacing with an old version of Krakatoa
    if( !pInterface && ( pInterface = pObj->GetInterface( MAXKRAKATOAPRTOBJECT_LEGACY1_INTERFACE ) ) )
        pInterface = new IMaxKrakatoaPRTObjectWrapper1( (IMaxKrakatoaPRTObject_Legacy1*)pInterface );

        // If that failed, try to see if the base object is a scripted plugin, and check it's delegate for the interface
        // as well.

#if MAX_VERSION_MAJOR >= 14
    MSPlugin* pMXSPlugin;

    if( !pInterface && ( pMXSPlugin = static_cast<MSPlugin*>( pObj->GetInterface( I_MAXSCRIPTPLUGIN ) ) ) ) {
        Animatable* pDelegate = pMXSPlugin->get_delegate();
#else
    if( !pInterface && pObj->GetInterface( I_MAXSCRIPTPLUGIN ) != NULL ) {
        Animatable* pDelegate = pObj->NumSubs() > 0 ? pObj->SubAnim( 0 ) : NULL;
#endif
        if( pDelegate ) {
            pInterface = pDelegate->GetInterface( MAXKRAKATOAPRTOBJECT_INTERFACE );

            // try to get the legacy2 interface if we are interfacing with an old version of Krakatoa
            if( !pInterface && ( pInterface = pDelegate->GetInterface( MAXKRAKATOAPRTOBJECT_LEGACY2_INTERFACE ) ) )
                pInterface = new IMaxKrakatoaPRTObjectWrapper2( (IMaxKrakatoaPRTObject_Legacy2*)pInterface );

            // try to get the legacy interface if we are interfacing with an old version of Krakatoa
            if( !pInterface && ( pInterface = pDelegate->GetInterface( MAXKRAKATOAPRTOBJECT_LEGACY1_INTERFACE ) ) )
                pInterface = new IMaxKrakatoaPRTObjectWrapper1( (IMaxKrakatoaPRTObject_Legacy1*)pInterface );
        }
    }

    return IMaxKrakatoaPRTObjectPtr( (IMaxKrakatoaPRTObject*)pInterface );
}

namespace {
class DefaultEvalContext : public IMaxKrakatoaPRTObject_Legacy2::IEvalContext_Legacy2 {
    frantic::graphics::camera<float> m_cameraImpl;
    frantic::channels::channel_map m_channelMap;

    boost::shared_ptr<frantic::logging::progress_logger> m_progressLogger;

    RenderGlobalContext m_globContext;

  public:
    DefaultEvalContext( const channel_map& pcm, const camera<float>& renderCam, TimeValue t,
                        boost::shared_ptr<frantic::logging::progress_logger> progressLogger )
        : m_cameraImpl( renderCam )
        , m_channelMap( pcm )
        , m_progressLogger( progressLogger ) {
        m_globContext.renderer = NULL;
        m_globContext.projType = ( renderCam.projection_mode() == frantic::graphics::projection_mode::orthographic )
                                     ? PROJ_PARALLEL
                                     : PROJ_PERSPECTIVE;
        m_globContext.devWidth = renderCam.get_output_size().xsize;
        m_globContext.devHeight = renderCam.get_output_size().ysize;
        m_globContext.xscale = m_globContext.yscale = 1.f;
        m_globContext.xc = m_globContext.yc = 0.f;
        m_globContext.antialias = FALSE;
        m_globContext.camToWorld = frantic::max3d::to_max_t( renderCam.world_transform() );
        m_globContext.worldToCam = frantic::max3d::to_max_t( renderCam.world_transform_inverse() );
        m_globContext.nearRange = renderCam.near_distance();
        m_globContext.farRange = renderCam.far_distance();
        m_globContext.devAspect = renderCam.pixel_aspect();
        m_globContext.frameDur = 1.f;
        m_globContext.envMap = NULL;
        m_globContext.globalLightLevel.White();
        m_globContext.atmos = NULL;
        m_globContext.pToneOp = NULL;
        m_globContext.time = t;
        m_globContext.wireMode = FALSE;
        m_globContext.wire_thick = 1.f;
        m_globContext.force2Side = FALSE;
        m_globContext.inMtlEdit = FALSE;
        m_globContext.fieldRender = FALSE;
        m_globContext.first_field = FALSE;
        m_globContext.field_order = FALSE;
        m_globContext.objMotBlur = FALSE;
        m_globContext.nBlurFrames = FALSE;

        if( !progressLogger )
            m_progressLogger.reset( new frantic::logging::null_progress_logger );
    }

    virtual ~DefaultEvalContext() {}

    // From IEvalContext
    virtual const frantic::graphics::camera<float>& get_camera() const { return m_cameraImpl; }
    virtual const frantic::channels::channel_map& get_channel_map() const { return m_channelMap; }
    virtual frantic::logging::progress_logger& get_progress_logger() { return *m_progressLogger; }
    virtual RenderGlobalContext& get_max_context() { return m_globContext; }
    virtual std::pair<float, float> get_motion_blur_params() const { return std::pair<float, float>( 0.5f, 0.f ); }
};
} // namespace

IMaxKrakatoaPRTObject_Legacy2::IEvalContext_Legacy2* IMaxKrakatoaPRTObject_Legacy2::CreateDefaultEvalContext(
    const channel_map& pcm, const camera<float>& camera, TimeValue t,
    boost::shared_ptr<frantic::logging::progress_logger> progress ) {
    return new DefaultEvalContext( pcm, camera, t, progress );
}

#pragma warning( pop )

boost::shared_ptr<frantic::particles::streams::particle_istream>
IMaxKrakatoaPRTObject_Legacy2::GetRenderStream( const frantic::channels::channel_map& pcm,
                                                const frantic::max3d::shaders::renderInformation& renderInfo,
                                                INode* pNode, TimeValue t, TimeValue /*timeStep*/ ) {
    std::unique_ptr<IEvalContext_Legacy2> tempContext( CreateDefaultEvalContext( pcm, renderInfo.m_camera, t ) );

    particle_istream_ptr result = this->GetParticleStream( tempContext.get(), pNode );

    // We need to tie the lifetime of the IEvalContext to the lifetime of the particle stream, so we do it here with
    // this particle_istream subclass.
    result.reset( new ContextHolderParticleIStream( tempContext.release(), result ) );

    return result;
}

class IMaxKrakatoaPRTEvalContextWrapper : public IMaxKrakatoaPRTEvalContext {
  public:
    IMaxKrakatoaPRTEvalContextWrapper( IMaxKrakatoaPRTObject_Legacy2::IEvalContext_Legacy2* pEvalContext );

    virtual ~IMaxKrakatoaPRTEvalContextWrapper();

    virtual Class_ID GetContextID() const;

    virtual bool WantsWorldSpaceParticles() const;

    virtual bool WantsMaterialEffects() const;

    virtual RenderGlobalContext& GetRenderGlobalContext() const;

    virtual const frantic::graphics::camera<float>& GetCamera() const;

    virtual const frantic::channels::channel_map& GetDefaultChannels() const;

    virtual frantic::logging::progress_logger& GetProgressLogger() const;

    virtual bool GetProperty( const Class_ID& propID, void* pTarget ) const;

  private:
    IMaxKrakatoaPRTObject_Legacy2::IEvalContext_Legacy2* m_pEvalContext;

    frantic::channels::channel_map m_adjustedMap;
};

IMaxKrakatoaPRTEvalContextWrapper::IMaxKrakatoaPRTEvalContextWrapper(
    IMaxKrakatoaPRTObject_Legacy2::IEvalContext_Legacy2* pEvalContext )
    : m_pEvalContext( pEvalContext ) {
    // Clients are calling IMaxKrakatoaPRTObject::GetParticleStream() are from a time when Age & LifeSpan were usually
    // reported as Ticks. We switched to using seconds so I need to fiddle with the channel map here.
    frantic::max3d::particles::streams::convert_time_channels_to_seconds( pEvalContext->get_channel_map(),
                                                                          m_adjustedMap );
}

IMaxKrakatoaPRTEvalContextWrapper::~IMaxKrakatoaPRTEvalContextWrapper() {}

Class_ID IMaxKrakatoaPRTEvalContextWrapper::GetContextID() const { return Class_ID( 0, 0 ); }

bool IMaxKrakatoaPRTEvalContextWrapper::WantsWorldSpaceParticles() const { return true; }

bool IMaxKrakatoaPRTEvalContextWrapper::WantsMaterialEffects() const { return true; }

RenderGlobalContext& IMaxKrakatoaPRTEvalContextWrapper::GetRenderGlobalContext() const {
    return const_cast<IMaxKrakatoaPRTObject_Legacy2::IEvalContext_Legacy2*>( m_pEvalContext )->get_max_context();
}

const frantic::graphics::camera<float>& IMaxKrakatoaPRTEvalContextWrapper::GetCamera() const {
    return m_pEvalContext->get_camera();
}

const frantic::channels::channel_map& IMaxKrakatoaPRTEvalContextWrapper::GetDefaultChannels() const {
    return /*m_pEvalContext->get_channel_map()*/ m_adjustedMap;
}

frantic::logging::progress_logger& IMaxKrakatoaPRTEvalContextWrapper::GetProgressLogger() const {
    return m_pEvalContext->get_progress_logger();
}

bool IMaxKrakatoaPRTEvalContextWrapper::GetProperty( const Class_ID& /*propID*/, void* /*pTarget*/ ) const {
    return false;
}

particle_istream_ptr IMaxKrakatoaPRTObject::GetParticleStream( IEvalContext_Legacy2* evalContext, INode* pNode ) {
    boost::shared_ptr<IMaxKrakatoaPRTEvalContextWrapper> pContextWrapper(
        new IMaxKrakatoaPRTEvalContextWrapper( evalContext ) );

    Interval dontCare;

    particle_istream_ptr pResult = this->CreateStream( pNode, dontCare, pContextWrapper );

    if( pResult != NULL ) {
        pResult = frantic::max3d::particles::streams::convert_time_channels_to_ticks( pResult );

        pResult->set_channel_map( evalContext->get_channel_map() );
    }

    return pResult;
}

namespace {
void null_deleter( void* ) {}

class DefaultMaxKrakatoaPRTEvalContext : public IMaxKrakatoaPRTEvalContext {
  public:
    DefaultMaxKrakatoaPRTEvalContext( TimeValue t, const Class_ID& requestOnwer );

    virtual ~DefaultMaxKrakatoaPRTEvalContext();

    void SetWantsWorldSpaceParticles( bool wantsWorldSpaceParticles );

    void SetWantsMaterialEffects( bool wantsMaterialEffects );

    void SetCamera( const frantic::graphics::camera<float>& theCamera );

    void SetDefaultChannels( const frantic::channels::channel_map& defaultChannels );

    void SetProgressLogger( const boost::shared_ptr<frantic::logging::progress_logger>& pLogger );

    virtual Class_ID GetContextID() const;

    virtual bool WantsWorldSpaceParticles() const;

    virtual bool WantsMaterialEffects() const;

    virtual RenderGlobalContext& GetRenderGlobalContext() const;

    virtual const frantic::graphics::camera<float>& GetCamera() const;

    virtual const frantic::channels::channel_map& GetDefaultChannels() const;

    virtual frantic::logging::progress_logger& GetProgressLogger() const;

    virtual bool GetProperty( const Class_ID& propID, void* pTarget ) const;

  private:
    Class_ID m_requestOwner;

    RenderGlobalContext m_globContext;

    frantic::graphics::camera<float> m_camera;

    frantic::channels::channel_map m_defaultChannels;

    bool m_wantsWorldSpaceParticles;
    bool m_wantsMaterialEffects;

    frantic::logging::null_progress_logger m_defaultLogger;

    boost::shared_ptr<frantic::logging::progress_logger> m_progressLogger;
};

DefaultMaxKrakatoaPRTEvalContext::DefaultMaxKrakatoaPRTEvalContext( TimeValue t, const Class_ID& requestOwner )
    : m_requestOwner( requestOwner )
    , m_progressLogger( &m_defaultLogger, &null_deleter ) {
    m_wantsWorldSpaceParticles = true;
    m_wantsMaterialEffects = true;

    m_defaultChannels.define_channel( _T("Position"), 3, frantic::channels::data_type_float32 );
    m_defaultChannels.end_channel_definition();

    m_globContext.time = t;
    m_globContext.renderer = NULL;
    m_globContext.antialias = TRUE;
    m_globContext.frameDur = 1.f;
    m_globContext.envMap = NULL;
    m_globContext.globalLightLevel.White();
    m_globContext.atmos = NULL;
    m_globContext.pToneOp = NULL;
    m_globContext.wireMode = FALSE;
    m_globContext.wire_thick = 1.f;
    m_globContext.force2Side = FALSE;
    m_globContext.inMtlEdit = FALSE;
    m_globContext.fieldRender = FALSE;
    m_globContext.first_field = TRUE;
    m_globContext.field_order = FALSE;
    m_globContext.objMotBlur = FALSE;
    m_globContext.nBlurFrames = 0;
    m_globContext.simplifyAreaLights = TRUE;

    this->SetCamera( frantic::graphics::camera<float>() );
}

DefaultMaxKrakatoaPRTEvalContext::~DefaultMaxKrakatoaPRTEvalContext() {}

void DefaultMaxKrakatoaPRTEvalContext::SetWantsWorldSpaceParticles( bool wantsWorldSpaceParticles ) {
    m_wantsWorldSpaceParticles = wantsWorldSpaceParticles;
}

void DefaultMaxKrakatoaPRTEvalContext::SetWantsMaterialEffects( bool wantsMaterialEffects ) {
    m_wantsMaterialEffects = wantsMaterialEffects;
}

void DefaultMaxKrakatoaPRTEvalContext::SetCamera( const frantic::graphics::camera<float>& theCamera ) {
    m_camera = theCamera;

    m_globContext.camToWorld = frantic::max3d::to_max_t( m_camera.world_transform() );
    m_globContext.worldToCam = frantic::max3d::to_max_t( m_camera.world_transform_inverse() );
    m_globContext.nearRange = m_camera.near_distance();
    m_globContext.farRange = m_camera.far_distance();
    m_globContext.devAspect = m_camera.pixel_aspect();
    m_globContext.devWidth = m_camera.get_output_size().xsize;
    m_globContext.devHeight = m_camera.get_output_size().ysize;
    m_globContext.projType = ( m_camera.projection_mode() == frantic::graphics::projection_mode::orthographic )
                                 ? PROJ_PARALLEL
                                 : PROJ_PERSPECTIVE;

    m_globContext.xc = static_cast<float>( m_globContext.devWidth ) * 0.5f;
    m_globContext.yc = static_cast<float>( m_globContext.devHeight ) * 0.5f;

    if( m_globContext.projType == PROJ_PERSPECTIVE ) {
        float v = m_globContext.xc / std::tan( 0.5f * m_camera.horizontal_fov() );
        m_globContext.xscale = -v;
        m_globContext.yscale = v * m_globContext.devAspect;
    } else {
        const float VIEW_DEFAULT_WIDTH = 400.f;
        m_globContext.xscale =
            static_cast<float>( m_globContext.devWidth ) / ( VIEW_DEFAULT_WIDTH * m_camera.orthographic_width() );
        m_globContext.yscale = -m_globContext.devAspect * m_globContext.xscale;
    }
}

void DefaultMaxKrakatoaPRTEvalContext::SetDefaultChannels( const frantic::channels::channel_map& defaultChannels ) {
    m_defaultChannels = defaultChannels;
}

void DefaultMaxKrakatoaPRTEvalContext::SetProgressLogger(
    const boost::shared_ptr<frantic::logging::progress_logger>& pLogger ) {
    m_progressLogger = pLogger;
}

Class_ID DefaultMaxKrakatoaPRTEvalContext::GetContextID() const { return m_requestOwner; }

bool DefaultMaxKrakatoaPRTEvalContext::WantsWorldSpaceParticles() const { return m_wantsWorldSpaceParticles; }

bool DefaultMaxKrakatoaPRTEvalContext::WantsMaterialEffects() const { return m_wantsMaterialEffects; }

RenderGlobalContext& DefaultMaxKrakatoaPRTEvalContext::GetRenderGlobalContext() const {
    return const_cast<RenderGlobalContext&>( m_globContext );
}

const frantic::graphics::camera<float>& DefaultMaxKrakatoaPRTEvalContext::GetCamera() const { return m_camera; }

const frantic::channels::channel_map& DefaultMaxKrakatoaPRTEvalContext::GetDefaultChannels() const {
    return m_defaultChannels;
}

frantic::logging::progress_logger& DefaultMaxKrakatoaPRTEvalContext::GetProgressLogger() const {
    return *m_progressLogger;
}

bool DefaultMaxKrakatoaPRTEvalContext::GetProperty( const Class_ID& /*propID*/, void* /*pTarget*/ ) const {
    return false;
}
} // namespace

IMaxKrakatoaPRTEvalContextPtr CreateMaxKrakatoaPRTEvalContext(
    TimeValue t, const Class_ID& contextID, const frantic::graphics::camera<float>* pCamera,
    const frantic::channels::channel_map* pChannels, bool wantsWorldSpaceParticles, bool wantsMaterialEffects,
    boost::shared_ptr<frantic::logging::progress_logger> pLogger ) {
    boost::shared_ptr<DefaultMaxKrakatoaPRTEvalContext> pResult =
        boost::make_shared<DefaultMaxKrakatoaPRTEvalContext>( t, contextID );

    pResult->SetWantsWorldSpaceParticles( wantsWorldSpaceParticles );
    pResult->SetWantsMaterialEffects( wantsMaterialEffects );

    if( pCamera )
        pResult->SetCamera( *pCamera );

    if( pChannels )
        pResult->SetDefaultChannels( *pChannels );

    if( pLogger )
        pResult->SetProgressLogger( pLogger );

    return pResult;
}

IMaxKrakatoaPRTEvalContext::~IMaxKrakatoaPRTEvalContext() {}

TimeValue IMaxKrakatoaPRTEvalContext::GetTime() const { return this->GetRenderGlobalContext().time; }

particle_istream_ptr IMaxKrakatoaPRTObject::CreateStream( INode* pNode, TimeValue t, Interval& outValidity,
                                                          const Class_ID& requestOwner ) {
    IMaxKrakatoaPRTEvalContextPtr pEvalContext = CreateMaxKrakatoaPRTEvalContext( t, requestOwner );

    return this->CreateStream( pNode, outValidity, pEvalContext );
}

} // namespace particles
} // namespace max3d
} // namespace frantic
