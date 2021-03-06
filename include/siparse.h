/*//////////////////////////////////////////////////////////////////////
//                                                                    //
//                                  _          _ _                    //
//                                 | |        | | |                   //
//                    _ __ ___  ___| |__   ___| | |                   //
//                   | '_ ` _ \/ __| '_ \ / _ \ | |                   //
//                   | | | | | \__ \ | | |  __/ | |                   //
//                   |_| |_| |_|___/_| |_|\___|_|_|                   //
//                                                                    //
//                           ( siparse.h )                            //
//                                                                    //
////////////////////////////////////////////////////////////////////////
///-------------- Copyright © 2014 Krzysztof Baranski  --------------///
//////////////////////////////////////////////////////////////////////*/
#ifndef __SIPARSE_H__
#define __SIPARSE_H__

//==========================================================| INCLUDE

//==========================================================| CONST
#define RIN 			1
#define LINBACKGROUND 	1

//==========================================================| DEFINE
#define ROUT			(1<<1)
#define RAPPEND			(1<<2)
#define RTMASK 			(RIN|ROUT|RAPPEND)
#define IS_RIN(x)		(((x)&RTMASK) == RIN )
#define IS_ROUT(x)		(((x)&RTMASK) == ROUT )
#define IS_RAPPEND(x)	(((x)&RTMASK) == (ROUT|RAPPEND) )

//==========================================================| TYPEDEF
typedef struct redirection{ char *filename; int flags; } redirection;
typedef struct command { char** argv; redirection** redirs; } command;  
typedef command** pipeline;
typedef pipeline* pipelineseq;
typedef struct line { pipelineseq pipelines; int flags; } line;

//==========================================================| VARIABLE

//==========================================================| FUNCTION
line*	parseline( char* );

#endif // __SIPARSE_H__

/*
 * 		line* parseline( char* )
 * 
 * Parses given string containing sequence of pipelines separated by ';'. 
 * Each pipeline is a sequence of commands separated by '|'.
 * Function returns a pointer to the static structure line or NULL if meets a parse error.
 * All structures referenced from the result of the function are statically allocated and shall not be freed.
 * Passing a string longer than MAX_LINE_LENGHT may result in an unspecified behaviour.
 * Consecutive calls to the function destroy the content of previously returned structures.
 * 
 * 
 */
