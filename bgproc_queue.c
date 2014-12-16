/*//////////////////////////////////////////////////////////////////////
//                                                                    //
//                                  _          _ _                    //
//                                 | |        | | |                   //
//                    _ __ ___  ___| |__   ___| | |                   //
//                   | '_ ` _ \/ __| '_ \ / _ \ | |                   //
//                   | | | | | \__ \ | | |  __/ | |                   //
//                   |_| |_| |_|___/_| |_|\___|_|_|                   //
//                                                                    //
//                         ( bgproc_queue.c )                         //
//                                                                    //
////////////////////////////////////////////////////////////////////////
///-------------- Copyright © 2014 Krzysztof Baranski  --------------///
//////////////////////////////////////////////////////////////////////*/

//==========================================================| INCLUDE
#include "bgproc_queue.h"
#include "debug.h"

//==========================================================| VARIABLE
static size_t	__bgproc_queue_size;
static pid_t	__bgproc_queue[ PID_MAX + 1 ];

//==========================================================| FUNCTION
void add_bgproc_queue( pid_t pid ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> add_bgproc_queue( %d )", pid );
	__bgproc_queue[ __bgproc_queue_size++ ] = pid;
}

void clean_bgproc_queue() {
	logger log = { debug_function };
	log.debug( INFO(4), "> clean_bgproc_queue()" );
	__bgproc_queue_size = 0;
}

size_t size_bgproc_queue() {
	logger log = { debug_function };
	log.debug( INFO(7), "> size_bgproc_queue() -> %d", __bgproc_queue_size );
	return __bgproc_queue_size;
}

pid_t get_bgproc_queue( int i ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> get_bgproc_queue( %d ) -> %d", i, __bgproc_queue[ i ] );
	return __bgproc_queue[ i ];
}
