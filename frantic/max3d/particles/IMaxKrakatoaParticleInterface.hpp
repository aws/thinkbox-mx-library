// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <SDKDDKVer.h>
#include <max.h>
#if MAX_VERSION_MAJOR >= 14
#include <maxscript/mxsplugin/mxsPlugin.h>
#endif

/**
 * Use this Interface_ID to to get this interface from a Krakatoa particle object.
 */
#define MAXKRAKATOA_PARTICLE_INTERFACE_ID Interface_ID( 0x57de093f, 0x621075b1 )

/**
 * The possible channel data types.
 * Used in KrakatoaParticleChannelAccessor. Use the dataType along with the arity to decide which get function to use.
 * get_int64(): INT8, INT16, INT32, INT64 and an arity of one.
 * get_uint64(): UINT8, UINT16, UINT32, UINT64 and an arity of one.
 * get_double() or get_float(): FLOAT16, FLOAT32, FLOAT64 and an arity of one.
 * get_double_vector() or get_float_vector(): FLOAT16, FLOAT32, FLOAT64 and an arity of three.
 */
enum channel_data_type_t {
    DATA_TYPE_INVALID,
    DATA_TYPE_INT8,
    DATA_TYPE_INT16,
    DATA_TYPE_INT32,
    DATA_TYPE_INT64,
    DATA_TYPE_UINT8,
    DATA_TYPE_UINT16,
    DATA_TYPE_UINT32,
    DATA_TYPE_UINT64,
    DATA_TYPE_FLOAT16,
    DATA_TYPE_FLOAT32,
    DATA_TYPE_FLOAT64
};

/**
 * This class stores info about a channel and can be used to got the channel value form a particle.
 * Use KrakatoaParticleStream's get_channel_data_accessor() to retrieved a pointer to a KrakatoaParticleChannelAccessor
 * object. Use with a particle to get the particle's channel value. Particles come from KrakatoaParticleStream's
 * get_next_particle(). For example usage, see the sample project included in this API.
 */
class KrakatoaParticleChannelAccessor {
  public:
    /**
     * \return The name of the channel associated with this accessor.
     */
    virtual const char* get_name() const = 0;

    /**
     * \return The data type of the channel associated with this accessor.
     */
    virtual channel_data_type_t get_data_type() const = 0;

    /**
     * \return The arity of the channel associated with this accessor.
     */
    virtual unsigned int get_arity() const = 0;

    /**
     * If this returns true and the channel's arity is one then you should use get_int64() to get the channel value.
     * The integer data types are DATA_TYPE_INT8, DATA_TYPE_INT16, DATA_TYPE_INT32 and DATA_TYPE_INT64.
     * \return True if this channel's data type is an integer.
     */
    virtual bool is_int_channel() const = 0;

    /**
     * If this returns true and the channel's arity is one then you should use get_uint64() to get the channel value.
     * The unsigned integer data types are DATA_TYPE_UINT8, DATA_TYPE_UINT16, DATA_TYPE_UINT32 and DATA_TYPE_UINT64.
     * \return True if this channel's data type is an unsigned integer.
     */
    virtual bool is_uint_channel() const = 0;

    /**
     * If this returns true and the channel's arity is one then you should use get_double() to get the channel value
     *    else if this returns true and the channel's arity is three then you should use get_double_vector() to get the
     * channel values. The floating point data types are DATA_TYPE_FLOAT16, DATA_TYPE_FLOAT32 and DATA_TYPE_FLOAT64.
     * \return True if this channel's data type is a floating point.
     */
    virtual bool is_float_channel() const = 0;

    /**
     * Get the channel value for any integer.
     * Promotes any integer to 64 bit.
     * Use is_int_channel() make sure the channel data type is an integer.
     * Use get_arity() to make sure the channel is scalar ( an arity of one ).
     * \param outValue An output integer for the value read from particleData.
     * \param particleData A particle. Particles are provided by the KrakatoaParticleStream's get_next_particle().
     */
    virtual void get_int64( INT64& outValue, const void* particleData ) const = 0;

    /**
     * Get the channel value for any unsigned integer.
     * Promotes any integer to 64 bit.
     * Use is_uint_channel() make sure the channel data type is an unsigned integer.
     * Use get_arity() to make sure the channel is scalar ( an arity of one ).
     * \param outValue An output unsigned integer for the value read from particleData.
     * \param particleData A particle. Particles are provided by the KrakatoaParticleStream's get_next_particle().
     */
    virtual void get_uint64( UINT64& outValue, const void* particleData ) const = 0;

    /**
     * Get the channel value for any floating point scalar.
     * Use is_float_channel() make sure the channel data type is a floating point.
     * Use get_arity() to make sure the channel is scalar ( an arity of one ).
     * \param outValue An output floating point for the value read from particleData.
     * \param particleData A particle. Particles are provided by the KrakatoaParticleStream's get_next_particle().
     */
    virtual void get_float( float& outValue, const void* particleData ) const = 0;

    /**
     * Get the channel values for three floating points.
     * Use is_float_channel() make sure the channel data type is a floating point.
     * Use get_arity() to make sure the channel has an arity of three.
     * \param outValue A pointer to an output vector of three floating points for the values read from particleData.
     * \param particleData A particle. Particles are provided by the KrakatoaParticleStream's get_next_particle().
     */
    virtual void get_float_vector( float outVector[3], const void* particleData ) const = 0;

    /**
     * Get the channel value for a floating point scalar.
     * Use is_float_channel() make sure the channel data type is a floating point.
     * Use get_arity() to make sure the channel is scalar ( an arity of one ).
     * \param outValue An output floating point for the value read from particleData.
     * \param particleData A particle. Particles are provided by the KrakatoaParticleStream's get_next_particle().
     */
    virtual void get_double( double& outValue, const void* particleData ) const = 0;

    /**
     * Get the channel values for three floating points.
     * Use is_float_channel() make sure the channel data type is a floating point.
     * Use get_arity() to make sure the channel has an arity of three.
     * \param outValue A pointer to an output vector of three floating points for the values read from particleData.
     * \param particleData A particle. Particles are provided by the KrakatoaParticleStream's get_next_particle().
     */
    virtual void get_double_vector( double outVector[3], const void* particleData ) const = 0;

    /**
     * Retieves data from the particle.
     * \warning Don't use this function unless you know what you are doing. Please use the appropriate get functions
     * instead, they are type safe: get_int64() for integer scalar values, get_uint64() for unsigned integer scalar
     * values, get_double() or get_float() for any floating point scalar values, get_double_vector() or
     * get_float_vector() for a vector of three floating points. \param outValue A pointer to an output buffer for the
     * value(s) read from particleData. This must point to an allocated buffer with enough bytes to hold the value. For
     * example: Pass in a 4-byte buffer to retrive a single float32, etc. \param particleData A particle. Particles are
     * provided by the KrakatoaParticleStream's get_next_particle().
     */
    virtual void get_channel_value( void* outValue, const void* particleData ) const = 0;
};

/**
 * This class defines the interface for a read-only stream of data.
 * For example usage, see the sample project included in this API.
 */
class KrakatoaParticleStream {
  public:
    /**
     * Gets the next particle in the stream.
     * The user must pass in a memory buffer of size particle_size() in bytes.
     * Channels values from the resulting buffer can be retrieved using a KrakatoaParticleChannelAccessor.
     * This function call be called particle_count() times.
     * However, if particle_count() returns -1, it will be called total_num_particles+1 times. For example: If there is
     * ONE particle in the stream, the user would call get_next_particle twice. The first time it would provide a
     * particle, and return true. The second time it would return false, and no valid particle would be provided. \param
     * outParticleData A buffer of memory of size particle_size() in bytes. \return True if a particle was read, false
     * otherwise. False indicates end of stream.
     */
    virtual bool get_next_particle( void* outParticleData ) = 0;

    /**
     * This function returns the number of particles in the stream. The user then can call "get_next_particle" that many
     * times. If this returns -1, the count is unknown, and the user must call get_next_particle, until
     * get_next_particle returns false. \return The number of particles in the stream, or -1 if unknown.
     */
    virtual INT64 particle_count() const = 0;

    /**
     * Provides the size of a single particle in bytes.
     * You must pass a memory buffer of this many bytes when calling get_next_particle().
     * \return The size of a particle in bytes in this stream.
     */
    virtual unsigned int particle_size() const = 0;

    /**
     * Checks if a specified channel exists.
     * \param name The name of the channel.
     */
    virtual bool has_channel( const char* name ) const = 0;

    /**
     * Gets the number of channels each particle has. This count can be used to iterate over the channels using
     * get_channel_data_accessor. \return The number of channels in the stream.
     */
    virtual unsigned int channel_count() const = 0;

    /**
     * Gets a channel data accessor by name.
     * You should call has_channel() to be sure the channel exists in the stream.
     * \warning NEVER call delete on the resulting accessor object. Memory is not leaked. It cleaned up when the stream
     * is destoryed. \warning NEVER use the resulting accessor object once the creating stream is destroyed. \param name
     * The name of the channel. \return A pointer to a KrakatoaParticleChannelAccessor object. This object can be used
     * to get a channel value from a particle.
     */
    virtual const KrakatoaParticleChannelAccessor* get_channel_data_accessor( const char* name ) const = 0;

    /**
     * Gets a channel data accessor by index.
     * You should call channel_count() to be sure the index is within bounds. Indexes start at 0.
     * \warning NEVER call delete on the resulting accessor object. Memory is not leaked. It cleaned up when the stream
     * is destoryed. \warning NEVER use the resulting accessor object once the creating stream is destroyed. \param name
     * The index of the channel. \return A pointer to a KrakatoaParticleChannelAccessor object. This object can be used
     * to get a channel value from a particle.
     */
    virtual const KrakatoaParticleChannelAccessor* get_channel_data_accessor( unsigned int index ) const = 0;
};

/**
 * An interface that can be used to get a stream of particles from a Krakatoa particle object.
 * To get one of these object from a node, use get_krakatoa_particle_interface().
 * For example usage, see the sample project included in this API.
 * \note You must call destroy_stream() on any created KrakatoaParticleStream once you are finished using it.
 */
class IMaxKrakatoaParticleInterface : public BaseInterface {
  public:
    virtual Interface_ID GetID() { return MAXKRAKATOA_PARTICLE_INTERFACE_ID; }

    /**
     * Creates a new KrakatoaParticleStream at the specified time from a particle object.
     * \note Remember to call destroy_stream() after completed using this resulting object.
     * \param pNode The INode that is associated with the Object that created this interface. The reason this is needed
     * is because some data (transformation matrix, material, node-level visibility) are not available at the object
     * level. \param t The scene time to create the stream. \param outValidity Will be set to the interval that this
     * stream is valid for. Can be ignored if you don't care. \param inWorldSpace If true, the particle positions (and
     * other relavent vector channels) will be provided in their world-space positions. \param applyMaterial If true,
     * and a supported material is applied to the node, Krakatoa will evaluate the material for color and density.
     * \param forceRenderMode Passing true for this parameter will override the current context, and instead pretend it
     * is in a render context. Explanation: Krakatoa produces different particles if it's in viewport display mode or in
     * render mode. If this function is called outside of a render, the viewport particles will be produced. If it
     * called during a render, the render particles will be produced. Setting this flag overrides this behavior. \return
     * A new KrakatoaParticleStream object.
     */
    virtual KrakatoaParticleStream* create_stream( INode* pNode, TimeValue t, Interval& outValidity,
                                                   bool inWorldSpace = true, bool applyMaterial = true,
                                                   bool forceRenderMode = false );

    /**
     * Closes and destroys a KrakatoaParticleStream that was created with CreateStream().
     * \param stream The KrakatoaParticleStream to destroy.
     */
    virtual void destroy_stream( KrakatoaParticleStream* stream );
};

/**
 * Convenience function to get the Krakatoa particle interface from an object.
 * For example usage, see the sample project included in this API.
 */
inline IMaxKrakatoaParticleInterface* get_krakatoa_particle_interface( Animatable* pObj ) {
    // There are two cases we have to handle: The case where the object itself implements the interface (PRT Volume,
    // etc.), and the case where the node is a scripted plugin, and its delegate (parent) object implements the
    // interface (PRT Loader). This code resolves which object to use.
    void* scriptedInterface = pObj->GetInterface( I_MAXSCRIPTPLUGIN );
    if( scriptedInterface ) {
#if MAX_VERSION_MAJOR >= 14
        pObj = static_cast<MSPlugin*>( scriptedInterface )->get_delegate();
#else
        pObj = pObj->NumSubs() > 0 ? pObj->SubAnim( 0 ) : NULL; // deprecated way of getting delegate (base) object.
#endif
    }

    // Get the Krakatoa interface from the object.
    // This will result in a NULL if the object itself is not a valid Krakatoa particle object.
    // It will also produce NULL if the version of Krakatoa running is prior to 2.4.0.
    return dynamic_cast<IMaxKrakatoaParticleInterface*>( pObj->GetInterface( MAXKRAKATOA_PARTICLE_INTERFACE_ID ) );
}