/*//////////////////////////////////////////////////////////////////////
//                                                                    //
//                                  _          _ _                    //
//                                 | |        | | |                   //
//                    _ __ ___  ___| |__   ___| | |                   //
//                   | '_ ` _ \/ __| '_ \ / _ \ | |                   //
//                   | | | | | \__ \ | | |  __/ | |                   //
//                   |_| |_| |_|___/_| |_|\___|_|_|                   //
//                                                                    //
//                       ( process_manager.c )                        //
//                                                                    //
////////////////////////////////////////////////////////////////////////
///-------------- Copyright © 2014 Krzysztof Baranski  --------------///
//////////////////////////////////////////////////////////////////////*/

//==========================================================| INCLUDE
#include "process_manager.h"
#include "debug.h"

//==========================================================| VARIABLE
static process		__processes[ PID_MAX + 1 ];
static size_t		__fgproc_counter;

//==========================================================| FUNCTION
size_t count_fgproc() {
	logger log = { debug_function };
	log.debug( INFO(7), "> count_fgproc() -> %d", __fgproc_counter );
	return __fgproc_counter;
}

void decrement_fgproc() {
	logger log = { debug_function };
	log.debug( INFO(4), "> decrement_fgproc() %d -> %d", __fgproc_counter, __fgproc_counter-1 );
	if( __fgproc_counter == 0 ) log.debug( DEBUG_ERROR, "fgproc_counter == 0" );
	--__fgproc_counter;
}

process* get_proc( pid_t pid ) {
	logger log = { debug_function };
	log.debug( INFO(7), "> get_proc()" );
	
	return __processes + pid;
}

int get_status_proc( pid_t pid ) {
	logger log = { debug_function };
	log.debug( INFO(6), "> get_status_proc() -> %d", get_proc( pid )->status );
	
	return get_proc( pid )->status;
}

type_t get_type_proc( pid_t pid ) {
	logger log = { debug_function };
	log.debug( INFO(6), "> get_type_proc() -> %d", get_proc( pid )->type );
	
	return get_proc( pid )->type;
}

void increment_fgproc() {
	logger log = { debug_function };
	log.debug( INFO(4), "> increment_fgproc() %d -> %d", __fgproc_counter, __fgproc_counter+1 );
	++__fgproc_counter;
}

bool is_bgproc( pid_t pid ) {
	logger log = { debug_function };
	log.debug( INFO(6), "> is_bgproc() -> %d", ( get_type_proc( pid ) == BACKGROUND ) );
	
	return ( get_type_proc( pid ) == BACKGROUND );
}

bool is_fgproc( pid_t pid ) {
	logger log = { debug_function };
	log.debug( INFO(6), "> is_fgproc() -> %d", ( get_type_proc( pid ) == FOREGROUND ) );
	
	return ( get_type_proc( pid ) == FOREGROUND );
}

void register_bgproc( pid_t pid ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> register_bgproc( %d )", pid );
	
	__processes[ pid ].type = BACKGROUND;
	
	if( !is_bgproc( pid ) ) log.debug( DEBUG_ERROR, "process (%d) is not registered as BACKGROUND after register", pid );
}

void register_fgproc( pid_t pid ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> register_fgproc( %d )", pid );
	
	__processes[ pid ].type = FOREGROUND;
	
	if( !is_fgproc( pid ) ) log.debug( DEBUG_ERROR, "process (%d) is not registered as FOREGROUND after register", pid );
}

void reset_fgproc_counter() {
	logger log = { debug_function };
	log.debug( INFO(4), "> reset_fgproc_counter()" );
	__fgproc_counter = 0;
}

void set_status_proc( pid_t pid, int status ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> set_status_proc( %d, %d )", pid, status );
	
	__processes[ pid ].status = status;
}

void unregister_proc( pid_t pid ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> unregister_proc()" );
	
	__processes[ pid ].type = FOREGROUND;
}
