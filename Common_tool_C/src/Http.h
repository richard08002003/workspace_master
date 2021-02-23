/*
 * Http.h
 *
 *  Created on: 2017年9月6日
 *      Author: richard
 */

#ifndef INC_HTTP_H_
#define INC_HTTP_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <netdb.h>
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

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <openssl/md5.h>
#include <openssl/err.h>
#include <openssl/des.h>
#include <openssl/err.h>

#include <ctype.h>
#include <stdarg.h>

#include "Common_Tool.h"
#include "Epoll.h"

// Client
typedef struct _S_Https S_Https ;
struct _S_Https {
	void *i_private ;

	int (*Connect) ( S_Https *obj , CStr ip , CStr port ) ;					// 連線
	int (*Rtn_sock_fd) ( S_Https *obj ) ;									// 回吐socket fd
	Str (*Rtn_host_name) ( S_Https *obj ) ;
	Str (*Rtn_ip) ( S_Https *obj ) ;

	int (*Close_sock_fd) ( S_Https *obj ) ;									// 關閉連線

	// Write
	int (*Write_bin) ( S_Https *obj , CUStr msg , int msg_bytes ) ;			// 寫Binary資料
	int (*Write_str) ( S_Https *obj , CStr msg ) ;							// 寫string資料

	// Read
	int (*Read_to_file) ( S_Https *obj , CStr file_path , bool keepalive ) ;// 讀資料到檔案中
	int (*Read_str) ( S_Https *obj , bool keepalive ) ;						// 讀string資料
	Str (*Rtn_read_str) ( S_Https *obj ) ;									// 回吐
	int (*Read_bin) ( S_Https *obj , bool keepalive ) ;						// 讀binary資料
	UStr (*Rtn_read_bin) ( S_Https *obj ) ;

#if 0
	// URL
	int (*Url_decode) ( S_Https *obj , CStr input ) ;						// url decode
	Str (*Rtn_url_decode) ( S_Https *obj ) ;
	int (*Url_encode) ( S_Https *obj , CStr input ) ;						// url encode
	Str (*Rtn_url_encode) ( S_Https *obj ) ;
#endif

	// SSL : SSLv23_client_method()
	int (*Ssl_connect) ( S_Https *obj ) ;									// ssl 連線
	int (*Ssl_write_bin) ( S_Https *obj , CUStr msg , int msg_bytes ) ;		// ssl 寫binary資料
	int (*Ssl_write_str) ( S_Https *obj , CStr msg ) ;						// ssl 寫string資料
	int (*Ssl_read_str) ( S_Https *obj ) ;									// ssl 讀string資料
	Str (*Rtn_ssl_read_str) ( S_Https *obj ) ;
	int (*Ssl_read_bin) ( S_Https *obj ) ;									// ssl 讀binary資料
	UStr (*Rtn_ssl_read_bin) ( S_Https *obj ) ;
	int (*Close_ssl) ( S_Https *obj ) ;										// 關閉ssl
};
extern S_Https *S_Https_tool_New ( void ) ;
extern void S_Https_tool_Delete ( S_Https **obj ) ;

// 設定讀寫的Timeout
typedef struct _S_RdWrTmO S_RdWrTmO ;
struct _S_RdWrTmO {
	int (*Set_sockopt) ( S_RdWrTmO *obj , int iFd , UInt wr_ms , UInt rd_ms ) ;
} ;
extern S_RdWrTmO *S_RdWrTmO_tool_New ( void ) ;
extern void S_RdWrTmO_tool_Delete ( S_RdWrTmO **obj ) ;


#endif /* INC_HTTP_H_ */
