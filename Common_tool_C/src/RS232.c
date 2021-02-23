/*
 * RS232.c
 *
 *  Created on: 2017/6/2
 *      Author: richard
 */
#include "RS232.h"

#define DF_Prolog     	"0000" 				// DF_Prolog : NAD + PCB
#define DF_Prolog_Len 	strlen(DF_Prolog) 	// DF_Prolog長度
#define DF_Info_Len   	( 2 )				// Information長度 LEN : 1 Bytes

typedef struct _S_private_data {
	int i_reader_fd ;
	int i_timeout ;
	char *mlc_baudrate ;
	char *mlc_comport ;

	char i_read_data [ 1024 + 1 ] ;	// 512Bytes
	LInt read_data_sz ;
} S_private_data ;

static int S_RS232_Setup ( S_RS232 *obj , CStr baudrate , CStr comport , int timeout ) ;
static int S_Get_baudrate ( S_RS232 *obj ) ;
static int S_Open_Device ( S_RS232 *obj ) ;
static int S_Rtn_reader_fd ( S_RS232 *obj ) ;
static int S_Rtn_timeout ( S_RS232 *obj ) ;

static int S_Apdu_string_to_hex ( CChar* info , UChar *o_buf , int o_buf_sz ) ;
static int S_Only_string_to_hex ( CStr input , UStr o_buf , int o_buf_sz ) ;					// 2017/12/21

//static int S_RS232_Write ( S_RS232 *obj , CStr info , UChar *send_buf ) ;
static int S_RS232_Write ( S_RS232 *obj , CStr info ) ;
static int S_RS232_Write_only ( S_RS232 *obj , CStr send ) ;


static int S_RS232_Read ( S_RS232 *obj ) ;
static Str Rtn_read_data ( S_RS232 *obj ) ;

static int S_Calculation_edc ( char *data , char *buf , int bufSz ) ;
static int S_Get_Information ( S_RS232 *obj , char *input , char **outputBody , int *sz ) ;

static int S_RS232_Setup ( S_RS232 *obj , CStr baudrate , CStr comport , int timeout ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! baudrate ) || ( ! comport ) || ( ! timeout ) ) {
		return - 1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	p_private->i_reader_fd = - 1 ;
	p_private->i_timeout = timeout ;

	int baudrate_sz = strlen ( baudrate ) ;
	p_private->mlc_baudrate = ( char * ) calloc ( baudrate_sz + 1 , sizeof(char) ) ;
	if ( NULL == p_private->mlc_baudrate ) {
		return - 2 ;
	}
	memcpy ( p_private->mlc_baudrate , baudrate , baudrate_sz ) ;

	int comport_sz = strlen ( comport ) ;
	p_private->mlc_comport = ( char * ) calloc ( comport_sz + 1 , sizeof(char) ) ;
	if ( NULL == p_private->mlc_comport ) {
		return - 3 ;
	}
	memcpy ( p_private->mlc_comport , comport , comport_sz ) ;

	return D_success ;	// success
}

static int S_Get_baudrate ( S_RS232 *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return -1 ;
	}
	S_private_data *p_private = (S_private_data *)obj->i_private ;
	if ( strcmp( p_private->mlc_baudrate , "75" ) == 0 ) {
		return B75;
	} else if ( strcmp( p_private->mlc_baudrate , "110" ) == 0 ) {
		return B110;
	} else if ( strcmp( p_private->mlc_baudrate , "134" ) == 0 ) {
		return B134;
	} else if ( strcmp( p_private->mlc_baudrate , "150" ) == 0 ) {
		return B150;
	} else if ( strcmp( p_private->mlc_baudrate , "200" ) == 0 ) {
		return B200;
	} else if ( strcmp( p_private->mlc_baudrate , "300" ) == 0 ) {
		return B300;
	} else if ( strcmp( p_private->mlc_baudrate , "600" ) == 0 ) {
		return B600;
	} else if ( strcmp( p_private->mlc_baudrate , "1200" ) == 0 ) {
		return B1200;
	} else if ( strcmp( p_private->mlc_baudrate , "1800" ) == 0 ) {
		return B1800;
	} else if ( strcmp( p_private->mlc_baudrate , "2400" ) == 0 ) {
		return B2400;
	} else if ( strcmp( p_private->mlc_baudrate , "4800" ) == 0 ) {
		return B4800;
	} else if ( strcmp( p_private->mlc_baudrate , "9600" ) == 0 ) {
		return B9600;
	} else if ( strcmp( p_private->mlc_baudrate , "38400" ) == 0 ) {
		return B38400;
	} else if ( strcmp( p_private->mlc_baudrate , "57600" ) == 0 ) {
		return B57600;
	} else if ( strcmp( p_private->mlc_baudrate , "115200" ) == 0 ) {
		return B115200;
	} else if ( strcmp( p_private->mlc_baudrate , "230400" ) == 0 ) {
		return B230400;
	} else if ( strcmp( p_private->mlc_baudrate , "460800" ) == 0 ) {
		return B460800;
	} else if ( strcmp( p_private->mlc_baudrate , "500000" ) == 0 ) {
		return B500000;
	} else if ( strcmp( p_private->mlc_baudrate , "576000" ) == 0 ) {
		return B576000;
	} else if ( strcmp( p_private->mlc_baudrate , "921600" ) == 0 ) {
		return B921600;
	} else if ( strcmp( p_private->mlc_baudrate , "1000000" ) == 0 ) {
		return B1000000;
	} else if ( strcmp( p_private->mlc_baudrate , "1152000" ) == 0 ) {
		return B1152000;
	} else if ( strcmp( p_private->mlc_baudrate , "2000000" ) == 0 ) {
		return B2000000;
	} else if ( strcmp( p_private->mlc_baudrate , "3000000" ) == 0 ) {
		return B3000000;
	} else if ( strcmp( p_private->mlc_baudrate , "4000000" ) == 0 ) {
		return B4000000;
	}
	return B115200;
}

static int S_Open_Device ( S_RS232 *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return -1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	p_private->i_reader_fd = open ( p_private->mlc_comport , O_RDWR | O_NOCTTY | O_NDELAY ) ;	// O_RDWR：讀寫開啟，NONBLOCK Mode
	if ( 0 > p_private->i_reader_fd ) {
		p_private->i_reader_fd = - 1 ;
		return p_private->i_reader_fd ;
	}

	struct termios options = { 0 };	//宣告一個設定 comport 所需要的結構體 並且清空內部
	int baudrate_Value = S_Get_baudrate ( obj ) ; // 取得 BaudRate

	/* c_cflag 控制模式：
	 * CLOCAL:忽略任何modem status lines
	 * CREAD:啟動字元接收
	 * CS8:傳送或接收時，使用八個位元
	 */
	options.c_cflag = (baudrate_Value | CLOCAL | CREAD | CS8); //依序,設定 baud rate,不改變 comport 擁有者, 接收致能, 8 data bits

	cfsetispeed( &options , baudrate_Value );
	cfsetospeed( &options , baudrate_Value );

	options.c_cc[ VTIME ] = 1;	//10 = 1秒,定義等待的時間，單位是百毫秒
	options.c_cc[ VMIN ] = 0;	//定義了要求等待的最小字節數,這個基本上給 0
	tcflush( p_private->i_reader_fd , TCIOFLUSH );	// 刷新和立刻寫進去fd

	if ( (tcsetattr( p_private->i_reader_fd , TCSANOW , &options )) == -1 ) { //寫回本機設備,TCSANOW >> 立刻改變數值
		return -2;
	}

	return D_success;	// success
}

static int S_Rtn_reader_fd ( S_RS232 *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return -1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	return p_private->i_reader_fd ;
}

static int S_Rtn_timeout ( S_RS232 *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return -1 ;
	}
	S_private_data *p_private = ( S_private_data * ) obj->i_private ;
	return p_private->i_timeout ;
}

/* 補齊 APDU Head & EDC
 * 1. info : APDU Information
 * 2. o_buf : 輸出的Buffer
 * 3. o_buf_sz：輸出Buffer的大小
 */
static int S_Apdu_string_to_hex ( CStr info , UStr o_buf , int o_buf_sz ) {	// 2018/01/02 有bug...//
	if ( ( ! info ) || ( ! o_buf ) || ( ! o_buf_sz ) ) {
		return - 1 ;
	}
	// NAD PCB LEN, SW1 SW2, EDC
//	if ( o_buf_sz < ( 3 + 4 + 1 ) ) {
//		return - 2 ;		// 資料長度不足 8
//	}
	memset ( o_buf , 0 , o_buf_sz ) ;

	char NAD_PCB [ ] = "0000" ;
	char LEN [ 2 + 1 ] = { 0 } ;
	int info_byte = strlen ( info ) >> 1 ; // information bytes數
	snprintf ( LEN , sizeof ( LEN ) , "%02X" , info_byte ) ;

	char input [ 6 + strlen ( info ) + 1 ] ;
	memset ( input , 0 , sizeof ( input ) ) ;
	snprintf ( input , sizeof ( input ) , "%s%s%s" , NAD_PCB , LEN , info ) ;

	int ct = ( int ) strlen ( input ) >> 1 ;	// Bytes 數
	int cot = 0 ;
	UChar edc = 0 ;
	char str [ 2 + 1 ] = { 0 } ;
	int i = 0 ;
	for ( i = 0 ; i < ct ; i ++ ) {
		memcpy ( str , input + ( i << 1 ) , 2 ) ;
		o_buf [ cot ++ ] = strtoul ( str , NULL , 16 ) ; // 將字串轉成Hex數字
		edc ^= o_buf [ cot - 1 ] ;		// XOR , EDC計算
	}
	o_buf [ cot ++ ] = edc ;
	return cot ;
}

// respon = 00 00 02 90 00 92
static int S_Only_string_to_hex ( CStr input , UStr o_buf , int o_buf_sz ) {
	if ( ( ! input ) || ( ! o_buf ) || ( ! o_buf_sz ) ) {
		return - 1 ;
	}
	// NAD PCB LEN, SW1 SW2, EDC
//	if ( o_buf_sz < ( 3 + 4 + 1 ) ) {		// 00 00 02 90 00 92
//		return - 2 ;		// 資料長度不足 8
//	}
	memset ( o_buf , 0 , o_buf_sz ) ;

	int ct = ( int ) strlen ( input ) >> 1 ;		// 資料Bytes 數
	int cot = 0 ;
	char str [ 2 + 1 ] = { 0 } ;
	int i = 0 ;
	for ( i = 0 ; i < ct ; i ++ ) {
		memcpy ( str , input + ( i << 1 ) , 2 ) ;
		o_buf [ cot ++ ] = strtoul ( str , NULL , 16 ) ; // 將字串轉成Hex數字
	}
	return cot ;	// 回吐Bytes
}



//static int S_RS232_Write ( S_RS232 *obj , CStr info , UChar *send_buf ) {
static int S_RS232_Write ( S_RS232 *obj , CStr info ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! info ) ) {
//		g_log->Save ( g_log , "Error S_RS232_Write_only , parameter error" ) ;
		return - 1 ;
	}
	S_private_data *p_private = obj->i_private ;
	if ( ( ! obj ) || ( p_private->i_reader_fd == - 1 ) || ( ! p_private->i_timeout ) || ( ! info ) || ( ( strlen ( info ) & 0x01 ) != 0 ) ) {
		return - 2 ;
	}

	S_Epoll *epoll_tool = S_Epoll_New ( ) ;
	if ( NULL == epoll_tool ) {
//		g_log->Save ( g_log , "Error S_Epoll_New error" ) ;
		return - 3 ;
	}
	int rtn = epoll_tool->Detect_hup ( epoll_tool , p_private->i_reader_fd ) ;
	S_Epoll_Delete ( & epoll_tool ) ;
	if ( rtn != 0 ) {
//		g_log->Save ( g_log , "Error , Detect_hup" ) ;
		return - 4 ;		//  斷線
	}

	int data_sz = strlen ( info ) >> 1 ; 	// data/2
	UChar u_buf [ data_sz ] ; 	// 輸出完整的Command
	memset ( u_buf , 0 , sizeof ( u_buf ) ) ;

	// APDU char to HEX：補APDU Head & EDC
	unsigned char send_buf [ 2048 ] = { 0x00 } ;
	rtn = S_Apdu_string_to_hex ( info , send_buf , sizeof ( send_buf ) ) ;
	if ( rtn <= 0 ) {
//		g_log->Save ( g_log , "S_Apdu_string_to_hex error, rtn:%d" , rtn ) ;
		return - 5 ;
	}

	rtn = write ( p_private->i_reader_fd , send_buf , rtn ) ;
	if ( rtn < 0 ) {
		return rtn ;
	}

	char cmd [ ( rtn << 1 ) + 1 ] ;
	memset ( cmd , 0 , sizeof ( cmd ) ) ;
	int ct = 0 ;
	for ( ct = 0 ; ct < rtn ; ct ++ ) {
		char tmp [ 2 + 1 ] = { 0 } ;
		snprintf ( tmp , sizeof ( tmp ) , "%02X" , send_buf [ ct ] ) ;
		strncat ( cmd , tmp , sizeof ( tmp ) ) ;
	}
//	g_log->Save ( g_log , "Write APDU command:%s" , cmd ) ;
	return rtn ; // 回傳寫出去的Bytes數量
}

static int S_RS232_Write_only ( S_RS232 *obj , CStr send ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! send ) ) {
//		g_log->Save ( g_log , "Error S_RS232_Write_only , parameter error" ) ;
		return - 1 ;
	}
	S_private_data *p_private = obj->i_private ;
	S_Epoll *epoll_tool = S_Epoll_New ( ) ;
	if ( NULL == epoll_tool ) {
//		g_log->Save ( g_log , "Error S_Epoll_New error" ) ;
		return - 2 ;
	}
	int rtn = epoll_tool->Detect_hup ( epoll_tool , p_private->i_reader_fd ) ;
	S_Epoll_Delete ( & epoll_tool ) ;
	if ( rtn != 0 ) {
//		g_log->Save ( g_log , "Error , Detect_hup" ) ;
		return - 3 ;		//  斷線
	}

	int data_sz = strlen ( send ) >> 1 ; 	// 除以2
	UChar u_buf [ data_sz ] ; 	// 輸出完整的Command
	memset ( u_buf , 0 , sizeof ( u_buf ) ) ;

	rtn = S_Only_string_to_hex ( send , u_buf , sizeof ( u_buf ) ) ;
	if ( rtn <= 0 ) {
//		g_log->Save ( g_log , "Error S_Only_string_to_hex error, rtn = %d" , rtn ) ;
		return - 5 ;
	}

	rtn = write ( p_private->i_reader_fd , u_buf , rtn ) ;
	if ( rtn < 0 ) {
//		g_log->Save ( g_log , "Error write error" ) ;
		return - 6 ;
	}

	char cmd [ ( rtn << 1 ) + 1 ] ;
	memset ( cmd , 0 , sizeof ( cmd ) ) ;
	int ct = 0 ;
	for ( ct = 0 ; ct < rtn ; ct ++ ) {
		char tmp [ 2 + 1 ] = { 0 } ;
		snprintf ( tmp , sizeof ( tmp ) , "%02X" , u_buf [ ct ] ) ;
		strncat ( cmd , tmp , sizeof ( tmp ) ) ;
	}
//	g_log->Save ( g_log , "Write APDU command:%s" , cmd ) ;
	return rtn ; // 回傳寫出去的Bytes數量
}


static int S_RS232_Read ( S_RS232 *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_private_data *p_private = obj->i_private ;

	S_Epoll *epoll_tool = S_Epoll_New ( ) ;
	if ( NULL == epoll_tool ) {
		return - 2 ;
	}
	int rtn = epoll_tool->Detect_hup ( epoll_tool , p_private->i_reader_fd ) ;
	S_Epoll_Delete ( & epoll_tool ) ;
	if ( rtn != 0 ) {
		return - 3 ;		//  斷線
	}

	time_t now_t = time ( NULL ) ;
	int rd_sz = 0 , ct = 0 ;
	UChar buf [ 1024 ] = { 0 } ;
	while ( p_private->i_timeout > ( time ( NULL ) - now_t ) ) {// Read Timeout（單位:秒）
		int rtn = read ( p_private->i_reader_fd , buf + rd_sz , sizeof ( buf ) - rd_sz ) ;
		usleep ( 1000 ) ;
		if ( 0 >= rtn ) {		// rtn = -1 ，錯誤
			if ( 0 < rd_sz ) {
				if ( ++ ct == 15 ) {
					break ; // 確認無其餘資料
				}
			}
			continue ;
		}
		rd_sz += rtn ;	// 資料量累計
	}

	p_private->read_data_sz = rd_sz << 1 ;
	int i = 0 ;

	memset ( p_private->i_read_data , 0 , sizeof ( p_private->i_read_data ) ) ;
	int respon_sz = sizeof ( p_private->i_read_data ) ;
	for ( i = 0 ; i < rd_sz ; ++ i ) {
		snprintf ( p_private->i_read_data + ( i << 1 ) , respon_sz - ( i << 1 ) , "%02X" , buf [ i ] ) ;// 輸出資料
	}

	return rd_sz ;
}

static Str Rtn_read_data ( S_RS232 *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_private_data *p_private = obj->i_private ;
	return p_private->i_read_data ;
}

static int S_Calculation_edc ( char *data , char *buf , int bufSz ) {
	if ( ( ! data ) || ( ! buf ) || ( ! bufSz ) ) {
		return -1 ;
	}
	// 計算資料長度是否為偶數
	if ( strlen ( data ) % 2 != 0 ) {
		printf ( "Data Len Error ! \n" ) ;
		return - 2 ;
	}

	//char *ptr = data ;
	int i = strlen ( data ) >> 1 ; // Bytes數
	int ct = 0 ;
	unsigned long result = 0 ;
	for ( ct = 0 ; ct < i ; ct ++ ) {
		char get_byte [ 2 + 1 ] = { 0 } ;
		snprintf ( get_byte , sizeof ( get_byte ) , "%s" , data ) ;

		unsigned long asd = strtoul ( get_byte , NULL , 16 ) ;
		data += 2 ;
		result = result ^ asd ; // XOR運算
	}
	snprintf ( buf , bufSz , "%02X" , ( unsigned int ) result ) ;// ############

	return D_success ;
}

static int S_Get_Information ( S_RS232 *obj , char *input , char **outputBody , int *sz ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! input ) ) {
		return - 1 ;
	}
	int Sz = ( int ) strlen ( input ) ;
	if ( Sz < ( DF_Prolog_Len + DF_Info_Len ) ) { // input資料長度不足Prolog的長度 (DF_Prolog + LEN)
		return - 2 ;
	}

	char *ptrS = strstr ( input , DF_Prolog ) ; // 尋找DF_Prolog "0000"
	char *checkptrS = strstr ( input , DF_Prolog ) ;
	if ( ( ptrS == NULL ) || ( checkptrS == NULL ) ) {
		return - 3 ;
	}

	ptrS += DF_Prolog_Len ;  // 移動到LEN的起始位置

	// 取得Information資料長度
	char lenBuf [ 2 + 1 ] = { 0 } ;
	snprintf ( lenBuf , sizeof ( lenBuf ) , "%s" , ptrS ) ;
	int dataSz = strtoul ( lenBuf , NULL , 16 ) ; // Information資料長度
	ptrS += DF_Info_Len ; //移動到Information的起始位置

	// 確認Info資料的長度
	int CheckDataSz = dataSz << 1 ;	// data * 2
	int expectSz = ( int ) strlen ( ptrS ) - 2 ;     // 預期的Info資料長度
	if ( CheckDataSz != expectSz ) {
		return - 4 ; 									// 資料長度錯誤
	}

	// Check EDC
	char willEdc [ 6 + CheckDataSz + 1 ] ;
	memset ( willEdc , 0 , sizeof ( willEdc ) ) ;
	snprintf ( willEdc , sizeof ( willEdc ) , "%s" , checkptrS ) ;

	//make edc
	char checkEDC [ 2 + 1 ] = { 0 } ;
	int rtn = S_Calculation_edc ( willEdc , checkEDC , sizeof ( checkEDC ) ) ;
	if ( rtn != D_success ) {
		return -5 ;
	}

	// 取得Respon的EDC
	checkptrS += 6 ;
	checkptrS += CheckDataSz ;
	char RspnEDC [ 2 + 1 ] = { 0 } ;
	snprintf ( RspnEDC , sizeof ( RspnEDC ) , "%s" , checkptrS ) ;

	if ( strcmp ( RspnEDC , checkEDC ) != 0 ) {  // 若EDC不同
		return - 6 ;
	}

	* sz = CheckDataSz ;
	* outputBody = calloc ( * sz + 1 , 1 ) ; // 動態記憶體
	if ( * outputBody == NULL ) {
		* sz = 0 ;
		* outputBody = NULL ;
		return - 7 ;
	}
	memcpy ( * outputBody , ptrS , * sz ) ;
	return D_success ;
}

S_RS232 * S_RS232_New ( void ) {
	S_RS232* p_tmp = ( S_RS232* ) calloc ( 1 , sizeof(S_RS232) ) ;
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

	p_tmp->Setup = S_RS232_Setup ;
	p_tmp->Open_device = S_Open_Device ;
	p_tmp->Rtn_reader_fd = S_Rtn_reader_fd ;
	p_tmp->Rtn_timeout = S_Rtn_timeout ;

	p_tmp->Write = S_RS232_Write ;
	p_tmp->Write_only = S_RS232_Write_only;

	p_tmp->Read = S_RS232_Read ;
	p_tmp->Rtn_read_data = Rtn_read_data ;

	p_tmp->Get_info = S_Get_Information ;

	return p_tmp ;
}

void S_RS232_Delete ( S_RS232** obj ) {
	S_private_data *p_private = ( * obj )->i_private ;
	if ( p_private->i_reader_fd > 0 ) {
		close ( p_private->i_reader_fd ) ;
	}

	free ( p_private->mlc_baudrate ) ;
	p_private->mlc_baudrate = NULL ;

	free ( p_private->mlc_comport ) ;
	p_private->mlc_comport = NULL ;

	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;
	free ( * obj ) ;
	* obj = NULL ;
	return ;
}
