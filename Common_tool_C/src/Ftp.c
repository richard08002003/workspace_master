/*
 * Ftp.c
 *
 *  Created on: 2017年9月6日
 *      Author: richard
 */
#include "Ftp.h"

#include "Setup.h"	// log 用

// About Command //
#define df_user		"user %s\r\n"
#define df_paswd	"pass %s\r\n"
#define df_list		"list %s\r\n"
#define df_type  	"type %s\r\n"
#define df_retr  	"retr %s\r\n"
#define df_rmd  	"rmd %s\r\n"
#define df_dele  	"dele %s\r\n"
#define df_rnfr  	"rnfr %s\r\n"
#define df_rnto  	"rnto %s\r\n"
#define df_cwd  	"cwd %s\r\n"
#define df_mkdir  	"mkd %s\r\n"
#define df_stor		"stor %s\r\n"
// End of Command //

typedef struct _S_private_data {
	S_RdWrTmO *i_tmo ;	// timeout用
	S_Https *i_cmmd ;	// 指令用
	S_Https *i_data ;	// 資料用
	Str i_data_ip ;		// 收資料用IP
	Str i_data_port ;	// 收資料用Port
} S_private_data ;

/// 僅內部使用 ///
static int S_Pasv_connect ( S_Ftp *obj ) ;							// 被動式連線
static ULInt S_Write_and_read_for_cmmd ( S_Ftp *obj , CStr wr ) ;	// cmd用寫＆讀
static ULInt S_Read_for_cmmd ( S_Ftp *obj ) ;						// cmd讀
/// 僅內部使用 ///

static int S_Begin_connect ( S_Ftp *obj , CStr ip , CStr port , CStr user , CStr pwd ) ;
static int S_Pwd ( S_Ftp *obj ) ;
static int S_List ( S_Ftp *obj , CStr dir_path ) ;
static Str S_List_show ( S_Ftp *obj ) ;
static int S_Download ( S_Ftp *obj , CStr type , CStr srv_file_path , CStr local_file_path ) ;
static int S_Upload ( S_Ftp *obj , CStr type , CStr srv_file_path , CStr local_file_path ) ;

static int S_Rm_dir ( S_Ftp *obj , CStr srv_dir_path ) ;
static int S_Rm_file ( S_Ftp *obj , CStr srv_file_path ) ;
static int S_Re_name ( S_Ftp *obj , CStr srv_file_name , CStr tar_file_name ) ;
static int S_Change_dir ( S_Ftp *obj , CStr will_change_path ) ;
static int S_Mkdir ( S_Ftp *obj , CStr will_mkdir ) ;
static int S_Quit ( S_Ftp *obj ) ;


/// 僅內部使用 ///
// 被動式連線
static int S_Pasv_connect ( S_Ftp *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	if ( p_private->i_data_port != NULL ) {
		free ( p_private->i_data_port ) ;
		p_private->i_data_port = NULL ;
	}
	if(p_private->i_data_ip != NULL ) {
		free ( p_private->i_data_ip ) ;
		p_private->i_data_ip = NULL ;
	}

	UChar msg[] = "pasv\r\n" ;
	int rtn = p_private->i_cmmd->Write_bin ( p_private->i_cmmd , msg , 6 ) ;
	if ( rtn != D_success ) {
		return - 2 ;
	}

	rtn = p_private->i_cmmd->Read_bin ( p_private->i_cmmd , true ) ;
	if ( rtn != D_success ) {
		return - 3 ;
	}

	UStr buf = p_private->i_cmmd->Rtn_read_bin ( p_private->i_cmmd ) ;

	UChar ipAndPort [ 6 ] = { 0 } ;
	rtn = sscanf ( ( char* ) buf , "%*[^(](%u,%u,%u,%u,%u,%u)" , \
			( UInt* ) &ipAndPort [ 0 ] , \
			( UInt* ) &ipAndPort [ 1 ] , \
			( UInt* ) &ipAndPort [ 2 ] , \
			( UInt* ) &ipAndPort [ 3 ] , \
			( UInt* ) &ipAndPort [ 4 ] , \
			( UInt* ) &ipAndPort [ 5 ] ) ;

	if ( 6 != rtn ) {
		return -4 ;
	}

	char ip [ 15 + 1 ] = { 0 } ;
	snprintf ( ip , sizeof ( ip ) , "%u.%u.%u.%u" , ipAndPort [ 0 ] , ipAndPort [ 1 ] , ipAndPort [ 2 ] , ipAndPort [ 3 ] ) ;
 	p_private->i_data_ip = ( char * ) calloc ( strlen ( ip ) + 1 , 1 ) ;
	if ( NULL == p_private->i_data_ip ) {
		return - 5 ;
	}
	memcpy ( p_private->i_data_ip , ip , strlen ( ip ) ) ;
//g_log->Save ( g_log , "i_data_ip = %s" , p_private->i_data_ip ) ;

	char port [ 5 + 1 ] = { 0 } ;
	snprintf ( port , sizeof ( port ) , "%u" , ( ipAndPort [ 4 ] << 8 ) + ipAndPort [ 5 ] ) ;
 	p_private->i_data_port = ( char * ) calloc ( strlen ( port ) + 1 , 1 ) ;
	if ( NULL == p_private->i_data_port ) {
		return - 6 ;
	}
	memcpy ( p_private->i_data_port , port , strlen ( port ) ) ;		// port = 255 * 256 + response
//g_log->Save ( g_log , "i_data_port = %s" , p_private->i_data_port ) ;

	return D_success ;
}
static ULInt S_Write_and_read_for_cmmd ( S_Ftp *obj , CStr wr ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! wr ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;

//g_log->Save ( g_log , "Write_str = [%s]" , wr ) ;

	int rtn = p_private->i_cmmd->Write_str ( p_private->i_cmmd , wr ) ;
	if ( D_success != rtn ) {
//g_log->Save ( g_log , "Write_str error, rtn = %d" , rtn ) ;
		return - 2 ;
	}


	rtn = p_private->i_cmmd->Read_str ( p_private->i_cmmd , true ) ;
	if ( D_success != rtn ) {
//g_log->Save ( g_log , "Read_str error, rtn = %d" , rtn ) ;
		return - 3 ;
	}

	Str str = p_private->i_cmmd->Rtn_read_str ( p_private->i_cmmd ) ;
//g_log->Save ( g_log , "Response = [%s]" , str ) ;
//printf ( "Response = [%s]\n" , str ) ;

	if ( str == NULL ) {
		return - 4 ;
	}
 	return strtoul ( str , NULL , 10 ) ;
}
static ULInt S_Read_for_cmmd ( S_Ftp *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	int rtn = p_private->i_cmmd->Read_str ( p_private->i_cmmd , true ) ;
	if ( D_success != rtn ) {
//g_log->Save ( g_log , "Read_str error, rtn = %d" , rtn ) ;
		return -2 ;
	}
	Str str = p_private->i_cmmd->Rtn_read_str ( p_private->i_cmmd ) ;
//g_log->Save ( g_log , "Response = [%s]" , str ) ;
	if ( str == NULL ) {
		return -3 ;
	}
	return strtoul ( str, NULL , 10 ) ;
}
/// End of 僅內部使用 ///

#if 0 // 測試用FTP Server
/********************************************************
* 1. connect to server 59.120.234.70:21					*
* 2. read from server : 220 (vsFTPd 2.0.5)				*
* 3. write to server  : USER witchery					*
* 4. read from server : 331 Please specify the password.*
* 5. write to server  : PASS witchery050611				*
* 6. read from server : 230 Login successful.			*
*********************************************************/
#endif
static int S_Begin_connect ( S_Ftp *obj , CStr ip , CStr port , CStr user , CStr pwd ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! ip ) || ( ! port ) || ( ! user ) || ( ! pwd ) ) {
		return - 1 ;
	}
//g_log->Save ( g_log , "Will Connect to ip = %s, port = %s" , ip , port ) ;

	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	int rtn = 0 ;

	// Connect
	rtn = p_private->i_cmmd->Connect ( p_private->i_cmmd , ip , port ) ;
	if ( D_success != rtn ) {
//g_log->Save ( g_log ,  "Connect fail , rtn = %d" , rtn  ) ;
		return - 2 ;
	}
	int i_fd = p_private->i_cmmd->Rtn_sock_fd ( p_private->i_cmmd ) ;
//g_log->Save ( g_log ,  "Connect success , i_fd = %d" , i_fd  ) ;

	rtn = p_private->i_tmo->Set_sockopt ( p_private->i_tmo , i_fd , 100000 , 100000 ) ;
	if ( D_success != rtn ) {
//g_log->Save ( g_log ,  "Set_sockopt fail , rtn = %d" , rtn ) ;
		return -3 ;
	}
//g_log->Save ( g_log ,  "Set_sockopt success" ) ;

	ULInt rtnFtpValue = S_Read_for_cmmd ( obj ) ;
	if ( 220 != rtnFtpValue ) {
		return - 5 ;
	}

	// User
	char user_str [ strlen ( df_user ) + strlen ( user ) + 1 ] ;
	memset ( user_str , 0 , sizeof ( user_str ) ) ;
	snprintf ( user_str , sizeof ( user_str ) , df_user , user ) ;

	rtnFtpValue = S_Write_and_read_for_cmmd ( obj , user_str ) ;
	if ( 230 == rtnFtpValue ) {   // 表示無須密碼
		return D_success ;
	} else if ( 331 != rtnFtpValue ) {
		return - 5 ;
	}

	// Password
	char paswd_str [ strlen ( df_paswd ) + strlen ( pwd ) + 1 ] ;
	memset ( paswd_str , 0 , sizeof ( paswd_str ) ) ;
	snprintf ( paswd_str , sizeof ( paswd_str ) , df_paswd , pwd ) ;
	rtnFtpValue = S_Write_and_read_for_cmmd ( obj , paswd_str ) ;
	if ( 230 != rtnFtpValue ) {
		return - 6 ;
	}
	return D_success ;
}
static int S_Pwd ( S_Ftp *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
//	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
//g_log->Save ( g_log , "<<< Run Pwd Command >>>" ) ;
	// Pwd
	char pwd_str [ ] = "pwd\r\n" ;

	int rtnFtpValue = S_Write_and_read_for_cmmd ( obj , pwd_str ) ;
	if ( ( 257 != rtnFtpValue ) ) {
//g_log->Save ( g_log , "257 != rtnFtpValue, rtnFtpValue = %s" , rtnFtpValue ) ;
		return - 2 ;
	}

	return D_success ;
}
static int S_List ( S_Ftp *obj , CStr dir_path ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! dir_path ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;

//g_log->Save ( g_log , "<<< Run List Command >>>" ) ;
//g_log->Save ( g_log , "Will S_Pasv_connect" ) ;
	int rtn = S_Pasv_connect ( obj ) ;
	if ( D_success != rtn ) {
//g_log->Save ( g_log , "S_Pasv_connect error, rtn = %d" , rtn ) ;
		return - 2 ;
	}
//g_log->Save ( g_log , "S_Pasv_connect success, rtn = %d" , rtn ) ;


	// 連線至"資料"用的IP與Port
//g_log->Save ( g_log , "Will Connect to ip = %s, port = %s" , p_private->i_data_ip , p_private->i_data_port ) ;
	rtn = p_private->i_data->Connect ( p_private->i_data , p_private->i_data_ip , p_private->i_data_port ) ;
	if ( D_success != rtn ) {
//g_log->Save ( g_log , "Connect error, rtn = %d" , rtn ) ;
		return - 3 ;
	}
//g_log->Save ( g_log , "i_data->Connect success, rtn = %d" , rtn ) ;

	// List
	char list_str [ strlen ( df_list ) + strlen ( dir_path ) + 1 ] ;
	memset ( list_str , 0 , sizeof ( list_str ) ) ;
	snprintf ( list_str , sizeof ( list_str ) , df_list , dir_path ) ;

	int rtnFtpValue = S_Write_and_read_for_cmmd ( obj , list_str ) ;
	if ( ( 300 <= rtnFtpValue ) || ( 0 == rtnFtpValue ) ) {
//g_log->Save ( g_log , "( 300 <= rtnFtpValue ) || ( 0 == rtnFtpValue ), rtnFtpValue = %s" , rtnFtpValue ) ;
		p_private->i_data->Close_sock_fd ( p_private->i_data ) ; // 關閉"資料"用的通訊
		return - 4 ;
	}

	rtn = p_private->i_data->Read_str ( p_private->i_data , false ) ;
	if ( D_success != rtn ) {
//g_log->Save ( g_log , "i_data->Read_str error, rtn = %d" , rtn ) ;
		return - 5 ;
	}

	rtnFtpValue = S_Read_for_cmmd ( obj ) ;
	if ( 226 != rtnFtpValue ) {
//g_log->Save ( g_log , "S_Read_for_cmmd, rtn = %d" , rtn ) ;
		return - 6 ;
	}
	p_private->i_data->Close_sock_fd ( p_private->i_data ) ; // 關閉"資料"用的通訊
	return D_success ;
}

static Str S_List_show ( S_Ftp *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	return p_private->i_data->Rtn_read_str ( p_private->i_data ) ;
}

static int S_Download ( S_Ftp *obj , CStr type , CStr srv_file_path , CStr local_file_path ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! type ) || ( ! srv_file_path ) || ( ! local_file_path ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
//g_log->Save ( g_log , "<<< Run Download Command >>>" ) ;
//g_log->Save ( g_log , "Will S_Pasv_connect" ) ;
		int rtn = S_Pasv_connect ( obj ) ;
		if ( D_success != rtn ) {
//g_log->Save ( g_log , "S_Pasv_connect error, rtn = %d" , rtn ) ;
			return - 2 ;
		}
//g_log->Save ( g_log , "S_Pasv_connect success, rtn = %d" , rtn ) ;


		// 連線至"資料"用的IP與Port
//g_log->Save ( g_log , "Will Connect to ip = %s, port = %s" , p_private->i_data_ip , p_private->i_data_port ) ;
		rtn = p_private->i_data->Connect ( p_private->i_data , p_private->i_data_ip , p_private->i_data_port ) ;
		if ( D_success != rtn ) {
//g_log->Save ( g_log , "Connect error, rtn = %d" , rtn ) ;
			return - 3 ;
		}
//g_log->Save ( g_log , "i_data->Connect success, rtn = %d" , rtn ) ;

	// TYPE
	char type_str [ strlen ( df_type ) + strlen ( type ) + 1 ] ;
	memset ( type_str , 0 , sizeof ( type_str ) ) ;
	snprintf ( type_str , sizeof ( type_str ) , df_type , type ) ;

	ULInt rtnFtpValue = S_Write_and_read_for_cmmd ( obj , type_str ) ;
	if ( rtnFtpValue != 200 ) {
		p_private->i_data->Close_sock_fd ( p_private->i_data ) ;
		return - 4 ;
	}

	// RETR
	char retr_str [ strlen ( df_retr ) + strlen ( srv_file_path ) + 1 ] ;
	memset ( retr_str , 0 , sizeof ( retr_str ) ) ;
	snprintf ( retr_str , sizeof ( retr_str ) , df_retr , srv_file_path ) ;

	rtnFtpValue = S_Write_and_read_for_cmmd ( obj , retr_str ) ;
	if ( ( 300 <= rtnFtpValue ) || ( 0 == rtnFtpValue ) ) {
		p_private->i_data->Close_sock_fd ( p_private->i_data ) ;
		return - 5 ;
	}

	rtn = p_private->i_data->Read_to_file ( p_private->i_data , local_file_path , false ) ;
	if ( rtn <= 0 ) {
//g_log->Save ( g_log , "Read_to_file fail, rtn = %d" , rtn ) ;
		return - 6 ;
	}
	p_private->i_data->Close_sock_fd ( p_private->i_data ) ;

	rtnFtpValue = S_Read_for_cmmd ( obj ) ;
	if ( rtnFtpValue != 226 ) {
		return - 7 ;
	}

	return D_success ;
}
static int S_Upload ( S_Ftp *obj , CStr type , CStr srv_file_path , CStr local_file_path ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! type ) || ( ! srv_file_path ) || ( ! local_file_path ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
//g_log->Save ( g_log , "<<< Run Upload Command >>>" ) ;
//g_log->Save ( g_log , "Will S_Pasv_connect" ) ;
		int rtn = S_Pasv_connect ( obj ) ;
		if ( D_success != rtn ) {
//g_log->Save ( g_log , "S_Pasv_connect error, rtn = %d" , rtn ) ;
			return - 2 ;
		}
//g_log->Save ( g_log , "S_Pasv_connect success, rtn = %d" , rtn ) ;


		// 連線至"資料"用的IP與Port
//g_log->Save ( g_log , "Will Connect to ip = %s, port = %s" , p_private->i_data_ip , p_private->i_data_port ) ;
		rtn = p_private->i_data->Connect ( p_private->i_data , p_private->i_data_ip , p_private->i_data_port ) ;
		if ( D_success != rtn ) {
//g_log->Save ( g_log , "Connect error, rtn = %d" , rtn ) ;
			return - 3 ;
		}
//g_log->Save ( g_log , "i_data->Connect success, rtn = %d" , rtn ) ;

	// TYPE
	char type_str [ strlen ( df_type ) + strlen ( type ) + 1 ] ;
	memset ( type_str , 0 , sizeof ( type_str ) ) ;
	snprintf ( type_str , sizeof ( type_str ) , df_type , type ) ;
	ULInt rtnFtpValue = S_Write_and_read_for_cmmd ( obj , type_str ) ;
	if ( rtnFtpValue != 200 ) {
		p_private->i_data->Close_sock_fd ( p_private->i_data ) ;
		return - 4 ;
	}

	// STOR
	char stor_str [ strlen ( df_stor ) + strlen ( srv_file_path ) + 1 ] ;
	memset ( stor_str , 0 , sizeof ( stor_str ) ) ;
	snprintf ( stor_str , sizeof ( stor_str ) , df_stor , srv_file_path ) ;
	rtnFtpValue = S_Write_and_read_for_cmmd ( obj , stor_str ) ;
	if ( ( 300 <= rtnFtpValue ) || ( 0 == rtnFtpValue ) ) {
		p_private->i_data->Close_sock_fd ( p_private->i_data ) ;
		return - 5 ;
	}

	// About Read Binary file
	S_File *file_obj = S_File_tool_New ( ) ;
	if ( NULL == file_obj ) {
		return - 6 ;
	}

	rtn = file_obj->Read_bin_from_file ( file_obj , local_file_path ) ;
	if ( D_success != rtn ) {
		return - 7 ;
	}
	UStr upload_data = file_obj->Rtn_bin_from_file ( file_obj ) ;
	if ( NULL == upload_data ) {
		return - 8 ;
	}

	LInt uplpad_data_sz = file_obj->Rtn_file_sz ( file_obj ) ;

	S_File_tool_Delete ( & file_obj ) ;

	// Upload
	rtn = p_private->i_data->Write_bin ( p_private->i_data , upload_data , uplpad_data_sz ) ;
	if ( D_success != rtn ) {
		return - 9 ;
	}

	p_private->i_data->Close_sock_fd ( p_private->i_data ) ;

	rtnFtpValue = S_Read_for_cmmd ( obj ) ;
	if ( rtnFtpValue != 226 ) {
		return - 10 ;
	}

	return D_success ;
}
static int S_Rm_dir ( S_Ftp *obj , CStr srv_dir_path ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! srv_dir_path ) ) {
		return - 1 ;
	}
//	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
//g_log->Save ( g_log , "<<< Run Rm_dir Command >>>" ) ;

	// RMD
	char rmd_str [ strlen ( df_rmd ) + strlen ( srv_dir_path ) + 1 ] ;
	memset ( rmd_str , 0 , sizeof ( rmd_str ) ) ;
	snprintf ( rmd_str , sizeof ( rmd_str ) , df_rmd , srv_dir_path ) ;

	ULInt rtnFtpValue = S_Write_and_read_for_cmmd ( obj , rmd_str ) ;
	if ( rtnFtpValue != 250 ) {
		return - 2 ;
	}
 	return D_success ;
}
static int S_Rm_file ( S_Ftp *obj , CStr srv_file_path ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! srv_file_path )  ) {
		return - 1 ;
	}
//	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
//g_log->Save ( g_log , "<<< Run Rm_file Command >>>" ) ;

	// DELE
	char dele_str [ strlen ( df_dele ) + strlen ( srv_file_path ) + 1 ] ;
	memset ( dele_str , 0 , sizeof ( dele_str ) ) ;
	snprintf ( dele_str , sizeof ( dele_str ) , df_dele , srv_file_path ) ;

	ULInt rtnFtpValue = S_Write_and_read_for_cmmd ( obj , dele_str ) ;
	if ( rtnFtpValue != 250 ) {
		return - 2 ;
	}
	return D_success ;
}
static int S_Re_name ( S_Ftp *obj , CStr srv_file_name , CStr tar_file_name ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! srv_file_name ) || ( ! tar_file_name ) ) {
		return - 1 ;
	}
//	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
//g_log->Save ( g_log , "<<< Run Re_name Command >>>" ) ;

	// RNFR
	char rnfr_str [ strlen ( df_rnfr ) + strlen ( srv_file_name ) + 1 ] ;
	memset ( rnfr_str , 0 , sizeof ( rnfr_str ) ) ;
	snprintf ( rnfr_str , sizeof ( rnfr_str ) , df_rnfr , srv_file_name ) ;

	ULInt rtnFtpValue = S_Write_and_read_for_cmmd ( obj , rnfr_str ) ;
	if ( rtnFtpValue != 350 ) {
		return - 2 ;
	}

	// RNTO
	char rnto_str [ strlen ( df_rnto ) + strlen ( tar_file_name ) + 1 ] ;
	memset ( rnto_str , 0 , sizeof ( rnto_str ) ) ;
	snprintf ( rnto_str , sizeof ( rnto_str ) , df_rnto , tar_file_name ) ;

	rtnFtpValue = S_Write_and_read_for_cmmd ( obj , rnto_str ) ;
	if ( rtnFtpValue != 250 ) {
		return - 3 ;
	}

	return D_success ;
}
static int S_Change_dir ( S_Ftp *obj , CStr will_change_path ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! will_change_path ) ) {
		return - 1 ;
	}
//	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
//g_log->Save ( g_log , "<<< Run Change_dir Command >>>" ) ;

	// CWD
	char cwd_str [ strlen ( df_cwd ) + strlen ( will_change_path ) + 1 ] ;
	memset ( cwd_str , 0 , sizeof ( cwd_str ) ) ;
	snprintf ( cwd_str , sizeof ( cwd_str ) , df_cwd , will_change_path ) ;

	ULInt rtnFtpValue = S_Write_and_read_for_cmmd ( obj , cwd_str ) ;
	if ( rtnFtpValue != 250 ) {
		return - 2 ;
	}
	return D_success ;
}
static int S_Mkdir ( S_Ftp *obj , CStr will_mkdir ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! will_mkdir ) ) {
		return - 1 ;
	}
//	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
//g_log->Save ( g_log , "<<< Run Mkdir Command >>>" ) ;

	// MKD
	char mkdir_str [ strlen ( df_mkdir ) + strlen ( will_mkdir ) + 1 ] ;
	memset ( mkdir_str , 0 , sizeof ( mkdir_str ) ) ;
	snprintf ( mkdir_str , sizeof ( mkdir_str ) , df_mkdir , will_mkdir ) ;

	ULInt rtnFtpValue = S_Write_and_read_for_cmmd ( obj , mkdir_str ) ;
	if ( rtnFtpValue != 257 ) {
		return - 2 ;			//	Note : 若是本身已經有建立或者 , 無法建立都會回應 550
	}

	return D_success ;
}
static int S_Quit ( S_Ftp *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
//g_log->Save ( g_log , "<<< Run Quit Command >>>" ) ;
	char str[] = "quit\r\n" ;
	ULInt rtnFtpValue = S_Write_and_read_for_cmmd ( obj ,str ) ;
	if ( rtnFtpValue != 221 ) {
		return -1 ;
	}
	p_private->i_cmmd->Close_sock_fd ( p_private->i_cmmd ) ;

	return D_success ;
}

S_Ftp *S_Ftp_tool_New ( void ) {
	S_Ftp *p_tmp = ( S_Ftp* ) calloc ( 1 , sizeof(S_Ftp) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}

	S_private_data *i_private_data = ( S_private_data* ) calloc ( 1 , sizeof(S_private_data) ) ;
	if ( NULL == i_private_data ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}
	i_private_data->i_tmo = S_RdWrTmO_tool_New ( ) ;
	if ( i_private_data->i_tmo == NULL ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		free ( i_private_data ) ;
		i_private_data = NULL ;
		return NULL ;
	}

	i_private_data->i_cmmd = S_Https_tool_New ( ) ;
	if ( i_private_data->i_cmmd == NULL ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		S_RdWrTmO_tool_Delete ( & i_private_data->i_tmo ) ;
		free ( i_private_data ) ;
		i_private_data = NULL ;
		return NULL ;
	}

	i_private_data->i_data = S_Https_tool_New ( ) ;
	if ( i_private_data->i_data == NULL ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		S_RdWrTmO_tool_Delete ( & i_private_data->i_tmo ) ;
		S_Https_tool_Delete ( & i_private_data->i_cmmd ) ;
		free ( i_private_data ) ;
		i_private_data = NULL ;
		return NULL ;
	}
	p_tmp->i_private = ( void* ) i_private_data ;

	p_tmp->Begin_connect = S_Begin_connect ;
	p_tmp->Pwd = S_Pwd ;

	p_tmp->List = S_List ;
	p_tmp->List_show = S_List_show ;

	p_tmp->Download = S_Download ;

	p_tmp->Upload = S_Upload ;

	p_tmp->Rm_dir = S_Rm_dir ;
	p_tmp->Rm_file = S_Rm_file ;
	p_tmp->Re_name = S_Re_name ;
	p_tmp->Change_dir = S_Change_dir ;
	p_tmp->Mkdir = S_Mkdir ;

	p_tmp->Quit = S_Quit ;

	return p_tmp ;
}
void S_Ftp_tool_Delete ( S_Ftp **obj ) {
	S_private_data *p_private = ( * obj )->i_private ;
	S_RdWrTmO_tool_Delete ( & ( p_private->i_tmo ) ) ;
	S_Https_tool_Delete ( & ( p_private->i_cmmd ) ) ;
	S_Https_tool_Delete ( & ( p_private->i_data ) ) ;

	free ( p_private->i_data_ip ) ;
	p_private->i_data_ip = NULL ;
	free ( p_private->i_data_port ) ;
	p_private->i_data_port = NULL ;

	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;

	free ( * obj ) ;
	* obj = NULL ;
	return ;
}
