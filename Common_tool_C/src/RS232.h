/*
 * RS232.h
 *
 *  Created on: 2017/6/2
 *      Author: richard
 */

#ifndef RS232_H_
#define RS232_H_

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include "Setup.h"
#include "Common_Tool.h"
#include "Epoll.h"
#include "MyLog.h"

typedef struct _S_RS232 S_RS232 ;
struct _S_RS232 {
	void *i_private ;

	int (*Setup) ( S_RS232 *obj , CStr baudrate , CStr comport , int timeout ) ;	// 設定baudrate,comport,timeout
	int (*Open_device) ( S_RS232 *obj ) ;			// 開啟device
	int (*Rtn_reader_fd) ( S_RS232 *obj ) ;			// 回吐開啟device fd
	int (*Rtn_timeout) ( S_RS232 *obj ) ;			// 回吐設定的timeout

//	int (*Write) ( S_RS232 *obj , CStr info , UChar *send_buf ) ;	// 寫(會加APDU Header&EDC..)
	int (*Write) ( S_RS232 *obj , CStr info ) ;						// 寫(會加APDU Header&EDC..)
	int (*Write_only) ( S_RS232 *obj , CStr send_buf ) ;			// 單純寫入原始資料	2017/12/21

	int (*Read) ( S_RS232 *obj ) ;									// 讀
	char *(*Rtn_read_data) ( S_RS232 *obj ) ;

	int (*Get_info) ( S_RS232 *obj , char *input , char **outputBody , int *sz ) ;
} ;
extern S_RS232 * S_RS232_New ( void ) ;
extern void S_RS232_Delete ( S_RS232** obj ) ;


#endif /* RS232_H_ */
