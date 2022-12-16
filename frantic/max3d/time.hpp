// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#pragma warning( push, 3 )
#include <ParticleFlow\PreciseTimeValue.h>
#include <control.h>
#include <iparamb2.h>
#include <maxtypes.h>
#pragma warning( pop )

namespace frantic {
namespace max3d {

/**
 * When converting from 3ds Max's TimeValue units to floating point seconds, this is the preferred data type.
 */
typedef double seconds_type;

/**
 * Converts from TimeValue ticks to floating point seconds.
 * \tparam T The floating point type to use when converting to seconds.
 * \param t The time in ticks to convert to seconds.
 * \return The time 't' as represented in seconds.
 */
template <class T>
T to_seconds( TimeValue t ) {
    return static_cast<T>( t ) / static_cast<T>( TIME_TICKSPERSEC );
}

/**
 * \overload This overload of to_seconds() works with ParticleFlow's PreciseTimeValue structs, which can measure
 * sub-tick times.
 */
template <class T>
T to_seconds( const PreciseTimeValue& t ) {
    return ( static_cast<T>( t.tick ) + static_cast<T>( t.fraction ) ) / static_cast<T>( TIME_TICKSPERSEC );
}

/**
 * Converts from TimeValue ticks to floating point milliseconds.
 * \tparam T The floating point type to use when converting to milliseconds.
 * \param t The time in ticks to convert to milliseconds.
 * \return The time 't' as represented in milliseconds.
 */
template <class T>
T to_milliseconds( TimeValue t ) {
    // Since we know 'TIME_TICKSPERSEC' is 4800, we can divide the denominator by 200 and multiply the numerator by 5 to
    // get an effective scaling of the numerator by 1000.

    boost::int64_t numerator = 5i64 * t;
    boost::int64_t denominator = TIME_TICKSPERSEC / 200;

    // Calculate the whole milliseconds and fractional part separately to avoid precision loss while converting
    // numerator to a floating point type.
    return static_cast<T>( numerator / denominator ) +
           static_cast<T>( numerator % denominator ) / static_cast<T>( denominator );
}

/**
 * \overload This overload of to_milliseconds() works with ParticleFlow's PreciseTimeValue structs, which can measure
 * sub-tick times.
 */
template <class T>
T to_milliseconds( const PreciseTimeValue& t ) {
    return to_milliseconds( t.tick ) +
           ( static_cast<T>( 5 ) * static_cast<T>( t.fraction ) ) / static_cast<T>( TIME_TICKSPERSEC / 200 );
}

/**
 * Converts from TimeValue ticks to floating point picoseconds.
 * \tparam T The floating point type to use when converting to picoseconds.
 * \param t The time in ticks to convert to picoseconds.
 * \return The time 't' as represented in picoseconds.
 */
template <class T>
T to_picoseconds( TimeValue t ) {
    // Since we know 'TIME_TICKSPERSEC' is 4800, we can divide the denominator by 1600 and multiply the numerator by 625
    // to get an effective scaling of the numerator by 1000000.

    boost::int64_t numerator = 625i64 * t;
    boost::int64_t denominator = TIME_TICKSPERSEC / 1600;

    // Calculate the whole picoseconds and fractional part separately to avoid precision loss while converting numerator
    // to a floating point type.
    return static_cast<T>( numerator / denominator ) +
           static_cast<T>( numerator % denominator ) / static_cast<T>( denominator );
}

/**
 * \overload This overload of to_picoseconds() works with ParticleFlow's PreciseTimeValue structs, which can measure
 * sub-tick times.
 */
template <class T>
T to_picoseconds( const PreciseTimeValue& t ) {
    return to_picoseconds( t.tick ) +
           ( static_cast<T>( 625 ) * static_cast<T>( t.fraction ) ) / static_cast<T>( TIME_TICKSPERSEC / 1600 );
}

/**
 * Converts from TimeValue ticks to floating point frames.
 * \tparam T The floating point type to use when converting to frames.
 * \param t The time in ticks to convert to frames.
 * \param frameRate The number of frames per second.
 * \return The time 't' as represented in frames.
 */
template <class T>
T to_frames( TimeValue t, int frameRate = GetFrameRate() ) {
    return static_cast<T>( t ) / static_cast<T>( TIME_TICKSPERSEC / frameRate );
}

/**
 * \overload This overload of to_frames() works with ParticleFlow's PreciseTimeValue structs, which can measure sub-tick
 * times.
 */
template <class T>
T to_frames( const PreciseTimeValue& t, int frameRate = GetFrameRate() ) {
    return ( static_cast<T>( t.tick ) + static_cast<T>( t.fraction ) ) / static_cast<T>( TIME_TICKSPERSEC / frameRate );
}

/**
 * Converts a time in seconds to 3ds Max TimeValue ticks.
 * \tparam T The type used to represent seconds. Usually float or double.
 * \param seconds The time in seconds to convert
 * \return The TimeValue that is equivalent to 'seconds'.
 */
template <class T>
TimeValue to_ticks( T seconds ) {
    return static_cast<TimeValue>( seconds * static_cast<T>( TIME_TICKSPERSEC ) );
}

/**
 * Evaluates the standard controls used to control sequence playback and determines the closest whole frame to the input
 * time. The next closest whole frame is also calculated in order to do interpolation if desired.
 *
 * \param t The "current" time to convert to a frame.
 * \param playbackControl Can be NULL. If provided, this parameter is evaluated at 't' in and the result is interpreted
 * as the frame requested. Doesn't really make sense if it is not animated. \param offset An offset in frames to add
 * onto the calculated frame. \param outInterpFrame[out] Receives the second closest frame. This is the return value
 * +- 1.0 frames. \param outInterpParam[out] Receives the interpolation parameter used to interpolate between the return
 * value and outInterpFrame. In [-0.5, 0.5) since outInterpFrame is +- 1 frame from the return value. \param
 * outDerivative[out] Receives the rate of change of time, as caused by an animated playback control. Linear (ie.
 * normal) time will have a derivative of 1. Stopped time will have a derivative of 0. This value should be used to
 * scale the magnitude of the velocity in the file. \param valid The validity interval is updated with the validity of
 * the playbackControl if provided and animated. \return The closest whole frame to the provided time, modified by the
 * playback control & offset.
 */
inline double get_sequence_whole_frame( TimeValue t, PB2Value* playbackControl, double offset, double& outInterpFrame,
                                        double& outInterpParam, double& outDerivative, Interval& valid ) {
    double frame;

    int frameRate = GetFrameRate();
    int ticksPerFrame = TIME_TICKSPERSEC / frameRate;

    if( playbackControl ) {
        if( !playbackControl->is_constant() ) {
            Interval garbage = FOREVER;
            TimeValue timestep = ticksPerFrame / 16;

            float vPrev, vCur, vNext;
            playbackControl->control->GetValue( t - timestep, &vPrev, garbage );
            playbackControl->control->GetValue( t, &vCur, valid );
            playbackControl->control->GetValue( t + timestep, &vNext, garbage );

            frame = vCur;

            // Centered difference calculation of the modified time rate.
            outDerivative = 8.0 * ( static_cast<double>( vNext ) - static_cast<double>( vPrev ) );
        } else {
            frame = playbackControl->f;
            outDerivative = 0;
        }
    } else {
        frame = to_frames<double>( t, frameRate );
        outDerivative = 1.0;
    }

    frame += offset;

    double wholeFrame = std::floor( frame + 0.5 );

    const double tol =
        1.5 / static_cast<double>( ticksPerFrame ); // +- 1.5 ticks is considered to be exactly on the whole frame.

    if( std::abs( frame - wholeFrame ) < tol ) {
        outInterpFrame = wholeFrame;
        outInterpParam = 0;
    } else if( wholeFrame < frame ) {
        outInterpFrame = wholeFrame + 1.0;
        outInterpParam = frame - wholeFrame;
    } else {
        outInterpFrame = wholeFrame - 1.0;
        outInterpParam = frame - wholeFrame;
    }

    return wholeFrame;
}

} // namespace max3d
} // namespace frantic
