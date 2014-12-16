/*//////////////////////////////////////////////////////////////////////
//                                                                    //
//                                  _          _ _                    //
//                                 | |        | | |                   //
//                    _ __ ___  ___| |__   ___| | |                   //
//                   | '_ ` _ \/ __| '_ \ / _ \ | |                   //
//                   | | | | | \__ \ | | |  __/ | |                   //
//                   |_| |_| |_|___/_| |_|\___|_|_|                   //
//                                                                    //
//                              ( debug.c )                           //
//                                                                    //
////////////////////////////////////////////////////////////////////////
///-------------- Copyright © 2014 Krzysztof Baranski  --------------///
//////////////////////////////////////////////////////////////////////*/

//==========================================================| INCLUDE
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "debug.h"

//==========================================================| FUNCTION
void debug_function( int level, char* fmt, ... ) {
	if( level > DEBUG_LEVEL ) return;
	
	int i;
	time_t rawtime;
	struct tm *info;
	char date_buffer[20];
	char time_buffer[20];

	time( &rawtime );
	info = localtime( &rawtime );

	strftime( date_buffer, 20, "%Y-%m-%d", info );
	strftime( time_buffer, 20, "%H:%M:%S", info );
	
	va_list argp;
	va_start( argp, fmt );
	
	fprintf( stderr, "[%s]", date_buffer );
	fprintf( stderr, "[%s]", time_buffer );
	fprintf( stderr, "[PID=%4d]", getpid() );
	fprintf( stderr, " " );
	if( level == DEBUG_ERROR ) fprintf( stderr, ANSI_COLOR_RED "ERROR! " ANSI_COLOR_RESET );
	for( i = 1; i < level; ++i ) fprintf( stderr, ".  " );
	vfprintf( stderr, fmt, argp );
	fprintf( stderr, "\n" );
}
