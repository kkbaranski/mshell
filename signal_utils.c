/*//////////////////////////////////////////////////////////////////////
//                                                                    //
//                                  _          _ _                    //
//                                 | |        | | |                   //
//                    _ __ ___  ___| |__   ___| | |                   //
//                   | '_ ` _ \/ __| '_ \ / _ \ | |                   //
//                   | | | | | \__ \ | | |  __/ | |                   //
//                   |_| |_| |_|___/_| |_|\___|_|_|                   //
//                                                                    //
//                         ( signal_utils.c )                         //
//                                                                    //
////////////////////////////////////////////////////////////////////////
///-------------- Copyright © 2014 Krzysztof Baranski  --------------///
//////////////////////////////////////////////////////////////////////*/

//==========================================================| INCLUDE
#include <stdlib.h>
#include <sys/wait.h>
#include "bgproc_queue.h"
#include "commons.h"
#include "debug.h"
#include "process_manager.h"
#include "signal_utils.h"

//==========================================================| VARIABLE
sigset_t	__sigchld_old_mask;

//==========================================================| FUNCTION
void block_sigchld() {
	logger log = { debug_function };
	log.debug( INFO(4), "> block_sigchld()" );
	
	sigset_t chld_block_mask;
	sigemptyset( &chld_block_mask );
	sigaddset( &chld_block_mask, SIGCHLD );
	sigprocmask( SIG_BLOCK, &chld_block_mask, &__sigchld_old_mask );
}

void set_sigchld_default() {
	logger log = { debug_function };
	log.debug( INFO(5), "> set_sigchld_default()" );
	
	if( signal( SIGCHLD, SIG_DFL ) == SIG_ERR ) {
		log.debug( DEBUG_ERROR, "error: 'signal( SIGINT, SIG_DFL )'" );
		exit( EXIT_ERROR );
	}
}

void set_sigchld_handler() {
	logger log = { debug_function };
	log.debug( INFO(5), "> set_sigchld_handler()" );
	
	if( signal( SIGCHLD, sigchld_handler ) == SIG_ERR ) {
		log.debug( DEBUG_ERROR, "error: 'signal( SIGINT, sigchld_handler )'" );
		exit( EXIT_ERROR );
	}
}

void set_sigint_default() {
	logger log = { debug_function };
	log.debug( INFO(5), "> set_sigint_default()" );
	
	if( signal( SIGINT, SIG_DFL ) == SIG_ERR ) {
		log.debug( DEBUG_ERROR, "error: 'signal( SIGINT, SIG_DFL )'" );
		exit( EXIT_ERROR );
	}
}

void set_sigint_ignore() {
	logger log = { debug_function };
	log.debug( INFO(5), "> set_sigint_ignore()" );
	
	if( signal( SIGINT, SIG_IGN ) == SIG_ERR ) {
		log.debug( DEBUG_ERROR, "error: 'signal( SIGINT, SIG_IGN )'" );
		exit( EXIT_ERROR );
	}
}

void sigchld_handler( int signum ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> sigchld_handler()" );
	
	pid_t cpid;
	int cstatus;
	
	while( ( cpid = waitpid( -1, &cstatus, WNOHANG ) ) > 0 ) {
		log.debug( INFO(5), "WNOHANG pid = %d, status = %d", cpid, cstatus );
		if( is_fgproc( cpid ) ) {
			decrement_fgproc();
			unregister_proc( cpid );
		} else if( is_bgproc( cpid ) ) {
			set_status_proc( cpid, cstatus );
			add_bgproc_queue( cpid );
		} else {
			log.debug( DEBUG_ERROR, "process type = NONE [ cpid=%d, cstatus=%d ]", cpid, cstatus );
		}
	}
}

void unblock_sigchld() {
	logger log = { debug_function };
	log.debug( INFO(4), "> unblock_sigchld()" );
	
	sigset_t chld_block_mask;
	sigemptyset( &chld_block_mask );
	sigaddset( &chld_block_mask, SIGCHLD );
	sigprocmask( SIG_UNBLOCK, &chld_block_mask, NULL );
}
