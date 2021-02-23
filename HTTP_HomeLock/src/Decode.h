/*
 * Http_Decode.h
 *
 *  Created on: 2018年4月25日
 *      Author: richard
 */

#ifndef SRC_DECODE_H_
#define SRC_DECODE_H_

//#include "/media/richard/richard_hdd/workspace_NCHU/Common_tool_C/src/Setup.h"
#include "../../Common_tool_C/src/Setup.h"

typedef void* Ipc_in_list ;		// pointer to "ipc input "Request"的linked list" //

extern Str Url_encode ( Str str ) ;					// url加密，使用後須free
extern Str Url_decode ( Str str ) ;					// url姐密，使用後須free
extern Str Get_Http_Json_data ( Str http_rspn ) ;	// 取得HTTP Response Json資料，使用後須free

extern int Decode_one_data ( char *source , int *sourceSz , char **oneDataBuf , int *sz ) ;

#endif /* SRC_DECODE_H_ */
