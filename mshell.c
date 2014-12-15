/*//////////////////////////////////////////////////////////////////////
///                                                                  ///
///                              SHELL                               ///
///                                                                  ///
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

//=====================================================================| DEFINES
#define PARENT(pid) ((pid)>0)
#define CHILD(pid) ((pid)==0)
#define ERROR(x) ((x)==-1)
#define PIPE_IN(x) (x).fd[0]
#define PIPE_OUT(x) (x).fd[1]
#define INFO(x) (x)

//=====================================================================| CONSTS
#define ETX 3
#define EXIT_ERROR -1
#define DEBUG_LEVEL 5
#define DEBUG_ERROR -1
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define PID_MAX 32768

//=====================================================================| TYPEDEFS
typedef enum _bool { false, true } bool;
typedef struct _pipe_t { int fd[2]; } pipe_t;
typedef int iterator;
typedef struct _logger { void (*debug)( int, char*, ... ); } logger;
typedef struct _process { bool running; int status; } process;

//=====================================================================| VARIABLES
static char		__buffer[ MAX_LINE_LENGTH + 1 ];
static char		__line[ MAX_LINE_LENGTH + 1 ];
static size_t	__line_size;
static bool		__ignore;
static bool		__end_of_file;
static process	__background_processes[ PID_MAX + 1 ];
static pid_t	__exited_bg_processes[ PID_MAX + 1 ];
static size_t	__exited_bg_processes_size;
static size_t	__foreground_processes_counter;

//=====================================================================| FUNCTIONS
void 	builtin_error( const char* command_name );
int		check_line( line* );
int		check_pipeline( pipeline );
void	debug_error( const char* syscall );
bool	empty_line( line* );
void	error_message( const char* message );
void	error_named_message( const char* name, const char* message );
void	execute_line( line* );
void	execute_pipeline( pipeline, int flags );
int		load_line( int _in_fileno );
void	print_prompt();
void	shell_init();
void	debug_function( int level, char* fmt, ... );
int		handle_error( int no, char *subject );
void	print_exited_bg_processes();



//====================================================================//
//                                                                    //
//                                MAIN                                //
//                                                                    //
//====================================================================//
int main( int argc, char *argv[] )
{
	logger log = { debug_function };
	log.debug( INFO(1), "SHELL START" );
	
	int status;
	line* parsed_line;
	
    shell_init();
    while( ! __end_of_file ) {
        print_prompt();
        status = load_line( STDIN_FILENO );
        if( ERROR( status ) ) {
			log.debug( INFO(2), "parsing line error" );
			error_message( SYNTAX_ERROR_STR );
			continue;
        }
        
        log.debug( INFO(2), "line = '%s'", __line );
        
        parsed_line = parseline( __line );
        execute_line( parsed_line );   
    }
    log.debug( INFO(1), "SHELL STOP" );
    exit( EXIT_SUCCESS );
}
//====================================================================//
//----------------------------- END MAIN -----------------------------//
//====================================================================//


//====================================================================//
//                                                                    //
//                             FUNCTIONS                              //
//                                                                    //
//====================================================================//
void builtin_error( const char* command_name ) {
	logger log = { debug_function };
	log.debug( INFO(3), "> builtin_error()" );
	
    fprintf( stderr, "Builtin %s error.\n", command_name );
}

int check_line( line* parsed_line ) {
	logger log = { debug_function };
	log.debug( INFO(3), "> check_line()" );
	
	if( empty_line( parsed_line ) ) {
		log.debug( INFO(3), "empty line" );
		return EXIT_ERROR;
	}
	int c, status;
	pipeline* p;

	bool empty_line = false;
	c = 0;
	for( p = parsed_line->pipelines; *p; ++p, ++c ) {
		status = check_pipeline( *p );
		if( status == -1 ) {
			log.debug( INFO(3), "pipeline nr %d error", c );
			error_message( SYNTAX_ERROR_STR );
			return EXIT_ERROR;
		}
	}
	log.debug( INFO(3), "line OK" );
	return c;
}

int check_pipeline( pipeline p ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> check_pipeline()" );

	int c;
	command** pcmd;

	if( p == NULL ) return EXIT_ERROR; // SIC!
	c = 0;
	for( pcmd = p; *pcmd; ++pcmd, ++c )
		if( (*pcmd)->argv[0] == NULL ) return EXIT_ERROR;
	return c;
}

int count_commands( pipeline pline ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> count_commands()" );
	
	return check_pipeline( pline );
}

bool empty_line( line* parsed_line ) {
	return 	parsed_line &&
			*parsed_line->pipelines && !*( parsed_line->pipelines + 1 ) &&
			**parsed_line->pipelines && !*( *parsed_line->pipelines + 1 ) &&
			!( **parsed_line->pipelines )->argv[0];
}

void error_message( const char* message ) {
	fprintf( stderr, "%s\n", message );
}

void error_named_message( const char* name, const char* message ) {
	fprintf( stderr, "%s: %s\n", name, message );
}

void execute_line( line* parsed_line ) {
	logger log = { debug_function };
	log.debug( INFO(2), "EXECUTE LINE FUNCTION" );
	
	int status;
	pipeline* pline;
	
	status = check_line( parsed_line );
	if( ERROR( status ) ) return;
	
	for( pline = parsed_line->pipelines; *pline; ++pline )
		execute_pipeline( *pline, parsed_line->flags );
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

pipe_t new_pipe() {
	logger log = { debug_function };
	log.debug( INFO(5), "> new_pipe()" );
	
	pipe_t p;
	pipe( p.fd );
	
	log.debug( INFO(6), "return pipe( in=%d, out=%d )", PIPE_IN( p ), PIPE_OUT( p ) );
	
	return p;
}

void close_in( pipe_t* p ) {
	logger log = { debug_function };
	log.debug( INFO(5), "CLOSE IN-DESCRIPTOR FUNCTION" );
	log.debug( INFO(6), "pipe( in=%d, out=%d )", PIPE_IN( *p ), PIPE_OUT( *p ) );
	
	if( PIPE_IN( *p ) <= 2 ) return;
	
	log.debug( INFO(6), "closing desriptor %d", PIPE_IN( *p ) );
	close( PIPE_IN( *p ) );
	PIPE_IN( *p ) = STDIN_FILENO;
	
	log.debug( INFO(6), "pipe( in=%d, out=%d )", PIPE_IN( *p ), PIPE_OUT( *p ) );
}

void close_out( pipe_t* p ) {
	logger log = { debug_function };
	log.debug( INFO(5), "CLOSE OUT-DESCRIPTOR FUNCTION" );
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
	log.debug( INFO(5), "next_pipe( in=%d, out=%d )", PIPE_IN( *next_pipe ), PIPE_OUT( *next_pipe ) );}

iterator shell_command( command* cmd ) {
    iterator it = 0;
    while(  builtins_table[ it ].name != NULL && strcmp( builtins_table[ it ].name, *cmd->argv ) != 0 ) ++it;
    if( builtins_table[ it ].name != NULL ) return it;
    return EXIT_ERROR;
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

void increment_foreground() {
	logger log = { debug_function };
	log.debug( INFO(4), "> increment_foreground()" );
	
	log.debug( INFO(5), "from %d to %d", __foreground_processes_counter, __foreground_processes_counter + 1 );
	++__foreground_processes_counter;
}

void decrement_foreground() {
	logger log = { debug_function };
	log.debug( INFO(4), "> decrement_foreground()" );
	
	log.debug( INFO(5), "from %d to %d", __foreground_processes_counter, __foreground_processes_counter - 1 );
	--__foreground_processes_counter;
}

size_t number_of_foreground() {
	logger log = { debug_function };
	log.debug( INFO(4), "> number_of_foreground()" );
	
	log.debug( INFO(5), "__foreground_processes_counter = %d", __foreground_processes_counter );
	return __foreground_processes_counter;
}

void execute_pipeline( pipeline pline, int flags ) {
	logger log = { debug_function };
	log.debug( INFO(3), "> execute_pipeline()" );
	
	pid_t pid;
	bool is_background = ( flags & LINBACKGROUND );
	
	log.debug( INFO(4), "counting commands" );
	int n = count_commands( pline );
	
	log.debug( INFO(4), "initializing pipes [ prev, next ]" );
	pipe_t prev_pipe = standard_pipe();
	pipe_t next_pipe = standard_pipe();
	
	log.debug( INFO(4), "blocking child signal" );
	block_chld_sig();
	
	for( command** cmd = pline; *cmd; ++cmd ) {
		log.debug( INFO(4), "working with command: '%s'", ( *cmd )->argv[0] );
		
		log.debug( INFO(4), "checking if shell command" );
		iterator it = shell_command( *cmd );
		if( it >= 0 ) {
			log.debug( INFO(4), "shell command" );
			execute_shell_command( it, *cmd );
			continue;
		}
		log.debug( INFO(4), "non-shell command" );
		
		if( *( cmd + 1 ) ) {
			log.debug( INFO(4), "there is next command - creating a new pipe" );
			next_pipe = new_pipe();
		}
		
		log.debug( INFO(4), "forking" );
		pid = fork();
		if( pid == 0 ) {
			log.debug( INFO(4), "-- CHILD( pid=%d, ppid=%d ) --", getpid(), getppid() );
			
			if( is_background ) {
				log.debug( INFO(4), "background command '%s'", ( *cmd )->argv[0] );
				setsid();
			} else {
				increment_foreground();
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
		
		log.debug( INFO(4), "closing descriptors in parent" );
		close_in( &prev_pipe );
		close_out( &next_pipe );
		
		log.debug( INFO(4), "moving pipes right" );
		prev_pipe = next_pipe;
		next_pipe = standard_pipe();
		
		if( !is_background ) {
			
		}
	}
	
	log.debug( INFO(4), "unblocking child signal" );
	unblock_chld_sig();
	
	log.debug( INFO(4), "exiting execute_pipeline()" );
}

int load_line( int _in_fileno ) {
	logger log = { debug_function };
	log.debug( INFO(2), "> load_line()" );
	
    char c;
    ssize_t returned_bytes;
    int cursor;
    __line_size = 0;
    cursor = -1;
    __ignore = false;

	while( true ) {
        c = __buffer[ ++cursor ];
        if( c == '\n' ) {
            __line[ __line_size ] = '\0';
            memmove( __buffer, __buffer+cursor+1, MAX_LINE_LENGTH-cursor+1 );
            break;
        } else if( c == ETX ) {
            cursor = -1;
            returned_bytes = read( _in_fileno, __buffer, MAX_LINE_LENGTH );
            if(returned_bytes == 0) {
                __line[ __line_size ] = '\0';
                __end_of_file = true;
				break;
			}
			__buffer[ returned_bytes ] = ETX;
		} else {
			if(__ignore) {
				continue;
			} else if( __line_size >= MAX_LINE_LENGTH ) {
				__line_size = 0;
				__ignore = true;
			} else {
				__line[ __line_size++ ] = c;
			}
		}
	}
	if( __ignore ) return EXIT_ERROR;
	return EXIT_SUCCESS;
}

void block_chld_sig() {
	logger log = { debug_function };
	log.debug( INFO(3), "> block_child_signal()" );
	
	sigset_t chld_block_mask;
	sigemptyset(&chld_block_mask);
	sigaddset(&chld_block_mask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &chld_block_mask, NULL);
}

void unblock_chld_sig() {
	logger log = { debug_function };
	log.debug( INFO(3), "> unblock_child_signal()" );
	
	sigset_t chld_block_mask;
	sigemptyset(&chld_block_mask);
	sigaddset(&chld_block_mask, SIGCHLD);
	sigprocmask(SIG_UNBLOCK, &chld_block_mask, NULL);
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
		block_chld_sig();

		log.debug( INFO(3), "removing zombie processes" );
		print_exited_bg_processes();
		
		log.debug( INFO(3), "printing prompt" );
		fprintf( stdout, "%s", PROMPT_STR );
		fflush( stdout );
		
		log.debug( INFO(3), "unblocking child signal" );
		unblock_chld_sig();
	} else {
		log.debug( INFO(3), "stdout is NOT character special file" );
	}
}

void clean_bg_proc( process* bgp ) {
	bgp->running = false;
}

bool is_running( process* bgp ) {
	return bgp->running;
}

int get_status( process* bgp ) {
	return bgp->status;
}

process* bg_proc( pid_t pid ) {
	return __background_processes + pid;
}

void shell_init() {
	logger log = { debug_function };
	log.debug( INFO(2), "> shell_init()" );
	
	int i;
	
	log.debug( INFO(5), "__buffer[0] = ETX" );
	__buffer[0] = ETX;
	
	log.debug( INFO(5), "__end_of_file = 0" );
    __end_of_file = 0;
    
    log.debug( INFO(5), "__exited_bg_processes_size = 0" );
    __exited_bg_processes_size = 0;
    
    log.debug( INFO(5), "__foreground_processes_counter = 0" );
    __foreground_processes_counter = 0;
    
    log.debug( INFO(5), "cleaning background_processes array" );
    for( i = 0; i <= PID_MAX; ++i ) clean_bg_proc( __background_processes + i );
    
}

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

int handle_error(int no, char *subject) {
  if(!no) { return 0; }

  switch( errno ) {
  case EACCES:
    fprintf( stderr, "%s: permission denied\n", subject );
    break;
  case ENOENT:
    fprintf( stderr, "%s: no such file or directory\n", subject );
    break;
  case ENOEXEC:
    fprintf( stderr, "%s: exec error\n", subject );
    break;
  }

  return no;
}

void print_exited_bg_process( pid_t pid ) {
	process* proc = bg_proc( pid );
	
	fprintf( stdout, "Background process %d terminated.", pid );
	fprintf( stdout, " " );
	if( WIFSIGNALED( get_status( proc ) ) ) {
		fprintf( stdout, "(killed by signal %d)", WTERMSIG( get_status( proc ) ) );
		fprintf( stdout, "\n" );
		
	} else {
		fprintf( stdout, "(exited with status %d)", get_status( proc ) );
		fprintf( stdout, "\n" );
	}
	fflush( stdout );
}

void print_exited_bg_processes() {
	int i;
	for( i = 0; i < __exited_bg_processes_size; ++i ) {
		print_exited_bg_process( __exited_bg_processes[ i ] );
		clean_bg_proc( bg_proc( __exited_bg_processes[ i ] ) );
	}
	__exited_bg_processes_size = 0;
}















































//====================================================================//
//--------------------------- END FUNCTIONS --------------------------//
//====================================================================//







/*
	//--------------------------------------------------------------
	// BUILT IN SHELL COMMAND
	//--------------------------------------------------------------
	comm = shell_command( *com->argv );
	if( comm >= 0 ) {               // shell command
		ret_val = invoke( comm, com->argv );
		if( ERROR( ret_val ) ) {
			builtin_error( *com->argv );
		}
		continue;
	}
	
	//--------------------------------------------------------------
	// OUT OF SHELL COMMAND
	//--------------------------------------------------------------
	pid = fork();
	if( PARENT(pid) ) {
		waitpid( pid, NULL, 0 );
	} else if( CHILD(pid) ) {
		exec_res = execvp( *com->argv, com->argv );
		if( ERROR(exec_res) ) {
			switch(errno) {
				case    EACCES: error_named_message( *com->argv, "permission denied" );break;
				case    ENOENT: error_named_message( *com->argv, "no such file or directory" );break;
				default:        error_named_message( *com->argv, "exec error" );break;
			}
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
	} else if( ERROR(pid) ) {
		debug_error( "fork" );
	}
	*/
