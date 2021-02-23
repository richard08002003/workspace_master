/*
 * MyLog.c
 *
 *  Created on: 2015/7/9
 *      Author: richard
 */

#include "MyLog.h"

typedef struct _S_private_data {
	char i_pgm_name [ 256 ] ;			// 程式名稱
	char i_log_path [ 256 ] ;				// log路徑
	char i_config_path [ 256 ] ;		// log設定檔路徑

	int i_log_tar_sz ;
	FILE *i_log_fp ;

	char i_lock_path [ 256 ] ;		// lock file path
} S_private_data ;

// 僅提供內部使用 //
static int S_Setup_pgm_info ( S_Log *obj , CStr pgm_name ) ;
static Str S_Get_config_val ( S_Log *obj , Str key ) ;
static int S_Open_fp ( S_Log *obj ) ;
static int S_Tar_log ( S_Log *obj ) ;
static int S_Get_pgm_lock ( S_Log *obj ) ;
// End of 內部使用 //

static int S_Initial ( S_Log *obj , CStr pgm_name ) ;
static void S_Close ( S_Log *obj ) ;
static Str S_Rtn_Pgm_name ( S_Log *obj ) ;
static int S_Save ( S_Log*obj , CStr format , ... ) ;
static Str S_Rtn_lock_path ( S_Log *obj ) ;


// 僅提供內部使用 //
#define DF_bin_dir		"/usr/local/HomeLock/%s/bin"

#define DF_log_path		"/usr/local/HomeLock/%s/log/%s.log"
#define DF_tar_dir		"/usr/local/HomeLock/%s/log/tar/"
#define Df_Tar_Num 		"/usr/local/HomeLock/%s/log/tar/tar_num"

#define	DF_conf_dir		"/usr/local/HomeLock/%s/config/"
#define DF_config_pah	"/usr/local/HomeLock/%s/config/%s_Log.config"

#define DF_lock_dir		"/usr/local/HomeLock/lock/"
#define DF_lock_path	"/usr/local/HomeLock/lock/%s.lk"

#define DF_ipc_path		"/usr/local/HomeLock/ipc"

static int S_Setup_pgm_info ( S_Log *obj , CStr pgm_name ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! pgm_name ) ) {
		return - 1 ;
	}
	S_private_data *p_private = obj->i_private ;
	memset ( p_private->i_pgm_name , 0 , sizeof ( p_private->i_pgm_name ) ) ;
	snprintf ( p_private->i_pgm_name , sizeof ( p_private->i_pgm_name ) , "%s" , pgm_name ) ;

	memset ( p_private->i_log_path , 0 , sizeof ( p_private->i_log_path ) ) ;
	snprintf ( p_private->i_log_path , sizeof ( p_private->i_log_path ) , DF_log_path , pgm_name , pgm_name ) ;

	memset ( p_private->i_config_path , 0 , sizeof ( p_private->i_config_path ) ) ;
	snprintf ( p_private->i_config_path , sizeof ( p_private->i_config_path ) , DF_config_pah , pgm_name , pgm_name ) ;

	memset ( p_private->i_lock_path , 0 , sizeof ( p_private->i_lock_path ) ) ;
	snprintf ( p_private->i_lock_path , sizeof ( p_private->i_lock_path ) , DF_lock_path , pgm_name  ) ;
	return D_success ;
}

#define DF_Keyname "$%s"
static Str S_Get_config_val ( S_Log *obj , Str key ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! key ) ) {
		return NULL ;
	}
	S_private_data *p_private = obj->i_private ;
	S_File *file_obj = S_File_tool_New ( ) ;
	if ( NULL == file_obj ) {
		return NULL ;
	}

	int rtn = file_obj->Read_str_from_file ( file_obj , p_private->i_config_path ) ;
	if ( D_success != rtn ) {
		S_File_tool_Delete ( & file_obj ) ;
		return NULL ;
	}

	Str config_data = file_obj->Rtn_str_from_file ( file_obj ) ;
	if ( NULL == config_data ) {
		S_File_tool_Delete ( & file_obj ) ;
		return NULL ;
	}

	Str value = NULL ;
	// KeyName
	char keyname [ strlen ( DF_Keyname ) + strlen ( key ) + 1 ] ;
	memset ( keyname , 0 , sizeof ( keyname ) ) ;
	snprintf ( keyname , sizeof ( keyname ) , DF_Keyname , key ) ;

	if ( strstr ( config_data , keyname ) ) {
		Str data_tmp = ( Str ) calloc ( strlen ( config_data ) + 1 , 1 ) ;
		if ( NULL == data_tmp ) {
			return NULL ;
		}
		memcpy ( data_tmp , config_data , strlen ( config_data ) ) ;
		Str data_ptr = data_tmp ;
#define DF_quotation_mark	"\""
		Str once_ptr = strstr ( data_ptr , DF_quotation_mark ) ;
		int once_ptr_sz = strlen ( once_ptr ) ;
		once_ptr += 1 ;
		Str seconde_ptr = strstr ( once_ptr , DF_quotation_mark ) ;
		int second_ptr_sz = strlen ( seconde_ptr ) ;
		int value_sz = once_ptr_sz - second_ptr_sz -1  ;	// 去掉"的長度
		value = ( Str ) calloc ( value_sz + 1 , 1 ) ;
		if ( NULL == value ) {
			return NULL ;
		}

		memcpy ( value , once_ptr , value_sz );
		free ( data_tmp ) ;
		data_tmp = NULL ;
	}
	S_File_tool_Delete ( & file_obj ) ;
	return value ;
}

static int S_Open_fp ( S_Log *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return -1 ;
	}
	S_private_data *p_private = obj->i_private ;
	p_private->i_log_fp = fopen ( p_private->i_log_path , "a+" ) ; //可寫，資料接在原有資料後面
	if ( p_private->i_log_fp == NULL ) {
printf ( "\"%s\"log_fp Error\n" , p_private->i_log_path ) ;
		return - 2 ;
	}
	return D_success ;
}

// 預設tar_num
#define tar_limit		10
static int S_Tar_log ( S_Log *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return -1 ;
	}
	S_private_data *p_private = obj->i_private ;
	fclose ( p_private->i_log_fp ) ;	 // 關閉Log資料

	char Tar_Num [ strlen ( Df_Tar_Num ) + strlen ( p_private->i_pgm_name ) + 1 ] ;
	memset ( Tar_Num , 0 , sizeof ( Tar_Num ) ) ;
	snprintf ( Tar_Num , sizeof ( Tar_Num ) , Df_Tar_Num , p_private->i_pgm_name ) ;

	FILE* tar_fp = fopen ( Tar_Num , "r+" ) ;
	if ( NULL == tar_fp ) {
		return - 2 ;
	}
	char s_tar_num [ 10 ] = { 0 } ;
	fgets ( s_tar_num , sizeof ( s_tar_num ) , tar_fp ) ;
	fclose ( tar_fp ) ;
	char *ptr = strrchr ( s_tar_num , '\n' ) ;
	if ( NULL != ptr ) {
		* ptr = 0 ;
	}

	int n_tar_num = atoi ( s_tar_num ) ;
	char TarName [ 2 + 1 ] = { 0 } ;
	if ( n_tar_num > 10 ) {
		n_tar_num = 0 ;
	}
	snprintf ( TarName , sizeof ( TarName ) , "%d" , n_tar_num ) ;

	char df_command [ ] = "tar -zcvf %s/%s.tar.gz  %s" ; // tar 壓縮指令
	char command [ sizeof ( df_command ) + sizeof ( DF_tar_dir ) + sizeof ( TarName ) + sizeof ( p_private->i_log_path ) + 1 ] ;
	memset ( command , 0 , sizeof ( command ) ) ;
	snprintf ( command , sizeof ( command ) , df_command , DF_tar_dir , TarName , p_private->i_log_path ) ;
	system ( command ) ; // Tar log起來

	n_tar_num ++ ;
	if ( n_tar_num > 10 ) {
		n_tar_num = 0 ;
	}

	tar_fp = fopen ( Tar_Num , "w" ) ;
	if ( NULL == tar_fp ) {
		return - 3 ;
	}
	fprintf ( tar_fp , "%d" , n_tar_num ) ;
	fflush ( tar_fp ) ;
	fclose ( tar_fp ) ;

	// 重新開起Log
	while ( 1 ) {
		unlink ( p_private->i_log_path ) ;
		int rtn = S_Open_fp ( obj ) ;
		if ( D_success == rtn ) {
			break ;
		}
		sleep ( 1 ) ;
	}
	return D_success ;
}

static int S_Get_pgm_lock ( S_Log *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return -1 ;
	}
	S_private_data *p_private = obj->i_private ;
	S_File *file_obj = S_File_tool_New ( ) ;
	if ( NULL == file_obj ) {
		return -2 ;
	}
	int rtn = file_obj->File_lock ( file_obj , p_private->i_lock_path ) ;
	S_File_tool_Delete ( & file_obj ) ;
	if ( D_success != rtn ) {
		fprintf ( stderr , "Error will get pgm:%s file lock , errcode:%d\n" , p_private->i_pgm_name , rtn ) ;
		fflush ( stderr ) ;
		exit ( 1 ) ;
	}
	return D_success ;
}
// End of 內部使用 //

static int S_Initial ( S_Log *obj , CStr pgm_name ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! pgm_name ) ) {
		return - 1 ;
	}
	S_private_data *p_private = obj->i_private ;
	int rtn = S_Setup_pgm_info ( obj , pgm_name ) ;
	if ( D_success != rtn ) {
		return - 2 ;
	}

	// mkdir directory //
	char df_mkdir_cmd [ ] = "mkdir -p %s" ;
	// tar dir
	char tar_path [ strlen ( DF_tar_dir ) + strlen ( pgm_name ) + 1 ] ;
	memset ( tar_path , 0 , sizeof ( tar_path ) ) ;
	snprintf ( tar_path , sizeof ( tar_path ) , DF_tar_dir , pgm_name ) ;
	char mkdir_tar_cmd [ strlen ( df_mkdir_cmd ) + strlen ( tar_path ) + 1 ] ;
	memset ( mkdir_tar_cmd , 0 , sizeof ( mkdir_tar_cmd ) ) ;
	snprintf ( mkdir_tar_cmd , sizeof ( mkdir_tar_cmd ) , df_mkdir_cmd , tar_path ) ;
	system ( mkdir_tar_cmd ) ;

	// config dir
	char config_path [ strlen ( DF_conf_dir ) + strlen ( pgm_name ) + 1 ] ;
	memset ( config_path , 0 , sizeof ( config_path ) ) ;
	snprintf ( config_path , sizeof ( config_path ) , DF_conf_dir , pgm_name ) ;
	char mkdir_conf_cmd [ strlen ( df_mkdir_cmd ) + strlen ( config_path )  + 1 ] ;
	memset ( mkdir_conf_cmd , 0 , sizeof ( mkdir_conf_cmd ) ) ;
	snprintf ( mkdir_conf_cmd , sizeof ( mkdir_conf_cmd ) , df_mkdir_cmd , config_path ) ;
	system ( mkdir_conf_cmd ) ;

	// lock dir
	char mkdir_lock_cmd [ strlen ( df_mkdir_cmd ) + strlen ( DF_lock_dir ) + 1 ] ;
	memset ( mkdir_lock_cmd , 0 , sizeof ( mkdir_lock_cmd ) ) ;
	snprintf ( mkdir_lock_cmd , sizeof ( mkdir_lock_cmd ) , df_mkdir_cmd , DF_lock_dir ) ;
	system ( mkdir_lock_cmd ) ;

	// ipc dir
	char mkdir_ipc_cmd [ strlen ( df_mkdir_cmd ) + strlen ( DF_ipc_path ) + 1 ] ;
	memset ( mkdir_ipc_cmd , 0 , sizeof ( mkdir_ipc_cmd ) ) ;
	snprintf ( mkdir_ipc_cmd , sizeof ( mkdir_ipc_cmd ) , df_mkdir_cmd , DF_ipc_path ) ;
	system ( mkdir_ipc_cmd ) ;

	// bin
	char bin_path [ strlen ( DF_bin_dir ) + strlen ( pgm_name ) + 1 ] ;
	memset ( bin_path , 0 , sizeof ( bin_path ) ) ;
	snprintf ( bin_path , sizeof ( bin_path ) , DF_bin_dir , pgm_name ) ;
	char mkdir_bin_cmd [ strlen ( df_mkdir_cmd ) + strlen ( bin_path ) + 1 ] ;
	memset ( mkdir_bin_cmd , 0 , sizeof ( mkdir_bin_cmd ) ) ;
	snprintf ( mkdir_bin_cmd , sizeof ( mkdir_bin_cmd ) , df_mkdir_cmd , bin_path ) ;
	system ( mkdir_bin_cmd ) ;

	// about log config
	FILE * config_fp = fopen ( p_private->i_config_path , "r+" ) ;
	if ( NULL == config_fp ) {
		while ( 1 ) {
			config_fp = fopen ( p_private->i_config_path , "w" ) ;	// 開檔案，可寫
			if ( config_fp != NULL ) {
				break ;
			}
			sleep ( 1 ) ;
		}
		fprintf ( config_fp , "%s" , "$tarLogSize   = \"500000\"\n" ) ;
		fflush ( config_fp ) ;
		fclose ( config_fp ) ;
	} else {
		fclose ( config_fp ) ;
	}

	// lock file
	FILE * lock_fp = fopen ( p_private->i_lock_path , "r+" ) ;
	if ( NULL == lock_fp ) {
		while ( 1 ) {
			lock_fp = fopen ( p_private->i_lock_path , "w" ) ;	// 開檔案，可寫
			if ( lock_fp != NULL ) {
				break ;
			}
			sleep ( 1 ) ;
		}
		fclose ( lock_fp ) ;
	} else {
		fclose ( lock_fp ) ;
	}

	// 讀取log設定檔
	Str tarLogSize = S_Get_config_val ( obj , "tarLogSize" ) ;
	if ( NULL == tarLogSize ) {
		return -3 ;
	}
	p_private->i_log_tar_sz = strtol ( tarLogSize , NULL , 10 ) ;
	free ( tarLogSize ) ;
	tarLogSize = NULL ;

	FILE *fp = fopen ( p_private->i_log_path , "r+" ) ;
	if ( NULL == fp ) {	// 若讀失敗代表沒有此檔案
		fp = fopen ( p_private->i_log_path , "w+" ) ;		// 若讀不到則創檔
		if ( NULL != fp ) {
			fclose ( fp ) ;
		}
	} else {	// 若讀成功代表有此檔案
		fclose ( fp ) ;
	}

	// 取程式鎖定
	rtn = S_Get_pgm_lock ( obj ) ;
	if ( D_success != rtn ) {
		return - 4 ;
	}

	// 開啟log，準備開始紀錄
	while ( 1 ) {
		int rtn = S_Open_fp ( obj ) ;
		if ( rtn == 0 ) {
			break ;
		}
		sleep ( 1 ) ;
	}

	char Tar_Num[ strlen( Df_Tar_Num ) + strlen( pgm_name ) + 1 ];
	memset( Tar_Num , 0 , sizeof(Tar_Num) );
	snprintf(Tar_Num , sizeof(Tar_Num) , Df_Tar_Num , pgm_name ) ;
	FILE *log_config = fopen( Tar_Num , "r+" );	// 開檔案，可讀，若開失敗則無檔案，成功則已經存在檔案
	if ( log_config == NULL ) {
		while ( 1 ) {
			log_config = fopen( Tar_Num , "w" );	// 開檔案，可寫
			if ( log_config != NULL ) {
				break;
			}
			sleep( 1 );
		}
		fprintf( log_config , "%s" , "0" );
		fflush( log_config );
		fclose( log_config );
	} else if ( log_config != NULL ) {
		fclose( log_config );
	}
	return D_success ;
}

static void S_Close ( S_Log *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return ;
	}
	S_private_data *p_private = obj->i_private ;
	fclose ( p_private->i_log_fp ) ;
	return ;
}

static Str S_Rtn_Pgm_name ( S_Log *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_private_data *p_private = obj->i_private ;
	return p_private->i_pgm_name ;
}

static int S_Save ( S_Log*obj , CStr format , ... ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_private_data *p_private = obj->i_private ;
	va_list args;	// 不定參數
	struct timeb ts;
	struct tm *timeinfo;
	ftime( &ts );
	timeinfo = localtime( &ts.time );

	// 判斷Log資料大小，將檔案指標移動到最後面，並且使用ftell查看檔案大小
	fseek( p_private->i_log_fp , 0 , SEEK_END );
	int size = ftell( p_private->i_log_fp );

	// 若log大於 tarLogSize Bytes時將log tar起來，關檔並且再開新Log檔
	if ( size >= p_private->i_log_tar_sz ) {
		S_Tar_log ( obj ) ;
	}

	// 寫Log
	char TimeBuf[ 64 ] = { 0 };
	strftime( TimeBuf , 80 , "[%Y_%m_%d_%H:%M:%S]:" , timeinfo ); // Log 紀錄時間
	// 寫入Log
	fprintf( p_private->i_log_fp , "%s" , TimeBuf );
	fflush( p_private->i_log_fp );
	va_start( args , format );
	vfprintf( p_private->i_log_fp , format , args );
	fprintf( p_private->i_log_fp , "\n" );
	fflush( p_private->i_log_fp );
	va_end( args );

	return D_success ;
}

static Str S_Rtn_lock_path ( S_Log *obj ) {
	if ( ! obj ) {
		return NULL ;
	}
	S_private_data *p_private = obj->i_private ;
	return p_private->i_lock_path ;
}

S_Log *S_Log_tool_New ( void ) {
	S_Log *p_tmp = ( S_Log* ) calloc ( 1 , sizeof(S_Log) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}

	S_private_data *i_private_data = ( S_private_data* ) calloc ( 1 , sizeof(S_private_data) ) ;
	if ( NULL == i_private_data ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}
	memset ( i_private_data->i_pgm_name , 0 , sizeof ( i_private_data->i_pgm_name ) ) ;
	memset ( i_private_data->i_log_path , 0 , sizeof ( i_private_data->i_log_path ) ) ;
	memset ( i_private_data->i_config_path , 0 , sizeof ( i_private_data->i_config_path ) ) ;
	i_private_data->i_log_tar_sz = 0 ;
	i_private_data->i_log_fp = NULL;

	p_tmp->i_private = ( void* ) i_private_data ;
 	p_tmp->Initial = S_Initial ;
	p_tmp->Close = S_Close ;
	p_tmp->Rtn_Pgm_name = S_Rtn_Pgm_name ;
	p_tmp->Save = S_Save ;
	p_tmp->Rtn_lock_path = S_Rtn_lock_path ;			// 2017/12/15	//

	return p_tmp ;
}
void S_Log_tool_Delete ( S_Log **obj ) {
	S_private_data *p_private = ( * obj )->i_private ;
	if ( p_private->i_log_fp != NULL ) {
		fclose ( p_private->i_log_fp ) ;
		p_private->i_log_fp = NULL ;
	}

	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;
	free ( * obj ) ;
	* obj = NULL ;
	return ;
}
