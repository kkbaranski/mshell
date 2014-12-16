/*//////////////////////////////////////////////////////////////////////
//                                                                    //
//                                  _          _ _                    //
//                                 | |        | | |                   //
//                    _ __ ___  ___| |__   ___| | |                   //
//                   | '_ ` _ \/ __| '_ \ / _ \ | |                   //
//                   | | | | | \__ \ | | |  __/ | |                   //
//                   |_| |_| |_|___/_| |_|\___|_|_|                   //
//                                                                    //
//                             ( pipe.c )                             //
//                                                                    //
////////////////////////////////////////////////////////////////////////
///-------------- Copyright © 2014 Krzysztof Baranski  --------------///
//////////////////////////////////////////////////////////////////////*/

//==========================================================| INCLUDE
#include <unistd.h>
#include "pipe.h"
#include "debug.h"

//==========================================================| FUNCTION
void close_in( pipe_t* p ) {
	logger log = { debug_function };
	log.debug( INFO(5), "> close_in-descriptor()" );
	log.debug( INFO(6), "pipe( in=%d, out=%d )", PIPE_IN( *p ), PIPE_OUT( *p ) );
	
	if( PIPE_IN( *p ) <= 2 ) return;
	
	log.debug( INFO(6), "closing desriptor %d", PIPE_IN( *p ) );
	close( PIPE_IN( *p ) );
	PIPE_IN( *p ) = STDIN_FILENO;
	
	log.debug( INFO(6), "pipe( in=%d, out=%d )", PIPE_IN( *p ), PIPE_OUT( *p ) );
}

void close_out( pipe_t* p ) {
	logger log = { debug_function };
	log.debug( INFO(5), "> close_out-descriptor()" );
	log.debug( INFO(6), "pipe( in=%d, out=%d )", PIPE_IN( *p ), PIPE_OUT( *p ) );
	
	if( PIPE_OUT( *p ) <= 2 ) return;
	
	log.debug( INFO(6), "closing desriptor %d", PIPE_OUT( *p ) );
	close( PIPE_OUT( *p ) );
	PIPE_OUT( *p ) = STDOUT_FILENO;
	
	log.debug( INFO(6), "pipe( in=%d, out=%d )", PIPE_IN( *p ), PIPE_OUT( *p ) );
}

void close_pipe( pipe_t* p ) {
	logger log = { debug_function };
	log.debug( INFO(5), "> close_pipe()" );
	
	close_in( p );
	close_out( p );
}

pipe_t new_pipe() {
	logger log = { debug_function };
	log.debug( INFO(5), "> new_pipe()" );
	
	pipe_t p;
	pipe( p.fd );
	
	log.debug( INFO(6), "return pipe( in=%d, out=%d )", PIPE_IN( p ), PIPE_OUT( p ) );
	
	return p;
}

void replace_fd( int a, int b ) {
	logger log = { debug_function };
	log.debug( INFO(5), "> replace_file_descriptors()" );
	log.debug( INFO(6), "replacing [ %d -> %d ]", a, b );
	
	if( a == b ) return;
	log.debug( INFO(6), "closing descriptor %d", a );
	close(a);
	log.debug( INFO(6), "copying descriptor %d into %d", b, a );
	dup2(b, a);
	log.debug( INFO(6), "closing descriptor %d", b );
	close(b);
}

pipe_t standard_pipe() {
	logger log = { debug_function };
	log.debug( INFO(5), "> standard_pipe()" );
	
	pipe_t p;
	PIPE_IN( p ) = STDIN_FILENO;
	PIPE_OUT( p ) = STDOUT_FILENO;
	
	log.debug( INFO(6), "return pipe( in=%d, out=%d )", PIPE_IN( p ), PIPE_OUT( p ) );
	
	return p;
}
