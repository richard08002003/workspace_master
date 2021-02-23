/*
 * MyLog.h
 *
 *  Created on: 2015/7/9
 *      Author: richard
 */
#ifndef MYLOG_H_
#define MYLOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>
#include <time.h>
#include <sys/timeb.h>
#include "Common_Tool.h"
#include "File.h"

typedef struct _S_Log S_Log ;
struct _S_Log {
	void *i_private ;
	int (*Initial) ( S_Log *obj , CStr pgm_name ) ;		// 初始化
	void (*Close) ( S_Log *obj ) ;						// 關閉 Log fd
	Str (*Rtn_Pgm_name) ( S_Log *obj ) ;				// 回吐程式名稱
	int (*Save) ( S_Log *obj , CStr format , ... ) ;	// 存log
	Str (*Rtn_lock_path) ( S_Log *obj ) ;				// 回吐lock路徑
} ;
extern S_Log *S_Log_tool_New ( void ) ;
extern void S_Log_tool_Delete ( S_Log **obj ) ;


#if 0 //測試程式
S_Log *g_log ;
int main ( ) {
	g_log = S_Log_tool_New ( ) ;
	if ( NULL == g_log ) {
		printf("S_Log_tool_New Error\n");
		return - 1 ;
	}
	int rtn = g_log->Initial ( g_log , DF_Pgm_Name ) ;
	if ( D_success != rtn ) {
		printf("Initial Error\n") ;
		return -2 ;
	}
	g_log->Save ( g_log , "Test = %s" , "123" ) ;
	S_Log_tool_Delete ( & g_log ) ;
	return 0 ;
}
#endif

#endif /* MYLOG_H_ */
