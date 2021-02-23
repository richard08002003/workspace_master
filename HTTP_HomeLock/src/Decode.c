/*
 * Http_Decode.c
 *
 *  Created on: 2018年4月25日
 *      Author: richard
 */

#include "Decode.h"
#define DF_Head      "<S>"
#define DF_Head_Len  strlen(DF_Head)
#define DF_Data_Len  ( 8 )

static char from_hex ( char ch ) ;
static char to_hex ( char code ) ;

/* Converts a hex character to its integer value */
static char from_hex ( char ch ) {
	return isdigit(ch) ? ch - '0' : tolower ( ch ) - 'a' + 10 ;
}

/* Converts an integer value to its hex character*/
static char to_hex ( char code ) {
	static char hex [ ] = "0123456789abcdef" ;
	return hex [ code & 15 ] ;
}

Str Url_encode ( Str str ) {
	char *pstr = str ;
	char *buf = malloc ( strlen ( str ) * 3 + 1 ) , *pbuf = buf ;
	while ( * pstr ) {
		if ( isalnum(*pstr) || * pstr == '-' || * pstr == '_' || * pstr == '.' || * pstr == '~' ) {
			* pbuf ++ = * pstr ;
		} else if ( * pstr == ' ' ) {
			* pbuf ++ = '+' ;
		} else {
			* pbuf ++ = '%' , * pbuf ++ = to_hex ( * pstr >> 4 ) , * pbuf ++ = to_hex ( * pstr & 15 ) ;
		}
		pstr ++ ;
	}
	* pbuf = '\0' ;
	return buf ;
}

Str Url_decode ( Str str ) {
	char *pstr = str , *buf = malloc ( strlen ( str ) + 1 ) , *pbuf = buf ;
	while ( * pstr ) {
		if ( * pstr == '%' ) {
			if ( pstr [ 1 ] && pstr [ 2 ] ) {
				* pbuf ++ = from_hex ( pstr [ 1 ] ) << 4 | from_hex ( pstr [ 2 ] ) ;
				pstr += 2 ;
			}
		} else if ( * pstr == '+' ) {
			* pbuf ++ = ' ' ;
		} else {
			* pbuf ++ = * pstr ;
		}
		pstr ++ ;
	}
	* pbuf = '\0' ;
	return buf ;
}

Str Get_Http_Json_data ( Str http_rspn ) {
	if ( ! http_rspn ) {
		return NULL ;
	}
	char *read_str_ptr = http_rspn ;
	char *ptr = strstr ( read_str_ptr , "HTTP/1.1 200 OK" ) ;
	if ( NULL == ptr ) {
		return NULL ;
	}
	ptr = strstr ( read_str_ptr , "data=" ) ;
	if ( NULL == ptr ) {
		return NULL ;
	}

	char json [ strlen ( ptr ) - strlen ( "data=" ) + 1 ] ;
	memset ( json , 0 , sizeof ( json ) ) ;
	snprintf ( json , sizeof ( json ) , "%s" , ptr + strlen ( "data=" ) ) ;

	char *mlc_json = ( char * ) calloc ( strlen ( json ) + 1 , sizeof(char) ) ;
	if ( NULL == mlc_json ) {
		return NULL ;
	}
	memcpy ( mlc_json , json , strlen ( json ) ) ;
	return mlc_json ;
}

int Decode_one_data ( char *source , int *sourceSz , char **oneDataBuf , int *sz ) {
	if ( * sourceSz < ( DF_Head_Len + DF_Data_Len ) ) { // 資料不足續接
		return - 1 ;
	}
	char* ptrS = strstr ( source , DF_Head ) ; // 抓取資料格式的頭 "<s>"，並且將指標指向 "<S>"
	if ( NULL == ptrS ) {
		* oneDataBuf = NULL ;
		* sz = 0 ;
		return - 2 ;
	}
	ptrS += DF_Head_Len ;	// 移動至DF_Head_Len的位置

	char lenBuf [ DF_Data_Len + 1 ] = { 0 } ; 				//資料的大小
	snprintf ( lenBuf , sizeof ( lenBuf ) , "%s" , ptrS ) ; // 將08X寫到lenBuf之中
	int szTmp = strtoul ( lenBuf , NULL , 16 ) ; 			// strtoul : 將字串轉為unsigned long整數型態
	if ( 0 >= szTmp ) {
		* oneDataBuf = NULL ;
		* sz = 0 ;
		return - 3 ;
	}
	ptrS += DF_Data_Len ;	// 移動到資料開頭

	// 確認資料是否有達到 * sz =  //
	if ( strlen ( ptrS ) < ( size_t ) szTmp ) {
		return - 4 ;	// 資料不足
	}
	* sz = szTmp ; // oneData的資料長度設定
	* oneDataBuf = calloc ( * sz + 1 , 1 ) ; // 借用動態記憶體
	if ( NULL == * oneDataBuf ) {
		* oneDataBuf = NULL ;
		* sz = 0 ;
		return - 5 ;
	}
	memcpy ( * oneDataBuf , ptrS , * sz ) ; // ptrS複製sz個字元到 oneDataBuf
	ptrS += * sz ; 							// ptrS = ptrS + * sz ，移動ptrS指標到下一筆資料的開頭

	// 看資料內是否有另外出現 <S>
	char* checkData = strstr ( * oneDataBuf , DF_Head ) ;
	if ( NULL != checkData ) {
		ptrS = checkData ;	// 有另外出現 <S>
		free ( * oneDataBuf ) ;
		* oneDataBuf = NULL ;
		* sz = - 1 ;
	}
	* sourceSz = strlen ( ptrS ) ;
	memmove ( source , ptrS , * sourceSz + 1 ) ;	// 向後移動
	return D_success ;
}
