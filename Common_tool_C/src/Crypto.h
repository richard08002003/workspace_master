/*
 * Crypto.h
 *
 *  Created on: 2017年8月14日
 *      Author: richard
 */

#ifndef INC_CRYPTO_H_
#define INC_CRYPTO_H_

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "Common_Tool.h"

typedef struct _S_Crypto S_Crypto ;
struct _S_Crypto {
	void *i_private ;

	int (*Sha1_for_file_bin) ( CStr filePath ) ;
	Str (*Rtn_sha1_for_file_bin) ( void ) ;

	int (*Encode_b64_bin) ( CStr surFilePath , bool forUrl ) ;
	Str (*Rtn_encode_b64_bin) ( void ) ;
	/*
	 * CChar* source     : 要解壓縮的 base 64 資料
	 * CChar* targetPath : 解壓縮後要寫入的檔案絕對路徑
	 * bool forUrl          : 若為 true 則 編碼出來的資料 ('+' 轉換成 '_') ('=' 轉換成 '-')
	 */
	int (*Decode_b64) ( Str surData , CStr targetPath , bool forUrl ) ;


	int (*Md5_from_file_bin) ( CStr surFilePath ) ;
	Str (*Rtn_md5_from_file_bin) ( void ) ;

//	int (*Md5_bin) ( CStrType* surData ) ;
	Str (*Rtn_md5_bin) ( void ) ;

	int (*Md5_str) ( Str surData ) ;
	Str (*Rtn_md5_str) ( void ) ;

};
extern S_Crypto *S_Crypto_tool_New ( void ) ;
extern void S_Crypto_tool_Delete ( S_Crypto **obj ) ;


#endif /* INC_CRYPTO_H_ */
