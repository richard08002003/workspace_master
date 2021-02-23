/*
 * Decode.h
 *
 *  Created on: 2018年1月30日
 *      Author: richard
 */

#ifndef SRC_DECODE_H_
#define SRC_DECODE_H_

#include "/media/richard/richard_hdd/workspace_NCHU/Common_tool_C/src/Setup.h"
//#include "/home/richard/Common_tool_C/src/Setup.h"

typedef void* Ipc_in_list ;		// pointer to "ipc input "Request"的linked list" //
typedef void* Request ;			// pointer to "API Request" //
typedef void* Respon ;			// pointer to "API Response" //


#define DF_Head      "<S>"
#define DF_Head_Len  strlen(DF_Head)
#define DF_Data_Len  ( 8 )

// 使用API列舉
typedef enum _E_api {
	E_null = 0 ,				// NULL
	E_get_temperature_humitidy ,// Get Temperature and Humidity
	E_get_pm_info,
} E_api ;


typedef void* Ret_ssc ;	// pointer to API result //

typedef struct _temp_humidity_ret_ssc {
	int ret_code ;
	float temperature ;
	float humidity ;
} temp_humidity_ret_ssc ;
typedef struct _pm_ret_ssc {
	int ret_code ;
	int pm1_0 ;	// PM1.0
	int pm2_5 ;	// PM2.5
	int pm10 ;	// PM10
} pm_ret_ssc ;

//********** About Request/Response Structure **********//
// Get Temperature and Humidity
typedef struct _S_Get_Temperature_Humidity_request {
	E_api m_e_api;
	char m_api[32 + 1];
	char m_api_type[32 + 1];
	char m_req_md5[128 + 1];
} S_Get_Temperature_Humidity_request;
typedef struct _S_Get_Temperature_Humidity_respon {
	E_api m_e_api;
	char m_api[32 + 1];
	char m_api_type[32 + 1];
	char m_ret_code[4 + 1];
	char m_ret_msg[1024 + 1];
	char m_temperature[128 + 1];
	char m_humitdity[128 + 1];
	char m_req_md5[128 + 1];
} S_Get_Temperature_Humidity_respon;

// Get Pm Information
typedef struct _S_Get_PM_Info_request {
	E_api m_e_api;
	char m_api[32 + 1];
	char m_api_type[32 + 1];
	char m_req_md5[128 + 1];
} S_Get_PM_Info_request;
typedef struct _S_Get_PM_Info_respon {
	E_api m_e_api;
	char m_api[32 + 1];
	char m_api_type[32 + 1];
	char m_ret_code[4 + 1];
	char m_ret_msg[1024 + 1];
	char m_pm1_0[128 + 1];		// PM1.0
	char m_pm2_5[128 + 1];		// PM2.5
	char m_pm10[128 + 1];		// PM10
	char m_req_md5[128 + 1];
} S_Get_PM_Info_respon;

/*
 * Des : 解析出一筆完整的資料
 *
 * Arg:
 * char* source_data : 原始資料
 *
 * Return :
 * succ : S_list 鍊結串列
 * fail : NULL
 *
 * Note :
 *
 */
extern Ipc_in_list *Decode_and_rtn_list ( char *input_data ) ;

extern int Delete_ipc_list ( Ipc_in_list **ipc_in_list ) ;

/*
 * Des : 設定API Respon結構體
 *
 * Arg:
 * Request *input_req : input Request的結構
 * E_api input_e_api : input api的enum
 *
 * Return :
 * succ : Respon *mlc_respon,
 * fail : NULL
 *
 * Note :
 * 使用完必須free掉
 */
extern Respon *Setup_response( Ret_ssc *ret_ssc , Request *input_req , E_api input_e_api ) ;

/*
 * Des : 輸出API Response
 *
 * Arg:
 * Respon *input_respon : input api的respon結構體
 * E_api input_e_api : input api的enum
 *
 * Return :
 * succ : respon, i.e. <S> + 08X + {JSON data}
 * fail : NULL
 *
 * Note :
 * 使用完必須free掉
 */
extern Str Make_api_respon ( Respon *input_respon , E_api input_e_api ) ;

#endif /* SRC_DECODE_H_ */
