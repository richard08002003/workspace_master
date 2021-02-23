/*
 * Env_HomeLock.c
 *
 *  Created on: 2018年1月30日
 *      Author: richard
 */
#include "Env_HomeLock.h"

#define DF_mlc_ipc_path		"/usr/local/HomeLock/ipc/Env_HomeLock.ipc"

// <<< About Client Information >>> //
typedef struct _S_client_info {
	int i_client_fd ;		// client accept fd
	Str i_mlc_data ;
	int i_mlc_data_sz ;
} S_client_info ;

static temp_humidity_ret_ssc *get_temperature_humitidy ( void ) ;
static pm_ret_ssc *get_pm_info( void ) ;
static int decode_pm_info ( Str input_data , int *pm1_0 , int *pm2_5 , int *pm10 ) ;
static void S_Api_process ( Ipc_in_list *input_list , S_Ipc_server *ipc_obj ) ;		// 執行Api process:裡面包含回寫ipc client的response //

static temp_humidity_ret_ssc *get_temperature_humitidy ( void ) {
	temp_humidity_ret_ssc *ssc = ( temp_humidity_ret_ssc* ) calloc ( 1 , sizeof(temp_humidity_ret_ssc) ) ;
	if ( ssc == NULL ) {
		return NULL ;
	}

	float Temperature = 0.0 ;
	float Humidity = 0.0 ;
	int rtn = pi_dht_read ( 11 , 2 , & Humidity , & Temperature ) ;	// gpio2
	ssc->ret_code = rtn ;
	ssc->humidity = Humidity ;
	ssc->temperature = Temperature ;
	g_log->Save ( g_log , "pi_dht_read rtn = %d" , ssc->ret_code ) ;
	g_log->Save ( g_log , "Humidity = %04f" , ssc->humidity ) ;
	g_log->Save ( g_log , "Temperature = %04f" , ssc->temperature ) ;
	return ssc ;
}

static pm_ret_ssc *get_pm_info ( void ) {
	pm_ret_ssc *ssc = ( pm_ret_ssc* ) calloc ( 1 , sizeof(pm_ret_ssc) ) ;
	if ( ssc == NULL ) {
		return NULL ;
	}

	S_RS232 *serial_obj = S_RS232_New ( ) ;
	if ( NULL == serial_obj ) {
		free ( ssc ) ;
		ssc = NULL ;
		return NULL ;
	}
//	int rtn = serial_obj->Setup ( serial_obj , "9600" , "/dev/ttyS0" , 2 ) ;
	int rtn = serial_obj->Setup ( serial_obj , "9600" , "/dev/ttyAMA0" , 2 ) ;
	if ( D_success != rtn ) {
		free ( ssc ) ;
		ssc = NULL ;
		S_RS232_Delete ( & serial_obj ) ;
		return NULL ;
	}

	rtn = serial_obj->Open_device ( serial_obj ) ;
	if ( D_success != rtn ) {
		g_log->Save ( g_log , "Error serial_obj Open_device error, rtn = %d" , rtn ) ;

		free ( ssc ) ;
		ssc = NULL ;
		S_RS232_Delete ( & serial_obj ) ;
		return NULL ;
	}

	serial_obj->Read ( serial_obj ) ;
	char *data = serial_obj->Rtn_read_data ( serial_obj ) ;
	g_log->Save ( g_log , "PMS3003 data = %s" , data ) ;
	int pm1_0 = 0 , pm2_5 = 0 , pm10 = 0 ;
	rtn = decode_pm_info ( data , & pm1_0 , & pm2_5 , & pm10 ) ;
	ssc->ret_code = rtn ;
	ssc->pm1_0 = pm1_0 ;
	ssc->pm2_5 = pm2_5 ;
	ssc->pm10 = pm10 ;
	g_log->Save ( g_log , "pi_dht_read rtn = %d" , ssc->ret_code ) ;
	g_log->Save ( g_log , "PM1.0 = %d ug/m3" , ssc->pm1_0 ) ;
	g_log->Save ( g_log , "PM2.5 = %d ug/m3" , ssc->pm2_5 ) ;
	g_log->Save ( g_log , "PM10 = %d ug/m3" , ssc->pm10 ) ;

	S_RS232_Delete ( & serial_obj ) ;
	return ssc ;
}

#define D_dust_header		"424D"
#define	D_dust_header_len	strlen(D_dust_header)
static int decode_pm_info ( Str input_data , int *pm1_0 , int *pm2_5 , int *pm10 ) {
	if ( ! input_data ) {
		return - 1 ;
	}

	char *input = input_data ;
	* pm1_0 = 0 ;
	* pm2_5 = 0 ;
	* pm10 = 0 ;

	char* ptrS = strstr ( input , D_dust_header ) ;
	if ( NULL == ptrS ) {
		return - 2 ;
	}

	ptrS += D_dust_header_len ;

	char frame_len [ 4 + 1 ] = { 0 } ;
	snprintf ( frame_len , sizeof ( frame_len ) , "%s" , ptrS ) ;
	ptrS += 4 ;

	int len = strtoul ( frame_len , NULL , 16 ) ;
	// 長度不足
	if ( ( len << 1 ) > strlen ( ptrS ) ) {
		return - 3 ;
	}

	char data_area [ ( len << 1 ) + 1 ] ;
	memset ( data_area , 0 , sizeof ( data_area ) ) ;
	snprintf ( data_area , sizeof ( data_area ) , "%s" , ptrS ) ;

	char s_pm1_0_cf [ 4 + 1 ] = { 0 } ;
	char s_pm2_5_cf [ 4 + 1 ] = { 0 } ;
	char s_pm10_cf [ 4 + 1 ] = { 0 } ;
	char s_pm1_0 [ 4 + 1 ] = { 0 } ;
	char s_pm2_5 [ 4 + 1 ] = { 0 } ;
	char s_pm10 [ 4 + 1 ] = { 0 } ;

	char *ptr_data = data_area ;
	snprintf ( s_pm1_0_cf , sizeof ( s_pm1_0_cf ) , "%s" , ptr_data ) ;
	ptr_data += 4 ;
	snprintf ( s_pm2_5_cf , sizeof ( s_pm2_5_cf ) , "%s" , ptr_data ) ;
	ptr_data += 4 ;
	snprintf ( s_pm10_cf , sizeof ( s_pm10_cf ) , "%s" , ptr_data ) ;
	ptr_data += 4 ;
	snprintf ( s_pm1_0 , sizeof ( s_pm1_0 ) , "%s" , ptr_data ) ;
	ptr_data += 4 ;
	snprintf ( s_pm2_5 , sizeof ( s_pm2_5 ) , "%s" , ptr_data ) ;
	ptr_data += 4 ;
	snprintf ( s_pm10 , sizeof ( s_pm10 ) , "%s" , ptr_data ) ;
	ptr_data += 4 ;

	* pm1_0 = strtoul ( s_pm1_0 , NULL , 16 ) ;
	* pm2_5 = strtoul ( s_pm2_5 , NULL , 16 ) ;
	* pm10 = strtoul ( s_pm10 , NULL , 16 ) ;

	return D_success ;
}

static void S_Api_process ( Ipc_in_list *input_list , S_Ipc_server *ipc_obj ) {
	S_list *in_list = ( S_list * ) input_list ;
	int list_num = Rtn_Linked_list_node_ct ( in_list ) ;
	int ct = 0 , list_ct = 0 ;
	Ret_ssc *result = NULL ;
	for ( ct = 0 ; ct < list_num ; ct ++ ) {
		++ list_ct ;
		switch ( in_list->type ) {
			case E_get_temperature_humitidy : {
				g_log->Save ( g_log , "Will run E_get_temperature_humitidy" ) ;
				temp_humidity_ret_ssc *ret_ssc = NULL ;
				ret_ssc = get_temperature_humitidy ( ) ;
				if ( ret_ssc != NULL ) {
					result = ( Ret_ssc * ) ret_ssc ;
				} else {
					g_log->Save ( g_log , "Error get_temperature_humitidy error" ) ;
				}
			}
				break ;
			case E_get_pm_info : {
				g_log->Save ( g_log , "Will run E_get_pm_info" ) ;
				pm_ret_ssc *ret_ssc ;
				ret_ssc = get_pm_info ( ) ;
				if ( ret_ssc != NULL ) {
					result = (Ret_ssc *) ret_ssc ;
				} else {
					g_log->Save ( g_log , "Error get_pm_info error" ) ;
				}
			}
				break ;
			default : {
				g_log->Save ( g_log , "Not this api" ) ;
			}
				break ;
		} // end of switch

		Respon *mlc_rspn = Setup_response ( result , in_list->p_data , in_list->type ) ;
		if ( NULL == mlc_rspn ) {
			// error
			g_log->Save ( g_log , "Error Setup_response error" ) ;
			in_list = in_list->next ;
			continue ;
		}

		Str respon = Make_api_respon ( mlc_rspn , in_list->type ) ;
		if ( NULL == mlc_rspn ) {
			// error
			g_log->Save ( g_log , "Error Make_api_respon error" ) ;
			free ( mlc_rspn ) ;
			mlc_rspn = NULL ;
			in_list = in_list->next ;
			continue ;
		}
		free ( mlc_rspn ) ;
		mlc_rspn = NULL ;

		g_log->Save ( g_log , "Will write to client is respon = [%s]" , respon ) ;
		// Send respon
		int rtn = ipc_obj->Write ( ipc_obj , respon ) ;
		if ( D_success != rtn ) {
			g_log->Save ( g_log , "Error write ipc client error, rtn = %d" , rtn ) ;
			in_list = in_list->next ;
			continue ;
		}
		g_log->Save ( g_log , "Write to client success, rtn = %d" , rtn ) ;


		free ( respon ) ;
		respon = NULL ;

		switch ( in_list->type ) {
			case E_get_temperature_humitidy : {
				temp_humidity_ret_ssc *ret_ssc = ( temp_humidity_ret_ssc* ) result ;
				free ( ret_ssc ) ;
				ret_ssc = NULL ;
			}
				break ;
			case E_get_pm_info : {
				pm_ret_ssc *ret_ssc = ( pm_ret_ssc* ) result ;
				free ( ret_ssc ) ;
				ret_ssc = NULL ;
			}
				break ;
			default :
				break ;
		}
		in_list = in_list->next ;
	} // end of for loop
	return ;
}

Ipc_in_list *ipc_in_list = NULL ;
int Pgm_Begin ( void ) {

#if 1
	// IPC Server //
	S_Ipc_server *ipc_obj = S_Ipc_Server_New ( ) ;
	if ( NULL == ipc_obj ) {
		g_log->Save ( g_log , "Error S_IPC_New error" ) ;
		return - 1 ;
	}
	int rtn = ipc_obj->Server_begin ( ipc_obj , DF_mlc_ipc_path , MAXEVENTS ) ;
	if ( D_success != rtn ) {
		g_log->Save ( g_log , "Error create ipc server:%s error, rtn = %d" , DF_mlc_ipc_path , rtn ) ;
		S_Ipc_Server_Delete ( & ipc_obj ) ;
		return - 2 ;
	}
	int srv_fd = ipc_obj->Rtn_srv_fd ( ipc_obj ) ;				// 取得ipc server fd
	g_log->Save ( g_log , "Success create ipc server, srv_fd = %d" , srv_fd ) ;

	// Epoll //
	S_Epoll *epl_obj = S_Epoll_New ( ) ;
	if ( NULL == epl_obj ) {
		g_log->Save ( g_log , "Error S_Epoll_New error" ) ;
		S_Ipc_Server_Delete ( & ipc_obj ) ;
		return - 5 ;
	}
	epl_obj->Setup_wait_num ( epl_obj , MAXEVENTS ) ;
	epl_obj->Setup_wait_time_ms ( epl_obj , - 1 ) ;
	rtn = epl_obj->Create ( epl_obj ) ;
	if ( D_success != rtn ) {
		g_log->Save ( g_log , "Error epoll Create error, rtn = %d" , rtn ) ;
		S_Ipc_Server_Delete ( & ipc_obj ) ;
		S_Epoll_Delete ( & epl_obj ) ;
		return - 6 ;
	}
	int epoll_fd = epl_obj->Rtn_epl_fd ( epl_obj ) ;
	g_log->Save ( g_log , "Success create epoll, i_epl_fd = %d" , epoll_fd ) ;

	rtn = epl_obj->Add_EpollCtl ( epl_obj , srv_fd , EPOLLIN ) ;
	if ( D_success != rtn ) {
		g_log->Save ( g_log , "Error epoll addctl error, rtn = %d" , rtn ) ;
		S_Ipc_Server_Delete ( & ipc_obj ) ;
		S_Epoll_Delete ( & epl_obj ) ;
		return - 7 ;
	}
	g_log->Save ( g_log , "Success epoll addctl, add fd = %d" , srv_fd ) ;

	S_client_info client_acpt [ MAXEVENTS ] = { { 0 } } ;
	int accept_fd = 0 ;
	struct epoll_event evnts [ epl_obj->Rtn_epl_n ( epl_obj ) ] ;
	memset ( & evnts , 0 , sizeof(struct epoll_event) ) ;
	int epll_wat_err_ct = 0 , i = 0 ;
	while ( 1 ) {
		errno = 0 ;
		if ( epll_wat_err_ct >= 20 ) {	// 異常太多
			g_log->Save ( g_log , "Error epoll_wait most error occurs and will close, epll_wat_err_ct = %d" , epll_wat_err_ct ) ;
			S_Ipc_Server_Delete ( & ipc_obj ) ;
			S_Epoll_Delete ( & epl_obj ) ;
			return - 8 ;
		}
		int evn_num = epl_obj->Wait_Epoll ( epl_obj , & evnts ) ;
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
				epl_obj->Del_EpollCtl ( epl_obj , trigger_fd ) ;	// epoll 刪除斷線client的事件

				if ( trigger_fd == srv_fd ) {	// ipc crash block //
					g_log->Save ( g_log , "Get event EPOLLHUP ipc server" ) ;
					ipc_obj->Close_srv_fd ( ipc_obj ) ;
					while ( 1 ) {	// 重啟IPC Server
						sleep ( 3 ) ;
						rtn = ipc_obj->Server_begin ( ipc_obj , DF_mlc_ipc_path , MAXEVENTS ) ;
						if ( D_success != rtn ) {
							g_log->Save ( g_log , "Error ipc re-Server_begin error, rtn = %d" , rtn ) ;
							continue ;
						}
						srv_fd = ipc_obj->Rtn_srv_fd ( ipc_obj ) ;
						rtn = epl_obj->Add_EpollCtl ( epl_obj , srv_fd , EPOLLIN ) ;
						if ( D_success != rtn ) {
							g_log->Save ( g_log , "Error re-epoll add ipc srv fd error, rtn = %d" , rtn ) ;
							continue ;
						}
						g_log->Save ( g_log , "Success re-create ,ipc server, fd = %d" , srv_fd ) ;
						break ;
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

				rtn = ipc_obj->Accept ( ipc_obj ) ;
				if ( D_success != rtn ) {
					g_log->Save ( g_log , "Error ipc Accept error, rtn = %d" , rtn ) ;
					continue ;
				}
				accept_fd = ipc_obj->Rtn_acpt_fd ( ipc_obj ) ;
				g_log->Save ( g_log , "Success accept, fd = %d" , accept_fd ) ;
				// 將新連線的fd加到SAcpt結構中
				int j = 0 ;
				for ( j = 0 ; j < MAXEVENTS ; j ++ ) {
					if ( client_acpt [ j ].i_client_fd == 0 ) {
						client_acpt [ j ].i_client_fd = accept_fd ;
						rtn = epl_obj->Add_EpollCtl ( epl_obj , accept_fd , EPOLLIN ) ; // 將新的client加入EPOLLIN型態的監控
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
						rtn = ipc_obj->Read ( ipc_obj , & client_acpt [ j ].i_mlc_data , & client_acpt [ j ].i_mlc_data_sz ) ;
						if ( D_success != rtn ) {
							g_log->Save ( g_log , "Error read ipc client error, rtn = %d" , rtn ) ;
							continue ;
						}
						g_log->Save ( g_log , "Client[%d] input all data = [%s], sz = %d" , client_acpt [ j ].i_client_fd , client_acpt [ j ].i_mlc_data , client_acpt [ j ].i_mlc_data_sz ) ;
						gettimeofday ( & ee , NULL ) ;
						long double time_use = ( ( ee.tv_sec * 1000000 ) + ee.tv_usec ) - ( ( ss.tv_sec * 1000000 ) + ss.tv_usec ) ;
						g_log->Save ( g_log , "#1. ipc Read Time use : %Lf秒" , ( time_use / 1000000 ) ) ;
						time_use = 0 ;

						// Decode and Parser IPC Input Data //
						gettimeofday ( & ss , NULL ) ;

						ipc_in_list = Decode_and_rtn_list ( client_acpt [ j ].i_mlc_data ) ;
						if ( NULL == ipc_in_list ) {
							g_log->Save ( g_log , "Error Decode_and_rtn_list error" ) ;
							continue ;
						}
						S_list *in_list = ( S_list * ) ipc_in_list ;
						int list_num = Rtn_Linked_list_node_ct ( in_list ) ;
						g_log->Save ( g_log , "Decode_and_rtn_list success, list node = %d" , list_num ) ;
						gettimeofday ( & ee , NULL ) ;
						time_use = ( ( ee.tv_sec * 1000000 ) + ee.tv_usec ) - ( ( ss.tv_sec * 1000000 ) + ss.tv_usec ) ;
						g_log->Save ( g_log , "#2. Decode Time use : %Lf秒" , ( time_use / 1000000 ) ) ;
						time_use = 0 ;

						// 執行API Process & 回傳 Response //
						gettimeofday ( & ss , NULL ) ;
						S_Api_process ( ipc_in_list , ipc_obj ) ;
						gettimeofday ( & ee , NULL ) ;
						time_use = ( ( ee.tv_sec * 1000000 ) + ee.tv_usec ) - ( ( ss.tv_sec * 1000000 ) + ss.tv_usec ) ;
						g_log->Save ( g_log , "#3. Api_Process Time use : %Lf秒" , ( time_use / 1000000 ) ) ;
						time_use = 0 ;

						Delete_ipc_list ( & ipc_in_list ) ;	// 刪除linked list //
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

	S_Ipc_Server_Delete ( & ipc_obj ) ;
	S_Epoll_Delete ( & epl_obj ) ;
#endif
	return D_success ;
}
