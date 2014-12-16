/*//////////////////////////////////////////////////////////////////////
//                                                                    //
//                                  _          _ _                    //
//                                 | |        | | |                   //
//                    _ __ ___  ___| |__   ___| | |                   //
//                   | '_ ` _ \/ __| '_ \ / _ \ | |                   //
//                   | | | | | \__ \ | | |  __/ | |                   //
//                   |_| |_| |_|___/_| |_|\___|_|_|                   //
//                                                                    //
//                            ( utils.h )                             //
//                                                                    //
////////////////////////////////////////////////////////////////////////
///-------------- Copyright © 2014 Krzysztof Baranski  --------------///
//////////////////////////////////////////////////////////////////////*/
#ifndef __UTILS_H__
#define __UTILS_H__

//==========================================================| INCLUDE
#include <unistd.h>
#include <sys/types.h>
#include "bool.h"
#include "pipe.h"
#include "siparse.h"

//==========================================================| CONST

//==========================================================| DEFINE
#define CHILD(pid) ((pid)==0)
#define PARENT(pid) ((pid)>0)

//==========================================================| TYPEDEF
typedef int iterator;

//==========================================================| VARIABLE

//==========================================================| FUNCTION
int			check_line( line* parsed_line );
int			check_pipeline( pipeline p );
int			count_commands( pipeline pline );
bool		empty_line( line* parsed_line );
int			execute_external_command( command* cmd );
void		execute_line( line* parsed_line );
void		execute_pipeline( pipeline pline, int flags );
int			execute_shell_command( iterator it, command* cmd );
iterator	is_shell_command( command* cmd );
void		print_exited_bgproc( pid_t pid );
void		print_exited_bgprocs();
void		print_prompt();
void		set_descriptors( pipe_t* prev_pipe, pipe_t* next_pipe );
void		set_redirections( redirection *redirs[] );

#endif // __UTILS_H__
