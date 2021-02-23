/*
 * Epoll.h
 *
 *  Created on: 2017/5/28
 *      Author: richard
 */

#ifndef EPOLL_H_
#define EPOLL_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "Common_Tool.h"


#if 1 // 2018/01/23
typedef enum EPOLL_EVENTS E_epl_evnt ;
typedef struct _S_Epoll S_Epoll ;
struct _S_Epoll {
	void *i_private ;
	void (*Setup_wait_time_ms) ( S_Epoll *obj , int ms ) ;	// 設定epoll wait時間
	void (*Setup_wait_num) ( S_Epoll *obj , int num ) ;		// 設定epoll wait數量

	int (*Create) ( S_Epoll *obj ) ;						// 建立epoll
	int (*Close) ( S_Epoll *obj ) ;							// 關閉 epoll

	int (*Rtn_epl_fd) ( S_Epoll *obj ) ;					// 回吐epoll fd
	int (*Rtn_epl_t) ( S_Epoll *obj ) ;						// 回吐epoll timeout
	int (*Rtn_epl_n) ( S_Epoll *obj ) ;						// 回吐epoll number

	int (*Wait_Epoll) ( S_Epoll *obj , void *i_events ) ;	// 2017/08/08, 2017/12/20

	int (*Add_EpollCtl) ( S_Epoll *obj , int fd , E_epl_evnt Mod ) ;	// Mod: EPOLLIN/EPOLLOUT/EPOLLHUP...
	int (*Mod_EpollCtl) ( S_Epoll *obj , int fd , E_epl_evnt Mod ) ;	// Mod: EPOLLIN/EPOLLOUT/EPOLLHUP...
	int (*Del_EpollCtl) ( S_Epoll *obj , int fd ) ;							// Mod: EPOLLIN/EPOLLOUT/EPOLLHUP...

	// 應用 //
	int (*Detect_hup) ( S_Epoll *obj , int fd ) ;			// 偵測是否斷線, return -1 is EPOLLHUP
} ;
extern S_Epoll *S_Epoll_New ( void ) ;
extern void S_Epoll_Delete ( S_Epoll **obj ) ;


#if 0
// 測試程式
int main() {
	int fd = 3 ;
	S_epoll *epoll_obj = S_Epoll_New ( ) ;
	int check = epoll_obj->Detect_hup ( epoll_obj , fd ) ;
	S_Epoll_Delete ( & epoll_obj ) ;
	if ( - 1 == check ) {
		return - 1 ;	// 斷線
	}
	return 0 ;
}
#endif
#endif

#if 0
typedef struct _S_Epoll S_Epoll ;
struct _S_Epoll {
	int i_epl_fd ;	// epoll fd
	int i_epl_t ;	// epoll_wait times
	int i_epl_n ;	// epoll_wait num

	void (*Epoll_Setup_wait_time_ms) ( S_Epoll*obj , int ms ) ;
	void (*Epoll_Setup_wait_num) ( S_Epoll*obj , int num ) ;
	int (*Epoll_CreateEpoll) ( S_Epoll*obj ) ;
	int (*Epoll_CloseEpoll) ( S_Epoll*obj ) ;
	int (*Epoll_WaitEpoll) ( S_Epoll*obj , void* iEvnts ) ;
	int (*Epoll_AddEpollCtl) ( S_Epoll*obj , int fd , enum EPOLL_EVENTS Mod ) ;	//Mod: EPOLLIN/EPOLLOUT/EPOLLHUP...
	int (*Epoll_ModEpollCtl) ( S_Epoll*obj , int fd , enum EPOLL_EVENTS Mod ) ;
	int (*Epoll_DelEpollCtl) ( S_Epoll*obj , int fd ) ;
} ;

extern S_Epoll * S_Epoll_New ( void ) ;
extern void S_Epoll_Delete ( S_Epoll** obj ) ;


// 用來判斷斷線用
int init_epoll ( int fd , enum EPOLL_EVENTS Mod ) ;
int epoll_wait_event ( struct epoll_event* events , int fd ) ;
#endif






#endif /* EPOLL_H_ */
