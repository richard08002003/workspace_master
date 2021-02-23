/*
 * Common_Tool.h
 *
 *  Created on: 2017/3/13
 *      Author: richard
 */

#ifndef COMMON_TOOL_H_
#define COMMON_TOOL_H_

/****************
 * 共用之TOOL	*
 ****************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include <unistd.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <sys/mman.h>

#include <termios.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/md5.h>
#include <fcntl.h>
#include <net/if.h>
#include <dirent.h>
#include <errno.h>
#include <netinet/in.h>

#include <sched.h>



//#include <scsi/sg.h> 					/* take care: fetches glibc's /usr/include/scsi/sg.h */
//#include <scsi/scsi_bsg_fc.h>
//#include <scsi/scsi.h>
//#include <scsi/scsi_ioctl.h>
//#include <scsi/scsi_netlink_fc.h>
//#include <scsi/scsi_netlink.h>
//#include <scsi/sg.h>

//#include <linux/fb.h>

typedef const char 					CChar ;
typedef const unsigned char 		CUChar ;
typedef unsigned char 				UChar ;
typedef char * 						Str ;
typedef unsigned char * 			UStr ;
typedef const char * 				CStr ;
typedef const unsigned char * 		CUStr ;

typedef const int 					CInt ;
typedef const unsigned int 			CUInt ;
typedef unsigned int 				UInt ;

typedef const long int 				CLInt ;
typedef long int 					LInt ;

typedef const unsigned long int 	CULInt ;
typedef unsigned long int 			ULInt ;

typedef const short int 			CSInt ;
typedef short int 					SInt ;
typedef const unsigned short int 	CUSInt ;
typedef unsigned short int 			USInt ;
typedef long double 				LDouble ;

#define D_success                  	( 0 )
#define D_fail                     	( -1 )

#define MAXEVENTS					( 256 )

/************************************************************************************/
/*
 * SYNOPSIS :
 * 		#include "Common_Tool.h"
 *		UStr String_to_hex ( CStr input , int *out_sz ) ;
 *
 * DESCRIPTION :
 * 		將字串表示的資料轉成HEX : 將字串表示的Hex轉成Unsigned char，Hex型態
 *
 * PARAMETER :
 * 		 CChar *input : 輸入的資料
 * 		 int *out_sz : 輸出資料的Bytes數
 *
 * RETURN VALUE :
 *		return data , NULL is failed
 * NOTES :
 * 	EX: "32" -> '0x32'
 * 	使用完畢後需要free()
 */
extern UStr String_to_hex ( CStr input , int *out_sz ) ;

/*
 * SYNOPSIS :
 * 		#include "Common_Tool.h"
 *		char *Text_to_hex ( CStr input ) ;
 *
 * DESCRIPTION :
 * 		將
 *
 * PARAMETER :
 * 		 CStr input : 輸入的資料
 *
 * RETURN VALUE :
 *		return data , NULL is failed
 * NOTES :
 * 	EX: '0x32' -> "32"
 * 	使用完畢後需要free()
 */
extern Str Text_to_hex ( CStr input ) ;

/*
 * SYNOPSIS :
 * 		#include "Cup_Tool.h"
 *		int Check_cup_tag ( CStr data , Str *output_tag ) ;
 *
 * DESCRIPTION :
 * 		用來確認中國銀聯的TAG是為幾個Byte的資料，Hex型態，最高為3Bytes
 *
 * PARAMETER :
 * 		 CStr data : 輸入的資料
 * 		 Str *output_tag : 輸出的資料
 *
 * RETURN VALUE :
 *		return Bytes, other is failed
 * NOTES :
 */
extern int Check_cup_tag ( CStr data , Str *output_tag ) ;

/************************************************************************************/
/*
 * SYNOPSIS :
 * 		#include "Common_Tool.h"
 * 		LInt Hex_msb_to_i ( UStr input , int bytes ) ;
 *
 * DESCRIPTION :
 * 		將輸入n Bytes的最高有效位的Hex轉成int
 *
 * PARAMETER :
 * 		UChar *input : 輸入的Hex
 * 		int bytes : Bytes數
 *
 * RETURN VALUE :
 *		返回long int型態的值
 *
 * NOTES :
 */
extern LInt Hex_msb_to_i ( UStr input , int bytes ) ;

/*
 * SYNOPSIS :
 * 		#include "Common_Tool.h"
 * 		LInt Hex_lsb_to_i ( UChar *input , int bytes ) ;
 *
 * DESCRIPTION :
 * 		將輸入n Bytes的最低有效位的Hex轉成int
 *
 * PARAMETER :
 * 		UChar *input : 輸入的Hex
 * 		int bytes : Bytes數
 *
 * RETURN VALUE :
 *		返回long int型態的值
 *
 * NOTES :
 */
extern LInt Hex_lsb_to_i ( UStr input , int bytes ) ;

/************************************************************************************/
/*
 * SYNOPSIS :
 * 		#include "Common_Tool.h"
 * 		int Rtn_conditional_expression ( Str cmmd , Str buf , int bufSz ) ;
 *
 * DESCRIPTION :
 * 		將輸入n Bytes的最低有效位的Hex轉成int
 *
 * PARAMETER :
 * 		Str cmmd :
 * 		Str buf :
 * 		int bufSz :
 *
 * RETURN VALUE :
 *		return 0 is success
 * NOTES :
 */
extern int Rtn_conditional_expression ( Str cmmd , Str buf , int bufSz ) ;
#if 0 // 測試code
int main ( void ) {
	char buf [ 1024 ] = {0} ;
	Rtn_conditional_expression ( "bash ./test.sh" , buf , sizeof ( buf ) ) ;

	printf ( "Buf:%s\n" , buf ) ;
	return 0 ;
}
#endif

/*
 * SYNOPSIS :
 * 		#include "Common_Tool.h"
 * 		int GetBit( UChar b , int bit ) ;
 *
 * DESCRIPTION :
 * 		取得輸入資料(unsigned char)的第n位元數值
 *
 * PARAMETER :
 * 		UChar b : 輸入的資料
 * 		int bit : 位元數
 *
 * RETURN VALUE :
 *		return bit is 0 or 1
 *
 * NOTES :
 */
extern int GetBit( UChar b , int bit ) ;
/************************************************************************************/

#endif	/* COMMON_TOOL_H_ */
