// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/lexical_cast.hpp>
#include <frantic/diagnostics/timeout_tracker.hpp>
#include <frantic/logging/progress_logger.hpp>
#include <frantic/strings/tstring.hpp>
#include <iomanip>
#include <string>

#pragma warning( push, 3 )
#include <maxapi.h>
#include <maxtypes.h>
#pragma warning( pop )

namespace frantic {
namespace max3d {
namespace logging {

/**
 * status_panel_progress_logger
 *
 * Adds functionality for progress to be displayed as a string
 * message in the 3dsmax status panel. Also will send a progressCancel
 * exception if the [Esc] keyboard button is pressed. This is only
 * detected on the next call to update_progress.
 */
class status_panel_progress_logger : public frantic::logging::progress_logger {
  private:
    frantic::diagnostics::timeout_tracker m_progressTimeout;
    frantic::diagnostics::timeout_tracker m_delayTimeout;
    std::stringstream m_floatConverter;
    frantic::tstring m_msgStart;
    frantic::tstring m_msgEnd;
    float m_lastProgress;
    bool m_going;

    int m_tickState;
    bool m_showPercentage;

    /**
     * Takes a progress percent and converts it into a string with a
     * beginning and ending message
     *
     * @param	progress  the progress percent to convert
     * @return		      the formatted message
     */
    frantic::tstring generate_output( float progress ) {
        if( m_showPercentage ) {
            frantic::tstring percentStr;

            m_floatConverter << get_adjusted_progress( progress );
            percentStr = frantic::strings::to_tstring( m_floatConverter.str() );
            m_floatConverter.str( "" );

            return m_msgStart + _T(" ") + percentStr + m_msgEnd;
        } else {
            const int STRING_SIZE = 10;

            int i;
            TCHAR progressString[STRING_SIZE + 1];

            for( i = 0; i < m_tickState; ++i )
                progressString[i] = '.';
            for( ; i < STRING_SIZE; ++i )
                progressString[i] = ' ';
            progressString[STRING_SIZE] = '\0';

            m_tickState = ( m_tickState + 1 ) % STRING_SIZE;
            return m_msgStart + _T(" Press [Esc] to cancel ") + progressString;
        }
    }

    inline void do_update_progress() {
        if( !m_going ) {
            if( !m_delayTimeout.timed_out() )
                return;

            m_going = true;
            GetCOREInterface()->PushPrompt( const_cast<TCHAR*>( generate_output( m_lastProgress ).c_str() ) );

            // Update progress 10 times a second
            m_progressTimeout.restart_timeout( 100 );
        } else if( is_esc_pressed() ) {
            end();
            throw frantic::logging::progress_cancel_exception( "Cancel message sent" );
        } else if( m_lastProgress >= 100.f || m_progressTimeout.timed_out() ) {
            GetCOREInterface()->ReplacePrompt( const_cast<TCHAR*>( generate_output( m_lastProgress ).c_str() ) );

            // Update progress 10 times a second
            m_progressTimeout.restart_timeout( 100 );
        }
    }

  protected:
    /**
     * Checks the windows message queue for an [Esc] key press message
     *
     * @return	pressed	true if the [Esc] key was pressed inbetween the last
     *					calling of this method, false otherwise
     */
    virtual bool is_esc_pressed() {
        bool pressed = false;

        MSG lpMsg;
        UINT wMsgFilterMin = WM_KEYFIRST;
        UINT wMsgFilterMax = WM_KEYLAST;
        UINT wRemoveMsg = PM_QS_INPUT | PM_REMOVE;

        // checks if there are any new keyboard or mouse messages
        if( GetInputState() ) {
            // check for esc key presses
            while( PeekMessage( &lpMsg, NULL, wMsgFilterMin, wMsgFilterMax, wRemoveMsg ) ) {
                if( lpMsg.message == WM_KEYDOWN ) {
                    if( lpMsg.wParam == VK_ESCAPE )
                        pressed = true;

                } else if( lpMsg.message == WM_QUIT ) {
                    PostQuitMessage( static_cast<int>( lpMsg.wParam ) );
                    pressed = true;
                }
            }
        }

        return pressed;
    }

  public:
    /**
     * Constructs a status_panel_progress_logger object with defalt values:
     * progressStart  = 0
     * progressEnd    = 100
     * delay          = 0
     * msgStart       = "Progress: "
     * showPercentage = true
     *
     * @param	progressStart	the initial progress percent
     * @param	progressEnd		the maximum allowed progress percent
     * @param	delay			time before the progress bar displays itself, measured in milliseconds.
     * @param	msgStart		the string to be displayed before the progress percentage on the status panel
     * @param	showPercentage	show the progress percentage in the status message
     */
    status_panel_progress_logger( float progressStart = 0, float progressEnd = 100, int delay = 0,
                                  const frantic::tstring& msgStart = _T("Progress:"), bool showPercentage = true )
        : progress_logger( progressStart, progressEnd )
        , m_msgStart( msgStart )
        , m_lastProgress( 0.f )
        , m_tickState( 0 )
        , m_showPercentage( showPercentage ) {
        m_floatConverter << std::fixed << std::setprecision( 2 );
        m_msgEnd = _T(" % completed.    Press [Esc] to cancel.");
        m_going = false;
        m_delayTimeout.restart_timeout( ( delay >= 0 ) ? ( delay ) : 0 );

        do_update_progress();
    }

    /**
     * Destructor, calls the end method
     */
    virtual ~status_panel_progress_logger() { end(); }

    /**
     * Updates the progress display with the passed percentage as a floating point number
     *
     * @param	progressPercent	the progress percent to update the display with
     */
    void update_progress( float progressPercent ) {
        m_lastProgress = progressPercent;
        do_update_progress();
    }

    /**
     * Updates the progress display with the percentage as a floating point number
     * calculated as 100 * (float)completed / maximum
     *
     * @param	completed	a number representing the amount of completion of a task
     * @param	maximum		a number representing the maximum amount of completion of that task
     */
    void update_progress( long long completed, long long maximum ) {
        m_lastProgress = ( maximum == 0 ) ? 0.f : 100.f * (float)completed / maximum;
        do_update_progress();
    }

    void check_for_abort() {
        // Don't change the progress level, just update with the previous value. This doesn't
        // just check for escape pressed in order to force the prompt to be updated if the
        // title has been changed.
        do_update_progress();
    }

    /**
     * If the progress bar has not yet been cleaned up with this method,
     * removes the prompt from the max status panel stack
     */
    void end() {
        if( m_going ) {
            GetCOREInterface()->PopPrompt();
            m_going = false;
        }
    }

    /**
     * Set the beginning of the status panel message to the specified title.
     *
     * @param	title	a title string
     */
    void set_title( const frantic::tstring& title ) { m_msgStart = title; }
};

} // namespace logging
} // namespace max3d
} // namespace frantic
