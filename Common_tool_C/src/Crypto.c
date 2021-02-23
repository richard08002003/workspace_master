/*
 * Crypto.c
 *
 *  Created on: 2017年8月14日
 *      Author: richard
 */

#include "Crypto.h"

typedef struct _S_private_data {
	Str i_sha1_for_file_bin ;
	Str i_encode_B64_bin ;

	Str i_md5_from_file_bin ;
	Str i_md5_bin ;
	Str i_md5_str ;

} S_private_data ;


S_Crypto *S_Crypto_tool_New ( void ) {
	S_Crypto *p_tmp = ( S_Crypto* ) calloc ( 1 , sizeof(S_Crypto) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}

	S_private_data *i_private_data = ( S_private_data* ) calloc ( 1 , sizeof(S_private_data) ) ;
	if ( NULL == i_private_data ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}
	i_private_data->i_sha1_for_file_bin = NULL ;
	i_private_data->i_encode_B64_bin = NULL ;
	i_private_data->i_md5_from_file_bin = NULL ;
	i_private_data->i_md5_bin = NULL ;
	i_private_data->i_md5_str = NULL ;

	p_tmp->i_private = ( void* ) i_private_data ;
//	p_tmp->Initial = S_Initial ;
//	p_tmp->Save = S_Save ;

	return p_tmp ;
}

void S_Crypto_tool_Delete ( S_Crypto **obj ) {

}
