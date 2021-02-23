/*
 * Ble.h
 *
 *  Created on: 2018年3月1日
 *      Author: richard
 */

#ifndef SRC_BLE_H_
#define SRC_BLE_H_

#include "Common_Tool.h"
#include "Epoll.h"			// 用於監控是否斷線
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/l2cap.h>

#define D_Scan	 0
#define D_Server 1
#define D_Client 1


// <<<< Scan >>>> //
#if D_Scan
typedef struct _S_Bluetooth S_Bluetooth ;
struct _S_Bluetooth {
	void *i_private ;
	int (*Bluetooth_open_device) ( S_Bluetooth* obj ) ;
	int (*Rtn_device_id) ( S_Bluetooth* obj ) ;
	int (*Rtn_device_handle) ( S_Bluetooth* obj ) ;
	int (*Bluetooth_start_scan) ( S_Bluetooth* obj ) ;
	int (*Bluetooth_stop_scan) ( S_Bluetooth* obj ) ;
	int (*Bluetooth_close_device) ( S_Bluetooth* obj ) ;
} ;
S_Bluetooth * S_Bluetooth_new ( void ) ;
void S_Bluetooth_Delete ( S_Bluetooth** obj ) ;
#endif

// <<<< Server >>>> //
#if D_Server
typedef struct _S_Ble_Server S_Ble_Server ;
struct _S_Ble_Server {
	void *i_private ;

	int (*Server_begin) ( S_Ble_Server *obj , UInt listen_num ) ;

	int (*Rtn_device_id) ( S_Ble_Server* obj ) ;
	int (*Rtn_device_handle) ( S_Ble_Server* obj ) ;
	int (*Rtn_srv_fd) ( S_Ble_Server *obj ) ;
	Str (*Rtn_connection_address) ( S_Ble_Server *obj ) ;

	int (*Get_local_address) ( S_Ble_Server *obj ) ;
	Str (*Rtn_local_address) ( S_Ble_Server *obj ) ;

	int (*Accept) ( S_Ble_Server *obj ) ;
	int (*Rtn_acpt_fd) ( S_Ble_Server *obj ) ;

	int (*Set_read_timeout) ( S_Ble_Server *obj , int timeout ) ;
	int (*Write) ( S_Ble_Server *obj , CStr str ) ;
	int (*Read) ( S_Ble_Server *obj , char **data , int *data_sz ) ;

	int (*Close) ( S_Ble_Server* obj ) ;
} ;
S_Ble_Server * S_Ble_Server_new ( void ) ;
void S_Ble_Server_Delete ( S_Ble_Server** obj ) ;
#endif

// <<<< Client >>>> //
#if D_Client
typedef struct _S_Ble_Client S_Ble_Client ;
struct _S_Ble_Client {
	void *i_private ;
	int (*Connect) ( S_Ble_Client *obj , CStr dest_addr ) ;
	int (*Rtn_device_id) ( S_Ble_Client *obj ) ;
	int (*Rtn_device_handle) ( S_Ble_Client *obj ) ;
	int (*Rtn_fd) ( S_Ble_Client *obj ) ;

	int (*Detect_connect) ( S_Ble_Client *obj ) ;

	int (*Set_read_timeout) ( S_Ble_Client *obj , int timeout ) ;

	int (*Write) ( S_Ble_Client *obj , CStr str ) ;
	int (*Read) ( S_Ble_Client *obj , char **data , int *data_sz ) ;

	int (*Close) ( S_Ble_Client *obj ) ;
} ;
S_Ble_Client * S_Ble_Client_new ( void ) ;
void S_Ble_Client_Delete ( S_Ble_Client** obj ) ;
#endif



#endif /* SRC_BLE_H_ */
