/*//////////////////////////////////////////////////////////////////////
//                                                                    //
//                                  _          _ _                    //
//                                 | |        | | |                   //
//                    _ __ ___  ___| |__   ___| | |                   //
//                   | '_ ` _ \/ __| '_ \ / _ \ | |                   //
//                   | | | | | \__ \ | | |  __/ | |                   //
//                   |_| |_| |_|___/_| |_|\___|_|_|                   //
//                                                                    //
//                           ( message.c )                            //
//                                                                    //
////////////////////////////////////////////////////////////////////////
///-------------- Copyright © 2014 Krzysztof Baranski  --------------///
//////////////////////////////////////////////////////////////////////*/

//==========================================================| INCLUDE
#include <errno.h>
#include <stdio.h>
#include "debug.h"
#include "message.h"

//==========================================================| VARIABLE

//==========================================================| FUNCTION
void builtin_error( char* command_name ) {
	logger log = { debug_function };
	log.debug( INFO(3), "> builtin_error( %s )", command_name );
	
    fprintf( stderr, "Builtin %s error.\n", command_name );
}

void error_message( const char* message ) {
	fprintf( stderr, "%s\n", message );
}

void error_named_message( const char* name, const char* message ) {
	fprintf( stderr, "%s: %s\n", name, message );
}

int handle_error( int no, char* subject ) {
	logger log = { debug_function };
	log.debug( INFO(3), "> handle_error( %d, %s )", no, subject );
	
	if( !no ) { return 0; }

	switch( errno ) {
	case EACCES:
		log.debug( INFO(4), "EACCES" );
		fprintf( stderr, "%s: permission denied\n", subject );
		break;
	case ENOENT:
		log.debug( INFO(4), "ENOENT" );
		fprintf( stderr, "%s: no such file or directory\n", subject );
		break;
	case ENOEXEC:
		log.debug( INFO(4), "ENOEXEC" );
		fprintf( stderr, "%s: exec error\n", subject );
		break;
	}
	return no;
}
