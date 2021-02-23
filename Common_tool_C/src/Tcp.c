/*
 * Tcp.c
 *
 *  Created on: 2017/5/28
 *      Author: richard
 */

#include "Tcp.h"
#include "Setup.h"	// log 用

// Server //
typedef struct _S_server_private_data {
	int i_timeout ;
	int i_port ;
	int i_socket_fd ;
	int i_acpt_fd ;									// accept fd
	UInt i_listen_num ;						// listen number

	UStr mlc_read_bin ;						// Binary
	LInt i_read_bin_bytes ;

	Str mlc_read_str ;							// String
	LInt i_read_str_bytes ;

	UStr mlc_ssl_read_bin ;				// Binary
	LInt i_ssl_read_bin_bytes ;
	Str mlc_ssl_read_str ;					// String
	LInt i_ssl_read_str_bytes ;

} S_server_private_data ;

// 僅供內部使用 //
static int S_Server_begin ( S_Tcp_server *obj , int port , UInt listen_num ) ;
static int S_Server_Rtn_srv_fd ( S_Tcp_server *obj ) ;
static int S_Server_Accept ( S_Tcp_server *obj ) ;
static int S_Server_Rtn_acpt_fd ( S_Tcp_server *obj ) ;
static int S_Server_Set_read_timeout ( S_Tcp_server *obj , int timeout ) ;
// Send
static int S_Server_Send_bin ( S_Tcp_server *obj , CStr tcp_msg , int tcp_msg_bytes ) ;
static int S_Server_Send_str ( S_Tcp_server *obj , CStr tcp_msg ) ;
// Read
static int S_Server_Read_bin ( S_Tcp_server *obj ) ;
static UStr S_Server_Rtn_read_bin ( S_Tcp_server *obj ) ;
static int S_Server_Read_str ( S_Tcp_server *obj ) ;
static Str S_Server_Rtn_read_str ( S_Tcp_server *obj ) ;
static int S_Server_Close_srv_fd ( S_Tcp_server *obj ) ;
static int S_Server_Close_acpt_fd ( S_Tcp_server *obj ) ;
// 僅供內部使用 //

static int S_Server_begin ( S_Tcp_server *obj , int port , UInt listen_num ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! listen_num ) ) {
		return - 1 ;
	}
	S_server_private_data *p_private = ( S_server_private_data * ) obj->i_private ;

	struct sockaddr_in saddr ;
	//Socket() :
	p_private->i_socket_fd = socket ( AF_INET , SOCK_STREAM , 0 ) ;
	if ( p_private->i_socket_fd < 0 ) {
		return - 2 ;
	}
	p_private->i_port = port ;

	memset ( & saddr , 0 , sizeof ( saddr ) ) ;
	saddr.sin_family = AF_INET ;
	saddr.sin_port = htons ( p_private->i_port ) ;
	saddr.sin_addr.s_addr = INADDR_ANY ;
	// Make non-blocking Socket：函數fnctl() : 改變已打開的檔的屬性，return -> 0  成功 , return -1 -> 失敗
	int flags = fcntl ( p_private->i_socket_fd , F_GETFL ) ;
	if ( flags < 0 ) {
		return - 3 ;
	}
	flags = flags | O_NONBLOCK ;
	int rtn = fcntl ( p_private->i_socket_fd , F_SETFL , flags ) ;
	if ( rtn < 0 ) {
		return - 4 ;
	}

	// reuse bind addr
	int reuse = 1 ;
	if ( setsockopt ( p_private->i_socket_fd , SOL_SOCKET , SO_REUSEADDR , & reuse , sizeof ( reuse ) ) < 0 ) {
		return - 5 ;
	}
	//Bind() :
	if ( bind ( p_private->i_socket_fd , ( struct sockaddr* ) & saddr , sizeof ( saddr ) ) < 0 ) {
		return - 6 ;
	}
	//Listen() :
	if ( listen ( p_private->i_socket_fd , listen_num ) < 0 ) {
		return - 7 ;
	}
	return D_success ;
}

static int S_Server_Rtn_srv_fd ( S_Tcp_server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_server_private_data *p_private = ( S_server_private_data * ) obj->i_private ;
	return p_private->i_socket_fd ;
}

static int S_Server_Accept ( S_Tcp_server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_server_private_data *p_private = obj->i_private ;

	socklen_t rmot_sckln = sizeof(struct sockaddr_in) ;
	struct sockaddr_in remote_addr = { 0 } ;
	p_private->i_acpt_fd = accept ( p_private->i_socket_fd , ( struct sockaddr * ) & remote_addr , & rmot_sckln ) ;
	if ( - 1 == p_private->i_acpt_fd ) {
		return - 2 ;
	}

	// send
	struct timeval set_time = { 0 } ;
	UInt write_ms = 10 ;
	UInt read_ms = 1 ;
	set_time.tv_sec = write_ms / 1000 ;
	set_time.tv_usec = ( write_ms - ( set_time.tv_sec * 1000 ) ) * 1000 ;
	if ( 0 != setsockopt ( p_private->i_acpt_fd , SOL_SOCKET , SO_SNDTIMEO , ( char * ) & set_time , sizeof(struct timeval) ) ) {
		return - 3 ;
	}

	// read
	set_time.tv_sec = read_ms / 1000 ;
	set_time.tv_usec = ( read_ms - ( set_time.tv_sec * 1000 ) ) * 1000 ;
	if ( 0 != setsockopt ( p_private->i_acpt_fd , SOL_SOCKET , SO_RCVTIMEO , ( char * ) & set_time , sizeof(struct timeval) ) ) {
		return - 4 ;
	}
	return D_success ;
}

static int S_Server_Rtn_acpt_fd ( S_Tcp_server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_server_private_data *p_private = obj->i_private ;
	return p_private->i_acpt_fd ;
}

static int S_Server_Set_read_timeout ( S_Tcp_server *obj , int timeout ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! timeout ) ) {
		return - 1 ;
	}
	S_server_private_data *p_private = obj->i_private ;
	if ( timeout < 0 ) {
		return - 2 ;
	}
	p_private->i_timeout = timeout ;
	return D_success ;
}

static int S_Server_Send_bin ( S_Tcp_server *obj , CStr tcp_msg , int tcp_msg_bytes ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! tcp_msg ) || ( ! tcp_msg_bytes ) ) {
		return - 1 ;
	}
	S_server_private_data *p_private = ( S_server_private_data * ) obj->i_private ;
	int rtn = write ( p_private->i_socket_fd , tcp_msg , tcp_msg_bytes ) ;
	if ( 0 >= rtn ) {
		return - 2 ;
	}
	return D_success ;
}

static int S_Server_Send_str ( S_Tcp_server *obj , CStr tcp_msg ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! tcp_msg ) ) {
		return - 1 ;
	}
	S_server_private_data *p_private = ( S_server_private_data * ) obj->i_private ;
	int rtn = write ( p_private->i_socket_fd , tcp_msg , strlen ( tcp_msg ) ) ;
	if ( - 1 == rtn ) {
		return - 2 ;
	}
	return D_success ;
}

static int S_Server_Read_bin ( S_Tcp_server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_server_private_data *p_private = ( S_server_private_data * ) obj->i_private ;
	if ( p_private->mlc_read_bin != NULL ) {
		free ( p_private->mlc_read_bin ) ;
		p_private->mlc_read_bin = NULL ;
	}
	while ( 1 ) {
		errno = 0 ;
		UChar buf [ 8191 + 1 ] = { 0 } ;
		int rtn = read ( p_private->i_socket_fd , buf , sizeof ( buf ) - 1 ) ;

		if ( - 1 == rtn ) {
			if ( EAGAIN == errno ) {
				continue ;
			}
			return - 2 ;
		} else if ( 0 == rtn ) {
			break ;
		}
		p_private->mlc_read_bin = ( UStr ) realloc ( p_private->mlc_read_bin , p_private->i_read_bin_bytes + rtn + 1 ) ;
		if ( p_private->mlc_read_bin == NULL ) {
			return - 3 ;
		}
		memcpy ( p_private->mlc_read_bin + p_private->i_read_bin_bytes , buf , rtn ) ;
		p_private->i_read_bin_bytes += rtn ;
	}
	return D_success ;
}

static UStr S_Server_Rtn_read_bin ( S_Tcp_server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_server_private_data *p_private = ( S_server_private_data * ) obj->i_private ;
	return p_private->mlc_read_bin ;
}

static int S_Server_Read_str ( S_Tcp_server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_server_private_data *p_private = ( S_server_private_data * ) obj->i_private ;
	if ( p_private->mlc_read_str != NULL ) {
		free ( p_private->mlc_read_str ) ;
		p_private->mlc_read_str = NULL ;
	}

	S_Epoll *epoll_tool = S_Epoll_New ( ) ;
	if ( NULL == epoll_tool ) {
		return - 2 ;
	}
	int rtn = epoll_tool->Detect_hup ( epoll_tool , p_private->i_socket_fd ) ;
	S_Epoll_Delete ( & epoll_tool ) ;
	if ( rtn != 0 ) {
		return - 3 ;		//  斷線
	}

	p_private->i_read_str_bytes = 0 ; // 總量
	while ( 1 ) {
		errno = 0 ;
		char msg [ 8191 + 1 ] = { 0 } ;
		int rtn = read ( p_private->i_socket_fd , msg , sizeof ( msg ) - 1 ) ;
		if ( rtn == 0 ) {
			break ;
		} else if ( rtn == - 1 ) {
			if ( p_private->i_read_str_bytes > 0 ) {
				break ;
			}
			continue ;
		}

		p_private->mlc_read_str = ( char* ) realloc ( p_private->mlc_read_str , p_private->i_read_str_bytes + rtn + 1 ) ;
		if ( p_private->mlc_read_str == NULL ) {
			return - 5 ;
		}
		memcpy ( p_private->mlc_read_str + p_private->i_read_str_bytes , msg , rtn ) ;// 將資料接到上筆資料量後面
		( p_private->mlc_read_str ) [ p_private->i_read_str_bytes + rtn ] = 0 ;
		p_private->i_read_str_bytes += rtn ;
	}
	return D_success ;
}

static Str S_Server_Rtn_read_str ( S_Tcp_server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_server_private_data *p_private = ( S_server_private_data * ) obj->i_private ;
	return p_private->mlc_read_str ;
}

static int S_Server_Close_srv_fd ( S_Tcp_server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_server_private_data *p_private = ( S_server_private_data * ) obj->i_private ;
	if ( p_private->i_socket_fd > 0 ) {
		close ( p_private->i_socket_fd ) ;
		p_private->i_socket_fd = - 1 ;
	}
	return D_success ;
}

static int S_Server_Close_acpt_fd ( S_Tcp_server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_server_private_data *p_private = ( S_server_private_data * ) obj->i_private ;
	if ( p_private->i_acpt_fd > 0 ) {
		close ( p_private->i_acpt_fd ) ;
		p_private->i_acpt_fd = - 1 ;
	}
	return D_success ;
}

S_Tcp_server *S_Tcp_server_tool_New ( void ) {
	S_Tcp_server *p_tmp = ( S_Tcp_server* ) calloc ( 1 , sizeof(S_Tcp_server) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}

	S_server_private_data *i_private_data = ( S_server_private_data* ) calloc ( 1 , sizeof(S_server_private_data) ) ;
	if ( NULL == i_private_data ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}
	i_private_data->i_timeout = 2 ;	// 預設2秒
	i_private_data->i_port = - 1 ;
	i_private_data->i_socket_fd = - 1 ;
	i_private_data->i_acpt_fd = - 1 ;

	i_private_data->mlc_read_bin = NULL ;	// binary
	i_private_data->i_read_bin_bytes = 0 ;
	i_private_data->mlc_read_str = NULL ;		// string
	i_private_data->i_read_str_bytes = 0 ;
	p_tmp->i_private = ( void* ) i_private_data ;

	p_tmp->Server_begin = S_Server_begin ;
	p_tmp->Rtn_srv_fd = S_Server_Rtn_srv_fd ;
	p_tmp->Accept = S_Server_Accept ;
	p_tmp->Rtn_acpt_fd = S_Server_Rtn_acpt_fd ;
	p_tmp->Set_read_timeout = S_Server_Set_read_timeout ;

	// Send
	p_tmp->Send_bin = S_Server_Send_bin ;
	p_tmp->Send_str = S_Server_Send_str ;

	// Read
	p_tmp->Read_bin = S_Server_Read_bin ;
	p_tmp->Rtn_read_bin = S_Server_Rtn_read_bin ;
	p_tmp->Read_str = S_Server_Read_str ;
	p_tmp->Rtn_read_str = S_Server_Rtn_read_str ;

	// Close
	p_tmp->Close_srv_fd = S_Server_Close_srv_fd ;
	p_tmp->Close_acpt_fd = S_Server_Close_acpt_fd ;
	return p_tmp ;
}

void S_Tcp_server_tool_Delete ( S_Tcp_server **obj ) {
	S_server_private_data *p_private = ( * obj )->i_private ;
	free ( p_private->mlc_read_bin ) ;
	p_private->mlc_read_bin = NULL ;
	free ( p_private->mlc_read_str ) ;
	p_private->mlc_read_str = NULL ;
	free ( p_private->mlc_ssl_read_bin ) ;
	p_private->mlc_ssl_read_bin = NULL ;
	free ( p_private->mlc_ssl_read_str ) ;
	p_private->mlc_ssl_read_str = NULL ;
	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;
	free ( * obj ) ;
	* obj = NULL ;
	return ;
}

/*******************************************************************************************************************/
// 具備 TLS/SSL 加密 TCP Server //
#if 0
typedef struct _S_ssl_server_private_data {
	int i_timeout ;
	int i_port ;
	int i_socket_fd ;
	int i_acpt_fd ;				// accept fd
	UInt i_listen_num ;			// listen number

	UStr mlc_read_bin ;			// Binary
	LInt i_read_bin_bytes ;
	Str mlc_read_str ;			// String
	LInt i_read_str_bytes ;

	SSL *i_ssl ;				// SSL
	SSL_CTX *i_ctx ;

	UStr mlc_ssl_read_bin ;		// Binary
	LInt i_ssl_read_bin_bytes ;
	Str mlc_ssl_read_str ;		// String
	LInt i_ssl_read_str_bytes ;

} S_ssl_server_private_data ;

// 僅供內部使用 //
static int S_Ssl_init ( S_Tcp_ssl_server *obj , CStr cer_path ) ;
static int S_SSL_Server_begin ( S_Tcp_ssl_server *obj , int port , CStr cer_path , UInt listen_num ) ;
static int S_SSL_Server_Rtn_srv_fd ( S_Tcp_ssl_server *obj ) ;
static int S_SSL_Server_Accept ( S_Tcp_ssl_server *obj ) ;
static int S_SSL_Server_Rtn_acpt_fd ( S_Tcp_ssl_server *obj ) ;
static int S_SSL_Server_Set_read_timeout ( S_Tcp_ssl_server *obj , int timeout ) ;
// Send
static int S_SSL_Server_Send_bin ( S_Tcp_ssl_server *obj , CUStr tcp_msg ) ;
static int S_SSL_Server_Send_str ( S_Tcp_ssl_server *obj , CStr tcp_msg ) ;
// Read
static int S_SSL_Server_Read_bin ( S_Tcp_ssl_server *obj ) ;
static UStr S_SSL_Server_Rtn_read_bin ( S_Tcp_ssl_server *obj ) ;
static int S_SSL_Server_Read_str ( S_Tcp_ssl_server *obj ) ;
static Str S_SSL_Server_Rtn_read_str ( S_Tcp_ssl_server *obj ) ;
static int S_SSL_Server_Close_srv_fd ( S_Tcp_ssl_server *obj ) ;
static int S_SSL_Server_Close_acpt_fd ( S_Tcp_ssl_server *obj ) ;
// 僅供內部使用 //

static int S_Ssl_init ( S_Tcp_ssl_server *obj , CStr cer_path ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! cer_path ) ) {
		return - 1 ;
	}
	S_ssl_server_private_data *p_private = ( S_ssl_server_private_data * ) obj->i_private ;

	SSL_library_init ( ) ; 			// SSL庫的初始化
	OpenSSL_add_all_algorithms() ; 	//裝載&註冊所有密碼的資訊
	SSL_load_error_strings ( ) ; 		//載入所有錯誤信息

	p_private->i_ctx = SSL_CTX_new ( SSLv23_server_method ( ) ) ; // 使用SSLv23_server_method
	if ( p_private->i_ctx == NULL ) {
		return - 2 ;
	}
	// 載入憑證&驗證憑證 //
	//載入用戶數位簽章，此憑證用來發給client端，包含公鑰( public key )
	if ( SSL_CTX_use_certificate_file ( p_private->i_ctx , cer_path , SSL_FILETYPE_PEM ) <= 0 ) {
		SSL_CTX_free ( p_private->i_ctx ) ;
		return - 3 ;
	}
	//載入用戶的的私鑰( private key )
	if ( SSL_CTX_use_PrivateKey_file ( p_private->i_ctx , cer_path , SSL_FILETYPE_PEM ) <= 0 ) {
		SSL_CTX_free ( p_private->i_ctx ) ;
		return - 4 ;
	}
	//檢查用戶私鑰( private key )是否正確
	if ( ! SSL_CTX_check_private_key ( p_private->i_ctx ) ) {
		SSL_CTX_free ( p_private->i_ctx ) ;
		return - 5 ;
	}
	//載入憑證(用戶的憑證)
	if ( SSL_CTX_load_verify_locations ( p_private->i_ctx , cer_path , NULL ) <= 0 ) {
		return - 6 ;
	}
	//用來驗證client端的憑證（server端會驗證client端的憑證）
	SSL_CTX_set_verify ( p_private->i_ctx , SSL_VERIFY_PEER , NULL ) ;
	//設定最大的驗證用戶憑證的數目
	SSL_CTX_set_verify_depth ( p_private->i_ctx , 100 ) ;

	// SSL 初始化完成 //
	return D_success ;
}

static int S_SSL_Server_begin ( S_Tcp_ssl_server *obj , int port , CStr cer_path , UInt listen_num ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! cer_path ) || ( ! listen_num ) ) {
		return - 1 ;
	}
	S_ssl_server_private_data *p_private = ( S_ssl_server_private_data * ) obj->i_private ;

	int rtn = S_Ssl_init ( obj , cer_path ) ;
	if ( D_success != rtn ) {
		return - 2 ;
	}

	struct sockaddr_in saddr ;
	//Socket() :
	p_private->i_socket_fd = socket ( AF_INET , SOCK_STREAM , 0 ) ;
	if ( p_private->i_socket_fd < 0 ) {
		SSL_CTX_free ( p_private->i_ctx ) ;
		return - 3 ;
	}
	p_private->i_port = port ;

	memset ( & saddr , 0 , sizeof ( saddr ) ) ;
	saddr.sin_family = AF_INET ;
	saddr.sin_port = htons ( p_private->i_port ) ;
	saddr.sin_addr.s_addr = INADDR_ANY ;
	// Make non-blocking Socket：函數fnctl() : 改變已打開的檔的屬性，return -> 0  成功 , return -1 -> 失敗
	int flags = fcntl ( p_private->i_socket_fd , F_GETFL ) ;
	if ( flags < 0 ) {
		return - 4 ;
	}
	flags = flags | O_NONBLOCK ;
	rtn = fcntl ( p_private->i_socket_fd , F_SETFL , flags ) ;
	if ( rtn < 0 ) {
		return - 5 ;
	}

	// reuse bind addr
	int reuse = 1 ;
	if ( setsockopt ( p_private->i_socket_fd , SOL_SOCKET , SO_REUSEADDR , & reuse , sizeof ( reuse ) ) < 0 ) {
		return - 6 ;
	}
	//Bind() :
	if ( bind ( p_private->i_socket_fd , ( struct sockaddr* ) & saddr , sizeof ( saddr ) ) < 0 ) {
		SSL_CTX_free ( p_private->i_ctx ) ;
		return - 7 ;
	}
	//Listen() :
	if ( listen ( p_private->i_socket_fd , listen_num ) < 0 ) {
		SSL_CTX_free ( p_private->i_ctx ) ;
		return - 8 ;
	}
	return D_success ;
}
static int S_SSL_Server_Rtn_srv_fd ( S_Tcp_ssl_server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_ssl_server_private_data *p_private = ( S_ssl_server_private_data * ) obj->i_private ;
	return p_private->i_socket_fd ;
}
static int S_SSL_Server_Accept ( S_Tcp_ssl_server *obj ) {
	return D_success ;
}
static int S_SSL_Server_Rtn_acpt_fd ( S_Tcp_ssl_server *obj ) {
	return D_success ;
}
static int S_SSL_Server_Set_read_timeout ( S_Tcp_ssl_server *obj , int timeout ) {
	return D_success ;
}
// Send
static int S_SSL_Server_Send_bin ( S_Tcp_ssl_server *obj , CUStr tcp_msg ) {
	return D_success ;
}
static int S_SSL_Server_Send_str ( S_Tcp_ssl_server *obj , CStr tcp_msg ) {
	return D_success ;
}
// Read
static int S_SSL_Server_Read_bin ( S_Tcp_ssl_server *obj ) {
	return D_success ;
}
static UStr S_SSL_Server_Rtn_read_bin ( S_Tcp_ssl_server *obj ) {

	return NULL ;
}
static int S_SSL_Server_Read_str ( S_Tcp_ssl_server *obj ) {
	return D_success ;
}
static Str S_SSL_Server_Rtn_read_str ( S_Tcp_ssl_server *obj ) {

	return NULL ;
}
static int S_SSL_Server_Close_srv_fd ( S_Tcp_ssl_server *obj ) {
	return D_success ;
}
static int S_SSL_Server_Close_acpt_fd ( S_Tcp_ssl_server *obj ) {
	return D_success ;
}

S_Tcp_ssl_server *S_Tcp_ssl_server_tool_New ( void ) {
	S_Tcp_ssl_server *p_tmp = ( S_Tcp_ssl_server* ) calloc ( 1 , sizeof(S_Tcp_ssl_server) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}

	S_ssl_server_private_data *i_private_data = ( S_ssl_server_private_data* ) calloc ( 1 , sizeof(S_ssl_server_private_data) ) ;
	if ( NULL == i_private_data ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}
	i_private_data->i_timeout = 0 ;
	i_private_data->i_port = -1 ;
	i_private_data->i_socket_fd = -1 ;
	i_private_data->i_acpt_fd = -1 ;

	i_private_data->mlc_read_bin = NULL ;	// binary
	i_private_data->i_read_bin_bytes = 0 ;
	i_private_data->mlc_read_str = NULL ;	// string
	i_private_data->i_read_str_bytes = 0 ;

	p_tmp->i_private = ( void* ) i_private_data ;

	p_tmp->Server_begin = S_SSL_Server_begin ;
	p_tmp->Rtn_srv_fd = S_SSL_Server_Rtn_srv_fd ;
	p_tmp->Accept = S_SSL_Server_Accept ;
	p_tmp->Rtn_acpt_fd = S_SSL_Server_Rtn_acpt_fd ;
	p_tmp->Set_read_timeout = S_SSL_Server_Set_read_timeout ;

	// Send
	p_tmp->Send_bin = S_SSL_Server_Send_bin ;
	p_tmp->Send_str = S_SSL_Server_Send_str ;

	// Read
	p_tmp->Read_bin = S_SSL_Server_Read_bin ;
	p_tmp->Rtn_read_bin = S_SSL_Server_Rtn_read_bin ;
	p_tmp->Read_str = S_SSL_Server_Read_str ;
	p_tmp->Rtn_read_str = S_SSL_Server_Rtn_read_str ;

	// Close
	p_tmp->Close_srv_fd = S_SSL_Server_Close_srv_fd ;
	p_tmp->Close_acpt_fd = S_SSL_Server_Close_acpt_fd ;

	return p_tmp ;
}

void S_Tcp_ssl_server_tool_Delete ( S_Tcp_ssl_server **obj ) {
	S_ssl_server_private_data *p_private = ( * obj )->i_private ;
	free ( p_private->mlc_read_bin ) ;
	p_private->mlc_read_bin = NULL ;

	free ( p_private->mlc_read_str ) ;
	p_private->mlc_read_str = NULL ;

	free ( p_private->mlc_ssl_read_bin ) ;
	p_private->mlc_ssl_read_bin = NULL ;

	free ( p_private->mlc_ssl_read_str ) ;
	p_private->mlc_ssl_read_str = NULL ;

	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;

	free ( * obj ) ;
	* obj = NULL ;
	return ;
}
#endif

/*******************************************************************************************************************/
// Client //
typedef struct _S_client_private_data {
	int i_timeout ;
	int i_socket_fd ;
	Str mlc_ip_address ;
	Str mlc_port ;

	UStr mlc_read_bin ;			// Binary
	LInt i_read_bin_bytes ;
	Str mlc_read_str ;			// String
	LInt i_read_str_bytes ;

	SSL *i_ssl ;				// SSL
	SSL_CTX *i_ctx ;

	UStr mlc_ssl_read_bin ;		// Binary
	LInt i_ssl_read_bin_bytes ;
	Str mlc_ssl_read_str ;		// String
	LInt i_ssl_read_str_bytes ;

} S_client_private_data ;

/// 僅內部使用 ///
static int S_Client_Tcp_Setup ( S_Tcp_client *obj , CStr ip , CStr port , int timeout ) ;
static int S_Client_Tcp_Connect ( S_Tcp_client *obj ) ;
static int S_Client_Rtn_sock_fd ( S_Tcp_client *obj ) ;
static int S_Client_Rtn_timeout ( S_Tcp_client *obj ) ;
static int S_Client_Tcp_close ( S_Tcp_client *obj ) ;
static int S_Client_Tcp_Send_bin ( S_Tcp_client *obj , CStr tcp_msg ) ;
static int S_Client_Tcp_Read_bin ( S_Tcp_client *obj ) ;
static UStr S_Client_Rtn_read_bin ( S_Tcp_client *obj ) ;
static int S_Client_Rtn_read_bin_bytes ( S_Tcp_client *obj ) ;
static int S_Client_Tcp_Send_str ( S_Tcp_client *obj , CStr tcp_msg ) ;
static int S_Client_Tcp_Read_str ( S_Tcp_client *obj ) ;
static Str S_Client_Rtn_read_str ( S_Tcp_client *obj ) ;
static int S_Client_Rtn_read_str_bytes ( S_Tcp_client *obj ) ;
// SSL : SSLv23_client_method()
static int S_Client_Ssl_connect ( S_Tcp_client *obj ) ;
static int S_Client_Ssl_write_bin ( S_Tcp_client *obj , CUStr msg , int msg_bytes ) ;
static int S_Client_Ssl_write_str ( S_Tcp_client *obj , CStr msg ) ;
static int S_Client_Ssl_read_str ( S_Tcp_client *obj ) ;
static Str S_Client_Rtn_ssl_read_str ( S_Tcp_client *obj ) ;
static int S_Client_Ssl_read_bin ( S_Tcp_client *obj ) ;
static UStr S_Client_Rtn_ssl_read_bin ( S_Tcp_client *obj ) ;
static int S_Client_Close_ssl ( S_Tcp_client *obj ) ;
/// 僅內部使用 ///



static int S_Client_Tcp_Setup ( S_Tcp_client *obj , CStr ip , CStr port , int timeout ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! ip ) || ( ! port ) || ( ! timeout ) ) {
		return - 1 ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	p_private->i_socket_fd = - 1 ;

	int ip_sz = strlen ( ip ) ;
	p_private->mlc_ip_address = ( char * ) calloc ( ip_sz + 1 , sizeof(char) ) ;
	if ( NULL == p_private->mlc_ip_address ) {
		return - 2 ;
	}
	memcpy ( p_private->mlc_ip_address , ip , ip_sz ) ;

	int port_sz = strlen ( port ) ;
	p_private->mlc_port = ( char * ) calloc ( port_sz + 1 , 1 ) ;
	if ( NULL == p_private->mlc_port ) {
		free ( p_private->mlc_ip_address ) ;
		p_private->mlc_ip_address = NULL ;
		return - 3 ;
	}
	memcpy ( p_private->mlc_port , port , port_sz ) ;
	p_private->i_timeout = timeout ;

	return D_success ;
}

static int S_Client_Tcp_Connect ( S_Tcp_client *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	if ( ! p_private->mlc_ip_address || ! p_private->mlc_port ) {
		p_private->i_socket_fd = - 1 ;
		return - 2 ;
	}

	struct sockaddr_in saddr = { 0 } ;

	// Create tcp socket
	if ( ( p_private->i_socket_fd = socket ( AF_INET , SOCK_STREAM , 0 ) ) < 0 ) {
		p_private->i_socket_fd = - 1 ;
		return - 3 ;
	}

	memset ( & saddr , 0 , sizeof ( saddr ) ) ;
	saddr.sin_family = AF_INET ;
	saddr.sin_port = htons ( strtol ( p_private->mlc_port , NULL , 10 ) ) ;	// Port
	saddr.sin_addr.s_addr = inet_addr ( p_private->mlc_ip_address ) ; // 設定欲連上的Host IP

	// 設定連線timeout >> 5 seconds
	struct timeval timeo = { 5 , 0 } ;
	if ( setsockopt ( p_private->i_socket_fd , SOL_SOCKET , SO_RCVTIMEO , & timeo , sizeof ( timeo ) ) == - 1 ) {
		close ( p_private->i_socket_fd ) ;
		p_private->i_socket_fd = - 1 ;
		return - 4 ;
	}

	if ( setsockopt ( p_private->i_socket_fd , SOL_SOCKET , SO_SNDTIMEO , & timeo , sizeof ( timeo ) ) == - 1 ) {
		close ( p_private->i_socket_fd ) ;
		p_private->i_socket_fd = - 1 ;
		return - 5 ;
	}

	int con = connect ( p_private->i_socket_fd , ( struct sockaddr* ) & saddr , sizeof ( saddr ) ) ;
	if ( con < 0 ) {
		close ( p_private->i_socket_fd ) ;
		p_private->i_socket_fd = - 1 ;
		return - 6 ;
	}

	// 設定讀timeout 100ms
	timeo.tv_sec = 0 ;
	timeo.tv_usec = 100 ;
	if ( setsockopt ( p_private->i_socket_fd , SOL_SOCKET , SO_RCVTIMEO , & timeo , sizeof ( timeo ) ) == - 1 ) {
		close ( p_private->i_socket_fd ) ;
		p_private->i_socket_fd = - 1 ;
		return - 7 ;
	}

	return D_success ;	// success
}

static int S_Client_Rtn_sock_fd ( S_Tcp_client *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	return p_private->i_socket_fd ;
}

static int S_Client_Rtn_timeout ( S_Tcp_client *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	return p_private->i_timeout ;
}

static int S_Client_Tcp_close ( S_Tcp_client *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	close ( p_private->i_socket_fd ) ;
	return D_success ;
}

static int S_Client_Tcp_Send_bin ( S_Tcp_client *obj , CStr tcp_msg ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! tcp_msg ) ) {
		return - 1 ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	if ( ( ! obj ) || ( p_private->i_socket_fd == - 1 ) || ( ! p_private->i_timeout ) || ( ! tcp_msg ) || ( ( strlen ( tcp_msg ) & 0x01 ) != 0 ) ) {
		return - 2 ;
	}
	S_Epoll *epoll_tool = S_Epoll_New ( ) ;
	if ( NULL == epoll_tool ) {
		return - 3 ;
	}

	int rtn = epoll_tool->Detect_hup ( epoll_tool , p_private->i_socket_fd ) ;
	S_Epoll_Delete ( & epoll_tool ) ;
	if ( rtn != 0 ) {
		return - 4 ;		//  斷線
	}
	int tcp_msg_bytes = strlen ( tcp_msg ) >> 1 ; // /2 欲送報文的Bytes數
	int bytes = 0 ;
	UStr u_msg = String_to_hex ( tcp_msg , & bytes ) ;

#if 0
	printf("Will Send = [%s]\n" , tcp_msg ) ;
	int ct = 0 ;
	printf("##########Post##########\n") ;
	for ( ct = 0 ; ct < bytes ; ct ++ ) {
		printf ( "%02X " , u_msg [ ct ] ) ;
	}
	printf("\n##########End##########\n") ;
#endif
  	// send
 	rtn = send ( p_private->i_socket_fd , u_msg , tcp_msg_bytes , 0 ) ;

 	free ( u_msg ) ;
 	u_msg = NULL ;
	return rtn ;	// 返回 送出去的Bytes數
}

static int S_Client_Tcp_Read_bin ( S_Tcp_client *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	S_Epoll *epoll_tool = S_Epoll_New ( ) ;
	if ( NULL == epoll_tool ) {
		return - 2 ;
	}
	int rtn = epoll_tool->Detect_hup ( epoll_tool , p_private->i_socket_fd ) ;
	S_Epoll_Delete ( & epoll_tool ) ;
	if ( rtn != 0 ) {
		return - 3 ;		//  斷線
	}

	time_t now_t = time ( NULL ) ;
	UChar msg [ 4 + 0xffff + 2 ] = { 0x00 } ;
	int rd_sz = 0 ;
	while ( p_private->i_timeout > ( time ( NULL ) - now_t ) ) {
		int rtn = read ( p_private->i_socket_fd , msg + rd_sz , sizeof ( msg ) - rd_sz ) ;
		if ( rtn == 0 ) {	// 錯誤
			return - 4 ;
		} else if ( rtn == - 1 ) {
			if ( rd_sz > 0 ) {
				break ;
			}
			continue ;
		}
		rd_sz += rtn ;
	}
#if 0
	int ct = 0 ;
	printf("########## Read msg ##########\n") ;
	for ( ct = 0 ; ct < rd_sz ; ct ++ ) {
		printf ( "%02X " , msg [ ct ] ) ;
	}
	printf("\n##########End##########\n") ;
#endif

	p_private->i_read_bin_bytes = rd_sz ;
	p_private->mlc_read_bin = ( UChar * ) calloc ( p_private->i_read_bin_bytes + 1 , sizeof(UChar) ) ;
	if ( NULL == p_private->mlc_read_bin ) {
		return - 6 ;
	}
	memcpy ( p_private->mlc_read_bin , msg , p_private->i_read_bin_bytes ) ;
	return D_success ;
}

static UStr S_Client_Rtn_read_bin ( S_Tcp_client *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	return p_private->mlc_read_bin ;
}

static int S_Client_Rtn_read_bin_bytes ( S_Tcp_client *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	return ( int ) p_private->i_read_bin_bytes ;
}

static int S_Client_Tcp_Send_str ( S_Tcp_client *obj , CStr tcp_msg ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! tcp_msg ) ) {
		return - 1 ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	if ( ( ! obj ) || ( p_private->i_socket_fd == - 1 ) || ( ! p_private->i_timeout ) || ( ! tcp_msg ) ) {
		return - 2 ;
	}
	S_Epoll *epoll_tool = S_Epoll_New ( ) ;
	if ( NULL == epoll_tool ) {
		return - 3 ;
	}

	int rtn = epoll_tool->Detect_hup ( epoll_tool , p_private->i_socket_fd ) ;
	S_Epoll_Delete ( & epoll_tool ) ;
	if ( rtn != 0 ) {
		return - 4 ;		//  斷線
	}

	// send
	rtn = send ( p_private->i_socket_fd , tcp_msg , strlen ( tcp_msg ) , 0 ) ;

	return rtn ;	// 返回 送出去的Bytes數
}
static int S_Client_Tcp_Read_str ( S_Tcp_client *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	if ( p_private->mlc_read_str != NULL ) {
		free ( p_private->mlc_read_str ) ;
		p_private->mlc_read_str = NULL ;
	}

 	S_Epoll *epoll_tool = S_Epoll_New ( ) ;
	if ( NULL == epoll_tool ) {
		return - 2 ;
	}
 	int rtn = epoll_tool->Detect_hup ( epoll_tool , p_private->i_socket_fd ) ;
	S_Epoll_Delete ( & epoll_tool ) ;
	if ( rtn != 0 ) {
		return - 3 ;		//  斷線
	}

	time_t now_t = time ( NULL ) ;
	p_private->i_read_str_bytes = 0 ; // 總量
	while ( p_private->i_timeout > ( time ( NULL ) - now_t ) ) {
		errno = 0 ;
		char msg [ 8191 + 1 ] = { 0 } ;
		int rtn = read ( p_private->i_socket_fd , msg , sizeof ( msg ) - 1 ) ;
		if ( 0 >= rtn ) {
			if ( p_private->i_read_str_bytes > 0 ) {
				break ;
			}
			continue ;
		}

		p_private->mlc_read_str = ( char* ) realloc ( p_private->mlc_read_str , p_private->i_read_str_bytes + rtn + 1 ) ;
		if ( p_private->mlc_read_str == NULL ) {
			return - 5 ;
		}
		memcpy ( p_private->mlc_read_str + p_private->i_read_str_bytes , msg , rtn ) ; // 將資料接到上筆資料量後面
		( p_private->mlc_read_str ) [ p_private->i_read_str_bytes + rtn ] = 0 ;
		p_private->i_read_str_bytes += rtn ;
	}
#if 0
//	p_private->i_read_str_bytes = 0 ; // 總量
//	while ( 1 ) {
//		errno = 0 ;
//		char msg [ 8191 + 1 ] = { 0 } ;
//		int rtn = read ( p_private->i_socket_fd , msg , sizeof ( msg ) - 1 ) ;
//		if ( rtn == 0 ) {
//			break ;
//		} else if ( rtn == - 1 ) {
//			if ( p_private->i_read_str_bytes > 0 ) {
//				break ;
//			}
//			continue ;
//		}
//
//		p_private->mlc_read_str = ( char* ) realloc ( p_private->mlc_read_str , p_private->i_read_str_bytes + rtn + 1 ) ;
//		if ( p_private->mlc_read_str == NULL ) {
//			return - 5 ;
//		}
//		memcpy ( p_private->mlc_read_str + p_private->i_read_str_bytes , msg , rtn ) ;	// 將資料接到上筆資料量後面
//		( p_private->mlc_read_str ) [ p_private->i_read_str_bytes + rtn ] = 0 ;
//		p_private->i_read_str_bytes += rtn ;
//	}
#endif
	return D_success ;
}
static Str S_Client_Rtn_read_str ( S_Tcp_client *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	return p_private->mlc_read_str ;
}
static int S_Client_Rtn_read_str_bytes ( S_Tcp_client *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	return p_private->i_read_str_bytes ;
}

// SSL : SSLv23_client_method()
static int S_Client_Ssl_connect ( S_Tcp_client *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	SSL_library_init ( ) ;   		//對OpenSSL進行初始化
	SSL_load_error_strings ( ) ;   	//對錯誤訊息進行初始化

	// 申請SSL會話的環境CTX(建立本次會話連接所使用的協議())
	p_private->i_ctx = SSL_CTX_new ( SSLv23_client_method ( ) ) ;
	if ( NULL == p_private->i_ctx ) {
		obj->Close ( obj ) ;
		return - 2 ;
	}

	// 申請一個SSL 套節字 //
	p_private->i_ssl = SSL_new ( p_private->i_ctx ) ;
	if ( NULL == p_private->i_ssl ) {
		obj->Close_ssl ( obj ) ;
		return - 3 ;
	}

	// 綁定讀寫Socket //
	if ( 0 == SSL_set_fd ( p_private->i_ssl , p_private->i_socket_fd ) ) {
		obj->Close_ssl ( obj ) ;
		return - 4 ;
	}

	// SSL 握手動作
	if ( 0 >= SSL_connect ( p_private->i_ssl ) ) {
		obj->Close_ssl ( obj ) ;
		return - 5 ;
	}
	return D_success ;
}
static int S_Client_Ssl_write_bin ( S_Tcp_client *obj , CUStr msg , int msg_bytes ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! msg ) || ( ! msg_bytes ) ) {
		return - 1 ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	while ( 1 ) {
		int rtnCode = SSL_write ( p_private->i_ssl , msg , msg_bytes ) ;
		int errCode = SSL_get_error ( p_private->i_ssl , rtnCode ) ;
		if ( SSL_ERROR_NONE == errCode ) {
			break ;
		} else if ( SSL_ERROR_WANT_WRITE == errCode ) {
			continue ;
		} else {
			return - 2 ;
		}
	}
	return D_success ;
}
static int S_Client_Ssl_write_str ( S_Tcp_client *obj , CStr msg ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! msg ) ) {
		return - 1 ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	while ( 1 ) {
		int rtnCode = SSL_write ( p_private->i_ssl , msg , strlen ( msg ) ) ;
		int errCode = SSL_get_error ( p_private->i_ssl , rtnCode ) ;
		if ( SSL_ERROR_NONE == errCode ) {
			break ;
		} else if ( SSL_ERROR_WANT_WRITE == errCode ) {
			continue ;
		} else {
			return - 2 ;
		}
	}
	return D_success ;
}
static int S_Client_Ssl_read_str ( S_Tcp_client *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	char buf [ 8191 + 1 ] = { 0 } ;
	while ( 1 ) {
		int rtn = SSL_read ( p_private->i_ssl , buf , sizeof ( buf ) - 1 ) ;
		int errCode = SSL_get_error ( p_private->i_ssl , rtn ) ;
		if ( SSL_ERROR_NONE == errCode ) {
			buf [ rtn ] = 0 ;
			p_private->mlc_ssl_read_str = ( char* ) realloc ( p_private->mlc_ssl_read_str , p_private->i_ssl_read_str_bytes + rtn + 1 ) ;
			if ( p_private->mlc_ssl_read_str == NULL ) {
				return - 2 ;
			}
			memcpy ( p_private->mlc_ssl_read_str + p_private->i_ssl_read_str_bytes , buf , rtn ) ;
			p_private->i_ssl_read_str_bytes += rtn ;
			continue ;
		} else if ( SSL_ERROR_WANT_READ == errCode ) {
			continue ;
		} else {
			if ( 0 < p_private->i_ssl_read_str_bytes ) {
				break ;
			}
			return - 3 ;
		}
	}
	return D_success ;
}
static Str S_Client_Rtn_ssl_read_str ( S_Tcp_client *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	return p_private->mlc_ssl_read_str ;
}
static int S_Client_Ssl_read_bin ( S_Tcp_client *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return -1 ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	UChar buf [ 8191 + 1 ] = { 0 } ;
	while ( 1 ) {
		int rtn = SSL_read ( p_private->i_ssl , buf , sizeof ( buf ) - 1 ) ;
		int errCode = SSL_get_error ( p_private->i_ssl , rtn ) ;
		if ( SSL_ERROR_NONE == errCode ) {
			p_private->mlc_ssl_read_bin = ( unsigned char* ) realloc ( p_private->mlc_ssl_read_bin , p_private->i_ssl_read_str_bytes + rtn + 1 ) ;
			if ( p_private->mlc_ssl_read_bin == NULL ) {
				return - 2 ;
			}
			memcpy ( p_private->mlc_ssl_read_bin + p_private->i_ssl_read_str_bytes , buf , rtn ) ;
			p_private->i_ssl_read_str_bytes += rtn ;
			continue ;
		} else if ( SSL_ERROR_WANT_READ == errCode ) {
			continue ;
		} else {
			if ( 0 < p_private->i_ssl_read_str_bytes ) {
				break ;
			}
			return - 3 ;
		}
	}
	return D_success ;
}
static UStr S_Client_Rtn_ssl_read_bin ( S_Tcp_client *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	return p_private->mlc_ssl_read_bin ;
}
static int S_Client_Close_ssl ( S_Tcp_client *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return -1 ;
	}
	S_client_private_data *p_private = ( S_client_private_data * ) obj->i_private ;
	if ( NULL != p_private->i_ctx ) {
		SSL_CTX_free ( p_private->i_ctx ) ;
		p_private->i_ctx = NULL ;
	}
	if ( NULL != p_private->i_ssl ) {
		SSL_shutdown ( p_private->i_ssl ) ;
		SSL_free ( p_private->i_ssl ) ;
		p_private->i_ssl = NULL ;
	}
	obj->Close ( obj ) ;
	return D_success ;
}

S_Tcp_client* S_Tcp_client_tool_New ( void ) {
	S_Tcp_client *p_tmp = ( S_Tcp_client* ) calloc ( 1 , sizeof(S_Tcp_client) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}

	S_client_private_data *i_private_data = ( S_client_private_data* ) calloc ( 1 , sizeof(S_client_private_data) ) ;
	if ( NULL == i_private_data ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}
	i_private_data->i_timeout = 0 ;
	i_private_data->i_socket_fd = 0 ;
	i_private_data->i_ctx = NULL ;
	i_private_data->i_ssl = NULL ;
	i_private_data->mlc_ip_address = NULL ;
	i_private_data->mlc_port = NULL ;
	i_private_data->mlc_read_bin = NULL ;	// binary
	i_private_data->i_read_bin_bytes = 0 ;
	i_private_data->mlc_read_str = NULL ;	// string
	i_private_data->i_read_str_bytes = 0 ;

	p_tmp->i_private = ( void* ) i_private_data ;

	p_tmp->Setup = S_Client_Tcp_Setup ;
	p_tmp->Connect = S_Client_Tcp_Connect ;
	p_tmp->Rtn_sock_fd = S_Client_Rtn_sock_fd ;
	p_tmp->Rtn_timeout = S_Client_Rtn_timeout ;
	p_tmp->Close = S_Client_Tcp_close ;

	// Send
	p_tmp->Send_bin = S_Client_Tcp_Send_bin ;
	p_tmp->Send_str = S_Client_Tcp_Send_str ;

	// Read
	p_tmp->Read_bin = S_Client_Tcp_Read_bin ;
	p_tmp->Rtn_read_bin = S_Client_Rtn_read_bin ;
	p_tmp->Rtn_read_bin_bytes = S_Client_Rtn_read_bin_bytes ;
	p_tmp->Read_str = S_Client_Tcp_Read_str ;
	p_tmp->Rtn_read_str = S_Client_Rtn_read_str ;
	p_tmp->Rtn_read_str_bytes = S_Client_Rtn_read_str_bytes ;

	// SSL
	p_tmp->Ssl_connect = S_Client_Ssl_connect ;
	p_tmp->Ssl_send_bin = S_Client_Ssl_write_bin ;
	p_tmp->Ssl_send_str = S_Client_Ssl_write_str ;
	p_tmp->Ssl_read_str = S_Client_Ssl_read_str ;
	p_tmp->Rtn_ssl_read_str = S_Client_Rtn_ssl_read_str ;
	p_tmp->Ssl_read_bin = S_Client_Ssl_read_bin ;
	p_tmp->Rtn_ssl_read_bin = S_Client_Rtn_ssl_read_bin ;
	p_tmp->Close_ssl = S_Client_Close_ssl ;

	return p_tmp ;
}

void S_Tcp_client_tool_Delete ( S_Tcp_client **obj ) {
	S_client_private_data *p_private = ( * obj )->i_private ;
	free ( p_private->mlc_ip_address ) ;
	p_private->mlc_ip_address = NULL ;

	free ( p_private->mlc_port ) ;
	p_private->mlc_port = NULL ;

	free ( p_private->mlc_read_bin ) ;
	p_private->mlc_read_bin = NULL ;

	free ( p_private->mlc_read_str ) ;
	p_private->mlc_read_str = NULL ;

	free ( p_private->mlc_ssl_read_bin ) ;
	p_private->mlc_ssl_read_bin = NULL ;

	free ( p_private->mlc_ssl_read_str ) ;
	p_private->mlc_ssl_read_str = NULL ;


	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;

	free ( * obj ) ;
	* obj = NULL ;
	return ;
}
