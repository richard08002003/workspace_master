/*
 * IPC.h
 *
 *  Created on: 2016/4/19
 *      Author: richard
 */

#ifndef IPC_H_
#define IPC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/un.h>
#include <sys/socket.h>
#include "Common_Tool.h"
#include "Epoll.h"			// 用於監控是否斷線

// Server //
typedef struct _S_Ipc_server S_Ipc_server;
struct _S_Ipc_server {
	void *i_private ;

	int (*Server_begin) ( S_Ipc_server *obj , CStr ipc_path , UInt listen_num ) ;	// 建立IPC Server
	int (*Rtn_srv_fd) ( S_Ipc_server *obj ) ;										// 回吐IPC Server fd
	int (*Accept) ( S_Ipc_server *obj ) ;											// Accept client
	int (*Rtn_acpt_fd) ( S_Ipc_server *obj ) ;										// 回吐Accept fd
	int (*Set_read_timeout) ( S_Ipc_server *obj , int timeout ) ;					// 設定讀取timeout
	int (*Write) ( S_Ipc_server *obj , CStr str ) ;									// 寫(會加入<S>08X)
	int (*Read) ( S_Ipc_server *obj , char **data , int *data_sz ) ;				// 讀
//	int (*Read) ( S_Ipc_server *obj) ;												// 讀
//	Str (*Rtn_read_data) ( S_Ipc_server *obj ) ;									// 回吐讀的資料
	void (*Free_read_buf) ( S_Ipc_server *obj ) ;									// 2018/01/24
	int (*Close_srv_fd) ( S_Ipc_server *obj ) ;										// 關閉Server
	int (*Close_acpt_fd) ( S_Ipc_server *obj ) ;									// 關閉Accept
} ;
extern S_Ipc_server * S_Ipc_Server_New ( void ) ;
extern void S_Ipc_Server_Delete ( S_Ipc_server** obj ) ;

// Client //
typedef struct _S_Ipc_client S_Ipc_client;
struct _S_Ipc_client {
	void *i_private ;

	int (*Connect) ( S_Ipc_client *obj , CStr ipc_path ) ;							// 連線至Server端
	int (*Rtn_fd) ( S_Ipc_client *obj ) ;											// 回吐fd
	Str (*Rtn_ipc_path)( S_Ipc_client *obj ) ;										// 回吐ipc server path	(2018/04/03)

	int (*Detect_connect) ( S_Ipc_client *obj ) ;									// 偵測連線狀態（2018/05/09）

	int (*Re_connect) ( S_Ipc_client*obj ) ;										// 重新連線
	int (*Set_read_timeout) ( S_Ipc_client *obj , int timeout ) ;					// 設定讀取timeout
	int (*Write) ( S_Ipc_client*obj , CStr str ) ;									// 寫
	int (*Read) ( S_Ipc_client *obj , char **data , int *data_sz ) ;				// 讀
//	Str (*Rtn_read_data) ( S_Ipc_client*obj ) ;										// 回吐讀的資料
	int (*Close) ( S_Ipc_client*obj ) ;												// 關閉連線
} ;
extern S_Ipc_client * S_Ipc_client_New ( void ) ;
extern void S_Ipc_client_Delete ( S_Ipc_client** obj ) ;


#if 0 // 測試程式
// Server
int main ( ) {
	printf ( "Hello\n" ) ;

	S_Ipc_server *srv_obj = S_Ipc_Server_New ( ) ;
	if ( NULL == srv_obj ) {
		printf ( "S_Ipc_Server_New error\n" ) ;
		return - 1 ;
	}

	char ipc_path [ ] = "./test.ipc" ;
	ULInt listen_num = 256 ;

	int rtn = srv_obj->Server_begin ( srv_obj , ipc_path , listen_num ) ;
	if ( D_success != rtn ) {
		printf ( "Server_begin error\n" ) ;
		return - 2 ;
	}

	int fd = srv_obj->Rtn_srv_fd ( srv_obj ) ;
	printf ( "fd = %d\n" , fd ) ;

	printf ( "Will Accept...\n" ) ;
	rtn = srv_obj->Accept ( srv_obj ) ;
	if ( D_success != rtn ) {
		printf ( "Accept error\n" ) ;
		return - 3 ;
	}
	int acpt_fd = srv_obj->Rtn_acpt_fd ( srv_obj ) ;
	printf ( "acpt_fd = %d\n" , acpt_fd ) ;

	S_Ipc_Server_Delete ( & srv_obj ) ;
	return 0 ;
}

// Client
int main ( ) {
	printf ( "Hello\n" ) ;

	S_Ipc_client *clt_obj = S_Ipc_client_New ( ) ;
	if ( NULL == clt_obj ) {
		printf ( "S_Ipc_client_New error\n" ) ;
		return - 1 ;
	}

	char ipc_path [ ] = "./test.ipc" ;
	int rtn = clt_obj->Connect ( clt_obj , ipc_path ) ;
	if ( D_success != rtn ) {
		printf("Connect error\n") ;
		return -2 ;
	}

	int fd = clt_obj->Rtn_fd ( clt_obj ) ;
	printf ( "fd = %d\n" , fd ) ;

	S_Ipc_client_Delete ( & clt_obj ) ;
	return 0 ;
}
#endif


#endif /* IPC_H_ */
