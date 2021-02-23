/*
 * File.h
 *
 *  Created on: 2017/5/9
 *      Author: richard
 */

#ifndef FILE_H_
#define FILE_H_

/****************
 * 關於處理檔案部份	*
 ****************/
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "Common_Tool.h"

typedef struct _S_File S_File ;
struct _S_File {
	void *i_private ;
	// read file
	int (*Read_str_from_file) ( S_File *obj , CChar *file_path ) ;	// string
	Str (*Rtn_str_from_file) ( S_File *obj ) ;

	int (*Read_bin_from_file) ( S_File *obj , CChar *file_path ) ;	// binary
	UStr (*Rtn_bin_from_file) ( S_File *obj ) ;

	LInt (*Rtn_file_sz) ( S_File *obj ) ;	// 回吐檔案大小

	// write file
	int (*Write_str_to_file_w) ( S_File *obj , CChar *file_path , Str data ) ;
	int (*Write_str_to_file_a) ( S_File *obj , CChar *file_path , Str data ) ;

	int (*Write_bin_to_file_w) ( S_File *obj , CChar *file_path , void *p_data , int sz ) ;
	int (*Write_bin_to_file_a) ( S_File *obj , CChar *file_path , void *p_data , int sz ) ;

	// 檔案鎖 ( 2017/09/19 新增)
	int (*File_lock) ( S_File *obj , CStr file_path ) ;
	int (*Rtn_lock_fd) ( S_File *obj ) ;
	int (*File_unlock) ( S_File *obj , int fd ) ;
} ;

extern S_File *S_File_tool_New ( void ) ;
extern void S_File_tool_Delete ( S_File **obj ) ;


#if 0	// 測試程式
int main() {
	int ct = 0 ;
	for ( ct = 0 ; ct < 10000 ; ct ++ ) {
		S_File *file_obj = S_File_tool_New ( ) ;
		if ( NULL == file_obj ) {
			printf ( "S_File_tool_New error\n" ) ;
			return - 1 ;
		}

		// 讀取Binary檔案
		char file_path [ ] = "/media/richard/richard_hdd/Richard/UBike/大陸票證/銀聯Level3/系統參數檔__文件/Landi联迪E5X0_银联S3POS_V400218_20161101_sign.para" ;

		int rtn = file_obj->Read_bin_from_file ( file_obj , file_path ) ;
		printf ( "rtn = %d\n" , rtn ) ;

		int sz = file_obj->Rtn_file_sz ( file_obj ) ;
		printf ( "sz = %d\n" , sz ) ;

		UChar* data = file_obj->Rtn_bin_from_file ( file_obj ) ;

		int i = 0 ;
		for ( i = 0 ; i < sz ; i ++ ) {
			printf ( "%02X " , data [ i ] ) ;
		}
		printf("\n") ;

		S_File_tool_Delete ( & file_obj ) ;
	}
	return 0 ;
}
#endif


#endif /* FILE_H_ */
