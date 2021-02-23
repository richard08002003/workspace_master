/*
 * Http_HomeLock.c
 *
 *  Created on: 2018年4月3日
 *      Author: richard
 */

#include "Http_HomeLock.h"

#define DF_http_head 			"POST /%s HTTP/1.1\r\nHost: %s\r\nContent-Length: %d\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n"
#define DF_http_data			"data=%s"

#define DF_ipc_path				"/usr/local/HomeLock/ipc/Http_HomeLock.ipc"
#define DF_http_config_path		"/usr/local/HomeLock/Http_HomeLock/config/Http_HomeLock_Http.config"		// 預授權交易金額設定檔路徑 //

// ********** 僅供內部使用 ********** //
typedef struct _S_Http_HomeLock_private_data {
	char i_http_ip [ 512 + 1 ] ;
	char i_http_port [ 512 + 1 ] ;

	Str i_mlc_http_host ;
	Str i_mlc_http_api ;

	S_Epoll *i_epl_obj ;
//	S_Https *i_http_obj ;
	S_Tcp_client *i_tcp_obj ;
	S_Ipc_server *i_ipc_obj ;
	Str i_mlc_http_read_data ;
} S_Http_HomeLock_private_data ;

// <<< About Client Information >>> //
typedef struct _S_client_info {
	int i_client_fd ;		// client accept fd
	Str i_mlc_data ;
	int i_mlc_data_sz ;
} S_client_info ;

static Str S_Get_config_val ( CStr config_path , CStr key ) ;							// 取設定檔的值
static int S_Load_config ( S_Http_HomeLock *obj ) ;										// 載入設定檔
static int S_Initial ( S_Http_HomeLock *obj , CStr Pgm_name ) ;							// 初始化

static int S_Re_connect_http ( S_Http_HomeLock *obj ) ;									// (HTTP)重新連線
static int S_Http_Send ( S_Http_HomeLock *obj , Str api , Str host , Str data ) ;		// (HTTP)寫資料
static int S_Http_Read ( S_Http_HomeLock *obj ) ;										// (HTTP)讀資料

static int S_Create_ipc_srv ( S_Http_HomeLock *obj , CStr srvFile , UInt listenNum ) ;	// (IPC)建立Server
static int S_Detect_ipc_srv ( S_Http_HomeLock *obj ) ;									// (IPC)判斷是否斷線
static void S_Close_ipc_srv ( S_Http_HomeLock *obj ) ;									// (IPC)關閉Server

static int S_Api_process ( S_Http_HomeLock *obj , Str will_send ) ;

static int S_Pgm_Begin ( S_Http_HomeLock *obj , CStr Pgm_name ) ;						// 程式起始
// ********** 僅供內部使用 ********** //


#define DF_Keyname "$%s"
static Str S_Get_config_val ( CStr config_path , CStr key ) {
	if ( ( ! config_path ) || ( ! key ) ) {
		return NULL ;
	}
	S_File *file_obj = S_File_tool_New ( ) ;
	if ( NULL == file_obj ) {
		return NULL ;
	}
	int rtn = file_obj->Read_str_from_file ( file_obj , config_path ) ;
	if ( D_success != rtn ) {
		S_File_tool_Delete ( & file_obj ) ;
		return NULL ;
	}
	Str config_data = file_obj->Rtn_str_from_file ( file_obj ) ;
	if ( NULL == config_data ) {
		S_File_tool_Delete ( & file_obj ) ;
		return NULL ;
	}
	Str value = NULL ;
	// KeyName
	char keyname [ strlen ( DF_Keyname ) + strlen ( key ) + 1 ] ;
	memset ( keyname , 0 , sizeof ( keyname ) ) ;
	snprintf ( keyname , sizeof ( keyname ) , DF_Keyname , key ) ;

	char *ptr = strstr ( config_data , keyname ) ;
	Str data_tmp = ( Str ) calloc ( strlen ( config_data ) + 1 , 1 ) ;
	if ( NULL == data_tmp ) {
		return NULL ;
	}
	memcpy ( data_tmp , ptr , strlen ( ptr ) ) ;

	Str data_ptr = data_tmp ;
#define DF_quotation_mark	"\""
	Str once_ptr = strstr ( data_ptr , DF_quotation_mark ) ;
	int once_ptr_sz = strlen ( once_ptr ) ;
	once_ptr += 1 ;
	Str seconde_ptr = strstr ( once_ptr , DF_quotation_mark ) ;
	int second_ptr_sz = strlen ( seconde_ptr ) ;
	int value_sz = once_ptr_sz - second_ptr_sz - 1 ;	// 去掉"的長度 //
	value = ( Str ) calloc ( value_sz + 1 , 1 ) ;
	if ( NULL == value ) {
		return NULL ;
	}
	memcpy ( value , once_ptr , value_sz ) ;
	free ( data_tmp ) ;
	data_tmp = NULL ;
	S_File_tool_Delete ( & file_obj ) ;
	return value ;
}
static int S_Load_config ( S_Http_HomeLock *obj ) {
	if ( ! obj ) {
		return - 1 ;
	}
	S_Http_HomeLock_private_data *p_private = ( S_Http_HomeLock_private_data * ) obj->i_private ;
	memset ( p_private->i_http_ip , 0 , sizeof ( p_private->i_http_ip ) ) ;
	memset ( p_private->i_http_port , 0 , sizeof ( p_private->i_http_port ) ) ;
	char *mlc_ip = S_Get_config_val ( DF_http_config_path , "IP" ) ;
	if ( NULL == mlc_ip ) {
		return - 2 ;
	}
	memcpy ( p_private->i_http_ip , mlc_ip , strlen ( mlc_ip ) ) ;
	free ( mlc_ip ) ;
	mlc_ip = NULL ;

	char *mlc_port = S_Get_config_val ( DF_http_config_path , "Port" ) ;
	if ( NULL == mlc_port ) {
		return - 3 ;
	}
	memcpy ( p_private->i_http_port , mlc_port , strlen ( mlc_port ) ) ;
	free ( mlc_port ) ;
	mlc_port = NULL ;

	if ( NULL != p_private->i_mlc_http_host ) {
		free ( p_private->i_mlc_http_host ) ;
		p_private->i_mlc_http_host = NULL ;
	}
	p_private->i_mlc_http_host = S_Get_config_val ( DF_http_config_path , "Host" ) ;
	if ( NULL == p_private->i_mlc_http_host ) {
		return - 4 ;
	}

	if ( NULL != p_private->i_mlc_http_api ) {
		free ( p_private->i_mlc_http_api ) ;
		p_private->i_mlc_http_api = NULL ;
	}
	p_private->i_mlc_http_api = S_Get_config_val ( DF_http_config_path , "Api" ) ;
	if ( NULL == p_private->i_mlc_http_api ) {
		return - 5 ;
	}
	return D_success ;
}
static int S_Initial ( S_Http_HomeLock *obj , CStr Pgm_name ) {
	if ( ( ! obj ) || ( ! Pgm_name ) ) {
		return - 1 ;
	}
	S_Http_HomeLock_private_data *p_private = ( S_Http_HomeLock_private_data * ) obj->i_private ;
	// http 預設值 //
	FILE *config_fp = fopen ( DF_http_config_path , "r+" ) ;
	if ( NULL == config_fp ) {
		while ( 1 ) {
			config_fp = fopen ( DF_http_config_path , "w" ) ;	// 開檔案，可寫
			if ( config_fp != NULL ) {
				break ;
			}
			sleep ( 1 ) ;
		}
		fprintf ( config_fp , "%s" , "$IP   = \"172.16.65.231\"\n" ) ;
		fprintf ( config_fp , "%s" , "$Port = \"443\"\n" ) ;
		fprintf ( config_fp , "%s" , "$Host = \"172.16.65.231\"\n" ) ;
		fprintf ( config_fp , "%s" , "$Api = \"api/cardVerify.php\"\n" ) ;
		fflush ( config_fp ) ;
		fclose ( config_fp ) ;
		config_fp = NULL ;
	} else {
		fclose ( config_fp ) ;
		config_fp = NULL ;
	}
	// 載入設定檔
	int rtn = S_Load_config ( obj ) ;
	if ( D_success != rtn ) {
		return -2 ;
	}
	// Epoll //
	p_private->i_epl_obj= S_Epoll_New ( ) ;
	if ( NULL == p_private->i_epl_obj ) {
		g_log->Save ( g_log , "Error S_Epoll_New error" ) ;
		return -3 ;
	}
	// Http Client //
	p_private->i_tcp_obj = S_Tcp_client_tool_New ( ) ;
	if ( NULL == p_private->i_tcp_obj ) {
		g_log->Save ( g_log , "Error S_Https_tool_New error" ) ;
		S_Epoll_Delete ( & p_private->i_epl_obj ) ;
		return -4 ;
	}
	rtn = p_private->i_tcp_obj->Setup ( p_private->i_tcp_obj , p_private->i_http_ip , p_private->i_http_port , 2 ) ;
	if ( D_success != rtn ) {
		return -5 ;
	}
	// IPC Server //
	p_private->i_ipc_obj = S_Ipc_Server_New ( ) ;
	if ( NULL == p_private->i_ipc_obj ) {
		g_log->Save ( g_log , "Error S_IPC_New error" ) ;
		S_Epoll_Delete ( & p_private->i_epl_obj ) ;
		S_Tcp_client_tool_Delete ( & p_private->i_tcp_obj ) ;
		return - 6 ;
	}
	return D_success ;
}

// <<< About HTTP >>> //
static int S_Re_connect_http ( S_Http_HomeLock *obj ) {
	if ( ! obj ) {
		return - 1 ;
	}
	S_Http_HomeLock_private_data *p_private = ( S_Http_HomeLock_private_data * ) obj->i_private ;
	g_log->Save ( g_log , "Http break ,will re_connect" ) ;
	while ( 1 ) {
		int rtn = p_private->i_tcp_obj->Connect ( p_private->i_tcp_obj ) ;
		if ( D_success == rtn ) {
			int sock_fd = p_private->i_tcp_obj->Rtn_sock_fd ( p_private->i_tcp_obj ) ;
			g_log->Save ( g_log , "HTTPS server re_connect success, sock_fd:%d, ip:%s, port:%s" , sock_fd , p_private->i_http_ip , p_private->i_http_port ) ;
			break ;
		}
		g_log->Save ( g_log , "Http reconnect fail" ) ;
		sleep ( 5 ) ;
	}
	return D_success ;
}
static int S_Http_Send ( S_Http_HomeLock *obj , Str api , Str host , Str data ) {
	if ( ( ! obj ) || ( ! api ) || ( ! host ) || ( ! data ) ) {
		return - 1 ;
	}
	S_Http_HomeLock_private_data *p_private = ( S_Http_HomeLock_private_data * ) obj->i_private ;

	int data_len = strlen ( data ) + 5 ;
	char http_header [ strlen ( DF_http_head ) + strlen ( api ) + strlen ( host ) + 4 + 1 ] ;
	memset ( http_header , 0 , sizeof ( http_header ) ) ;
	snprintf ( http_header , sizeof ( http_header ) , DF_http_head , api , host , data_len ) ;

	// URL encode
#if 0
	Str url_data = Url_encode( data );
	char http_data [ strlen ( DF_http_data ) + strlen ( url_data ) + 1 ] ;
	memset ( http_data , 0 , sizeof ( http_data ) ) ;
	snprintf ( http_data , sizeof ( http_data ) , DF_http_data , url_data ) ;
	free ( url_data ) ;
	url_data = NULL ;
#endif

	char http_data [ strlen ( DF_http_data ) + strlen ( data ) + 1 ] ;
	memset ( http_data , 0 , sizeof ( http_data ) ) ;
	snprintf ( http_data , sizeof ( http_data ) , DF_http_data , data ) ;


	char http_post [ strlen ( http_header ) + strlen ( http_data ) + 1 ] ;
	memset ( http_post , 0 , sizeof ( http_post ) ) ;
	snprintf ( http_post , sizeof ( http_post ) , "%s%s" , http_header , http_data ) ;
	g_log->Save ( g_log , "Will http_post data : \n<=====\n%s\n=====>" , http_post ) ;

	int rtn = p_private->i_tcp_obj->Send_str ( p_private->i_tcp_obj , http_post ) ;
	if ( rtn < 0 ) {
		g_log->Save ( g_log , "Error Http Send_str error, rtn=%d" , rtn ) ;
		if ( rtn == - 3 ) {
			S_Re_connect_http ( obj ) ;
		}
		return - 2 ;
	}
	g_log->Save ( g_log , "Will Send_str data success, bytes: %d" , rtn ) ;
	return D_success ;
}
static int S_Http_Read ( S_Http_HomeLock *obj ) {
	if ( ! obj ) {
		return -1 ;
	}
	S_Http_HomeLock_private_data *p_private = ( S_Http_HomeLock_private_data * ) obj->i_private ;

	// 讀資料
	int rtn = p_private->i_tcp_obj->Read_str ( p_private->i_tcp_obj) ;
	if ( rtn != D_success ) {	// 讀失敗
		g_log->Save ( g_log , "Error Http Read_str error , rtn=%d" , rtn ) ;
		if ( rtn == - 3 ) {
			S_Re_connect_http ( obj ) ;
		}
		return - 2 ;
	}
	g_log->Save ( g_log , "tcp Read_str , rtn=%d" , rtn ) ;

int read_bytes = p_private->i_tcp_obj->Rtn_read_str_bytes ( p_private->i_tcp_obj ) ;
g_log->Save ( g_log , "tcp read_bytes , rtn=%d" , read_bytes ) ;

	Str read_str = p_private->i_tcp_obj->Rtn_read_str ( p_private->i_tcp_obj ) ;	// 取資料
	g_log->Save ( g_log , "Xps input : \n<=====\n%s\n=====>" , read_str ) ;

	if ( NULL == read_str) {
		g_log->Save ( g_log , "tcp Read data error, len=%d" , strlen(read_str) ) ;
	}

	Str url_decode = Url_decode ( read_str ) ;
	// Decode : 將HTTP 的表頭去掉，把資料部份取出來
	if ( NULL != p_private->i_mlc_http_read_data ) {
		free ( p_private->i_mlc_http_read_data ) ;
		p_private->i_mlc_http_read_data = NULL ;
	}
	p_private->i_mlc_http_read_data = Get_Http_Json_data ( url_decode ) ;
	free ( url_decode ) ;
	url_decode = NULL ;
	if ( NULL == p_private->i_mlc_http_read_data ) {
		g_log->Save ( g_log , "Error Get_Http_Json_data error") ;
		return -3 ;
	}

	// 寫回去給Reader_HomeLock
	g_log->Save ( g_log , "Will write to ipc, fd :%d , str : %s" , p_private->i_ipc_obj->Rtn_acpt_fd ( p_private->i_ipc_obj ) , p_private->i_mlc_http_read_data ) ;
	rtn = p_private->i_ipc_obj->Write ( p_private->i_ipc_obj , p_private->i_mlc_http_read_data ) ;
	if ( D_success != rtn ) {
		g_log->Save ( g_log , "Error write to ipc error" ) ;
		return - 4 ;
	}
	return D_success ;
}
// <<< End of About HTTP >>> //

// <<< About IPC >>> //
static int S_Create_ipc_srv ( S_Http_HomeLock *obj , CStr srvFile , UInt listenNum ) {
	if ( ( ! obj ) || ( ! srvFile ) ) {
		return - 1 ;
	}
	S_Http_HomeLock_private_data *p_private = ( S_Http_HomeLock_private_data * ) obj->i_private ;
	int rtn = p_private->i_ipc_obj->Server_begin ( p_private->i_ipc_obj , srvFile , listenNum ) ;
	if ( D_success != rtn ) {
		return - 2 ;
	}
	int srv_fd = p_private->i_ipc_obj->Rtn_srv_fd(p_private->i_ipc_obj) ;
	rtn = p_private->i_epl_obj->Add_EpollCtl ( p_private->i_epl_obj , srv_fd , EPOLLIN ) ;
	if ( D_success != rtn ) {
		S_Close_ipc_srv ( obj ) ;
		return rtn ;   // 因為 return 回去程式就結束了 , 所以無須另外回吐資訊 //
	}
	return D_success ;
}
static int S_Detect_ipc_srv ( S_Http_HomeLock *obj ) {	// 判斷Server是否還有連線，若斷線則重啟Server
	if ( ! obj ) {
		return - 1 ;
	}
	S_Http_HomeLock_private_data *p_private = ( S_Http_HomeLock_private_data * ) obj->i_private ;
	if ( - 1 != p_private->i_ipc_obj->Rtn_srv_fd ( p_private->i_ipc_obj ) ) {
		return D_success ;
	}
	return S_Create_ipc_srv ( obj , DF_ipc_path , MAXEVENTS ) ;
}
static void S_Close_ipc_srv ( S_Http_HomeLock *obj ) {
	if ( ! obj ) {
		return ;
	}
	S_Http_HomeLock_private_data *p_private = ( S_Http_HomeLock_private_data * ) obj->i_private ;
	p_private->i_ipc_obj->Close_srv_fd ( p_private->i_ipc_obj ) ;
	return ;
}
// <<< End of About IPC >>> //

static int S_Api_process ( S_Http_HomeLock *obj , Str will_send ) {
	if ( ( ! obj ) || ( ! will_send ) ) {
		return - 1 ;
	}
	g_log->Save ( g_log , "## S_Api_process" ) ;
	S_Http_HomeLock_private_data *p_private = ( S_Http_HomeLock_private_data * ) obj->i_private ;

	// Https Client //
	int rtn = 0 ;
	g_log->Save ( g_log , "Will connect to HTTP server, ip = %s, port = %s" , p_private->i_http_ip , p_private->i_http_port ) ;
	while ( 1 ) {	// 直到連上Https Server
		rtn = p_private->i_tcp_obj->Connect ( p_private->i_tcp_obj ) ;
		if ( D_success == rtn ) {
			int sock_fd = p_private->i_tcp_obj->Rtn_sock_fd ( p_private->i_tcp_obj ) ;
			g_log->Save ( g_log , "HTTPS server connect success, sock_fd : %d, ip : %s, port : %s" , sock_fd , p_private->i_http_ip , p_private->i_http_port ) ;
			break ;
		}
		g_log->Save ( g_log , "HTTPS server connect error , rtn : %d\n" , rtn ) ;
		sleep ( 1 ) ;
	}

	// Send to Https Server
	rtn = S_Http_Send ( obj , p_private->i_mlc_http_api , p_private->i_mlc_http_host , will_send ) ;
	if ( D_success != rtn ) {
		g_log->Save ( g_log , "Error S_Http_Send error , rtn = %d" , rtn ) ;
		p_private->i_tcp_obj->Close ( p_private->i_tcp_obj ) ;
		return - 2 ;
	}

	// Read from Https Server & Send to IPC Client
	rtn = S_Http_Read ( obj ) ;
	if ( D_success != rtn ) {
		g_log->Save ( g_log , "Error S_Http_Read error , rtn = %d" , rtn ) ;
		p_private->i_tcp_obj->Close ( p_private->i_tcp_obj ) ;
		return - 3 ;
	}

	p_private->i_tcp_obj->Close ( p_private->i_tcp_obj ) ;
	return D_success ;
}

#define D_connect_num ( 5 )
static int S_Pgm_Begin ( S_Http_HomeLock *obj , CStr Pgm_name ) {
	if ( ( ! obj ) || ( ! Pgm_name ) ) {
		return - 1 ;
	}
	g_log->Save ( g_log , "## Start S_Pgm_Begin()" ) ;

	S_Http_HomeLock_private_data *p_private = ( S_Http_HomeLock_private_data * ) obj->i_private ;
	// 初始化
	int rtn = S_Initial ( obj , DF_Pgm_Name ) ;
	if ( D_success != rtn ) {
		g_log->Save ( g_log , "Error S_Initial error" ) ;
		return - 2 ;
	}
	g_log->Save ( g_log , "S_Initial success" ) ;

	// Epoll //
	p_private->i_epl_obj->Setup_wait_num ( p_private->i_epl_obj , D_connect_num ) ;	// 設定等待數量
	p_private->i_epl_obj->Setup_wait_time_ms ( p_private->i_epl_obj , 10 ) ;		// 設定時間
	rtn = p_private->i_epl_obj->Create ( p_private->i_epl_obj ) ;
	if ( D_success != rtn ) {
		g_log->Save ( g_log , "Error epoll Create error, rtn = %d" , rtn ) ;
		S_Epoll_Delete ( & p_private->i_epl_obj ) ;
		S_Tcp_client_tool_Delete ( & p_private->i_tcp_obj ) ;
		S_Ipc_Server_Delete ( & p_private->i_ipc_obj ) ;
		return - 3 ;
	}
	int epoll_fd = p_private->i_epl_obj->Rtn_epl_fd ( p_private->i_epl_obj ) ;
	g_log->Save ( g_log , "Success create epoll, i_epl_fd = %d" , epoll_fd ) ;

	// IPC Server //
	rtn = p_private->i_ipc_obj->Server_begin ( p_private->i_ipc_obj , DF_ipc_path , MAXEVENTS ) ;
	if ( D_success != rtn ) {
		g_log->Save ( g_log , "Error create ipc server:%s error, rtn = %d" , DF_ipc_path , rtn ) ;
		S_Epoll_Delete ( & p_private->i_epl_obj ) ;
		S_Tcp_client_tool_Delete ( & p_private->i_tcp_obj ) ;
		S_Ipc_Server_Delete ( & p_private->i_ipc_obj ) ;
		return - 4 ;
	}
	int srv_fd = p_private->i_ipc_obj->Rtn_srv_fd ( p_private->i_ipc_obj ) ;				// 取得ipc server fd
	g_log->Save ( g_log , "Success create ipc server, srv_fd = %d" , srv_fd ) ;

	rtn = p_private->i_epl_obj->Add_EpollCtl ( p_private->i_epl_obj , srv_fd , EPOLLIN ) ;	// 加入epoll 監控
	if ( D_success != rtn ) {
		g_log->Save ( g_log , "Error epoll addctl error, rtn = %d" , rtn ) ;
		S_Epoll_Delete ( & p_private->i_epl_obj ) ;
		S_Tcp_client_tool_Delete ( & p_private->i_tcp_obj ) ;
		S_Ipc_Server_Delete ( & p_private->i_ipc_obj ) ;
		return - 5 ;
	}
	g_log->Save ( g_log , "Success epoll addctl, add fd = %d" , srv_fd ) ;


	S_client_info client_acpt [ MAXEVENTS ] = { { 0 } } ;
	int accept_fd = 0 ;
	struct epoll_event evnts [ p_private->i_epl_obj->Rtn_epl_n ( p_private->i_epl_obj ) ] ;
	memset ( & evnts , 0 , sizeof(struct epoll_event) ) ;
	int epll_wat_err_ct = 0 , i = 0 ;
	while ( 1 ) {
		errno = 0 ;
		if ( epll_wat_err_ct >= 20 ) {	// 異常太多
			g_log->Save ( g_log , "Error epoll_wait most error occurs and will close, epll_wat_err_ct = %d" , epll_wat_err_ct ) ;
			S_Epoll_Delete ( & p_private->i_epl_obj ) ;
			S_Tcp_client_tool_Delete ( & p_private->i_tcp_obj ) ;
			S_Ipc_Server_Delete ( & p_private->i_ipc_obj ) ;
			return - 6 ;
		}

		int evn_num = p_private->i_epl_obj->Wait_Epoll ( p_private->i_epl_obj , & evnts ) ;
		if ( evn_num < 0 ) {
			epll_wat_err_ct ++ ;
			g_log->Save ( g_log , "Error epoll wait errno = %d" , errno ) ;
			sleep ( 3 ) ;
			continue ;
		}

		for ( i = 0 ; i < evn_num ; i ++ ) {
			UInt trigger_evnts = evnts [ i ].events ;
			int trigger_fd = evnts [ i ].data.fd ;
			if ( trigger_evnts & EPOLLHUP ) {	// 1. 斷線 //
				p_private->i_epl_obj->Del_EpollCtl ( p_private->i_epl_obj , trigger_fd ) ;	// epoll 刪除斷線client的事件
				if ( trigger_fd == srv_fd ) {	// ipc crash block //
					g_log->Save ( g_log , "Get event EPOLLHUP ipc server" ) ;
					p_private->i_ipc_obj->Close_srv_fd ( p_private->i_ipc_obj ) ;

					rtn = S_Detect_ipc_srv ( obj ) ;
					if ( D_success != rtn ) {
						g_log->Save ( g_log , "Error ipc re-Server_begin error, rtn = %d" , rtn ) ;
						continue ;
					}

				} else {	// ipc client
					int j = 0 ;
					for ( j = 0 ; j < MAXEVENTS ; j ++ ) {	// 掃描斷線所佔用的fd，並且將其歸零
						g_log->Save ( g_log , "Get event EPOLLHUP ,ipc client fd = %d" , client_acpt [ j ].i_client_fd ) ;
						if ( trigger_fd == client_acpt [ j ].i_client_fd ) {
							client_acpt [ j ].i_client_fd = 0 ;
							break ;
						}
					}
					close ( trigger_fd ) ;
				}
			} else if ( trigger_fd == srv_fd ) {	// 2. 新的Client連線
				g_log->Save ( g_log , "Get event EPOLLIN ipc server , new client connect" ) ;

				rtn = p_private->i_ipc_obj->Accept ( p_private->i_ipc_obj ) ;
				if ( D_success != rtn ) {
					g_log->Save ( g_log , "Error ipc Accept error, rtn = %d" , rtn ) ;
					continue ;
				}
				accept_fd = p_private->i_ipc_obj->Rtn_acpt_fd ( p_private->i_ipc_obj ) ;
				g_log->Save ( g_log , "Success accept, fd = %d" , accept_fd ) ;
				// 將新連線的fd加到SAcpt結構中
				int j = 0 ;
				for ( j = 0 ; j < MAXEVENTS ; j ++ ) {
					if ( client_acpt [ j ].i_client_fd == 0 ) {
						client_acpt [ j ].i_client_fd = accept_fd ;
						rtn = p_private->i_epl_obj->Add_EpollCtl ( p_private->i_epl_obj , accept_fd , EPOLLIN ) ; // 將新的client加入EPOLLIN型態的監控
						if ( D_success != rtn ) {
							close ( accept_fd ) ;
							g_log->Save ( g_log , "Error ipc srv accept fd = %d , add epoll EPOLLIN error , rtn = %d" , accept_fd , rtn ) ;
							continue ;
						}
						g_log->Save ( g_log , "New Client Connection, fd = %d" , client_acpt [ j ].i_client_fd ) ;	// 成功將新連線的fd加入監控
						break ;
					}
				}

			} else if ( trigger_evnts & EPOLLIN ) {	// 3. 讀取資料
				int j = 0 ;
				for ( j = 0 ; j < MAXEVENTS ; j ++ ) {
					if ( trigger_fd == client_acpt [ j ].i_client_fd ) {
						g_log->Save ( g_log , "Get event EPOLLIN , fd = %d" , trigger_fd ) ;

						accept_fd = trigger_fd ;

						struct timeval start , end ;
						gettimeofday ( & start , NULL ) ;

						struct timeval ss , ee ;

						// IPC 讀取資料 //
						gettimeofday ( & ss , NULL ) ;
						rtn = p_private->i_ipc_obj->Read ( p_private->i_ipc_obj , & client_acpt [ j ].i_mlc_data , & client_acpt [ j ].i_mlc_data_sz ) ;
						if ( D_success != rtn ) {
							g_log->Save ( g_log , "Error read ipc client error, rtn = %d" , rtn ) ;
							continue ;
						}
						g_log->Save ( g_log , "Client[%d] input all data = [%s], sz = %d" , client_acpt [ j ].i_client_fd , client_acpt [ j ].i_mlc_data , client_acpt [ j ].i_mlc_data_sz ) ;
						gettimeofday ( & ee , NULL ) ;
						long double time_use = ( ( ee.tv_sec * 1000000 ) + ee.tv_usec ) - ( ( ss.tv_sec * 1000000 ) + ss.tv_usec ) ;
						g_log->Save ( g_log , "#1. ipc Read Time use : %Lf秒" , ( time_use / 1000000 ) ) ;
						time_use = 0 ;

						// 解析並執行 //
						char *one_data = NULL;	// decode出來 的一筆資料
						int one_sz = 0 , dect = 0;
						while ( 1 ) {
//							g_log->Save ( g_log , "client_acpt [ j ].i_mlc_data_szs:%d" , client_acpt [ j ].i_mlc_data_sz ) ;

							if ( client_acpt [ j ].i_mlc_data_sz <= 0 ) {
								break ;
							}
							dect ++ ;
							g_log->Save ( g_log , "Decode One Data Times:%d" , dect ) ;	// decode 次數

							// Decode 資料
							gettimeofday ( & start , NULL ) ;
							rtn = Decode_one_data ( client_acpt [ j ].i_mlc_data , & client_acpt [ j ].i_mlc_data_sz , & one_data , & one_sz ) ;
							gettimeofday ( & end , NULL ) ;
							time_use = ( ( end.tv_sec * 1000000 ) + end.tv_usec ) - ( ( start.tv_sec * 1000000 ) + start.tv_usec ) ;
							g_log->Save ( g_log , "#2. MyDecode Time use : %Lf秒" , ( time_use / 1000000 ) ) ;
							time_use = 0 ;

							if ( D_success == rtn ) {
								// 解析成功，把資料送往HTTPS Server//
								g_log->Save ( g_log , "Decode success, one data:%s, len:%d" , one_data , one_sz ) ;
								rtn = S_Api_process ( obj , one_data ) ;
								if ( D_success != rtn ) {
									g_log->Save ( g_log , "Error S_Api_process error ,rtn:%d" , rtn ) ;
								}
							} else {
								// Decode 資料錯誤
								g_log->Save ( g_log , "Decode Data Error, error_rtn:%d" , rtn ) ;
								break ;
							}

							if ( NULL != one_data ) {
								free ( one_data ) ;
								one_data = NULL ;
							}

						}

						free ( client_acpt [ j ].i_mlc_data ) ;
						client_acpt [ j ].i_mlc_data = NULL ;
						client_acpt [ j ].i_mlc_data_sz = 0 ;

						gettimeofday ( & end , NULL ) ;
						time_use = ( ( end.tv_sec * 1000000 ) + end.tv_usec ) - ( ( start.tv_sec * 1000000 ) + start.tv_usec ) ;
						g_log->Save ( g_log , "### Total Time use : %Lf秒 ###" , ( time_use / 1000000 ) ) ;
					}
				}	// end of for loop
			}
		} // end of for loop
	} // end of while loop
	return D_success ;
}

S_Http_HomeLock * S_Http_HomeLock_New ( void ) {
	S_Http_HomeLock *p_tmp = ( S_Http_HomeLock* ) calloc ( 1 , sizeof(S_Http_HomeLock) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}
	S_Http_HomeLock_private_data *i_private_data = ( S_Http_HomeLock_private_data* ) calloc ( 1 , sizeof(S_Http_HomeLock_private_data) ) ;
	if ( NULL == i_private_data ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}

	memset ( i_private_data->i_http_ip , 0 , sizeof ( i_private_data->i_http_ip ) ) ;
	memset ( i_private_data->i_http_port , 0 , sizeof ( i_private_data->i_http_port ) ) ;
	i_private_data->i_mlc_http_host = NULL ;
	i_private_data->i_mlc_http_api = NULL ;
	i_private_data->i_epl_obj = NULL ;
	i_private_data->i_tcp_obj = NULL ;
	i_private_data->i_ipc_obj = NULL ;
	i_private_data->i_mlc_http_read_data = NULL ;
	p_tmp->i_private = ( void* ) i_private_data ;

	p_tmp->Pgm_Begin = S_Pgm_Begin ;
	return p_tmp ;
}

void S_Http_HomeLock_Delete ( S_Http_HomeLock** obj ) {
	S_Http_HomeLock_private_data *p_private = ( * obj )->i_private ;
	if ( NULL != p_private->i_epl_obj ) {
		S_Epoll_Delete ( & p_private->i_epl_obj ) ;
	}
	if ( NULL != p_private->i_tcp_obj ) {
		S_Tcp_client_tool_Delete ( & p_private->i_tcp_obj ) ;
	}
	if ( NULL != p_private->i_ipc_obj ) {
		S_Ipc_Server_Delete ( & p_private->i_ipc_obj ) ;
	}
	if ( NULL != p_private->i_mlc_http_read_data ) {
		free ( p_private->i_mlc_http_read_data ) ;
		p_private->i_mlc_http_read_data = NULL ;
	}
	if ( NULL != p_private->i_mlc_http_host ) {
		free ( p_private->i_mlc_http_host ) ;
		p_private->i_mlc_http_host = NULL ;
	}
	if ( NULL != p_private->i_mlc_http_api ) {
		free ( p_private->i_mlc_http_api ) ;
		p_private->i_mlc_http_api = NULL ;
	}
	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;
	free ( * obj ) ;
	* obj = NULL ;
	return ;
}
