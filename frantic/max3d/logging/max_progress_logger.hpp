// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <frantic/diagnostics/timeout_tracker.hpp>
#include <frantic/logging/render_progress_logger.hpp>
#include <string>

#include <render.h>

namespace frantic {
namespace max3d {
namespace logging {

namespace detail {
// just a dummy function for the progress bar
inline DWORD WINAPI progress_dummy( LPVOID /*arg*/ ) { return ( 0 ); }
} // namespace detail

class max_progress_logger : public frantic::logging::progress_logger {
    frantic::diagnostics::timeout_tracker m_progressTimeout;
    frantic::diagnostics::timeout_tracker m_delayTimeout;
    Interface* m_ip;
    bool m_going, m_throwOnCancel;
    frantic::tstring m_message, m_title;

  public:
    max_progress_logger( const frantic::tstring& message, float progressStart = 0, float progressEnd = 100,
                         bool throwOnCancel = true, Interface* ip = GetCOREInterface() )
        : progress_logger( progressStart, progressEnd )
        , m_title( _T("") ) {
        m_message = message;
        m_ip = ip;
        m_ip->ProgressStart( const_cast<TCHAR*>( message.c_str() ), TRUE, detail::progress_dummy, NULL );
        m_going = true;
        m_throwOnCancel = throwOnCancel;
        // Update progress 10 times a second
        m_progressTimeout.restart_timeout( 100 );
    }

    /**
     * @param delay  time before the progress bar displays itself, measured in milliseconds.
     */
    max_progress_logger( const frantic::tstring& message, float progressStart = 0, float progressEnd = 100,
                         int delay = 0, bool throwOnCancel = true, Interface* ip = GetCOREInterface() )
        : progress_logger( progressStart, progressEnd )
        , m_title( _T("") ) {
        m_message = message;
        m_ip = ip;
        m_delayTimeout.restart_timeout( ( delay >= 0 ) ? ( delay ) : 0 );
        m_throwOnCancel = throwOnCancel;
        // Update progress 10 times a second
        m_progressTimeout.restart_timeout( 100 );

        if( m_delayTimeout.timed_out() ) {
            m_ip->ProgressStart( const_cast<TCHAR*>( message.c_str() ), TRUE, detail::progress_dummy, NULL );
            m_going = true;
        } else {
            m_going = false;
        }
    }
    virtual ~max_progress_logger() { end(); }

    void update_progress( float progressPercent ) {
        if( m_going ) {
            if( progressPercent >= 100.f || m_progressTimeout.timed_out() ) {
                m_ip->ProgressUpdate( (int)get_adjusted_progress( progressPercent ), m_title.empty(),
                                      const_cast<TCHAR*>( m_title.c_str() ) );
                if( m_throwOnCancel && canceled() ) {
                    end();
                    throw frantic::logging::progress_cancel_exception( frantic::strings::to_string( m_message ) );
                }
                // Update progress 10 times a second
                m_progressTimeout.restart_timeout( 100 );
            }
        } else if( m_delayTimeout.timed_out() ) {
            m_ip->ProgressStart( const_cast<TCHAR*>( m_message.c_str() ), TRUE, detail::progress_dummy, NULL );
            m_going = true;
            update_progress( progressPercent );
        }
    }

    void update_progress( long long completed, long long maximum ) {
        if( m_going ) {
            if( maximum == 0 )
                update_progress( 0 );
            else
                update_progress( 100 * (float)completed / maximum );
        } else if( m_delayTimeout.timed_out() ) {
            m_ip->ProgressStart( const_cast<TCHAR*>( m_message.c_str() ), TRUE, detail::progress_dummy, NULL );
            m_going = true;
            update_progress( completed, maximum );
        }
    }

    void check_for_abort() {
        if( canceled() )
            throw frantic::logging::progress_cancel_exception( frantic::strings::to_string( m_message ) );
    }

    void set_title( const frantic::tstring& title ) { m_title = title; }

    bool canceled() { return m_ip->GetCancel() != 0; }

    void end() {
        if( m_going ) {
            m_ip->ProgressEnd();
            m_going = false;
        }
    }
};

class maxrender_progress_logger : public frantic::logging::render_progress_logger {
    frantic::diagnostics::timeout_tracker m_progressTimeout;
    RendProgressCallback* m_rendProgressCallback;
    bool m_throwOnCancel;
    frantic::tstring m_message;
    Bitmap* m_vfb;

  public:
    maxrender_progress_logger( RendProgressCallback* rendProgressCallback, const frantic::tstring& message,
                               Bitmap* vfb = 0, float progressStart = 0, float progressEnd = 100,
                               bool throwOnCancel = true )
        : render_progress_logger( progressStart, progressEnd )
        , m_rendProgressCallback( rendProgressCallback ) {
        m_message = message;
        if( m_rendProgressCallback ) {
            m_rendProgressCallback->SetTitle( message.c_str() );
            m_rendProgressCallback->Progress( 0, 1000 );
        }
        m_throwOnCancel = throwOnCancel;
        // Update progress 10 times a second
        m_progressTimeout.restart_timeout( 100 );
        m_vfb = vfb;
    }
    virtual ~maxrender_progress_logger() { end(); }

    void set_render_progress_callback( RendProgressCallback* rendProgressCallback ) {
        m_rendProgressCallback = rendProgressCallback;
        if( m_rendProgressCallback ) {
            m_rendProgressCallback->SetTitle( m_message.c_str() );
            m_rendProgressCallback->Progress( 0, 1000 );
        }
    }

    virtual void set_title( const frantic::tstring& title ) {
        if( m_rendProgressCallback )
            m_rendProgressCallback->SetTitle( title.c_str() );
    }

    void update_progress( float progressPercent ) {
        if( m_rendProgressCallback && ( progressPercent >= 100.f || m_progressTimeout.timed_out() ) ) {
            bool cancel = m_rendProgressCallback->Progress( int( get_adjusted_progress( progressPercent ) * 10 ),
                                                            1000 ) == RENDPROG_ABORT;
            if( m_throwOnCancel && cancel ) {
                end();
                throw frantic::logging::progress_cancel_exception( frantic::strings::to_string( m_message ) );
            }
            // Update progress 10 times a second
            m_progressTimeout.restart_timeout( 100 );
        }
    }

    void update_progress( long long completed, long long maximum ) {
        if( m_rendProgressCallback ) {
            if( maximum == 0 )
                update_progress( 0 );
            else
                update_progress( 100 * (float)completed / maximum );
        }
    }

    void check_for_abort() {
        if( canceled() )
            throw frantic::logging::progress_cancel_exception( frantic::strings::to_string( m_message ) );
    }

    bool canceled() {
        // BUG: This changes the progress potentially .... Is it a good idea? Could Interface::CheckForRenderAbort() be
        // better?
        if( m_rendProgressCallback )
            return m_rendProgressCallback->Progress( 0, 0 ) == RENDPROG_ABORT;
        else
            return false; // Changed to false, because an invalid m_rendProgressCallback does not mean we should cancel,
                          // it just means there is no GUI progress logger.
    }

    void end() { m_rendProgressCallback = 0; }

    // Frame buffer update functions
    void update_frame_buffer( frantic::graphics2d::framebuffer<frantic::graphics::color6f>& buffer ) {
        if( m_vfb ) {
            // Copy the buffer to the bitmap
            buffer.to_3dsMaxBitmap( m_vfb );
            m_vfb->RefreshWindow();
            // Call the progress function to trigger the repaint
            if( m_rendProgressCallback )
                m_rendProgressCallback->Progress( 0, 0 );
        }
    }
};

} // namespace logging
} // namespace max3d
} // namespace frantic
