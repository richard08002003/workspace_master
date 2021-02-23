/*
 * main.c
 *
 *  Created on: 2018年4月3日
 *      Author: richard
 */

#include "Http_HomeLock.h"

S_Log *g_log ;
int main ( int argc , char *argv [ ] ) {
	char *pgm_ptr = argv [ 0 ] ;
	char *ptr = strrchr ( pgm_ptr , '/' ) ;
	ptr += 1 ;
	char Pgm_Name [ 32 + 1 ] ;
	memset ( Pgm_Name , 0 , sizeof ( Pgm_Name ) ) ;
	snprintf ( Pgm_Name , sizeof ( Pgm_Name ) , "%s" , ptr ) ;

	// 顯示程式名稱與版本 //
	if ( 2 == argc ) {
		if ( 0 == memcmp ( argv [ 1 ] , "-v" , 3 ) ) {
			printf ( "Version : %s\n" , Ver ) ;
			printf ( "Program : %s\n" , Pgm_Name ) ;
			return 0 ;
		}
	}

	g_log = S_Log_tool_New ( ) ;
	if ( NULL == g_log ) {
		printf ( "Error S_Log_tool_New error\n" ) ;
		return - 1 ;
	}

	int rtn = g_log->Initial ( g_log , DF_Pgm_Name ) ;
	if ( D_success != rtn ) {
		printf ( "Error g_log->Initial() error\n" ) ;
		return - 2 ;
	}

	S_Http_HomeLock *homelock = NULL ;
	while ( 1 ) {
		if ( fork ( ) == 0 ) {
			printf ( "********** Pgm:%s , Ver:%s **********\n" , Pgm_Name , Ver ) ;
			g_log->Save ( g_log , "********** Pgm:%s , Ver:%s **********" , Pgm_Name , Ver ) ;

			// 程式起始
			homelock = S_Http_HomeLock_New ( ) ;
			if ( NULL == homelock) {
				g_log->Save ( g_log , "Error S_Http_HomeLock_New error" ) ;
				exit ( 0 ) ;
			}
			g_log->Save ( g_log , "S_Http_HomeLock_New success" ) ;

			rtn = homelock->Pgm_Begin ( homelock , Pgm_Name ) ;
			if ( rtn != D_success ) {
				g_log->Save ( g_log , "Error Pgm_Begin error , rtn = %d" , rtn ) ;
				g_log->Close ( g_log ) ;
				S_Log_tool_Delete ( & g_log ) ;
				exit ( 0 ) ;
			}
		}
		int fork_status = 0 ;
		if ( waitpid ( 0 , & fork_status , 0 ) == - 1 ) {
			printf ( "<%s> Fork parents process error, will exit\n" , Pgm_Name ) ;
			g_log->Save ( g_log , "<%s> Fork parents process error, will exit" , Pgm_Name ) ;
			g_log->Close ( g_log ) ;
			exit ( 0 ) ;
		}
		g_log->Save ( g_log , "pgm:<%s> parent wait succ , will remake , error" , Pgm_Name ) ;
		g_log->Save ( g_log , "1:%d" , WEXITSTATUS( fork_status ) ) ;
		g_log->Save ( g_log , "2:%d" , WTERMSIG( fork_status ) ) ;
		g_log->Save ( g_log , "3:%d" , WSTOPSIG( fork_status ) ) ;
		g_log->Save ( g_log , "4:%d" , WIFEXITED( fork_status ) ) ;
		g_log->Save ( g_log , "5:%d" , WIFSIGNALED( fork_status ) ) ;
		g_log->Save ( g_log , "6:%d" , WIFSTOPPED( fork_status ) ) ;
		g_log->Save ( g_log , "7:%d" , WIFCONTINUED( fork_status ) ) ;

		printf ( "pgm:<%s> parent wait succ , will remake , error\n" , Pgm_Name ) ;
		printf ( "1:%d\n" , WEXITSTATUS( fork_status ) ) ;
		printf ( "2:%d\n" , WTERMSIG( fork_status ) ) ;
		printf ( "3:%d\n" , WSTOPSIG( fork_status ) ) ;
		printf ( "4:%d\n" , WIFEXITED( fork_status ) ) ;
		printf ( "5:%d\n" , WIFSIGNALED( fork_status ) ) ;
		printf ( "6:%d\n" , WIFSTOPPED( fork_status ) ) ;
		printf ( "7:%d\n" , WIFCONTINUED( fork_status ) ) ;

		sleep ( 5 ) ;
	}
	S_Http_HomeLock_Delete ( & homelock ) ;
	S_Log_tool_Delete ( & g_log ) ;
	return 0 ;
}
