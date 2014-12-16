/*//////////////////////////////////////////////////////////////////////
//                                                                    //
//                                  _          _ _                    //
//                                 | |        | | |                   //
//                    _ __ ___  ___| |__   ___| | |                   //
//                   | '_ ` _ \/ __| '_ \ / _ \ | |                   //
//                   | | | | | \__ \ | | |  __/ | |                   //
//                   |_| |_| |_|___/_| |_|\___|_|_|                   //
//                                                                    //
//                            ( utils.c )                             //
//                                                                    //
////////////////////////////////////////////////////////////////////////
///-------------- Copyright © 2014 Krzysztof Baranski  --------------///
//////////////////////////////////////////////////////////////////////*/

//==========================================================| INCLUDE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "bgproc_queue.h"
#include "builtins.h"
#include "commons.h"
#include "config.h"
#include "debug.h"
#include "message.h"
#include "process_manager.h"
#include "signal_utils.h"
#include "utils.h"

//==========================================================| VARIABLE

//==========================================================| FUNCTION
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

bool empty_line( line* parsed_line ) {
	logger log = { debug_function };
	log.debug( INFO(4), "> empty_line()" );
	return 	parsed_line &&
			*parsed_line->pipelines && !*( parsed_line->pipelines + 1 ) &&
			**parsed_line->pipelines && !*( *parsed_line->pipelines + 1 ) &&
			!( **parsed_line->pipelines )->argv[0];
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
	
	// log.debug( INFO(4), "blocking child signal" );
	// block_sigchld();
	
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
	
	// log.debug( INFO(4), "unblocking child signal" );
	// unblock_sigchld();
	
	log.debug( INFO(4), "exiting execute_pipeline()" );
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
