/*
 * Decode.h
 *
 *  Created on: 2018年3月6日
 *      Author: richard
 */

#ifndef DECODE_H_
#define DECODE_H_

#include "/media/richard/richard_hdd/workspace_NCHU/Common_tool_C/src/Setup.h"
//#include "/home/richard/Common_tool_C/src/Setup.h"

typedef void* Ble_in_list ;
typedef void* Request ;			// pointer to "API Request" //
typedef void* Respon ;			// pointer to "API Response" //

typedef void* Ret_ssc ;	// pointer to API result //
typedef struct _ssc {
	int ret_code ;
} ssc ;


#define DF_Head      "<S>"
#define DF_Head_Len  strlen(DF_Head)
#define DF_Data_Len  ( 8 )

// 使用API列舉
typedef enum _E_api {
	E_null = 0 ,			// NULL
	E_self_test ,			// 自我檢測
	E_light ,				// 電燈
	E_air_conditioner,		// 冷氣
	E_air_purifier,			// 空氣清淨機
	E_sweeping_robot,		// 掃地機器人
	E_de_humidifier, 		// 除濕機
} E_api ;

typedef struct _request {
	E_api m_e_api ;
	char m_api [ 32 + 1 ] ;
	char m_api_type [ 32 + 1 ] ;
	char m_action [ 32 + 1 ] ;
	int i_action ;
	char m_req_md5 [ 128 + 1 ] ;
} in_request ;
typedef struct _respon {
	E_api m_e_api ;
	char m_api [ 32 + 1 ] ;
	char m_api_type [ 32 + 1 ] ;
	char m_ret_code [ 4 + 1 ] ;
	char m_ret_msg [ 1024 + 1 ] ;
	char m_req_md5 [ 128 + 1 ] ;
} out_respon ;


extern Ble_in_list *Decode_and_rtn_list ( char *input_data ) ;	// 解析並回吐串列
extern int Delete_ipc_list ( Ble_in_list **ipc_in_list ) ;		// 刪除串列


extern Respon *Setup_response( Ret_ssc *ret_ssc , Request *input_req , E_api input_e_api  ) ;
extern Str Make_api_respon ( Respon *input_respon , E_api input_e_api ) ;

#endif /* DECODE_H_ */
