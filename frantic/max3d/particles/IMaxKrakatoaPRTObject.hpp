// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#pragma warning( push, 3 )
#if !defined(NO_INIUTIL_USING)
#define NO_INIUTIL_USING
#endif
#include <baseinterface.h>
#include <max.h>
#pragma warning( pop )

#undef base_type

#include <frantic/particles/streams/particle_istream.hpp>

#include <boost/intrusive_ptr.hpp>

// Interface IDs for use with InterfaceServer::GetInterface(Interface_ID)
#define MAXKRAKATOAPRTOBJECT_INTERFACE Interface_ID( 0x43e3357b, 0x1a872a98 )
#define MAXKRAKATOAPRTOBJECT_LEGACY2_INTERFACE Interface_ID( 0x63102b19, 0x38b229b5 )
#define MAXKRAKATOAPRTOBJECT_LEGACY1_INTERFACE Interface_ID( 0xec102f8, 0x29ab10bc )

// Forward declarations
#pragma region FwdDecl
namespace frantic {
namespace channels {
class channel_map;
}
} // namespace frantic

namespace frantic {
namespace graphics {
template <typename FloatType>
class camera;
}
} // namespace frantic

namespace frantic {
namespace logging {
class progress_logger;
}
} // namespace frantic

namespace frantic {
namespace max3d {
namespace shaders {
class renderInformation;
}
} // namespace max3d
} // namespace frantic
#pragma endregion

namespace frantic {
namespace max3d {
namespace particles {

using frantic::particles::particle_istream_ptr;

class IMaxKrakatoaPRTObject;

typedef boost::intrusive_ptr<IMaxKrakatoaPRTObject> IMaxKrakatoaPRTObjectPtr;

/**
 * Will return a pointer to an IMaxKrakatoaPRTObject if one is available from the passed ReferenceMaker*
 * @param pObj The object that might have a IMaxKrakatoaPRTObject interface available.
 * @return A pointer to an IMaxKrakatoaPRTObject object. Will be a NULL ptr if pObj does not support this interface.
 */
IMaxKrakatoaPRTObjectPtr GetIMaxKrakatoaPRTObject( ReferenceMaker* pObj );

/**
 * This is a legacy interface which is around for compatibility reasons. In modern MaxKrakatoa versions, there is a
 * potential performance penalty for using this interface compared to IMaxKrakatoaPRTObject.
 */
class IMaxKrakatoaPRTObject_Legacy1 : public BaseInterface {
  public:
    Interface_ID GetID() { return MAXKRAKATOAPRTOBJECT_LEGACY1_INTERFACE; }

    virtual boost::shared_ptr<frantic::particles::streams::particle_istream>
    GetRenderStream( const frantic::channels::channel_map& pcm,
                     const frantic::max3d::shaders::renderInformation& renderInfo, INode* pNode, TimeValue t,
                     TimeValue timeStep ) = 0;
};

/**
 * This is a legacy interface which is around for compatibility reasons. In modern MaxKrakatoa versions, there is a
 * potential performance penalty for using this interface compared to IMaxKrakatoaPRTObject. \note This class/interface
 * was deprecated and replaced on 2/1/2013 (Krakatoa MX 2.1.7, Frost MX 1.3.4)
 */
class IMaxKrakatoaPRTObject_Legacy2 : public IMaxKrakatoaPRTObject_Legacy1 {
  public:
    typedef IMaxKrakatoaPRTObjectPtr ptr_type;

    class IEvalContext_Legacy2;

  public:
    static IEvalContext_Legacy2*
    CreateDefaultEvalContext( const frantic::channels::channel_map& pcm, const frantic::graphics::camera<float>& camera,
                              TimeValue t,
                              boost::shared_ptr<frantic::logging::progress_logger> progress =
                                  boost::shared_ptr<frantic::logging::progress_logger>() );

  public:
    Interface_ID GetID() { return MAXKRAKATOAPRTOBJECT_LEGACY2_INTERFACE; }

    /**
     * Deprecated.
     */
    __declspec( deprecated ) inline virtual boost::
        shared_ptr<frantic::particles::streams::particle_istream> GetRenderStream(
            const frantic::channels::channel_map& pcm, const frantic::max3d::shaders::renderInformation& renderInfo,
            INode* pNode, TimeValue t, TimeValue timeStep );

    /**
     * Retrieves an instance of a particle_istream that will produce the particles of this object instance's world
     * state.
     * @deprecated
     * @param evalContext A pointer to the global context for evaluating the particles. This is used for MANY things,
     * like the time at which to evaluate, channels expected for the particle streams, camera information, etc.
     * @param pNode The specific instance of this object to evaluate. The node's TM and Modifiers are used to affect the
     * particles.
     */
    virtual particle_istream_ptr GetParticleStream( IEvalContext_Legacy2* evalContext, INode* pNode ) = 0;
};

class IMaxKrakatoaPRTEvalContext;

/**
 * This interface is the primary way for particles, via particle_istream, to be retrieved from Krakatoa's PRT objects.
 */
class IMaxKrakatoaPRTObject : public IMaxKrakatoaPRTObject_Legacy2 {
  public:
    typedef IMaxKrakatoaPRTObjectPtr ptr_type;

  public:
    Interface_ID GetID() { return MAXKRAKATOAPRTOBJECT_INTERFACE; }

    /**
     * This is the primary way to create a particle_istream that encapsulates the particles in this object.
     * @param pNode The scene node that the particle object is associated with.
     * @param outValidity Receives a validity interval indicating the time over which the particles are unchanging, or
     * have a constant velocity.
     * @param pEvalContext An object that includes extra parameters that control the particle_istream created.
     * @return A particle_istream that generates the particles for the queried object.
     */
    virtual particle_istream_ptr CreateStream( INode* pNode, Interval& outValidity,
                                               boost::shared_ptr<IMaxKrakatoaPRTEvalContext> pEvalContext ) = 0;

    /**
     * @overload This version of CreateStream() applies default settings for the 'pEvalContext' parameter. You can use
     * this if you don't care about the various ways to control the created particles, or if you have no idea what they
     * should be.
     * @param requestOwner This parameter is used to uniquely identify the DLL that is evaluating the particles. We
     * intend to use this in order to apply specific work-arounds for bugs and future changes that need to be handled
     * with knowledge of the client. If you really can't fill this in properly, you can use Class_ID(0,0) to indicate
     * that.
     */
    virtual particle_istream_ptr CreateStream( INode* pNode, TimeValue t, Interval& outValidity,
                                               const Class_ID& requestOwner );

    /**
     * Returns all the channels that this stream can populate. The function is provided as an alternative to creating
     * the stream with CreateStream() and calling particle_istream::get_native_channel_map(), though it is not
     * guaranteed to be more efficient.
     *
     * @note Calling this function, then calling CreateStream() is likely twice as slow as just calling CreateStream()
     * and looking at the native channel_map so try not to do that.
     *
     * @param pNode The node this particle object is attached to.
     * @param t The time to evaluate the particle channels. If you don't have a good one, use
     * GetCOREInterface()->GetTime().
     * @param outChannelMap Will be assigned the native channel map of this particle object.
     */
    virtual void GetStreamNativeChannels( INode* pNode, TimeValue t,
                                          frantic::channels::channel_map& outChannelMap ) = 0;

    /**
     * @deprecated This function implements IMaxKrakatoaPRTObject_Legacy2::GetParticleStream() by calling CreateStream()
     * with the appropriate parameters.
     */
    __declspec( deprecated ) virtual particle_istream_ptr
        GetParticleStream( IEvalContext_Legacy2* evalContext, INode* pNode );
};

class IMaxKrakatoaPRTEvalContext {
  public:
    typedef boost::shared_ptr<IMaxKrakatoaPRTEvalContext> ptr_type;

  public:
    virtual ~IMaxKrakatoaPRTEvalContext();

    /**
     * In order to apply specific workarounds to streams generated for specific clients, this ID is used to determine
     * who exactly is asking for the particles.
     * @return A unique ID indicating who is asking for the particles. If unknown, use Class_ID(0,0)
     */
    virtual Class_ID GetContextID() const = 0;

    /**
     * @return True if the particles should be transformed into world space. If false, only the object-space portion of
     * the pipeline is evaluated.
     */
    virtual bool WantsWorldSpaceParticles() const = 0;

    /**
     * @return True if the particles should be affected by the material on the associated INode. If false, no material
     * is applied.
     */
    virtual bool WantsMaterialEffects() const = 0;

    /**
     * @return The time the particle evaluation is being done at.
     */
    TimeValue GetTime() const;

    /**
     * Returns a RenderGlobalContext which contains rendering information about the context. If the data members that
     * relate time and camera information don't match the equivalent parameters to IMaxKrakatoaPRTObject::CreateStream()
     * or the results
     * @note This is non-const due to Max's poor const handling.
     * @return The stored RenderGlobalContext.
     */
    virtual RenderGlobalContext& GetRenderGlobalContext() const = 0;

    /**
     * Returns the camera that view-dependent particles should use. Many 3dsMax TexMaps require the camera properties to
     * correctly assign a color to a particle. Also make sure that the camera related properties in RenderGlobalContext
     * match this object or underfined behaviour will occur.
     * @return A reference to the camera to be used by view-dependent particle sources.
     */
    virtual const frantic::graphics::camera<float>& GetCamera() const = 0;

    /**
     * Gets a list of channels that the client is requesting from this particle object. Must contain at least "Position
     * float32[3]". The resulting particle_istream of IMaxKrakatoaPRTObject::CreateStream() will have this as the
     * "current" channel_map. Most clients will just provide a default minimal value for this, so we cannot attempt to
     * optimize the created stream by assuming these are the only channels that could ever be asked for.
     * @return The channel_map to assign to the resulting particle_istream by default.
     */
    virtual const frantic::channels::channel_map& GetDefaultChannels() const = 0;

    /**
     * Return a progress_logger object to be used periodically updating the progress while an object is being evaluated.
     * @return A refernce to a progress_logger.
     */
    virtual frantic::logging::progress_logger& GetProgressLogger() const = 0;

    /**
     * We may need to extend the IMaxKrakatoaPRTEvalContext interface in the future, but it might be too cumbersome to
     * fully replace IMaxKrakatoaPRTObject so we could just access the property via this function.
     *
     * Ex.
     *  float motionBlurWidth = 1.f;
     *  theIMaxKrakatoaPRTEvalContext->GetProperty( MOTION_BLUR_WIDTH_PROP, &motionBlurWidth );
     *
     *
     * @param propID The unique property ID. This should be defined in a header somewhere (This one?)
     * @param pTarget Pointer to the memory location that receives a copy of the property's value.
     * @return True if the property was found and stored at 'pTarget'
     */
    virtual bool GetProperty( const Class_ID& propID, void* pTarget ) const = 0;
};

typedef IMaxKrakatoaPRTEvalContext::ptr_type IMaxKrakatoaPRTEvalContextPtr;

/**
 * Creates a new IMaxKrakatoaPRTEvalContext with a suitable default implementation.
 * @param t The time the context reports via IMaxKrakatoaPRTEvalContext::GetTime()
 * @param contextID A unique ID describing the client that is calling this function. Can be Class_ID(0, 0) if you don't
 * have a good choice.
 * @param pCamera If non-null, this camera will be copied and the copy returned via
 * IMaxKrakatoaPRTEvalContext::GetCamera().
 * @param pChannels If non-null, this channel_map will be copied and the copy returned via
 * IMaxKrakatoaPRTEvalContext::GetDefaultChannels().
 * @param wantsWorldSpaceParticles Determines the result of IMaxKrakatoaPRTEvalContext::WantsWorldSpaceParticles()
 * @param wantsMaterialEffects Determines the result of IMaxKrakatoaPRTEvalContext::WantsMaterialEffects()
 * @param pLogger If non-null, this progress_logger derived class instance will be returned via
 * IMaxKrakatoaPRTEvalContext::GetProgressLogger().
 * @return A smart pointer to a new IMaxKrakatoaPRTEvalContext derived class instance.
 */
IMaxKrakatoaPRTEvalContextPtr
CreateMaxKrakatoaPRTEvalContext( TimeValue t, const Class_ID& contextID,
                                 const frantic::graphics::camera<float>* pCamera = NULL,
                                 const frantic::channels::channel_map* pChannels = NULL,
                                 bool wantsWorldSpaceParticles = true, bool wantsMaterialEffects = true,
                                 boost::shared_ptr<frantic::logging::progress_logger> pLogger =
                                     boost::shared_ptr<frantic::logging::progress_logger>() );

/**
 * Required to use a boost::intrusive_ptr with BaseInterface objects.
 */
inline void intrusive_ptr_add_ref( BaseInterface* iface ) { iface->AcquireInterface(); }

/**
 * Required to use a boost::intrusive_ptr with BaseInterface objects.
 */
inline void intrusive_ptr_release( BaseInterface* iface ) {
    if( iface->LifetimeControl() == BaseInterface::wantsRelease )
        iface->ReleaseInterface();
}

} // namespace particles
} // namespace max3d
} // namespace frantic
