/*
 * My_Sqlite3.h
 *
 *  Created on: 2017年11月15日
 *      Author: richard
 */

#ifndef SRC_MY_SQLITE3_H_
#define SRC_MY_SQLITE3_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sqlite3.h>

#include "Common_Tool.h"

typedef struct _S_Sqlite3 S_Sqlite3 ;
struct _S_Sqlite3 {
	void *i_private ;
	int (*Open_db) ( S_Sqlite3 *obj , CStr db_path ) ;														// 開啟資料庫
	int (*Close_db) ( S_Sqlite3 *obj ) ;																	// 關閉資料庫
	int (*Insert_into_table) ( S_Sqlite3 *obj , CStr table_name , CStr fieldname , CStr value ) ;			// Insert
	int (*Select_from) ( S_Sqlite3 *obj , CStr data , CStr table_name ) ;									// Select From
	int (*Select_from_where) ( S_Sqlite3 *obj , CStr table_name , CStr data , CStr where ) ;				// Select From Where
	int (*Delete_from_where) ( S_Sqlite3 *obj , CStr table_name , CStr where ) ;							// Delete From Where
	int (*Update_set_where) ( S_Sqlite3 *obj , CStr table_name , CStr field_name_value , CStr where ) ;		// Update Set Where

	Str (*Rtn_err_msg) ( S_Sqlite3 *obj ) ;			// 回吐錯誤信息
	Str (*Rtn_callback_msg) ( S_Sqlite3 *obj ) ;	// 回吐Callback信息
} ;
extern S_Sqlite3 *S_Sqlite3_tool_New ( void ) ;
extern void S_Sqlite3_tool_Delete ( S_Sqlite3 **obj ) ;

#if 0 // 測試程式
int main ( ) {
	S_Sqlite3 *sql_obj = S_Sqlite3_tool_New ( ) ;
	if ( NULL == sql_obj ) {
		printf ( "S_Sqlite3_tool_New() error\n" ) ;
		return - 1 ;
	}
	printf ( "S_Sqlite3_tool_New() success\n" ) ;

	int rtn = sql_obj->Open_db ( sql_obj , "/media/richard/richard_hdd/workspace_CUP/Common_tool_C/db_dir/Test.db" ) ;
	if ( rtn != D_success ) {
		printf ( "Open_db error, rtn = %d\n" , rtn ) ;
		S_Sqlite3_tool_Delete ( & sql_obj ) ;
		return - 2 ;
	}
	printf ( "Open_db success\n" ) ;

//	rtn = sql_obj->Insert_into_table ( sql_obj , "User" , "id , name" , "1 , 'Richard'" ) ;
	rtn = sql_obj->Select_from ( sql_obj , "*" , "User" ) ;

	sql_obj->Close_db ( sql_obj ) ;

	S_Sqlite3_tool_Delete ( & sql_obj ) ;
	return 0 ;
}
#endif

#endif /* SRC_MY_SQLITE3_H_ */
