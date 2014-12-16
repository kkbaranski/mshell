/*//////////////////////////////////////////////////////////////////////
//                                                                    //
//                                  _          _ _                    //
//                                 | |        | | |                   //
//                    _ __ ___  ___| |__   ___| | |                   //
//                   | '_ ` _ \/ __| '_ \ / _ \ | |                   //
//                   | | | | | \__ \ | | |  __/ | |                   //
//                   |_| |_| |_|___/_| |_|\___|_|_|                   //
//                                                                    //
//                            ( reader.c )                            //
//                                                                    //
////////////////////////////////////////////////////////////////////////
///-------------- Copyright © 2014 Krzysztof Baranski  --------------///
//////////////////////////////////////////////////////////////////////*/

//==========================================================| INCLUDE
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include "bool.h"
#include "commons.h"
#include "config.h"
#include "debug.h"
#include "message.h"
#include "reader.h"
#include "signal_utils.h"

//==========================================================| VARIABLE
static volatile bool	__end_of_file;
static volatile bool	__ignore;
static volatile size_t	__line_size;

static char				__buffer[ MAX_LINE_LENGTH + 1 ];
static char				__line[ MAX_LINE_LENGTH + 1 ];

//==========================================================| FUNCTION
char* get_line() {
	logger log = { debug_function };
	log.debug( INFO(2), "> get_line()" );
	
	log.debug( INFO(3), "blocking SIGCHLD" );
	block_sigchld();
	
	log.debug( INFO(3), "loading line" );
	int status = load_line( STDIN_FILENO );
	
	if( ERROR( status ) ) {
		log.debug( INFO(3), "parsing line error" );
		error_message( SYNTAX_ERROR_STR );
		return NULL;
	}
	
	log.debug( INFO(3), "unblocking SIGCHLD" );
	unblock_sigchld();
	
	log.debug( INFO(3), "line = '%s'", __line );
	return __line;
}

void get_line_init() {
	logger log = { debug_function };
	log.debug( INFO(3), "> get_line_init()" );
	
	log.debug( INFO(6), "__buffer[0] = ETX" );
	__buffer[0] = ETX;
	
	log.debug( INFO(6), "__end_of_file = 0" );
    __end_of_file = 0;
}

bool is_end_of_file() {
	logger log = { debug_function };
	log.debug( INFO(3), "> is_end_of_file() -> %d", __end_of_file );
	
	return __end_of_file;
}

int load_line( int in_fileno ) {
	logger log = { debug_function };
	log.debug( INFO(3), "> load_line()" );
	
    char c;
    ssize_t returned_bytes;
    int cursor;
    __line_size = 0;
    cursor = -1;
    __ignore = false;
    log.debug( INFO(7), "__line_size = %d", __line_size );
    log.debug( INFO(7), "__ignore = %d", __ignore );
    log.debug( INFO(7), "cursor = %d", cursor );

	while( true ) {
        c = __buffer[ ++cursor ];
        log.debug( INFO(7), "char = '%c'", c );
        if( c == '\n' ) {
			log.debug( INFO(7), "new line char [ __ignore=%d ]", __ignore );
            __line[ __line_size ] = '\0';
            memmove( __buffer, __buffer+cursor+1, MAX_LINE_LENGTH-cursor+1 );
            break;
        } else if( c == ETX ) {
			log.debug( INFO(7), "end of text (ETX)" );
            cursor = -1;
            returned_bytes = read( in_fileno, __buffer, MAX_LINE_LENGTH );
            if(returned_bytes == 0) {
				log.debug( INFO(7), "end of file (EOF)" );
                __line[ __line_size ] = '\0';
                __end_of_file = true;
				break;
			}
			__buffer[ returned_bytes ] = ETX;
		} else {
			log.debug( INFO(7), "non-special character" );
			if( __ignore ) {
				log.debug( INFO(7), "ignored" );
				continue;
			} else if( __line_size >= MAX_LINE_LENGTH ) {
				log.debug( INFO(7), "[ %d ] line_size >= MAX_LINE_LENGTH [ %d ]", __line_size, MAX_LINE_LENGTH );
				__line_size = 0;
				__ignore = true;
			} else {
				log.debug( INFO(7), "char '%c' saved at index %d", c, __line_size );
				__line[ __line_size++ ] = c;
			}
		}
	}
	if( __ignore ) {
		log.debug( INFO(2), "line ignored" );
		return EXIT_ERROR;
	}
	
	log.debug( INFO(3), "loading successful" );
	return EXIT_SUCCESS;
}
