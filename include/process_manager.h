/*//////////////////////////////////////////////////////////////////////
//                                                                    //
//                                  _          _ _                    //
//                                 | |        | | |                   //
//                    _ __ ___  ___| |__   ___| | |                   //
//                   | '_ ` _ \/ __| '_ \ / _ \ | |                   //
//                   | | | | | \__ \ | | |  __/ | |                   //
//                   |_| |_| |_|___/_| |_|\___|_|_|                   //
//                                                                    //
//                        ( process_manager.h )                       //
//                                                                    //
////////////////////////////////////////////////////////////////////////
///-------------- Copyright © 2014 Krzysztof Baranski  --------------///
//////////////////////////////////////////////////////////////////////*/
#ifndef __PROCESS_MANAGER_H__
#define __PROCESS_MANAGER_H__

//==========================================================| INCLUDE
#include <stddef.h>
#include <sys/types.h>
#include "bool.h"
#include "commons.h"

//==========================================================| CONST

//==========================================================| DEFINE

//==========================================================| TYPEDEF
typedef enum _type_t { NONE, FOREGROUND, BACKGROUND } type_t;
typedef struct _process { type_t type; int status; } process;

//==========================================================| VARIABLE
static process		__processes[ PID_MAX + 1 ];
static size_t		__fgproc_counter;

//==========================================================| FUNCTION
size_t		count_fgproc();
void		decrement_fgproc();
process*	get_proc( pid_t );
int			get_status_proc( pid_t );
type_t		get_type_proc( pid_t );
void		increment_fgproc();
bool		is_bgproc( pid_t );
bool		is_fgproc( pid_t );
void		register_bgproc( pid_t );
void		register_fgproc( pid_t );
void		reset_fgproc_counter();
void		set_status_proc( pid_t, int status );
void		unregister_proc( pid_t );

#endif // __PROCESS_MANAGER_H__
