/*
 * IPC.c
 *
 *  Created on: 2016/4/19
 *      Author: richard
 */

#include "IPC.h"

// 僅供內部使用 //
// Server //
typedef struct _S_server_private {
	int i_fd ;				// server fd
	int i_acpt_fd ;			// accept fd
	UInt i_listen_num ;		// listen number					// not use yet
	Str i_srv_path ;		// ipc server path

	int i_read_timeout ;
//	Str i_read_str ;		// read data
//	LInt i_read_str_sz ;
} S_server_private ;

static int S_Server_Server_begin ( S_Ipc_server *obj , CStr ipc_path , UInt listen_num ) ;
static int S_Server_Rtn_srv_fd ( S_Ipc_server *obj ) ;
static int S_Server_Accept ( S_Ipc_server *obj ) ;
static int S_Server_Rtn_acpt_fd ( S_Ipc_server *obj ) ;
static int S_Server_Set_read_timeout( S_Ipc_server *obj , int timeout) ;
static int S_Server_Write ( S_Ipc_server *obj , CStr str ) ;
static int S_Server_Read ( S_Ipc_server *obj , char **data , int *data_sz ) ;
//static Str S_Server_Rtn_read_data ( S_Ipc_server *obj ) ;
//static void S_Free_read_buf ( S_Ipc_server *obj ) ;
static int S_Server_Close_srv_fd ( S_Ipc_server *obj ) ;
static int S_Server_Close_acpt_fd ( S_Ipc_server *obj ) ;

//static int S_Make_myformat_data ( Str inputData , Str outputData , int outputSz ) ; // 2018/01/11
//static int S_Make_myformat_data ( Str inputData , Str outputData , int outputSz ) {
//	snprintf ( outputData , outputSz , "<S>%08X%s" , ( UInt ) strlen ( inputData ) , inputData ) ;
//	return D_success ;
//}
// 僅供內部使用 //

static int S_Server_Server_begin ( S_Ipc_server *obj , CStr ipc_path , UInt listen_num ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! ipc_path ) || ( ! listen_num ) ) {
		return - 1 ;
	}
	S_server_private *p_private = obj->i_private ;
	p_private->i_listen_num = 0 ;

	struct sockaddr_un server ;
	p_private->i_fd = socket ( AF_UNIX , SOCK_STREAM , 0 ) ;
	if ( p_private->i_fd < 0 ) {
		return - 2 ;
	}
	server.sun_family = AF_UNIX ;
	strcpy ( server.sun_path , ipc_path ) ;
	unlink ( server.sun_path ) ;  //刪除文件

	p_private->i_srv_path = ( Str ) calloc ( strlen ( ipc_path ) + 1 , sizeof(char) ) ;
	if ( NULL == p_private->i_srv_path ) {
		return - 3 ;
	}
	memcpy ( p_private->i_srv_path , ipc_path , strlen ( ipc_path ) ) ;

	// 設定reuse bind addr
	int reuse = 1 ;
	if ( setsockopt ( p_private->i_fd , SOL_SOCKET , SO_REUSEADDR , & reuse , sizeof ( reuse ) ) < 0 ) {
		return - 4 ;
	}

	if ( bind ( p_private->i_fd , ( struct sockaddr * ) & server , sizeof ( server ) ) < 0 ) {
		obj->Close_srv_fd ( obj ) ;
		return - 5 ;
	}

	if ( listen ( p_private->i_fd , listen_num ) < 0 ) {
		obj->Close_srv_fd ( obj ) ;
		return - 6 ;
	}
	p_private->i_listen_num = listen_num ;
	return D_success ;	// success
}

static int S_Server_Rtn_srv_fd ( S_Ipc_server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_server_private *p_private = obj->i_private ;
	return p_private->i_fd ;
}

static int S_Server_Accept ( S_Ipc_server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_server_private *p_private = obj->i_private ;

	struct sockaddr_un client;
	socklen_t clen = sizeof(client);
	p_private->i_acpt_fd = accept ( p_private->i_fd , (struct sockaddr *) &client , &clen  ) ;
	if ( - 1 == p_private->i_acpt_fd ) {
		return - 2 ;
	}

	// 設定 IPC timeout
	struct timeval set_time = { 0 } ;
	UInt write_ms = 10 ;
	UInt read_ms = 1 ;
	set_time.tv_sec = write_ms / 1000 ;
	set_time.tv_usec = ( write_ms - ( set_time.tv_sec * 1000 ) ) * 1000 ;
	if ( 0 != setsockopt ( p_private->i_acpt_fd , SOL_SOCKET , SO_SNDTIMEO , ( char * ) & set_time , sizeof(struct timeval) ) ) {
		return - 3 ;
	}

	set_time.tv_sec = read_ms / 1000 ;
	set_time.tv_usec = ( read_ms - ( set_time.tv_sec * 1000 ) ) * 1000 ;
	if ( 0 != setsockopt ( p_private->i_acpt_fd , SOL_SOCKET , SO_RCVTIMEO , ( char * ) & set_time , sizeof(struct timeval) ) ) {
		return - 4 ;
	}
	return D_success ;
}

static int S_Server_Rtn_acpt_fd ( S_Ipc_server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_server_private *p_private = obj->i_private ;
	return p_private->i_acpt_fd ;
}

static int S_Server_Set_read_timeout ( S_Ipc_server *obj , int timeout ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! timeout ) ) {
		return - 1 ;
	}
	S_server_private *p_private = obj->i_private ;
	if ( timeout < 0 ) {
		return - 2 ;
	}
	p_private->i_read_timeout = timeout ;
	return D_success ;
}

static int S_Server_Write ( S_Ipc_server *obj , CStr str ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! str ) ) {
		return - 1 ;
	}
	S_server_private *p_private = obj->i_private ;
	// 偵測Client是否斷線，斷線則不讀
	S_Epoll *epoll_obj = S_Epoll_New ( ) ;
	int check = epoll_obj->Detect_hup ( epoll_obj , p_private->i_acpt_fd ) ;
	S_Epoll_Delete ( & epoll_obj ) ;
	if ( - 1 == check ) {
		return - 2 ;	// 斷線
	}

	// 加Head與資料長度 <S> + 08X + JsonData	// 2018/01/11
	int rtn = write ( p_private->i_acpt_fd , str , strlen ( str ) ) ;
	if ( - 1 == rtn ) {
		return - 3 ;
	}
	return D_success ;
}

static int S_Server_Read ( S_Ipc_server *obj , char **data , int *data_sz ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_server_private *p_private = obj->i_private ;
	// 偵測Client是否斷線，斷線則不讀
	S_Epoll *epoll_obj = S_Epoll_New ( ) ;
	int check = epoll_obj->Detect_hup ( epoll_obj , p_private->i_acpt_fd ) ;
	S_Epoll_Delete ( & epoll_obj ) ;
	if ( - 1 == check ) {
		return - 1 ;	// 斷線
	}
	* data = NULL ;	// 存放讀取資料的指標
	* data_sz = 0 ;				// 大小
	time_t now_t = time ( NULL ) ;
	while ( p_private->i_read_timeout > ( time ( NULL ) - now_t ) ) {
		char buf [ 8192 + 1 ] = { 0 } ;
		int rtn = recv ( p_private->i_acpt_fd , buf , sizeof ( buf ) , 0 ) ;
		if ( rtn == - 1 ) {
			if ( * data_sz > 0 ) {	// 確認資料已經收完，沒有資料了
				break ;
			}
			continue ;
		} else if ( rtn == 0 ) {	// 錯誤
			return - 2 ;
		}

		* data = ( char * ) realloc ( * data , * data_sz + rtn + 1 ) ;
		memcpy ( * data + * data_sz , buf , rtn ) ;	// 將資料接到上筆資料量後面
		( * data ) [ * data_sz + rtn ] = 0 ;
		* data_sz += rtn ;	// 累計資料量
	}
	return D_success ;
}

//static Str S_Server_Rtn_read_data ( S_Ipc_server *obj ) {
//	if ( ( ! obj ) || ( ! obj->i_private ) ) {
//		return NULL ;
//	}
//	S_server_private *p_private = obj->i_private ;
//	return p_private->i_read_str ;
//}

//static void S_Free_read_buf ( S_Ipc_server *obj ) {
//	if ( ( ! obj ) || ( ! obj->i_private ) ) {
//		return ;
//	}
//	S_server_private *p_private = obj->i_private ;
//	if ( NULL != p_private->i_read_str ) {
//		free ( p_private->i_read_str ) ;
//		p_private->i_read_str = NULL ;
//	}
//	return ;
//}


static int S_Server_Close_srv_fd ( S_Ipc_server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_server_private *p_private = obj->i_private ;
	if ( - 1 != p_private->i_fd ) {
		close ( p_private->i_fd ) ;
		p_private->i_fd = - 1 ;
	}
	return D_success ;
}

static int S_Server_Close_acpt_fd ( S_Ipc_server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_server_private *p_private = obj->i_private ;
	if ( - 1 != p_private->i_acpt_fd ) {
		close ( p_private->i_acpt_fd ) ;
		p_private->i_acpt_fd = - 1 ;
	}
	return D_success ;
}

S_Ipc_server * S_Ipc_Server_New ( void ) {
	S_Ipc_server *p_tmp = ( S_Ipc_server* ) calloc ( 1 , sizeof(S_Ipc_server) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}

	S_server_private *i_private_data = ( S_server_private* ) calloc ( 1 , sizeof(S_server_private) ) ;
	if ( NULL == i_private_data ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}
	i_private_data->i_fd = - 1 ;
	i_private_data->i_acpt_fd = - 1 ;
	i_private_data->i_read_timeout = 2 ; // 預設2秒
//	i_private_data->i_read_str = NULL ;
//	i_private_data->i_read_str_sz = 0 ;
	p_tmp->i_private = ( void* ) i_private_data ;

	p_tmp->Server_begin = S_Server_Server_begin ;
	p_tmp->Rtn_srv_fd = S_Server_Rtn_srv_fd ;
	p_tmp->Accept = S_Server_Accept ;
	p_tmp->Rtn_acpt_fd = S_Server_Rtn_acpt_fd ;
	p_tmp->Set_read_timeout = S_Server_Set_read_timeout ;
	p_tmp->Write = S_Server_Write ;
	p_tmp->Read = S_Server_Read ;
//	p_tmp->Rtn_read_data = S_Server_Rtn_read_data ;
//	p_tmp->Free_read_buf = S_Free_read_buf ;
	p_tmp->Close_srv_fd = S_Server_Close_srv_fd ;
	p_tmp->Close_acpt_fd = S_Server_Close_acpt_fd ;

	return p_tmp ;
}

void S_Ipc_Server_Delete( S_Ipc_server** obj ) {
	S_server_private* p_private = ( * obj )->i_private ;
	free ( p_private->i_srv_path ) ;
	p_private->i_srv_path = NULL ;
//	free ( p_private->i_read_str ) ;
//	p_private->i_read_str = NULL ;
	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;
	free ( * obj ) ;
	* obj = NULL ;
	return ;
}




// Client //
typedef struct _S_client_private {
	int i_fd ;
	Str i_srv_path ;		// ipc server path
	int i_read_timeout ;
//	Str i_read_str ;		// read data
//	LInt i_read_str_sz ;
} S_client_private ;

static int S_Client_Connect ( S_Ipc_client *obj , CStr ipc_path ) ;
static int S_Client_Rtn_fd ( S_Ipc_client *obj ) ;
static Str S_Client_Rtn_ipc_path ( S_Ipc_client *obj ) ;	// (2018/04/03)
static int S_Client_Detect_connect ( S_Ipc_client*obj ) ;	// (2018/05/09)
static int S_Client_Re_connect ( S_Ipc_client*obj ) ;
static int S_Set_read_timeout ( S_Ipc_client *obj , int timeout ) ;
static int S_Client_Write ( S_Ipc_client*obj , CStr str ) ;
static int S_Client_Read ( S_Ipc_client *obj , char **data , int *data_sz ) ;
//static Str S_Client_Rtn_read_data ( S_Ipc_client*obj ) ;
static int S_Client_Close ( S_Ipc_client*obj ) ;

static int S_Client_Connect ( S_Ipc_client *obj , CStr ipc_path ) {
	S_client_private *p_private = obj->i_private ;
	p_private->i_fd = socket ( AF_UNIX , SOCK_STREAM , 0 ) ;
	if ( 0 > p_private->i_fd ) {
		return -1 ;
	}

	p_private->i_srv_path = ( Str ) calloc ( strlen ( ipc_path ) + 1 , sizeof(char) ) ;
	if ( NULL == p_private->i_srv_path ) {
		return - 2 ;
	}
	memcpy ( p_private->i_srv_path , ipc_path , strlen ( ipc_path ) ) ;

	struct timeval set_time = { 0 } ;
	UInt write_ms = 10 ;
	UInt read_ms = 1 ;
	// send timeout
	set_time.tv_sec = write_ms / 1000 ;
	set_time.tv_usec = ( write_ms - ( set_time.tv_sec * 1000 ) ) * 1000 ;
	if ( 0 != setsockopt ( p_private->i_fd , SOL_SOCKET , SO_SNDTIMEO , ( char * ) & set_time , sizeof(struct timeval) ) ) {
		return - 3 ;
	}

	// read timeout
	set_time.tv_sec = read_ms / 1000 ;
	set_time.tv_usec = ( read_ms - ( set_time.tv_sec * 1000 ) ) * 1000 ;
	if ( 0 != setsockopt ( p_private->i_fd , SOL_SOCKET , SO_RCVTIMEO , ( char * ) & set_time , sizeof(struct timeval) ) ) {
		return - 4 ;
	}

	struct sockaddr_un unStruct = { 0 } ;
	unStruct.sun_family = AF_UNIX ;
	snprintf ( unStruct.sun_path , sizeof ( unStruct.sun_path ) , "%s" , ipc_path ) ;
	int sizeLen = sizeof ( unStruct.sun_family ) + strlen ( unStruct.sun_path ) ;
	int rtn = connect ( p_private->i_fd , ( struct sockaddr * ) & unStruct , sizeLen ) ;
	if ( 0 > rtn ) {
		obj->Close(obj) ;
 		return - 5 ;
	}
	return D_success ;
}

static int S_Client_Rtn_fd ( S_Ipc_client *obj ) {
	S_client_private *p_private = obj->i_private ;
	return p_private->i_fd ;
}

// (2018/04/03)
static Str S_Client_Rtn_ipc_path ( S_Ipc_client *obj ) {
	S_client_private *p_private = obj->i_private ;
	return p_private->i_srv_path ;
}

static int S_Client_Detect_connect ( S_Ipc_client*obj ) {
	S_client_private *p_private = obj->i_private ;
	// 偵測Client是否斷線，斷線則不讀
	S_Epoll *epoll_obj = S_Epoll_New ( ) ;
	int check = epoll_obj->Detect_hup ( epoll_obj , p_private->i_fd ) ;
	S_Epoll_Delete ( & epoll_obj ) ;
	if ( - 1 == check ) {
		obj->Close ( obj ) ;
		return p_private->i_fd ;	// IPC斷線
	}
	return p_private->i_fd ;
}

static int S_Client_Re_connect ( S_Ipc_client *obj ) {
	S_client_private *p_private = obj->i_private ;
	obj->Close ( obj ) ;
	int rtn = obj->Connect ( obj , p_private->i_srv_path ) ;
	if ( D_success != rtn ) {
		return -1 ;
	}
	return D_success ;
}

static int S_Set_read_timeout ( S_Ipc_client *obj , int timeout ) {
	S_client_private *p_private = obj->i_private ;
 	if ( timeout < 0 ) {
		return - 1 ;
	}
	p_private->i_read_timeout = timeout ;
	return D_success ;
}

static int S_Client_Write ( S_Ipc_client *obj , CStr str ) {
	S_client_private *p_private = obj->i_private ;
	// 偵測Client是否斷線，斷線則不讀
	S_Epoll *epoll_obj = S_Epoll_New ( ) ;
	int check = epoll_obj->Detect_hup ( epoll_obj , p_private->i_fd ) ;
	S_Epoll_Delete ( & epoll_obj ) ;
	if ( - 1 == check ) {
//		obj->Close ( obj ) ;
		return - 1 ;	// IPC斷線
	}

	char will_send_buf[ 3 + 8 + (int) strlen( str ) + 1 ];
	memset( will_send_buf , 0 , sizeof(will_send_buf) );
	snprintf ( will_send_buf , sizeof(will_send_buf) , "<S>%08X%s" , (UInt)strlen( str ) , str ) ;
	int rtn = write ( p_private->i_fd , will_send_buf , strlen ( will_send_buf ) ) ;
	if ( - 1 == rtn ) {
		return - 2 ;
	}

	return D_success ;
}

static int S_Client_Read ( S_Ipc_client *obj , char **data , int *data_sz ) {
	S_client_private *p_private = obj->i_private ;
	// 偵測Client是否斷線，斷線則不讀
	S_Epoll *epoll_obj = S_Epoll_New ( ) ;
	int check = epoll_obj->Detect_hup ( epoll_obj , p_private->i_fd ) ;
	S_Epoll_Delete ( & epoll_obj ) ;
	if ( - 1 == check ) {
//		obj->Close ( obj ) ;
		return - 1 ;	// IPC斷線
	}

	* data = NULL ;		// 存放讀取資料的指標
	* data_sz = 0 ;		// 大小

//	if ( NULL != p_private->i_read_str ) {
//		free ( p_private->i_read_str ) ;
//		p_private->i_read_str = NULL ;
//	}

	time_t now_t = time ( NULL ) ;
	while ( p_private->i_read_timeout > ( time ( NULL ) - now_t ) ) {
		char buf [ 8192 + 1 ] = { 0 } ;
		int rtn = recv ( p_private->i_fd , buf , sizeof ( buf ) , 0 ) ;
		if ( rtn == - 1 ) {
			if ( * data_sz > 0 ) {	// 確認資料已經收完，沒有資料了
				break ;
			}
			continue ;
		} else if ( rtn == 0 ) {	// 錯誤
			return - 2 ;
		}

		* data = ( char* ) realloc ( * data , * data_sz + rtn + 1 ) ;
		memcpy ( * data + * data_sz , buf , rtn ) ;	// 將資料接到上筆資料量後面
		( * data ) [ * data_sz + rtn ] = 0 ;
		* data_sz += rtn ;	// 累計資料量
	}


//	fprintf ( stdout , "@@@@@@@@@@ %s @@@@@@@@@@@@@@\n" , p_private->i_read_str ) ;
//	fflush ( stdout ) ;
	return D_success ;
}


static int S_Client_Close ( S_Ipc_client*obj ) {
	S_client_private *p_private = obj->i_private ;
	if ( - 1 != p_private->i_fd ) {
		close ( p_private->i_fd ) ;
		p_private->i_fd = - 1 ;
	}
	return D_success ;
}

S_Ipc_client * S_Ipc_client_New ( void ) {
	S_Ipc_client *p_tmp = ( S_Ipc_client* ) calloc ( 1 , sizeof(S_Ipc_client) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}
	S_client_private *i_private_data = ( S_client_private* ) calloc ( 1 , sizeof(S_client_private) ) ;
	if ( NULL == i_private_data ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}
	i_private_data->i_fd = - 1 ;
	i_private_data->i_read_timeout = 2 ;	// 預設2秒
//	i_private_data->i_read_str_sz = 0 ;

	p_tmp->i_private = ( void* ) i_private_data ;

	p_tmp->Connect = S_Client_Connect ;
	p_tmp->Rtn_fd = S_Client_Rtn_fd ;
	p_tmp->Rtn_ipc_path = S_Client_Rtn_ipc_path ;

	p_tmp->Detect_connect = S_Client_Detect_connect ;

	p_tmp->Re_connect = S_Client_Re_connect ;
	p_tmp->Set_read_timeout = S_Set_read_timeout ;
	p_tmp->Write = S_Client_Write ;
	p_tmp->Read = S_Client_Read ;
//	p_tmp->Rtn_read_data = S_Client_Rtn_read_data ;
	p_tmp->Close = S_Client_Close ;

	return p_tmp ;
}

void S_Ipc_client_Delete( S_Ipc_client** obj ) {
	S_client_private *p_private = ( * obj )->i_private ;

	free ( p_private->i_srv_path ) ;
	p_private->i_srv_path = NULL ;

//	free ( p_private->i_read_str ) ;
//	p_private->i_read_str = NULL ;

	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;

	free ( * obj ) ;
	* obj = NULL ;
}
