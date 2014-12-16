/*//////////////////////////////////////////////////////////////////////
//                                                                    //
//                                  _          _ _                    //
//                                 | |        | | |                   //
//                    _ __ ___  ___| |__   ___| | |                   //
//                   | '_ ` _ \/ __| '_ \ / _ \ | |                   //
//                   | | | | | \__ \ | | |  __/ | |                   //
//                   |_| |_| |_|___/_| |_|\___|_|_|                   //
//                                                                    //
//                           ( builtins.c )                           //
//                                                                    //
////////////////////////////////////////////////////////////////////////
///-------------- Copyright © 2014 Krzysztof Baranski  --------------///
//////////////////////////////////////////////////////////////////////*/

//==========================================================| INCLUDE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <dirent.h>
#include "builtins.h"

//==========================================================| VARIABLE
builtin_pair builtins_table[] = {
	{ "exit",	&lexit },
	{ "lecho",	&lecho },
	{ "lcd",	&lcd },
	{ "lkill",	&lkill },
	{ "lls",	&lls },
	{ NULL, NULL }
};
//==========================================================| FUNCTION
int lcd(char * argv[]) {
    if( argv[0] == NULL ) return -1;
    if( argv[1] == NULL ) return chdir( getenv( "HOME" ) );
    else if( argv[2] != NULL ) return -1;
    return chdir( argv[1] );
}

int lecho( char * argv[]) {
	int i =1;
	if (argv[i]) printf("%s", argv[i++]);
	while  (argv[i])
		printf(" %s", argv[i++]);

	printf("\n");
	fflush(stdout);
	return 0;
}

int lexit(char * argv[]) {
    if( argv[0] == NULL || argv[1] != NULL ) return -1;
    exit(0);
}

int lkill(char * argv[]) {
    int signal_number = SIGTERM;
    pid_t pid;
    
    if( argv[0] == NULL || argv[1] == NULL ) return -1;
    if( argv[1][0] == '-' ) {
        if( argv[2] == NULL || argv[3] != NULL ) return -1;
        signal_number = atoi( argv[1] + 1 );
        pid = atoi( argv[2] );
    } else {
        if( argv[2] != NULL ) return -1;
        pid = atoi( argv[1] );
    }
    return kill( pid, signal_number );
}

int lls(char * argv[]) {
    if( argv[0] == NULL || argv[1] != NULL ) return -1;
    DIR *dp;
    struct dirent *ep;     
    
    dp = opendir (".");
    if( dp == NULL ) return -1;
    while(( ep = readdir (dp) )) if( ep->d_name[0] != '.' )
        printf( "%s\n", ep->d_name );

    (void) closedir (dp);
    return EXIT_SUCCESS;
}

int undefined(char * argv[]) {
	fprintf(stderr, "Command %s undefined.\n", argv[0]);
	return BUILTIN_ERROR;
}
