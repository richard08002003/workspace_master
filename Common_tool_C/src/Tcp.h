/*
 * Tcp.h
 *
 *  Created on: 2017/5/28
 *      Author: richard
 */

#ifndef TCP_H_
#define TCP_H_

/**************************************************
 * 無使用SSL/TLS安全加密，為直接的TCP/IP連線（Client）*
 **************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/wait.h>

#include "Common_Tool.h"
#include "Epoll.h"

typedef struct _S_Tcp_server 			S_Tcp_server ;
//typedef struct _S_Tcp_ssl_server 	S_Tcp_ssl_server ;
typedef struct _S_Tcp_client 			S_Tcp_client ;

// TCP Server //
struct _S_Tcp_server {
	void *i_private ;

	int (*Server_begin) ( S_Tcp_server *obj , int port , UInt listen_num ) ;
	int (*Rtn_srv_fd) ( S_Tcp_server *obj ) ;
	int (*Accept) ( S_Tcp_server *obj ) ;
	int (*Rtn_acpt_fd) ( S_Tcp_server *obj ) ;
	int (*Set_read_timeout) ( S_Tcp_server *obj , int timeout ) ;

	// Send
	int (*Send_bin) ( S_Tcp_server *obj , CStr tcp_msg , int tcp_msg_bytes ) ;	// Binary
	int (*Send_str) ( S_Tcp_server *obj , CStr tcp_msg ) ;													// String

	// Read
	int (*Read_bin) ( S_Tcp_server *obj ) ;							// Binary
	UStr (*Rtn_read_bin) ( S_Tcp_server *obj ) ;

	int (*Read_str) ( S_Tcp_server *obj ) ;							// String
	Str (*Rtn_read_str) ( S_Tcp_server *obj ) ;

	int (*Close_srv_fd) ( S_Tcp_server *obj ) ;
	int (*Close_acpt_fd) ( S_Tcp_server *obj ) ;
} ;
extern S_Tcp_server *S_Tcp_server_tool_New ( void ) ;
extern void S_Tcp_server_tool_Delete ( S_Tcp_server **obj ) ;

#if 0
// 具備 TLS/SSL 加密 TCP Server
/************************************************************************************************
 * < SSL 憑證 >																																															*
 * 1. 安裝/更新Openssl:																																										*
 *	  sudo apt-get install openssl																																						*
 * 2. 必須要先產生一個Certificate憑證，在終端機上輸入以下指令：																			*
 *	  openssl req -x509 -nodes -days 365 -newkey rsa:1024 -keyout mycert.pem -out mycert.pem				*
 ************************************************************************************************/
struct _S_Tcp_ssl_server {
	void *i_private ;

	int (*Server_begin) ( S_Tcp_ssl_server *obj , int port , CStr cer_path , UInt listen_num ) ;
	int (*Rtn_srv_fd) ( S_Tcp_ssl_server *obj ) ;
	int (*Accept) ( S_Tcp_ssl_server *obj ) ;
	int (*Rtn_acpt_fd) ( S_Tcp_ssl_server *obj ) ;
	int (*Set_read_timeout) ( S_Tcp_ssl_server *obj , int timeout ) ;

	// Send
	int (*Send_bin) ( S_Tcp_ssl_server *obj , CUStr tcp_msg , int tcp_msg_bytes ) ;	// Binary
	int (*Send_str) ( S_Tcp_ssl_server *obj , CStr tcp_msg ) ;			// String

	// Read
	int (*Read_bin) ( S_Tcp_ssl_server *obj ) ;							// Binary
	UStr (*Rtn_read_bin) ( S_Tcp_ssl_server *obj ) ;

	int (*Read_str) ( S_Tcp_ssl_server *obj ) ;							// String
	Str (*Rtn_read_str) ( S_Tcp_ssl_server *obj ) ;

	int (*Close_srv_fd) ( S_Tcp_ssl_server *obj ) ;
	int (*Close_acpt_fd) ( S_Tcp_ssl_server *obj ) ;
} ;
extern S_Tcp_ssl_server *S_Tcp_ssl_server_tool_New ( void ) ;
extern void S_Tcp_ssl_server_tool_Delete ( S_Tcp_ssl_server **obj ) ;
#endif


// TCP Client //
struct _S_Tcp_client {
	void *i_private ;

	int (*Setup) ( S_Tcp_client *obj , CStr ip , CStr port , int timeout ) ;
	int (*Connect) ( S_Tcp_client *obj ) ;
	int (*Rtn_sock_fd) ( S_Tcp_client *obj ) ;
	int (*Rtn_timeout) ( S_Tcp_client *obj ) ;
	int (*Close) ( S_Tcp_client *obj ) ;

	/// Send
	int (*Send_bin) ( S_Tcp_client *obj , CStr tcp_msg ) ;
	int (*Send_str) ( S_Tcp_client *obj , CStr tcp_msg ) ;

	// Read
	int (*Read_bin) ( S_Tcp_client *obj ) ;
	UStr (*Rtn_read_bin) ( S_Tcp_client *obj ) ;
	int (*Rtn_read_bin_bytes) ( S_Tcp_client *obj ) ;
	int (*Read_str) ( S_Tcp_client *obj ) ;
	Str (*Rtn_read_str) ( S_Tcp_client *obj ) ;
	int (*Rtn_read_str_bytes) ( S_Tcp_client *obj ) ;

	// SSL : SSLv23_client_method()
	int (*Ssl_connect) ( S_Tcp_client *obj ) ;
	int (*Ssl_send_bin) ( S_Tcp_client *obj , CUStr msg , int msg_bytes ) ;
	int (*Ssl_send_str) ( S_Tcp_client *obj , CStr msg ) ;
	int (*Ssl_read_str) ( S_Tcp_client *obj ) ;
	Str (*Rtn_ssl_read_str) ( S_Tcp_client *obj ) ;
	int (*Ssl_read_bin) ( S_Tcp_client *obj ) ;
	UStr (*Rtn_ssl_read_bin) ( S_Tcp_client *obj ) ;
	int (*Close_ssl) ( S_Tcp_client *obj ) ;
} ;
extern S_Tcp_client *S_Tcp_client_tool_New ( void ) ;
extern void S_Tcp_client_tool_Delete ( S_Tcp_client **obj ) ;


#if 0	// 測試
int main ( ) {
	//  new tool obj
	S_Tcp_client *tcp_client = S_Tcp_client_tool_New ( ) ;
	if ( NULL == tcp_client ) {
		return - 1 ;
	}

	// Set ip , port & timeout
	int rtn = tcp_client->Setup ( tcp_client , "101.231.204.80" , "5000" , 2 ) ;
	printf ( "Tcp_Setup rtn = %d\n" , rtn ) ;

	// create socket and connect to server
	rtn = tcp_client->Connect ( tcp_client ) ;

	// send
	rtn = tcp_client->Send ( tcp_client , "test" ) ;
	printf ( "Tcp_Send = %d\n" , rtn ) ;

	// close socket
	tcp_client->Close ( tcp_client ) ;

	// delete tcp tool
	S_Tcp_client_Delete ( & tcp_client ) ;
}
#endif



#endif /* TCP_H_ */
