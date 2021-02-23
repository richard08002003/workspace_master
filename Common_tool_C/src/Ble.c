/*
 * Ble.c
 *
 *  Created on: 2018年3月1日
 *      Author: richard
 */
#include "Ble.h"




#if D_Scan
#define HCI_STATE_NONE       0
#define HCI_STATE_OPEN       1
#define HCI_STATE_SCANNING   2
#define HCI_STATE_FILTERING  3
// private data
struct S_bluetooth_private {
	inquiry_info *ii ;
	int device_id ;
	int device_handle ;

	int state ;

	struct hci_filter original_filter ;
	char error_message [ 10240 + 1 ] ;

	char device_address [ 18 + 1 ] ;
	char device_name [ 256 + 1 ] ;
} ;
static int S_Bluetooth_open_device ( S_Bluetooth* obj );
static int S_Rtn_device_id ( S_Bluetooth* obj );
static int S_Rtn_device_handle ( S_Bluetooth* obj );
static int S_Bluetooth_start_scan ( S_Bluetooth* obj );
static int S_Bluetooth_stop_scan ( S_Bluetooth* obj );
static int S_Bluetooth_close_device ( S_Bluetooth* obj );

// 開啟藍牙裝置
static int S_Bluetooth_open_device ( S_Bluetooth* obj ) {

	struct S_bluetooth_private *p_private = obj->i_private ;
	memset ( p_private->error_message , 0 , sizeof ( p_private->error_message ) ) ;

	p_private->device_id = hci_get_route ( NULL ) ;						// get device id
	if ( 0 > p_private->device_id ) {
		snprintf ( p_private->error_message , sizeof ( p_private->error_message ) , "Could not get device: %s" , strerror ( errno ) ) ;
		fprintf ( stderr , "%s" , p_private->error_message ) ;
		return D_fail ;
	}

	p_private->device_handle = hci_open_dev ( p_private->device_id ) ;	// open device
	if ( 0 > p_private->device_handle ) {
		snprintf ( p_private->error_message , sizeof ( p_private->error_message ) , "Could not open device: %s" , strerror ( errno ) ) ;
		fprintf ( stderr , "%s" , p_private->error_message ) ;
		return D_fail ;
	}

	// non-blocking mode
	int on = 1 ;
	if ( ioctl ( p_private->device_handle , FIONBIO , ( char * ) & on ) < 0 ) {
		snprintf ( p_private->error_message , sizeof ( p_private->error_message ) , "Could not open device: %s" , strerror ( errno ) ) ;
		fprintf ( stderr , "%s" , p_private->error_message ) ;
		return D_fail ;
	}

	p_private->state = HCI_STATE_OPEN ;
	return D_success ;
}

static int S_Rtn_device_id ( S_Bluetooth* obj ) {
	struct S_bluetooth_private *p_private = obj->i_private ;
	return p_private->device_id ;
}
static int S_Rtn_device_handle ( S_Bluetooth* obj ) {
	struct S_bluetooth_private *p_private = obj->i_private ;
	return p_private->device_handle ;
}

// 開始掃描
static int S_Bluetooth_start_scan ( S_Bluetooth* obj ) {

	struct S_bluetooth_private *p_private = obj->i_private ;
	memset ( p_private->error_message , 0 , sizeof ( p_private->error_message ) ) ;

	// 設定scan參數
	if ( hci_le_set_scan_parameters ( p_private->device_handle , 0x01 , htobs( 0x0010 ) , htobs( 0x0010 ) , 0x00 , 0x00 , 1000 ) < 0 ) {
		snprintf ( p_private->error_message , sizeof ( p_private->error_message ) , "Failed to set scan parameters: %s" , strerror ( errno ) ) ;
		fprintf ( stderr , "%s" , p_private->error_message ) ;
		return D_fail ;
	}

	// 開啟sacn
	if ( hci_le_set_scan_enable ( p_private->device_handle , 0x01 , 1 , 1000 ) < 0 ) {
		snprintf ( p_private->error_message , sizeof ( p_private->error_message ) , "Failed to enable scan: %s" , strerror ( errno ) ) ;
		fprintf ( stderr , "%s" , p_private->error_message ) ;
		return D_fail ;
	}

	p_private->state = HCI_STATE_SCANNING ;

	socklen_t olen = sizeof ( p_private->original_filter ) ;
	if ( getsockopt ( p_private->device_handle , SOL_HCI , HCI_FILTER , & p_private->original_filter , & olen ) < 0 ) {
		snprintf ( p_private->error_message , sizeof ( p_private->error_message ) , "Could not get socket options: %s" , strerror ( errno ) ) ;
		fprintf ( stderr , "%s" , p_private->error_message ) ;
		return D_fail ;
	}

	// Create and set the new filter
	struct hci_filter new_filter ;

	hci_filter_clear ( & new_filter ) ;
	hci_filter_set_ptype ( HCI_EVENT_PKT , & new_filter ) ;
	hci_filter_set_event ( EVT_LE_META_EVENT , & new_filter ) ;

	if ( setsockopt ( p_private->device_handle , SOL_HCI , HCI_FILTER , & new_filter , sizeof ( new_filter ) ) < 0 ) {
		snprintf ( p_private->error_message , sizeof ( p_private->error_message ) , "Could not set socket options: %s" , strerror ( errno ) ) ;
		fprintf ( stderr , "%s" , p_private->error_message ) ;
		return D_fail ;
	}
	p_private->state = HCI_STATE_FILTERING ;

	return D_success ;
}

// 停止掃描
static int S_Bluetooth_stop_scan ( S_Bluetooth* obj ) {

	struct S_bluetooth_private *p_private = obj->i_private ;
	memset ( p_private->error_message , 0 , sizeof ( p_private->error_message ) ) ;

	if ( p_private->state == HCI_STATE_FILTERING ) {
		p_private->state = HCI_STATE_SCANNING ;
		setsockopt ( p_private->device_handle , SOL_HCI , HCI_FILTER , & p_private->original_filter , sizeof ( p_private->original_filter ) ) ;
	}

	if ( hci_le_set_scan_enable ( p_private->device_handle , 0x00 , 1 , 1000 ) < 0 ) {
		snprintf ( p_private->error_message , sizeof ( p_private->error_message ) , "Disable scan failed: %s" , strerror ( errno ) ) ;
		fprintf ( stderr , "%s" , p_private->error_message ) ;
		return D_fail ;
	}

	return D_success ;
}

// 關閉藍牙裝置
static int S_Bluetooth_close_device ( S_Bluetooth* obj ) {
	struct S_bluetooth_private *p_private = obj->i_private ;
	hci_close_dev ( p_private->device_handle ) ;
	return D_success ;
}

S_Bluetooth * S_Bluetooth_new ( void ) {
	S_Bluetooth* p_tmp = ( S_Bluetooth* ) calloc ( 1 , sizeof(S_Bluetooth) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}
	p_tmp->i_private = ( struct S_bluetooth_private* ) calloc ( 1 , sizeof(struct S_bluetooth_private) ) ;
	if ( NULL == p_tmp->i_private ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}

	// private
	struct S_bluetooth_private* p_private = p_tmp->i_private;
	p_private->device_id = - 1;
	p_private->device_handle = - 1;
	p_private->state = - 1;
	memset ( &p_private->ii , 0 , sizeof(inquiry_info) );
	memset ( &p_private->original_filter , 0 , sizeof(struct hci_filter) );
	memset ( p_private->error_message , 0 , sizeof ( p_private->error_message ) );
	memset ( p_private->device_address , 0 , sizeof ( p_private->device_address ) );
	memset ( p_private->device_name , 0 , sizeof ( p_private->device_name ) );

	// method
	p_tmp->Bluetooth_open_device = S_Bluetooth_open_device;
	p_tmp->Rtn_device_id = S_Rtn_device_id;
	p_tmp->Rtn_device_handle = S_Rtn_device_handle;

	p_tmp->Bluetooth_start_scan = S_Bluetooth_start_scan;
	p_tmp->Bluetooth_start_scan = S_Bluetooth_stop_scan;
	p_tmp->Bluetooth_close_device = S_Bluetooth_close_device;

	return p_tmp ;
}
void S_Bluetooth_Delete ( S_Bluetooth** obj ) {
	struct S_bluetooth_private *p_private = ( * obj )->i_private ;
	p_private->device_handle = - 1 ;
	p_private->device_id = - 1 ;
	p_private->state = - 1 ;

	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;

	free ( * obj ) ;
	* obj = NULL ;

	return ;
}
#endif





// <<<< Server >>>> //
#if D_Server
// private data
typedef struct _S_ble_server_private {
	int i_device_id ;
	int i_device_handle ;

	char i_loacl_address[ 17 + 1 ];
	char i_connection_address [ 17 + 1 ] ;

	int i_server_fd ;
	int i_acpt_fd ;
	UInt i_listen_num ;
	int i_read_timeout ;
} S_ble_server_private ;

static int S_Server_begin ( S_Ble_Server *obj , UInt listen_num ) ;
static int S_Server_Rtn_device_id ( S_Ble_Server* obj ) ;
static int S_Server_Rtn_device_handle ( S_Ble_Server* obj ) ;
static int S_Server_Rtn_srv_fd ( S_Ble_Server *obj ) ;
static Str S_Server_Rtn_connection_address ( S_Ble_Server *obj ) ;
static int S_Server_Get_local_address( S_Ble_Server *obj ) ;
static Str S_Server_Rtn_local_address ( S_Ble_Server *obj ) ;
static int S_Server_Accept ( S_Ble_Server *obj ) ;
static int S_Server_Rtn_acpt_fd ( S_Ble_Server *obj ) ;
static int S_Server_Set_read_timeout ( S_Ble_Server *obj , int timeout ) ;
static int S_Server_Write ( S_Ble_Server *obj , CStr str ) ;
static int S_Server_Read ( S_Ble_Server *obj , char **data , int *data_sz ) ;
static int S_Server_Close ( S_Ble_Server* obj ) ;

static int S_Server_begin ( S_Ble_Server *obj , UInt listen_num ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! listen_num ) ) {
		return - 1 ;
	}

	S_ble_server_private *p_private = obj->i_private ;
	p_private->i_device_id = hci_get_route ( NULL ) ;						// get device id
	if ( 0 > p_private->i_device_id ) {
		return - 2 ;
	}

	p_private->i_device_handle = hci_open_dev ( p_private->i_device_id ) ;	// open device
	if ( 0 > p_private->i_device_handle ) {
		return - 3 ;
	}

	// non-blocking mode
	int on = 1 ;
	if ( ioctl ( p_private->i_device_handle , FIONBIO , ( char * ) & on ) < 0 ) {
		return - 4 ;
	}

	struct sockaddr_rc local_addr = { 0 } ;
	p_private->i_server_fd = socket ( AF_BLUETOOTH , SOCK_STREAM , BTPROTO_RFCOMM ) ;
	if ( p_private->i_server_fd < 0 ) {
		return - 5 ;
	}

	// 設定reuse bind addr
	int reuse = 1 ;
	if ( setsockopt ( p_private->i_server_fd , SOL_SOCKET , SO_REUSEADDR , & reuse , sizeof ( reuse ) ) < 0 ) {
		obj->Close ( obj ) ;
		return - 6 ;
	}

	local_addr.rc_family = AF_BLUETOOTH ;
	local_addr.rc_bdaddr = * BDADDR_ANY ;
	local_addr.rc_channel = ( uint8_t ) 1 ;
	int rtn = bind ( p_private->i_server_fd , ( struct sockaddr * ) & local_addr , sizeof ( local_addr ) ) ;
	if ( rtn < 0 ) {
		obj->Close ( obj ) ;
		return - 7 ;
	}

	// put socket into listening mode
	rtn = listen ( p_private->i_server_fd , listen_num ) ;
	if ( rtn < 0 ) {
		obj->Close ( obj ) ;
		return - 8 ;
	}
	p_private->i_listen_num = listen_num ;

	return D_success ;	// success
}

static int S_Server_Rtn_device_id ( S_Ble_Server* obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_ble_server_private *p_private = obj->i_private ;
	return p_private->i_device_id ;
}

static int S_Server_Rtn_device_handle ( S_Ble_Server* obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_ble_server_private *p_private = obj->i_private ;
	return p_private->i_device_handle ;
}

static int S_Server_Rtn_srv_fd ( S_Ble_Server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_ble_server_private *p_private = obj->i_private ;
	return p_private->i_server_fd ;
}

static int S_Server_Get_local_address( S_Ble_Server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_ble_server_private *p_private = obj->i_private ;

	// Get local address
	struct hci_dev_info device_info = { 0 } ;
	hci_devinfo ( p_private->i_device_id , &device_info );
 	ba2str ( & device_info.bdaddr , p_private->i_loacl_address );

	return D_success ;	// success
}

static Str S_Server_Rtn_local_address ( S_Ble_Server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_ble_server_private *p_private = obj->i_private ;
	return p_private->i_loacl_address ;
}

static Str S_Server_Rtn_connection_address ( S_Ble_Server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return NULL ;
	}
	S_ble_server_private *p_private = obj->i_private ;
	return p_private->i_connection_address ;
}

static int S_Server_Accept ( S_Ble_Server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}

	S_ble_server_private *p_private = obj->i_private ;
	struct sockaddr_rc remote_addr = { 0 } ;
	socklen_t opt = sizeof ( remote_addr ) ;
	p_private->i_acpt_fd = accept ( p_private->i_server_fd , ( struct sockaddr * ) & remote_addr , & opt );
	if ( p_private->i_acpt_fd < 0 ) {
		return -4 ;
	}
	char connection_addr [ 1024 ] = { 0 } ;
	ba2str ( & remote_addr.rc_bdaddr , connection_addr );
	snprintf ( p_private->i_connection_address , sizeof ( p_private->i_connection_address ) , "%s" , connection_addr ) ;

	// 設定 timeout
	struct timeval set_time = { 0 } ;
	UInt write_ms = 10 ;
	UInt read_ms = 1 ;
	set_time.tv_sec = write_ms / 1000 ;
	set_time.tv_usec = ( write_ms - ( set_time.tv_sec * 1000 ) ) * 1000 ;
	if ( 0 != setsockopt ( p_private->i_acpt_fd , SOL_SOCKET , SO_SNDTIMEO , ( char * ) & set_time , sizeof(struct timeval) ) ) {
		return - 3 ;
	}

	set_time.tv_sec = read_ms / 1000 ;
	set_time.tv_usec = ( read_ms - ( set_time.tv_sec * 1000 ) ) * 1000 ;
	if ( 0 != setsockopt ( p_private->i_acpt_fd , SOL_SOCKET , SO_RCVTIMEO , ( char * ) & set_time , sizeof(struct timeval) ) ) {
		return - 4 ;
	}

	return D_success ;
}

static int S_Server_Rtn_acpt_fd ( S_Ble_Server *obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_ble_server_private *p_private = obj->i_private ;
	return p_private->i_acpt_fd ;
}

static int S_Server_Set_read_timeout ( S_Ble_Server *obj , int timeout ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_ble_server_private *p_private = obj->i_private ;
	p_private->i_read_timeout = timeout ;
	return D_success ;
}

static int S_Server_Write ( S_Ble_Server *obj , CStr str ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! str ) ) {
		return - 1 ;
	}
	S_ble_server_private *p_private = obj->i_private ;
	// 偵測Client是否斷線，斷線則不讀
	S_Epoll *epoll_obj = S_Epoll_New ( ) ;
	int check = epoll_obj->Detect_hup ( epoll_obj , p_private->i_acpt_fd ) ;
	S_Epoll_Delete ( & epoll_obj ) ;
	if ( - 1 == check ) {
		return - 2 ;	// 斷線
	}

	// 加Head與資料長度 <S> + 08X + JsonData	// 2018/01/11
	char will_send_buf[ 3 + 8 + (int) strlen( str ) + 1 ];
	memset( will_send_buf , 0 , sizeof(will_send_buf) );
	snprintf ( will_send_buf , sizeof(will_send_buf) , "<S>%08X%s" , (UInt)strlen( str ) , str ) ;
	int rtn = write ( p_private->i_acpt_fd , will_send_buf , strlen ( will_send_buf ) ) ;
	if ( - 1 == rtn ) {
		return - 3 ;
	}
	return D_success ;
}

static int S_Server_Read ( S_Ble_Server *obj , char **data , int *data_sz ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_ble_server_private *p_private = obj->i_private ;
	// 偵測Client是否斷線，斷線則不讀
	S_Epoll *epoll_obj = S_Epoll_New ( ) ;
	int check = epoll_obj->Detect_hup ( epoll_obj , p_private->i_acpt_fd ) ;
	S_Epoll_Delete ( & epoll_obj ) ;
	if ( - 1 == check ) {
		return - 1 ;	// 斷線
	}

	* data = NULL ;	// 存放讀取資料的指標
	* data_sz = 0 ;				// 大小
	time_t now_t = time ( NULL ) ;
	while ( p_private->i_read_timeout > ( time ( NULL ) - now_t ) ) {
		char buf [ 8192 + 1 ] = { 0 } ;
		int rtn = read ( p_private->i_acpt_fd , buf , sizeof ( buf ) ) ;
		if ( rtn == - 1 ) {
			if ( * data_sz > 0 ) {	// 確認資料已經收完，沒有資料了
				break ;
			}
			continue ;
		} else if ( rtn == 0 ) {	// 錯誤
			return - 2 ;
		}

		* data = ( char * ) realloc ( * data , * data_sz + rtn + 1 ) ;
		memcpy ( * data + * data_sz , buf , rtn ) ;	// 將資料接到上筆資料量後面
		( * data ) [ * data_sz + rtn ] = 0 ;
		* data_sz += rtn ;	// 累計資料量
	}
	return D_success ;
}

static int S_Server_Close ( S_Ble_Server* obj ) {
	S_ble_server_private *p_private = obj->i_private ;
	if ( - 1 != p_private->i_server_fd ) {
		close ( p_private->i_server_fd ) ;
		p_private->i_server_fd = - 1 ;
	}
	hci_close_dev ( p_private->i_device_handle ) ;
	return D_success ;
}

S_Ble_Server * S_Ble_Server_new ( void ) {
	S_Ble_Server *p_tmp = ( S_Ble_Server* ) calloc ( 1 , sizeof(S_Ble_Server) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}
	S_ble_server_private *i_private_data = ( S_ble_server_private* ) calloc ( 1 , sizeof(S_ble_server_private) ) ;
	if ( NULL == i_private_data ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}

	// private
	i_private_data->i_device_id = - 1 ;
	i_private_data->i_device_handle = - 1 ;
	memset ( i_private_data->i_loacl_address , 0 , sizeof ( i_private_data->i_loacl_address ) ) ;
	memset ( i_private_data->i_connection_address , 0 , sizeof ( i_private_data->i_connection_address ) ) ;
	i_private_data->i_server_fd = - 1 ;
	i_private_data->i_acpt_fd = - 1 ;
	i_private_data->i_listen_num = 256 ;
	i_private_data->i_read_timeout = 2 ;
	p_tmp->i_private = ( void* ) i_private_data ;

	// method
	p_tmp->Server_begin = S_Server_begin ;
	p_tmp->Rtn_device_id = S_Server_Rtn_device_id ;
	p_tmp->Rtn_device_handle = S_Server_Rtn_device_handle ;
	p_tmp->Rtn_srv_fd = S_Server_Rtn_srv_fd ;
	p_tmp->Rtn_connection_address = S_Server_Rtn_connection_address ;

	p_tmp->Get_local_address = S_Server_Get_local_address ;
	p_tmp->Rtn_local_address = S_Server_Rtn_local_address ;

	p_tmp->Accept = S_Server_Accept ;
	p_tmp->Rtn_acpt_fd = S_Server_Rtn_acpt_fd ;

	p_tmp->Set_read_timeout = S_Server_Set_read_timeout ;

	p_tmp->Write = S_Server_Write ;
	p_tmp->Read = S_Server_Read ;
	p_tmp->Close = S_Server_Close ;

	return p_tmp ;
}
void S_Ble_Server_Delete ( S_Ble_Server** obj ) {
	( * obj )->Close ( * obj ) ;
	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;
	free ( * obj ) ;
	* obj = NULL ;
	return ;
}
#endif




// <<<< Client >>>> //
#if D_Client
// private data
typedef struct _S_ble_client_private {
	int i_device_id ;
	int i_device_handle ;
	int i_fd ;
	int i_read_timeout ;

} S_ble_client_private ;

static int S_Client_Connect ( S_Ble_Client *obj , CStr dest_addr ) ;
static int S_Client_Rtn_device_id ( S_Ble_Client* obj ) ;
static int S_Client_Rtn_device_handle ( S_Ble_Client* obj ) ;
static int S_Client_Rtn_fd ( S_Ble_Client* obj ) ;

static int S_Client_Detect_connect ( S_Ble_Client*obj ) ;

static int S_Client_Set_read_timeout ( S_Ble_Client *obj , int timeout ) ;
static int S_Client_Write ( S_Ble_Client*obj , CStr str ) ;
static int S_Client_Read ( S_Ble_Client *obj , char **data , int *data_sz ) ;
static int S_Client_Close ( S_Ble_Client* obj ) ;

static int S_Client_Connect ( S_Ble_Client *obj , CStr dest_addr ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! dest_addr ) ) {
		return - 1 ;
	}
	S_ble_client_private *p_private = obj->i_private ;
	p_private->i_device_id = hci_get_route ( NULL ) ;						// get device id
	if ( 0 > p_private->i_device_id ) {
		return - 2 ;
	}

	p_private->i_device_handle = hci_open_dev ( p_private->i_device_id ) ;	// open device
	if ( 0 > p_private->i_device_handle ) {
		return - 3 ;
	}

	// non-blocking mode
	int on = 1 ;
	if ( ioctl ( p_private->i_device_handle , FIONBIO , ( char * ) & on ) < 0 ) {
		return - 4 ;
	}

	p_private->i_fd = socket ( AF_BLUETOOTH , SOCK_STREAM , BTPROTO_RFCOMM ) ;
	if ( p_private->i_fd < 0 ) {
		return - 5 ;
	}

	struct sockaddr_rc addr = { 0 } ;
	addr.rc_family = AF_BLUETOOTH ;
	addr.rc_channel = ( uint8_t ) 1 ;
	str2ba ( dest_addr , & addr.rc_bdaddr ) ;	// 將欲連結的mac address轉成struct bdaddr_t

	int rtn = connect ( p_private->i_fd , ( struct sockaddr * ) & addr , sizeof ( addr ) ) ;
	if ( 0 == rtn ) {
		return D_success ;
	}
	obj->Close ( obj ) ;
	return - 8 ;
}
static int S_Client_Rtn_device_id ( S_Ble_Client* obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_ble_client_private *p_private = obj->i_private ;
	return p_private->i_device_id ;
}
static int S_Client_Rtn_device_handle ( S_Ble_Client* obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_ble_client_private *p_private = obj->i_private ;
	return p_private->i_device_handle ;
}
static int S_Client_Rtn_fd ( S_Ble_Client* obj ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_ble_client_private *p_private = obj->i_private ;
	return p_private->i_fd ;
}
static int S_Client_Detect_connect ( S_Ble_Client *obj ) {
	S_ble_client_private *p_private = obj->i_private ;
	// 偵測Client是否斷線，斷線則不讀
	S_Epoll *epoll_obj = S_Epoll_New ( ) ;
	int check = epoll_obj->Detect_hup ( epoll_obj , p_private->i_fd ) ;
	S_Epoll_Delete ( & epoll_obj ) ;
	if ( - 1 == check ) {
		obj->Close ( obj ) ;
		return p_private->i_fd ;	// IPC斷線
	}
	return p_private->i_fd ;
}

static int S_Client_Set_read_timeout ( S_Ble_Client *obj , int timeout ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_ble_client_private *p_private = obj->i_private ;
	p_private->i_read_timeout = timeout ;
	return D_success ;
}
static int S_Client_Write ( S_Ble_Client*obj , CStr str ) {
	if ( ( ! obj ) || ( ! obj->i_private ) || ( ! str ) ) {
fprintf( stdout , "!!!!!! S_Client_Write ( ! obj ) || ( ! obj->i_private ) || ( ! str )\n");
fflush ( stdout ) ;
		return - 1 ;
	}
	S_ble_client_private *p_private = obj->i_private ;
	// 偵測Client是否斷線，斷線則不讀
	S_Epoll *epoll_obj = S_Epoll_New ( ) ;
	int check = epoll_obj->Detect_hup ( epoll_obj , p_private->i_fd ) ;
	S_Epoll_Delete ( & epoll_obj ) ;
	if ( - 1 == check ) {
fprintf( stdout , "!!!!!! S_Client_Write - 1 == check\n");
fflush ( stdout ) ;
		return - 2 ;	// 斷線
	}

	// 加Head與資料長度 <S> + 08X + JsonData
	char will_send_buf[ 3 + 8 + (int) strlen( str ) + 1 ];
	memset( will_send_buf , 0 , sizeof(will_send_buf) );
	snprintf ( will_send_buf , sizeof(will_send_buf) , "<S>%08X%s" , (UInt)strlen( str ) , str ) ;
	int rtn = write ( p_private->i_fd , will_send_buf , strlen ( will_send_buf ) ) ;
	if ( - 1 == rtn ) {
fprintf( stdout , "!!!!!! S_Client_Write write - 1 == rtn \n");
fflush ( stdout ) ;
		return - 3 ;
	}
	return rtn ;
}
static int S_Client_Read ( S_Ble_Client *obj , char **data , int *data_sz ) {
	if ( ( ! obj ) || ( ! obj->i_private ) ) {
		return - 1 ;
	}
	S_ble_client_private *p_private = obj->i_private ;
	// 偵測Client是否斷線，斷線則不讀
	S_Epoll *epoll_obj = S_Epoll_New ( ) ;
	int check = epoll_obj->Detect_hup ( epoll_obj , p_private->i_fd ) ;
	S_Epoll_Delete ( & epoll_obj ) ;
	if ( - 1 == check ) {
		return - 1 ;	// 斷線
	}
	* data = NULL ;	// 存放讀取資料的指標
	* data_sz = 0 ;				// 大小
	time_t now_t = time ( NULL ) ;
	while ( p_private->i_read_timeout > ( time ( NULL ) - now_t ) ) {
		char buf [ 8192 + 1 ] = { 0 } ;
		int rtn = recv ( p_private->i_fd , buf , sizeof ( buf ) , 0 ) ;
		if ( rtn == - 1 ) {
			if ( * data_sz > 0 ) {	// 確認資料已經收完，沒有資料了
				break ;
			}
			continue ;
		} else if ( rtn == 0 ) {	// 錯誤
			return - 2 ;
		}

		* data = ( char * ) realloc ( * data , * data_sz + rtn + 1 ) ;
		memcpy ( * data + * data_sz , buf , rtn ) ;	// 將資料接到上筆資料量後面
		( * data ) [ * data_sz + rtn ] = 0 ;
		* data_sz += rtn ;	// 累計資料量
	}
	return * data_sz ;
}
static int S_Client_Close ( S_Ble_Client* obj ) {
	S_ble_client_private *p_private = obj->i_private ;
	if ( - 1 != p_private->i_fd ) {
		close ( p_private->i_fd ) ;
		p_private->i_fd = - 1 ;
	}
	hci_close_dev ( p_private->i_device_handle ) ;
	return D_success ;
}

S_Ble_Client * S_Ble_Client_new ( void ) {
	S_Ble_Client* p_tmp = ( S_Ble_Client* ) calloc ( 1 , sizeof(S_Ble_Client) ) ;
	if ( NULL == p_tmp ) {
		return NULL ;
	}
	S_ble_client_private *i_private_data = ( S_ble_client_private* ) calloc ( 1 , sizeof(S_ble_client_private) ) ;
	if ( NULL == i_private_data ) {
		free ( p_tmp ) ;
		p_tmp = NULL ;
		return NULL ;
	}

	// private
	i_private_data->i_device_id = - 1 ;
	i_private_data->i_device_handle = - 1 ;
 	i_private_data->i_fd = - 1 ;
	i_private_data->i_read_timeout = 2 ;
	p_tmp->i_private = ( void* ) i_private_data ;

	// method
	p_tmp->Connect = S_Client_Connect ;
	p_tmp->Rtn_device_id = S_Client_Rtn_device_id ;
	p_tmp->Rtn_device_handle = S_Client_Rtn_device_handle ;
	p_tmp->Rtn_fd = S_Client_Rtn_fd ;

	p_tmp->Detect_connect = S_Client_Detect_connect ;

	p_tmp->Set_read_timeout = S_Client_Set_read_timeout ;
	p_tmp->Write = S_Client_Write ;
	p_tmp->Read = S_Client_Read ;
	p_tmp->Close = S_Client_Close ;

	return p_tmp ;
}
void S_Ble_Client_Delete ( S_Ble_Client** obj ) {
	( * obj )->Close ( * obj ) ;
	free ( ( * obj )->i_private ) ;
	( * obj )->i_private = NULL ;
	free ( * obj ) ;
	* obj = NULL ;
	return ;
}
#endif
