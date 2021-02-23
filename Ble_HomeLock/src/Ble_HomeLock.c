/*
 * Ble_HomeLock.c
 *
 *  Created on: 2018年3月8日
 *      Author: richard
 */

#include "Ble_HomeLock.h"

// ********** 僅供內部使用 ********** //
// <<< About Client Information >>> //
typedef struct _S_client_info {
	int i_client_fd ;		// client accept fd
	Str i_mlc_data ;
	int i_mlc_data_sz ;
} S_client_info ;

S_Gpio *Gpio_Light , *Gpio_Air_conditioner , *Gpio_Air_purifier , *Gpio_Sweeping_robot , *Gpio_Dehumidifier ;

// 執行Api process:裡面包含回寫ipc client的response //
static void S_Api_process ( Ble_in_list *input_list , S_Ble_Server *ble_obj ) ;

static int S_Gpio_init ( int gpio_light , int gpio_air_conditioner , int gpio_air_purifier , int gpio_sweeping_robot , int gpio_dehumidifier ) ;
static void S_Gpio_delete ( void ) ;

static ssc *S_Self_test ( void ) ;
static ssc *S_Light ( int action ) ;			// 1 >> open , 0 >> close //
static ssc *S_Air_conditioner ( int action ) ;	// 1 >> open , 0 >> close //
static ssc *S_Air_purifier ( int action ) ;		// 1 >> open , 0 >> close //
static ssc *S_Sweeping_robot ( int action ) ;	// 1 >> open , 0 >> close //
static ssc *S_Dehumidifier ( int action ) ;	// 1 >> open , 0 >> close //

/*
 * 電燈：light					<-----> light
 * 冷氣：air conditioner			<-----> air_conditioning
 * 空氣清淨機：air purifier		<-----> air_cleaner
 * 掃地機器人：sweeping robot		<-----> sweeping_robot
 * 除濕機：dehumidifier			<-----> dehumidifier
 *
 */
static int S_Gpio_init ( int gpio_light , int gpio_air_conditioner , int gpio_air_purifier , int gpio_sweeping_robot , int gpio_dehumidifier ) {
	Gpio_Light = S_Gpio_new ( ) ;
	if ( NULL == Gpio_Light ) {
		return - 1 ;
	}
	Gpio_Light->ForcePinOut ( Gpio_Light , gpio_light ) ;
	Gpio_Light->Write ( Gpio_Light , D_low ) ;

	Gpio_Air_conditioner = S_Gpio_new ( ) ;
	if ( NULL == Gpio_Air_conditioner ) {
		S_Gpio_delete ( ) ;
		return - 2 ;
	}
	Gpio_Air_conditioner->ForcePinOut ( Gpio_Air_conditioner , gpio_air_conditioner ) ;
	Gpio_Air_conditioner->Write ( Gpio_Air_conditioner , D_low ) ;

	Gpio_Air_purifier = S_Gpio_new ( ) ;
	if ( NULL == Gpio_Air_purifier ) {
		S_Gpio_delete ( ) ;
		return - 3 ;
	}
	Gpio_Air_purifier->ForcePinOut ( Gpio_Air_purifier , gpio_air_purifier ) ;
	Gpio_Air_purifier->Write ( Gpio_Air_purifier , D_low ) ;

	Gpio_Sweeping_robot = S_Gpio_new ( ) ;
	if ( NULL == Gpio_Sweeping_robot ) {
		S_Gpio_delete ( ) ;
		return - 4 ;
	}
	Gpio_Sweeping_robot->ForcePinOut ( Gpio_Sweeping_robot , gpio_sweeping_robot ) ;
	Gpio_Sweeping_robot->Write ( Gpio_Sweeping_robot , D_low ) ;

	Gpio_Dehumidifier = S_Gpio_new ( ) ;
	if ( NULL == Gpio_Dehumidifier ) {
		S_Gpio_delete ( ) ;
		return - 5 ;
	}
	Gpio_Dehumidifier->ForcePinOut ( Gpio_Dehumidifier , gpio_dehumidifier ) ;
	Gpio_Dehumidifier->Write ( Gpio_Dehumidifier , D_low ) ;

	return D_success ;
}

static void S_Gpio_delete ( void ) {
	if ( NULL != Gpio_Light ) {
		S_Gpio_Delete ( & Gpio_Light ) ;
		Gpio_Light = NULL ;
	}
	if ( NULL != Gpio_Air_conditioner ) {
		S_Gpio_Delete ( & Gpio_Air_conditioner ) ;
		Gpio_Air_conditioner = NULL ;
	}
	if ( NULL != Gpio_Air_purifier ) {
		S_Gpio_Delete ( & Gpio_Air_purifier ) ;
		Gpio_Air_purifier = NULL ;
	}
	if ( NULL != Gpio_Sweeping_robot ) {
		S_Gpio_Delete ( & Gpio_Sweeping_robot ) ;
		Gpio_Sweeping_robot = NULL ;
	}
	if ( NULL != Gpio_Dehumidifier ) {
		S_Gpio_Delete ( & Gpio_Dehumidifier ) ;
		Gpio_Dehumidifier = NULL ;
	}
	return ;
}

static ssc *S_Self_test ( void ) {
	ssc *ret_sc = ( ssc* ) calloc ( 1 , sizeof(ssc) ) ;
	if ( ret_sc == NULL ) {
		return NULL ;
	}
	// Light
	int rtn = Gpio_Light->Write ( Gpio_Light , D_hight ) ;
	if ( D_success != rtn ) {
		ret_sc->ret_code = - 1 ;
		return ret_sc ;
	}
	sleep ( 1 ) ;

	// Air Conditioner
	rtn = Gpio_Air_conditioner->Write ( Gpio_Air_conditioner , D_hight ) ;
	if ( D_success != rtn ) {
		ret_sc->ret_code = - 2 ;
		return ret_sc ;
	}
	sleep ( 1 ) ;

	// Air Purifier
	rtn = Gpio_Air_purifier->Write ( Gpio_Air_purifier , D_hight ) ;
	if ( D_success != rtn ) {
		ret_sc->ret_code = - 3 ;
		return ret_sc ;
	}
	sleep ( 1 ) ;

	// Sweeping Robot
	rtn = Gpio_Sweeping_robot->Write ( Gpio_Sweeping_robot , D_hight ) ;
	if ( D_success != rtn ) {
		ret_sc->ret_code = - 4 ;
		return ret_sc ;
	}
	sleep ( 1 ) ;

	// Dehumidifier
	rtn = Gpio_Dehumidifier->Write ( Gpio_Dehumidifier , D_hight ) ;
	if ( D_success != rtn ) {
		ret_sc->ret_code = - 5 ;
		return ret_sc ;
	}
	sleep ( 1 ) ;

	// All Close
	rtn = Gpio_Light->Write ( Gpio_Light , D_low ) ;
	if ( D_success != rtn ) {
		ret_sc->ret_code = - 6 ;
		return ret_sc ;
	}
	sleep ( 1 ) ;

	rtn = Gpio_Air_conditioner->Write ( Gpio_Air_conditioner , D_low ) ;
	if ( D_success != rtn ) {
		ret_sc->ret_code = - 7 ;
		return ret_sc ;
	}
	sleep ( 1 ) ;

	rtn = Gpio_Air_purifier->Write ( Gpio_Air_purifier , D_low ) ;
	if ( D_success != rtn ) {
		ret_sc->ret_code = - 8 ;
		return ret_sc ;
	}
	sleep ( 1 ) ;

	rtn = Gpio_Sweeping_robot->Write ( Gpio_Sweeping_robot , D_low ) ;
	if ( D_success != rtn ) {
		ret_sc->ret_code = - 9 ;
		return ret_sc ;
	}
	sleep ( 1 ) ;

	rtn = Gpio_Dehumidifier->Write ( Gpio_Dehumidifier , D_low ) ;
	if ( D_success != rtn ) {
		ret_sc->ret_code = - 10 ;
		return ret_sc ;
	}

	ret_sc->ret_code = rtn ;
	return ret_sc ;
}
static ssc *S_Light ( int action ) {
	ssc *ret_sc = ( ssc* ) calloc ( 1 , sizeof(ssc) ) ;
	if ( ret_sc == NULL ) {
		return NULL ;
	}
	int rtn = Gpio_Light->Write ( Gpio_Light , action ) ;
	if ( D_success != rtn ) {
		ret_sc->ret_code = - 1 ;
		return ret_sc ;
	}
	ret_sc->ret_code = rtn ;
	return ret_sc ;
}
static ssc *S_Air_conditioner ( int action ) {
	ssc *ret_sc = ( ssc* ) calloc ( 1 , sizeof(ssc) ) ;
	if ( ret_sc == NULL ) {
		return NULL ;
	}
	int rtn = Gpio_Air_conditioner->Write ( Gpio_Air_conditioner , action ) ;
	if ( D_success != rtn ) {
		ret_sc->ret_code = - 1 ;
		return ret_sc ;
	}
	ret_sc->ret_code = rtn ;
	return ret_sc ;
}
static ssc *S_Air_purifier ( int action ) {
	ssc *ret_sc = ( ssc* ) calloc ( 1 , sizeof(ssc) ) ;
	if ( ret_sc == NULL ) {
		return NULL ;
	}
	int rtn = Gpio_Air_purifier->Write ( Gpio_Air_purifier , action ) ;
	if ( D_success != rtn ) {
		ret_sc->ret_code = - 1 ;
		return ret_sc ;
	}
	ret_sc->ret_code = rtn ;
	return ret_sc ;
}
static ssc *S_Sweeping_robot ( int action ) {
	ssc *ret_sc = ( ssc* ) calloc ( 1 , sizeof(ssc) ) ;
	if ( ret_sc == NULL ) {
		return NULL ;
	}
	int rtn = Gpio_Sweeping_robot->Write ( Gpio_Sweeping_robot , action ) ;
	if ( D_success != rtn ) {
		ret_sc->ret_code = - 1 ;
		return ret_sc ;
	}
	ret_sc->ret_code = rtn ;
	return ret_sc ;
}

static ssc *S_Dehumidifier ( int action ) {
	ssc *ret_sc = ( ssc* ) calloc ( 1 , sizeof(ssc) ) ;
	if ( ret_sc == NULL ) {
		return NULL ;
	}
	int rtn = Gpio_Dehumidifier->Write ( Gpio_Dehumidifier , action ) ;
	if ( D_success != rtn ) {
		ret_sc->ret_code = - 1 ;
		return ret_sc ;
	}
	ret_sc->ret_code = rtn ;
	return ret_sc ;
}


// ********** 僅供內部使用 ********** //

static void S_Api_process ( Ble_in_list *input_list , S_Ble_Server *ble_obj ) {
	S_list *in_list = ( S_list * ) input_list ;
	int list_num = Rtn_Linked_list_node_ct ( in_list ) ;
	int ct = 0 , list_ct = 0 ;
	Ret_ssc *result = NULL ;
	for ( ct = 0 ; ct < list_num ; ct ++ ) {
		++ list_ct ;
		switch ( in_list->type ) {
			case E_self_test : {
				g_log->Save ( g_log , "Will run E_self_test" ) ;
				ssc *ret_ssc = NULL ;
				ret_ssc = S_Self_test ( ) ;
				if ( ret_ssc != NULL ) {
					result = ( Ret_ssc * ) ret_ssc ;
				}
			}
				break ;
			case E_light : {
				g_log->Save ( g_log , "Will run E_light" ) ;
				ssc *ret_ssc = NULL ;
				in_request *request = in_list->p_data ;
				ret_ssc = S_Light ( request->i_action ) ;
				if ( ret_ssc != NULL ) {
					result = ( Ret_ssc * ) ret_ssc ;
				}
			}
				break ;
			case E_air_conditioner : {
				g_log->Save ( g_log , "Will run E_air_conditioner" ) ;
				ssc *ret_ssc = NULL ;
				in_request *request = in_list->p_data ;
				ret_ssc = S_Air_conditioner ( request->i_action ) ;
				if ( ret_ssc != NULL ) {
					result = ( Ret_ssc * ) ret_ssc ;
				}
			}
				break ;
			case E_air_purifier : {
				g_log->Save ( g_log , "Will run E_air_purifier" ) ;
				ssc *ret_ssc = NULL ;
				in_request *request = in_list->p_data ;
				ret_ssc = S_Air_purifier ( request->i_action ) ;
				if ( ret_ssc != NULL ) {
					result = ( Ret_ssc * ) ret_ssc ;
				}
			}
				break ;
			case E_sweeping_robot : {
				g_log->Save ( g_log , "Will run E_sweeping_robot" ) ;
				ssc *ret_ssc = NULL ;
				in_request *request = in_list->p_data ;
				ret_ssc = S_Sweeping_robot ( request->i_action ) ;
				if ( ret_ssc != NULL ) {
					result = ( Ret_ssc * ) ret_ssc ;
				}
			}
				break;
			case E_de_humidifier : {
				g_log->Save ( g_log , "Will run E_de_humidifier" ) ;
				ssc *ret_ssc = NULL ;
				in_request *request = in_list->p_data ;
				ret_ssc = S_Dehumidifier ( request->i_action ) ;
				if ( ret_ssc != NULL ) {
					result = ( Ret_ssc * ) ret_ssc ;
				}
			}
				break;
			default : {
				g_log->Save ( g_log , "Not this api" ) ;
			}
				break ;
		} // end of switch

#if 0
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

		g_log->Save ( g_log , "Will write to client is respon = [%s]" , respon ) ;
		// Send respon
		int rtn = ble_obj->Write ( ble_obj , respon ) ;
		if ( D_success != rtn ) {
			g_log->Save ( g_log , "Error write ble client error, rtn = %d" , rtn ) ;
		}

		free ( mlc_rspn ) ;
		mlc_rspn = NULL ;
		free ( respon ) ;
		respon = NULL ;
#endif

		ssc *ret_ssc = ( ssc* ) result ;
		free ( ret_ssc ) ;
		ret_ssc = NULL ;
		in_list = in_list->next ;
	} // end of for loop
	return ;
}





Ble_in_list *ble_in_list = NULL ;
int Pgm_Begin ( void ) {

	// Ble Server //
	S_Ble_Server *ble_obj = S_Ble_Server_new ( ) ;
	if (  NULL == ble_obj ) {
		g_log->Save ( g_log , "Error S_Ble_Server_new error" ) ;
		return - 1 ;
	}

	int rtn = ble_obj->Server_begin ( ble_obj , MAXEVENTS ) ;
	if ( D_success != rtn ) {
		g_log->Save ( g_log , "Error create ble server error, rtn = %d" , rtn ) ;
		S_Ble_Server_Delete ( & ble_obj ) ;
		return - 2 ;
	}

	rtn = ble_obj->Get_local_address ( ble_obj ) ;
	if (D_success != rtn ) {
		g_log->Save ( g_log , "Error Get_local_address error, rtn = %d" , rtn ) ;
		S_Ble_Server_Delete ( & ble_obj ) ;
		return - 3 ;
	}

	Str mac_addr = ble_obj->Rtn_local_address ( ble_obj ) ;

	int device_id = ble_obj->Rtn_device_id ( ble_obj ) ;
	int device_handle = ble_obj->Rtn_device_handle ( ble_obj ) ;
	int srv_fd = ble_obj->Rtn_srv_fd ( ble_obj ) ;
	g_log->Save ( g_log , "Success create ble server, device_id = %d, device_handle = %d, srv_fd = %d, local address = %s" , device_id , device_handle , srv_fd , mac_addr ) ;


	// Epoll //
	S_Epoll *epl_obj = S_Epoll_New ( ) ;
	if ( NULL == epl_obj ) {
		g_log->Save ( g_log , "Error S_Epoll_New error" ) ;
		S_Ble_Server_Delete ( & ble_obj ) ;
		return - 5 ;
	}
	epl_obj->Setup_wait_num ( epl_obj , MAXEVENTS ) ;
	epl_obj->Setup_wait_time_ms ( epl_obj , - 1 ) ;
	rtn = epl_obj->Create ( epl_obj ) ;
	if ( D_success != rtn ) {
		g_log->Save ( g_log , "Error epoll Create error, rtn = %d" , rtn ) ;
		S_Ble_Server_Delete ( & ble_obj ) ;
		S_Epoll_Delete ( & epl_obj ) ;
		return - 6 ;
	}
	int epoll_fd = epl_obj->Rtn_epl_fd ( epl_obj ) ;
	g_log->Save ( g_log , "Success create epoll, i_epl_fd = %d" , epoll_fd ) ;

	rtn = epl_obj->Add_EpollCtl ( epl_obj , srv_fd , EPOLLIN ) ;
	if ( D_success != rtn ) {
		g_log->Save ( g_log , "Error epoll addctl error, rtn = %d" , rtn ) ;
		S_Ble_Server_Delete ( & ble_obj ) ;
		S_Epoll_Delete ( & epl_obj ) ;
		return - 7 ;
	}
	g_log->Save ( g_log , "Success epoll addctl, add fd = %d" , srv_fd ) ;



	// 2018/03/27 新增初始化GPIO
	rtn = S_Gpio_init ( 22 , 23 , 24 , 25 , 27 ) ;
	if ( D_success != rtn ) {
		g_log->Save ( g_log , "Error gpio initial error, rtn = %d" , rtn ) ;
		S_Ble_Server_Delete ( & ble_obj ) ;
		S_Epoll_Delete ( & epl_obj ) ;
		return - 8 ;
	}

	S_client_info client_acpt [ MAXEVENTS ] = { { 0 } } ;
	int accept_fd = 0 ;
	struct epoll_event evnts [ epl_obj->Rtn_epl_n ( epl_obj ) ] ;
	memset ( & evnts , 0 , sizeof(struct epoll_event) ) ;
	int epll_wat_err_ct = 0 , i = 0 ;
	while ( 1 ) {
		errno = 0 ;
		if ( epll_wat_err_ct >= 20 ) {	// 異常太多
			g_log->Save ( g_log , "Error epoll_wait most error occurs and will close, epll_wat_err_ct = %d" , epll_wat_err_ct ) ;
			S_Ble_Server_Delete ( & ble_obj ) ;
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

				if ( trigger_fd == srv_fd ) {	// server crash block //
					g_log->Save ( g_log , "Get event EPOLLHUP ipc server" ) ;
					ble_obj->Close ( ble_obj ) ;
					while ( 1 ) {	// 重啟IPC Server
						sleep ( 3 ) ;
						rtn = ble_obj->Server_begin ( ble_obj , MAXEVENTS ) ;
						if ( D_success != rtn ) {
							g_log->Save ( g_log , "Error ble re-Server_begin error, rtn = %d" , rtn ) ;
							continue ;
						}
						srv_fd = ble_obj->Rtn_srv_fd ( ble_obj ) ;
						rtn = epl_obj->Add_EpollCtl ( epl_obj , srv_fd , EPOLLIN ) ;
						if ( D_success != rtn ) {
							g_log->Save ( g_log , "Error re-epoll add ble srv fd error, rtn = %d" , rtn ) ;
							continue ;
						}
						g_log->Save ( g_log , "Success re-create ,ble server, fd = %d" , srv_fd ) ;
						break ;
					}
				} else {	// ipc client
					int j = 0 ;
					for ( j = 0 ; j < MAXEVENTS ; j ++ ) {	// 掃描斷線所佔用的fd，並且將其歸零
						g_log->Save ( g_log , "Get event EPOLLHUP ,ble client fd = %d" , client_acpt [ j ].i_client_fd ) ;
						if ( trigger_fd == client_acpt [ j ].i_client_fd ) {
							client_acpt [ j ].i_client_fd = 0 ;
							break ;
						}
					}
					close ( trigger_fd ) ;
				}
			} else if ( trigger_fd == srv_fd ) {	// 2. 新的Client連線
				g_log->Save ( g_log , "Get event EPOLLIN ble server , new client connection" ) ;

				rtn = ble_obj->Accept ( ble_obj ) ;
				if ( D_success != rtn ) {
					g_log->Save ( g_log , "Error ipc Accept error, rtn = %d" , rtn ) ;
					continue ;
				}
				accept_fd = ble_obj->Rtn_acpt_fd ( ble_obj ) ;
				g_log->Save ( g_log , "Success accept, fd = %d" , accept_fd ) ;
				// 將新連線的fd加到SAcpt結構中
				int j = 0 ;
				for ( j = 0 ; j < MAXEVENTS ; j ++ ) {
					if ( client_acpt [ j ].i_client_fd == 0 ) {
						client_acpt [ j ].i_client_fd = accept_fd ;
						Str connection_address = ble_obj->Rtn_connection_address ( ble_obj ) ;
						g_log->Save ( g_log , "New Client Connection, fd = %d, remote address = %s" , client_acpt [ j ].i_client_fd , connection_address ) ;	// 成功將新連線的fd加入監控
						break ;
					}
				}
				rtn = epl_obj->Add_EpollCtl ( epl_obj , accept_fd , EPOLLIN ) ; // 將新的client加入EPOLLIN型態的監控
				if ( D_success != rtn ) {
					close ( accept_fd ) ;
					g_log->Save ( g_log , "Error ble srv accept fd = %d , add epoll EPOLLIN error , rtn = %d" , accept_fd , rtn ) ;
					continue ;
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
//						g_log->Save ( g_log , ">>>>>>>>>>>>>> Will Read <<<<<<<<<<<<<<<<<" ) ;
						rtn = ble_obj->Read ( ble_obj , & client_acpt [ j ].i_mlc_data , & client_acpt [ j ].i_mlc_data_sz ) ;
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
						ble_in_list = Decode_and_rtn_list ( client_acpt [ j ].i_mlc_data ) ;
						if ( NULL == ble_in_list ) {
							g_log->Save ( g_log , "Error Decode_and_rtn_list error" ) ;
							continue ;
						}
						S_list *in_list = ( S_list * ) ble_in_list ;
						int list_num = Rtn_Linked_list_node_ct ( in_list ) ;
						g_log->Save ( g_log , "Decode_and_rtn_list success, list node = %d" , list_num ) ;
						gettimeofday ( & ee , NULL ) ;
						time_use = ( ( ee.tv_sec * 1000000 ) + ee.tv_usec ) - ( ( ss.tv_sec * 1000000 ) + ss.tv_usec ) ;
						g_log->Save ( g_log , "#2. Decode Time use : %Lf秒" , ( time_use / 1000000 ) ) ;
						time_use = 0 ;

						// 執行API Process & 回傳 Response //
						gettimeofday ( & ss , NULL ) ;
						S_Api_process ( ble_in_list , ble_obj ) ;
						gettimeofday ( & ee , NULL ) ;
						time_use = ( ( ee.tv_sec * 1000000 ) + ee.tv_usec ) - ( ( ss.tv_sec * 1000000 ) + ss.tv_usec ) ;
						g_log->Save ( g_log , "#3. Api_Process Time use : %Lf秒" , ( time_use / 1000000 ) ) ;

						time_use = 0 ;
						Delete_ipc_list ( & ble_in_list ) ;	// 刪除linked list //
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

	S_Ble_Server_Delete ( & ble_obj ) ;
	S_Epoll_Delete ( & epl_obj ) ;

	return D_success ;
}
