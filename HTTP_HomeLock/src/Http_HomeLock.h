/*
 * Http_HomeLock.h
 *
 *  Created on: 2018年4月3日
 *      Author: richard
 *     	用來轉傳https資料的IPC程式
 */

#ifndef SRC_HTTP_HOMELOCK_H_
#define SRC_HTTP_HOMELOCK_H_

#include "Decode.h"

#define DF_Pgm_Name "Http_HomeLock"

/* 2018/05/02
 * 測試完成：初版
 */
#define Ver "0002"

///* 2018/04/07
// * 初版0
// */
//#define Ver "0001"


typedef struct _S_Http_HomeLock S_Http_HomeLock ;
struct _S_Http_HomeLock {
	void *i_private ;
	int (*Pgm_Begin) ( S_Http_HomeLock *obj , CStr Pgm_name ) ;
} ;
extern S_Http_HomeLock * S_Http_HomeLock_New ( void ) ;
extern void S_Http_HomeLock_Delete ( S_Http_HomeLock** obj ) ;

#if 0	// 測試程式
int main ( ) {
	S_Http_HomeLock *http = S_Http_HomeLock_New ( ) ;
	if ( NULL == http ) {
		return - 1 ;
	}

	int rtn = http->Pgm_Begin ( http , "Http_HomeLock" ) ;
	if ( rtn != D_success ) {
		return - 2 ;
	}
	return 0 ;
}
#endif


#endif /* SRC_HTTP_HOMELOCK_H_ */
