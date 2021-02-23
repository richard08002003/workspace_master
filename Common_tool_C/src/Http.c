/*
 * Http.c
 *
 *  Created on: 2017年9月6日
 *      Author: richard
 */
#include "Http.h"

#include "Setup.h"	// log 用


typedef struct _S_private_data {

	Str mlc_host_name ;
	Str mlc_ip ;

	SSL *i_ssl ;
	SSL_CTX *i_ctx ;

	int i_socket_fd ;

	Str mlc_read_str ;
	LInt i_read_str_sz ;

	UStr mlc_read_bin ;
	LInt i_read_bin_sz ;

#if 0
	// URL //
	Str mlc_url_decode ;
	LInt i_url_decode_sz ;

	Str mlc_url_encode ;
	LInt i_url_encode_sz ;
#endif

	// SSL //
	Str mlc_ssl_read_str ;
	LInt i_ssl_read_str_sz ;

	UStr mlc_ssl_read_bin ;
	LInt i_ssl_read_bin_sz ;

} S_private_data ;

// ********** 僅內部使用 ********** ///
static Str S_Hostname_to_ip ( CStr hostname ) ;
static int S_Connect ( S_Https *obj , CStr ip , CStr port ) ;
static int S_Rtn_sock_fd ( S_Https *obj ) ;
static Str S_Rtn_host_name ( S_Https *obj ) ;
static Str S_Rtn_ip ( S_Https *obj ) ;
static int S_Close_sock_fd ( S_Https *obj ) ;
static int S_Write_bin ( S_Https *obj , CUStr msg , int msg_bytes ) ;
static int S_Write_str ( S_Https *obj , CStr msg ) ;
static int S_Read_to_file ( S_Https *obj , CStr file_path , bool keepalive ) ;
static int S_Read_str ( S_Https *obj , bool keepalive ) ;
static Str S_Rtn_read_str ( S_Https *obj ) ;
static int S_Read_bin ( S_Https *obj , bool keepalive ) ;
static UStr S_Rtn_read_bin ( S_Https *obj ) ;
// URL
//static char S_From_hex ( char ch ) ;
//static char S_To_hex ( char code ) ;
//static int S_Url_decode ( S_Https *obj , CStr input ) ;
//static Str S_Rtn_url_decode ( S_Https *obj ) ;
//static int S_Url_encode ( S_Https *obj , CStr input ) ;
//static Str S_Rtn_url_encode ( S_Https *obj ) ;
// SSL : SSLv23_client_method()
static int S_Ssl_connect ( S_Https *obj ) ;
static int S_Ssl_write_bin ( S_Https *obj , CUStr msg , int msg_bytes ) ;
static int S_Ssl_write_str ( S_Https *obj , CStr msg ) ;
static int S_Ssl_read_str ( S_Https *obj ) ;
static Str S_Rtn_ssl_read_str ( S_Https *obj ) ;
static int S_Ssl_read_bin ( S_Https *obj ) ;
static UStr S_Rtn_ssl_read_bin ( S_Https *obj ) ;
static int S_Close_ssl ( S_Https *obj ) ;

static Str S_Hostname_to_ip ( CStr hostname ) {
	if ( hostname == NULL ) {
		return NULL ;
	}
	struct hostent *he;
	struct in_addr **addr_list;
	if ((he = gethostbyname(hostname)) == NULL) {
		herror("gethostbyname");			// get the host info
		return NULL ;
	}
	addr_list = (struct in_addr **) he->h_addr_list;
	int i = 0;
	char ip[100] = {0};
	for (i = 0; addr_list[i] != NULL; i++) {
		strcpy(ip, inet_ntoa(*addr_list[i]));
	}
	char *output = (char*) calloc(strlen(ip) + 1, sizeof(char));
	memcpy(output, ip, strlen(ip));
	return output;
}
// ********** End of 僅內部使用 ********** ///

static int S_Connect ( S_Https *obj , CStr ip , CStr port ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! ip ) || ( ! port ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;

	Str IP = S_Hostname_to_ip ( ip ) ;
	if ( IP == NULL ) {
		return - 2 ;
	}

	p_private->mlc_host_name = ( char* ) calloc ( strlen ( IP ) + 1 , sizeof(char) ) ;
	if ( p_private->mlc_host_name == NULL ) {
		return - 3 ;
	}
	memcpy ( p_private->mlc_host_name , IP , strlen ( IP ) ) ;
	p_private->mlc_ip = ( char* ) calloc ( strlen ( ip ) + 1 , sizeof(char) ) ;
	if ( p_private->mlc_ip == NULL ) {
		return - 4 ;
	}
	memcpy ( p_private->mlc_ip , ip , strlen ( ip ) ) ;

	struct addrinfo req = { 0 } ;	// 設定用
	struct addrinfo *pai = NULL ;	//取得傳回資訊用
	req.ai_family = AF_UNSPEC ;
	req.ai_socktype = SOCK_STREAM ;
	int rtn = getaddrinfo ( IP , port , & req , & pai ) ;
	free ( IP ) ;
	IP = NULL ;
	if ( 0 != rtn ) {
		return - 5 ;
	}
 	p_private->i_socket_fd = socket ( pai->ai_family , pai->ai_socktype , pai->ai_protocol ) ;
	if ( 0 > p_private->i_socket_fd ) {
		p_private->i_socket_fd = - 1 ;
		freeaddrinfo ( pai ) ;
		return - 6 ;
	}

	// 設定連線timeout >> 10 seconds
	struct timeval timeo = { 10 , 0 } ;
	if ( setsockopt ( p_private->i_socket_fd , SOL_SOCKET , SO_RCVTIMEO , & timeo , sizeof ( timeo ) ) == - 1 ) {
		return - 7 ;
	}

	if ( setsockopt ( p_private->i_socket_fd , SOL_SOCKET , SO_SNDTIMEO , & timeo , sizeof ( timeo ) ) == - 1 ) {
		return - 8 ;
	}

	if ( 0 > connect ( p_private->i_socket_fd , pai->ai_addr , pai->ai_addrlen ) ) {
		obj->Close_sock_fd ( obj ) ;
		freeaddrinfo ( pai ) ;
		return - 9 ;
	}

	// 啟用read timeout
	struct timeval after_timeo = { 0 , 100 };
	if ( setsockopt ( p_private->i_socket_fd , SOL_SOCKET , SO_SNDTIMEO , & after_timeo , sizeof ( after_timeo ) ) == - 1 ) {
		return - 10 ;
	}

	freeaddrinfo ( pai ) ;
	return D_success ;
}
static int S_Rtn_sock_fd ( S_Https *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	return p_private->i_socket_fd ;
}
static Str S_Rtn_host_name ( S_Https *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	return p_private->mlc_host_name ;
}
static Str S_Rtn_ip ( S_Https *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	return p_private->mlc_ip ;
}
static int S_Close_sock_fd ( S_Https *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	if ( p_private->i_socket_fd > 0 ) {
		close ( p_private->i_socket_fd ) ;
		p_private->i_socket_fd = - 1 ;
	}
	return D_success ;
}
static int S_Write_bin ( S_Https *obj , CUStr msg , int msg_bytes ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! msg ) || ( ! msg_bytes ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;

	S_Epoll *epoll_tool = S_Epoll_New ( ) ;
	if ( NULL == epoll_tool ) {
		return - 2 ;
	}
 	int rtn = epoll_tool->Detect_hup ( epoll_tool , p_private->i_socket_fd ) ;
	S_Epoll_Delete ( & epoll_tool ) ;
	if ( rtn != 0 ) {
		return - 3 ;		//  斷線
	}

	rtn = write ( p_private->i_socket_fd , msg , msg_bytes ) ;
	if ( 0 >= rtn ) {
		return - 4 ;
	}
	return D_success ;
}
static int S_Write_str ( S_Https *obj , CStr msg ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! msg ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
 	S_Epoll *epoll_tool = S_Epoll_New ( ) ;
	if ( NULL == epoll_tool ) {
		return - 2 ;
	}
 	int rtn = epoll_tool->Detect_hup ( epoll_tool , p_private->i_socket_fd ) ;
	S_Epoll_Delete ( & epoll_tool ) ;
	if ( rtn != 0 ) {
		return - 3 ;		//  斷線
	}
	rtn = write ( p_private->i_socket_fd , msg , strlen ( msg ) ) ;
	if ( - 1 == rtn ) {
		return - 4 ;
	}
	return D_success ;
}
static int S_Read_to_file ( S_Https *obj , CStr file_path , bool keepalive ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! file_path ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	S_Epoll *epoll_tool = S_Epoll_New ( ) ;
	if ( NULL == epoll_tool ) {
		return - 2 ;
	}
	int rtn = epoll_tool->Detect_hup ( epoll_tool , p_private->i_socket_fd ) ;
	S_Epoll_Delete ( & epoll_tool ) ;
	if ( rtn != 0 ) {
		return - 3 ;		//  斷線
	}
	FILE *fp = fopen ( file_path , "wb+" ) ;
	if ( NULL == fp ) {
		return - 4 ;
	}
	while ( 1 ) {
		errno = 0 ;
		UChar buf [ 8191 + 1 ] = { 0 } ;
		rtn = read ( p_private->i_socket_fd , buf , sizeof ( buf ) - 1 ) ;
		if ( - 1 == rtn ) {
			if ( EAGAIN == errno ) {
				continue ;
			}
			return - 5 ;
		} else if ( 0 == rtn ) {
			fflush ( fp ) ;
			rtn = ftell ( fp ) ;
			fclose ( fp ) ;
			return rtn ;
		}
		fwrite ( buf , sizeof(UChar) , rtn , fp ) ;
		if ( true == keepalive ) {
			if ( rtn < ( ( int ) sizeof ( buf ) - 1 ) ) { // 基本上應該是沒有資料了 , 在這種沒頭尾的資料這樣就當作沒資料了
				break ;
			}
		}

	}
	fflush ( fp ) ;
	rtn = ftell ( fp ) ;
	fclose ( fp ) ;
	return rtn ;
}
static int S_Read_str ( S_Https *obj , bool keepalive ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
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

#if 0		// Test 1
	char msg [ 8191 + 1  ] = { 0 } ;
	int rd_sz = 0 ;
	while ( 1 ) {
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
		if ( true == keepalive ) {
			if ( rtn < ( ( int ) sizeof ( msg ) - 1 ) ) { // 基本上應該是沒有資料了 , 在這種沒頭尾的資料這樣就當作沒資料了
				break ;
			}
		}
	}
	p_private->i_read_str_sz = rd_sz ;
	p_private->mlc_read_str = ( char * ) calloc ( p_private->i_read_str_sz + 1 , sizeof(char) ) ;
	if ( NULL == p_private->mlc_read_str ) {
		return - 6 ;
	}
	memcpy ( p_private->mlc_read_str , msg , p_private->i_read_str_sz ) ;
#endif

#if 1 // Test 2
	p_private->i_read_str_sz = 0 ; // 總量
	while ( 1 ) {
		errno = 0 ;
		char msg [ 8191 + 1 ] = { 0 } ;
		int rtn = read ( p_private->i_socket_fd , msg , sizeof ( msg ) - 1 ) ;
		if ( rtn == 0 ) {
			break ;
		} else if ( rtn == - 1 ) {
			if ( p_private->i_read_str_sz > 0 ) {
				break ;
			}
			continue ;
		}

		p_private->mlc_read_str = ( char* ) realloc ( p_private->mlc_read_str , p_private->i_read_str_sz + rtn + 1 ) ;
		if ( p_private->mlc_read_str == NULL ) {
			return - 4 ;
		}
		memcpy ( p_private->mlc_read_str + p_private->i_read_str_sz , msg , rtn ) ;	// 將資料接到上筆資料量後面
		( p_private->mlc_read_str ) [ p_private->i_read_str_sz + rtn ] = 0 ;
		p_private->i_read_str_sz += rtn ;
		if ( true == keepalive ) {
			if ( rtn < ( ( int ) sizeof ( msg ) - 1 ) ) { // 基本上應該是沒有資料了 , 在這種沒頭尾的資料這樣就當作沒資料了
				break ;
			}
		}
	}
#endif

	return D_success ;
}
static Str S_Rtn_read_str ( S_Https *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	return p_private->mlc_read_str ;
}
static int S_Read_bin ( S_Https *obj , bool keepalive ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	if ( p_private->mlc_read_bin != NULL ) {
		free ( p_private->mlc_read_bin ) ;
		p_private->mlc_read_bin = NULL ;
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
	while ( 1 ) {
		errno = 0 ;
		UChar buf [ 8191 + 1 ] = { 0 } ;
		int rtn = read ( p_private->i_socket_fd , buf , sizeof ( buf ) - 1 ) ;

		if ( - 1 == rtn ) {
			if ( EAGAIN == errno ) {
				continue ;
			}
			return - 4 ;
		} else if ( 0 == rtn ) {
			break ;
		}
		p_private->mlc_read_bin = ( unsigned char* ) realloc ( p_private->mlc_read_bin , p_private->i_read_bin_sz + rtn + 1 ) ;
		if ( p_private->mlc_read_bin == NULL ) {
			return - 5 ;
		}
		memcpy ( p_private->mlc_read_bin + p_private->i_read_bin_sz , buf , rtn ) ;
		p_private->i_read_bin_sz += rtn ;

		if ( true == keepalive ) {
			if ( rtn < ( ( int ) sizeof ( buf ) - 1 ) ) { // 基本上應該是沒有資料了 , 在這種沒頭尾的資料這樣就當作沒資料了
				break ;
			}
		}
	}
	return D_success ;
}
static UStr S_Rtn_read_bin ( S_Https *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	return p_private->mlc_read_bin ;
}

#if 0
// URL
static char S_From_hex ( char ch ) {
	return isdigit ( ch ) ? ( ch - '0' ) : ( tolower ( ch ) - 'a' + 10 ) ;
}
static char S_To_hex ( char code ) {
	static char hex [ ] = "0123456789abcdef" ;
	return hex [ code & 15 ] ;
}
static int S_Url_decode ( S_Https *obj , CStr input ) {
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	int inputI = 0 ;
	while ( inputI < ( int ) strlen ( input ) ) {
		if ( input [ inputI ] == '%' ) {
			if ( input [ inputI + 1 ] && input [ inputI + 2 ] ) {
//				iUrlDecode += S_From_hex ( input [ inputI + 1 ] ) << 4 | FromHex ( input [ inputI + 2 ] ) ;
				char tmp = S_From_hex ( input [ inputI + 1 ] ) << 4 | S_From_hex ( input [ inputI + 2 ] ) ;
				p_private->mlc_url_decode = ( char * ) realloc ( p_private->mlc_url_decode , p_private->i_url_decode_sz + inputI + 1 ) ;
				if ( p_private->mlc_url_decode == NULL ) {
					return - 1 ;
				}
				memcpy ( p_private->mlc_url_decode + p_private->i_url_decode_sz , tmp , 2 ) ;
				inputI += 2 ;
				p_private->i_url_decode_sz += inputI ;
			}
		} else if ( input [ inputI ] == '+' ) {
//			iUrlDecode += ' ' ;
		} else {
//			iUrlDecode += input [ inputI ] ;
		}
		++ inputI ;
	}
	return D_success ;
}
static Str S_Rtn_url_decode ( S_Https *obj ) {
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	return p_private->mlc_url_decode ;
}
static int S_Url_encode ( S_Https *obj , CStr input ) {
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;

}
static Str S_Rtn_url_encode ( S_Https *obj ) {
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	return p_private->mlc_url_encode ;
}
#endif

// SSL : SSLv23_client_method()
static int S_Ssl_connect ( S_Https *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	SSL_library_init ( ) ;   		//對OpenSSL進行初始化
	SSL_load_error_strings ( ) ;   	//對錯誤訊息進行初始化

	// 申請SSL會話的環境CTX(建立本次會話連接所使用的協議())
	p_private->i_ctx = SSL_CTX_new ( SSLv23_client_method ( ) ) ;
	if ( NULL == p_private->i_ctx ) {
		obj->Close_sock_fd ( obj ) ;
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
static int S_Ssl_write_bin ( S_Https *obj , CUStr msg , int msg_bytes ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! msg ) || ( ! msg_bytes ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	S_Epoll *epoll_tool = S_Epoll_New ( ) ;
	if ( NULL == epoll_tool ) {
		return - 2 ;
	}
	int rtn = epoll_tool->Detect_hup ( epoll_tool , p_private->i_socket_fd ) ;
	S_Epoll_Delete ( & epoll_tool ) ;
	if ( rtn != 0 ) {
		return - 3 ;		//  斷線
	}
	while ( 1 ) {
		int rtnCode = SSL_write ( p_private->i_ssl , msg , msg_bytes ) ;
		int errCode = SSL_get_error ( p_private->i_ssl , rtnCode ) ;
		if ( SSL_ERROR_NONE == errCode ) {
			break ;
		} else if ( SSL_ERROR_WANT_WRITE == errCode ) {
			continue ;
		} else {
			return - 4 ;
		}
	}
	return D_success ;
}
static int S_Ssl_write_str ( S_Https *obj , CStr msg ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! msg ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
 	S_Epoll *epoll_tool = S_Epoll_New ( ) ;
	if ( NULL == epoll_tool ) {
		return - 2 ;
	}
 	int rtn = epoll_tool->Detect_hup ( epoll_tool , p_private->i_socket_fd ) ;
	S_Epoll_Delete ( & epoll_tool ) ;
	if ( rtn != 0 ) {
		return - 3 ;		//  斷線
	}
	while ( 1 ) {
		int rtnCode = SSL_write ( p_private->i_ssl , msg , strlen ( msg ) ) ;
		int errCode = SSL_get_error ( p_private->i_ssl , rtnCode ) ;
		if ( SSL_ERROR_NONE == errCode ) {
			break ;
		} else if ( SSL_ERROR_WANT_WRITE == errCode ) {
			continue ;
		} else {
			return - 4 ;
		}
	}
	return D_success ;
}
static int S_Ssl_read_str ( S_Https *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
 	S_Epoll *epoll_tool = S_Epoll_New ( ) ;
	if ( NULL == epoll_tool ) {
		return - 2 ;
	}
 	int rtn = epoll_tool->Detect_hup ( epoll_tool , p_private->i_socket_fd ) ;
	S_Epoll_Delete ( & epoll_tool ) ;
	if ( rtn != 0 ) {
		return - 3 ;		//  斷線
	}
	char buf [ 8191 + 1 ] = { 0 } ;
	while ( 1 ) {
		int rtn = SSL_read ( p_private->i_ssl , buf , sizeof ( buf ) - 1 ) ;
		int errCode = SSL_get_error ( p_private->i_ssl , rtn ) ;
		if ( SSL_ERROR_NONE == errCode ) {
			buf [ rtn ] = 0 ;
			p_private->mlc_ssl_read_str = ( char* ) realloc ( p_private->mlc_ssl_read_str , p_private->i_ssl_read_str_sz + rtn + 1 ) ;
			if ( p_private->mlc_ssl_read_str == NULL ) {
				return - 4 ;
			}
			memcpy ( p_private->mlc_ssl_read_str + p_private->i_ssl_read_str_sz , buf , rtn ) ;
			p_private->i_ssl_read_str_sz += rtn ;
			continue ;
		} else if ( SSL_ERROR_WANT_READ == errCode ) {
			continue ;
		} else {
			if ( 0 < p_private->i_ssl_read_str_sz ) {
				break ;
			}
			return - 5 ;
		}
	}
	return D_success ;
}
static Str S_Rtn_ssl_read_str ( S_Https *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	return p_private->mlc_ssl_read_str ;
}
static int S_Ssl_read_bin ( S_Https *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return -1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
 	S_Epoll *epoll_tool = S_Epoll_New ( ) ;
	if ( NULL == epoll_tool ) {
		return - 2 ;
	}
 	int rtn = epoll_tool->Detect_hup ( epoll_tool , p_private->i_socket_fd ) ;
	S_Epoll_Delete ( & epoll_tool ) ;
	if ( rtn != 0 ) {
		return - 3 ;		//  斷線
	}
	UChar buf [ 8191 + 1 ] = { 0 } ;
	while ( 1 ) {
		int rtn = SSL_read ( p_private->i_ssl , buf , sizeof ( buf ) - 1 ) ;
		int errCode = SSL_get_error ( p_private->i_ssl , rtn ) ;
		if ( SSL_ERROR_NONE == errCode ) {
			p_private->mlc_ssl_read_bin = ( unsigned char* ) realloc ( p_private->mlc_ssl_read_bin , p_private->i_ssl_read_bin_sz + rtn + 1 ) ;
			if ( p_private->mlc_ssl_read_bin == NULL ) {
				return - 4 ;
			}
			memcpy ( p_private->mlc_ssl_read_bin + p_private->i_ssl_read_bin_sz , buf , rtn ) ;
			p_private->i_ssl_read_bin_sz += rtn ;
			continue ;
		} else if ( SSL_ERROR_WANT_READ == errCode ) {
			continue ;
		} else {
			if ( 0 < p_private->i_ssl_read_bin_sz ) {
				break ;
			}
			return - 5 ;
		}
	}
	return D_success ;
}
static UStr S_Rtn_ssl_read_bin ( S_Https *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	return p_private->mlc_ssl_read_bin ;
}
static int S_Close_ssl ( S_Https *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	if ( NULL != p_private->i_ctx ) {
		SSL_CTX_free ( p_private->i_ctx ) ;
		p_private->i_ctx = NULL ;
	}
	if ( NULL != p_private->i_ssl ) {
		SSL_shutdown ( p_private->i_ssl ) ;
		SSL_free ( p_private->i_ssl ) ;
		p_private->i_ssl = NULL ;
	}
	obj->Close_sock_fd ( obj ) ;
	return D_success ;
}

S_Https *S_Https_tool_New ( void ) {
	S_Https *p_tmp = ( S_Https* ) calloc ( 1 , sizeof(S_Https) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}

	S_private_data *i_private_data = ( S_private_data* ) calloc ( 1 , sizeof(S_private_data) ) ;
	if ( NULL == i_private_data ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}
	i_private_data->mlc_host_name = NULL ;
	i_private_data->mlc_ip = NULL ;
	i_private_data->i_ssl = NULL ;
	i_private_data->i_ctx = NULL ;
	i_private_data->i_socket_fd = - 1 ;
	i_private_data->mlc_read_str = NULL ;
	i_private_data->i_read_str_sz = 0 ;
	i_private_data->mlc_read_bin = NULL ;
	i_private_data->i_read_bin_sz = 0 ;
#if 0
	i_private_data->mlc_url_decode = NULL ;
	i_private_data->i_url_decode_sz = - 1 ;
	i_private_data->mlc_url_encode = NULL ;
	i_private_data->i_url_encode_sz = - 1 ;
#endif
	i_private_data->mlc_ssl_read_str = NULL ;
	i_private_data->i_ssl_read_str_sz = - 1 ;
	i_private_data->mlc_ssl_read_bin = NULL ;
	i_private_data->i_ssl_read_bin_sz = 0 ;
	p_tmp->i_private = ( void* ) i_private_data ;

	p_tmp->Connect = S_Connect ;
	p_tmp->Rtn_sock_fd = S_Rtn_sock_fd ;
	p_tmp->Rtn_host_name = S_Rtn_host_name ;
	p_tmp->Rtn_ip = S_Rtn_ip ;
	p_tmp->Close_sock_fd = S_Close_sock_fd ;
	p_tmp->Write_bin = S_Write_bin ;
	p_tmp->Write_str = S_Write_str ;
	p_tmp->Read_to_file = S_Read_to_file;
	p_tmp->Read_str = S_Read_str ;
	p_tmp->Rtn_read_str = S_Rtn_read_str ;
	p_tmp->Read_bin = S_Read_bin ;
	p_tmp->Rtn_read_bin = S_Rtn_read_bin ;
//	p_tmp->Url_decode = S_Url_decode ;
//	p_tmp->Rtn_url_decode = S_Rtn_url_decode ;
//	p_tmp->Url_encode = S_Url_encode ;
//	p_tmp->Rtn_url_encode = S_Rtn_url_encode ;
	p_tmp->Ssl_connect = S_Ssl_connect ;
	p_tmp->Ssl_write_bin = S_Ssl_write_bin ;
	p_tmp->Ssl_write_str = S_Ssl_write_str ;
	p_tmp->Ssl_read_str = S_Ssl_read_str ;
	p_tmp->Rtn_ssl_read_str = S_Rtn_ssl_read_str ;
	p_tmp->Ssl_read_bin = S_Ssl_read_bin ;
	p_tmp->Rtn_ssl_read_bin = S_Rtn_ssl_read_bin ;
	p_tmp->Close_ssl = S_Close_ssl ;
	return p_tmp ;
}
void S_Https_tool_Delete ( S_Https **obj ) {
	S_private_data *p_private = ( * obj )->i_private ;

	free ( p_private->mlc_host_name ) ;
	p_private->mlc_host_name = NULL ;

	free ( p_private->mlc_ip ) ;
	p_private->mlc_ip = NULL ;

	free ( p_private->mlc_read_str ) ;
	p_private->mlc_read_str = NULL ;

	free ( p_private->mlc_read_bin ) ;
	p_private->mlc_read_bin = NULL ;

#if 0
	free ( p_private->mlc_url_decode ) ;
	p_private->mlc_url_decode = NULL ;

	free ( p_private->mlc_url_encode ) ;
	p_private->mlc_url_encode = NULL ;
#endif

	free ( p_private->mlc_ssl_read_str ) ;
	p_private->mlc_ssl_read_str = NULL ;

	free ( p_private->mlc_ssl_read_bin ) ;
	p_private->mlc_ssl_read_bin = NULL ;

	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;

	free ( * obj ) ;
	* obj = NULL ;
	return ;
}



// About  Write Read Timeout //
static int Set_sockopt ( S_RdWrTmO *obj , int fd , UInt wr_ms , UInt rd_ms ) ;
static int Set_sockopt ( S_RdWrTmO *obj , int fd , UInt wr_ms , UInt rd_ms ) {
	struct timeval setTimeStu = { 0 } ;
	// 寫
	setTimeStu.tv_sec = wr_ms / 1000 ;
	setTimeStu.tv_usec = ( wr_ms - ( setTimeStu.tv_sec * 1000 ) ) * 1000 ;
	if ( 0 != setsockopt ( fd , SOL_SOCKET , SO_SNDTIMEO , & setTimeStu , sizeof(struct timeval) ) ) {
		return - 1 ;
	}
	// 讀
	setTimeStu.tv_sec = rd_ms / 1000 ;
	setTimeStu.tv_usec = ( rd_ms - ( setTimeStu.tv_sec * 1000 ) ) * 1000 ;
	if ( 0 != setsockopt ( fd , SOL_SOCKET , SO_RCVTIMEO , & setTimeStu , sizeof(struct timeval) ) ) {
		return - 2 ;
	}
	return D_success ;
}

S_RdWrTmO *S_RdWrTmO_tool_New ( void ) {
	S_RdWrTmO *p_tmp = ( S_RdWrTmO* ) calloc ( 1 , sizeof(S_Https) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}
	p_tmp->Set_sockopt = Set_sockopt ;
	return p_tmp ;
}
void S_RdWrTmO_tool_Delete ( S_RdWrTmO **obj ) {
	free ( * obj ) ;
	* obj = NULL ;
	return ;
}

























