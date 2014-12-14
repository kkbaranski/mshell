#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 
#include <fcntl.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>


#include "builtins.h"

//=====================================================================| FUNCTIONS
int _echo       (char*[]);
int _lcd        (char*[]);
int _lexit      (char*[]);
int _lkill      (char*[]);
int _lls        (char*[]);
int _lexit      (char*[]);

int _undefined  (char *[]);

builtin_pair builtins_table[]={
	{"exit",	&_lexit},
	{"lecho",	&_echo},
	{"lcd",	    &_lcd},
	{"lkill",	&_lkill},
	{"lls",		&_lls},
	{NULL,NULL}
};

//====================================================================//
//                                                                    //
//                             FUNCTIONS                              //
//                                                                    //
//====================================================================//
int _echo( char * argv[]) {
	int i =1;
	if (argv[i]) printf("%s", argv[i++]);
	while  (argv[i])
		printf(" %s", argv[i++]);

	printf("\n");
	fflush(stdout);
	return 0;
}

int _undefined(char * argv[]) {
	fprintf(stderr, "Command %s undefined.\n", argv[0]);
	return BUILTIN_ERROR;
}

///
/// @author krzysztof baranski
///
int _lcd(char * argv[]) {
    if( argv[0] == NULL ) return -1;
    if( argv[1] == NULL ) return chdir( getenv( "HOME" ) );
    else if( argv[2] != NULL ) return -1;
    return chdir( argv[1] );
}

///
/// @author krzysztof baranski
///
int _lexit(char * argv[]) {
    if( argv[0] == NULL || argv[1] != NULL ) return -1;
    exit(0);
}

///
/// @author krzysztof baranski
///
int _lkill(char * argv[]) {
    int signal_number = SIGTERM;
    pid_t pid;
    
    if( argv[0] == NULL || argv[1] == NULL ) return -1;
    if( argv[1][0] == '-' ) {
        //*DEBUG*/fprintf(stderr,"<1>");
        if( argv[2] == NULL || argv[3] != NULL ) return -1;
        //*DEBUG*/fprintf(stderr,"<2>");
        signal_number = atoi( argv[1] + 1 );
        //*DEBUG*/fprintf(stderr,"<3>");
        pid = atoi( argv[2] );
        //*DEBUG*/fprintf(stderr,"<4>");
    } else {
    //*DEBUG*/fprintf(stderr,"<5>");
        if( argv[2] != NULL ) return -1;
        pid = atoi( argv[1] );
    }
    //*DEBUG*/fprintf(stderr,"<6>");
    //*DEBUG*/fprintf(stderr,"<pid = %d, signal = %d>",pid,signal_number);
    return kill( pid, signal_number );
}

///
/// @author krzysztof baranski
///
int _lls(char * argv[]) {
    if( argv[0] == NULL ) return -1;
    DIR *dp;
    struct dirent *ep;     
    
    dp = opendir (".");
    if( dp == NULL ) return -1;
    while(( ep = readdir (dp) )) if( ep->d_name[0] != '.' )
        printf( "%s\n", ep->d_name );

    (void) closedir (dp);
    return EXIT_SUCCESS;
}
//====================================================================//
//--------------------------- END FUNCTIONS --------------------------//
//====================================================================//
