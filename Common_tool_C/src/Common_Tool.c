/*
 * Common_Tool.c
 *
 *  Created on: 2017/3/13
 *      Author: richard
 */
#include "Common_Tool.h"

UStr String_to_hex ( CStr input , int *out_sz ) {
	* out_sz = 0 ;
	if ( ! input ) {
		return NULL ;
	}
	int inputSz = ( int ) strlen ( input ) ;
	if ( ( inputSz & 0x01 ) != 0 ) {    // 長度非偶數
		return NULL ;
	}

	int ct = inputSz >> 1 ;
	UStr o_buf = ( UStr ) calloc ( ct + 1 , sizeof(UChar) ) ;
	if ( NULL == o_buf ) {
		return NULL ;
	}
	int cot = 0 ;
	char str [ 2 + 1 ] = { 0 } ; // 取兩個字元出來的Buffer
	int i = 0 ;
	for ( i = 0 ; i < ct ; i ++ ) {
		memcpy ( str , input + ( i << 1 ) , 2 ) ; // 複製兩個字元到str
		o_buf [ cot ++ ] = strtoul ( str , NULL , 16 ) ;
	}
	* out_sz = cot ;
	return o_buf ;
}



Str Text_to_hex ( CStr input ) {
	if ( ! input ) {
		return NULL ;
	}
	int ct = strlen ( input ) ;
 	char *output = ( char * ) calloc ( ( ct << 1 ) + 1 , sizeof(char) ) ;	// 借兩倍空間
	if ( NULL == output ) {
		return NULL ;
	}
	int i = 0 , sz = 0  ;
	for ( i = 0 ; i < ct ; i ++ ) {
		char tmp [ 2 + 1 ] = { 0 } ;
		snprintf ( tmp , sizeof ( tmp ) , "%02X" , input [ i ] ) ;
		memcpy ( output + sz , tmp , 2 ) ;
		output[ sz + 2 ] = 0 ;
		sz += 2 ;
	}
	return output ;
}


int Check_cup_tag ( CStr data , Str *output_tag ) {
	if ( ! data ) {
		return - 1 ;
	}
	char *data_ptr = ( char* ) data ;
	int check_byte = 0 ;

	char tag_byte_tmp [ 6 + 1 ] = { 0 } ;
	snprintf ( tag_byte_tmp , sizeof ( tag_byte_tmp ) , "%s" , data_ptr ) ;

	char first_byte [ 2 + 1 ] = { 0 } ;
	char second_byte [ 2 + 1 ] = { 0 } ;
	char *ptr = tag_byte_tmp ;
	snprintf ( first_byte , sizeof ( first_byte ) , "%s" , ptr ) ;
	ptr += 2 ;
	int i_first_byte = strtoul ( first_byte , NULL , 16 ) ;
	if ( i_first_byte & 0x1F ) {					// &(0001 1111)
		snprintf ( second_byte , sizeof ( second_byte ) , "%s" , ptr ) ;
		ptr += 2 ;
		int i_second_byte = strtoul ( second_byte , NULL , 16 ) ;
		if ( i_second_byte & 0x80 ) {				// &(1000 0000)
			check_byte = 3 ; // Tag為 3 Bytes
		} else {
			check_byte = 2 ; // Tag為 2 Bytes
		}
	} else {
		check_byte = 1 ;	// Tag為 1 Byte
	}

	* output_tag = NULL ;
	switch ( check_byte ) {
		case 1 :
			* output_tag = ( char* ) calloc ( 2 + 1 , sizeof(char) ) ;
			if ( NULL == * output_tag ) {
				return - 1 ;
			}
			memcpy ( * output_tag , tag_byte_tmp , 2 ) ;
			break ;
		case 2 :
			* output_tag = ( char* ) calloc ( 4 + 1 , sizeof(char) ) ;
			if ( NULL == * output_tag ) {
				return - 1 ;
			}
			memcpy ( * output_tag , tag_byte_tmp , 4 ) ;
			break ;
		case 3 :
			* output_tag = ( char* ) calloc ( 6 + 1 , sizeof(char) ) ;
			if ( NULL == * output_tag ) {
				return - 1 ;
			}
			memcpy ( * output_tag , tag_byte_tmp , 6 ) ;
			break ;
		default :
			check_byte = - 1 ;
			* output_tag = NULL ;
			break ;
	}
	return ( check_byte << 1 ) ;	// output sz
}

#if 0 // 測試
int main() {

	char data[ 6 + 1 ] = {0} ;
	snprintf( data , sizeof(data) , "%s" , "9F33555") ;
	printf("data = %s\n" , data ) ;

	char *tag= NULL ;
	int tt = Check_cup_tag( data , &tag ) ;
	printf(" tt = %d, tag = %s\n" , tt , tag ) ;
	free(tag ) ;
	tag = NULL ;
	return 0 ;
}
#endif

/************************************************************************************/
LInt Hex_msb_to_i ( UStr input , int bytes ) {
	if ( ! input ) {
		return - 1 ;
	}
	char *output_string = ( char* ) calloc ( ( bytes << 1 ) + 1 , sizeof(char) ) ;
	if ( NULL == output_string ) {
		return - 1 ;
	}
	int i = 0 ;
	for ( i = 0 ; i < bytes ; i ++ ) {
		char tmp [ 2 + 1 ] = { 0 } ;
		snprintf ( tmp , sizeof ( tmp ) , "%02X" , input [ i ] ) ;
		strncat ( output_string , tmp , 2 ) ;
	}
	LInt value = strtoul ( output_string , NULL , 16 ) ;
	free ( output_string ) ;

	return value ;
}

LInt Hex_lsb_to_i ( UStr input , int bytes ) {
	if ( ! input ) {
		return - 1 ;
	}
	LInt value = 0 ;
	memcpy ( & value , input , bytes ) ;
	return value ;
}

int Rtn_conditional_expression ( Str cmmd , Str buf , int bufSz ) {
	if ( ! cmmd || ! bufSz ) {
		return - 1 ;
	}
	FILE* fp = popen ( cmmd , "r" ) ;
	if ( NULL == fp ) {
		perror ( "popen error " ) ;
		return - 1 ;
	}
	/* popen r 的關鍵 */
	fread ( buf , 1 , bufSz , fp ) ;
	pclose ( fp ) ;
	return 0 ;
}

int GetBit ( UChar b , int bit ) {
	if ( ! b ) {
		return - 1 ;
	}
	UChar mask = 0x80 ;  		// 初始遮罩是個最高位元為 1 的byte
	mask >>= ( bit - 1 ) ;
	if ( ( b & mask ) != 0 ) { 	// 如果 b 和 mask 做 AND 的結果不為0
		return 1 ;
	}
	return 0 ;
}

int File_lock ( int fd ) {
	int fk = 0 ;
	time_t now_t = time ( NULL ) ;
	while ( 3 > ( time ( NULL ) - now_t ) ) {	//3秒 timeout
		fk = flock ( fd , LOCK_EX | LOCK_NB ) ; // 上鎖
		if ( fk == 0 ) {
			break ;	// 取鎖成功 //
		}
	}
	return fk ;
}

int File_unlock ( int fd ) {
	int fk = flock ( fd , LOCK_UN ) ; // 解鎖
	if ( fk != 0 ) {
		fk = - 1 ;
	}
	return fk ;
}










