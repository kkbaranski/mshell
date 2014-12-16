/*//////////////////////////////////////////////////////////////////////
//                                                                    //
//                                  _          _ _                    //
//                                 | |        | | |                   //
//                    _ __ ___  ___| |__   ___| | |                   //
//                   | '_ ` _ \/ __| '_ \ / _ \ | |                   //
//                   | | | | | \__ \ | | |  __/ | |                   //
//                   |_| |_| |_|___/_| |_|\___|_|_|                   //
//                                                                    //
//                            ( config.c )                            //
//                                                                    //
////////////////////////////////////////////////////////////////////////
///-------------- Copyright © 2014 Krzysztof Baranski  --------------///
//////////////////////////////////////////////////////////////////////*/

//==========================================================| INCLUDE
#include <signal.h>
#include "bgproc_queue.h"
#include "commons.h"
#include "config.h"
#include "debug.h"
#include "process_manager.h"
#include "reader.h"
#include "signal_utils.h"

//==========================================================| VARIABLE

//==========================================================| FUNCTION
void shell_init() {
	logger log = { debug_function };
	log.debug( INFO(2), "> shell_init()" );
	
	int i, tmp_debug_level;
	static struct sigaction sigchld;
	
	log.debug( INFO(3), "ignoring SIGINT" );
	set_sigint_ignore();
	
	log.debug( INFO(3), "changing SIGCHLD handler" );
	set_sigchld_handler();
	
	log.debug( INFO(3), "get_line initializing" );
    get_line_init();
    
    log.debug( INFO(3), "resetting fgproc_counter" );
    reset_fgproc_counter();
    
    log.debug( INFO(3), "cleaning bgproc_queue" );
    clean_bgproc_queue();
    
    log.debug( INFO(3), "cleaning processes array" );
    tmp_debug_level = DEBUG_LEVEL;
    DEBUG_LEVEL = 0;
    for( i = 0; i <= PID_MAX; ++i ) unregister_proc( i );
    DEBUG_LEVEL = tmp_debug_level;
}
