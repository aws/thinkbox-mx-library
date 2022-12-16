// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <boost/function.hpp>
#include <boost/regex.hpp>
#include <frantic/logging/logging_level.hpp>
#include <frantic/win32/log_window.hpp>
#include <frantic/win32/utility.hpp>
#include <fstream>
#include <map>
#include <maxscrpt/streams.h>
#include <set>
#include <strbasic.h>

namespace frantic {
namespace max3d {
namespace logging {

class max_log_window;

class max_log_stream : public CharStream {
    frantic::max3d::logging::max_log_window* m_window;
    std::stringstream m_buffer;
    // allow us to reuse the stream for different debug levels.
    int m_log_level;
    std::string m_log_tag;

  public:
// character types changed between max 8 and 9
#ifdef MCHAR
    typedef MCHAR char_t;
#else
    typedef TCHAR char_t;
#endif

    max_log_stream( frantic::logging::logging_level level, max_log_window* mw, std::string const& tag = "" )
        : m_window( mw )
        , m_log_level( level )
        , m_log_tag( tag ) {}
    // 100 is a
    max_log_stream( std::string const& tag, max_log_window* mw )
        : m_window( mw )
        , m_log_level( frantic::logging::LOG_CUSTOM )
        , m_log_tag( tag ) {}
    virtual ~max_log_stream() {}

    // void set_log_level(int level){m_log_level = level;}
    int log_level() { return m_log_level; }

    /* internal MCHAR stream protocol */
    char_t get_char() { return 0; }
    void unget_char( char_t ) {}
    char_t peek_char() { return 0; }
    int at_eos() { return TRUE; }
    void rewind() {}
    void flush_to_eol() {}
    void flush_whitespace() {}

    void collect(); // m_window->log_message("collect called()";}
    // function implementations at the bottom of this file.
    virtual char_t putch( char_t c );
    virtual char_t* puts( char_t* str );
    virtual int printf( const char_t* format, ... );
    virtual void flush();
};

class max_log_window : public fpwrapper::FFIObject<max_log_window> {

    std::map<std::string, CharStream*> m_customTags;
    bool m_doLogPopups;
    bool m_logToFile;
    std::string m_logFile;
    frantic::win32::log_window m_window;

    // regex/callback pairs.
    // if a regex matches a log message, the callback maxscript is executed.
    std::map<std::string, std::string> m_callbackScripts;
    // to avoid infinite recursion, disable callback processing while processing callbacks.
    bool m_processingCallbacks;

    // streams that write to the window.
    // These are MXS Values so we'll allocate them with new
    // and allow the max garbage collector to delete them
    // later.
    max_log_stream* m_debug_stream;
    max_log_stream* m_error_stream;
    max_log_stream* m_warning_stream;
    max_log_stream* m_stats_stream;
    max_log_stream* m_progress_stream;
    max_log_stream* m_message_stream;

    void log_to_window_and_file( const std::string& msg ) {
        std::string message = "(" + boost::lexical_cast<std::string>( GetCurrentThreadId() ) + ") " + msg;
        if( m_logToFile ) {
            std::ofstream fout( m_logFile.c_str(), std::ios::out | std::ios::app );
            if( fout.is_open() ) {
                fout << message << "\n";
                fout.close();
            } else {
                m_logToFile = false;
                log_error( "Unable to open file \"" + m_logFile + "\" for writing.  File logging will be disabled." );
            }
        }

        // HACK TO STOP MT Logging errors
        if( GetCurrentThreadId() == GetWindowThreadProcessId( m_window.handle(), NULL ) )
            m_window.log( message );
    }

    void split_and_log_internal( const std::string& tag, const std::string& message ) {
        char splitChar = '\n';
        std::string::size_type index = 0, split_index;
        while( ( split_index = message.find( splitChar, index ) ) != std::string::npos ) {
            log_internal( tag + " " + message.substr( index, split_index - index ) );
            index = split_index + 1;
        }
        if( index < message.size() )
            log_internal( tag + " " + message.substr( index ) );
    }

    void log_internal( const std::string& message ) {
        log_to_window_and_file( frantic::strings::string_replace( message, "\t", "    " ) );
        // check the regular expression callbacks.
        if( !m_callbackScripts.empty() ) {
            if( !m_processingCallbacks ) {
                m_processingCallbacks = true;
                // test the message against each regex in our regex map.
                std::map<std::string, std::string>::iterator i = m_callbackScripts.begin(),
                                                             end = m_callbackScripts.end();
                std::vector<std::map<std::string, std::string>::iterator> to_erase;

                for( ; i != end; i++ ) {
                    try {
                        boost::smatch what;
                        if( boost::regex_match( message, what, boost::regex( i->first ) ) ) {
                            // in the case of a match, call the callback.
                            // create an mxs array of submatches.

                            boost::smatch::iterator m_i = what.begin(), m_end = what.end();
                            std::string submatch_arr = "#()";
                            if( m_i != m_end ) {
                                submatch_arr = "#(" + frantic::strings::get_quoted_string( *m_i );
                                m_i++;
                                for( ; m_i != m_end; m_i++ )
                                    submatch_arr = submatch_arr + ", " + frantic::strings::get_quoted_string( *m_i );
                                submatch_arr = submatch_arr + ")";
                            }
                            std::string callback = "(  local theMatches = " + submatch_arr + "; " + i->second + " ) ";

                            try {
                                mxs::expression( callback ).redirect_stdout( m_debug_stream ).evaluate<Value*>();
                            } catch( MAXScriptException& e ) {
                                if( frantic::logging::is_logging_errors() ) {
                                    log_to_window_and_file( "[ERR] Exception processing logging callback." );
                                    log_to_window_and_file( "[ERR]  exception:" );
                                    log_to_window_and_file( "[ERR]    " + mxs::to_string( e ) );
                                    log_to_window_and_file( "[ERR]  callback:" );
                                    log_to_window_and_file( "[ERR]    " + callback );
                                    log_to_window_and_file( "[ERR]  matching regex: " );
                                    log_to_window_and_file( "[ERR]    " + i->first );
                                    log_to_window_and_file( "[ERR]  regex will be removed from callbacks" );
                                }
                                to_erase.push_back( i );
                            }
                        }
                    } catch( const std::exception& e ) {
                        if( frantic::logging::is_logging_errors() ) {
                            log_to_window_and_file( "[ERR] Exception processing logging callback." );
                            log_to_window_and_file( "[ERR]  exception:" );
                            log_to_window_and_file( "[ERR]    " + std::string( e.what() ) );
                            log_to_window_and_file( "[ERR]  matching regex: " );
                            log_to_window_and_file( "[ERR]    " + i->first );
                            log_to_window_and_file( "[ERR]  regex will be removed from callbacks" );
                        }
                        to_erase.push_back( i );
                    }
                } // end for each callback
                for( unsigned j = 0; j < to_erase.size(); j++ )
                    m_callbackScripts.erase( to_erase[j] );

                m_processingCallbacks = false;
            } // if not processing callbacks.
        }
    }

  public:
    max_log_window( const std::string& windowName )
        : m_window( windowName, GetCOREInterface()->GetMAXHWnd() )
        , m_doLogPopups( false )
        , m_logToFile( false )
        , m_processingCallbacks( false ) {
        GetCOREInterface()->RegisterDlgWnd( m_window.handle() );
        using namespace frantic::logging;
        // allocate the maxscript streams.
        // and protect them from garbage collection.
        m_debug_stream = new max_log_stream( LOG_DEBUG, this );
        m_debug_stream = static_cast<max_log_stream*>( m_debug_stream->make_heap_permanent() );
        m_stats_stream = new max_log_stream( LOG_STATS, this );
        m_stats_stream = static_cast<max_log_stream*>( m_stats_stream->make_heap_permanent() );
        m_progress_stream = new max_log_stream( LOG_PROGRESS, this );
        m_progress_stream = static_cast<max_log_stream*>( m_progress_stream->make_heap_permanent() );
        m_warning_stream = new max_log_stream( LOG_WARNINGS, this );
        m_warning_stream = static_cast<max_log_stream*>( m_warning_stream->make_heap_permanent() );
        m_error_stream = new max_log_stream( LOG_ERRORS, this );
        m_error_stream = static_cast<max_log_stream*>( m_error_stream->make_heap_permanent() );
        m_message_stream = new max_log_stream( LOG_NONE, this );
        m_message_stream = static_cast<max_log_stream*>( m_message_stream->make_heap_permanent() );

        // Set up the maxscript interface.
        FFCreateDescriptor c( this, Interface_ID( 0x25c65470, 0x267d6823 ), "FranticLogWindow", 0 );

        // file logging
        c.add_property( &max_log_window::get_log_to_file, &max_log_window::log_to_file, "LogToFile" );
        c.add_property( &max_log_window::get_log_file, &max_log_window::set_log_file, "LogFile" );

        // visibility functions.
        c.add_property( &max_log_window::is_visible, &max_log_window::show, "Visible" );
        c.add_property( &max_log_window::get_popup_on_message, &max_log_window::popup_on_message,
                        "PopupLogWindowOnMessage" );

        // logging functions.
        c.add_function( &max_log_window::log_error, "ErrorMsg", "Message" );
        c.add_function( &max_log_window::log_warning, "WarningMsg", "Message" );
        c.add_function( &max_log_window::log_progress, "ProgressMsg", "Message" );
        c.add_function( &max_log_window::log_stats, "StatsMsg", "Message" );
        c.add_function( &max_log_window::log_debug, "DebugMsg", "Message" );
        c.add_function( &max_log_window::log_message, "LogMsg", "Message" );
        c.add_function( &max_log_window::log_generic, "Log", "Tag", "Message" );

        // special logging tags
        c.add_function( &max_log_window::enable_logging_tag, "EnableLoggingTag", "Tag" );
        c.add_function( &max_log_window::disable_logging_tag, "DisableLoggingTag", "Tag" );
        c.add_property( &max_log_window::logging_tags, "LoggingTags" );
        c.add_function( &max_log_window::clear_logging_tags, "ClearLoggingTags" );

        c.add_property( &max_log_window::logging_level_string, "LoggingLevelString" );
        c.add_property( &max_log_window::get_logging_level, &max_log_window::set_logging_level, "LoggingLevel" );

        c.add_function( &max_log_window::add_callback, "AddCallback", "Regex", "Script" );
        c.add_function( &max_log_window::remove_callback, "RemoveCallback", "Regex" );
        c.add_property( &max_log_window::callbacks, "Callbacks" );
        c.add_function( &max_log_window::clear_callback_scripts, "ClearCallbacks" );

        c.add_property( &max_log_window::debug_stream_value, "Debug" );
        c.add_property( &max_log_window::error_stream_value, "Error" );
        c.add_property( &max_log_window::progress_stream_value, "Progress" );
        c.add_property( &max_log_window::warning_stream_value, "Warning" );
        c.add_property( &max_log_window::stats_stream_value, "Stats" );
        c.add_property( &max_log_window::message_stream_value, "Message" );
        c.add_function( &max_log_window::generic_stream_value, "Tag", "LoggingTag" );
    }

    ~max_log_window() {
        // release the Value * objects.
        m_debug_stream->make_collectable();
        m_stats_stream->make_collectable();
        m_progress_stream->make_collectable();
        m_warning_stream->make_collectable();
        m_error_stream->make_collectable();
        m_message_stream->make_collectable();
    }

    bool is_visible() { return m_window.is_visible(); }
    void show( bool visible = true ) { m_window.show( visible ); }

    // Override deletion
    // to avoid the max garbage collector.
    void DeleteIObject(){};

    //		CharStream * stream(){return &m_stream;}
    //		Value * stream_value(){return &m_stream;}

    Value* debug_stream_value() { return debug_stream(); }
    CharStream* debug_stream() { return m_debug_stream; }

    Value* error_stream_value() { return error_stream(); }
    CharStream* error_stream() { return m_error_stream; }

    Value* warning_stream_value() { return warning_stream(); }
    CharStream* warning_stream() { return m_warning_stream; }

    Value* progress_stream_value() { return progress_stream(); }
    CharStream* progress_stream() { return m_progress_stream; }

    Value* stats_stream_value() { return stats_stream(); }
    CharStream* stats_stream() { return m_stats_stream; }

    Value* message_stream_value() { return message_stream(); }
    CharStream* message_stream() { return m_message_stream; }

    Value* generic_stream_value( std::string const& stream_tag ) { return generic_stream( stream_tag ); }

    CharStream* generic_stream( std::string const& stream_tag ) {
        // This returns the charstream associated with a particular tag
        std::string tag = frantic::strings::to_lower( stream_tag );
        if( is_logging( tag ) )
            return m_customTags[tag];
        else
            return m_debug_stream;
    }
    // register a regex/callback script pair. If a regex matches
    // the log message, the callback script is called. The log message
    // that matched the script will be visible to the callback script
    // as theMessage
    void add_callback( std::string const& regex, std::string const& mxsScript ) {
        m_callbackScripts[regex] = mxsScript;
    }
    void remove_callback( std::string const& regex ) { m_callbackScripts.erase( regex ); }
    const std::map<std::string, std::string>& callbacks() { return m_callbackScripts; }
    void clear_callback_scripts() { m_callbackScripts.clear(); }

    std::string logging_level_string() { return frantic::logging::get_logging_level_string(); }
    void set_logging_level( int level ) { frantic::logging::set_logging_level( level ); }
    int get_logging_level() { return frantic::logging::get_logging_level(); }

    void enable_logging_tag( const std::string& thetag ) {
        std::string tag = frantic::strings::to_lower( thetag );
        if( !is_logging( tag ) ) {
            CharStream* stream = new max_log_stream( tag, this );
            stream = static_cast<max_log_stream*>( stream->make_heap_permanent() );
            m_customTags[tag] = stream;
        }
    }
    void disable_logging_tag( const std::string& theTag ) {
        std::string tag = frantic::strings::to_lower( theTag );
        if( is_logging( tag ) ) {
            m_customTags[tag]->make_collectable();
            m_customTags.erase( tag );
        }
    }
    const std::vector<std::string> logging_tags() {
        std::vector<std::string> ret;
        std::map<std::string, CharStream*>::iterator i = m_customTags.begin(), end = m_customTags.end();
        for( ; i != end; i++ )
            ret.push_back( i->first );
        return ret;
    }
    void clear_logging_tags() { m_customTags.clear(); }

    void log_to_file( bool log ) { m_logToFile = log; }
    bool get_log_to_file() { return m_logToFile; }
    std::string get_log_file() { return m_logFile; }
    void set_log_file( const std::string& file ) { m_logFile = file; }

    void popup_on_message( bool popup ) { m_doLogPopups = popup; }
    bool get_popup_on_message() { return m_doLogPopups; }

    // Logging functions.
    void log_error( const std::string& message ) {
        if( frantic::logging::is_logging_errors() ) {
            if( m_doLogPopups )
                show();
            split_and_log_internal( "[ERR]", message );
        }
    }

    void log_warning( const std::string& message ) {
        if( frantic::logging::is_logging_warnings() ) {
            if( m_doLogPopups )
                show();
            split_and_log_internal( "[WRN]", message );
        }
    }

    void log_progress( const std::string& message ) {
        if( frantic::logging::is_logging_progress() ) {
            if( m_doLogPopups )
                show();
            split_and_log_internal( "[PRG]", message );
        }
    }

    void log_stats( const std::string& message ) {
        if( frantic::logging::is_logging_stats() ) {
            if( m_doLogPopups )
                show();
            split_and_log_internal( "[STS]", message );
        }
    }

    void log_debug( const std::string& message ) {
        if( frantic::logging::is_logging_debug() ) {
            if( m_doLogPopups )
                show();
            split_and_log_internal( "[DBG]", message );
        }
    }

    void log_message( const std::string& message ) {
        if( m_doLogPopups )
            show();
        split_and_log_internal( "[MSG]", message );
    }

    bool is_logging( std::string const& theTag ) {
        return m_customTags.find( frantic::strings::to_lower( theTag ) ) != m_customTags.end();
    }

    void log_generic( const std::string& theTag, const std::string& message ) {
        std::string tag = frantic::strings::to_lower( theTag );
        if( is_logging( tag ) ) {
            if( m_doLogPopups )
                show();
            split_and_log_internal( "[" + tag + "]", message );
        }
    }
};

inline max_log_stream::char_t max_log_stream::putch( max_log_stream::char_t c ) {
    // m_window->log_message("max_log_stream::putch ");
    if( c == '\n' ) {
        flush();
    } else {
        m_buffer << c;
    }

    return c;
}
inline max_log_stream::char_t* max_log_stream::puts( max_log_stream::char_t* str ) {
    if( frantic::logging::get_logging_level() >= m_log_level ) {
        for( unsigned i = 0; str[i] != 0; i++ )
            putch( str[i] );
    }
    return str;
}

inline int max_log_stream::printf( const max_log_stream::char_t* format, ... ) {
    if( frantic::logging::get_logging_level() >= m_log_level ) {
        va_list args;
        int len;
        boost::shared_array<char> buffer;

        va_start( args, format );
        len = _vscprintf( format, args ) // _vscprintf doesn't count
              + 1;                       // terminating '\0'
        buffer.reset( new char[len * sizeof( char )] );
        vsprintf( buffer.get(), format, args );
        puts( buffer.get() );
    }
    return 1;
}

inline void max_log_stream::flush() {
    using namespace frantic::logging;
    if( frantic::logging::get_logging_level() >= m_log_level && !m_buffer.str().empty() ) {
        std::string prefix = m_log_tag.empty() ? "" : "[" + m_log_tag + "]";

        // flush the buffer.
        switch( m_log_level ) {
        case LOG_CUSTOM:
            m_window->log_generic( m_log_tag, m_buffer.str() );
            break;
        case LOG_NONE:
            m_window->log_message( prefix + m_buffer.str() );
            break;
        case LOG_ERRORS:
            m_window->log_error( prefix + m_buffer.str() );
            break;
        case LOG_WARNINGS:
            m_window->log_warning( prefix + m_buffer.str() );
            break;
        case LOG_PROGRESS:
            m_window->log_progress( prefix + m_buffer.str() );
            break;
        case LOG_STATS:
            m_window->log_stats( prefix + m_buffer.str() );
            break;
        default:
            m_window->log_debug( prefix + m_buffer.str() );
            break;
        }
    }
    m_buffer.str( std::string() );
}

inline void max_log_stream::collect() { /*mprintf("Deleting a log stream\n");*/
    delete this;
}

} // namespace logging
} // namespace max3d
} // namespace frantic
