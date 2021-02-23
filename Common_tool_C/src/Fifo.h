/*
 * Fifo.h
 *
 *  Created on: 2017年12月7日
 *      Author: richard
 */

#ifndef SRC_FIFO_H_
#define SRC_FIFO_H_

// 命名管線 //
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "Common_Tool.h"

typedef struct _S_Fifo S_Fifo ;
struct _S_Fifo {
	void *i_private ;
	int (*Mkfifo) ( S_Fifo *obj , CStr path_name , mode_t mode ) ;// 創建一個FIFO文件

	/************************************************************************************************************
	 * O_RDONLY：打開將會調用阻塞，除非有另外一個進程以寫的方式打開同一個FIFO，否則一直等待								*
	 * O_WRONLY：打開將會調用阻塞，除非有另外一個進程以讀的方式打開同一個FIFO，否則一直等待								*
	 * O_RDONLY| O_NONBLOCK：如果此時沒有其他進程以寫的方式打開FIFO，此時打開也會成功返回，此時FIFO被讀打開，而不會返回錯誤	*
	 * O_WRONLY| O_NONBLOCK：立即返回，如果此時沒有其他進程以讀的方式打開，打開會失敗打開，此時FIFO沒有被打開，返回-1		*
	 ************************************************************************************************************/
	int (*Open) ( S_Fifo *obj , CStr path_name , int flags ) ;	// 打開FIFO
	void (*Close) ( S_Fifo *obj ) ;								// 關閉FIFO
	int (*Read) ( S_Fifo *obj ) ;								// 讀資料
	Str (*Rtn_read_data) ( S_Fifo *obj ) ;						// 回吐資料
	int (*Write) ( S_Fifo *obj , CStr msg ) ;					// 寫資料
} ;
extern S_Fifo *S_Fifo_New ( void ) ;
extern void S_Fifo_Delete ( S_Fifo **obj ) ;

#if 0	// 測試程式
// Write //
int main (void ) {
	printf ( "######FIFO Write Test######\n" ) ;
	// write object //
	S_Fifo *fifo_obj_1 = S_Fifo_New ( ) ;
	if ( NULL == fifo_obj_1 ) {
		printf ( "fifo_obj_1 S_Fifo_New error\n" ) ;
		return - 1 ;
	}
	printf ( "fifo_obj_1 S_Fifo_New success\n" ) ;

	char fifo_path [ ] = "/tmp/test.fifo" ;
	unlink ( fifo_path ) ;

	int rtn = fifo_obj_1->Mkfifo ( fifo_obj_1 , fifo_path , 0777 ) ;
	if ( rtn != D_success ) {
		printf ( "fifo_obj_1->Mkfifo error\n" ) ;
		S_Fifo_Delete ( & fifo_obj_1 ) ;
		return - 3 ;
	}
	printf ( "fifo_obj_1->Mkfifo success\n" ) ;

	// open fifo //
	rtn = fifo_obj_1->Open ( fifo_obj_1 , fifo_path , O_WRONLY ) ;	// 為block模式，必須要同時有進程開啟讀取能順利運行
	if ( rtn != D_success ) {
		printf ( "fifo_obj_1->Open error\n" ) ;
		S_Fifo_Delete ( & fifo_obj_1 ) ;
		return - 4 ;
	}
	printf ( "fifo_obj_1->Open success\n" ) ;

	while ( 1 ) {
		printf ( "\nEnter Key in:" ) ;
		char buf [ 100 ] = { 0 } ;
		fgets ( buf , sizeof ( buf ) , stdin ) ;

		printf ( "Will Write to fifo\n" ) ;
		// write //
		rtn = fifo_obj_1->Write ( fifo_obj_1 , "Hello process~~" ) ;
		if ( rtn != D_success ) {
			printf ( "fifo_obj_1->Write error\n" ) ;
			S_Fifo_Delete ( & fifo_obj_1 ) ;
			return - 6 ;
		}
	}

	// close //
	fifo_obj_1->Close ( fifo_obj_1 ) ;

	S_Fifo_Delete ( & fifo_obj_1 ) ;
	return 0 ;
}

// Read //
int main ( void) {
	printf ( "######FIFO Read Test######\n" ) ;
	// read object //
	S_Fifo *fifo_obj_2 = S_Fifo_New ( ) ;
	if ( NULL == fifo_obj_2 ) {
		printf ( "fifo_obj_2 S_Fifo_New error\n" ) ;
		return - 2 ;
	}
	printf ( "fifo_obj_2 S_Fifo_New success\n" ) ;

	int rtn = fifo_obj_2->Open ( fifo_obj_2 , "/tmp/test.fifo" , O_RDONLY ) ;
	if ( rtn != D_success ) {
		printf ( "fifo_obj_2->Open error\n" ) ;
		S_Fifo_Delete ( & fifo_obj_2 ) ;
		return - 5 ;
	}
	printf ( "fifo_obj_2->Open success\n" ) ;

	while ( 1 ) {
		printf ( "\nEnter Key in:" ) ;
		char buf [ 100 ] = { 0 } ;
		fgets ( buf , sizeof ( buf ) , stdin ) ;
		printf ( "Will Read from fifo\n" ) ;

		// read //
		rtn = fifo_obj_2->Read ( fifo_obj_2 ) ;
		if ( rtn != D_success ) {
			printf ( "fifo_obj_2->Read error\n" ) ;
			S_Fifo_Delete ( & fifo_obj_2 ) ;
			return - 7 ;
		}

		Str read_data = fifo_obj_2->Rtn_read_data ( fifo_obj_2 ) ;
		printf ( "read_data = [%s]\n" , read_data ) ;
	}

	// close //
	fifo_obj_2->Close ( fifo_obj_2 ) ;
	S_Fifo_Delete ( & fifo_obj_2 ) ;
	return 0 ;
}
#endif

#endif /* SRC_FIFO_H_ */
