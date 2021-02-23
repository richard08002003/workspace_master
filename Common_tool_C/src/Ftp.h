/*
 * Ftp.h
 *
 *  Created on: 2017年9月6日
 *      Author: richard
 */

#ifndef INC_FTP_H_
#define INC_FTP_H_

/********************************************************************************************************************************************************************
* http://phorum.com.tw/ShowPost/5609.aspx																															*
* FTP 是屬於 TCP 服務的一種， FTP 是所有通訊協定裡最特殊的，其他的通訊協定例如 HTTP、SMTP、POP3...都只需要一個通訊埠，然而 FTP 卻需要兩個通訊埠，一個用來傳遞客戶端與伺服器之間的命令，	*
* 一般設在 port 21，稱之為命令通訊埠(Command Port)；另一個是真正用來傳遞資料的，一般都設在 port 20，稱之為資料通訊埠(Data Port)。												*
*
* 被動式 FTP (PASV)																																					*
* 為了解決由伺服器連線到用戶端所產生的安全疑慮，因此發展出了另一種不同的連線模式，稱之為被動模式(Passive Mode, PASV)。讓用戶端程式可以在連線的時候，通知伺服器使用動模式連線。			*
* 使用被動模式 FTP ，不論命令連線或是資料連線都是由用戶端建立，以解決防火牆以及相關資安問題。當用戶端開啟 FTP 連線時，用戶端程式先在本機開兩個大於1023的通訊埠(N, N+1)，				*
* 利用 port N 與伺服器的 port 21 建立連線。不同於主動模式的連線方式，用戶端這次不再提供 N+1 port  與 IP 位址給伺服器，而是送出 PASV 的命令。伺服器收到 PASV 的命令之後，				*
* 即開啟一個大於 1023 的通訊埠(P)，並將這個通訊埠連同伺服器 IP 回覆給用戶端，被動等待用戶端連線，用戶端即利用 port N+1與伺服器所提供的 port P 建立資料連線。							*
*********************************************************************************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "Http.h"
#include "File.h"

typedef struct _S_Ftp S_Ftp ;
struct _S_Ftp {
	void *i_private ;

	int (*Begin_connect) ( S_Ftp *obj , CStr ip , CStr port , CStr user , CStr pwd ) ;			// 2017/09/11 完成
	int (*Pwd) ( S_Ftp *obj ) ;																	// 2017/09/12 完成
	int (*List) ( S_Ftp *obj , CStr dir_path ) ;												// 2017/09/11 完成
	Str (*List_show) ( S_Ftp *obj ) ;															// 2017/09/11 完成


	/********************************************************
	 * type I : binary   << 用 binary 的話就不會有換行的差異	*
	 * type A : asscii   << 換行在 linux and windows 會不同 	*
	 ********************************************************/
	int (*Download) ( S_Ftp *obj , CStr type , CStr srv_file_path , CStr local_file_path ) ;	// 2017/09/12 完成

	/********************************************************
	 * type I : binary   << 用 binary 的話就不會有換行的差異	*
	 * type A : asscii   << 換行在 linux and windows 會不同 	*
	 ********************************************************/
	int (*Upload) ( S_Ftp *obj , CStr type , CStr srv_file_path , CStr local_file_path ) ;		// 2017/09/12 完成

	int (*Rm_dir) ( S_Ftp *obj , CStr srv_dir_path ) ;											// 2017/09/12 完成
	int (*Rm_file) ( S_Ftp *obj , CStr srv_file_path ) ;										// 2017/09/12 完成
	int (*Re_name) ( S_Ftp *obj , CStr srv_file_name , CStr tar_file_name ) ;					// 2017/09/11 完成
	int (*Change_dir) ( S_Ftp *obj , CStr will_change_path ) ; 		// 變更工作目錄				// 2017/09/11 完成
	int (*Mkdir) ( S_Ftp *obj , CStr will_mkdir ) ;												// 2017/09/11 完成
	int (*Quit) ( S_Ftp *obj ) ;																// 2017/09/11 完成

} ;
extern S_Ftp *S_Ftp_tool_New ( void ) ;
extern void S_Ftp_tool_Delete ( S_Ftp **obj ) ;


#if 0	// 測試程式

int main ( ) {

	g_log = S_Log_tool_New ( ) ;
	if ( NULL == g_log ) {
		printf("S_Log_tool_New Error\n");
		return - 1 ;
	}
	int rtn = g_log->Initial ( g_log , DF_Pgm_Name ) ;
	if ( D_success != rtn ) {
		printf("Initial Error\n") ;
		S_Log_tool_Delete ( & g_log ) ;
		return -2 ;
	}


	S_Ftp *ftp = S_Ftp_tool_New ( ) ;
	if ( NULL == ftp ) {
		printf ( "NULL == ftp" ) ;
g_log->Save ( g_log , "NULL == ftp" ) ;
		S_Log_tool_Delete ( & g_log ) ;
		return - 3 ;
	}

 	char *ip = "59.120.234.70" ;
	char *port = "21" ;
	char *user = "witchery" ;
	char *password = "witchery050611" ;

	// 開始連線
	rtn = ftp->Begin_connect ( ftp , ip , port , user , password ) ;
	if( D_success != rtn){
g_log->Save ( g_log , "NULL == ftp, rtn = %d" ,  rtn ) ;
		S_Log_tool_Delete ( & g_log ) ;
	}
	g_log->Save ( g_log , "Connect to FTP success") ;
	printf ("Connect to FTP success\n") ;


	while ( 1 ) {
		char enter [ 5 + 1 ] = { 0 } ;
		printf ( ">> 1. Pwd.\n" ) ;
		printf ( ">> 2. List.\n" ) ;
		printf ( ">> 3. Download.\n" ) ;
		printf ( ">> 4. Upload.\n" ) ;
		printf ( ">> 5. RmDir.\n" ) ;
		printf ( ">> 6. RmFile.\n" ) ;
		printf ( ">> 7. ReName.\n" ) ;
		printf ( ">> 8. ChangeDir.\n" ) ;
		printf ( ">> 9. MkDir.\n" ) ;
		printf ( ">> 10. Quit.\n" ) ;

		fgets ( enter , sizeof ( enter ) , stdin ) ;
		char *ptr = strrchr ( enter , '\n' ) ;
		if ( NULL != ptr ) {
			* ptr = 0 ;
		}

		int i_enter = strtoul ( enter , NULL , 10 ) ;
		if ( strcmp ( enter , "clear" ) == 0 ) {
			i_enter = 98 ;
		} else if ( strcmp ( enter , "q" ) == 0 ) {
			i_enter = 99 ;
		}

		switch ( i_enter ) {
			case 1 : 	// pwd
			{
				rtn = ftp->Pwd ( ftp ) ;
				if ( D_success != rtn ) {
g_log->Save ( g_log , "Run Pwd Error, rtn = %d" , rtn ) ;
				}
			}
				break ;
			case 2 :	// list
			{
				printf ( "Please enter List Directory : " ) ;
				char will_list_dir [ 1024 + 1 ] = { 0 } ;
				fgets ( will_list_dir , sizeof ( will_list_dir ) , stdin ) ;
				char *ptr = strrchr ( will_list_dir , '\n' ) ;
				if ( NULL != ptr ) {
					* ptr = 0 ;
				}

				rtn = ftp->List ( ftp , will_list_dir ) ;
				if ( D_success != rtn ) {
g_log->Save ( g_log , "Run List Error, rtn = %d" , rtn  ) ;
				} else {
					Str list_data = ftp->List_show ( ftp ) ;
					printf ( "list_data = [%s]\n" , list_data ) ;
g_log->Save ( g_log , "list_data = [%s]\n" , list_data ) ;
				}
			}
				break ;
			case 3 :	// download
			{
				printf ( "Please enter the FTP file path : " ) ;
				char will_download [ 1024 + 1 ] = { 0 } ;
				fgets ( will_download , sizeof ( will_download ) , stdin ) ;
				char *ptr = strrchr ( will_download , '\n' ) ;
				if ( NULL != ptr ) {
					* ptr = 0 ;
				}

				rtn = ftp->Download ( ftp , "I" , will_download , "/media/richard/richard_hdd/workspace_CUP/Common_tool/Download_data" ) ;
				if ( D_success != rtn ) {
g_log->Save ( g_log , "Run Download Error, rtn = %d" , rtn ) ;
				}
			}
				break ;
			case 4 :	// upload
			{
				rtn = ftp->Upload ( ftp , "I" , "/usr/local/upgrade_system/richard/V1.0/Upload_data" , "/media/richard/richard_hdd/workspace_CUP/Common_tool/Upload_data" ) ;
				if ( D_success != rtn ) {
g_log->Save ( g_log , "Run Upload Error, rtn = %d" , rtn ) ;
				}
			}
				break ;
			case 5 :	// rmdir
			{
				rtn = ftp->Rm_dir ( ftp , "/usr/local/upgrade_system/richard/Test_Dir") ;
				if ( D_success != rtn ) {
g_log->Save ( g_log , "Run Rm_dir Error, rtn = %d" , rtn ) ;
				}
			}
				break ;
			case 6 :	// rmfile
			{
				rtn = ftp->Rm_file ( ftp , "/usr/local/upgrade_system/richard/Test_Dir/test_data") ;
				if ( D_success != rtn ) {
g_log->Save ( g_log , "Run Rm_file Error, rtn = %d" , rtn ) ;
				}
			}
				break ;
			case 7 :	// rename
			{
				rtn = ftp->Re_name ( ftp , "/usr/local/upgrade_system/richard/Test_Dir/test_data", "/usr/local/upgrade_system/richard/Test_Dir/rename_data") ;
				if ( D_success != rtn ) {
g_log->Save ( g_log , "Run Rm_file Error, rtn = %d" , rtn ) ;
				}
			}
				break ;
			case 8 :	// changedir
			{
				rtn = ftp->Change_dir ( ftp , "/usr/local/upgrade_system/richard/Test_Dir/") ;
				if ( D_success != rtn ) {
g_log->Save ( g_log , "Run Change_dir Error, rtn = %d" , rtn ) ;
				}
			}
				break ;
			case 9 :	// mkdir
			{
				rtn = ftp->Mkdir ( ftp , "/usr/local/upgrade_system/richard/Test_Dir/") ;
				if ( D_success != rtn ) {
g_log->Save ( g_log , "Run Mkdir Error, rtn = %d" , rtn ) ;
				}
			}
				break ;
			case 10 :	// quit
			{
				rtn = ftp->Quit ( ftp ) ;
				if ( D_success != rtn ) {
g_log->Save ( g_log , "Run Quit Error" ) ;
				}
				S_Ftp_tool_Delete ( & ftp ) ;
				S_Log_tool_Delete ( & g_log ) ;
				return 0 ;
			}
			default :
				system ( "clear" ) ;
				break ;
		}
	}



	S_Ftp_tool_Delete ( & ftp ) ;
	S_Log_tool_Delete ( & g_log ) ;
	return 0 ;
}
#endif
#endif /* INC_FTP_H_ */
