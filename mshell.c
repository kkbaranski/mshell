/*//////////////////////////////////////////////////////////////////////
//                                                                    //
//                                  _          _ _                    //
//                                 | |        | | |                   //
//                    _ __ ___  ___| |__   ___| | |                   //
//                   | '_ ` _ \/ __| '_ \ / _ \ | |                   //
//                   | | | | | \__ \ | | |  __/ | |                   //
//                   |_| |_| |_|___/_| |_|\___|_|_|                   //
//                                                                    //
//                             ( mshel.c )                            //
//                                                                    //
////////////////////////////////////////////////////////////////////////
///-------------- Copyright © 2014 Krzysztof Baranski  --------------///
//////////////////////////////////////////////////////////////////////*/
                                                 

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
#include <time.h>
#include <stdarg.h>
#include <signal.h>

#include "config.h"
#include "siparse.h"
#include "utils.h"
#include "builtins.h"
#include "bool.h"
#include "debug.h"
#include "commons.h"
#include "pipe.h"
#include "bgproc_queue.h"
#include "process_manager.h"

//=====================================================================| DEFINES
#define PARENT(pid) ((pid)>0)
#define CHILD(pid) ((pid)==0)


typedef int iterator;



//=====================================================================| VARIABLES
static char		__buffer[ MAX_LINE_LENGTH + 1 ];
static char		__line[ MAX_LINE_LENGTH + 1 ];
static volatile size_t	__line_size;
static volatile bool		__ignore;
static volatile bool		__end_of_file;





static sigset_t __sigchld_old_mask;



//=====================================================================| FUNCTIONS
// debug


// error
int handle_error( int no, char* subject );
void builtin_error( char* command_name );
void error_message( const char* message );
void error_named_message( const char* name, const char* message );








// redirections
void set_descriptors( pipe_t* prev_pipe, pipe_t* next_pipe );
void set_redirections( redirection *redirs[] );

// read
int load_line( int in_fileno );
char* get_line();
void get_line_init();

// line
bool empty_line( line* parsed_line );
int check_line( line* parsed_line );
int check_pipeline( pipeline p );
int count_commands( pipeline pline );
void execute_line( line* parsed_line );
int execute_external_command( command* cmd );
int execute_shell_command( iterator it, command* cmd );
void execute_pipeline( pipeline pline, int flags );

// signals
void block_sigchld();
void unblock_sigchld();
void sigchld_handler( int signum );


// main
void shell_init();
void print_prompt();
iterator is_shell_command( command* cmd );
void print_exited_bgproc( pid_t pid );
void print_exited_bgprocs();
void set_sigint_ignore();
void set_sigchld_handler();
void set_sigint_default();
void set_sigchld_default();















































// debug

// error
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

// processes

// pipe

// redirections
void set_descriptors( pipe_t* prev_pipe, pipe_t* next_pipe ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> set_descriptors()" );
	log.debug( INFO(5), "prev_pipe( in=%d, out=%d )", PIPE_IN( *prev_pipe ), PIPE_OUT( *prev_pipe ) );
	log.debug( INFO(5), "next_pipe( in=%d, out=%d )", PIPE_IN( *next_pipe ), PIPE_OUT( *next_pipe ) );

	replace_fd( STDIN_FILENO, PIPE_IN( *prev_pipe ) );
	replace_fd( STDOUT_FILENO, PIPE_OUT( *next_pipe ) );
	close_in( next_pipe );
	close_out( prev_pipe );
	
	log.debug( INFO(5), "prev_pipe( in=%d, out=%d )", PIPE_IN( *prev_pipe ), PIPE_OUT( *prev_pipe ) );
	log.debug( INFO(5), "next_pipe( in=%d, out=%d )", PIPE_IN( *next_pipe ), PIPE_OUT( *next_pipe ) );
}
void set_redirections( redirection *redirs[] ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> set_redirections()" );

	int fd, mode;
	while( *redirs ) {
		if( IS_RIN( ( *redirs )->flags ) ) {
			log.debug( INFO(5), "in-redirection ( filename='%s' )", ( *redirs )->filename );
			fd = open( ( *redirs )->filename, O_RDONLY );
		} else if( IS_ROUT( ( *redirs )->flags ) ) {
			log.debug( INFO(5), "out-redirection ( filename='%s' )", ( *redirs )->filename );
			mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;
			fd = open( ( *redirs )->filename, O_WRONLY|O_TRUNC|O_CREAT, mode );
		} else if( IS_RAPPEND( ( *redirs )->flags ) ) {
			log.debug( INFO(5), "append-redirection ( filename='%s' )", ( *redirs )->filename );
			mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;
			fd = open( ( *redirs )->filename, O_WRONLY|O_APPEND|O_CREAT, mode );
		} else {
			log.debug( DEBUG_ERROR, "undefined redirection ( filename='%s' )", ( *redirs )->filename );
		}
		
		if( ERROR( fd ) ) {
			log.debug( INFO(5), "redirection error ( filename='%s' )", ( *redirs )->filename );
			handle_error( errno, ( *redirs )->filename );
			exit( EXIT_ERROR );
		}
		
		if( IS_RIN( ( *redirs )->flags ) ) replace_fd( STDIN_FILENO, fd );
		else replace_fd( STDOUT_FILENO, fd );
	
		++redirs;
	}
}

// read
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
char* get_line() {
	logger log = { debug_function };
	log.debug( INFO(2), "> get_line()" );
	
	log.debug( INFO(3), "loading line" );
	block_sigchld();
	int status = load_line( STDIN_FILENO );
	unblock_sigchld();
	if( ERROR( status ) ) {
		log.debug( INFO(3), "parsing line error" );
		error_message( SYNTAX_ERROR_STR );
		return NULL;
	}
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

// line
bool empty_line( line* parsed_line ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> empty_line()" );
	return 	parsed_line &&
			*parsed_line->pipelines && !*( parsed_line->pipelines + 1 ) &&
			**parsed_line->pipelines && !*( *parsed_line->pipelines + 1 ) &&
			!( **parsed_line->pipelines )->argv[0];
}
int check_line( line* parsed_line ) {
	logger log = { debug_function };
	log.debug( INFO(3), "> check_line()" );
	
	if( empty_line( parsed_line ) ) {
		log.debug( INFO(3), "empty line" );
		return EXIT_ERROR;
	} else {
		log.debug( INFO(3), "non empty line" );
	}
	
	int c, status;
	pipeline* p;

	c = 0;
	for( p = parsed_line->pipelines; *p; ++p, ++c ) {
		status = check_pipeline( *p );
		if( ERROR( status ) ) {
			log.debug( INFO(3), "pipeline nr %d error", c );
			error_message( SYNTAX_ERROR_STR );
			return EXIT_ERROR;
		}
	}
	log.debug( INFO(3), "line OK -> %d pipelines", c );
	return c;
}
int check_pipeline( pipeline p ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> check_pipeline()" );

	int c;
	command** pcmd;

	if( p == NULL ) {
		log.debug( DEBUG_ERROR, "pipeline == NULL" );
		return EXIT_ERROR;
	}
	
	c = 0;
	for( pcmd = p; *pcmd; ++pcmd, ++c )
		if( (*pcmd)->argv[0] == NULL ) {
			log.debug( INFO(5), "command == NULL" );
			return EXIT_ERROR;
		}
		
	log.debug( INFO(5), "pipeline OK -> %d commands", c );
	return c;
}
int count_commands( pipeline pline ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> count_commands()" );
	
	int counter = check_pipeline( pline );
	log.debug( INFO(5), "number of commands = %d", counter );
	
	return counter;
}
void execute_line( line* parsed_line ) {
	logger log = { debug_function };
	log.debug( INFO(2), "> execute_line()" );
	
	int status;
	pipeline* pline;
	
	log.debug( INFO(3), "checking line" );
	status = check_line( parsed_line );
	if( ERROR( status ) ) {
		log.debug( INFO(3), "line error" );
		return;
	}
	
	log.debug( INFO(3), "executing pipelines" );
	for( pline = parsed_line->pipelines; *pline; ++pline ) {
		execute_pipeline( *pline, parsed_line->flags );
		
		block_sigchld();
		while( count_fgproc() > 0 ) sigsuspend( &__sigchld_old_mask );
		unblock_sigchld();
	}
}
int execute_external_command( command* cmd ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> execute_external_command( '%s' )", *cmd->argv );
	
	int status = execvp( *cmd->argv, cmd->argv );
	if( ERROR( status ) ) {
		log.debug( INFO(5), "execution failed" );
		handle_error( errno, *cmd->argv );
		return EXIT_ERROR;
	}
	
	log.debug( INFO(5), "execution successful" );
	return EXIT_SUCCESS;
}
int execute_shell_command( iterator it, command* cmd ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> execute_shell_command( '%s' )", *cmd->argv );
	
	int status = builtins_table[ it ].fun( cmd->argv );
	if( ERROR( status ) ) {
		log.debug( INFO(5), "execution failed" );
		builtin_error( *cmd->argv );
		return EXIT_ERROR;
	}
	
	log.debug( INFO(5), "execution successful" );
	return EXIT_SUCCESS;
}
void execute_pipeline( pipeline pline, int flags ) {
	logger log = { debug_function };
	log.debug( INFO(3), "> execute_pipeline()" );
	
	pid_t pid;
	command** cmd = pline;
	bool is_background = ( flags & LINBACKGROUND );
	
	log.debug( INFO(4), "counting commands" );
	int n = count_commands( pline );
	
	if( n == 1 ) {
		log.debug( INFO(4), "checking if shell command" );
		iterator it = is_shell_command( *cmd );
		if( it >= 0 ) {
			log.debug( INFO(4), "shell command" );
			execute_shell_command( it, *cmd );
			return;
		}
		log.debug( INFO(4), "non-shell command" );
	}
	
	log.debug( INFO(4), "initializing pipes [ prev, next ]" );
	pipe_t prev_pipe = standard_pipe();
	pipe_t next_pipe = standard_pipe();
	
	//log.debug( INFO(4), "blocking child signal" );
	//block_sigchld();
	
	for( cmd = pline; *cmd; ++cmd ) {
		log.debug( INFO(4), "working with command: '%s'", ( *cmd )->argv[0] );
		
		if( is_background ) {
			log.debug( INFO(4), "background command" );
		} else {
			log.debug( INFO(4), "foreground command" );
			increment_fgproc();
		}
		
		if( *( cmd + 1 ) ) {
			log.debug( INFO(4), "there is next command - creating a new pipe" );
			next_pipe = new_pipe();
		}
		
		log.debug( INFO(4), "forking" );
		pid = fork();
		
		if( ERROR( pid ) ) {
			log.debug( DEBUG_ERROR, "fork error: %s", strerror( errno ) );
			exit( errno );
		}
		
		if( CHILD( pid ) ) {
			if( DEBUG_LEVEL > 0 ) sleep(1);
			
			log.debug( INFO(4), "### child( pid=%d, ppid=%d ) ###", getpid(), getppid() );
			
			set_sigchld_default();
			set_sigint_default();
			
			if( is_background ) {
				log.debug( INFO(4), "background command '%s'", ( *cmd )->argv[0] );
				setsid();
			}
			
			log.debug( INFO(4), "setting descriptors:" );
			set_descriptors( &prev_pipe, &next_pipe );
			
			log.debug( INFO(4), "setting redirections:" );
			set_redirections( ( *cmd )->redirs );
			
			log.debug( INFO(4), "executing command:" );
			execute_external_command( *cmd );
			
			log.debug( INFO(4), "child( pid=%d, ppid=%d ) says bye!", getpid(), getppid() );
			exit( EXIT_SUCCESS );
		}
		
		log.debug( INFO(4), "after forking pid = %d", pid );
		
		if( is_background ) {
			log.debug( INFO(4), "registering bgproc [ pid = %d ]", pid );
			register_bgproc( pid );
		} else {
			log.debug( INFO(4), "registering fgproc [ pid = %d ]", pid );
			register_fgproc( pid );
		}
		
		
		log.debug( INFO(4), "closing descriptors in parent" );
		close_in( &prev_pipe );
		close_out( &next_pipe );
		
		log.debug( INFO(4), "moving pipes right" );
		prev_pipe = next_pipe;
		next_pipe = standard_pipe();
	}
	
	//log.debug( INFO(4), "unblocking child signal" );
	//unblock_sigchld();
	
	log.debug( INFO(4), "exiting execute_pipeline()" );
}

// signals
void block_sigchld() {
	logger log = { debug_function };
	log.debug( INFO(4), "> block_sigchld()" );
	
	sigset_t chld_block_mask;
	sigemptyset( &chld_block_mask );
	sigaddset( &chld_block_mask, SIGCHLD );
	sigprocmask( SIG_BLOCK, &chld_block_mask, &__sigchld_old_mask );
}
void unblock_sigchld() {
	logger log = { debug_function };
	log.debug( INFO(4), "> unblock_sigchld()" );
	
	sigset_t chld_block_mask;
	sigemptyset( &chld_block_mask );
	sigaddset( &chld_block_mask, SIGCHLD );
	sigprocmask( SIG_UNBLOCK, &chld_block_mask, NULL );
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

// main
int main( int argc, char *argv[] ) {
	logger log = { debug_function };
	log.debug( INFO(1), "SHELL START" );
	
	int status;
	char* new_line;
	line* parsed_line;
	
    shell_init();
    while( ! __end_of_file ) {
        print_prompt();
        new_line = get_line();
        if( !new_line ) continue;
        parsed_line = parseline( new_line );
        execute_line( parsed_line );   
    }
    
    log.debug( INFO(1), "SHELL STOP" );
    exit( EXIT_SUCCESS );
}
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
void print_prompt() {
	logger log = { debug_function };
	log.debug( INFO(2), "> print_prompt()" );
	
	struct stat st;
	if( fstat( STDIN_FILENO, &st ) ) {
		log.debug( DEBUG_ERROR, "fstat error" );
		exit( EXIT_FAILURE );
	}
	
	if( S_ISCHR( st.st_mode ) ) {
		log.debug( INFO(3), "stdout is character special file" );
		
		log.debug( INFO(3), "blocking child signal" );
		block_sigchld();

		log.debug( INFO(3), "printing statuses of exited background processes" );
		print_exited_bgprocs();
		
		log.debug( INFO(3), "printing prompt" );
		fprintf( stdout, "%s", PROMPT_STR );
		fflush( stdout );
		
		log.debug( INFO(3), "unblocking child signal" );
		unblock_sigchld();
	} else {
		log.debug( INFO(3), "stdout is NOT character special file" );
	}
}
iterator is_shell_command( command* cmd ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> is_shell_command()" );
	
    iterator it = 0;
    while(  builtins_table[ it ].name != NULL && strcmp( builtins_table[ it ].name, *cmd->argv ) != 0 ) ++it;
    if( builtins_table[ it ].name != NULL ) {
		log.debug( INFO(5), "shell command" );
		return it;
	}
	
	log.debug( INFO(5), "non-shell command" );
    return EXIT_ERROR;
}
void print_exited_bgproc( pid_t pid ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> print_exited_bgproc() -> queue_size = %d", size_bgproc_queue() );
	
	fprintf( stdout, "Background process %d terminated.", pid );
	fprintf( stdout, " " );
	
	if( WIFSIGNALED( get_status_proc( pid ) ) ) {
		fprintf( stdout, "(killed by signal %d)", WTERMSIG( get_status_proc( pid ) ) );
		fprintf( stdout, "\n" );
		
	} else {
		fprintf( stdout, "(exited with status %d)", get_status_proc( pid ) );
		fprintf( stdout, "\n" );
	}
	fflush( stdout );
}
void print_exited_bgprocs() {
	logger log = { debug_function };
	log.debug( INFO(3), "> print_exited_bgprocs() -> queue_size = %d", size_bgproc_queue() );
	
	int i;
	for( i = 0; i < size_bgproc_queue(); ++i ) {
		pid_t pid = get_bgproc_queue( i );
		
		log.debug( INFO(4), "printing bgproc pid = %d", pid );
		if( !is_bgproc( pid ) ) log.debug( DEBUG_ERROR, "process type != BACKGROUND in queue [ pid=%d, type=%d ]", pid, get_type_proc( pid ) );
		print_exited_bgproc( pid );
		unregister_proc( pid );
	}
	clean_bgproc_queue();
}
void set_sigint_ignore() {
	logger log = { debug_function };
	log.debug( INFO(5), "> set_sigint_ignore()" );
	
	if( signal( SIGINT, SIG_IGN ) == SIG_ERR ) {
		log.debug( DEBUG_ERROR, "error: 'signal( SIGINT, SIG_IGN )'" );
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
void set_sigchld_default() {
	logger log = { debug_function };
	log.debug( INFO(5), "> set_sigchld_default()" );
	
	if( signal( SIGCHLD, SIG_DFL ) == SIG_ERR ) {
		log.debug( DEBUG_ERROR, "error: 'signal( SIGINT, SIG_DFL )'" );
		exit( EXIT_ERROR );
	}
}


































//====================================================================//
//                                                                    //
//                                MAIN                                //
//                                                                    //
//====================================================================//

//====================================================================//
//----------------------------- END MAIN -----------------------------//
//====================================================================//


//====================================================================//
//                                                                    //
//                             FUNCTIONS                              //
//                                                                    //
//====================================================================//
























































//====================================================================//
//--------------------------- END FUNCTIONS --------------------------//
//====================================================================//



















