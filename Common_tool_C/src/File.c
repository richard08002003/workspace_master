/*
 * File.c
 *
 *  Created on: 2017/5/9
 *      Author: richard
 */

#include "File.h"

typedef struct _S_private_data {
	LInt i_file_sz ;
	Str mlc_file_str ;	// String
	UStr mlc_file_bin ;	// Binary

	int i_lock_fd ;
} S_private_data ;

static int S_Read_str_from_file ( S_File *obj , CChar *file_path ) ;
static Str S_Rtn_str_from_file ( S_File *obj ) ;
static int S_Read_bin_from_file ( S_File *obj , CChar *file_path ) ;
static UStr S_Rtn_bin_from_file ( S_File *obj ) ;
static LInt S_Rtn_file_sz ( S_File *obj ) ;
static int S_Write_str_to_file_w ( S_File *obj , CChar *file_path , Str data ) ;
static int S_Write_str_to_file_a ( S_File *obj , CChar *file_path , Str  data ) ;
static int S_Write_bin_to_file_w ( S_File *obj , CChar *file_path , void* p_data , int sz ) ;
static int S_Write_bin_to_file_a ( S_File *obj , CChar *file_path , void* p_data , int sz ) ;
static int S_File_lock ( S_File *obj , CStr file_path ) ;
static int S_Rtn_lock_fd ( S_File *obj ) ;
static int S_File_unlock ( S_File *obj , int fd ) ;


static int S_Read_str_from_file ( S_File *obj , CChar *file_path ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! file_path ) ) {
		return - 1 ;
	}
	S_private_data *p_private = obj->i_private ;
	p_private->i_file_sz = 0 ;

	FILE* fp = fopen ( file_path , "r+" ) ;
	if ( NULL == fp ) {
		return - 2 ;
	}
	fseek ( fp , 0 , SEEK_END ) ;
	LInt sz = ftell ( fp ) ;
	if ( 0 >= sz ) {
		fclose ( fp ) ;
		return - 3 ;
	}
	fseek ( fp , 0 , SEEK_SET ) ;

	char *p_buf_tmp = ( char * ) calloc ( sz , 1 ) ;
	if ( NULL == p_buf_tmp ) {
		fclose ( fp ) ;
		return - 4 ;
	}

	fread ( p_buf_tmp , 1 , sz , fp ) ;
	fclose ( fp ) ;

 	p_private->mlc_file_str = ( Str ) calloc ( sz + 1 , sizeof(char) ) ;
	if ( NULL == p_private->mlc_file_str ) {
		return - 5 ;
	}
	memcpy ( p_private->mlc_file_str , p_buf_tmp , sz ) ;
	p_private->i_file_sz = sz ;

	free ( p_buf_tmp ) ;
	p_buf_tmp = NULL ;

	return D_success ;
}
static Str S_Rtn_str_from_file ( S_File *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_private_data *p_private = obj->i_private ;
	return p_private->mlc_file_str ;
}

static int S_Read_bin_from_file ( S_File *obj , CChar *file_path ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! file_path ) ) {
		return - 1 ;
	}
	S_private_data *p_private = obj->i_private ;
	p_private->i_file_sz = 0 ;

	FILE* fp = fopen ( file_path , "r+" ) ;
	if ( NULL == fp ) {
		return - 2 ;
	}
	fseek ( fp , 0 , SEEK_END ) ;
	LInt sz = ftell ( fp ) ;
	if ( 0 >= sz ) {
		fclose ( fp ) ;
		return - 3 ;
	}
	fseek ( fp , 0 , SEEK_SET ) ;

	UChar*p_buf_tmp = ( UChar* ) calloc ( sz , 1 ) ;
	if ( NULL == p_buf_tmp ) {
		fclose ( fp ) ;
		return - 4 ;
	}
	fread ( p_buf_tmp , 1 , sz , fp ) ;
	fclose ( fp ) ;


	p_private->mlc_file_bin = ( UStr ) calloc ( sz , sizeof(UChar) ) ;
	if ( NULL == p_private->mlc_file_bin ) {
		return -5 ;
	}
	memcpy ( p_private->mlc_file_bin , p_buf_tmp , sz ) ;
	p_private->i_file_sz = sz ;

	free ( p_buf_tmp ) ;
	p_buf_tmp = NULL ;

	return D_success ;
}
static UStr S_Rtn_bin_from_file ( S_File *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_private_data *p_private = obj->i_private ;
	return p_private->mlc_file_bin ;
}

static LInt S_Rtn_file_sz ( S_File *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_private_data* p_private = obj->i_private ;
	return p_private->i_file_sz ;
}

static int S_Write_str_to_file_w ( S_File *obj , CChar *file_path , Str data ) {
	if ( ( ! obj ) || ( ! file_path ) || ( ! data ) ) {
		return - 1 ;
	}
	FILE* fp = fopen ( file_path , "w+" ) ;	// 新建文字檔案並讀取、寫入資料
	if ( NULL == fp ) {
		return - 2 ;
	}
	fwrite ( data , 1 , strlen ( data ) , fp ) ;
	fclose ( fp ) ;
	return D_success ;
}

static int S_Write_str_to_file_a ( S_File *obj , CChar *file_path , Str data ) {
	if ( ( ! obj ) || ( ! file_path ) || ( ! data ) ) {
		return - 1 ;
	}
	FILE* fp = fopen ( file_path , "a+" ) ;	// 讀取文字檔案將附加資料在檔案最後
	if ( NULL == fp ) {
		return - 2 ;
	}
	fseek ( fp , 0 , SEEK_END ) ;
	fwrite ( data , 1 , strlen ( data ) , fp ) ;
	fclose ( fp ) ;
	return D_success ;
}

static int S_Write_bin_to_file_w ( S_File *obj , CChar *file_path , void* p_data , int sz ) {
	if ( ( ! obj ) || ( ! file_path ) || ( ! p_data ) || ( ! sz ) ) {
		return - 1 ;
	}
	FILE* fp = fopen ( file_path , "wb+" ) ;
	if ( NULL == fp ) {
		return - 2 ;
	}
	fwrite ( p_data , 1 , sz , fp ) ;
	fclose ( fp ) ;
	return D_success ;
}

static int S_Write_bin_to_file_a ( S_File *obj , CChar *file_path , void* p_data , int sz ) {
	if ( ( ! obj ) || ( ! file_path ) || ( ! p_data ) || ( ! sz ) ) {
		return - 1 ;
	}
	FILE* fp = fopen ( file_path , "ab+" ) ;
	if ( NULL == fp ) {
		return - 1 ;
	}
	fseek ( fp , 0 , SEEK_END ) ;
	fwrite ( p_data , 1 , sz , fp ) ;
	fclose ( fp ) ;
	return D_success ;
}

static int S_File_lock ( S_File *obj , CStr file_path ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! file_path ) ) {
		return - 1 ;
	}
	S_private_data *p_private = obj->i_private ;
//	p_private->i_lock_fd = open ( file_path , O_CREAT | O_RDWR | O_CLOEXEC , 0750 ) ;
//	p_private->i_lock_fd = open ( file_path , O_CREAT | O_RDWR  , 0750 ) ;
	p_private->i_lock_fd = open ( file_path , O_RDWR ) ;
	if ( - 1 == p_private->i_lock_fd ) {
		return - 2 ;
	}

//	if ( - 1 == flock ( p_private->i_lock_fd , LOCK_EX | LOCK_NB ) ) {
//		close ( p_private->i_lock_fd ) ;
//		return - 3 ;
//	}

	time_t now_t = time ( NULL ) ;
	while ( 3 > ( time ( NULL ) - now_t ) ) {	//3秒 timeout
		int fk = flock ( p_private->i_lock_fd , LOCK_EX | LOCK_NB ) ; // 上鎖
		if ( fk == 0 ) {
			return D_success ; // 取鎖成功//
		}
	}
	return - 3 ;
}
static int S_Rtn_lock_fd ( S_File *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_private_data *p_private = obj->i_private ;
	return p_private->i_lock_fd ;
}
static int S_File_unlock ( S_File *obj, int fd ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	if ( fd < 0 ) {
		return - 2 ;
	}
	if ( - 1 == flock ( fd , LOCK_UN ) ) {
		return - 3 ;
	}

	return D_success ;
}

S_File *S_File_tool_New ( void ) {
	S_File *p_tmp = ( S_File* ) calloc ( 1 , sizeof(S_File) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}

	S_private_data *i_private_data = ( S_private_data* ) calloc ( 1 , sizeof(S_private_data) ) ;
	if ( NULL == i_private_data ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}

	p_tmp->i_private = ( void* ) i_private_data ;

	p_tmp->Read_str_from_file = S_Read_str_from_file ;
	p_tmp->Rtn_str_from_file = S_Rtn_str_from_file ;

	p_tmp->Read_bin_from_file = S_Read_bin_from_file ;
	p_tmp->Rtn_bin_from_file = S_Rtn_bin_from_file ;

	p_tmp->Rtn_file_sz = S_Rtn_file_sz ;

	p_tmp->Write_str_to_file_w = S_Write_str_to_file_w ;
	p_tmp->Write_str_to_file_a = S_Write_str_to_file_a ;

	p_tmp->Write_bin_to_file_w = S_Write_bin_to_file_w ;
	p_tmp->Write_bin_to_file_a = S_Write_bin_to_file_a ;

	p_tmp->File_lock = S_File_lock ;
	p_tmp->Rtn_lock_fd = S_Rtn_lock_fd ;
	p_tmp->File_unlock = S_File_unlock ;

	return p_tmp ;
}

void S_File_tool_Delete ( S_File** obj ) {
	S_private_data* p_private = ( * obj )->i_private ;
	free ( p_private->mlc_file_bin ) ;
	p_private->mlc_file_bin = NULL ;
	free ( p_private->mlc_file_str ) ;
	p_private->mlc_file_str = NULL ;
	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;
	free ( * obj ) ;
	* obj = NULL ;
	return ;
}

