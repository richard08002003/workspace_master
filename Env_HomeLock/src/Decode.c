/*
 * Decode.c
 *
 *  Created on: 2018年1月30日
 *      Author: richard
 */


#include "Decode.h"
static int S_Decode_one_data ( char *source , int *sourceSz , char **oneDataBuf , int *sz ) ;
static E_api S_Get_enum_api ( char *api ) ;
static Request *S_Parser_api_request ( char* inputData , E_api *output_e_api ) ;

static int S_Decode_one_data(char *source, int *sourceSz, char **oneDataBuf, int *sz) {
	if (*sourceSz < ( DF_Head_Len + DF_Data_Len)) { // strlen(<s>) + 8 = 11
		// 資料不足續接
		return -1;
	}
	char* ptrS = strstr(source, DF_Head); // 抓取資料格式的頭 "<s>"，並且將指標指向 "<S>"
	if (NULL == ptrS) {
		*oneDataBuf = NULL;
		*sz = 0;
		return -2;
	}
	ptrS += DF_Head_Len;	// 移動至DF_Head_Len的位置

	char lenBuf[DF_Data_Len + 1] = { 0 }; 			//資料的大小
	snprintf(lenBuf, sizeof(lenBuf), "%s", ptrS); 	// 將08X寫到lenBuf之中
	int szTmp = strtoul(lenBuf, NULL, 16); 	// strtoul : 將字串轉為unsigned long整數型態
	if (0 >= szTmp) {
		*oneDataBuf = NULL;
		*sz = 0;
		return -3;
	}
	ptrS += DF_Data_Len;	// 移動到資料開頭

	// 確認資料是否有達到 * sz =  //
	if (strlen(ptrS) < szTmp) {
		return -4;	// 資料不足
	}
	*sz = szTmp; // oneData的資料長度設定
	*oneDataBuf = calloc(*sz + 1, 1); // 借用動態記憶體
	if (NULL == *oneDataBuf) {
		*oneDataBuf = NULL;
		*sz = 0;
		return -5;
	}
	memcpy(*oneDataBuf, ptrS, *sz); // ptrS複製sz個字元到 oneDataBuf
	ptrS += *sz; 						// ptrS = ptrS + * sz ，移動ptrS指標到下一筆資料的開頭

	// 看資料內是否有另外出現 <S>
	char* checkData = strstr(*oneDataBuf, DF_Head);
	if (NULL != checkData) {
		ptrS = checkData;	// 有另外出現 <S>
		free(*oneDataBuf);
		*oneDataBuf = NULL;
		*sz = -1;
	}
	*sourceSz = strlen(ptrS);
	memmove(source, ptrS, *sourceSz + 1);	// 向後移動
	return D_success;
}

static E_api S_Get_enum_api ( char *api ) {
	if ( ! api ) {
		return E_null ;
	}
	if ( strcmp ( api , "get_temperature_humitidy" ) == 0 ) {
		return E_get_temperature_humitidy ;
	} else if ( strcmp ( api , "get_pm_info" ) == 0 ) {
		return E_get_pm_info ;
	}
	return E_null ;
}

static Request *S_Parser_api_request ( char* inputData , E_api *output_e_api ) {
	if ( ! inputData ) {
		return NULL ;
	}

	// api
	char api [ 128 + 1 ] = { 0 } ;
	int rtn = Json_value ( inputData , "api" , api , sizeof ( api ) ) ;
	if ( rtn != D_success ) {
		return NULL ;
	}
	* output_e_api = S_Get_enum_api ( api ) ;
	Request *request = NULL ;
	switch ( * output_e_api ) {
		case E_null : {
			return NULL ;
		}
			break ;
		case E_get_temperature_humitidy : {
			S_Get_Temperature_Humidity_request *initQcup_request = ( S_Get_Temperature_Humidity_request * ) calloc ( 1 , sizeof(S_Get_Temperature_Humidity_request) ) ;
			if ( NULL == initQcup_request ) {
				return NULL ;
			}
			initQcup_request->m_e_api = * output_e_api ;
			rtn = Json_value ( inputData , "api" , initQcup_request->m_api , sizeof ( initQcup_request->m_api ) ) ;
			if ( D_success != rtn ) {
				free ( initQcup_request ) ;
				initQcup_request = NULL ;
				return NULL ;
			}
			rtn = Json_value ( inputData , "api_type" , initQcup_request->m_api_type , sizeof ( initQcup_request->m_api_type ) ) ;
			if ( D_success != rtn ) {
				free ( initQcup_request ) ;
				initQcup_request = NULL ;
				return NULL ;
			}
			rtn = Json_value ( inputData , "req_md5" , initQcup_request->m_req_md5 , sizeof ( initQcup_request->m_req_md5 ) ) ;
			if ( D_success != rtn ) {
				free ( initQcup_request ) ;
				initQcup_request = NULL ;
				return NULL ;
			}
			request = ( Request * ) initQcup_request ;
		}
			break ;
		case E_get_pm_info : {
			S_Get_PM_Info_request *signOn_request = ( S_Get_PM_Info_request* ) calloc ( 1 , sizeof(S_Get_PM_Info_request) ) ;
			if ( NULL == signOn_request ) {
				return NULL ;
			}
			signOn_request->m_e_api = * output_e_api ;
			rtn = Json_value ( inputData , "api" , signOn_request->m_api , sizeof ( signOn_request->m_api ) ) ;
			if ( D_success != rtn ) {
				free ( signOn_request ) ;
				signOn_request = NULL ;
				return NULL ;
			}
			rtn = Json_value ( inputData , "api_type" , signOn_request->m_api_type , sizeof ( signOn_request->m_api_type ) ) ;
			if ( D_success != rtn ) {
				free ( signOn_request ) ;
				signOn_request = NULL ;
				return NULL ;
			}
			rtn = Json_value ( inputData , "req_md5" , signOn_request->m_req_md5 , sizeof ( signOn_request->m_req_md5 ) ) ;
			if ( D_success != rtn ) {
				free ( signOn_request ) ;
				signOn_request = NULL ;
				return NULL ;
			}
			request = ( Request * ) signOn_request ;
		}
			break ;
		default :
			return NULL ;
			break ;
	}
	return request ;
}


Ipc_in_list *Decode_and_rtn_list ( char *input_data ) {
	if ( ! input_data ) {
		return NULL ;
	}
	S_list *list_head = NULL ;
	Request *ptr = NULL ;
	int source_data_sz = strlen ( input_data ) ;
	char *one_data_buf = NULL ;
	int one_data_sz = 0 , decode_ct = 0 ;
	while ( source_data_sz > 0 ) {
		int decode_rtn = S_Decode_one_data ( input_data , & source_data_sz , & one_data_buf , & one_data_sz ) ;
		if ( D_success != decode_rtn ) {	// decode error
			g_log->Save ( g_log , "Error S_Decode_one_data() error" ) ;
			return NULL ;
		}
		++ decode_ct ;
		E_api e_api = E_null ;
		ptr = S_Parser_api_request ( one_data_buf , & e_api ) ;
		if ( NULL == ptr ) {	// Parser error
			g_log->Save ( g_log , "Error S_Parser_api_request() error" ) ;
			if ( NULL != one_data_buf ) {
				free ( one_data_buf ) ;
				one_data_buf = NULL ;
			}
			return NULL ;
		}

		// decode and parser success, will add to linked list
		S_list add_node = { 0 } ;
		add_node.ct = decode_ct ;
		add_node.data_sz = 0 ;
		add_node.type = e_api ;
		add_node.p_data = ptr ;
		int rtn = Linked_list_add_node ( & list_head , & add_node ) ;
		if ( D_success != rtn ) {	// List_add_node error
			g_log->Save ( g_log , "Error Linked_list_add_node() error" ) ;
			if ( NULL != one_data_buf ) {
				free ( one_data_buf ) ;
				one_data_buf = NULL ;
			}
			if ( NULL != ptr ) {
				free ( ptr ) ;
				ptr = NULL ;
			}
			return NULL ;
		}
		free ( one_data_buf ) ;
		one_data_buf = NULL ;
	}	// end of while loop
	return ( Ipc_in_list * ) list_head ;
}

int Delete_ipc_list ( Ipc_in_list **ipc_in_list ) {
	S_list *delte_list = ( S_list* ) ( * ipc_in_list ) ;
	S_list *data = ( S_list* ) ( * ipc_in_list ) ;
	int list_num = Rtn_Linked_list_node_ct ( data ) ;
	int ct = 0 ;
	for ( ct = 1 ; ct <= list_num ; ct ++ ) {
		int rtn = Linked_list_delete_node ( & delte_list , ct ) ;
		if ( D_success != rtn ) {	// delete error
			return - 1 ;
		}
		data = data->next ;
	}
	list_num = Rtn_Linked_list_node_ct ( data ) ;
	if ( 0 != list_num ) {	// 清除後節點若非0
		return - 2 ;
	}
	return D_success ;
}

#define	D_succ_ret_code			0x9000
#define	D_fail_ret_code			0x0000
#define	D_fail_ret_msg			"fail"

#define D_api_type_req			"request"
#define D_api_type_rspn			"respon"
#define D_api_ret_msg			"success"
// 2018/01/22 異常時 ret_code byte 數會會有問題 //
Respon *Setup_response (  Ret_ssc *ret_ssc , Request *input_req , E_api input_e_api ) {
	if ( ( NULL == ret_ssc ) || ( ! input_req ) || ( ! input_e_api ) ) {
		return NULL ;
	}
	Respon *mlc_respon = NULL ;
	switch ( input_e_api ) {
		case E_null :{
			mlc_respon = NULL ;
		}
			break ;
		case E_get_temperature_humitidy : {
			S_Get_Temperature_Humidity_request *req = ( S_Get_Temperature_Humidity_request* ) input_req ;
			S_Get_Temperature_Humidity_respon *rspn = ( S_Get_Temperature_Humidity_respon* ) calloc ( 1 , sizeof(S_Get_Temperature_Humidity_respon) ) ;
			if ( rspn == NULL ) {
				return NULL ;
			}

			rspn->m_e_api = req->m_e_api ;
			snprintf ( rspn->m_api , sizeof ( rspn->m_api ) , "%s" , req->m_api ) ;
			snprintf ( rspn->m_api_type , sizeof ( rspn->m_api_type ) , "%s" , D_api_type_rspn ) ;

			temp_humidity_ret_ssc *reslut = (temp_humidity_ret_ssc *)ret_ssc ;
			if ( reslut->ret_code == D_success ) {	// api success
				snprintf ( rspn->m_ret_code , sizeof ( rspn->m_ret_code ) , "%04X" , D_succ_ret_code ) ;
				snprintf ( rspn->m_ret_msg , sizeof ( rspn->m_ret_msg ) , "%s" , D_api_ret_msg ) ;
				snprintf ( rspn->m_temperature , sizeof(rspn->m_temperature) , "%f" , reslut->temperature ) ;
				snprintf ( rspn->m_humitdity , sizeof(rspn->m_humitdity) , "%f" , reslut->humidity ) ;
			} else {	// api fail
				snprintf ( rspn->m_ret_code , sizeof ( rspn->m_ret_code ) , "%04X" , D_fail_ret_code ) ;
			snprintf(rspn->m_ret_msg, sizeof(rspn->m_ret_msg), "%s", D_fail_ret_msg ) ;
			}


			snprintf ( rspn->m_req_md5 , sizeof ( rspn->m_req_md5 ) , "%s" , req->m_req_md5 ) ;
			mlc_respon = ( Respon * ) rspn ;
		}
			break ;
		case E_get_pm_info : {
			S_Get_PM_Info_request *req = ( S_Get_PM_Info_request * ) input_req ;
			S_Get_PM_Info_respon *rspn = ( S_Get_PM_Info_respon * ) calloc ( 1 , sizeof(S_Get_PM_Info_respon) ) ;
			if ( rspn == NULL ) {
				return NULL ;
			}

			rspn->m_e_api = req->m_e_api ;
			snprintf ( rspn->m_api , sizeof ( rspn->m_api ) , "%s" , req->m_api ) ;
			snprintf ( rspn->m_api_type , sizeof ( rspn->m_api_type ) , "%s" , D_api_type_rspn ) ;

			pm_ret_ssc *reslut = (pm_ret_ssc *)ret_ssc ;
			if ( reslut->ret_code == D_success ) {	// api success
				snprintf ( rspn->m_ret_code , sizeof ( rspn->m_ret_code ) , "%04X" , D_succ_ret_code ) ;
				snprintf ( rspn->m_ret_msg , sizeof ( rspn->m_ret_msg ) , "%s" , D_api_ret_msg ) ;
				snprintf( rspn->m_pm1_0 , sizeof(rspn->m_pm1_0) , "%d" , reslut->pm1_0 ) ;
				snprintf ( rspn->m_pm2_5 , sizeof(rspn->m_pm2_5) , "%d" , reslut->pm2_5 ) ;
				snprintf ( rspn->m_pm10 , sizeof(rspn->m_pm10) , "%d" , reslut->pm10 ) ;
			} else {	// api fail
				snprintf ( rspn->m_ret_code , sizeof ( rspn->m_ret_code ) , "%04X" , D_fail_ret_code ) ;
				snprintf ( rspn->m_ret_msg , sizeof ( rspn->m_ret_msg ) , "%s" , D_fail_ret_msg ) ;
			}
			snprintf ( rspn->m_req_md5 , sizeof ( rspn->m_req_md5 ) , "%s" , req->m_req_md5 ) ;
			mlc_respon = ( Respon * ) rspn ;
		}
			break ;
		default :
			mlc_respon = NULL ;
			break ;
	}
	return mlc_respon ;
}

// API Response //
// Respon = Header(<S>) + Len(%08X) + Data(Json)
#define D_format_respon				"<S>%08X%s"
// Get Temperature and Humidity
#define D_temp_humidity_json_respon	"{\"api\":\"%s\",\"api_type\":\"%s\",\"ret_code\":\"%s\",\"ret_msg\":\"%s\",\"temperature\":\"%s\",\"humidity\":\"%s\",\"req_md5\":\"%s\"}"
// Get Pm Information
#define D_air_pm_json_respon		"{\"api\":\"%s\",\"api_type\":\"%s\",\"ret_code\":\"%s\",\"ret_msg\":\"%s\",\"pm1_0\":\"%s\",\"pm2_5\":\"%s\",\"pm10\":\"%s\",\"req_md5\":\"%s\"}"

Str Make_api_respon ( Respon *input_respon , E_api input_e_api ) {
	if ( ( ! input_respon ) || ( ! input_e_api ) ) {
		return NULL ;
	}
	switch ( input_e_api ) {
		case E_null : {
			return NULL ;
		}
			break ;
		case E_get_temperature_humitidy : {
			S_Get_Temperature_Humidity_respon *p_respon = ( S_Get_Temperature_Humidity_respon * ) input_respon ;
			char json_respon [ strlen ( D_temp_humidity_json_respon ) + strlen ( p_respon->m_api ) +
							   strlen ( p_respon->m_api_type ) + strlen ( p_respon->m_ret_code ) +
							   strlen ( p_respon->m_ret_msg ) + strlen ( p_respon->m_temperature ) +
							   strlen ( p_respon->m_humitdity ) + strlen ( p_respon->m_req_md5 ) + 1 ] ;
			memset ( json_respon , 0 , sizeof ( json_respon ) ) ;
			snprintf(json_respon, sizeof(json_respon), D_temp_humidity_json_respon,
				p_respon->m_api, p_respon->m_api_type, p_respon->m_ret_code,
				p_respon->m_ret_msg, p_respon->m_temperature,
				p_respon->m_humitdity, p_respon->m_req_md5);

			char respon [ strlen ( D_format_respon ) + 8 + strlen ( json_respon ) + 1 ] ;
			memset ( respon , 0 , sizeof ( respon ) ) ;
			snprintf ( respon , sizeof ( respon ) , D_format_respon , ( unsigned int ) strlen ( json_respon ) , json_respon ) ;

			char *mlc_respon = ( char * ) calloc ( strlen ( respon ) + 1 , sizeof(char) ) ;
			if ( NULL == mlc_respon ) {
				return NULL ;
			}
			memcpy ( mlc_respon , respon , strlen ( respon ) ) ;
			return mlc_respon ;
		}
			break ;
		case E_get_pm_info : {
			S_Get_PM_Info_respon *p_respon = ( S_Get_PM_Info_respon * ) input_respon ;
			char json_respon [ strlen ( D_air_pm_json_respon ) + strlen ( p_respon->m_api ) +
							   strlen ( p_respon->m_api_type ) + strlen ( p_respon->m_ret_code ) +
							   strlen ( p_respon->m_ret_msg )  + strlen ( p_respon->m_pm1_0 ) +
							   strlen ( p_respon->m_pm2_5 ) + strlen ( p_respon->m_pm10 )+ strlen ( p_respon->m_req_md5 ) + 1 ] ;
			memset ( json_respon , 0 , sizeof ( json_respon ) ) ;
			snprintf ( json_respon , sizeof ( json_respon ) , D_air_pm_json_respon ,
					p_respon->m_api , p_respon->m_api_type , p_respon->m_ret_code , p_respon->m_ret_msg ,
					p_respon->m_pm1_0 , p_respon->m_pm2_5 , p_respon->m_pm10 , p_respon->m_req_md5 ) ;

			char respon [ strlen ( D_format_respon ) + 8 + strlen ( json_respon ) + 1 ] ;
			memset ( respon , 0 , sizeof ( respon ) ) ;
			snprintf ( respon , sizeof ( respon ) , D_format_respon , ( unsigned int ) strlen ( json_respon ) , json_respon ) ;

			char *mlc_respon = ( char * ) calloc ( strlen ( respon ) + 1 , sizeof(char) ) ;
			if ( NULL == mlc_respon ) {
				return NULL ;
			}
			memcpy ( mlc_respon , respon , strlen ( respon ) ) ;
			return mlc_respon ;
		}
			break ;
		default :
			return NULL ;
			break ;
	}
	return NULL ;
}
