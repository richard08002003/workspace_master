/*
 * Client_Test.c
 *
 *  Created on: 2018年3月8日
 *      Author: richard
 */

#include "Client_Test.h"


typedef struct _s_info {
	S_Ble_Client *ble_obj ;
	char *read_buf ;
	int read_sz ;
} info ;

void *Read_Function ( void *data ) {
	info *input_info = data ;
	input_info->ble_obj->Read ( input_info->ble_obj , & input_info->read_buf , & input_info->read_sz ) ;
	printf ( "read_buf = %s, read_sz = %d\n" , input_info->read_buf , input_info->read_sz ) ;
	free ( input_info->read_buf ) ;
	input_info->read_buf = NULL ;
	return NULL ;
}

// Client
int main ( ) {
	printf ( "Hello Client\n" ) ;

	S_Ble_Client *clt_obj = S_Ble_Client_new ( ) ;
	if ( NULL == clt_obj ) {
		printf ( "S_Ble_Client_new error\n" ) ;
		return - 1 ;
	}

	int rtn = clt_obj->Connect ( clt_obj , "B8:27:EB:38:CE:A2" ) ;
	if ( D_success != rtn ) {
		printf ( "Connect error, rtn = %d\n" , rtn ) ;
		return - 2 ;
	}

	int device_id = clt_obj->Rtn_device_id ( clt_obj ) ;
	int device_handle = clt_obj->Rtn_device_id ( clt_obj ) ;
	int fd = clt_obj->Rtn_fd ( clt_obj ) ;
	printf ( "device_id = %d, device_handle = %d, fd = %d\n" , device_id , device_handle , fd ) ;

	clt_obj->Set_read_timeout ( clt_obj , 2 ) ;

	info read_info ;
	read_info.ble_obj = clt_obj ;
//	Read_Function ( & read_info ) ;

	int send_rtn = 0 ;
	while ( 1 ) {
		char enter [ 10 ] = { 0 } ;
		while ( 1 ) {
			printf ( "1: self test\n" ) ;
			printf ( "2: light on\n" ) ;			// 電燈
			printf ( "3: air conditioner on\n" ) ;	// 冷氣
			printf ( "4: air purifier on\n" ) ;		// 空氣清淨機
			printf ( "5: sweeping robot on\n" ) ;	// 掃地機器人
			printf ( "6: light off\n" ) ;			// 電燈
			printf ( "7: air conditioner off\n" ) ;	// 冷氣
			printf ( "8: air purifier off\n" ) ;	// 空氣清淨機
			printf ( "9: sweeping robot off\n" ) ;	// 掃地機器人
			printf ( "10: quit\n :" ) ;
			fgets ( enter , sizeof ( enter ) , stdin ) ;
			char *ptr = strrchr ( enter , '\n' ) ;
			if ( NULL != ptr ) {
				* ptr = 0 ;
			}
			printf(">>[%s]\n" , enter ) ;
			break ;
		}
		int i_enter = atoi ( enter ) ;
		printf ( "i_enter = [%d]\n" , i_enter ) ;
		switch ( i_enter ) {
			case 1 : {
				char reset_request [ ] = "{\"api\":\"self_test\",\"api_type\":\"request\",\"req_md5\":\"88888888888888888888888888888888\"}" ;

				while ( 1 ) {
					send_rtn = clt_obj->Write ( clt_obj , reset_request ) ;
					printf("Write rtn = %d\n" , send_rtn );
					if ( send_rtn > 0 ) {
						break ;
					}
					sleep ( 1 ) ;
				}
			}
				break ;
			case 2 : {
				char reset_request [ ] = "{\"api\":\"light\",\"api_type\":\"request\",\"action\":\"open\",\"req_md5\":\"88888888888888888888888888888888\"}" ;

				while ( 1 ) {
					send_rtn = clt_obj->Write ( clt_obj , reset_request ) ;
					printf("Write rtn = %d\n" , send_rtn );
					if ( send_rtn > 0 ) {
						break ;
					}
					sleep ( 1 ) ;
				}
			}
				break ;
			case 3 : {
				char reset_request [ ] = "{\"api\":\"air_conditioner\",\"api_type\":\"request\",\"action\":\"open\",\"req_md5\":\"88888888888888888888888888888888\"}" ;


				while ( 1 ) {
					send_rtn = clt_obj->Write ( clt_obj , reset_request ) ;
					printf("Write rtn = %d\n" , send_rtn );
					if ( send_rtn > 0 ) {
						break ;
					}
					sleep ( 1 ) ;
				}

			}
				break ;
			case 4 : {
				char reset_request [ ] = "{\"api\":\"air_purifier\",\"api_type\":\"request\",\"action\":\"open\",\"req_md5\":\"88888888888888888888888888888888\"}" ;

				while ( 1 ) {
					send_rtn = clt_obj->Write ( clt_obj , reset_request ) ;
					printf("Write rtn = %d\n" , send_rtn );
					if ( send_rtn > 0 ) {
						break ;
					}
					sleep ( 1 ) ;
				}

			}
				break ;
			case 5 : {
				char reset_request [ ] = "{\"api\":\"sweeping_robot\",\"api_type\":\"request\",\"action\":\"open\",\"req_md5\":\"88888888888888888888888888888888\"}" ;

				while ( 1 ) {
					send_rtn = clt_obj->Write ( clt_obj , reset_request ) ;
					printf("Write rtn = %d\n" , send_rtn );
					if ( send_rtn > 0 ) {
						break ;
					}
					sleep ( 1 ) ;
				}

			}
				break ;
			case 6 : {
				char reset_request [ ] = "{\"api\":\"light\",\"api_type\":\"request\",\"action\":\"close\",\"req_md5\":\"88888888888888888888888888888888\"}" ;

				while ( 1 ) {
					send_rtn = clt_obj->Write ( clt_obj , reset_request ) ;
					printf("Write rtn = %d\n" , send_rtn );
					if ( send_rtn > 0 ) {
						break ;
					}
					sleep ( 1 ) ;
				}
			}
				break ;
			case 7 : {
				char reset_request [ ] = "{\"api\":\"air_conditioner\",\"api_type\":\"request\",\"action\":\"close\",\"req_md5\":\"88888888888888888888888888888888\"}" ;


				while ( 1 ) {
					send_rtn = clt_obj->Write ( clt_obj , reset_request ) ;
					printf("Write rtn = %d\n" , send_rtn );
					if ( send_rtn > 0 ) {
						break ;
					}
					sleep ( 1 ) ;
				}

			}
				break ;
			case 8 : {
				char reset_request [ ] = "{\"api\":\"air_purifier\",\"api_type\":\"request\",\"action\":\"close\",\"req_md5\":\"88888888888888888888888888888888\"}" ;

				while ( 1 ) {
					send_rtn = clt_obj->Write ( clt_obj , reset_request ) ;
					printf("Write rtn = %d\n" , send_rtn );
					if ( send_rtn > 0 ) {
						break ;
					}
					sleep ( 1 ) ;
				}

			}
				break ;
			case 9 : {
				char reset_request [ ] = "{\"api\":\"sweeping_robot\",\"api_type\":\"request\",\"action\":\"close\",\"req_md5\":\"88888888888888888888888888888888\"}" ;

				while ( 1 ) {
					send_rtn = clt_obj->Write ( clt_obj , reset_request ) ;
					printf("Write rtn = %d\n" , send_rtn );
					if ( send_rtn > 0 ) {
						break ;
					}
					sleep ( 1 ) ;
				}

			}
				break ;
			case 10 : {
				return 0 ;
			}
				break ;
			default : {
				system ( "clear" ) ;
			}
				break ;
		}

//		char *read_buf = NULL ;
//		int read_sz = 0 ;
//		clt_obj->Read ( clt_obj , & read_buf , & read_sz ) ;
//
//		printf ( "read_buf = %s, read_sz = %d\n" , read_buf , read_sz ) ;
//		free ( read_buf ) ;
//		read_buf = NULL ;
//
		// Read //
		pthread_t thread1 ;
		pthread_create ( & thread1 , NULL , Read_Function , & read_info ) ;	// "Read_Function" > 多工處理的function

	}
	close ( fd ) ;
	S_Ble_Client_Delete ( & clt_obj ) ;

	return 0 ;
}
